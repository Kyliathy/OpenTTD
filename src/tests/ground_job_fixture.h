/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file ground_job_fixture.h Explicit prepared geological owner workloads,not actual train or bootstrap substitutes.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group21DC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_GROUND_JOB_FIXTURE_H
#define TESTS_GROUND_JOB_FIXTURE_H
#include "excavation_fixture.h"
#include "../core/format.hpp"
#include "../core/string_consumer.hpp"
#include <chrono>

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
inline ExcavationJob BudgetJob(TileIndex anchor)
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


/** @return Steady nanoseconds for observation,never simulation progress. */
inline uint64_t GroundJobNow()
{
 return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

/** @param name Explicit native workload selector. @return Validated nonnegative integer;no implicit latest/default workload. */
inline uint GroundJobArgument(const char *name)
{
 const char *value = std::getenv(name); REQUIRE(value != nullptr);
 const auto number = ParseInteger<uint>(value); REQUIRE(number.has_value());
 return *number;
}

/** @param world Real isolated profile owner. @param width Explicit map axis. @param count Actual reserved job count. @param hardness Explicit geological generator contract version. @return Issued live job identities. */
inline std::vector<ExcavationJobID> PrepareGroundJobs(ExcavationFixture &world, uint width, uint count, uint16_t hardness = 0)
{
 REQUIRE((width == 512 || width == 1024 || width == 2048)); REQUIRE(count <= MAX_EXCAVATION_JOBS);
 ResetGeologyWorldState(); Map::Allocate(width, width, 4);
 // Initialize every physical plane without simulation RNG or camera-dependent work.
 for (auto layer : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space, MapLayer::DeepUnderground}) MultiLayerMap::InitialiseLayer(layer);
 REQUIRE(GenerateWorldGeology(420, hardness) == GeologyError::None);
 std::vector<TileIndex> floor, anchors;
 // Fixed eight-tile spacing keeps every explicit starter room independent in the dense workload.
 for (uint i = 0; i < count; ++i) {
  const auto anchor = TileXYL(64 + (i % 16) * 8, 64 + (i / 16) * 8, MapLayer::Underground); anchors.push_back(anchor);
  for (int y = -1; y <= 1; ++y) for (int x = -1; x <= 1; ++x) floor.push_back(TileAddWrap(anchor, x, y));
 }
 if (!floor.empty()) world.Open(floor);
 std::vector<ExcavationJobID> ids;
 // Reserve each actual accepted geometry through the native geological owner.
 for (auto anchor : anchors) {
  const auto result = ReserveExcavationJob(BudgetJob(anchor), true);
  REQUIRE(result.error == ExcavationError::None); ids.push_back(result.id);
 }
 REQUIRE(GetGeologyWorldState().jobs.size() == count);
 return ids;
}

/** @return Retained actual vector/scalar payload estimate;map-node/allocator overhead and temporary copies require OS peaks. */
inline size_t GroundJobPayload()
{
 const auto &state = GetGeologyWorldState();
 size_t bytes = state.jobs.capacity() * sizeof(ExcavationJob);
 // Count actual retained vector capacities once; this excludes allocator and process peaks.
 for (const auto &job : state.jobs) {
  bytes += (job.requested_tiles.capacity() + job.authorized_tiles.capacity()) * sizeof(TileIndex);
  bytes += job.route.capacity() * sizeof(ExcavationCutGeometry);
  for (const auto &cut : job.route) bytes += (cut.swept_tiles.capacity() + cut.support_tiles.capacity()) * sizeof(TileIndex);
  if (job.pass_terms) bytes += job.pass_terms->exchange_path.capacity() * sizeof(TileIndex) + job.pass_terms->exchange_after.capacity() * sizeof(uint16_t);
 }
 return bytes;
}
#endif /* TESTS_GROUND_JOB_FIXTURE_H */
