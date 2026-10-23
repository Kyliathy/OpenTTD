/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file native_input_fixture.h Real mouse/keyboard input for native observers of a loaded game.
 *
 * Observers drive the player's actual input path (hit testing, modal/cursor ownership, deferred dropdowns) instead of
 * calling window callbacks directly, so a reproduction exercises the same code as a person at the keyboard.
 * Shared by the TBM feedback and demo-feedback observers.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group1
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 1
 * @see docs/zz-reach2/architecture/gui/archi-gui.md
 */

#ifndef TESTS_NATIVE_INPUT_FIXTURE_H
#define TESTS_NATIVE_INPUT_FIXTURE_H

#include "../window_gui.h"
#include "../window_func.h"
#include "../gfx_func.h"
#include "../viewport_func.h"
#include "../landscape.h"
#include "../zoom_func.h"
#include "../timer/timer.h"
#include "../timer/timer_window.h"

/**
 * Feed a real press/release through native hit testing, including modal/cursor ownership.
 * @param point Screen position supplied by the automated input device.
 * @param down Whether the left button remains down (for an actual drag).
 */
static inline void FeedbackMouse(Point point, bool down)
{
	_cursor.pos = point;
	_cursor.in_window = true;
	_left_button_down = down;
	if (!down) _left_button_clicked = false;
	HandleMouseEvents();
}

/**
 * Click the centre of a real, currently visible widget through the native input dispatcher.
 * @param window Existing owner; no direct OnClick invocation bypasses hit testing.
 * @param widget Widget whose real layout supplies the coordinates.
 */
static inline void FeedbackClick(Window &window, WidgetID widget)
{
	const Rect rect = window.GetWidget<NWidgetBase>(widget)->GetCurrentRect();
	const Point point{window.left + (rect.left + rect.right) / 2, window.top + (rect.top + rect.bottom) / 2};
	FeedbackMouse(point, false);
	FeedbackMouse(point, true);
	FeedbackMouse(point, false);
}

/**
 * Resolve a physical tile to its visible main-viewport position for ordinary placement input.
 * @param tile Actual explicit-layer world tile, centred before use.
 * @return Screen position of the tile's ground centre.
 */
static inline Point FeedbackTilePoint(TileIndex tile)
{
	const Viewport &view = *GetMainWindow()->viewport;
	const Point world = RemapCoords(TileX(tile) * TILE_SIZE + TILE_SIZE / 2, TileY(tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(tile));
	return {view.left + UnScaleByZoom(world.x - view.virtual_left, view.zoom), view.top + UnScaleByZoom(world.y - view.virtual_top, view.zoom)};
}

/**
 * Let native dropdown and deferred window input dispatch, without pressing any button again.
 * Native dropdown selections are applied after a few ordinary mouse-loop passes.
 * @param passes Number of 30 ms window-timer passes to run.
 */
static inline void FeedbackPumpWindows(uint passes = 5)
{
	for (uint pass = 0; pass < passes; ++pass) TimerManager<TimerWindow>::Elapsed(std::chrono::milliseconds(30));
}

#endif /* TESTS_NATIVE_INPUT_FIXTURE_H */
