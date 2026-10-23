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
#include "../widgets/industry_widget.h"

#include "../safeguards.h"

/** The information, recipe and footer must occupy separate rows, and settled paints must not rebuild/recentre the viewport. */
static void CheckIndustryWindowLayout(Window *window)
{
	window->OnPaint(); // Allow the information panel to acquire its measured height.
	const Rect info = window->GetWidget<NWidgetBase>(WID_IV_INFO)->GetCurrentRect();
	const Rect recipe = window->GetWidget<NWidgetBase>(WID_IV_RECIPE)->GetCurrentRect();
	const Rect footer = window->GetWidget<NWidgetBase>(WID_IV_GRAPH)->GetCurrentRect();
	CHECK(info.bottom < recipe.top);
	CHECK(recipe.bottom < footer.top);
	CHECK(info.Height() > 2 * GetCharacterHeight(FontSize::Normal) + static_cast<int>(WidgetDimensions::scaled.framerect.Vertical()));

	// A repeated ReInit can leave the dimensions unchanged while invalidating and recentring the window every frame.
	const int scroll_x = ++window->viewport->dest_scrollpos_x;
	for (int paint = 0; paint < 3; ++paint) {
		window->OnPaint();
		CHECK(window->viewport->dest_scrollpos_x == scroll_x);
		CHECK(window->GetWidget<NWidgetBase>(WID_IV_INFO)->GetCurrentRect().bottom == info.bottom);
	}
}

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
	CheckIndustryWindowLayout(window);
	ResizeWindow(window, 160, 0, false, false);
	CheckIndustryWindowLayout(window); // Wrapping must settle again after the player widens the window.
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
	CheckIndustryWindowLayout(window);
	ResizeWindow(window, 160, 0, false, false);
	CheckIndustryWindowLayout(window);
	CHECK(GetLayerProcessorState(rig.industry->index)->revision == revision);
	CHECK(rig.Input(CargoLabel{"NORE"}).waiting == 4);
}
