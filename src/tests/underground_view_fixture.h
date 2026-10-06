/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_view_fixture.h Shared real viewport/event setup for connected underground workflows. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Groups 3D/9B-9D */

#ifndef TESTS_UNDERGROUND_VIEW_FIXTURE_H
#define TESTS_UNDERGROUND_VIEW_FIXTURE_H

#include "layer_window_fixture.h"
#include "../viewport_type.h"
#include "../viewport_func.h"
#include "../zoom_func.h"
#include "../landscape.h"
#include "../tilehighlight_func.h"

/** Viewport geometry for an actual main-toolbar caller; no screen raster assets are required. */
static constexpr std::initializer_list<NWidgetPart> _underground_main_view_widgets = {
	NWidget(NWID_VIEWPORT, Colours::Invalid, 0), SetMinimalSize(640, 360), SetResize(1, 1),
};
/** Main-window identity and viewport used by the real toolbar's existing public entry. */
static WindowDesc _underground_main_view_desc(WindowPosition::Manual, {}, 640, 360, WindowClass::MainWindow, WindowClass::None, {}, _underground_main_view_widgets);
/** Independent extra viewport geometry; its layer must not redefine the main viewport or simulation. */
static WindowDesc _underground_extra_view_desc(WindowPosition::Manual, {}, 640, 360, WindowClass::ExtraViewport, WindowClass::None, {}, _underground_main_view_widgets);

/** Actual viewport owner for testing toolbar/hotkey routing across camera layer changes. */
class UndergroundTestViewport : public Window {
public:
	/** @param desc Main or extra viewport identity; initialize its actual geometry on the fixture's Surface plane. */
	explicit UndergroundTestViewport(WindowDesc &desc = _underground_main_view_desc) : Window(desc)
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
		CloseWindowByClass(WindowClass::ExtraViewport);
		CloseWindowByClass(WindowClass::MainWindow);
		Window::DeleteClosedWindows();
	}
};

/**
 * Pick a requested tile using the real viewport projection and ordinary placement event dispatcher.
 * @param view Actual main or extra viewport, already on the intended plane.
 * @param tile Actual world tile the user clicks.
 * @param offset Visible point within the tile; road drags use its lower half to build a complete first approach.
 * @return Picked world tile; the real placement window has received its normal callback.
 */
static TileIndex ClickUndergroundWorldTile(Window *view, TileIndex tile, Point offset = {TILE_SIZE / 2, TILE_SIZE / 2})
{
	ScrollWindowToTile(tile, view, true);
	// Publish the native scroll projection without invoking raster repaint: this fixture tests picking and commands.
	// A full repaint needs the game's sprite-sorter/video-driver startup and belongs to the separate runtime art checks.
	view->viewport->virtual_left = view->viewport->scrollpos_x;
	view->viewport->virtual_top = view->viewport->scrollpos_y;
	const int world_x = TileX(tile) * TILE_SIZE + offset.x;
	const int world_y = TileY(tile) * TILE_SIZE + offset.y;
	const Point projected = RemapCoords(world_x, world_y, GetSlopePixelZ(world_x, world_y));
	const Viewport &viewport = *view->viewport;
	const Point screen{viewport.left + UnScaleByZoom(projected.x - viewport.virtual_left, viewport.zoom),
		viewport.top + UnScaleByZoom(projected.y - viewport.virtual_top, viewport.zoom)};
	// Move floating dialogs beside the viewport, as a player can, so the ordinary global cursor picker reaches this visible world point.
	// Calling HandleViewportClicked alone does not make placement click through a vehicle, purchase or other overlapping window.
	for (Window *window : Window::Iterate()) {
		// Main/extra viewports keep their actual camera identity; only ordinary window chrome is moved out of the requested click.
		if (window == view || window->window_class == WindowClass::MainWindow || window->window_class == WindowClass::ExtraViewport) continue;
		// Reposition only a dialog that actually covers this click, preserving all other window geometry and toolbar arrangements.
		if (screen.x >= window->left && screen.x < window->left + window->width && screen.y >= window->top && screen.y < window->top + window->height) {
			if (window->viewport != nullptr) window->viewport->left += 750 - window->left;
			window->left = 750;
		}
	}
	const Point picked = TranslateXYToTileCoord(viewport, screen.x, screen.y);
	INFO("viewport=" << viewport.left << ',' << viewport.top << ',' << viewport.width << ',' << viewport.height
		<< " virtual=" << viewport.virtual_left << ',' << viewport.virtual_top << ',' << viewport.virtual_width << ',' << viewport.virtual_height
		<< " screen=" << screen.x << ',' << screen.y << " projected=" << projected.x << ',' << projected.y);
	REQUIRE(picked.x >= 0);
	REQUIRE(picked.y >= 0);
	REQUIRE(TileVirtXY(picked.x, picked.y) == tile);
	_cursor.pos = screen;
	REQUIRE(HandleViewportClicked(viewport, screen.x, screen.y));
	return TileVirtXY(picked.x, picked.y);
}

#endif /* TESTS_UNDERGROUND_VIEW_FIXTURE_H */
