/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_calendar.cpp Canonical capsule launch dates under native startup and old saved-definition restoration.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DCBA.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_fixture.h"
#include "../engine_func.h"
#include "../road.h"
#include "../safeguards.h"

/** Apply the actual native month-boundary publication/aging owner, as the registered calendar timer does. */
void CalendarEnginesMonthlyLoop();

/** Every original capsule is unavailable immediately before its published launch and buildable on that exact date, without fixture availability assignment. */
TEST_CASE("Native capsule startup honors all five exact launch dates", "[transport-tube-calendar-startup]")
{
	TubePrototypeRig world; const uint16_t model = GENERATE(9, 10, 11, 12, 13);
	const bool launched = GENERATE(false, true); auto *engine = Engine::Get(GetTubeEngine(model));
	AutoRestoreBackup starting_year{_settings_game.game_creation.starting_year, TimerGameCalendar::Year{1950}};
	const auto date = engine->info.base_intro - (launched ? 0 : 1);
	AutoRestoreBackup calendar{TimerGameCalendar::date, date};
	const auto aging = TimerGameCalendar::ConvertDateToYMD(date); AutoRestoreBackup year{TimerGameCalendar::year, aging.year};
	StartupOneEngine(engine, aging, 420);
	CHECK(engine->intro_date == engine->info.base_intro); CHECK(engine->flags.Test(EngineFlag::Available) == launched);
	CHECK(engine->company_avail.Test(_current_company) == launched);
	CHECK(IsEngineBuildable(engine->index, engine->type, _current_company) == launched);
}

/** A continuing world publishes every original capsule on its real launch month without an ordinary exclusive-preview year or a fixture availability assignment. */
TEST_CASE("Native calendar publication uses each original capsule launch without a preview year", "[transport-tube-calendar-monthly]")
{
	TubePrototypeRig world; const uint16_t model = GENERATE(9, 10, 11, 12, 13);
	const bool launched = GENERATE(false, true); auto *engine = Engine::Get(GetTubeEngine(model));
	AutoRestoreBackup starting_year{_settings_game.game_creation.starting_year, TimerGameCalendar::Year{1950}};
	const auto prior = engine->info.base_intro - 1; AutoRestoreBackup calendar{TimerGameCalendar::date, prior};
	AutoRestoreBackup year{TimerGameCalendar::year, TimerGameCalendar::ConvertDateToYMD(prior).year};
	// Normal content finalization establishes the engine-aging horizon before
	// startup. A zero-initialized unit horizon would suppress every native
	// monthly publication and test an uninitialized world instead of gameplay.
	ResetRoadTypes(); SetYearEngineAgingStops(); StartupEngines();
	REQUIRE_FALSE(engine->flags.Test(EngineFlag::Available)); REQUIRE_FALSE(engine->company_avail.Test(_current_company));
	// Only the real calendar boundary changes. Native monthly publication must
	// acquire the existing definition itself; this test supplies no company flag,
	// preview state or alternate engine to manufacture first-day availability.
	if (launched) { TimerGameCalendar::date = engine->info.base_intro; TimerGameCalendar::year = TimerGameCalendar::ConvertDateToYMD(engine->info.base_intro).year; }
	CalendarEnginesMonthlyLoop();
	CHECK(engine->intro_date == engine->info.base_intro); CHECK(engine->flags.Test(EngineFlag::Available) == launched);
	CHECK(engine->company_avail.Test(_current_company) == launched); CHECK_FALSE(engine->flags.Test(EngineFlag::ExclusivePreview));
	CHECK(IsEngineBuildable(engine->index, engine->type, _current_company) == launched);
}

/** A formerly randomized saved capsule publishes only once date-valid, preserving all unrelated saved engine preferences, age/reliability and both RNG streams. */
TEST_CASE("Saved native capsule dates reconcile without changing other owners", "[transport-tube-calendar-restore]")
{
	TubePrototypeRig world; const uint16_t model = GENERATE(9, 10, 11, 12, 13);
	const bool launched = GENERATE(false, true); auto *engine = Engine::Get(GetTubeEngine(model));
	const auto date = engine->info.base_intro - (launched ? 0 : 1); AutoRestoreBackup calendar{TimerGameCalendar::date, date};
	const auto ymd = TimerGameCalendar::ConvertDateToYMD(date); AutoRestoreBackup year{TimerGameCalendar::year, ymd.year};
	engine->intro_date = engine->info.base_intro + 284; engine->flags.Reset(EngineFlag::Available); engine->company_avail = {};
	engine->company_hidden.Set(_current_company); engine->age = 17; engine->reliability = 23456;
	const auto original = std::tuple{engine->age, engine->reliability, engine->company_hidden,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
	RestoreTubeEngineIntroduction(*engine);
	CHECK(engine->intro_date == engine->info.base_intro); CHECK(engine->flags.Test(EngineFlag::Available) == launched);
	CHECK(engine->company_avail.Test(_current_company) == launched);
	CHECK((original == std::tuple{engine->age, engine->reliability, engine->company_hidden,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}}));
	const auto repeated = std::tuple{engine->intro_date, engine->flags, engine->company_avail}; RestoreTubeEngineIntroduction(*engine);
	CHECK((repeated == std::tuple{engine->intro_date, engine->flags, engine->company_avail}));
}

/** Calendar reconciliation applies only to actual native capsule EIDS identities; conventional/TBM/foreign definitions remain untouched. */
TEST_CASE("Native capsule date restoration leaves other engine families unchanged", "[transport-tube-calendar-isolation]")
{
	TubePrototypeRig world; uint checked = 0;
	// Inspect every actual noncapsule definition already present in this
	// admitted world, rather than a guessed original ID or copied local model.
	for (auto *engine : Engine::Iterate()) {
		if (GetTubeVehicleCapability(engine->index)) continue;
		const auto before = std::tuple{engine->intro_date, engine->flags, engine->company_avail, engine->company_hidden, engine->age, engine->reliability};
		RestoreTubeEngineIntroduction(*engine);
		CHECK((before == std::tuple{engine->intro_date, engine->flags, engine->company_avail, engine->company_hidden, engine->age, engine->reliability})); ++checked;
	}
	CHECK(checked > 0);
}
