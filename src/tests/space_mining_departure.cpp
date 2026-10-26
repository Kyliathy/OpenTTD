/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_departure.cpp Native pre-translation release with original hulls, earned freight and exact generation-qualified admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group25B.
 * Body projections and optional blockers are declared geometry components, not executed lifecycle events or paid procurement.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "probe_mining_fixture.h"
#include "space_return_fixture.h"
#include "../space_mining_departure.h"
#include "../space_mining_continuation.h"
#include "../space_mining_target.h"
#include "../space_mining_tuning.h"
#include "../orbital_service.h"
#include "../vehicle_cmd.h"
#include "../company_base.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/** Project only the leased body's declared geometry to the actual due-release boundary.
 * @param craft Actual attached miner in its isolated original field; no craft position is assigned.
 * @return Current native proposal at a still-valid pre-translation point.
 * Actual shared attachment publication follows each straight body projection.
 */
static SpaceBodyDeparture ProjectMiningReleaseBoundary(SpaceVehicle &craft)
{
	// Jump by the analytic safe release delay, then recompute the bounded local
	// path. This is a geometry input, with no elapsed upkeep/work/event claim.
	for (uint attempt = 0; attempt < 32; ++attempt) {
		if (const auto due = PlanSpaceBodyDeparture(GetSpaceCraftReference(craft))) return *due;
		const auto plan = PlanSpaceDeparture(craft); REQUIRE(plan.release_after_ticks); REQUIRE(*plan.release_after_ticks > 0);
		auto field = GetAsteroidField(); auto body = std::ranges::find(field.bodies, craft.mission.target, &AsteroidBody::id); REQUIRE(body != field.bodies.end());
		const auto projected = ProjectSpaceMotionStraight(body->motion, *GetSpacePlaneBounds(field.width, field.height), *plan.release_after_ticks);
		REQUIRE(projected); REQUIRE_FALSE(projected->crossed_edges.Any()); body->motion = projected->proposed;
		REQUIRE(InstallAsteroidField(std::move(field))); UpdateAttachedSpaceCraftTransforms();
	}
	FAIL("bounded geometry did not reach its predicted native release boundary");
	return {};
}

/** Due release changes only original lease/work/intent; ordinary Tick alone earns one upkeep and any subsequent movement. */
TEST_CASE("Unsafe mining attachment releases before translation without an implicit Start", "[space-mining-departure]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); rig.Attach(craft);
	// Original opportunities earn the packet and an incomplete next work batch.
	for (uint tick = 0; tick < 91; ++tick) rig.Step();
	REQUIRE(craft.cargo.TotalCount() == 2); REQUIRE(craft.mission.work_ticks != 0);
	const bool stopped = GENERATE(false, true);
	if (stopped) REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded());
	const auto proposal = ProjectMiningReleaseBoundary(craft);
	CHECK(proposal.reason == SpaceBodyDepartureReason::UnsafeAttachment); CHECK(proposal.plan.release_after_ticks == 0);
	const auto position = craft.position; const auto packet = craft.cargo.Packets()->front()->index;
	const auto source = craft.cargo.Packets()->front()->GetSource(); const auto target = craft.mission.target;
	const auto reserve = GetMutableAsteroidBody(target)->remaining_reserve; const auto extracted = craft.extracted_units;
	const auto cost = craft.mission.cost_remainder; const auto money = Company::Get(craft.owner)->money;
	const auto clock = TimerGameTick::counter;
	const auto renewal = GetAsteroidField().renewal;
	const auto random = std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	REQUIRE(BeginSpaceBodyDeparture(proposal));
	CHECK(craft.position == position); CHECK(craft.vehstatus.Test(VehState::Stopped) == stopped);
	CHECK(craft.mission.work_ticks == 0); CHECK(craft.mission.continuation.departure_point == proposal.plan.destination);
	CHECK(GetMutableAsteroidBody(target)->mining_slot == VehicleID::Invalid()); CHECK(craft.space_orders == SpaceOrderQueue{});
	CHECK(craft.extracted_units == extracted); CHECK(craft.mission.cost_remainder == cost); CHECK(Company::Get(craft.owner)->money == money);
	CHECK(TimerGameTick::counter == clock); CHECK(ValidateNativeSpaceOrders());
	CHECK(GetAsteroidField().renewal == renewal);
	CHECK(std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random);
	CHECK_FALSE(BeginSpaceBodyDeparture(proposal)); CHECK(ReleaseUnsafeSpaceMiningAttachments() == 0);
	// The body can publish its next transform only after release. The craft
	// must not follow it or receive the final incomplete extraction batch.
	auto field = GetAsteroidField(); auto body = std::ranges::find(field.bodies, target, &AsteroidBody::id);
	const auto step = StepSpaceMotionStraight(body->motion, *GetSpacePlaneBounds(field.width, field.height)); REQUIRE(step); REQUIRE_FALSE(step->crossed_edges.Any());
	body->motion = step->proposed; REQUIRE(InstallAsteroidField(std::move(field))); UpdateAttachedSpaceCraftTransforms(); CHECK(craft.position == position);
	const auto intent = craft.mission.continuation; REQUIRE(craft.Tick());
	if (stopped) { CHECK(craft.position == position); CHECK(craft.mission.continuation == intent); }
	const auto idle = GetSpaceMiningTuning(GetSpaceVehicleModel(craft))->idle_per_day; const auto day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	CHECK(Company::Get(craft.owner)->money == money - (cost + idle) / day); CHECK(craft.mission.cost_remainder == (cost + idle) % day);
	CHECK(craft.cargo.TotalCount() == 2); CHECK(craft.cargo.Packets()->front()->index == packet); CHECK(craft.cargo.Packets()->front()->GetSource() == source);
	CHECK(GetMutableAsteroidBody(target)->remaining_reserve == reserve); CHECK(craft.extracted_units == extracted);
}

/** A doomed pursuit is released under its actual role and remaining budget; stale incarnations/reasons cannot authorize it. */
TEST_CASE("Doomed approaching miner uses exact original target and generation", "[space-mining-departure]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); REQUIRE(craft.mission.phase == SpaceMissionPhase::Approach);
	auto &competitor = rig.ProbeAt({20 * SPACE_Q16, 20 * SPACE_Q16});
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, competitor.index, true).Succeeded());
	CHECK_FALSE(PlanSpaceBodyDeparture(GetSpaceCraftReference(craft)));
	// Declare the actual target's final contained straight-motion checkpoint;
	// this approaching craft has no lease and receives no attachment transform.
	auto field = GetAsteroidField(); auto body = std::ranges::find(field.bodies, craft.mission.target, &AsteroidBody::id);
	const auto exit = ForecastAsteroidExit(*body, *GetSpacePlaneBounds(field.width, field.height)); REQUIRE(exit); REQUIRE(exit->ticks_until_exit);
	body->motion = exit->last_safe_motion;
	field.elapsed_ticks = 1; // Declared evolved geometry; a birth checkpoint must remain inside its original placement band.
	REQUIRE(InstallAsteroidField(std::move(field)));
	const auto proposal = PlanSpaceBodyDeparture(GetSpaceCraftReference(craft)); REQUIRE(proposal);
	CHECK(proposal->reason == SpaceBodyDepartureReason::UnsafeInterception);
	const auto pose = craft.position; const auto queue = craft.space_orders;
	auto stale = *proposal; ++stale.craft.generation; CHECK_FALSE(BeginSpaceBodyDeparture(stale));
	stale = *proposal; stale.body = OrbitalBodyID{1}; CHECK_FALSE(BeginSpaceBodyDeparture(stale));
	stale = *proposal; stale.reason = SpaceBodyDepartureReason::UnsafeAttachment; CHECK_FALSE(BeginSpaceBodyDeparture(stale));
	CHECK(craft.position == pose); CHECK(craft.space_orders == queue);
	REQUIRE(BeginSpaceBodyDeparture(*proposal)); CHECK(craft.position == pose); CHECK(craft.mission.phase == SpaceMissionPhase::Holding);
	const auto competitor_pose = competitor.position;
	CHECK(ReleaseUnsafeSpaceMiningAttachments() == 1); CHECK(ReleaseUnsafeSpaceMiningAttachments() == 0);
	CHECK(competitor.position == competitor_pose); CHECK(competitor.vehstatus.Test(VehState::Stopped));
	CHECK(competitor.mission.continuation.departure_point != craft.mission.continuation.departure_point);
	CHECK(craft.space_orders == SpaceOrderQueue{}); CHECK(ValidateNativeSpaceOrders());
}

/** Contained pending endpoints participate in the sole occupancy owner, invalidating stale quotes without a sidecar reservation. */
TEST_CASE("Sequential mining release respects competing contained departure points", "[space-mining-departure]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); rig.Attach(craft);
	const auto proposal = ProjectMiningReleaseBoundary(craft); REQUIRE(proposal.plan.destination);
	auto &competitor = rig.ProbeAt({20 * SPACE_Q16, 20 * SPACE_Q16});
	const auto participants = GatherSpaceMiningDepartureParticipants(); REQUIRE(participants.size() == 2);
	CHECK(participants[0].vehicle < participants[1].vehicle);
	REQUIRE(BeginSpaceMiningContinuation(competitor, proposal.plan.destination));
	CHECK(GetMutableAsteroidBody(craft.mission.target)->mining_slot == craft.index);
	CHECK_FALSE(IsSpaceDeparturePointAvailable(craft, *proposal.plan.destination));
	CHECK_FALSE(BeginSpaceBodyDeparture(proposal));
	const auto fresh = PlanSpaceBodyDeparture(GetSpaceCraftReference(craft));
	// The new route can require a later safe release. If already due, only its
	// freshly observed nonconflicting endpoint may be committed.
	if (fresh) { CHECK(fresh->plan.destination != proposal.plan.destination); REQUIRE(BeginSpaceBodyDeparture(*fresh)); }
	CHECK(ValidateNativeSpaceOrders());
}

/** Newly occupied saved geometry is revalidated before any native movement; packets stay aboard during retry. */
TEST_CASE("Mining departure rechecks native occupancy after publication", "[space-mining-departure]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); rig.Attach(craft);
	const auto proposal = ProjectMiningReleaseBoundary(craft); REQUIRE(proposal.plan.destination);
	REQUIRE(BeginSpaceBodyDeparture(proposal)); const auto position = craft.position;
	auto &blocker = rig.ProbeAt(*proposal.plan.destination, false); blocker.vehstatus.Set(VehState::Stopped); // Declared new physical occupant, no paid-world claim.
	REQUIRE(craft.Tick()); CHECK(craft.position == position); CHECK(craft.space_orders == SpaceOrderQueue{});
	CHECK(craft.mission.continuation.waiting == MiningContinuationWait::DepartureBlocked);
	CHECK(craft.mission.continuation.retry_ticks == GetSpaceMiningTargetRetryDelay()); CHECK_FALSE(craft.mission.continuation.departure_point);
	CHECK(ValidateNativeSpaceOrders());
}

/** Both configured native mining roles keep their original instruction/pin when actual due safety replaces attachment. */
TEST_CASE("Configured mining safety preserves the held original instruction", "[space-mining-departure]")
{
	SpaceReturnRig rig; const auto model = GENERATE(SPACE_PROBE_MODEL, SPACE_HEAVY_MINER_MODEL);
	auto &craft = rig.Miner(model);
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::FullLoadNearest).Succeeded());
	rig.MineAtLeast(craft, 2); const auto instructions = craft.space_orders.entries; const auto mineral = craft.mission.continuation.mineral;
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded());
	const auto proposal = ProjectMiningReleaseBoundary(craft); const auto pose = craft.position; const auto units = craft.cargo.TotalCount();
	REQUIRE(ReleaseUnsafeSpaceMiningAttachments() == 1); CHECK_FALSE(BeginSpaceBodyDeparture(proposal));
	CHECK(craft.space_orders.entries == instructions); CHECK(craft.space_orders.cursor == 0); CHECK_FALSE(craft.space_orders.started);
	CHECK(craft.mission.continuation.mineral == mineral); CHECK(craft.mission.continuation.previous_body == proposal.body);
	CHECK(craft.vehstatus.Test(VehState::Stopped)); REQUIRE(craft.Tick()); CHECK(craft.position == pose); CHECK(craft.cargo.TotalCount() == units);
	CHECK(ValidateNativeSpaceOrders());
}

/** No local endpoint still releases at the last safe point, then retains an explicit bounded wait. */
TEST_CASE("Crowded native safety releases without moving its blockers or original hull", "[space-mining-departure]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); rig.Attach(craft);
	const auto boundary = ProjectMiningReleaseBoundary(craft); REQUIRE(boundary.plan.attachment); REQUIRE(boundary.plan.attachment->first_unsafe_tick);
	auto field = GetAsteroidField(); auto body = std::ranges::find(field.bodies, craft.mission.target, &AsteroidBody::id);
	const auto bounds = *GetSpacePlaneBounds(field.width, field.height);
	const auto final = ProjectSpaceMotionStraight(body->motion, bounds, *boundary.plan.attachment->first_unsafe_tick - 1); REQUIRE(final);
	body->motion = final->proposed; REQUIRE(InstallAsteroidField(std::move(field))); UpdateAttachedSpaceCraftTransforms();
	// Declare stationary physical unit blockers at every bounded endpoint. This
	// challenges native occupancy without fabricating a paid fleet or berth.
	for (int radius = 1; radius <= SPACE_STAGING_MAX_RADIUS; ++radius) for (const auto [dx, dy] : SPACE_STAGING_DIRECTIONS) {
		const SpacePoint point{craft.position.x + dx * radius * SPACE_Q16, craft.position.y + dy * radius * SPACE_Q16};
		if (!bounds.Contains(point)) continue;
		auto &blocker = rig.ProbeAt(point, false); blocker.vehstatus.Set(VehState::Stopped);
	}
	const auto proposal = PlanSpaceBodyDeparture(GetSpaceCraftReference(craft)); REQUIRE(proposal);
	CHECK(proposal->plan.status == SpaceDepartureStatus::NoClearPoint); CHECK_FALSE(proposal->plan.destination);
	const auto pose = craft.position; const auto population = SpaceVehicle::GetNumItems();
	REQUIRE(ReleaseUnsafeSpaceMiningAttachments() == 1); CHECK(craft.position == pose); CHECK(SpaceVehicle::GetNumItems() == population);
	CHECK(GetMutableAsteroidBody(craft.mission.target)->mining_slot == VehicleID::Invalid());
	REQUIRE(craft.Tick()); CHECK(craft.position == pose); CHECK(craft.mission.continuation.waiting == MiningContinuationWait::DepartureBlocked);
	CHECK(craft.mission.continuation.retry_ticks == GetSpaceMiningTargetRetryDelay()); CHECK(ValidateNativeSpaceOrders());
}

/** Actual full-hold completion has already released extraction; safety cannot run that paid work again. */
TEST_CASE("Completed full holds and unrelated idle hulls are not mining release participants", "[space-mining-departure]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); rig.Attach(craft);
	// Earn all real batches through the original native clock and finite stock;
	// the bounded driver supplies neither a packet nor a completed work interval.
	for (uint tick = 0; craft.cargo.TotalCount() < craft.cargo_cap; ++tick) { REQUIRE(tick < 2000); rig.Step(); }
	REQUIRE(craft.mission.phase == SpaceMissionPhase::Return); const auto units = craft.cargo.TotalCount();
	auto &idle = rig.ProbeAt({20 * SPACE_Q16, 20 * SPACE_Q16}, false); const auto pose = idle.position;
	const auto money = Company::Get(craft.owner)->money;
	CHECK(GatherSpaceMiningDepartureParticipants().empty()); CHECK(ReleaseUnsafeSpaceMiningAttachments() == 0);
	CHECK(craft.cargo.TotalCount() == units); CHECK(idle.position == pose); CHECK(idle.mission.phase == SpaceMissionPhase::Idle);
	CHECK(Company::Get(craft.owner)->money == money); CHECK(ValidateNativeSpaceOrders());
}

/** The original service job/berth retains its partial timer, materials, freight and expense owner during safety gathering. */
TEST_CASE("Mining safety does not commandeer a separately owned static service job", "[space-mining-departure]")
{
	SpaceReturnRig rig; auto &craft = rig.Miner(); rig.MineAtLeast(craft, 4); rig.ReachAndHold(craft, *rig.core);
	craft.manifest->returns_remaining = 0; // Declared exhausted unit allowance, not a paid exchange history.
	const auto tuning = GetSpaceServiceTuning(SPACE_HEAVY_MINER_MODEL); REQUIRE(tuning);
	rig.Add(*rig.core, CargoLabel{"RFMT"}, tuning->inputs[0]); rig.Add(*rig.core, CargoLabel{"ELEC"}, tuning->inputs[1]);
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded()); // Actual service requires a running original hull at its real berth.
	REQUIRE(ExecuteSpaceReturnArrival(craft, SpaceReturnRig::Target(*rig.core), SpaceReturnPurpose::Service));
	REQUIRE(HasOrbitalServiceAssignment(craft)); const auto remaining = GetOrbitalServiceJobState(craft)->remaining_ticks;
	const auto fleet = ComputeSpaceVehicleDigest(), orders = ComputeSpaceOrdersDigest(), structures = ComputeSpaceStructureDigest();
	const auto money = Company::Get(craft.owner)->money; const auto units = craft.cargo.TotalCount();
	CHECK(GatherSpaceMiningDepartureParticipants().empty()); CHECK_FALSE(PlanSpaceBodyDeparture(GetSpaceCraftReference(craft)));
	CHECK(ReleaseUnsafeSpaceMiningAttachments() == 0); CHECK(GetOrbitalServiceJobState(craft)->remaining_ticks == remaining);
	CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeSpaceOrdersDigest() == orders); CHECK(ComputeSpaceStructureDigest() == structures);
	CHECK(GetSpaceStructureBerth(craft, *rig.core)); CHECK(craft.cargo.TotalCount() == units); CHECK(Company::Get(craft.owner)->money == money);
}
