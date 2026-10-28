/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_policy.cpp Inert policy defaults, checked edits, mineral pinning and cargo-preserving cancellation.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group17.
 * Group23 also exercises the registered command and its portable argument layout after matching persistence passes.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-2-merge-oct28.md Group 23; every saved intent and policy field reaches the fleet and order diagnostics.
 */
#include "../stdafx.h"
#include "probe_mining_fixture.h"
#include "../space_manifest.h"
#include "../space_mining_tuning.h"
#include "../space_structure.h"
#include "../command_func.h"
#include "../safeguards.h"

/** Every policy has an explicit meaning; ordinary instructions cannot carry a hidden mining payload. */
TEST_CASE("Mining policy defaults and continuation choices are explicit", "[space-mining-policy]")
{
	CHECK(SpaceOrder{}.mining_policy == SpaceMiningPolicy::LegacyReturn);
	CHECK(SpaceMission{}.continuation == SpaceMiningContinuationState{});
	CHECK(SpaceMission{}.continuation.mineral.Empty());
	const SpaceOrderTarget body{SpaceOrderTargetKind::Body, 0, 0};
	// Exhaust the finite policy/hold/service table; this query creates no target,
	// packet, service receipt or timer and cannot infer completion from Holding.
	for (const auto policy : {SpaceMiningPolicy::LegacyReturn, SpaceMiningPolicy::ReturnNearest, SpaceMiningPolicy::FullLoadNearest}) {
		CHECK(IsSpaceOrderAllowed(15, {SpaceOrderAction::Mine, body, policy}));
		CHECK(IsSpaceOrderAllowed(SPACE_HEAVY_MINER_MODEL, {SpaceOrderAction::Mine, body, policy}));
		CHECK_FALSE(IsSpaceOrderAllowed(19, {SpaceOrderAction::Mine, body, policy}));
		CHECK(IsSpaceOrderAllowed(15, {SpaceOrderAction::Visit, body, policy}) == (policy == SpaceMiningPolicy::LegacyReturn));
		for (const bool full : {false, true}) for (const bool service : {false, true}) {
			const auto expected = policy == SpaceMiningPolicy::LegacyReturn ? MiningContinuationPhase::None :
				service ? MiningContinuationPhase::NeedService :
				(policy == SpaceMiningPolicy::ReturnNearest || full) ? MiningContinuationPhase::ReturnCargo : MiningContinuationPhase::SeekMineral;
			CHECK(SelectSpaceMiningContinuationPhase(policy, full, service) == expected);
		}
	}
	CHECK_FALSE(IsSpaceOrderAllowed(15, {SpaceOrderAction::Mine, body, SpaceMiningPolicy::End}));
	CHECK(SelectSpaceMiningContinuationPhase(static_cast<SpaceMiningPolicy>(255), true, true) == MiningContinuationPhase::None);
}

/** @param craft Original unit miner. @param body Original existing natural target. Append through the old native command; default policy stays LegacyReturn. */
static void AddPolicyMine(SpaceVehicle &craft, uint32_t body)
{
	REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert,
		static_cast<uint8_t>(craft.space_orders.entries.size()), SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, body, 0).Succeeded());
	CHECK(craft.space_orders.entries.back().mining_policy == SpaceMiningPolicy::LegacyReturn);
}

/** Registered policy dispatch retains exact incarnation and queue index; byte transport and quotes cannot change native work or freight. */
TEST_CASE("Registered mining policy command serializes exact identity and preserves quote purity", "[space-mining-policy]")
{
	using Args = CommandTraits<Commands::SetSpaceMiningPolicy>::Args;
	const Args args{VehicleID{70'000}, UINT64_C(0x1122334455667788), uint8_t{31}, SpaceMiningPolicy::FullLoadNearest};
	const auto bytes = EndianBufferWriter<CommandDataBuffer>::FromValue(args);
	CHECK(bytes.size() == 14); CHECK(EndianBufferReader::ToValue<Args>(bytes) == args);
	CHECK(GetCommandName(Commands::SetSpaceMiningPolicy) == "CmdSetSpaceMiningPolicy");
	ProbeExtractionRig rig; auto &craft = rig.Probe(4, false); AddPolicyMine(craft, 0);
	const auto before = ComputeSpaceVehicleDigest(); const auto orders = ComputeSpaceOrdersDigest();
	CHECK(Command<Commands::SetSpaceMiningPolicy>::Do(DoCommandFlag::QueryCost, craft.index, craft.generation, uint8_t{0}, SpaceMiningPolicy::FullLoadNearest).Succeeded());
	CHECK(ComputeSpaceVehicleDigest() == before); CHECK(ComputeSpaceOrdersDigest() == orders);
	CHECK(Command<Commands::SetSpaceMiningPolicy>::Do(DoCommandFlag::Execute, craft.index, craft.generation + 1, uint8_t{0}, SpaceMiningPolicy::FullLoadNearest).Failed());
	CHECK(ComputeSpaceOrdersDigest() == orders);
	REQUIRE(Command<Commands::SetSpaceMiningPolicy>::Do(DoCommandFlag::Execute, craft.index, craft.generation, uint8_t{0}, SpaceMiningPolicy::FullLoadNearest).Succeeded());
	CHECK(craft.space_orders.entries.front().mining_policy == SpaceMiningPolicy::FullLoadNearest);
	CHECK(craft.mission.continuation == SpaceMiningContinuationState{}); CHECK(craft.cargo.TotalCount() == 0);
}

/** New intent requires an exact owned supported instruction; quotes and refused edits preserve native work and cargo. */
TEST_CASE("Mining policy command validates identity ownership service and quote purity", "[space-mining-policy]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(4, false); AddPolicyMine(craft, 0); AddPolicyMine(craft, 1);
	const auto initial = ComputeSpaceVehicleDigest(); const auto field = GetAsteroidField();
	// All estimation forms must retain the entire native state, even when both
	// Execute and QueryCost are present in the same incoming flags.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::QueryCost}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) {
		CHECK(CmdSetSpaceMiningPolicy(flags, craft.index, craft.generation, 0, SpaceMiningPolicy::FullLoadNearest).Succeeded());
		CHECK(ComputeSpaceVehicleDigest() == initial); CHECK(GetAsteroidField() == field);
	}
	CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, 0, 0, SpaceMiningPolicy::ReturnNearest).Failed());
	CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation + 1, 0, SpaceMiningPolicy::ReturnNearest).Failed());
	CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, VehicleID::Invalid(), craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Failed());
	CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 2, SpaceMiningPolicy::ReturnNearest).Failed());
	CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, static_cast<SpaceMiningPolicy>(255)).Failed());
	{
		AutoRestoreBackup owner(craft.owner, Owner{OWNER_NONE});
		CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Failed());
	}
	{
		AutoRestoreBackup placement(craft.construction.placement, SpacePlacement::SurfaceStored);
		CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Failed());
	}
	{
		AutoRestoreBackup engine(craft.engine_type, EngineID::Invalid());
		CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Failed());
	}
	{
		AutoRestoreBackup status(craft.vehstatus); craft.vehstatus.Set(VehState::Crashed);
		CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Failed());
	}
	{
		AutoRestoreBackup action(craft.space_orders.entries[0].action, SpaceOrderAction::Visit);
		CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Failed());
	}
	// Explicit unit service ownership is enough to test command exclusion; this
	// declares neither paid material nor earned service progress.
	craft.manifest = std::make_unique<SpaceManifest>(); craft.manifest->service_ticks = 1; craft.manifest->service_paused = true;
	CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Failed());
	craft.manifest.reset();
	{
		REQUIRE(SpaceStructure::CanAllocateItem()); auto site = std::unique_ptr<SpaceStructure>(SpaceStructure::Create());
		site->module = std::make_unique<OrbitalModule>(); site->module->service.craft = GetSpaceCraftReference(craft);
		site->module->service.remaining_ticks = 1; site->module->service.paused = true;
		CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Failed());
	}
	CHECK(ComputeSpaceVehicleDigest() == initial); CHECK(GetAsteroidField() == field);
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 1, SpaceMiningPolicy::ReturnNearest).Succeeded());
	CHECK(craft.mission.continuation == SpaceMiningContinuationState{}); CHECK(craft.space_orders.cursor == 0);
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::FullLoadNearest).Succeeded());
	CHECK(craft.mission.continuation.mineral.Empty()); CHECK(craft.cargo.TotalCount() == 0);
	rig.Step();
	CHECK(craft.mission.continuation.mineral == CargoLabel{"NORE"}); CHECK(craft.cargo.TotalCount() == 0);
	CHECK(craft.mission.continuation.previous_body == OrbitalBodyID{0});
	CHECK(AssignSpaceMiningMission(craft, 1, false, SpaceMiningPolicy::FullLoadNearest).Failed());
	CHECK(craft.mission.continuation.mineral == CargoLabel{"NORE"});
	const auto mission = craft.mission; const auto queue = craft.space_orders;
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Succeeded());
	CHECK(craft.mission.continuation == mission.continuation); CHECK(craft.mission.work_ticks == mission.work_ticks);
	CHECK(craft.mission.target == mission.target); CHECK(craft.space_orders.approach == queue.approach);
	CHECK(craft.space_orders.started == queue.started); CHECK(craft.space_orders.cursor == queue.cursor);
}

/** Explicit Recall and existing queue replacement/clear cancel the associated intent while original earned freight survives. */
TEST_CASE("Mining policy cancellation retains original mined packets", "[space-mining-policy]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(4, false); AddPolicyMine(craft, 0);
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::FullLoadNearest).Succeeded());
	rig.Attach(craft);
	// Earn the packet through ordinary work; no test-assigned stock or completed
	// work counter participates in cancellation conservation.
	for (uint tick = 0; tick < 74; ++tick) rig.Step();
	REQUIRE(craft.cargo.TotalCount() == 2);
	const auto *packet = craft.cargo.Packets()->front(); const auto id = packet->index; const auto body = craft.mission.target;
	const auto source = packet->GetSource(); const auto before = ComputeSpaceVehicleDigest();
	CHECK_FALSE(craft.mission.continuation.mineral.Empty());
	CHECK(CmdSetSpaceMissionChecked(DoCommandFlag::QueryCost, craft.index, craft.generation, UINT32_MAX).Succeeded());
	CHECK(ComputeSpaceVehicleDigest() == before);
	SECTION("Recall") { REQUIRE(CmdSetSpaceMissionChecked(DoCommandFlag::Execute, craft.index, craft.generation, UINT32_MAX).Succeeded()); }
	SECTION("Clear") { REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Clear, 0,
		SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, 0, 0).Succeeded()); }
	SECTION("Replace") {
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Replace, 0,
			SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, 0, 0).Succeeded());
		CHECK(craft.space_orders.entries.front().mining_policy == SpaceMiningPolicy::LegacyReturn);
	}
	CHECK(craft.mission.continuation == SpaceMiningContinuationState{});
	CHECK(craft.cargo.TotalCount() == 2); CHECK(craft.cargo.Packets()->front()->index == id);
	CHECK(craft.cargo.Packets()->front()->GetSource() == source);
	CHECK(FindAsteroidBody(GetAsteroidField(), body)->mining_slot == VehicleID::Invalid());
}

/** Opting a running legacy instruction into continuation pins only its admitted source and does not restart physical pursuit. */
TEST_CASE("Active legacy mining can acquire explicit policy without restarting work", "[space-mining-policy]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(4, false); AddPolicyMine(craft, 0); rig.Step();
	REQUIRE(craft.space_orders.started); REQUIRE(craft.mission.continuation == SpaceMiningContinuationState{});
	const auto mission = craft.mission; const auto approach = craft.space_orders.approach;
	const auto field = GetAsteroidField(); const auto digest = ComputeSpaceVehicleDigest();
	CHECK(CmdSetSpaceMiningPolicy(DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Succeeded());
	CHECK(ComputeSpaceVehicleDigest() == digest);
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Succeeded());
	CHECK(craft.mission.continuation.mineral == CargoLabel{"NORE"}); CHECK(craft.cargo.TotalCount() == 0);
	CHECK(craft.mission.target == mission.target); CHECK(craft.mission.phase == mission.phase);
	CHECK(craft.mission.approach_ticks_left == mission.approach_ticks_left); CHECK(craft.mission.work_ticks == mission.work_ticks);
	CHECK(craft.space_orders.approach == approach); CHECK(GetAsteroidField() == field);
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::LegacyReturn).Succeeded());
	CHECK(craft.mission.continuation == SpaceMiningContinuationState{});
}

/** Each saved contained-intent field changes the fleet diagnostic, and a configured policy the order diagnostic; restoring
 * the value restores the diagnostic (m2o28 Group 23).
 */
TEST_CASE("Fleet and order diagnostics change with every saved mining intent field", "[space-mining-policy]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(4, false); AddPolicyMine(craft, 0);
	const auto fleet = ComputeSpaceVehicleDigest(); const auto orders = ComputeSpaceOrdersDigest();
	const auto saved = craft.mission.continuation;
	/** @param mutate One saved-field change to the contained intent. @return Whether the fleet diagnostic differs, after which the intent is restored. */
	const auto changes = [&](auto mutate) {
		mutate(craft.mission.continuation); const bool changed = ComputeSpaceVehicleDigest() != fleet;
		craft.mission.continuation = saved; return changed;
	};
	CHECK(changes([](SpaceMiningContinuationState &c) { c.phase = MiningContinuationPhase::SeekMineral; }));
	CHECK(changes([](SpaceMiningContinuationState &c) { c.mineral = CargoLabel{"NORE"}; }));
	CHECK(changes([](SpaceMiningContinuationState &c) { c.previous_body = OrbitalBodyID{1}; }));
	CHECK(changes([](SpaceMiningContinuationState &c) { c.effective_target = SpaceOrderTarget{SpaceOrderTargetKind::Body, 1, 0}; }));
	CHECK(changes([](SpaceMiningContinuationState &c) { c.departure_point = SpacePoint{16, 16}; }));
	CHECK(changes([](SpaceMiningContinuationState &c) { c.retry_ticks = 3; }));
	CHECK(changes([](SpaceMiningContinuationState &c) { c.waiting = MiningContinuationWait::NoCompatibleAsteroid; }));
	CHECK(changes([](SpaceMiningContinuationState &c) { c.route_cursor = 0; }));
	CHECK(changes([](SpaceMiningContinuationState &c) { c.travel_ticks = 9; }));
	// A target of another generation is a different incarnation.
	auto with_target = saved; with_target.phase = MiningContinuationPhase::SeekMineral;
	with_target.effective_target = SpaceOrderTarget{SpaceOrderTargetKind::Body, 1, 0};
	craft.mission.continuation = with_target; const auto generation0 = ComputeSpaceVehicleDigest();
	craft.mission.continuation.effective_target->generation = 1; CHECK(ComputeSpaceVehicleDigest() != generation0);
	craft.mission.continuation = saved; CHECK(ComputeSpaceVehicleDigest() == fleet);
	// A configured Mine policy is order state; the default keeps the old stream.
	craft.space_orders.entries.front().mining_policy = SpaceMiningPolicy::FullLoadNearest; CHECK(ComputeSpaceOrdersDigest() != orders);
	craft.space_orders.entries.front().mining_policy = SpaceMiningPolicy::LegacyReturn; CHECK(ComputeSpaceOrdersDigest() == orders);
}

