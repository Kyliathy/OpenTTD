/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_input_view_fixture.h Actual side-by-side viewport mouse input for delivered tube controls.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md Group4B.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * Only client window geometry moves; native world/cargo/utility owners remain untouched.
 */
#ifndef TESTS_TUBE_INPUT_VIEW_FIXTURE_H
#define TESTS_TUBE_INPUT_VIEW_FIXTURE_H

#include "native_input_fixture.h"
#include "underground_view_fixture.h"
#include "../map_layer_gui.h"

/** @param layer Explicit extra camera plane. @return Actual extra viewport positioned beside the main view, with a visible centre inside the test screen. */
static inline Window *CreateTubeInputExtraViewport(MapLayer layer)
{
	auto *extra = new UndergroundTestViewport(_underground_extra_view_desc);
	constexpr int left = 750;
	extra->viewport->left += left - extra->left;
	extra->left = left;
	SetViewportLayer(extra, layer);
	return extra;
}

/**
 * Click an actual world tile through normal mouse hit testing and placement dispatch.
 * @param view Explicit main/extra viewport already showing the tile's actual plane.
 * @param tile Real world cell selected by the player, never an implicit camera-relative alias.
 * @return Picked original tile after a real press/release; no direct window/viewport callback is invoked.
 */
static inline TileIndex MouseClickTubeWorldTile(Window &view, TileIndex tile)
{
	ScrollWindowToTile(tile, &view, true);
	// Publish the native scroll projection without repainting. These focused
	// input tests need picking and commands; native raster acceptance is separate.
	view.viewport->virtual_left = view.viewport->scrollpos_x;
	view.viewport->virtual_top = view.viewport->scrollpos_y;
	const int x = TileX(tile) * TILE_SIZE + TILE_SIZE / 2;
	const int y = TileY(tile) * TILE_SIZE + TILE_SIZE / 2;
	const Point projected = RemapCoords(x, y, GetSlopePixelZ(x, y));
	const Viewport &viewport = *view.viewport;
	const Point screen{viewport.left + UnScaleByZoom(projected.x - viewport.virtual_left, viewport.zoom),
		viewport.top + UnScaleByZoom(projected.y - viewport.virtual_top, viewport.zoom)};
	// Move only chrome covering this real click into the other visible column.
	// Main/extra camera geometry and every world owner retain their actual state;
	// a dialog must never make the test click through an invisible obstruction.
	for (Window *window : Window::Iterate()) {
		if (window->viewport != nullptr) continue;
		// The native hit target is world space only when no floating control
		// covers this screen point; preserve every unrelated dialog position.
		if (screen.x >= window->left && screen.x < window->left + window->width &&
				screen.y >= window->top && screen.y < window->top + window->height) window->left = screen.x < 640 ? 750 : 0;
	}
	const Point picked = TranslateXYToTileCoord(viewport, screen.x, screen.y);
	REQUIRE(picked.x >= 0); REQUIRE(picked.y >= 0);
	REQUIRE(TileVirtXY(picked.x, picked.y) == tile);
	REQUIRE(FindWindowFromPt(screen.x, screen.y) == &view);
	FeedbackMouse(screen, false); FeedbackMouse(screen, true); FeedbackMouse(screen, false);
	return TileVirtXY(picked.x, picked.y);
}

#endif /* TESTS_TUBE_INPUT_VIEW_FIXTURE_H */
