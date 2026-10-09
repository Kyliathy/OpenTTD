/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_budget.cpp Representative large-map sparse storage and bounded active-job work measurements. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 18; r2mlvs15-integrated-acceptance-and-balance.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include <chrono>
#include <iostream>

#include "../safeguards.h"

/** Read-only actual support-floor signal; this does not emulate a train controller or material inventory. */
class BudgetSupport final : public ExcavationWorkParticipant {
public:
	/**
	 * Validate actual floor support for one real accepted job.
	 * @param job Exact company-owned geometry.
	 * @param cut Actual next cut, without path search or material mutation.
	 * @return Whether its owner and backing floor are still present.
	 */
	bool IsSupported(const ExcavationJob &job, const ExcavationCutGeometry &cut) const override
	{
		if (!Company::IsValidID(job.company)) return false;
		for (TileIndex tile : cut.support_tiles) {
			if (GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated) return false;
		}
		return true;
	}
};

/**
 * Supply one actual bounded forward pass on an explicitly supported starter room.
 * @param anchor Actual open cutter centre on the Underground plane.
 * @return One canonical complete 3x3 advancing cut with explicit support and authorization.
 */
static ExcavationJob BudgetJob(TileIndex anchor)
{
	ExcavationJob job;
	job.company = _current_company; job.model_id = 0; job.commissioning_anchor = anchor;
	const auto next = TileAddWrap(anchor, 1, 0);
	job.requested_tiles = {next};
	ExcavationCutGeometry cut{next, Direction::SW, {}, {anchor}};
	// The actual target cutter footprint is fully authorized; no neighbouring solid floor is silently opened by the benchmark.
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) cut.swept_tiles.push_back(TileAddWrap(next, x, y));
	}
	job.authorized_tiles = cut.swept_tiles;
	job.route.push_back(std::move(cut));
	return job;
}

/** Storage and tick work scale with explicit packed planes/bounded jobs; timings are evidence, never flaky pass thresholds. */
TEST_CASE("Excavation large-map storage and active job work budgets", "[excavation-runtime-budget]")
{
	using Clock = std::chrono::steady_clock;
	// Representative square/rectangular worlds exercise different plane sizes; max-map packed payload is measured separately.
	for (const auto [width, height] : {std::pair{512U, 512U}, std::pair{1024U, 512U}, std::pair{1024U, 1024U}}) {
		ExcavationFixture fixture;
		ResetGeologyWorldState();
		Map::Allocate(width, height, 4);
		for (auto layer : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space, MapLayer::DeepUnderground}) MultiLayerMap::InitialiseLayer(layer);
		const auto start = Clock::now();
		REQUIRE(GenerateWorldGeology(420, 0) == GeologyError::None); // Preserve the frozen uniform-rock economics baseline.
		const auto generated = Clock::now();
		std::vector<TileIndex> starters;
		std::vector<TileIndex> anchors;
		// 128 spatially separate real reservations reach the exact sparse job bound without sharing work cells.
		for (uint i = 0; i < MAX_EXCAVATION_JOBS; ++i) {
			const auto anchor = TileXYL(64 + (i % 16) * 8, 64 + (i / 16) * 8, MapLayer::Underground);
			anchors.push_back(anchor);
			for (int y = -1; y <= 1; ++y) {
				for (int x = -1; x <= 1; ++x) starters.push_back(TileAddWrap(anchor, x, y));
			}
		}
		fixture.Open(starters);
		std::vector<ExcavationJobID> ids;
		for (TileIndex anchor : anchors) {
			const auto reserved = ReserveExcavationJob(BudgetJob(anchor), true);
			REQUIRE(reserved.error == ExcavationError::None);
			ids.push_back(reserved.id);
		}
		BudgetSupport participant;
		const auto work_start = Clock::now();
		bool accepted = true;
		// Exercise synchronized per-job work, not planning/save/render scans: each call performs only bounded identity/support work.
		for (uint round = 0; round < 64; ++round) {
			for (auto id : ids) accepted &= AdvanceExcavationWork(id, 1, participant) == ExcavationError::None;
		}
		const auto worked = Clock::now();
		CHECK(accepted);
		CHECK(GetExcavationJob(ids.back())->partial_work_ticks == 64);
		CHECK(GetGeologyWorldState().jobs.size() == MAX_EXCAVATION_JOBS);
		const auto lookup_start = Clock::now();
		uint found = 0;
		// A fixed query count/sequence measures rebuilt bucket lookup independently of map area and global RNG.
		for (uint i = 0; i < 10000; ++i) {
			found += GetGeologyWorldState().deposits.Find(TileXYL(1 + (i * 31) % (width - 2), 1 + (i * 17) % (height - 2), MapLayer::Underground)) != GeologyDepositID{UINT32_MAX};
		}
		const auto looked_up = Clock::now();
		const auto &state = GetGeologyWorldState();
		CHECK(state.terrain.StorageBytes() == static_cast<size_t>(width) * height / 2);
		std::cout << "Excavation runtime budget: map=" << width << 'x' << height
			<< " packed_bytes=" << state.terrain.StorageBytes() << " deposits=" << state.deposits.Records().size()
			<< " index_payload_bytes=" << state.deposits.IndexPayloadBytes() << " jobs=" << ids.size()
			<< " claimed_cells=" << state.job_claims.size() << " job_record_bytes=" << sizeof(ExcavationJob)
			<< " generation_us=" << std::chrono::duration_cast<std::chrono::microseconds>(generated - start).count()
			<< " work_8192_us=" << std::chrono::duration_cast<std::chrono::microseconds>(worked - work_start).count()
			<< " lookup_10000_us=" << std::chrono::duration_cast<std::chrono::microseconds>(looked_up - lookup_start).count()
			<< " lookup_hits=" << found << '\n';
	}
}
