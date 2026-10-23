/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_mine_production.cpp Actual finite reserve/output/work transactions. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 7 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mine_production_fixture.h"

#include "../safeguards.h"


/** All ten native resources use their content quantity and work rate, without touching inherited production rates. */
TEST_CASE("Every native mine produces exact finite output", "[layer-mine-production]")
{
	const size_t index = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
	MineProductionRig rig(index);
	const auto &recipe = GetMultiLayerMineRecipes()[index].recipe;
	const auto reserve = rig.Reserve();
	const auto revision = GetLayerMineState(rig.industry->index)->revision;
	const auto quote = ProduceLayerMineCargo(*rig.industry, recipe.work_economy_ticks, false);
	CHECK(quote.error == LayerIndustryError::None);
	CHECK(quote.amount == recipe.outputs[0].quantity);
	CHECK(rig.Reserve() == reserve);
	CHECK(rig.industry->produced[0].waiting == 0);
	CHECK(GetLayerMineState(rig.industry->index)->revision == revision);
	const auto result = ProduceLayerMineCargo(*rig.industry, recipe.work_economy_ticks, true);
	CHECK(result.amount == quote.amount);
	CHECK(rig.Reserve() + result.amount == reserve);
	CHECK(rig.industry->produced[0].waiting == result.amount);
	CHECK(GetLayerMineState(rig.industry->index)->extracted == result.amount);
	CHECK(GetLayerMineState(rig.industry->index)->work_ticks == 0);
	CHECK(GetLayerMineState(rig.industry->index)->work_fraction == 0);
}

/** Small per-tick outputs retain exact rational work instead of rounding away part of the frozen recipe interval. */
TEST_CASE("Mine work carries exactly across small output commits and real industry ticks", "[layer-mine-production]")
{
	MineProductionRig rig;
	const auto reserve = rig.Reserve();
	rig.industry->produced[0].rate = UINT8_MAX;
	const auto random = _random;
	// Native production runs through OnTick_Industry exactly once per economy tick, independently of legacy rates.
	for (uint tick = 0; tick < 74; ++tick) OnTick_Industry();
	CHECK(rig.industry->produced[0].waiting == 4);
	CHECK(rig.Reserve() == reserve - 4);
	CHECK(GetLayerMineState(rig.industry->index)->extracted == 4);
	CHECK(GetLayerMineState(rig.industry->index)->work_ticks == 0);
	CHECK(GetLayerMineState(rig.industry->index)->work_fraction == 0);
	CHECK(_random.state[0] == random.state[0]);
	CHECK(_random.state[1] == random.state[1]);
}

/** A partial seam and nearly full output slot both constrain the same exact geological debit. */
TEST_CASE("Mine reserve and output limits preserve every unproduced unit", "[layer-mine-production]")
{
	MineProductionRig rig;
	SECTION("Last three tonnes deplete the original seam") {
		rig.SetReserve(3);
		const auto result = ProduceLayerMineCargo(*rig.industry, 74, true);
		CHECK(result.amount == 3);
		CHECK(rig.Reserve() == 0);
		CHECK(rig.industry->produced[0].waiting == 3);
		CHECK(GetLayerMineState(rig.industry->index)->phase == LayerMinePhase::Depleted);
		const auto work = GetLayerMineState(rig.industry->index)->work_ticks;
		CHECK(ProduceLayerMineCargo(*rig.industry, UINT32_MAX, true).error == LayerIndustryError::Depleted);
		CHECK(rig.industry->produced[0].waiting == 3);
		CHECK(GetLayerMineState(rig.industry->index)->work_ticks == work);
	}
	SECTION("One free output unit bounds a huge work allowance") {
		rig.industry->produced[0].waiting = 1023;
		const auto reserve = rig.Reserve();
		const auto result = ProduceLayerMineCargo(*rig.industry, UINT32_MAX, true);
		CHECK(result.amount == 1);
		CHECK(result.work_ticks == 18);
		CHECK(result.work_fraction == 2);
		CHECK(rig.Reserve() == reserve - 1);
		CHECK(rig.industry->produced[0].waiting == 1024);
		const auto revision = GetLayerMineState(rig.industry->index)->revision;
		CHECK(ProduceLayerMineCargo(*rig.industry, UINT32_MAX, true).amount == 0);
		CHECK(GetLayerMineState(rig.industry->index)->revision == revision);
		CHECK(rig.Reserve() == reserve - 1);
	}
	SECTION("Corrupt narrow output counter is refused unchanged") {
		rig.industry->produced[0].waiting = UINT16_MAX;
		const auto reserve = rig.Reserve();
		CHECK(ProduceLayerMineCargo(*rig.industry, UINT32_MAX, true).error == LayerIndustryError::Capacity);
		CHECK(rig.Reserve() == reserve);
		CHECK(rig.industry->produced[0].waiting == UINT16_MAX);
	}
}

/** Pausing and technology/slot refusals must preserve the actual geological and work balances. */
TEST_CASE("Inactive or changed mines preserve reserve output and work", "[layer-mine-production]")
{
	MineProductionRig rig(5);
	SECTION("Paused game") { _pause_mode.Set(PauseMode::Normal); }
	SECTION("Stopped script production") { rig.industry->prod_level = PRODLEVEL_CLOSURE; }
	SECTION("Scenario editor") { _game_mode = GameMode::Editor; }
	SECTION("Deep date locked") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("Changed physical output") { rig.industry->produced[0].cargo = GetCargoTypeByLabel(CargoLabel{"COAL"}); }
	const auto reserve = rig.Reserve();
	const auto revision = GetLayerMineState(rig.industry->index)->revision;
	CHECK(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 0);
	CHECK(rig.Reserve() == reserve);
	CHECK(rig.industry->produced[0].waiting == 0);
	CHECK(GetLayerMineState(rig.industry->index)->work_ticks == 0);
	CHECK(GetLayerMineState(rig.industry->index)->revision == revision);
}
