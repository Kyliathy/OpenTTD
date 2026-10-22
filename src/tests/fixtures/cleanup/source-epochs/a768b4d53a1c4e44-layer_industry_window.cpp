/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_industry_window.cpp Actual tile-click construction and paint of mine/factory controls with generated language. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 15 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "mock_environment.h"
#include "mine_production_fixture.h"
#include "processor_fixture.h"
#include "../tile_cmd.h"

#include "../safeguards.h"

/** Actual underground machinery opens its expanded reserve/status/storage panel without a widget exception or production side effect. */
TEST_CASE("Native mine tile opens its real reserve and storage controls", "[layer-industry-window]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	MineProductionRig rig;
	LayerWindowTestUI ui(WindowClass::IndustryView, _current_company);
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	const auto reserve = rig.Reserve();
	const auto revision = GetLayerMineState(rig.industry->index)->revision;
	REQUIRE(ClickTile(rig.industry->location.tile));
	Window *window = FindWindowById(WindowClass::IndustryView, rig.industry->index);
	REQUIRE(window != nullptr);
	CHECK(window->width > 0);
	CHECK(window->height > 0);
	window->OnPaint();
	window->OnPaint(); // First paint may resize the real info panel; the second must reuse its complete layout.
	CHECK(rig.Reserve() == reserve);
	CHECK(GetLayerMineState(rig.industry->index)->revision == revision);
	CHECK(rig.industry->produced[0].waiting == 4);
}

/** The real refinery window exposes four actual incoming slots and its choice control without requiring a three-input callback translation table. */
TEST_CASE("Native factory tile opens real input storage and recipe controls", "[layer-industry-window]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ProcessorRig rig;
	LayerWindowTestUI ui(WindowClass::IndustryView, _current_company);
	REQUIRE(rig.industry->accepted.size() == 4);
	REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(CargoLabel{"NORE"}), 4) == 4);
	const auto revision = GetLayerProcessorState(rig.industry->index)->revision;
	REQUIRE(ClickTile(rig.industry->location.tile));
	Window *window = FindWindowById(WindowClass::IndustryView, rig.industry->index);
	REQUIRE(window != nullptr);
	CHECK(window->width > 0);
	CHECK(window->height > 0);
	window->OnPaint();
	window->OnPaint();
	CHECK(GetLayerProcessorState(rig.industry->index)->revision == revision);
	CHECK(rig.Input(CargoLabel{"NORE"}).waiting == 4);
}
