/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_target.cpp Native immediate-slot eligibility for automatic mining destination queries.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group18.
 */
#include "../stdafx.h"
#include "probe_mining_fixture.h"
#include "../space_mining_target.h"
#include "../space_mining_tuning.h"
#include "../safeguards.h"

/** Immediate eligibility uses the original shared requests and real leases without saving a hypothetical request. */
TEST_CASE("Automatic body eligibility respects actual arbitration and extraction leases", "[space-mining-target]")
{
	ProbeExtractionRig rig; auto &first = rig.Probe(4, false); auto &second = rig.Probe(6, false);
	const SpaceOrderTarget target{SpaceOrderTargetKind::Body, 0, 0};
	const auto field = GetAsteroidField(); const auto fleet = ComputeSpaceVehicleDigest();
	CHECK(CanAcquireSpaceApproach(first, target)); CHECK(CanAcquireSpaceApproach(second, target));
	CHECK_FALSE(CanAcquireSpaceApproach(first, {SpaceOrderTargetKind::Body, 70'000, 0}));
	CHECK(GetAsteroidField() == field); CHECK(ComputeSpaceVehicleDigest() == fleet);
	REQUIRE(RequestSpaceApproach(first, target, true));
	const auto pending = ComputeSpaceVehicleDigest();
	CHECK(CanAcquireSpaceApproach(first, target));
	CHECK_FALSE(CanAcquireSpaceApproach(second, target)); // Earlier native request wins; the query cannot preempt it.
	CHECK(ComputeSpaceVehicleDigest() == pending);
	PrepareSpaceApproachTick();
	REQUIRE(first.space_orders.approach); CHECK(first.space_orders.approach->slot == 0);
	CHECK_FALSE(CanAcquireSpaceApproach(second, target));
	REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, first.index, 0).Succeeded());
	const auto &body = rig.Attach(first); REQUIRE(body.mining_slot == first.index);
	const auto leased = ComputeSpaceVehicleDigest();
	CHECK(CanAcquireSpaceApproach(first, target)); CHECK_FALSE(CanAcquireSpaceApproach(second, target));
	CHECK(ComputeSpaceVehicleDigest() == leased);
	// A normal explicit command can still queue for a busy body. Automatic
	// replacement instead waits for actual eligibility and rechecks before commit.
	CHECK(AssignSpaceMiningMission(second, 0, false).Succeeded());
	CHECK(AssignSpaceMiningMission(second, 0, true, SpaceMiningPolicy::FullLoadNearest).Failed());
	CHECK(ComputeSpaceVehicleDigest() == leased); // Lost eligibility between selection and commit cannot revoke the winner's lease.
	REQUIRE(CmdSetSpaceMissionChecked(DoCommandFlag::Execute, first.index, first.generation, UINT32_MAX).Succeeded());
	CHECK(body.mining_slot == VehicleID::Invalid());
	rig.Step(); // Ordinary movement clears the released hull's original physical point.
	CHECK(CanAcquireSpaceApproach(second, target));
	CHECK(second.space_orders.approach == std::nullopt);
}

/** Copied candidate identities remain full-width and distance ties remain deterministic after storage reversal or retirement holes. */
TEST_CASE("Mining candidate ordering uses finite mineral matching and full sparse IDs", "[space-mining-target]")
{
	std::vector<AsteroidBody> bodies(5);
	const std::array ids{70'000U, 8U, UINT32_MAX - 1, 4U, 9U};
	// These detached rows deliberately tie in space; only full stable identity
	// may order them. They allocate no native body, reserve or field history.
	for (uint index = 0; index < bodies.size(); ++index) {
		bodies[index].id = OrbitalBodyID{ids[index]}; bodies[index].remaining_reserve = 2;
		bodies[index].motion.position = {20 * SPACE_Q16, 20 * SPACE_Q16};
	}
	bodies[3].resource_class = AsteroidClass::Carbonaceous; bodies[4].remaining_reserve = 0;
	const std::vector expected{OrbitalBodyID{8}, OrbitalBodyID{70'000}, OrbitalBodyID{UINT32_MAX - 1}};
	CHECK(RankMiningTargetCandidates(bodies, {21 * SPACE_Q16, 20 * SPACE_Q16}, CargoLabel{"NORE"}) == expected);
	std::ranges::reverse(bodies);
	CHECK(RankMiningTargetCandidates(bodies, {21 * SPACE_Q16, 20 * SPACE_Q16}, CargoLabel{"NORE"}) == expected);
	CHECK(RankMiningTargetCandidates(bodies, {}, CargoLabel{"WICE"}) == std::vector{OrbitalBodyID{4}});
	CHECK(RankMiningTargetCandidates(bodies, {}, CargoLabel{"RARE"}).empty());
	CHECK(RankMiningTargetCandidates(bodies, {}, CargoLabel{}).empty());
}

/** Partial actual freight constrains subsequent selection independently of empty/inert continuation metadata. */
TEST_CASE("Nearest mining query preserves partial cargo and every original owner", "[space-mining-target]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); rig.Attach(craft);
	// The held mineral is earned by the original extraction path; the query
	// cannot relabel these packets to satisfy another body's physical cargo.
	for (uint tick = 0; tick < 74; ++tick) rig.Step();
	REQUIRE(craft.cargo.TotalCount() == 2); REQUIRE(craft.mission.continuation.mineral.Empty());
	const auto field = GetAsteroidField(); const auto fleet = ComputeSpaceVehicleDigest();
	CHECK(FindNearestEligibleMiningTarget(craft, CargoLabel{"NORE"}) == OrbitalBodyID{0});
	CHECK_FALSE(FindNearestEligibleMiningTarget(craft, CargoLabel{"WICE"}));
	CHECK(GetAsteroidField() == field); CHECK(ComputeSpaceVehicleDigest() == fleet);
}

/** Actual arrival time, not the pursuit deadline's minimum, decides whether nearby edge resources leave a safe departure window. */
TEST_CASE("Mining interception accepts a reachable edge body and rejects an imminent unsafe attachment", "[space-mining-target]")
{
	const auto bounds = GetSpacePlaneBounds(64, 64); REQUIRE(bounds);
	AsteroidBody body; body.id = OrbitalBodyID{70'000}; body.remaining_reserve = 2;
	body.motion.position = {61 * SPACE_Q16, 32 * SPACE_Q16}; body.motion.velocity_x = SPACE_Q16;
	const SpacePoint start{62 * SPACE_Q16, 32 * SPACE_Q16 - SPACE_Q16 / 2};
	// Both native mining roles share the exact pursuit mover. This copied test
	// isolates boundary timing and grants no role Engine, hull, lease or packet.
	for (const auto model : {SPACE_PROBE_MODEL, SPACE_HEAVY_MINER_MODEL}) {
		const auto forecast = ForecastSpaceMiningIntercept(start, 0, model, body, *bounds, {}); REQUIRE(forecast);
		CHECK(forecast->arrival_ticks == 1); CHECK(forecast->departure_ticks > 0);
		CHECK(bounds->Contains(forecast->departure_point));
		CHECK(SpaceDistanceSquared(forecast->departure_point, body.motion.position) >= static_cast<uint64_t>(GetSpaceBodyApproachClearance()) * GetSpaceBodyApproachClearance());
		auto late = body; late.motion.position.x = 62 * SPACE_Q16 - 1;
		CHECK_FALSE(ForecastSpaceMiningIntercept(start, 0, model, late, *bounds, {}));
		CHECK_FALSE(ForecastSpaceMiningIntercept({10 * SPACE_Q16, 10 * SPACE_Q16}, 0, model, body, *bounds, {}));
	}
	CHECK_FALSE(ForecastSpaceMiningIntercept(start, 0, 19, body, *bounds, {}));
	CHECK_FALSE(ForecastSpaceMiningIntercept(start, 74, SPACE_PROBE_MODEL, body, *bounds, {}));
	CHECK_FALSE(ForecastSpaceMiningIntercept({-1, 0}, 0, SPACE_PROBE_MODEL, body, *bounds, {}));
}

/** A nearer target across a static footprint is not safely interceptable even if its destination and eventual departure point are clear. */
TEST_CASE("Mining interception checks swept travel through original static geometry", "[space-mining-target]")
{
	const auto bounds = GetSpacePlaneBounds(64, 64); REQUIRE(bounds);
	AsteroidBody body; body.motion.position = {40 * SPACE_Q16, 32 * SPACE_Q16};
	const SpacePoint start{30 * SPACE_Q16, 32 * SPACE_Q16 - SPACE_Q16 / 2};
	CHECK(ForecastSpaceMiningIntercept(start, 0, SPACE_PROBE_MODEL, body, *bounds, {}));
	const std::array obstacles{SpaceAttachmentObstacle{{35 * SPACE_Q16, 30 * SPACE_Q16}, {36 * SPACE_Q16, 33 * SPACE_Q16}, SpaceAttachmentObstacleKind::Footprint}};
	CHECK_FALSE(ForecastSpaceMiningIntercept(start, 0, SPACE_PROBE_MODEL, body, *bounds, obstacles));
	const std::array berth{SpaceAttachmentObstacle{start, start, SpaceAttachmentObstacleKind::Approach}};
	CHECK_FALSE(ForecastSpaceMiningIntercept(start, 0, SPACE_PROBE_MODEL, body, *bounds, berth));
	CHECK(ForecastSpaceMiningIntercept(start, 0, SPACE_PROBE_MODEL, body, *bounds, berth, start));
	CHECK_FALSE(ForecastSpaceMiningIntercept(start, 0, SPACE_PROBE_MODEL, body, *bounds, berth, SpacePoint{}));
	const std::array footprint{SpaceAttachmentObstacle{start, start, SpaceAttachmentObstacleKind::Footprint}};
	CHECK_FALSE(ForecastSpaceMiningIntercept(start, 0, SPACE_PROBE_MODEL, body, *bounds, footprint, start));
}

/** Copied edge metadata exercises the future lifecycle before sparse/edge field installation is enabled in Group22. */
TEST_CASE("A soon-exiting nearest candidate yields to a safely interceptable second body", "[space-mining-target]")
{
	const auto bounds = GetSpacePlaneBounds(64, 64); REQUIRE(bounds);
	std::array<AsteroidBody, 2> bodies{};
	bodies[0].id = OrbitalBodyID{70'000}; bodies[0].remaining_reserve = 2;
	bodies[0].motion.position = {62 * SPACE_Q16 - 1, 32 * SPACE_Q16}; bodies[0].motion.velocity_x = SPACE_Q16;
	bodies[1].id = OrbitalBodyID{70'001}; bodies[1].remaining_reserve = 2;
	bodies[1].motion.position = {58 * SPACE_Q16, 32 * SPACE_Q16}; bodies[1].motion.velocity_x = -SPACE_Q16;
	const SpacePoint start{61 * SPACE_Q16, 32 * SPACE_Q16 - SPACE_Q16 / 2};
	CHECK(RankMiningTargetCandidates(bodies, start, CargoLabel{"NORE"}) == std::vector{bodies[0].id, bodies[1].id});
	CHECK_FALSE(ForecastSpaceMiningIntercept(start, 0, SPACE_PROBE_MODEL, bodies[0], *bounds, {}));
	CHECK(ForecastSpaceMiningIntercept(start, 0, SPACE_PROBE_MODEL, bodies[1], *bounds, {}));
}

/** Resource events wake only their matching original policy wait; selection and all physical owners stay unchanged. */
TEST_CASE("Mining retry events preserve unrelated mineral and receiver waits", "[space-mining-target]")
{
	ProbeExtractionRig rig; auto &ore = rig.Probe(4, false); auto &ice = rig.Probe(6, false);
	const std::array crafts{&ore, &ice};
	// These are declared mission-intent inputs for event admission, not earned
	// movement, extraction or cargo. Actual native commands still own the queues.
	for (uint index = 0; index < crafts.size(); ++index) {
		auto &craft = *crafts[index];
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 0,
			SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, index, 0).Succeeded());
		REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::FullLoadNearest).Succeeded());
		auto &intent = craft.mission.continuation; intent.phase = MiningContinuationPhase::Waiting;
		intent.waiting = MiningContinuationWait::NoCompatibleAsteroid; intent.retry_ticks = GetSpaceMiningTargetRetryDelay();
		intent.mineral = index == 0 ? CargoLabel{"NORE"} : CargoLabel{"WICE"};
	}
	const auto field = GetAsteroidField(); const auto packet_count = CargoPacket::GetNumItems();
	CHECK(GetSpaceMiningTargetRetryDelay() == 74);
	NotifySpaceMiningTargetChange(CargoLabel{"NORE"});
	CHECK(ore.mission.continuation.retry_ticks == 0); CHECK(ice.mission.continuation.retry_ticks == 74);
	ice.mission.continuation.waiting = MiningContinuationWait::NoReceiver;
	NotifySpaceMiningTargetChange(CargoLabel{"WICE"}); CHECK(ice.mission.continuation.retry_ticks == 74);
	CHECK(GetAsteroidField() == field); CHECK(CargoPacket::GetNumItems() == packet_count);
	CHECK(ore.space_orders.approach == std::nullopt); CHECK(ice.space_orders.approach == std::nullopt);
}
