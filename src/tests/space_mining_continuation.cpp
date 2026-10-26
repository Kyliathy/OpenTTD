/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_continuation.cpp Original queue/approach/native mining continuation across real extraction, waits and handling.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups20/25.
 * Parent factory/frame/legacy-hull inputs and explicitly reduced remaining reserves are declared component inputs. Freight, arrival and extraction are earned through native ticks; no paid-world checkpoint is claimed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "space_return_fixture.h"
#include "../space_mining_continuation.h"
#include "../space_mining_target.h"
#include "../orbital_service.h"
#include "../safeguards.h"

/** Native component driver retaining the original parent owners and checked policy command. */
class ContinuationMiningRig : public SpaceReturnRig {
public:
	/** @param policy Current explicit Mine preference. @param model Actual native mining role. @return Original miner configured before its first actual assignment. */
	SpaceVehicle &Configured(SpaceMiningPolicy policy = SpaceMiningPolicy::FullLoadNearest, uint16_t model = SPACE_PROBE_MODEL)
	{
		auto &craft = this->Miner(model);
		REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, policy).Succeeded()); return craft;
	}
	/** @tparam Predicate Read-only completion observation. @param done Actual native result to await. Offer bounded normal field/approach/vehicle opportunities. */
	template <typename Predicate> void Until(Predicate done)
	{
		// The finite bound detects stalled controller transitions without a
		// private movement clock, assigned pose or fabricated completed work.
		for (uint tick = 0; !done(); ++tick) { REQUIRE(tick < 5000); this->Step(); }
	}
};

/** Both native mining roles keep the same original Mine through actual finite depletion and another same-mineral attachment. */
TEST_CASE("Full-load mining continues across two actual bodies without advancing its instruction", "[space-mining-continuation]")
{
	const uint16_t model = GENERATE(SPACE_PROBE_MODEL, SPACE_HEAVY_MINER_MODEL);
	ContinuationMiningRig rig; auto &craft = rig.Configured(SpaceMiningPolicy::FullLoadNearest, model);
	auto &first = rig.Attach(craft); first.remaining_reserve = 2; // Explicit finite source boundary; no output or completed work is supplied.
	const auto original = craft.space_orders.entries;
	rig.Until([&] { return craft.cargo.TotalCount() >= 2; });
	REQUIRE(craft.mission.phase == SpaceMissionPhase::Return);
	CHECK_FALSE(ResolveEffectiveSpaceOrderTarget(craft)); CHECK(ValidateNativeSpaceOrders());
	PrepareSpaceApproachTick(); CHECK_FALSE(craft.space_orders.approach); // Completion cannot reacquire the just-released body between real native opportunities.
	rig.Until([&] { return craft.cargo.TotalCount() >= 4; });
	CHECK(first.remaining_reserve == 0); CHECK(craft.mission.target != first.id);
	CHECK(craft.space_orders.cursor == 0); CHECK(craft.space_orders.entries == original);
	CHECK(craft.mission.continuation.mineral == CargoLabel{"NORE"});
	CHECK(craft.mission.continuation.phase == MiningContinuationPhase::SeekMineral);
	const auto target = ResolveEffectiveSpaceOrderTarget(craft); REQUIRE(target);
	CHECK(target->kind == SpaceOrderTargetKind::Body); CHECK(target->id == craft.mission.target.base());
	REQUIRE(craft.space_orders.approach); CHECK(craft.space_orders.approach->target == *target);
	CHECK(ValidateNativeSpaceMiningContinuation(craft)); CHECK(ValidateNativeSpaceOrders());
	// Original fragments retain their two actual source identities. Continuation
	// does not merge different asteroid histories into a relabelled packet.
	REQUIRE(craft.cargo.Packets()->size() == 2);
	CHECK(craft.cargo.Packets()->front()->GetSource() != craft.cargo.Packets()->back()->GetSource());
}

/** No compatible finite source retains a safe named wait; an actual resource-change event wakes only matching intent. */
TEST_CASE("Mining continuation waits retries and preserves freight through edits and Recall", "[space-mining-continuation]")
{
	ContinuationMiningRig rig; auto &craft = rig.Configured(); auto &first = rig.Attach(craft); first.remaining_reserve = 2;
	AsteroidBody *other = nullptr; uint32_t other_reserve = 0;
	// These explicitly declared reserve boundaries leave one original finite
	// batch available. No production path depletes or refills resources here.
	for (const auto &row : GetAsteroidField().bodies) {
		if (row.id == first.id || row.resource_class != first.resource_class) continue;
		auto *body = GetMutableAsteroidBody(row.id);
		if (other == nullptr) { other = body; other_reserve = body->remaining_reserve; }
		body->remaining_reserve = 0;
	}
	REQUIRE(other != nullptr);
	rig.Until([&] { return craft.mission.continuation.phase == MiningContinuationPhase::Waiting; });
	REQUIRE(craft.mission.continuation.waiting == MiningContinuationWait::NoCompatibleAsteroid);
	CHECK(craft.cargo.TotalCount() == 2); CHECK(craft.space_orders.cursor == 0); CHECK_FALSE(ResolveEffectiveSpaceOrderTarget(craft));
	CHECK_FALSE(craft.space_orders.approach); CHECK(ValidateNativeSpaceOrders());
	const auto pose = craft.position; const auto packet = craft.cargo.Packets()->front()->index;
	const auto source = craft.cargo.Packets()->front()->GetSource(); const auto retry = craft.mission.continuation.retry_ticks;
	rig.Step(); CHECK(craft.position == pose); CHECK(craft.mission.continuation.retry_ticks + 1 == retry); CHECK(craft.cargo.TotalCount() == 2);
	SECTION("matching event resumes ordinary replacement mining") {
		other->remaining_reserve = other_reserve; // Restore the named declared fixture input; this is not a runtime refill producer.
		NotifySpaceMiningTargetChange(CargoLabel{"WICE"}); CHECK(craft.mission.continuation.retry_ticks != 0);
		NotifySpaceMiningTargetChange(CargoLabel{"NORE"}); CHECK(craft.mission.continuation.retry_ticks == 0);
		rig.Until([&] { return craft.cargo.TotalCount() > 2; });
		CHECK(craft.space_orders.cursor == 0); CHECK(craft.mission.target == other->id);
	}
	SECTION("explicit Recall cancels only continuation") {
		REQUIRE(CmdSetSpaceMissionChecked(DoCommandFlag::Execute, craft.index, craft.generation, UINT32_MAX).Succeeded());
		CHECK(craft.mission.continuation == SpaceMiningContinuationState{}); CHECK(craft.space_orders.entries.empty());
		CHECK(craft.cargo.TotalCount() == 2); CHECK(craft.cargo.Packets()->front()->index == packet); CHECK(craft.cargo.Packets()->front()->GetSource() == source);
	}
	SECTION("queue edit cancels associated intent") {
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Clear, 0,
			SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Succeeded());
		CHECK(craft.mission.continuation == SpaceMiningContinuationState{}); CHECK(craft.cargo.TotalCount() == 2);
		CHECK(craft.cargo.Packets()->front()->index == packet); CHECK(craft.cargo.Packets()->front()->GetSource() == source);
	}
}

/** Return policy reaches every original Visit before real receiving work and completes only those original entries afterward. */
TEST_CASE("Mining return retains explicit Visit order before nearest original unloading", "[space-mining-continuation]")
{
	ContinuationMiningRig rig; auto &craft = rig.Miner();
	REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 1,
		SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 1, 0).Succeeded());
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Succeeded());
	auto &first = rig.Attach(craft); first.remaining_reserve = 2; const auto original = craft.space_orders.entries;
	rig.Until([&] { return craft.mission.continuation.phase == MiningContinuationPhase::ReturnCargo; });
	CHECK(craft.space_orders.cursor == 0); CHECK(craft.mission.continuation.route_cursor == 1);
	CHECK(ResolveEffectiveSpaceOrderTarget(craft) == original[1].target);
	rig.Until([&] { return craft.mission.continuation.route_cursor == 2; });
	const auto *visited = GetMutableAsteroidBody(OrbitalBodyID{1}); REQUIRE(visited);
	CHECK(craft.position == SpacePoint{visited->motion.position.x + SPACE_Q16, visited->motion.position.y - SPACE_Q16 / 2});
	CHECK(craft.cargo.TotalCount() == 2); CHECK(craft.space_orders.cursor == 0);
	rig.Until([&] { return craft.space_orders.cursor == original.size(); });
	CHECK(craft.space_orders.entries == original); CHECK(craft.cargo.TotalCount() == 0);
	CHECK(rig.core->module->stock[craft.cargo_type].TotalCount() + rig.storage->module->stock[craft.cargo_type].TotalCount() == 2);
	CHECK(craft.mission.continuation == SpaceMiningContinuationState{}); CHECK(ValidateNativeSpaceOrders());
}

/** Full holds take the return branch even under full-load policy; ReturnService keeps its original non-unloading meaning. */
TEST_CASE("A full mining hold returns and a service visit never becomes an implicit unload", "[space-mining-continuation]")
{
	ContinuationMiningRig rig;
	SECTION("actual full hold unloads once") {
		auto &craft = rig.Configured(); const auto original = craft.space_orders.entries;
		rig.Until([&] { return craft.space_orders.cursor == original.size(); });
		CHECK(craft.extracted_units == craft.cargo_cap); CHECK(craft.cargo.TotalCount() == 0);
		CHECK(rig.core->module->stock[craft.cargo_type].TotalCount() + rig.storage->module->stock[craft.cargo_type].TotalCount() == craft.extracted_units);
	}
	SECTION("positive allowance completes original service visit with cargo intact") {
		auto &craft = rig.Miner(SPACE_PROBE_MODEL);
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Replace, 1,
			SpaceOrderAction::ReturnService, SpaceOrderTargetKind::Structure, rig.core->index.base(), rig.core->generation).Succeeded());
		craft.manifest = std::make_unique<SpaceManifest>(); craft.manifest->returns_remaining = 2; // Explicit legacy unit allowance, not paid service evidence.
		REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Succeeded());
		auto &body = rig.Attach(craft); body.remaining_reserve = 2;
		rig.Until([&] { return craft.space_orders.cursor == 2; });
		CHECK(craft.cargo.TotalCount() == 2); CHECK(craft.manifest->returns_remaining == 2);
		CHECK(craft.manifest->services_completed == 0); CHECK_FALSE(HasOrbitalServiceAssignment(craft));
		CHECK(rig.core->module->stock[craft.cargo_type].TotalCount() == 0); CHECK(GetSpaceStructureBerth(craft, *rig.core));
	}
}

/** Real missing parts produce a retained wait; the original service job and actual source berth then permit resumed mining without advancing Mine. */
TEST_CASE("Held full-load mining resumes only after original required service completes", "[space-mining-continuation]")
{
	ContinuationMiningRig rig; auto &craft = rig.Configured(SpaceMiningPolicy::FullLoadNearest, SPACE_HEAVY_MINER_MODEL);
	rig.MineAtLeast(craft, 4); const auto original = craft.space_orders.entries; const auto units = craft.cargo.TotalCount();
	const auto tuning = GetSpaceServiceTuning(SPACE_HEAVY_MINER_MODEL); REQUIRE(tuning);
	// This explicit exhausted-allowance component boundary triggers continuation
	// without fabricating paid exchanges or completed service. Actual service
	// admission, parts, power, work and subsequent renewal remain original owners.
	craft.manifest->returns_remaining = 0;
	for (uint slot = 0; slot < tuning->inputs.size(); ++slot) {
		auto &stock = rig.core->module->stock[GetSpaceSupplyCargo(static_cast<SpaceSupply>(slot + 1))];
		if (stock.TotalCount() != 0) REQUIRE(HubCargoTransfer::Consume(stock, stock.TotalCount()));
	}
	rig.Until([&] { return craft.mission.continuation.phase == MiningContinuationPhase::Waiting; });
	CHECK(craft.mission.continuation.waiting == MiningContinuationWait::ServiceUnavailable);
	CHECK(craft.space_orders.blocked == SpaceOrderBlock::ServiceRequired); CHECK(craft.space_orders.cursor == 0);
	CHECK(craft.cargo.TotalCount() == units); CHECK_FALSE(HasOrbitalServiceAssignment(craft));
	rig.Add(*rig.core, CargoLabel{"RFMT"}, tuning->inputs[0]); rig.Add(*rig.core, CargoLabel{"ELEC"}, tuning->inputs[1]);
	rig.Until([&] { return HasOrbitalServiceAssignment(craft); });
	REQUIRE(GetSpaceStructureBerth(craft, *rig.core)); CHECK(craft.space_orders.cursor == 0); CHECK(craft.space_orders.entries == original);
	CHECK(craft.manifest->returns_remaining == 0); CHECK(craft.cargo.TotalCount() == units); CHECK(ValidateNativeSpaceOrders());
	const auto ticks = GetOrbitalServiceJobState(craft)->remaining_ticks;
	REQUIRE(CmdServiceOrbitalProbe(DoCommandFlag::Execute, rig.core->index, rig.core->generation, craft.index, craft.generation, OrbitalServiceControl::Pause).Succeeded());
	rig.Allocate(); TickOrbitalServices(); CHECK(GetOrbitalServiceJobState(craft)->remaining_ticks == ticks);
	REQUIRE(CmdServiceOrbitalProbe(DoCommandFlag::Execute, rig.core->index, rig.core->generation, craft.index, craft.generation, OrbitalServiceControl::Resume).Succeeded());
	// Only the real common grant and retained job settle the immutable service
	// bill. Partial work and pause never renew allowance or complete Mine.
	for (uint tick = 0; tick < ticks; ++tick) { rig.Allocate(); TickOrbitalServices(); }
	REQUIRE_FALSE(HasOrbitalServiceAssignment(craft)); REQUIRE(craft.manifest->returns_remaining == tuning->returns);
	rig.Step(); CHECK(craft.mission.continuation.phase == MiningContinuationPhase::SeekMineral);
	REQUIRE(GetSpaceStructureBerth(craft, *rig.core)); // The selector must still have its real source-berth proof.
	rig.Until([&] { return craft.space_orders.started; });
	CHECK(craft.space_orders.cursor == 0); CHECK(craft.space_orders.entries == original);
	CHECK(craft.mission.phase == SpaceMissionPhase::Approach); CHECK(craft.cargo.TotalCount() == units);
	CHECK(ValidateNativeSpaceOrders());
}

/** Explicit safe-release publication never implies a Start action, movement, packet creation or generic Holding completion. */
TEST_CASE("A stopped miner retains its pose instruction and cargo through explicit continuation publication", "[space-mining-continuation]")
{
	ContinuationMiningRig rig; auto &craft = rig.Configured(); rig.MineAtLeast(craft, 2);
	const auto plan = PlanSpaceDeparture(craft); REQUIRE(plan.status == SpaceDepartureStatus::Ready);
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded());
	const auto pose = craft.position; const auto units = craft.cargo.TotalCount(); const auto body = craft.mission.target;
	REQUIRE(BeginSpaceMiningContinuation(craft, plan.destination));
	CHECK(craft.position == pose); CHECK(craft.vehstatus.Test(VehState::Stopped)); CHECK(craft.cargo.TotalCount() == units);
	CHECK(craft.space_orders.cursor == 0); CHECK(GetMutableAsteroidBody(body)->mining_slot == VehicleID::Invalid());
	CHECK_FALSE(ResolveEffectiveSpaceOrderTarget(craft)); CHECK(ValidateNativeSpaceOrders());
	rig.Step(); CHECK(craft.position == pose); CHECK(craft.space_orders.cursor == 0);
}

/** Empty holds still retain their admitted mineral and issued history; malformed contained state cannot borrow current native work. */
TEST_CASE("Native mining continuation validation rejects forged pins and hidden progress", "[space-mining-continuation]")
{
	ContinuationMiningRig rig; auto &craft = rig.Configured(); rig.Attach(craft);
	REQUIRE(craft.cargo.TotalCount() == 0); REQUIRE(ValidateNativeSpaceOrders());
	const auto valid = craft.mission.continuation;
	craft.mission.continuation.mineral = CargoLabel{"WICE"}; CHECK_FALSE(ValidateNativeSpaceMiningContinuation(craft));
	craft.mission.continuation = {}; CHECK_FALSE(ValidateNativeSpaceMiningContinuation(craft));
	craft.mission.continuation = valid; craft.mission.continuation.previous_body = OrbitalBodyID{GetAsteroidField().next_body_id};
	CHECK_FALSE(ValidateNativeSpaceMiningContinuation(craft));
	craft.mission.continuation = valid; craft.mission.continuation.route_cursor = 1;
	CHECK_FALSE(ValidateNativeSpaceMiningContinuation(craft));
	craft.mission.continuation = valid; REQUIRE(ValidateNativeSpaceOrders());
	const auto pose = craft.position; const auto source = craft.mission.target;
	CHECK_FALSE(BeginSpaceMiningContinuation(craft, SpacePoint{-1, 0})); CHECK(craft.position == pose);
	CHECK(GetMutableAsteroidBody(source)->mining_slot == craft.index); CHECK(craft.mission.continuation == valid);
}

/** Actual unordered mining keeps its original home/packets/stop choice through the sole saved local departure. */
TEST_CASE("Unordered legacy mining departure preserves the original native return", "[space-mining-continuation][space-legacy-departure]")
{
	ContinuationMiningRig rig; auto &craft = rig.Miner(SPACE_PROBE_MODEL);
	REQUIRE(CmdSetSpaceMissionChecked(DoCommandFlag::Execute, craft.index, craft.generation, 0).Succeeded());
	REQUIRE(craft.space_orders.entries.empty()); rig.MineAtLeast(craft, 2);
	const auto home = craft.home; const auto units = craft.cargo.TotalCount(); const auto packet = craft.cargo.Packets()->front()->index;
	const auto source = craft.cargo.Packets()->front()->GetSource(); const auto pose = craft.position; const auto history = craft.mission.target;
	const auto plan = PlanSpaceDeparture(craft); REQUIRE(plan.status == SpaceDepartureStatus::Ready); REQUIRE(plan.destination != pose);
	const bool stopped = GENERATE(false, true);
	if (stopped) REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded());
	const auto money = Company::Get(craft.owner)->money; const auto cost = craft.mission.cost_remainder;
	REQUIRE(BeginSpaceMiningContinuation(craft, plan.destination));
	CHECK(craft.position == pose); CHECK(craft.home == home); CHECK(craft.vehstatus.Test(VehState::Stopped) == stopped);
	CHECK(Company::Get(craft.owner)->money == money); CHECK(craft.mission.cost_remainder == cost);
	CHECK(craft.mission.work_ticks == 0); CHECK(craft.space_orders == SpaceOrderQueue{}); CHECK(ValidateNativeSpaceOrders());
	CHECK(GetMutableAsteroidBody(history)->mining_slot == VehicleID::Invalid()); CHECK_FALSE(ResolveEffectiveSpaceOrderTarget(craft));
	SECTION("a retired source is history while the original local point survives") {
		// Declared terminal component: fund the exit and erase the released body.
		// This checks history independence before Group28 owns live retirement.
		auto field = GetAsteroidField(); const auto *body = FindAsteroidBody(field, history); REQUIRE(body);
		const std::array exits{AsteroidExitRecord{history, {body->size, body->resource_class}}};
		const auto renewal = ApplyAsteroidExitCredit(field.renewal, exits, field.next_body_id); REQUIRE(renewal);
		field.renewal = *renewal; REQUIRE(EraseAsteroidBody(field, history));
		/** @param record Original live disclosure. @return Whether it belongs to the declared retired body. */
		std::erase_if(field.knowledge, [&](const auto &record) { return record.body == history; });
		REQUIRE(InstallAsteroidField(std::move(field))); CHECK(ValidateNativeSpaceOrders());
		if (stopped) {
			const auto intent = craft.mission.continuation; REQUIRE(craft.Tick());
			CHECK(craft.position == pose); CHECK(craft.mission.continuation == intent);
			REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded());
		}
		// Consume only ordinary native opportunities. The first Return state
		// proves local completion without another homeward move in that tick.
		for (uint tick = 0; craft.mission.continuation.phase != MiningContinuationPhase::None; ++tick) {
			REQUIRE(tick < 1000); REQUIRE(craft.Tick());
		}
		CHECK(craft.position == *plan.destination); CHECK(craft.mission.phase == SpaceMissionPhase::Return);
		CHECK(craft.space_orders == SpaceOrderQueue{}); CHECK(craft.home == home); CHECK(ValidateNativeSpaceOrders());
		CHECK(craft.mission.target == history);
	}
	SECTION("explicit Recall supersedes the retained local departure") {
		REQUIRE(CmdSetSpaceMissionChecked(DoCommandFlag::Execute, craft.index, craft.generation, UINT32_MAX).Succeeded());
		CHECK(craft.mission.continuation == SpaceMiningContinuationState{}); CHECK(craft.mission.phase == SpaceMissionPhase::Return);
		CHECK(craft.space_orders == SpaceOrderQueue{}); CHECK(craft.position == pose);
		CHECK(craft.mission.target == history);
	}
	SECTION("explicit cancellation clears only its own intent") {
		CancelSpaceOrders(craft); CHECK(craft.mission.continuation == SpaceMiningContinuationState{});
		CHECK(craft.space_orders == SpaceOrderQueue{}); CHECK(craft.position == pose);
		CHECK(craft.mission.target == INVALID_ORBITAL_BODY);
	}
	CHECK(craft.cargo.TotalCount() == units); CHECK(craft.cargo.Packets()->front()->index == packet);
	CHECK(craft.cargo.Packets()->front()->GetSource() == source);
}

/** A bounded saved point is a proposal; newly obstructed travel waits locally without inventing an order or berth. */
TEST_CASE("Unordered legacy departure retains a named bounded obstruction wait", "[space-mining-continuation][space-legacy-departure]")
{
	ContinuationMiningRig rig; auto &craft = rig.Miner(SPACE_PROBE_MODEL);
	REQUIRE(CmdSetSpaceMissionChecked(DoCommandFlag::Execute, craft.index, craft.generation, 0).Succeeded()); rig.MineAtLeast(craft, 2);
	const auto units = craft.cargo.TotalCount();
	// Deliberately stale but bounded component waypoint inside the real Core.
	// It grants no passage through the native static footprint after reload.
	const SpacePoint obstructed{static_cast<int32_t>(rig.core->state.x) * SPACE_Q16,
		static_cast<int32_t>(rig.core->state.y) * SPACE_Q16};
	REQUIRE(BeginSpaceMiningContinuation(craft, obstructed));
	// Drive original movement until its swept admission sees the real obstacle;
	// the bounded loop supplies neither a new pose nor an occupancy override.
	for (uint tick = 0; craft.mission.continuation.waiting != MiningContinuationWait::DepartureBlocked; ++tick) {
		REQUIRE(tick < 5000); REQUIRE(craft.Tick());
	}
	CHECK(craft.mission.continuation.retry_ticks == GetSpaceMiningTargetRetryDelay()); CHECK_FALSE(craft.mission.continuation.departure_point);
	CHECK(craft.space_orders == SpaceOrderQueue{}); CHECK(craft.cargo.TotalCount() == units); CHECK(ValidateNativeSpaceOrders());
	const auto pose = craft.position; REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded());
	const auto intent = craft.mission.continuation; REQUIRE(craft.Tick()); CHECK(craft.position == pose); CHECK(craft.mission.continuation == intent);
}

/** Local safety departure delays legacy Mine completion but must not execute or consume a later original instruction. */
TEST_CASE("Typed legacy Mine releases only itself after local departure", "[space-mining-continuation][space-legacy-departure]")
{
	ContinuationMiningRig rig; auto &craft = rig.Miner(SPACE_PROBE_MODEL);
	const bool has_route = GENERATE(false, true);
	if (!has_route) REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Erase, 1,
		SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Succeeded());
	rig.MineAtLeast(craft, 2); const auto entries = craft.space_orders.entries; const auto units = craft.cargo.TotalCount();
	const auto plan = PlanSpaceDeparture(craft); REQUIRE(plan.status == SpaceDepartureStatus::Ready);
	REQUIRE(BeginSpaceMiningContinuation(craft, plan.destination));
	// Complete only local native travel, stopping before another queue dispatch
	// can earn a Visit or perform receiving work under its original owner.
	for (uint tick = 0; craft.space_orders.cursor == 0; ++tick) { REQUIRE(tick < 1000); REQUIRE(craft.Tick()); }
	CHECK(craft.space_orders.cursor == 1); CHECK(craft.space_orders.entries == entries);
	CHECK(craft.mission.continuation == SpaceMiningContinuationState{}); CHECK(craft.cargo.TotalCount() == units);
	CHECK(craft.mission.phase == SpaceMissionPhase::Holding); CHECK(craft.position == *plan.destination); CHECK(ValidateNativeSpaceOrders());
}
