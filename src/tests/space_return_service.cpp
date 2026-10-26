/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_return_service.cpp Real service admission, original paid work and retained berth ownership for automatic return queries.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group19.
 * Parent factory/frame/parts inputs and explicitly named retained-job boundaries are unit fixtures, not a gameplay procurement claim.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "space_return_fixture.h"
#include "../orbital_service.h"
#include "../safeguards.h"

/** Only an exhausted original return allowance and a complete original parts/power owner can admit automatic service. */
TEST_CASE("Automatic module service spends original parts and earns allowance only through work", "[space-return-service]")
{
	SpaceReturnRig rig; auto &craft = rig.Miner(); const auto allowance = craft.manifest->returns_remaining;
	rig.MineAtLeast(craft, allowance + 1); rig.ReachAndHold(craft, *rig.core);
	const auto target = SpaceReturnRig::Target(*rig.core); const auto tuning = GetSpaceServiceTuning(SPACE_HEAVY_MINER_MODEL); REQUIRE(tuning);
	CHECK_FALSE(GetSpaceReturnCandidateAdmission(craft, target, SpaceReturnPurpose::Service));
	// Original positive module exchanges each spend one real return. Retained
	// ore remains aboard after exhaustion; no quota or earned work is assigned.
	for (uint count = 0; count < allowance; ++count) {
		REQUIRE(CmdMoveOrbitalFreight(DoCommandFlag::Execute, rig.core->index, rig.core->generation,
			craft.index, craft.generation, craft.cargo_type, 1, false).Succeeded());
	}
	REQUIRE(craft.manifest->returns_remaining == 0); REQUIRE(craft.cargo.TotalCount() != 0);
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded());
	REQUIRE_FALSE(craft.vehstatus.Test(VehState::Stopped));
	// Clear only explicitly supplied unit factory leftovers before proving the
	// receiver's complete immutable bill; no controller can invent missing parts.
	for (uint slot = 0; slot < tuning->inputs.size(); ++slot) {
		auto &stock = rig.core->module->stock[GetSpaceSupplyCargo(static_cast<SpaceSupply>(slot + 1))];
		if (stock.TotalCount() != 0) REQUIRE(HubCargoTransfer::Consume(stock, stock.TotalCount()));
	}
	CHECK_FALSE(GetSpaceReturnCandidateAdmission(craft, target, SpaceReturnPurpose::Service));
	rig.Add(*rig.core, CargoLabel{"RFMT"}, tuning->inputs[0]); rig.Add(*rig.core, CargoLabel{"ELEC"}, tuning->inputs[1]);
	const auto fleet = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
	const auto cargo = craft.cargo.TotalCount(); const auto completed = craft.manifest->services_completed;
	REQUIRE(GetSpaceReturnCandidateAdmission(craft, target, SpaceReturnPurpose::Service));
	CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == power);
	REQUIRE(ExecuteSpaceReturnArrival(craft, target, SpaceReturnPurpose::Service));
	REQUIRE(GetOrbitalServiceJobState(craft)); CHECK(GetOrbitalServiceJobState(craft)->remaining_ticks == tuning->ticks);
	CHECK(craft.manifest->returns_remaining == 0); CHECK(craft.manifest->services_completed == completed);
	CHECK_FALSE(ExecuteSpaceReturnArrival(craft, target, SpaceReturnPurpose::Service));
	PrepareSpaceApproachTick(); REQUIRE(GetSpaceStructureBerth(craft, *rig.core));
	// Real common power allocation and the original retained job own each
	// service opportunity. The final work step alone consumes parts and renews.
	for (uint tick = 0; tick < tuning->ticks; ++tick) { rig.Allocate(); TickOrbitalServices(); }
	CHECK_FALSE(HasOrbitalServiceAssignment(craft)); CHECK(craft.manifest->services_completed == completed + 1);
	CHECK(craft.manifest->returns_remaining == tuning->returns); CHECK(craft.cargo.TotalCount() == cargo);
	CHECK(craft.space_orders.cursor == 0);
}

/** Exact lost-owner service receipts retain their own slots; a Dock's independently free second slot remains usable. */
TEST_CASE("Retained service generations block only their original physical berth", "[space-return-service]")
{
	SpaceReturnRig rig; auto &craft = rig.Miner();
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Succeeded());
	auto &dock = rig.Frame(SpaceBuildKind::Dock, 2, 4);
	const auto core_target = SpaceReturnRig::Target(*rig.core), dock_target = SpaceReturnRig::Target(dock);
	REQUIRE(CanAcquireSpaceApproach(craft, core_target)); REQUIRE(CanAcquireSpaceApproach(craft, dock_target));
	// These explicit retained-job unit inputs model loss of the old incarnation
	// while its numeric ID exists again. They grant no parts, work or service.
	const SpaceCraftRef lost{craft.index, craft.generation + 1};
	auto &core_job = *rig.core->module->GetServiceJobs()[0]; core_job.craft = lost; core_job.remaining_ticks = 1;
	CHECK_FALSE(CanAcquireSpaceApproach(craft, core_target));
	auto jobs = dock.module->GetServiceJobs(); jobs[0]->craft = lost; jobs[0]->remaining_ticks = 1;
	CHECK(CanAcquireSpaceApproach(craft, dock_target));
	CHECK(GetSpaceReturnCandidateAdmission(craft, dock_target, SpaceReturnPurpose::Cargo));
	jobs[1]->craft = lost; jobs[1]->remaining_ticks = 1;
	CHECK_FALSE(CanAcquireSpaceApproach(craft, dock_target));
	CHECK_FALSE(GetSpaceReturnCandidateAdmission(craft, dock_target, SpaceReturnPurpose::Cargo));
	// Remove only declared empty unit receipts before normal fixture teardown;
	// no real retained packet/job is discarded by production code or this test.
	static_cast<OrbitalServiceState &>(core_job) = {};
	for (auto *job : jobs) static_cast<OrbitalServiceState &>(*job) = {};
}
