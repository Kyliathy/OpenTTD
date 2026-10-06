/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_window.cpp Actual underground window, survey purity and calendar/context fixtures. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Groups 1-3 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "excavation_fixture.h"
#include "../underground_gui.h"
#include "../widgets/underground_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Real widget initialization and survey clicks cannot change geological reserves, work, cash or RNG. */
TEST_CASE("Underground tools open each rock layer and survey without mutation", "[underground-window]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture fixture;
	LayerWindowTestUI ui(WindowClass::UndergroundConstruction, _current_company);
	const auto policy = GetCurrentMultiLayerProfileSelection();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto money = Company::Get(_current_company)->money;
	// Both real rock-plane windows must preserve their own identities and refuse a cross-plane survey click.
	for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		Window *window = ShowUndergroundConstructionToolbar(layer, policy);
		REQUIRE(window != nullptr);
		CHECK(window->width > 0);
		CHECK(window->height > 0);
		window->OnPaint();
		CHECK_FALSE(window->IsWidgetDisabled(WID_UC_RAIL));
		CHECK_FALSE(window->IsWidgetDisabled(WID_UC_ROAD_TERMINAL));
		CHECK(ShowUndergroundConstructionToolbar(layer, policy) == window);
		window->OnClick({}, WID_UC_SURVEY, 1);
		window->OnPlaceObject({}, TileXYL(15, 15, layer));
		CHECK(window->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL).find("Solid rock") != std::string::npos);
		window->OnPlaceObject({}, TileXYL(15, 15, MapLayer::Surface));
		CHECK(window->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL).find("another layer") != std::string::npos);
		window->OnPlaceObjectAbort();
		window->OnClick({}, WID_UC_EXCHANGE, 1);
		CHECK(window->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL).find("No active project") != std::string::npos);
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(ShowUndergroundConstructionToolbar(MapLayer::Surface, policy) == nullptr);
	CHECK(ShowUndergroundConstructionToolbar(MapLayer::Space, policy) == nullptr);
	auto invalid = policy;
	++invalid.version;
	CHECK(ShowUndergroundConstructionToolbar(MapLayer::Underground, invalid) == nullptr);
}

/** Deep availability and visible dates come from the content calendar owner; Underground stays available. */
TEST_CASE("Deep underground tools explain the canonical calendar unlock", "[underground-window]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture fixture;
	LayerWindowTestUI ui(WindowClass::UndergroundConstruction, _current_company);
	const auto policy = GetCurrentMultiLayerProfileSelection();
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	Window *deep = ShowUndergroundConstructionToolbar(MapLayer::DeepUnderground, policy);
	REQUIRE(deep != nullptr);
	deep->OnPaint();
	CHECK(deep->IsWidgetDisabled(WID_UC_SURVEY));
	CHECK(deep->IsWidgetDisabled(WID_UC_RAIL_STATION));
	CHECK(deep->GetWidgetString(WID_UC_REQUIREMENTS, STR_UNDERGROUND_REQUIREMENTS).find("1980") != std::string::npos);
	Window *underground = ShowUndergroundConstructionToolbar(MapLayer::Underground, policy);
	REQUIRE(underground != nullptr);
	underground->OnPaint();
	CHECK_FALSE(underground->IsWidgetDisabled(WID_UC_RAIL_STATION));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	deep->OnPaint();
	CHECK_FALSE(deep->IsWidgetDisabled(WID_UC_RAIL_STATION));
	CHECK_FALSE(deep->IsWidgetDisabled(WID_UC_RAIL_SERVICE));
}
