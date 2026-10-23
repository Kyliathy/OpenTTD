/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file smallmap_layer.cpp The small map sizes its legend for the shown map type, colours every height and follows the main view's layer. @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 2 (R1-R3, D1-D2), docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_view_fixture.h"
#include "excavation_fixture.h"
#include "../smallmap_gui.h"
#include "../widgets/smallmap_widget.h"
#include "../widgets/toolbar_widget.h"
#include "../toolbar_gui.h"
#include "../map_layer_gui.h"
#include "../map_layer.h"
#include "../gfx_func.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "../tile_map.h"
#include "../safeguards.h"

/**
 * Open the real small map with every legend built, as the game does before its first small map.
 * @return The small map window.
 */
static Window &OpenSmallMapWithLegends()
{
	BuildIndustriesLegend();
	BuildLinkStatsLegend();
	BuildOwnerLegend();
	ShowSmallMap();
	Window *map = FindWindowById(WindowClass::SmallMap, 0);
	REQUIRE(map != nullptr);
	return *map;
}

/** Deliver scheduled window invalidations, as the window system's next frame does before it draws. */
static void FlushWindowInvalidations()
{
	for (Window *w : Window::Iterate()) w->ProcessScheduledInvalidations();
}

/**
 * Press a key through the real dispatcher: focused windows, the main toolbar's hotkeys, then global hotkeys.
 * @param keycode Key with modifiers, as the video driver reports it.
 */
static void PressKey(uint keycode)
{
	HandleKeypress(keycode, 0);
	FlushWindowInvalidations();
}

/** R1: the legend reserves rows for the shown map type only, so a short legend lets the window shrink. */
TEST_CASE("Small map legend reserves rows only for the shown map type", "[smallmap-layer]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture fixture;
	UndergroundToolbarTestUI ui(_current_company);
	LayerWindowTestUI map_ui(WindowClass::SmallMap, _current_company);
	new UndergroundTestViewport();
	Window &map = OpenSmallMapWithLegends();

	// The link statistics legend reserves its seven-row link-colour column; the contour legend needs fewer rows.
	map.OnClick({}, WID_SM_CONTOUR, 1);
	const uint contour = map.nested_root->smallest_y;
	map.OnClick({}, WID_SM_LINKSTATS, 1);
	const uint links = map.nested_root->smallest_y;
	CHECK(contour < links);
	// Returning to contours gives the contour minimum back; nothing of the other legend stays reserved.
	map.OnClick({}, WID_SM_CONTOUR, 1);
	CHECK(map.nested_root->smallest_y == contour);
	// A far too short request is clamped to that minimum, which is what the player can now reach.
	ResizeWindow(&map, 0, -2000);
	CHECK(static_cast<uint>(map.height) == contour);
}

/** R1 "0 ft": a world that keeps the "auto" height limit still gets a contour colour for every height. */
TEST_CASE("Small map colours heights of a world with an unresolved height limit", "[smallmap-layer]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture fixture;
	UndergroundToolbarTestUI ui(_current_company);
	LayerWindowTestUI map_ui(WindowClass::SmallMap, _current_company);
	AutoRestoreBackup limit(_settings_game.construction.map_height_limit, static_cast<uint8_t>(0));
	const TileIndex low = TileXYL(10, 10, MapLayer::Surface);
	const TileIndex high = TileXYL(12, 10, MapLayer::Surface);
	REQUIRE(IsTileType(low, TileType::Clear));
	REQUIRE(IsTileType(high, TileType::Clear));
	SetTileHeight(low, 0);
	SetTileHeight(high, 10);
	new UndergroundTestViewport();
	OpenSmallMapWithLegends();
	// With one colour for the whole range both heights would look alike (or the high one would read past the table).
	CHECK(GetSmallMapOwnerPixels(low, TileType::Clear, IncludeHeightmap::Always) != GetSmallMapOwnerPixels(high, TileType::Clear, IncludeHeightmap::Always));
	// The game setting itself stays unresolved: only the small map interprets it.
	CHECK(_settings_game.construction.map_height_limit == 0);
}

/** D1/D2: N, Alt+1..4 and the toolbar dropdown move the main view and the small map with it; extra views never do. */
TEST_CASE("Small map follows every main view layer change", "[smallmap-layer]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture fixture;
	UndergroundToolbarTestUI ui(_current_company);
	LayerWindowTestUI map_ui(WindowClass::SmallMap, _current_company);
	Window *main = new UndergroundTestViewport();
	Window *extra = new UndergroundTestViewport(_underground_extra_view_desc);
	AllocateToolbar();
	Window *toolbar = FindWindowById(WindowClass::MainToolbar, 0);
	REQUIRE(toolbar != nullptr);
	SetViewportLayer(main, MapLayer::Surface);
	Window &map = OpenSmallMapWithLegends();
	REQUIRE(GetSmallMapLayer(&map) == MapLayer::Surface);

	// N cycles the main view in display order; the small map follows it.
	PressKey('N');
	CHECK(main->viewport->layer == MapLayer::Underground);
	CHECK(GetSmallMapLayer(&map) == MapLayer::Underground);

	// Alt+1..4 select the layers in display order: Surface, Underground, Deep Underground, Outer Space.
	const std::array<MapLayer, 4> order{MapLayer::Surface, MapLayer::Underground, MapLayer::DeepUnderground, MapLayer::Space};
	for (uint index = 0; index < order.size(); ++index) {
		PressKey(WKC_ALT | ('1' + index));
		CHECK(main->viewport->layer == order[index]);
		CHECK(GetSmallMapLayer(&map) == order[index]);
	}

	// The toolbar's layer dropdown is one more main-view change.
	toolbar->OnDropdownSelect(WID_TN_MAP_LAYER, to_underlying(MapLayer::Underground), 0);
	FlushWindowInvalidations();
	CHECK(GetSmallMapLayer(&map) == MapLayer::Underground);

	// The small map's own dropdown previews another layer without moving the main view.
	map.OnDropdownSelect(WID_SM_MAP_LAYER, to_underlying(MapLayer::DeepUnderground), 0);
	CHECK(GetSmallMapLayer(&map) == MapLayer::DeepUnderground);
	CHECK(main->viewport->layer == MapLayer::Underground);

	// An extra viewport's layer never moves the small map.
	SetViewportLayer(extra, MapLayer::Space);
	FlushWindowInvalidations();
	CHECK(GetSmallMapLayer(&map) == MapLayer::DeepUnderground);

	// The next main-view change takes the preview back to the main view's layer.
	PressKey(WKC_ALT | '1');
	CHECK(main->viewport->layer == MapLayer::Surface);
	CHECK(GetSmallMapLayer(&map) == MapLayer::Surface);
}

/** D2: a per-layer key for a layer this game did not allocate is ignored. */
TEST_CASE("Per-layer keys ignore layers the map did not allocate", "[smallmap-layer]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture fixture;
	UndergroundToolbarTestUI ui(_current_company);
	// A two-layer world: Surface and Underground only; the fixture's teardown reallocates nothing.
	Map::Allocate(64, 64, 2);
	for (MapLayer layer : {MapLayer::Surface, MapLayer::Underground}) MultiLayerMap::InitialiseLayer(layer);
	Window *main = new UndergroundTestViewport();
	SetViewportLayer(main, MapLayer::Surface);
	// Deep Underground and Outer Space are third and fourth in display order and absent here.
	CHECK_FALSE(ShowMainViewportLayerInDisplayOrder(2));
	CHECK_FALSE(ShowMainViewportLayerInDisplayOrder(3));
	CHECK_FALSE(ShowMainViewportLayerInDisplayOrder(4));
	CHECK(main->viewport->layer == MapLayer::Surface);
	CHECK(ShowMainViewportLayerInDisplayOrder(1));
	CHECK(main->viewport->layer == MapLayer::Underground);
}
