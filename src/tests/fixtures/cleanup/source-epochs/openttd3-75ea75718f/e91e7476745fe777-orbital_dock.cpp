/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_dock.cpp Paid-frame Dock activation, explicit original source admission and bounded service transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4C.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md prerequisite queue.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_approach.h"
#include "../orbital_service.h"
#include "../core/backup_type.hpp"
#include "excavation_fixture.h"
#include "../safeguards.h"

/** Build isolated boundary data; this never substitutes for transported native acceptance.
 * @param kind Canonical module recipe. @param index Explicit test identity and positive next incarnation.
 * @param x Test plane X. @param y Test plane Y.
 * @return Exact consumed inert frame, with no packet, panel, connection or service grant.
 */
static SpaceStructureRecord DockBoundaryFrame(SpaceBuildKind kind, uint16_t index, uint16_t x, uint16_t y)
{
	const auto &definition = *GetSpaceConstructionDefinition(kind);
	SpaceStructureRecord row;
	row.ref = {SpaceStructureID{index}, uint64_t{index} + 1}; row.state.owner = _current_company; row.state.kind = kind;
	row.state.x = x; row.state.y = y; row.state.capital_paid = definition.capital;
	row.state.phase = SpaceStructurePhase::FrameComplete; row.work_ticks = definition.work_ticks;
	row.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)};
	return row;
}

/** Real profile/companies and isolated consumed receipts; no gameplay delivery, material or hull is manufactured by this fixture. */
class OrbitalDockRig {
	ExcavationFixture world;
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{1995}};
public:
	/** Restore test frames through the public physical owner at legal Core-adjacent footprints.
	 * @param independent Add an unrelated Core/Storage pair for derived-dependency boundary checks.
	 */
	OrbitalDockRig(bool independent = false)
	{
		ResetUtilityPowerState(); ResetSpaceStructures();
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1995}, 0, 1);
		// The southeast pair preserves every south approach while allowing Dock and Power to share a real orthogonal edge.
		std::vector rows{DockBoundaryFrame(SpaceBuildKind::Core, 0, 8, 4), DockBoundaryFrame(SpaceBuildKind::Dock, 1, 9, 7),
			DockBoundaryFrame(SpaceBuildKind::Power, 2, 11, 6)};
		if (independent) {
			rows.push_back(DockBoundaryFrame(SpaceBuildKind::Core, 3, 20, 4));
			rows.push_back(DockBoundaryFrame(SpaceBuildKind::Storage, 4, 23, 4));
		}
		REQUIRE(InstallLoadedSpaceStructures(rows, rows.size() + 1));
	}
	/** Discard only isolated empty physical owners and utility accounts before restoring the fixture's original content/company state. */
	~OrbitalDockRig() { _space_structure_pool.CleanPool(); ResetSpaceStructures(); ResetUtilityPowerState(); }
	/** Advance the existing activation owner; no independent timer or energy publication is introduced. */
	void ActivateCountdown()
	{
		for (uint tick = 0; tick < GetMultiLayerSpaceBudget().core_activation_ticks; ++tick) TickOrbitalConstructionInventory();
	}
};

/** Estimates and completed activation must preserve native receipts and issue no Dock panel or free energy. */
TEST_CASE("Dock activation grants two bounded physical berths and no electrical source", "[orbital-dock-contract]")
{
	OrbitalDockRig rig;
	auto &core = *SpaceStructure::Get(SpaceStructureID{0}); auto &dock = *SpaceStructure::Get(SpaceStructureID{1});
	CHECK(CmdActivateOrbitalModule({}, dock.index, dock.generation).Failed());
	REQUIRE(CmdActivateOrbitalCore({DoCommandFlag::Execute}, core.index, core.generation).Succeeded()); rig.ActivateCountdown();
	const auto before = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	CHECK(CmdActivateOrbitalModule({}, dock.index, dock.generation).Succeeded());
	CHECK(CmdActivateOrbitalModule({}, dock.index, dock.generation + 1).Failed());
	CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeUtilityPowerDigest() == power);
	REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, dock.index, dock.generation).Succeeded());
	CHECK(dock.module->producer == UtilityProducerID{UINT32_MAX}); CHECK(GetSpaceStructureApproachCapacity(dock) == 1);
	rig.ActivateCountdown();
	CHECK(IsOrbitalModuleOperational(dock)); CHECK(GetSpaceStructureApproachCapacity(dock) == 2);
	CHECK_FALSE(GetSpaceStructureApproachSlot(dock, 2)); CHECK(GetOrbitalModulePanelRate(dock) == 0);
	CHECK(CaptureUtilityPowerState().producers.size() == 1); CHECK(CaptureUtilityPowerState().consumers.empty());
	CHECK(CmdCommissionOrbitalServices({}, dock.index, dock.generation).Failed());
	CHECK(CmdActivateOrbitalModule({DoCommandFlag::Execute}, dock.index, dock.generation).Failed());
	CHECK(Company::Get(_current_company)->money == cash); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	REQUIRE(ValidateSpaceStructures());
}

/** Explicit physical connections register one immutable bounded consumer without granting any generation or use. */
TEST_CASE("Dock service connectivity binds an original paid adjacent panel without energy", "[orbital-dock-contract]")
{
	OrbitalDockRig rig;
	auto &core = *SpaceStructure::Get(SpaceStructureID{0}); auto &dock = *SpaceStructure::Get(SpaceStructureID{1}); auto &power = *SpaceStructure::Get(SpaceStructureID{2});
	REQUIRE(CmdActivateOrbitalCore({DoCommandFlag::Execute}, core.index, core.generation).Succeeded()); rig.ActivateCountdown();
	REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, dock.index, dock.generation).Succeeded());
	REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, power.index, power.generation).Succeeded()); rig.ActivateCountdown();
	const auto state = ComputeSpaceStructureDigest(), ledger = ComputeUtilityPowerDigest();
	CHECK(CmdConnectOrbitalServices({}, dock.index, dock.generation, power.index, power.generation).Succeeded());
	CHECK(CmdConnectOrbitalServices({}, dock.index, dock.generation, power.index, power.generation + 1).Failed());
	CHECK(CmdConnectOrbitalServices({}, power.index, power.generation, core.index, core.generation).Failed());
	CHECK(ComputeSpaceStructureDigest() == state); CHECK(ComputeUtilityPowerDigest() == ledger);
	REQUIRE(CmdConnectOrbitalServices({DoCommandFlag::Execute}, dock.index, dock.generation, power.index, power.generation).Succeeded());
	CHECK(dock.module->utility_source == GetSpaceStructureReference(power)); CHECK(GetOrbitalServicePowerSite(dock) == &power);
	const auto *consumer = GetUtilityPowerConsumer(dock.module->consumer); REQUIRE(consumer != nullptr);
	CHECK(consumer->capacity_j == 1500); CHECK(consumer->linked_producers == std::vector{power.module->producer});
	CHECK(consumer->meter.granted_energy_j == 0); CHECK(consumer->shares.empty()); CHECK(CaptureUtilityPowerState().producers.size() == 2);
	CHECK(CmdConnectOrbitalServices({DoCommandFlag::Execute}, dock.index, dock.generation, core.index, core.generation).Failed());
	REQUIRE(ValidateSpaceStructures());
	auto copied = CaptureSpaceStructures();
	// Independently malformed transport may neither invent a Dock panel nor attribute a connected consumer to the wrong source incarnation.
	copied[1].module->producer = core.module->producer; CHECK_FALSE(ValidateSpaceStructureRecords(copied, 4, 64, 64));
	copied = CaptureSpaceStructures(); ++copied[1].module->utility_source.generation;
	CHECK_FALSE(ValidateOrbitalServiceUtilityRecords(copied, CaptureUtilityPowerState()));
	copied = CaptureSpaceStructures(); copied[0].module->dock_service.remaining_ticks = 1;
	CHECK_FALSE(ValidateSpaceStructureRecords(copied, 4, 64, 64));
	const auto old_consumer = dock.module->consumer; const auto connected = ComputeSpaceStructureDigest(), connected_power = ComputeUtilityPowerDigest();
	CHECK(CmdDisconnectOrbitalServices({}, dock.index, dock.generation).Succeeded());
	CHECK(ComputeSpaceStructureDigest() == connected); CHECK(ComputeUtilityPowerDigest() == connected_power);
	REQUIRE(CmdDisconnectOrbitalServices({DoCommandFlag::Execute}, dock.index, dock.generation).Succeeded());
	CHECK_FALSE(GetUtilityPowerConsumer(old_consumer)->active); CHECK(GetUtilityPowerConsumer(old_consumer)->linked_producers.empty());
	CHECK(dock.module->utility_source == SpaceStructureRef{}); CHECK(dock.module->consumer == UtilityConsumerID{UINT32_MAX});
	CHECK(CmdDisconnectOrbitalServices({}, dock.index, dock.generation).Failed());
	REQUIRE(CmdConnectOrbitalServices({DoCommandFlag::Execute}, dock.index, dock.generation, core.index, core.generation).Succeeded());
	CHECK(dock.module->consumer != old_consumer); CHECK(GetOrbitalServicePowerSite(dock) == &core);
	CHECK(GetSpaceStructureApproachCapacity(dock) == 2); REQUIRE(ValidateSpaceStructures());
}

/** Derive availability from actual paid activation/source fields without caching readiness or disturbing another station's capacity. */
TEST_CASE("Dock dependencies remain local and preserve accountable geometry through outages", "[orbital-dock-contract]")
{
	OrbitalDockRig rig(true);
	auto &core = *SpaceStructure::Get(SpaceStructureID{0}); auto &dock = *SpaceStructure::Get(SpaceStructureID{1}); auto &power = *SpaceStructure::Get(SpaceStructureID{2});
	auto &other_core = *SpaceStructure::Get(SpaceStructureID{3}); auto &other_stock = *SpaceStructure::Get(SpaceStructureID{4});
	REQUIRE(CmdActivateOrbitalCore({DoCommandFlag::Execute}, other_core.index, other_core.generation).Succeeded()); rig.ActivateCountdown();
	REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, other_stock.index, other_stock.generation).Succeeded()); rig.ActivateCountdown();
	CHECK(IsOrbitalModuleOperational(other_stock)); CHECK_FALSE(IsOrbitalModuleOperational(dock)); CHECK(GetOrbitalModulePanelRate(power) == 0);
	REQUIRE(CmdActivateOrbitalCore({DoCommandFlag::Execute}, core.index, core.generation).Succeeded()); rig.ActivateCountdown();
	REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, dock.index, dock.generation).Succeeded());
	REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, power.index, power.generation).Succeeded()); rig.ActivateCountdown();
	REQUIRE(CmdConnectOrbitalServices({DoCommandFlag::Execute}, dock.index, dock.generation, power.index, power.generation).Succeeded());
	CHECK(GetOrbitalModulePanelRate(power) == 60000);
	{
		// An isolated valid unfinished-Core boundary changes only derived eligibility; this is not gameplay removal or a transported predecessor.
		AutoRestoreBackup pending(core.module->activation_ticks, uint16_t{1});
		CHECK_FALSE(IsOrbitalModuleOperational(dock)); CHECK_FALSE(IsOrbitalModuleOperational(power));
		CHECK(GetOrbitalServicePowerSite(dock) == nullptr); CHECK(GetOrbitalModulePanelRate(power) == 0);
		CHECK(GetSpaceStructureApproachCapacity(dock) == 2); CHECK(IsOrbitalModuleOperational(other_stock));
		CHECK(GetOrbitalModulePanelRate(other_core) == 30000); REQUIRE(ValidateSpaceStructures());
	}
	CHECK(IsOrbitalModuleOperational(dock)); CHECK(GetOrbitalServicePowerSite(dock) == &power);
	REQUIRE(CmdSetOrbitalPanelPaused({DoCommandFlag::Execute}, power.index, power.generation, true).Succeeded());
	CHECK(GetOrbitalModulePanelRate(power) == 0); CHECK(GetSpaceStructureApproachCapacity(dock) == 2);
	CHECK(GetOrbitalModulePanelRate(other_core) == 30000); CHECK(IsOrbitalModuleOperational(other_stock));
	REQUIRE(ValidateSpaceStructures());
}
