/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_removal.cpp Isolated native removal/refusal/receipt boundaries; not a delivered gameplay predecessor.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups5/5C/6.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../map_func.h"
#include "excavation_fixture.h"
#include <type_traits>
#include "../safeguards.h"

// The public construction handoff binds exact native owners/signatures. A consumer cannot silently substitute a copied inventory or a different job/utility namespace.
static_assert(std::is_same_v<decltype(&ReserveOrbitalConstructionInputs), bool (*)(SpaceStructure &, SpaceVehicle &, bool)>);
static_assert(std::is_same_v<decltype(&CommitOrbitalConstructionStep), bool (*)(SpaceVehicle &)>);
static_assert(std::is_same_v<decltype(SpaceStructure::module), std::unique_ptr<OrbitalModule>>);
static_assert(std::is_same_v<decltype(OrbitalConstructionJob::inputs), std::array<StationCargoList, 2>>);
static_assert(std::is_same_v<decltype(OrbitalModule::stock), std::array<StationCargoList, NUM_CARGO>>);
static_assert(std::is_same_v<decltype(std::declval<OrbitalModule &>().GetServiceJobs()), std::array<OrbitalServiceJob *, 2>>);
static_assert(std::is_same_v<decltype(OrbitalFacilityState::utility_source), SpaceStructureRef>);
static_assert(std::is_same_v<decltype(OrbitalFacilityState::producer), UtilityProducerID>);
static_assert(std::is_same_v<decltype(OrbitalFacilityState::consumer), UtilityConsumerID>);
static_assert(std::is_same_v<decltype(OrbitalConstructionState::last_work_tick), uint64_t>);

/** Construct detached canonical boundary data, never physical delivered material.
 * @param kind Static recipe. @param index Isolated pool identity. @param x Plane X. @param y Plane Y.
 * @return Consumed inert test frame with no stock, energy or job granted by this helper.
 */
static SpaceStructureRecord RemovalBoundaryFrame(SpaceBuildKind kind, uint16_t index, uint16_t x, uint16_t y)
{
	const auto &definition = *GetSpaceConstructionDefinition(kind);
	SpaceStructureRecord row; row.ref = {SpaceStructureID{index}, uint64_t{index} + 1};
	row.state.owner = _current_company; row.state.kind = kind; row.state.x = x; row.state.y = y;
	row.state.capital_paid = definition.capital; row.state.phase = SpaceStructurePhase::FrameComplete;
	row.work_ticks = definition.work_ticks;
	row.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)};
	return row;
}

/** Actual native owners restored from isolated receipts; real paid delivery/evacuation acceptance is a separate native-world fixture. */
class OrbitalRemovalRig {
	ExcavationFixture world;
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{1995}};
public:
	/** @param overflow Restore a legal bounded history whose service sum cannot accept another completed service. */
	OrbitalRemovalRig(bool overflow = false)
	{
		ResetUtilityPowerState(); ResetSpaceStructures();
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1995}, 0, 1);
		std::array rows{RemovalBoundaryFrame(SpaceBuildKind::Core, 0, 8, 4), RemovalBoundaryFrame(SpaceBuildKind::Storage, 1, 11, 4),
			RemovalBoundaryFrame(SpaceBuildKind::Dock, 2, 9, 7)};
		SpaceStructureRetirementHistory history;
		if (overflow) {
			history.structures_removed = 1; history.services_completed = UINT64_MAX;
			rows[2].module.emplace(); rows[2].module->services_completed = 1;
		}
		REQUIRE(InstallLoadedSpaceStructures(rows, rows.size() + 1 + history.structures_removed, history));
	}
	/** Clear isolated empty owners and accounts before restoring the real profile/company fixture. */
	~OrbitalRemovalRig() { _space_structure_pool.CleanPool(); ResetSpaceStructures(); ResetUtilityPowerState(); }
	/** Complete only the existing native activation countdown; no test energy or material is injected. */
	void ActivateCountdown()
	{
		for (uint tick = 0; tick < GetMultiLayerSpaceBudget().core_activation_ticks; ++tick) TickOrbitalConstructionInventory();
	}
};

/** Empty removal retains spent receipts, changes no money/RNG, and issues a fresh incarnation when a numeric pool slot is reused. */
TEST_CASE("Empty module removal preserves canonical spent receipts and rejects stale identities", "[orbital-removal-contract]")
{
	OrbitalRemovalRig rig;
	auto &storage = *SpaceStructure::Get(SpaceStructureID{1}); const auto reference = GetSpaceStructureReference(storage); const auto state = storage.state;
	const auto before = ComputeSpaceStructureDigest(), ledger = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	CHECK(CmdRemoveOrbitalModule({}, reference.structure, reference.generation).Succeeded());
	CHECK(CmdRemoveOrbitalModule({}, reference.structure, reference.generation + 1).Failed());
	{ AutoRestoreBackup foreign(_current_company, COMPANY_SPECTATOR); CHECK(CmdRemoveOrbitalModule({}, reference.structure, reference.generation).Failed()); }
	CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeUtilityPowerDigest() == ledger);
	REQUIRE(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, reference.structure, reference.generation).Succeeded());
	CHECK(ResolveSpaceStructure(reference) == nullptr); CHECK(GetSpaceStructureRetirementHistory().structures_removed == 1);
	CHECK((GetSpaceStructureRetirementHistory().consumed == std::array<uint64_t, 2>{8, 8})); CHECK(GetSpaceStructureRetirementHistory().services_completed == 0);
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]); REQUIRE(ValidateSpaceStructures());
	CHECK(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, reference.structure, reference.generation).Failed());
	auto &core = *SpaceStructure::Get(SpaceStructureID{0}); REQUIRE(CmdActivateOrbitalCore({DoCommandFlag::Execute}, core.index, core.generation).Succeeded()); rig.ActivateCountdown();
	// This isolated direct command exercises issuance only; it is not evidence of a paid gameplay purchase or delivered construction.
	REQUIRE(CmdBuildSpaceBlueprint({DoCommandFlag::Execute}, TileXYL(state.x, state.y, MapLayer::Space), state.kind).Succeeded());
	const auto *replacement = SpaceStructure::Get(reference.structure); REQUIRE(replacement != nullptr);
	CHECK(replacement->generation > reference.generation); CHECK(replacement->module == nullptr); CHECK(replacement->job.work_ticks == 0);
	CHECK(replacement->job.inputs[0].TotalCount() == 0); CHECK(replacement->job.inputs[1].TotalCount() == 0);
	CHECK(ResolveSpaceStructure(reference) == nullptr); REQUIRE(ValidateSpaceStructures());
}

/** Exact connection retirement precedes source removal; empty dependent facilities become inert without receiving replacement accounts. */
TEST_CASE("Source removal requires explicit disconnection and leaves empty dependent owners intact", "[orbital-removal-contract]")
{
	OrbitalRemovalRig rig;
	auto &core = *SpaceStructure::Get(SpaceStructureID{0}); auto &dock = *SpaceStructure::Get(SpaceStructureID{2});
	REQUIRE(CmdActivateOrbitalCore({DoCommandFlag::Execute}, core.index, core.generation).Succeeded()); rig.ActivateCountdown();
	REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, dock.index, dock.generation).Succeeded()); rig.ActivateCountdown();
	REQUIRE(CmdConnectOrbitalServices({DoCommandFlag::Execute}, dock.index, dock.generation, core.index, core.generation).Succeeded());
	const auto producer = core.module->producer; const auto core_ref = GetSpaceStructureReference(core); const auto consumer = dock.module->consumer;
	const auto before = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
	CHECK(ClassifyOrbitalModuleRemoval(core, _current_company) == OrbitalRemovalBlock::Connection);
	CHECK(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, core.index, core.generation).Failed()); CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeUtilityPowerDigest() == power);
	REQUIRE(CmdDisconnectOrbitalServices({DoCommandFlag::Execute}, dock.index, dock.generation).Succeeded());
	CHECK_FALSE(GetUtilityPowerConsumer(consumer)->active);
	REQUIRE(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, core.index, core.generation).Succeeded());
	CHECK(ResolveSpaceStructure(core_ref) == nullptr); CHECK_FALSE(GetUtilityPowerProducer(producer)->active); CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == 0);
	CHECK_FALSE(IsOrbitalModuleOperational(dock)); CHECK(dock.module != nullptr); CHECK(dock.module->consumer == UtilityConsumerID{UINT32_MAX});
	CHECK((GetSpaceStructureRetirementHistory().consumed == std::array<uint64_t, 2>{16, 24})); REQUIRE(ValidateSpaceStructures());
}

/** Refusal preflights every historical addition before either physical ownership or electrical eligibility is changed. */
TEST_CASE("Retired service history overflow refuses module removal unchanged", "[orbital-removal-contract]")
{
	OrbitalRemovalRig rig(true); auto &dock = *SpaceStructure::Get(SpaceStructureID{2});
	const auto before = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
	CHECK(ClassifyOrbitalModuleRemoval(dock, _current_company) == OrbitalRemovalBlock::History);
	CHECK(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, dock.index, dock.generation).Failed());
	CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeUtilityPowerDigest() == power); REQUIRE(ValidateSpaceStructures());
}

/** Copied history remains bounded by original issuance and canonical static recipe maxima without overflowing multiplication. */
TEST_CASE("Copied module retirement history cannot manufacture spent receipts or incarnations", "[orbital-removal-contract]")
{
	SpaceStructureRetirementHistory history;
	CHECK(ValidateSpaceStructureRecords({}, 1, 64, 64, UINT64_MAX, history));
	history.structures_removed = 1; CHECK_FALSE(ValidateSpaceStructureRecords({}, 1, 64, 64, UINT64_MAX, history));
	CHECK(ValidateSpaceStructureRecords({}, 2, 64, 64, UINT64_MAX, history));
	history.consumed[0] = UINT64_MAX; CHECK_FALSE(ValidateSpaceStructureRecords({}, 2, 64, 64, UINT64_MAX, history));
	history = {}; history.services_completed = 1; CHECK_FALSE(ValidateSpaceStructureRecords({}, 1, 64, 64, UINT64_MAX, history));
	history.structures_removed = UINT64_MAX - 1; history.consumed = {UINT64_MAX, UINT64_MAX};
	CHECK(ValidateSpaceStructureRecords({}, UINT64_MAX, 64, 64, UINT64_MAX, history));
}
