/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_budget.cpp Representative large-map sparse storage and bounded active-job work measurements. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 18; r2mlvs16-integrated-acceptance-and-balance.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "ground_job_fixture.h"
#include <chrono>
#include <iostream>

#include "../safeguards.h"

/** Repeated real bounded owner work;no cut,cargo or cash commit is represented by this isolated participant. */
TEST_CASE("Dense ground owner work and reservation estimates expose repeatable samples", "[.ground-job-performance]")
{
	const uint width = GroundJobArgument("OTTD_GROUND_STRESS_AXIS"), count = GroundJobArgument("OTTD_GROUND_STRESS_JOBS");
	REQUIRE((count == 0 || count == 32 || count == 128));
	ExcavationFixture world;
	const auto setup = GroundJobNow(); const auto ids = PrepareGroundJobs(world, width, count); const auto setup_ns = GroundJobNow() - setup;
	BudgetSupport participant; bool accepted = true;
	const auto money = Company::Get(_current_company)->money;
	for (uint interval = 0; interval < 6; ++interval) {
		for (uint round = 0; round < 64; ++round) {
			const auto begin = GroundJobNow();
			for (auto id : ids) accepted &= AdvanceExcavationWork(id, 1, participant) == ExcavationError::None;
			const auto elapsed = GroundJobNow() - begin;
			if (interval != 0) fmt::print("GROUND_JOB_SAMPLE operation=work index={} ns={} calls={}\n", (interval - 1) * 64 + round, elapsed, count);
		}
	}
	REQUIRE(accepted);
	for (auto id : ids) {
		const auto *job = GetExcavationJob(id); REQUIRE(job != nullptr);
		CHECK(job->partial_work_ticks == 384); CHECK(job->cursor == 0);
		CHECK(job->completed_earth_t == 0); CHECK(job->completed_rock_t == 0); CHECK(job->completed_work_cost == Money{0});
	}
	CHECK(Company::Get(_current_company)->money == money);
	const auto candidate_anchor = TileXYL(400, 400, MapLayer::Underground);
	std::vector<TileIndex> support;
	for (int y = -1; y <= 1; ++y) for (int x = -1; x <= 1; ++x) support.push_back(TileAddWrap(candidate_anchor, x, y));
	world.Open(support); const auto proposal = BudgetJob(candidate_anchor);
	const auto expected = count == MAX_EXCAVATION_JOBS ? ExcavationError::Limit : ExcavationError::None;
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	for (uint interval = 0; interval < 6; ++interval) {
		for (uint round = 0; round < 64; ++round) {
			const auto begin = GroundJobNow(); const auto quote = ReserveExcavationJob(proposal, false); const auto elapsed = GroundJobNow() - begin;
			REQUIRE(quote.error == expected);
			if (interval != 0) fmt::print("GROUND_JOB_SAMPLE operation=reserve-estimate index={} ns={} calls=1\n", (interval - 1) * 64 + round, elapsed);
		}
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before); CHECK(Company::Get(_current_company)->money == money);
	for (uint interval = 0; interval < 6; ++interval) {
		const auto begin = GroundJobNow(); uint hits = 0;
		for (uint i = 0; i < 10000; ++i) hits += GetGeologyWorldState().deposits.Find(TileXYL(1 + (i * 31) % (width - 2), 1 + (i * 17) % (width - 2), MapLayer::Underground)) != GeologyDepositID{UINT32_MAX};
		const auto elapsed = GroundJobNow() - begin;
		if (interval != 0) fmt::print("GROUND_JOB_LOOKUP index={} ns={} queries=10000 hits={}\n", interval - 1, elapsed, hits);
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	const auto &state = GetGeologyWorldState();
	fmt::print("GROUND_JOB_COMPLETE width={} height={} jobs={} claims={} warmup=1 intervals=5 rounds=64 partial_ticks=384 generated_earth=0 generated_rock=0 cash_changed=0 setup_ns={} terrain_bytes={} index_payload_bytes={} job_payload_estimate={} map_packed_payload_estimate={} deposits={} reserve_refused={}\n",
		width, width, count, state.job_claims.size(), setup_ns, state.terrain.StorageBytes(), state.deposits.IndexPayloadBytes(), GroundJobPayload(), uint64_t{Map::Size()} * 12,
		state.deposits.Records().size(), expected == ExcavationError::Limit ? 1 : 0);
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
