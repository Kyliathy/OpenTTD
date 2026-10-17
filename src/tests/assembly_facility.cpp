/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file assembly_facility.cpp Paid Assembly role, explicit original-source connection and refusal contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group6B
 * Detached consumed-frame fixtures characterize commands; actual imported material/work proof lives in assembly_facility_native.cpp.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_approach.h"
#include "../space_vehicle.h"
#include "../company_base.h"
#include "../core/backup_type.hpp"
#include "excavation_fixture.h"
#include "../safeguards.h"

/** Prepare one explicitly isolated consumed-frame record, never a gameplay construction predecessor.
 * @param kind Canonical recipe whose exact materials/work/capital are represented.
 * @param id Isolated native pool identity; its generation is the next positive integer.
 * @param x Layer-local left footprint coordinate.
 * @param y Layer-local top footprint coordinate.
 * @return Detached paid frame without any module, stock, vehicle or utility grant.
 */
static SpaceStructureRecord AssemblyFacilityFrame(SpaceBuildKind kind, uint16_t id, uint16_t x, uint16_t y)
{
	const auto &definition = *GetSpaceConstructionDefinition(kind);
	SpaceStructureRecord row;
	row.ref = {SpaceStructureID{id}, uint64_t{id} + 1}; row.state.owner = _current_company;
	row.state.kind = kind; row.state.x = x; row.state.y = y;
	row.state.capital_paid = definition.capital; row.state.phase = SpaceStructurePhase::FrameComplete;
	row.work_ticks = definition.work_ticks;
	row.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)};
	return row;
}

/** Actual profile/company and isolated native frame owners; no fixture cargo or free energy is created. */
class AssemblyFacilityRig {
	ExcavationFixture world;
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{1995}};
public:
	/** Install one Core, Assembly and Power through public detached-owner validation. */
	AssemblyFacilityRig()
	{
		ResetUtilityPowerState(); ResetSpaceStructures();
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1995}, 0, 1);
		const std::array rows{AssemblyFacilityFrame(SpaceBuildKind::Core, 0, 4, 4),
			AssemblyFacilityFrame(SpaceBuildKind::Assembly, 1, 7, 6), AssemblyFacilityFrame(SpaceBuildKind::Power, 2, 5, 7)};
		REQUIRE(InstallLoadedSpaceStructures(rows, 4));
	}
	/** Retire only the isolated native owners before restoring the caller's company/content registry. */
	~AssemblyFacilityRig()
	{
		_space_structure_pool.CleanPool(); ResetSpaceStructures(); ResetUtilityPowerState();
	}
	/** @param site Actual isolated frame to activate. Complete only its real public countdown; manufacture remains unavailable. */
	void Activate(SpaceStructure &site)
	{
		REQUIRE(CmdActivateOrbitalModule(DoCommandFlag::Execute, site.index, site.generation).Succeeded());
		// Only native inventory opportunities may complete activation; repeated player queries cannot shorten the countdown.
		for (uint tick = 0; tick < GetMultiLayerSpaceBudget().core_activation_ticks; ++tick) TickOrbitalConstructionInventory();
		REQUIRE(IsOrbitalModuleOperational(site));
	}
};

/** A real Assembly role has bounded electrical work, while service, packets, panels and vehicles remain with their existing producers. */
TEST_CASE("Assembly activation and source estimation grant no free production", "[assembly-facility]")
{
	AssemblyFacilityRig rig;
	auto &core = *SpaceStructure::Get(SpaceStructureID{0});
	auto &assembly = *SpaceStructure::Get(SpaceStructureID{1});
	CHECK(CmdActivateOrbitalModule({}, assembly.index, assembly.generation).Failed());
	rig.Activate(core);
	const auto money = Company::Get(_current_company)->money;
	const auto structures = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest();
	REQUIRE(CmdActivateOrbitalModule({}, assembly.index, assembly.generation).Succeeded());
	CHECK(assembly.module == nullptr); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == utility);
	REQUIRE(CmdActivateOrbitalModule(DoCommandFlag::Execute, assembly.index, assembly.generation).Succeeded());
	CHECK(assembly.module->activation_ticks == 74); CHECK_FALSE(IsOrbitalModuleOperational(assembly));
	CHECK(CmdConnectOrbitalServices({}, assembly.index, assembly.generation, core.index, core.generation).Failed());
	// Every real activation opportunity is required; the last tick alone publishes operational eligibility.
	for (uint tick = 0; tick < 73; ++tick) TickOrbitalConstructionInventory();
	CHECK_FALSE(IsOrbitalModuleOperational(assembly)); TickOrbitalConstructionInventory();
	REQUIRE(IsOrbitalModuleOperational(assembly));
	CHECK(GetOrbitalModuleServiceCapacity(SpaceBuildKind::Assembly) == 0);
	CHECK(GetOrbitalModuleStockLimit(SpaceBuildKind::Assembly) == 0);
	CHECK(GetSpaceStructureApproachCapacity(assembly) == 1);
	CHECK(GetOrbitalModulePanelRate(assembly) == 0); CHECK(GetSpaceVehicleCount() == 0);
	const auto before_connection = ComputeUtilityPowerDigest();
	REQUIRE(CmdConnectOrbitalServices({}, assembly.index, assembly.generation, core.index, core.generation).Succeeded());
	CHECK(ComputeUtilityPowerDigest() == before_connection); CHECK(assembly.module->utility_source == SpaceStructureRef{});
	REQUIRE(CmdConnectOrbitalServices(DoCommandFlag::Execute, assembly.index, assembly.generation, core.index, core.generation).Succeeded());
	CHECK(GetOrbitalServicePowerSite(assembly) == &core);
	const auto *consumer = GetUtilityPowerConsumer(assembly.module->consumer); REQUIRE(consumer != nullptr);
	CHECK(consumer->capacity_j == 750); CHECK(consumer->effect == UtilityConsumerEffect::AssemblyWork);
	CHECK(consumer->linked_producers == std::vector{core.module->producer});
	CHECK(CmdServiceOrbitalProbe({}, assembly.index, assembly.generation, VehicleID::Invalid(), 0, OrbitalServiceControl::Start).Failed());
	CHECK(Company::Get(_current_company)->money == money); CHECK(CaptureUtilityPowerState().producers.size() == 1);
	CHECK(CaptureUtilityPowerState().producers.front().generated_lifetime_j == 0);
	REQUIRE(ValidateSpaceStructures()); REQUIRE(ValidateOrbitalServices());
}

/** Copied previews must resolve the exact original panel and reject role/capacity substitutions without borrowing live state. */
TEST_CASE("Assembly copied facility bindings reject invented power and service", "[assembly-facility]")
{
	AssemblyFacilityRig rig;
	auto &core = *SpaceStructure::Get(SpaceStructureID{0});
	auto &assembly = *SpaceStructure::Get(SpaceStructureID{1});
	rig.Activate(core); rig.Activate(assembly);
	REQUIRE(CmdConnectOrbitalServices(DoCommandFlag::Execute, assembly.index, assembly.generation, core.index, core.generation).Succeeded());
	const auto rows = CaptureSpaceStructures(); const auto power = CaptureUtilityPowerState();
	CHECK(ValidateSpaceStructureRecords(rows, 4, 64, 64)); CHECK(ValidateOrbitalServiceUtilityRecords(rows, power));
	auto altered = rows; ++altered[1].module->utility_source.generation;
	CHECK_FALSE(ValidateOrbitalServiceUtilityRecords(altered, power));
	altered = rows; altered[1].module->producer = core.module->producer;
	CHECK_FALSE(ValidateSpaceStructureRecords(altered, 4, 64, 64));
	altered = rows; altered[1].module->services_completed = 1;
	CHECK_FALSE(ValidateSpaceStructureRecords(altered, 4, 64, 64));
	altered = rows; altered[1].module->utility_source = {};
	CHECK_FALSE(ValidateSpaceStructureRecords(altered, 4, 64, 64));
	auto altered_power = power; altered_power.consumers.front().capacity_j = 1500;
	CHECK_FALSE(ValidateOrbitalServiceUtilityRecords(rows, altered_power));
	const auto digest = ComputeSpaceStructureDigest();
	CHECK(CmdConnectOrbitalServices({}, assembly.index, assembly.generation, core.index, core.generation + 1).Failed());
	CHECK(ComputeSpaceStructureDigest() == digest);
}

/** Original source outage, disconnection and alternate paid panel admission preserve ownership and empty-facility removal. */
TEST_CASE("Assembly work connections retain original sources through outage and retirement", "[assembly-facility]")
{
	AssemblyFacilityRig rig;
	auto &core = *SpaceStructure::Get(SpaceStructureID{0});
	auto &assembly = *SpaceStructure::Get(SpaceStructureID{1});
	auto &panel = *SpaceStructure::Get(SpaceStructureID{2});
	rig.Activate(core); rig.Activate(assembly); rig.Activate(panel);
	REQUIRE(CmdConnectOrbitalServices(DoCommandFlag::Execute, assembly.index, assembly.generation, core.index, core.generation).Succeeded());
	const auto original = assembly.module->consumer;
	CHECK(ClassifyOrbitalModuleRemoval(core, _current_company) == OrbitalRemovalBlock::Connection);
	REQUIRE(CmdSetOrbitalPanelPaused(DoCommandFlag::Execute, core.index, core.generation, true).Succeeded());
	CHECK(GetOrbitalServicePowerSite(assembly) == &core); CHECK(GetOrbitalModulePanelRate(core) == 0);
	CHECK(assembly.module->consumer == original);
	REQUIRE(CmdDisconnectOrbitalServices(DoCommandFlag::Execute, assembly.index, assembly.generation).Succeeded());
	CHECK_FALSE(GetUtilityPowerConsumer(original)->active); CHECK(assembly.module->utility_source == SpaceStructureRef{});
	REQUIRE(CmdConnectOrbitalServices(DoCommandFlag::Execute, assembly.index, assembly.generation, panel.index, panel.generation).Succeeded());
	CHECK(GetOrbitalServicePowerSite(assembly) == &panel); CHECK(assembly.module->consumer != original);
	CHECK(GetUtilityPowerConsumer(assembly.module->consumer)->linked_producers == std::vector{panel.module->producer});
	REQUIRE(ValidateSpaceStructures()); REQUIRE(ValidateOrbitalServices());
	CHECK(ClassifyOrbitalModuleRemoval(assembly, _current_company) == OrbitalRemovalBlock::None);
	const auto retired = assembly.module->consumer;
	REQUIRE(CmdRemoveOrbitalModule(DoCommandFlag::Execute, assembly.index, assembly.generation).Succeeded());
	CHECK_FALSE(GetUtilityPowerConsumer(retired)->active);
	CHECK((GetSpaceStructureRetirementHistory().consumed == std::array<uint64_t, 2>{8, 16}));
}
