/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_hangar_input.h Native mouse selection and real rendered evidence for selected hangars.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group8
 * @see docs/zz-reach2/architecture/gui/archi-gui.md
 */

#ifndef TESTS_TBM_HANGAR_INPUT_H
#define TESTS_TBM_HANGAR_INPUT_H

#include <filesystem>
#include "native_art_render_fixture.h"
#include "../tbm_gui.h"
#include "../tbm_hangar.h"
#include "../depot_base.h"
#include "../dropdown_func.h"
#include "../vehicle_gui.h"
#include "../gui.h"
#include "../widgets/tbm_widget.h"
#include "../widgets/vehicle_widget.h"
#include "../widgets/dropdown_widget.h"
#include "../widgets/order_widget.h"
#include "../timer/timer_window.h"
#include "../timer/timer.h"
#include "../window_func.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../table/strings.h"

/** @param elapsed Native window time; presentation timers neither cut rock nor move vehicles. */
static void ElapseHangarInputTimers(uint elapsed = 5)
{
	for (uint pass = 0; pass < elapsed; ++pass) TimerManager<TimerWindow>::Elapsed(std::chrono::milliseconds(30));
}

/** @param window Actual window. @param point Local visible point. Dispatch cursor movement and native press/release hit testing. */
static void ClickHangarPoint(Window &window, Point point)
{
	_cursor.UpdateCursorPosition(window.left + point.x, window.top + point.y);
	_cursor.in_window = true;
	_left_button_down = _left_button_clicked = false;
	HandleMouseEvents();
	ElapseHangarInputTimers(); // Dismiss any previous tooltip before the new real hit test.
	REQUIRE(FindWindowFromPt(_cursor.pos.x, _cursor.pos.y) == &window);
	for (bool down : {false, true, false}) {
		_left_button_down = down;
		if (!down) _left_button_clicked = false;
		HandleMouseEvents();
	}
}

/** @param window Real rendered window. @param name Current-group evidence name. Capture native pixels without simulation changes. */
static void CaptureHangarWindow(Window &window, std::string_view name)
{
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
	GfxClearSpriteCache();
	FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	CaptureNativeArtWindow(window, std::filesystem::current_path().parent_path() /
		"docs/zz-reach2/upgrades/2026-10/assets/reviews" / fmt::format("tbm-hangar-{}.ppm", name));
	REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
	GfxClearSpriteCache();
	FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
}

/**
 * Select an actual compatible hangar through the visible project or vehicle menu.
 * @param project Actual loaded job; source, exchange and work remain with their real owners.
 * @param target Real facility tile to locate in the native dropdown.
 * @param vehicle_menu True uses the vehicle's depot button; false uses the project palette.
 * @return None; the UI posts the synchronized command, never a direct callback or private order mutation.
 */
static void SelectHangarThroughInput(const TBMProject &project, TileIndex target, bool vehicle_menu)
{
	const Train *front = Train::Get(project.cutter);
	Window *window;
	const WidgetID widget = vehicle_menu ? WID_VV_GOTO_DEPOT : WID_TP_HANGAR;
	if (vehicle_menu) {
		ShowVehicleViewWindow(front);
		window = FindWindowById(WindowClass::VehicleView, front->index);
	} else {
		window = ShowTBMProjectsWindow(GetTileLayer(GetExcavationJob(project.job)->commissioning_anchor));
	}
	REQUIRE(window != nullptr);
	const std::string_view name = vehicle_menu ? "vehicle" : "project";
	CaptureHangarWindow(*window, fmt::format("{}-controls", name));
	REQUIRE_FALSE(window->IsWidgetDisabled(widget));
	const Rect button = window->GetWidget<NWidgetBase>(widget)->GetCurrentRect();
	ClickHangarPoint(*window, {(button.left + button.right) / 2, (button.top + button.bottom) / 2});
	ElapseHangarInputTimers(); // Leave initial dropdown drag mode at the released anchor.
	Window *menu = FindWindowById(WindowClass::DropdownMenu, 0);
	REQUIRE(menu != nullptr);
	CaptureHangarWindow(*menu, fmt::format("{}-choices", name));

	// Locate the known destination among real facilities. Native row heights
	// and hit testing still decide which visible command actually executes.
	std::set<TileIndex> candidates;
	for (const Depot *depot : Depot::Iterate()) if (GetTileOwner(depot->xy) == front->owner) candidates.insert(depot->xy);
	for (const auto &portal : CaptureLayerPortalWorkRecords()) if (portal.company == front->owner) candidates.insert(portal.pair_key);
	const uint row_height = MakeDropDownListStringItem(STR_TBM_HANGAR_PARK, 0)->Height();
	uint offset = 2 * row_height + MakeDropDownListDividerItem()->Height(); // Existing return/finish actions precede facilities.
	bool found = false;
	for (TileIndex tile : candidates) {
		const auto hangar = DescribeTBMHangar(*front, tile);
		if (hangar.error != TBMHangarError::None && hangar.error != TBMHangarError::Occupied && hangar.error != TBMHangarError::UnfinishedPortal) continue;
		if (hangar.tile == target) {
			REQUIRE(hangar.error == TBMHangarError::None);
			found = true;
			break;
		}
		offset += row_height;
	}
	REQUIRE(found);
	const Rect list = menu->GetWidget<NWidgetBase>(WID_DM_ITEMS)->GetCurrentRect().Shrink(WidgetDimensions::scaled.dropdownlist);
	REQUIRE(offset + row_height <= static_cast<uint>(list.Height()));
	ClickHangarPoint(*menu, {(list.left + list.right) / 2, list.top + static_cast<int>(offset + row_height / 2)});
	ElapseHangarInputTimers(8); // Native delayed selection invokes the real parent action.
	CHECK(FindWindowById(WindowClass::DropdownMenu, 0) == nullptr);
	REQUIRE(front->current_order.IsType(OT_GOTO_DEPOT));
	CHECK(GetTBMOrderedHangar(*front).tile == target);
	fmt::print("HANGAR_INPUT menu={} cutter={} target={} depot={}\n", name, front->index.base(), target.base(), GetTBMOrderedHangar(*front).depot.base());
}

/** @param front Actual cutter. Select the native Follow order control, including a retained one-order list. */
static void FollowNativeOrdersThroughInput(const Train &front)
{
	ShowOrdersWindow(&front);
	Window *window = FindWindowById(WindowClass::VehicleOrders, front.index);
	REQUIRE(window != nullptr);
	CaptureHangarWindow(*window, "native-orders");
	REQUIRE_FALSE(window->IsWidgetDisabled(WID_O_SKIP));
	const Rect button = window->GetWidget<NWidgetBase>(WID_O_SKIP)->GetCurrentRect();
	ClickHangarPoint(*window, {(button.left + button.right) / 2, (button.top + button.bottom) / 2});
	ElapseHangarInputTimers();
	CHECK(GetTBMProjectForCutter(front.index)->phase == TBMPhase::FollowOrders);
}

/** @param project Actual retained work. Click the native project Resume control after ordinary travel. */
static void ResumeNativeWorkThroughInput(const TBMProject &project)
{
	Window *window = ShowTBMProjectsWindow(GetTileLayer(GetExcavationJob(project.job)->commissioning_anchor));
	REQUIRE(window != nullptr);
	CaptureHangarWindow(*window, "orders-resume");
	REQUIRE_FALSE(window->IsWidgetDisabled(WID_TP_RESUME));
	const Rect button = window->GetWidget<NWidgetBase>(WID_TP_RESUME)->GetCurrentRect();
	ClickHangarPoint(*window, {(button.left + button.right) / 2, (button.top + button.bottom) / 2});
	ElapseHangarInputTimers();
	CHECK(GetTBMProject(project.id)->phase == TBMPhase::ReturnForExchange);
	CHECK(Train::Get(project.cutter)->current_order.IsType(OT_GOTO_DEPOT));
}

#endif /* TESTS_TBM_HANGAR_INPUT_H */
