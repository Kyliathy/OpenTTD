/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_mine_presentation.cpp Derived operating status reads sole mine/geology/output owners without advancing simulation. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 15 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mine_production_fixture.h"

#include "../safeguards.h"

/** Actual calendar, pause, construction and company states explain availability without changing reserve or work. */
TEST_CASE("Mine status follows actual availability without producing or dispatching cargo", "[layer-mine-presentation]")
{
	MineProductionRig rig(5);
	const auto before = *GetLayerMineState(rig.industry->index);
	const uint64_t reserve = rig.Reserve();
	CHECK(GetLayerMineOperationalStatus(*rig.industry) == LayerMineOperationalStatus::Operating);
	rig.industry->prod_level = PRODLEVEL_CLOSURE;
	CHECK(GetLayerMineOperationalStatus(*rig.industry) == LayerMineOperationalStatus::Stopped);
	rig.industry->prod_level = PRODLEVEL_DEFAULT;
	_pause_mode.Set(PauseMode::Normal);
	CHECK(GetLayerMineOperationalStatus(*rig.industry) == LayerMineOperationalStatus::Paused);
	_pause_mode = {};
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK(GetLayerMineOperationalStatus(*rig.industry) == LayerMineOperationalStatus::Unavailable);
	CHECK(rig.Reserve() == reserve);
	CHECK(GetLayerMineState(rig.industry->index)->work_ticks == before.work_ticks);
	CHECK(GetLayerMineState(rig.industry->index)->revision == before.revision);
}

/** Finite producer output and exhausted reserve determine distinct storage/depletion messages; read-only quotes never allocate supply. */
TEST_CASE("Mine status distinguishes pending pickup full output and permanent depletion", "[layer-mine-presentation]")
{
	MineProductionRig rig;
	const auto *capability = GetLayerIndustryCapability(rig.industry->type);
	REQUIRE(capability != nullptr);
	REQUIRE(rig.Reserve() > capability->output_capacity);
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	CHECK(GetLayerMineOperationalStatus(*rig.industry) == LayerMineOperationalStatus::AwaitPickup);
	// Fill output through actual reserve debits so the presentation fixture never invents historical extracted stock.
	for (uint32_t units = 4; units < capability->output_capacity; units += 4) REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	CHECK(GetLayerMineOperationalStatus(*rig.industry) == LayerMineOperationalStatus::OutputFull);
	CHECK(GetLayerMineState(rig.industry->index)->extracted == capability->output_capacity);
	CHECK(rig.industry->produced[0].waiting == capability->output_capacity);
}

/** The final geological unit remains visible as real output while the finite seam stays depleted. */
TEST_CASE("Mine status preserves depleted reserve and stored final cargo", "[layer-mine-presentation]")
{
	MineProductionRig rig;
	rig.SetReserve(1);
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 1);
	CHECK(GetLayerMineOperationalStatus(*rig.industry) == LayerMineOperationalStatus::Depleted);
	CHECK(rig.Reserve() == 0);
	CHECK(rig.industry->produced[0].waiting == 1);
	CHECK(GetLayerMineState(rig.industry->index)->dispatched == 0);
}
