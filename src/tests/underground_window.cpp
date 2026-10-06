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
#include "../widgets/toolbar_widget.h"
#include "../toolbar_gui.h"
#include "../map_layer_gui.h"
#include "../viewport_type.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Viewport geometry for an actual main-toolbar caller; no screen raster assets are required. */
static constexpr std::initializer_list<NWidgetPart> _underground_main_view_widgets = {
	NWidget(NWID_VIEWPORT, Colours::Invalid, 0), SetMinimalSize(640, 360), SetResize(1, 1),
};
/** Main-window identity and viewport used by the real toolbar's existing public entry. */
static WindowDesc _underground_main_view_desc(WindowPosition::Manual, {}, 640, 360, WindowClass::MainWindow, WindowClass::None, {}, _underground_main_view_widgets);

/** Actual viewport owner for testing toolbar/hotkey routing across camera layer changes. */
class UndergroundMainTestViewport : public Window {
public:
	/** Initialize real viewport geometry on the fixture's Surface plane. */
	UndergroundMainTestViewport() : Window(_underground_main_view_desc)
	{
		this->InitNested(0);
		this->GetWidget<NWidgetViewport>(0)->InitializeViewport(this, TileXY(32, 32), ZoomLevel::Viewport);
	}
};

/** Close real toolbar and viewport families before the inherited fixture closes its underground controls. */
class UndergroundToolbarTestUI : public LayerWindowTestUI {
public:
	/** @param company Real player used by the toolbar's command-independent availability. */
	explicit UndergroundToolbarTestUI(CompanyID company) : LayerWindowTestUI(WindowClass::UndergroundConstruction, company) {}
	/** Close all helper windows while the test-owned company/map still exist. */
	~UndergroundToolbarTestUI()
	{
		CloseWindowByClass(WindowClass::MainToolbar);
		CloseWindowByClass(WindowClass::MainWindow);
		Window::DeleteClosedWindows();
	}
};

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

/** The configured airport shortcut, normal/split toolbar identity and editor construction entry share rock-plane routing. */
TEST_CASE("Main and editor toolbar entries open the captured underground layer", "[underground-toolbar]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture fixture;
	UndergroundToolbarTestUI ui(_current_company);
	Window *view = new UndergroundMainTestViewport();
	AllocateToolbar();
	Window *toolbar = FindWindowById(WindowClass::MainToolbar, 0);
	REQUIRE(toolbar != nullptr);
	// The same configured action must open the explicitly viewed rock plane through the existing toolbar class.
	for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		SetViewportLayer(view, layer);
		toolbar->OnPaint();
		CHECK(toolbar->GetWidget<NWidgetCore>(WID_TN_AIR)->tool_tip == STR_UNDERGROUND_MENU);
		CHECK(toolbar->OnHotkey(MTHK_BUILD_AIRPORT) == EventState::Handled);
		REQUIRE(FindWindowById(WindowClass::UndergroundConstruction, layer) != nullptr);
	}
	// Dropdown identity was captured before this view changed; its result still opens Underground.
	SetViewportLayer(view, MapLayer::Surface);
	toolbar->OnDropdownSelect(WID_TN_AIR, 100 + to_underlying(MapLayer::Underground), -1);
	CHECK(FindWindowById(WindowClass::UndergroundConstruction, MapLayer::Underground) != nullptr);
	toolbar->OnPaint();
	CHECK(toolbar->GetWidget<NWidgetCore>(WID_TN_AIR)->tool_tip == STR_TOOLBAR_TOOLTIP_BUILD_AIRPORTS);
	CloseWindowByClass(WindowClass::MainToolbar);
	Window::DeleteClosedWindows();
	_game_mode = GameMode::Editor;
	SetViewportLayer(view, MapLayer::Underground);
	AllocateToolbar();
	toolbar = FindWindowById(WindowClass::MainToolbar, 0);
	REQUIRE(toolbar != nullptr);
	toolbar->OnPaint();
	CHECK(toolbar->GetWidget<NWidgetCore>(WID_TE_INDUSTRY)->tool_tip == STR_UNDERGROUND_MENU);
	toolbar->OnClick({}, WID_TE_INDUSTRY, 1);
	CHECK(FindWindowById(WindowClass::UndergroundConstruction, MapLayer::Underground) != nullptr);
}
