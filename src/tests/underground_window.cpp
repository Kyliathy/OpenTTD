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
#include "underground_view_fixture.h"
#include "excavation_fixture.h"
#include "tbm_commission_fixture.h"
#include "../underground_gui.h"
#include "../widgets/underground_widget.h"
#include "../widgets/toolbar_widget.h"
#include "../toolbar_gui.h"
#include "../map_layer_gui.h"
#include "../viewport_type.h"
#include "../viewport_func.h"
#include "../zoom_func.h"
#include "../landscape.h"
#include "../construction_preview.h"
#include "../tilehighlight_func.h"
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
	CHECK(deep->GetWidgetString(WID_UC_MODEL_DATES, STR_UNDERGROUND_AREA_EMPTY).find("Deep TBM (1980)") != std::string::npos);
	CHECK(underground->GetWidgetString(WID_UC_MODEL_DATES, STR_UNDERGROUND_AREA_EMPTY).find("Standard TBM (1950)") != std::string::npos);
	CHECK(underground->GetWidgetString(WID_UC_MODEL_DATES, STR_UNDERGROUND_AREA_EMPTY).find("Deep TBM (1980)") != std::string::npos);
}

/** The configured airport shortcut, normal/split toolbar identity and editor construction entry share rock-plane routing. */
TEST_CASE("Main and editor toolbar entries open the captured underground layer", "[underground-toolbar]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture fixture;
	UndergroundToolbarTestUI ui(_current_company);
	Window *view = new UndergroundTestViewport();
	AllocateToolbar();
	Window *toolbar = FindWindowById(WindowClass::MainToolbar, 0);
	REQUIRE(toolbar != nullptr);
	// The same configured action must open the explicitly viewed rock plane through the existing toolbar class.
	for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		SetViewportLayer(view, layer);
		toolbar->OnPaint();
		CHECK(toolbar->GetWidget<NWidgetCore>(WID_TN_AIR)->GetToolTip() == STR_UNDERGROUND_MENU);
		CHECK(toolbar->OnHotkey(MTHK_BUILD_AIRPORT) == EventState::Handled);
		REQUIRE(FindWindowById(WindowClass::UndergroundConstruction, layer) != nullptr);
	}
	// Dropdown identity was captured before this view changed; its result still opens Underground.
	SetViewportLayer(view, MapLayer::Surface);
	toolbar->OnDropdownSelect(WID_TN_AIR, 100 + to_underlying(MapLayer::Underground), -1);
	CHECK(FindWindowById(WindowClass::UndergroundConstruction, MapLayer::Underground) != nullptr);
	toolbar->OnPaint();
	CHECK(toolbar->GetWidget<NWidgetCore>(WID_TN_AIR)->GetToolTip() == STR_TOOLBAR_TOOLTIP_BUILD_AIRPORTS);
	CloseWindowByClass(WindowClass::MainToolbar);
	Window::DeleteClosedWindows();
	// Force the existing narrow-screen arrangement, then verify the shared entry after switching its displayed row.
	// Mock icons are one pixel wide; fourteen screen pixels force the minimum arrangement even at that scale.
	_screen.width = 14;
	ScreenSizeChanged();
	SetViewportLayer(view, MapLayer::DeepUnderground);
	AllocateToolbar();
	toolbar = FindWindowById(WindowClass::MainToolbar, 0);
	REQUIRE(toolbar != nullptr);
	ResizeWindow(toolbar, 14 - toolbar->width, 0, false, false);
	toolbar->OnClick({}, WID_TN_SWITCH_BAR, 1);
	toolbar->OnPaint();
	CHECK(toolbar->IsWidgetLowered(WID_TN_SWITCH_BAR));
	CHECK(toolbar->GetWidget<NWidgetCore>(WID_TN_AIR)->GetToolTip() == STR_UNDERGROUND_MENU);
	CHECK(toolbar->OnHotkey(MTHK_BUILD_AIRPORT) == EventState::Handled);
	CloseWindowByClass(WindowClass::MainToolbar);
	Window::DeleteClosedWindows();
	_screen.width = 1280;
	ScreenSizeChanged();
	_game_mode = GameMode::Editor;
	SetViewportLayer(view, MapLayer::Underground);
	AllocateToolbar();
	toolbar = FindWindowById(WindowClass::MainToolbar, 0);
	REQUIRE(toolbar != nullptr);
	toolbar->OnPaint();
	CHECK(toolbar->GetWidget<NWidgetCore>(WID_TE_INDUSTRY)->GetToolTip() == STR_UNDERGROUND_MENU);
	toolbar->OnClick({}, WID_TE_INDUSTRY, 1);
	CHECK(FindWindowById(WindowClass::UndergroundConstruction, MapLayer::Underground) != nullptr);
}

/** Both viewport owners cancel their unfinished world-coordinate drag before a displayed layer switch. */
TEST_CASE("Changing either viewport layer visibly cancels an unfinished underground selection", "[underground-placement]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture fixture;
	UndergroundToolbarTestUI ui(_current_company);
	Window *main = new UndergroundTestViewport();
	Window *extra = new UndergroundTestViewport(_underground_extra_view_desc);
	Window *tools = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(tools != nullptr);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	// Exercise each real view's layer handler while the shared cursor is a live sizing drag owned by the native tool.
	for (Window *view : {main, extra}) {
		SetViewportLayer(view, MapLayer::Underground);
		tools->OnClick({}, WID_UC_SURVEY, 1);
		REQUIRE(_thd.GetCallbackWnd() == tools);
		VpStartPlaceSizing(TileXYL(15, 15, MapLayer::Underground), VPM_X_AND_Y, DDSP_DEMOLISH_AREA);
		CHECK(tools->IsWidgetLowered(WID_UC_SURVEY));
		SetViewportLayer(view, MapLayer::DeepUnderground);
		CHECK(_thd.place_mode == HT_NONE);
		CHECK_FALSE(tools->IsWidgetLowered(WID_UC_SURVEY));
		CHECK(tools->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_UNDERGROUND_SELECT_TOOL));
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	}
	CHECK(main->viewport->layer == MapLayer::DeepUnderground);
	CHECK(extra->viewport->layer == MapLayer::DeepUnderground);
}

/** Real main/extra viewport picking posts the same native area purchase, and a centring layer switch refuses the unfinished one. */
TEST_CASE("Area purchase from either actual viewport preserves the clicked world layer", "[underground-viewport-command]")
{
	bool use_extra = false;
	bool switch_before_confirm = false;
	SECTION("main viewport") {}
	SECTION("extra viewport with Surface still in the main view") { use_extra = true; }
	SECTION("extra viewport switches layer before confirmation") { use_extra = true; switch_before_confirm = true; }
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMCommissionRig c;
	UndergroundToolbarTestUI ui(_current_company);
	AutoRestoreBackup<CommandPauseLevel> pause_commands(_settings_game.construction.command_pause_level, CommandPauseLevel::AllActions);
	AutoRestoreBackup<bool> shift(_shift_pressed, false);
	AutoRestoreBackup<Point> cursor(_cursor.pos);
	Window *main = new UndergroundTestViewport();
	Window *view = use_extra ? new UndergroundTestViewport(_underground_extra_view_desc) : main;
	CHECK(view->viewport->virtual_width == ScaleByZoom(view->viewport->width, view->viewport->zoom));
	CHECK(view->viewport->virtual_height == ScaleByZoom(view->viewport->height, view->viewport->zoom));
	ScrollWindowToTile(c.rock, view, true);
	Window *tools = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(tools != nullptr);
	// Keep the actual controls beside the test viewport so ordinary mouse picking reaches the visible world.
	tools->left = 750;
	tools->OnPaint();
	const auto before = c.rig.Digest();
	const auto projects = CaptureTBMProjects().size();
	tools->OnClick({}, WID_UC_AREA, 1);
	ClickUndergroundWorldTile(view, c.rock);
	const TileIndex start = ClickUndergroundWorldTile(view, c.start);
	tools->OnPlaceDrag(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {static_cast<int>(TileX(c.end) * TILE_SIZE), static_cast<int>(TileY(c.end) * TILE_SIZE)});
	tools->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, start, c.end);
	tools->OnPaint();
	REQUIRE_FALSE(tools->IsWidgetDisabled(WID_UC_AREA_CONFIRM));
	CHECK(c.rig.Digest() == before);
	if (use_extra) CHECK(main->viewport->layer == MapLayer::Surface);
	if (switch_before_confirm) {
		// Tile-centred navigation must cancel the same live selection as the explicit layer menu, before any purchase.
		ScrollWindowToTile(MultiLayerMap::TileInLayer(c.start, MapLayer::DeepUnderground), view, true);
		tools->OnPaint();
		CHECK(tools->IsWidgetDisabled(WID_UC_AREA_CONFIRM));
		tools->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, start, c.end);
		tools->OnClick({}, WID_UC_AREA_CONFIRM, 1);
		CHECK(GetLayerPortalWorkState(c.portal) == LayerPortalWorkState::Sealed);
		CHECK(c.rig.Digest() == before);
	} else {
		tools->OnClick({}, WID_UC_AREA_CONFIRM, 1);
		REQUIRE(GetLayerPortalWorkState(c.portal) == LayerPortalWorkState::Commissioning);
		REQUIRE(CaptureTBMProjects().size() == projects + 1);
		const auto project = CaptureTBMProjects().back();
		const auto *job = GetExcavationJob(project.job);
		REQUIRE(job != nullptr);
		CHECK(project.portal == c.portal);
		CHECK(GetTileLayer(job->commissioning_anchor) == MapLayer::Underground);
		CHECK(job->requested_tiles.front() == c.start);
		CHECK(Company::Get(_current_company)->money < before.money);
		CHECK(_thd.place_mode == HT_NONE);
	}
	CHECK(GetConstructionPreviewTile(c.start) == ConstructionPreviewTile::None);
}
