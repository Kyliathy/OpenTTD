/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_return_fixture.h Original mining, blueprint identity, activation and real visit participants for receiver checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group19.
 * Completed-frame receipts are explicit unit inputs, as in manufacturing_fixture.h; no gameplay construction or financial trial is claimed.
 */
#ifndef TESTS_SPACE_RETURN_FIXTURE_H
#define TESTS_SPACE_RETURN_FIXTURE_H
#include "space_mining_fixture.h"
#include "../space_return_target.h"
#include "../vehicle_cmd.h"

/** Native facility/miner participants; initial frame metadata is declared while mining and travel remain actual earned native behavior. */
class SpaceReturnRig : public FactoryMiningRig {
public:
	/** @param site Actual original static owner. @return Its complete generation-qualified route target. */
	static SpaceOrderTarget Target(const SpaceStructure &site) { return {SpaceOrderTargetKind::Structure, site.index.base(), site.generation}; }
	/** Create a declared isolated complete frame using native identity issuance and real activation.
	 * @param kind Supported static module recipe.
	 * @param x Legal original Space-plane blueprint column.
	 * @param y Legal original Space-plane blueprint row.
	 * @return Actual registered operational module; frame consumption/work receipts are declared unit inputs, never claimed as earned construction.
	 */
	SpaceStructure &Frame(SpaceBuildKind kind, uint16_t x, uint16_t y)
	{
		INFO("declared frame kind=" << to_underlying(kind) << " x=" << x << " y=" << y);
		const auto generation = GetNextSpaceStructureGeneration();
		REQUIRE(CmdBuildSpaceBlueprint(DoCommandFlag::Execute, TileXYL(x, y, MapLayer::Space), kind).Succeeded());
		SpaceStructure *made = nullptr;
		// Resolve the identity actually issued by the original blueprint command;
		// the test does not assign a future generation or assume a pool slot.
		for (auto *site : SpaceStructure::Iterate()) if (site->generation == generation) made = site;
		REQUIRE(made != nullptr);
		const auto &definition = *GetSpaceConstructionDefinition(kind);
		made->state.phase = SpaceStructurePhase::FrameComplete;
		made->job.work_ticks = definition.work_ticks;
		made->job.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)};
		REQUIRE(CmdActivateOrbitalModule(DoCommandFlag::Execute, made->index, made->generation).Succeeded());
		const auto ticks = made->module->activation_ticks;
		// The original activation owner advances its actual countdown; no module
		// inventory, source registration or completed activation is assigned here.
		for (uint tick = 0; tick < ticks; ++tick) TickOrbitalConstructionInventory();
		REQUIRE(IsOrbitalModuleOperational(*made)); return *made;
	}
	/** @param craft Actual routed original miner. @param minimum_units Positive wanted test holding, within native capacity. Earn through ordinary extraction without assigning a packet or work counter. */
	void MineAtLeast(SpaceVehicle &craft, uint minimum_units)
	{
		this->Attach(craft); REQUIRE(minimum_units <= craft.cargo_cap);
		// Finite original stock and normal native work must fund the complete
		// offering; the bound prevents a bad fixture from becoming an endless run.
		for (uint tick = 0; craft.cargo.TotalCount() < minimum_units; ++tick) { REQUIRE(tick < 5000); this->Step(); }
	}
	/** Reach a real module through native Visit, then prepare its original pending Mine context and retain the actual berth while stopped.
	 * @param craft Actual original miner; cargo survives every checked queue edit.
	 * @param site Actual owned receiving module.
	 * No position, Holding phase or admitted slot is assigned. Group20 will own the live continuation dispatcher; this adapter uses existing commands and actual travel to test its future arrival producer now.
	 */
	void ReachAndHold(SpaceVehicle &craft, const SpaceStructure &site)
	{
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Clear, 0,
			SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure, site.index.base(), site.generation).Succeeded());
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 0,
			SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure, site.index.base(), site.generation).Succeeded());
		// Actual native movement/approach completion earns the physical arrival;
		// no temporary craft is moved to make an arrival-sensitive quote pass.
		for (uint tick = 0; craft.space_orders.cursor < craft.space_orders.entries.size(); ++tick) { REQUIRE(tick < 5000); this->Step(); }
		REQUIRE(GetSpaceStructureBerth(craft, site));
		this->Route(craft, 0, site);
		REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Succeeded());
		REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded());
		REQUIRE(craft.vehstatus.Test(VehState::Stopped));
		REQUIRE(RequestSpaceApproach(craft, Target(site), true));
		this->Step(); // Stopped gathering retains the new real berth request rather than starting another Mine.
		REQUIRE(GetSpaceStructureBerth(craft, site));
		CHECK(craft.mission.phase == SpaceMissionPhase::Holding);
	}
};
#endif /* TESTS_SPACE_RETURN_FIXTURE_H */
