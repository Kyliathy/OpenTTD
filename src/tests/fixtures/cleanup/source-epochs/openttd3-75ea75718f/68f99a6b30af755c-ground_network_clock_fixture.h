/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file ground_network_clock_fixture.h Legal starting time modes before actual paid ground work.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Group19C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Declared generated/logistics/cash fixture inputs are disclosed;this is not normal-newgame bootstrap acceptance.
 */
#ifndef TESTS_GROUND_NETWORK_CLOCK_FIXTURE_H
#define TESTS_GROUND_NETWORK_CLOCK_FIXTURE_H

#include "ground_network_active_fixture.h"
#include "underground_depot_fixture.h"
#include "layer_window_fixture.h"
#include "../newgame_settings.h"
#include "../settings_func.h"
#include "../transport_tube.h"
#include "../transport_tube_model.h"
#include "../table/sprites.h"

/** Prepare legal next-game settings while no map,company,vehicle or paid resource owner exists. */
inline void PrepareGroundNetworkClockSettings()
{
	REQUIRE(_game_mode == GameMode::Menu);
	REQUIRE_FALSE(_networking);
	REQUIRE(Map::Size() == 0);
	REQUIRE(Company::GetNumItems() == 0);
	REQUIRE(Vehicle::GetNumItems() == 0);
	REQUIRE(Industry::GetNumItems() == 0);
	const auto units = GroundNetworkUnsigned("OTTD_GROUND_CLOCK_UNITS");
	const auto rate = GroundNetworkUnsigned("OTTD_GROUND_CLOCK_RATE");
	REQUIRE(units <= 1);
	REQUIRE((rate == 0 || rate == 12 || rate == 24));
	// These are actual public next-game changes,not a live-world conversion.
	// The existing application owner activates them before this fresh fixture
	// allocates any geology,map,company,work,cargo or accounting participant.
	IConsoleSetSetting("economy.timekeeping_units", std::to_string(units), true);
	IConsoleSetSetting("economy.minutes_per_calendar_year", std::to_string(rate), true);
	MakeNewgameSettingsLive();
	CHECK(to_underlying(_settings_game.economy.timekeeping_units) == units);
	CHECK(_settings_game.economy.minutes_per_calendar_year == rate);
}

/** Earn an original unfinished paid job under the legally selected native starting clock policy. */
inline void ProduceGroundNetworkClockWorld()
{
	PrepareGroundNetworkClockSettings();
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	NativeDepotEconomy prices;
	UndergroundWorkflowWorld world{MapLayer::Underground, true, true, 1};
	LayerSaveContent content{world.ActivatedContent(), true};
	REQUIRE(InitialiseTubeRailType(world.ActivatedContent()) != INVALID_RAILTYPE);
	REQUIRE(InitialiseTubeEngineDefinitions(world.ActivatedContent()));
	StartupEngines();
	world.simulate_company_limits = true;
	world.native_clocks = true;
	const auto configured_units = GroundNetworkUnsigned("OTTD_GROUND_CLOCK_UNITS");
	const auto configured_rate = GroundNetworkUnsigned("OTTD_GROUND_CLOCK_RATE");
	REQUIRE(to_underlying(_settings_game.economy.timekeeping_units) == configured_units);
	REQUIRE(_settings_game.economy.minutes_per_calendar_year == configured_rate);
	/**
	 * Stop only after an actual paid cut while unfinished work remains.
	 * @param current Same native stepped world;no tile,cargo,cursor or position is written here.
	 * @return Whether a genuine earned unfinished native checkpoint now exists.
	 */
	const auto earned = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		if (assignment == nullptr) return false;
		const auto *project = GetTBMProject(assignment->project);
		if (project == nullptr) return false;
		const auto *job = GetExcavationJob(project->job);
		if (job == nullptr) return false;
		return job->cursor > 0 && job->cursor < job->route.size();
	};
	const auto id = RunUndergroundExcavationWorkflow(world, 0, {}, {}, true, std::nullopt, earned);
	const auto *project = GetTBMProject(id);
	REQUIRE(project != nullptr);
	const auto *job = GetExcavationJob(project->job);
	REQUIRE(job != nullptr);
	REQUIRE(job->cursor > 0);
	REQUIRE(job->cursor < job->route.size());
	REQUIRE(job->completed_earth_t > 0);
	REQUIRE(job->completed_rock_t > 0);
	REQUIRE(job->completed_work_cost > Money{0});
	REQUIRE(world.setup_cost > Money{0});
	REQUIRE(Company::Get(_current_company)->money < Money{10000000} - world.setup_cost);
	REQUIRE(_pause_mode.Test(PauseMode::Normal));
	const auto before = std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(),
		ComputeTBMExchangeDigest(), Company::Get(_current_company)->money, TimerGameTick::counter,
		TimerGameCalendar::date, TimerGameCalendar::date_fract, TimerGameCalendar::sub_date_fract,
		TimerGameEconomy::date, TimerGameEconomy::date_fract, _random.state[0], _random.state[1]};
	AutoRestoreBackup local{_local_company, _current_company};
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	// The declared source fixture has mock presentation only. Use its established
	// menu serialization context to avoid GUI-only busy-cursor work;actual saved
	// profile,paid owners,clocks,pause policy and resources remain unchanged.
	AutoRestoreBackup menu{_game_mode, GameMode::Menu};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	const auto paths = GetTestCaseArtifactPaths("network", "paid-ground");
	// Immediate complete repeats prove that observation does not bank a second
	// work tick,move a physical part,repair a reference or consume another bill.
	for (const std::string_view suffix : {"", "-repeat"}) {
		const auto path = paths.Output("saves", fmt::format("ground-clock-original{}.sav", suffix));
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK((std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(),
		ComputeTBMExchangeDigest(), Company::Get(_current_company)->money, TimerGameTick::counter,
		TimerGameCalendar::date, TimerGameCalendar::date_fract, TimerGameCalendar::sub_date_fract,
		TimerGameEconomy::date, TimerGameEconomy::date_fract, _random.state[0], _random.state[1]} == before));
	fmt::print("GROUND_CLOCK_SOURCE units={} rate={} tick={} project={} job={} cursor={} earth={} rock={} work={} setup={} cash={}\n",
		configured_units, configured_rate, TimerGameTick::counter, project->id.base(), project->job.base(), job->cursor,
		job->completed_earth_t, job->completed_rock_t, job->completed_work_cost.base(), world.setup_cost.base(), Company::Get(_current_company)->money.base());
}

#endif /* TESTS_GROUND_NETWORK_CLOCK_FIXTURE_H */
