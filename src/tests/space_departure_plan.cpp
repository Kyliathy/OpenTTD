/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_departure_plan.cpp Bounded original-owner departure queries and swept static-route checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group10B.
 * Native hulls/blockers use the declared isolated ProbeExtractionRig; these unit
 * cases claim no paid procurement, current-save migration or live edge release.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_departure.h"
#include "../space_structure.h"
#include "../space_manifest.h"
#include "../utility_power.h"
#include "../map_func.h"
#include "probe_mining_fixture.h"
#include "../safeguards.h"

/** Original native owners observed around a read-only departure query. */
struct DepartureQuerySnapshot {
	uint64_t fleet = ComputeSpaceVehicleDigest(), field = ComputeAsteroidFieldDigest(GetAsteroidField());
	uint64_t sites = ComputeSpaceStructureDigest(), manifests = ComputeSpaceManifestDigest(), power = ComputeUtilityPowerDigest();
	uint64_t tick = TimerGameTick::counter;
	Randomizer random = _random;
	/** Verify that planning did not move cargo/hulls, release a lease, spend power, advance work or consume random state. */
	void CheckUnchanged() const
	{
		CHECK(ComputeSpaceVehicleDigest() == this->fleet);
		CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == this->field);
		CHECK(ComputeSpaceStructureDigest() == this->sites);
		CHECK(ComputeSpaceManifestDigest() == this->manifests);
		CHECK(ComputeUtilityPowerDigest() == this->power);
		CHECK(TimerGameTick::counter == this->tick);
		CHECK(_random.state[0] == this->random.state[0]);
		CHECK(_random.state[1] == this->random.state[1]);
	}
};

/** Local routes use actual native movement duration and cannot sweep through a footprint or exact reserved berth. */
TEST_CASE("Departure paths retain native travel timing and original static clearance", "[space-departure-plan]")
{
	const auto bounds = *GetSpacePlaneBounds(64, 64);
	const SpacePoint origin{20 * SPACE_Q16, 20 * SPACE_Q16}, east{28 * SPACE_Q16, 20 * SPACE_Q16};
	CHECK(GetSpaceDepartureTravelTicks(origin, east, 0, bounds, {}) == 148);
	CHECK(GetSpaceDepartureTravelTicks(east, origin, 73, bounds, {}) == 148);
	CHECK(GetSpaceDepartureTravelTicks(origin, {28 * SPACE_Q16, 28 * SPACE_Q16}, 0, bounds, {}) == 210);
	CHECK(GetSpaceDepartureTravelTicks(origin, origin, 0, bounds, {}) == 0);
	CHECK_FALSE(GetSpaceDepartureTravelTicks(origin, {29 * SPACE_Q16, 20 * SPACE_Q16}, 0, bounds, {}));
	CHECK_FALSE(GetSpaceDepartureTravelTicks(origin, east, 74, bounds, {}));
	const std::array wall{SpaceAttachmentObstacle{{23 * SPACE_Q16, 19 * SPACE_Q16}, {24 * SPACE_Q16 - 1, 21 * SPACE_Q16 - 1}}};
	CHECK(IsSpaceAttachmentPointClear(east, bounds, wall));
	CHECK_FALSE(GetSpaceDepartureTravelTicks(origin, east, 0, bounds, wall));
	CHECK(GetSpaceDepartureTravelTicks(origin, {20 * SPACE_Q16, 28 * SPACE_Q16}, 0, bounds, wall) == 148);
	const SpacePoint between{origin.x + 1000, origin.y};
	const std::array berth{SpaceAttachmentObstacle{between, between, SpaceAttachmentObstacleKind::Approach}};
	// A real native step jumps farther than this point; swept admission must
	// still protect the original berth rather than checking endpoints alone.
	CHECK(IsSpaceAttachmentPointClear(east, bounds, berth));
	CHECK_FALSE(GetSpaceDepartureTravelTicks(origin, east, 0, bounds, berth));
	const std::array missed{SpaceAttachmentObstacle{{between.x, between.y + 1}, {between.x, between.y + 1}, SpaceAttachmentObstacleKind::Approach}};
	CHECK(GetSpaceDepartureTravelTicks(origin, east, 0, bounds, missed) == 148);
	const SpaceMotion body_motion{{bounds.max.x - SPACE_Q16, 20 * SPACE_Q16}, 16384, 0, 0, 0};
	const auto body = *ForecastSpaceMotionExit(body_motion, bounds);
	const auto safety = *ForecastSpaceAttachmentSafety(body_motion, body, {}, bounds, {});
	REQUIRE(safety.first_unsafe_tick == 297);
	CHECK(GetSpaceDepartureReleaseDelay(safety, uint32_t{148}) == 148);
	CHECK(GetSpaceDepartureReleaseDelay(safety, std::nullopt) == 296);
	const SpaceAttachmentSafety next_step{1, origin, SpaceAttachmentHazard::MapEdge};
	CHECK(GetSpaceDepartureReleaseDelay(next_step, std::nullopt) == 0);
	CHECK(GetSpaceDepartureReleaseDelay(next_step, uint32_t{148}) == 0);
	CHECK_FALSE(GetSpaceDepartureReleaseDelay(SpaceAttachmentSafety{}, uint32_t{148}));
}

/** A stopped real unit attachment receives the same read-only safety proposal while retaining its earned cargo and sole lease. */
TEST_CASE("Departure planning preserves an attached native miner and its stopped intent", "[space-departure-plan]")
{
	ProbeExtractionRig rig;
	REQUIRE(SpaceStructure::GetNumItems() == 0);
	auto &craft = rig.Probe();
	auto &body = rig.Attach(craft);
	// Ordinary isolated native opportunities earn a real partial hold. The
	// departure query must not fabricate extraction or consume that original hold.
	for (uint tick = 0; tick < 74; ++tick) rig.Step();
	REQUIRE(craft.cargo.TotalCount() == 2);
	const auto position = craft.position;
	const DepartureQuerySnapshot running;
	const auto plan = PlanSpaceDeparture(craft, craft.home);
	REQUIRE(plan.status == SpaceDepartureStatus::Ready);
	REQUIRE(plan.destination.has_value());
	REQUIRE(plan.attachment.has_value());
	CHECK(plan.retained_position == position);
	CHECK(plan.candidates_examined <= 65);
	CHECK(plan.travel_ticks > 0);
	CHECK(plan.travel_ticks <= 298);
	CHECK(SpaceDistanceSquared(*plan.destination, body.motion.position) >= uint64_t{2 * SPACE_Q16} * (2 * SPACE_Q16));
	running.CheckUnchanged();
	craft.vehstatus.Set(VehState::Stopped); // Declared unit running flag; no paid-world start/stop claim.
	const DepartureQuerySnapshot stopped;
	const auto stopped_plan = PlanSpaceDeparture(craft, craft.home);
	CHECK(stopped_plan == plan);
	CHECK(craft.vehstatus.Test(VehState::Stopped));
	CHECK(craft.position == position);
	CHECK(body.mining_slot == craft.index);
	CHECK(craft.cargo.TotalCount() == 2);
	stopped.CheckUnchanged();
}

/** Fixed ring order resolves equal onward distances and never selects an off-map corner candidate. */
TEST_CASE("Departure candidates retain deterministic ties at a map corner", "[space-departure-plan]")
{
	ProbeExtractionRig rig;
	auto &craft = rig.ProbeAt({0, 0}, false);
	auto &claimant = rig.ProbeAt({32 * SPACE_Q16, 32 * SPACE_Q16}, false);
	claimant.vehstatus.Set(VehState::Stopped);
	// Explicit isolated waiting-claim input blocks only the original point. Its
	// remote hull does not block the equal-distance south/east alternatives.
	claimant.space_orders.approach = SpaceApproachRequest{{SpaceOrderTargetKind::Body, 0, 0}, TimerGameTick::counter, UINT8_MAX, craft.position};
	const DepartureQuerySnapshot before;
	const auto plan = PlanSpaceDeparture(craft, SpacePoint{0, 0});
	REQUIRE(plan.status == SpaceDepartureStatus::Ready);
	CHECK(plan.destination == SpacePoint{0, SPACE_Q16});
	CHECK(plan.candidates_examined == 65);
	CHECK(plan.travel_ticks == 19);
	CHECK_FALSE(plan.attachment);
	CHECK_FALSE(plan.release_after_ticks);
	CHECK(PlanSpaceDeparture(craft, SpacePoint{0, 0}) == plan);
	CHECK(PlanSpaceDeparture(craft, SpacePoint{-1, 0}).status == SpaceDepartureStatus::InvalidPosition);
	before.CheckUnchanged();
}

/** Exhausting the bounded ring cannot move blockers, teleport the original hull or release its packets/lease from a query. */
TEST_CASE("A crowded departure retains its original valid attached point", "[space-departure-plan]")
{
	ProbeExtractionRig rig;
	auto &craft = rig.Probe();
	auto &body = rig.Attach(craft);
	const auto bounds = *GetSpacePlaneBounds(Map::SizeX(), Map::SizeY());
	// Each in-map endpoint receives an explicitly declared stationary unit
	// blocker. Out-of-map candidates already fail the original shared predicate.
	for (int radius = 1; radius <= 8; ++radius) {
		for (const auto [dx, dy] : std::array<std::pair<int, int>, 8>{{{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}}}) {
			const SpacePoint position{craft.position.x + dx * radius * SPACE_Q16, craft.position.y + dy * radius * SPACE_Q16};
			if (!bounds.Contains(position)) continue;
			auto &blocker = rig.ProbeAt(position, false);
			blocker.vehstatus.Set(VehState::Stopped);
		}
	}
	const DepartureQuerySnapshot before;
	const auto plan = PlanSpaceDeparture(craft, craft.home);
	CHECK(plan.status == SpaceDepartureStatus::NoClearPoint);
	CHECK_FALSE(plan.destination);
	CHECK(plan.travel_ticks == 0);
	CHECK(plan.candidates_examined == 65);
	CHECK(plan.retained_position == craft.position);
	CHECK(bounds.Contains(plan.retained_position));
	CHECK(body.mining_slot == craft.index);
	REQUIRE(plan.attachment.has_value());
	CHECK(plan.release_after_ticks == GetSpaceDepartureReleaseDelay(*plan.attachment, std::nullopt));
	before.CheckUnchanged();
}
