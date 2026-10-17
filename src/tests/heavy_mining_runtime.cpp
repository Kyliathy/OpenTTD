/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_mining_runtime.cpp Per-model native extraction, finite reserve, return admission and actual allowance settlement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9C
 * Isolated frames, packet stock and cargo dimensions are declared unit inputs; this file claims no gameplay procurement or Surface pod producer.
 */
#include "../stdafx.h"
#include "manufacturing_fixture.h"
#include "../space_mining_tuning.h"
#include "../space_manifest.h"
#include "../space_vehicle_cmd.h"
#include "../asteroid.h"
#include "../safeguards.h"

/** Real isolated paid factory products and generated finite bodies, with explicit physical unit dimensions and restored original field state. */
class HeavyMiningRuntimeRig : public ManufacturingMaterializationRig {
	AsteroidField previous = GetAsteroidField(); ///< Caller resource owner, restored only after this fixture's physical hulls are gone.
	AutoRestoreBackup<uint8_t> ore_weight{CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"NORE"}))->weight, uint8_t{16}}; ///< Explicit one-tonne metallic unit.
	AutoRestoreBackup<uint8_t> rare_weight{CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"RARE"}))->weight, uint8_t{32}}; ///< Deliberate two-tonne rare unit exercises actual mass refits.
	AutoRestoreBackup<uint8_t> ice_weight{CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"WICE"}))->weight, uint8_t{16}}; ///< Explicit one-tonne ice unit.
public:
	/** Install an actual bounded generated field; no reserve is changed by a cargo/mission observer. */
	HeavyMiningRuntimeRig() { REQUIRE(GenerateWorldAsteroids(12345)); }
	/** Retire all isolated native hull owners before restoring the caller's resource record and cargo dimensions. */
	~HeavyMiningRuntimeRig()
	{
		_vehicle_pool.CleanPool(); ResetVehicleHash(); CHECK(InstallAsteroidField(std::move(this->previous)));
	}
	/** Materialize a native hull through the original actual paid-input/common-work owner.
	 * @param model Canonical role under test.
	 * @return Original product after retiring only its finished factory job.
	 */
	SpaceVehicle &Build(uint16_t model)
	{
		auto &job = this->Start(model); this->Work(GetSpaceVehicleConstruction(model)->work_ticks);
		REQUIRE(job.phase == OrbitalManufacturePhase::Complete); auto *craft = ResolveSpaceCraft(job.product); REQUIRE(craft != nullptr);
		REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, assembly->index, assembly->generation, core->index, core->generation).Succeeded());
		return *craft;
	}
	/** Assign an explicit real Mine followed by a generation-qualified operational module Unload.
	 * @param craft Actual native hull.
	 * @param body Existing natural body; starter0/1/2 cover the three resource classes.
	 * @param receiver Actual destination, which can subsequently become unavailable without deleting the hold.
	 */
	void Route(SpaceVehicle &craft, uint32_t body, const SpaceStructure &receiver)
	{
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, body, 0).Succeeded());
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 0, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, body, 0).Succeeded());
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 1, SpaceOrderAction::Unload, SpaceOrderTargetKind::Structure, receiver.index.base(), receiver.generation).Succeeded());
	}
	/** Offer one native motion/admission/vehicle opportunity, keeping actual finite resource, cargo and money owners authoritative. */
	void Step()
	{
		++TimerGameTick::counter; TickAsteroidField(); UpdateAttachedSpaceCraftTransforms(); PrepareSpaceApproachTick();
		// Stable native pool order matches the real fleet driver; a second hull cannot obtain a second copy of the same attachment.
		for (auto *craft : SpaceVehicle::Iterate()) REQUIRE(craft->Tick());
	}
	/** @param ticks Exact number of native opportunities; no progress or clock field is assigned by the test. */
	void Steps(uint ticks) { for (uint tick = 0; tick < ticks; ++tick) this->Step(); }
	/** @param craft Actual routed miner. @return Its original body after normal pursuit acquires the sole attachment. */
	AsteroidBody &Attach(SpaceVehicle &craft)
	{
		// Bound the actual moving-target pursuit; this unit path uses normal movement rather than placing the hull on an attachment coordinate.
		for (uint tick = 0; craft.mission.phase != SpaceMissionPhase::Mining; ++tick) { REQUIRE(tick < 5000); this->Step(); }
		auto *body = GetMutableAsteroidBody(craft.mission.target); REQUIRE(body != nullptr); REQUIRE(body->mining_slot == craft.index); return *body;
	}
};

/** The new runtime capability cannot be forged into a commissioning-only source or borrowed by the preserved constructor/survey/hauler roles. */
TEST_CASE("Heavy mining role and source boundary reject future or unrelated progress", "[heavy-mining-runtime]")
{
	const SpaceOrder mine{SpaceOrderAction::Mine, {SpaceOrderTargetKind::Body, 0, 0}};
	CHECK(IsSpaceOrderAllowed(21, mine)); CHECK(IsSpaceOrderAllowed(15, mine));
	// The established non-mining roles retain their own actions even when a native record contains plausible mining timer values.
	for (uint16_t model : {14, 16, 17, 19, 20, 22}) {
		CHECK_FALSE(IsSpaceOrderAllowed(model, mine)); CHECK_FALSE(IsSpaceMiningStateSupported(model, SpaceMissionPhase::Mining, 1, 4, true));
	}
	CHECK_FALSE(IsSpaceOrderAllowed(21, {SpaceOrderAction::Survey, mine.target}));
	CHECK_FALSE(IsSpaceOrderAllowed(21, {SpaceOrderAction::Construct, {SpaceOrderTargetKind::Blueprint, 0, 1}}));
	CHECK(IsSpaceMiningStateSupported(21, SpaceMissionPhase::Holding, 0, 0, false));
	CHECK_FALSE(IsSpaceMiningStateSupported(21, SpaceMissionPhase::Mining, 37, 4, false));
	CHECK_FALSE(IsSpaceMiningStateSupported(21, SpaceMissionPhase::Holding, 0, 4, false));
	CHECK(IsSpaceMiningStateSupported(21, SpaceMissionPhase::Mining, 73, 4, true));
	CHECK_FALSE(IsSpaceMiningStateSupported(21, SpaceMissionPhase::Mining, 74, 4, true));
	CHECK(IsSpaceMiningStateSupported(15, SpaceMissionPhase::Mining, 73, 2, false));
	CHECK_FALSE(IsSpaceMiningStateSupported(21, static_cast<SpaceMissionPhase>(255), 0, 0, true));
}

/** Actual forty-tonne extraction follows a moving body, charges the declared rate, retains a congested whole hold and spends exactly one real orbital return. */
TEST_CASE("Paid heavy miner extracts finite packets and settles its explicit orbital return once", "[heavy-mining-runtime]")
{
	HeavyMiningRuntimeRig rig; AutoRestoreBackup running(_pause_mode, PauseModes{});
	auto &craft = rig.Build(21); rig.Route(craft, 0, *rig.core); REQUIRE(craft.manifest != nullptr); REQUIRE(craft.manifest->pods.empty());
	REQUIRE(GetSpaceMiningBlock(craft) == SpaceOrderBlock::None); auto &body = rig.Attach(craft);
	const auto reserve = body.remaining_reserve; const auto original_position = body.motion.position;
	craft.vehstatus.Set(VehState::Stopped); const auto idle_profit = craft.profit_this_year;
	rig.Steps(74); CHECK(body.motion.position != original_position); CHECK(craft.mission.work_ticks == 0); CHECK(body.remaining_reserve == reserve);
	CHECK(craft.profit_this_year == idle_profit - Money{200} * 256); craft.vehstatus.Reset(VehState::Stopped);
	const auto active_profit = craft.profit_this_year; rig.Steps(74);
	CHECK(craft.cargo.TotalCount() == 4); CHECK(body.remaining_reserve == reserve - 4); CHECK(craft.extracted_units == 4); CHECK(craft.mission.work_ticks == 0);
	CHECK(craft.profit_this_year == active_profit - Money{1000} * 256); CHECK(craft.manifest->returns_remaining == 2);
	// Nine further complete intervals fill the same original40t hold; the final reserve debit must release only this hull's actual lease.
	rig.Steps(9 * 74); CHECK(craft.cargo.TotalCount() == 40); CHECK(craft.extracted_units == 40); CHECK(body.remaining_reserve == reserve - 40);
	CHECK(body.mining_slot == VehicleID::Invalid()); CHECK(craft.mission.phase == SpaceMissionPhase::Return); CHECK(craft.mission.work_ticks == 0);
	rig.Add(*rig.core, CargoLabel{"NORE"}, 1000); // Explicit unit congestion; no native procurement/production acceptance is claimed.
	// Pursue the actual module berth and wait on its original whole-transfer headroom, retaining every extracted unit and the unspent allowance.
	for (uint tick = 0; craft.space_orders.blocked != SpaceOrderBlock::ProducerUnavailable; ++tick) { REQUIRE(tick < 5000); rig.Step(); }
	CHECK(craft.space_orders.cursor == 1); CHECK(craft.cargo.TotalCount() == 40); CHECK(craft.manifest->returns_remaining == 2);
	REQUIRE(GetSpaceStructureBerth(craft, *rig.core).has_value()); rig.Steps(10); CHECK(craft.extracted_units == 40); CHECK(craft.cargo.TotalCount() == 40);
	const auto cargo = craft.cargo_type; REQUIRE(HubCargoTransfer::Consume(rig.core->module->stock[cargo], 16));
	const auto before_quote = ComputeSpaceVehicleDigest();
	// Both ordinary and mixed execution-cost estimates must retain actual packets, work, funds and the same original return quota.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) {
		REQUIRE(CmdMoveOrbitalFreight(flags, rig.core->index, rig.core->generation, craft.index, craft.generation, cargo, 40, false).Succeeded());
		CHECK(ComputeSpaceVehicleDigest() == before_quote); CHECK(craft.manifest->returns_remaining == 2);
	}
	rig.Step(); CHECK(craft.cargo.TotalCount() == 0); CHECK(rig.core->module->stock[cargo].TotalCount() == 1024); CHECK(craft.manifest->returns_remaining == 1);
	CHECK(craft.space_orders.blocked == SpaceOrderBlock::Completed); CHECK(body.remaining_reserve == reserve - 40);
	rig.Steps(2); CHECK(craft.manifest->returns_remaining == 1); CHECK(craft.manifest->services_completed == 0); CHECK(craft.manifest->pods.empty());
}

/** Resource assignment obeys actual CargoSpec mass and loaded-cargo compatibility; unavailable routes and old small-miner pod rules remain strict. */
TEST_CASE("Heavy admission requires a real mineral return and preserves whole loaded refits", "[heavy-mining-runtime]")
{
	HeavyMiningRuntimeRig rig; AutoRestoreBackup running(_pause_mode, PauseModes{}); auto &craft = rig.Build(21);
	REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 0, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, 2, 0).Succeeded());
	CHECK(GetSpaceMiningBlock(craft) == SpaceOrderBlock::PodsMissing); CHECK(AssignSpaceMiningMission(craft, 2, true).Failed());
	rig.Route(craft, 2, *rig.assembly); CHECK(GetSpaceMiningBlock(craft) == SpaceOrderBlock::PodsMissing); // An operational Assembly has no mineral inventory; a paid frame is not a receiver.
	rig.Route(craft, 2, *rig.core); REQUIRE(GetSpaceMiningBlock(craft) == SpaceOrderBlock::None);
	{
		AutoRestoreBackup incarnation(craft.space_orders.entries[1].target.generation, rig.core->generation + 1);
		CHECK(GetSpaceMiningBlock(craft) == SpaceOrderBlock::PodsMissing);
	}
	{
		AutoRestoreBackup foreign(rig.core->state.owner, Owner{1}); CHECK(GetSpaceMiningBlock(craft) == SpaceOrderBlock::PodsMissing);
	}
	{
		AutoRestoreBackup pending(craft.manifest->pending_returns, uint8_t{2}); CHECK(GetSpaceMiningBlock(craft) == SpaceOrderBlock::ReturnsPending);
	}
	{
		AutoRestoreBackup service(craft.manifest->service_ticks, uint16_t{1}); CHECK(GetSpaceMiningBlock(craft) == SpaceOrderBlock::ServiceRequired);
	}
	const auto digest = ComputeSpaceVehicleDigest(); REQUIRE(AssignSpaceMiningMission(craft, 2, false).Succeeded()); CHECK(ComputeSpaceVehicleDigest() == digest);
	CHECK(CmdSetSpaceMission(DoCommandFlag::Execute, craft.index, 2).Failed()); CHECK(ComputeSpaceVehicleDigest() == digest);
	// Rich starter2 owns RARE; Carbonaceous starter1 owns WICE. Resolve the actual resource before expecting the declared two-tonne rare unit's20-unit capacity.
	rig.Attach(craft); CHECK(craft.cargo_cap == 20); CHECK(craft.cargo_type == GetCargoTypeByLabel(CargoLabel{"RARE"})); rig.Steps(74);
	REQUIRE(craft.cargo.TotalCount() == 4); const auto held = ComputeSpaceVehicleDigest();
	CHECK(AssignSpaceMiningMission(craft, 0, true).Failed()); CHECK(ComputeSpaceVehicleDigest() == held);
	craft.manifest->returns_remaining = 0; // Explicit exhausted unit input; integrated acceptance must exhaust real batches.
	CHECK(GetSpaceMiningBlock(craft) == SpaceOrderBlock::ServiceRequired); CHECK(AssignSpaceMiningMission(craft, 2, true).Failed());
	REQUIRE(AssignSpaceMiningMission(craft, UINT32_MAX, true).Succeeded()); CHECK(craft.cargo.TotalCount() == 4);
	CHECK(GetMutableAsteroidBody(OrbitalBodyID{2})->mining_slot == VehicleID::Invalid());
	auto &small = rig.Build(15); rig.Route(small, 0, *rig.core); CHECK(GetSpaceMiningBlock(small) == SpaceOrderBlock::PodsMissing);
}

/** A second real heavy hull must wait for the same exclusive moving attachment; recall preserves the first hold and releases only its original lease. */
TEST_CASE("Heavy miners cannot duplicate an occupied finite asteroid attachment", "[heavy-mining-runtime]")
{
	HeavyMiningRuntimeRig rig; AutoRestoreBackup running(_pause_mode, PauseModes{}); auto &first = rig.Build(21);
	rig.Route(first, 0, *rig.core); auto &body = rig.Attach(first); const auto reserve = body.remaining_reserve;
	auto &second = rig.Build(21); rig.Route(second, 0, *rig.core); rig.Steps(74);
	CHECK(body.mining_slot == first.index); CHECK(first.cargo.TotalCount() == 4); CHECK(second.cargo.TotalCount() == 0);
	CHECK(second.mission.phase == SpaceMissionPhase::Approach); CHECK(second.mission.work_ticks == 0); CHECK(body.remaining_reserve == reserve - 4);
	REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, first.index, UINT32_MAX).Succeeded());
	CHECK(first.cargo.TotalCount() == 4); CHECK(first.mission.work_ticks == 0); CHECK(body.mining_slot == VehicleID::Invalid());
	rig.Attach(second); CHECK(body.mining_slot == second.index); CHECK(first.cargo.TotalCount() == 4); CHECK(second.extracted_units == 0);
}
