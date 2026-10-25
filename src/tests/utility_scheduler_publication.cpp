/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_scheduler_publication.cpp The sole native interval driver's publication, guard and source-loss boundaries.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8DC-8E.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1B.
 * Native generator fixtures disclose floor/material inputs. Orbital consumed frame is a restore-boundary input, not transported construction acceptance.
 */
#include "../stdafx.h"
#include <any> // Standard allocation internals precede fixture safeguards.
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../orbital_service.h"
#include "../openttd.h"
#include "../vehicle_base.h"
#include "../landscape_cmd.h"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "geothermal_generator_operating_fixture.h"
#include "geothermal_town_service_fixture.h"
#include "../safeguards.h"

/** Both actual completed models acquire original coupled accounts and publish once per real synchronized tick, even with no capsules. */
TEST_CASE("Common native scheduler publishes completed generators without capsules", "[utility-scheduler-generator-publication]")
{
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	GeothermalGeneratorOperatingRig rig{false, true, 1000000, model}; REQUIRE(Vehicle::GetNumItems() == 0);
	REQUIRE(GetGeothermalGeneratorState(rig.generator)->electrical_producer.base() == UINT32_MAX);
	const auto owner = _current_company; ++TimerGameTick::counter; PrepareTubePowerTick();
	const auto first = *GetGeothermalGeneratorState(rig.generator); REQUIRE(first.electrical_producer.base() != UINT32_MAX);
	const uint64_t output = model == GeothermalGeneratorModel::VentTap ? 4050 : 9450;
	CHECK(first.active_service_ticks == 1); CHECK(GetUtilityPowerProducer(first.electrical_producer)->generated_lifetime_j == output);
	CHECK(IsUtilityHeatAllocated()); CHECK(IsUtilityPowerAllocated()); CHECK(GetTubePowerPreparedTick() == TimerGameTick::counter);
	CHECK(_current_company == owner);
	const auto utility = ComputeUtilityPowerDigest(), native = ComputeGeothermalGeneratorDigest(); const auto cash = Company::Get(owner)->money; const auto random = _random;
	PrepareTubePowerTick(); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native);
	CHECK(Company::Get(owner)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
	++TimerGameTick::counter; PrepareTubePowerTick();
	CHECK(GetGeothermalGeneratorState(rig.generator)->active_service_ticks == 2);
	CHECK(GetUtilityPowerProducer(first.electrical_producer)->generated_lifetime_j == output * 2);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Explicit pause/editor policy cannot advance generation, invoices or the common prepared-tick guard even if an external test clock moves. */
TEST_CASE("Common native scheduler respects pause and game mode", "[utility-scheduler-publication-policy]")
{
	GeothermalGeneratorOperatingRig rig; ++TimerGameTick::counter; PrepareTubePowerTick();
	AutoRestoreBackup pause{_pause_mode}; AutoRestoreBackup mode{_game_mode};
	SECTION("paused world") { _pause_mode.Set(PauseMode::Normal); }
	SECTION("editor world") { _game_mode = GameMode::Editor; }
	const auto utility = ComputeUtilityPowerDigest(), native = ComputeGeothermalGeneratorDigest(), prepared = GetTubePowerPreparedTick();
	const auto cash = Company::Get(_current_company)->money; ++TimerGameTick::counter; PrepareTubePowerTick();
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(GetTubePowerPreparedTick() == prepared);
	CHECK(Company::Get(_current_company)->money == cash);
}

/** A refused shared rotation must leave the old prepared tick and original paid generation intact; retries cannot relabel an older grant. */
TEST_CASE("Common native scheduler does not prepare a failed rotation", "[utility-scheduler-rotation-refusal]")
{
	GeothermalGeneratorOperatingRig rig; ++TimerGameTick::counter; PrepareTubePowerTick();
	auto state = CaptureUtilityPowerState(); state.operation_sequence = UINT64_MAX - 1;
	StageLoadedUtilityPowerState(std::move(state)); REQUIRE(AcceptLoadedUtilityPowerState());
	const auto utility = ComputeUtilityPowerDigest(), native = ComputeGeothermalGeneratorDigest(), prepared = GetTubePowerPreparedTick();
	const auto cash = Company::Get(_current_company)->money; ++TimerGameTick::counter;
	// Repeated callers at this refused native tick must preserve all older
	// publication and cannot buy service again or relabel its original grant.
	for (uint attempt = 0; attempt < 2; ++attempt) {
		PrepareTubePowerTick(); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native);
		CHECK(GetTubePowerPreparedTick() == prepared); CHECK(Company::Get(_current_company)->money == cash);
	}
}

/** A world with no physical participants neither initializes Heat nor creates an electrical interval opportunity. */
TEST_CASE("Common native scheduler leaves a source free world untouched", "[utility-scheduler-source-free]")
{
	ExcavationFixture world; ResetTubePowerState(); ResetUtilityPowerState(); ResetGeothermalCollectors(); ResetGeothermalGenerators();
	AutoRestoreBackup pause{_pause_mode, PauseModes{}}; const auto utility = ComputeUtilityPowerDigest(), prepared = GetTubePowerPreparedTick();
	++TimerGameTick::counter; PrepareTubePowerTick();
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(GetTubePowerPreparedTick() == prepared); CHECK_FALSE(HasUtilityHeatState());
}

/** Owned consumed Core frame and actual portable activation work provide an orbital-only native source without fabricated electricity. */
class SchedulerOrbitalSourceRig {
	ExcavationFixture world; ///< Existing admitted native company/profile and four map layers.
	AutoRestoreBackup<PauseModes> pause{_pause_mode, PauseModes{}}; ///< Unpause after the underlying fixture installs its deliberate graphics-effect pause.
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{1995}}; ///< Exact original Core calendar gate.
	AutoRestoreBackup<uint64_t> clock{TimerGameTick::counter}; ///< Restore explicitly advanced native component clock.
public:
	/** Restore the declared consumed original frame, then activate it through the actual native command/work owners. */
	SchedulerOrbitalSourceRig()
	{
		REQUIRE(SpaceStructure::GetNumItems() == 0); ResetSpaceStructures(); ResetTubePowerState(); ResetUtilityPowerState();
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1995}, 0, 1);
		const auto &definition = *GetSpaceConstructionDefinition(SpaceBuildKind::Core); SpaceStructureRecord frame;
		frame.ref = {SpaceStructureID{0}, 1}; frame.state.owner = _current_company; frame.state.kind = SpaceBuildKind::Core;
		frame.state.x = frame.state.y = 4; frame.state.capital_paid = definition.capital; frame.state.phase = SpaceStructurePhase::FrameComplete;
		frame.work_ticks = definition.work_ticks;
		frame.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)};
		REQUIRE(InstallLoadedSpaceStructures(std::array{frame}, 2));
		const auto *core = ResolveSpaceStructure(frame.ref); REQUIRE(core != nullptr);
		REQUIRE(CmdActivateOrbitalModule(DoCommandFlag::Execute, core->index, core->generation).Succeeded());
		// Each explicit synchronized opportunity uses the real portable Core work
		// owner; the source cannot publish until its complete activation is earned.
		for (uint tick = 0; tick < GetMultiLayerSpaceBudget().core_activation_ticks; ++tick) {
			++TimerGameTick::counter; PrepareTubePowerTick(); TickOrbitalConstructionInventory();
		}
	}
	/** Clear only this declared frame/source world before restoring the native company/map owner. */
	~SchedulerOrbitalSourceRig() { _space_structure_pool.CleanPool(); ResetSpaceStructures(); ResetTubePowerState(); ResetUtilityPowerState(); }
};

/** A real original orbital Core source runs through the common driver while the unrelated thermal domain remains absent. */
TEST_CASE("Common native scheduler keeps orbital only publication free of Heat", "[utility-scheduler-orbital-only]")
{
	SchedulerOrbitalSourceRig rig;
	const auto &core = *SpaceStructure::Get(SpaceStructureID{0}); REQUIRE(IsOrbitalModuleOperational(core)); REQUIRE(HasOrbitalUtilitySources());
	REQUIRE_FALSE(HasUtilityHeatState()); ++TimerGameTick::counter; PrepareTubePowerTick();
	CHECK(IsUtilityPowerAllocated()); CHECK_FALSE(HasUtilityHeatState()); CHECK_FALSE(IsUtilityHeatAllocated());
	CHECK(GetTubePowerPreparedTick() == TimerGameTick::counter);
	const auto digest = ComputeUtilityPowerDigest(); PrepareTubePowerTick(); CHECK(ComputeUtilityPowerDigest() == digest);
}

/** Retained town subscriptions still expire their last paid effect after all physical generator/collector sources disappear. */
TEST_CASE("Common native scheduler expires a retained town after final source loss", "[utility-scheduler-town-source-loss]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTownServiceRig rig;
	REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded()); rig.Operate(); REQUIRE(GetUtilityTownPowerBonus(rig.service.town) == 25);
	const auto collector = rig.source_port;
	REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, rig.plant_port, std::make_tuple(rig.plant_port)).Succeeded());
	REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, collector, std::make_tuple(collector)).Succeeded());
	REQUIRE_FALSE(HasGeothermalGenerators()); REQUIRE_FALSE(HasGeothermalHeatSources()); REQUIRE(HasUtilityTownServices());
	const auto interval = GetUtilityPowerInterval(); const auto cash = Company::Get(_current_company)->money;
	++TimerGameTick::counter; PrepareTubePowerTick(); CHECK(GetUtilityPowerInterval() == interval + 1);
	CHECK(GetUtilityTownPowerBonus(rig.service.town) == 0); CHECK(Company::Get(_current_company)->money == cash);
	const auto digest = ComputeUtilityPowerDigest(); PrepareTubePowerTick(); CHECK(ComputeUtilityPowerDigest() == digest);
}
