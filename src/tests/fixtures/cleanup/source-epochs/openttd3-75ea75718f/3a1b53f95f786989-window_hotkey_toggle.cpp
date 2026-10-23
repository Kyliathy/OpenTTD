/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file window_hotkey_toggle.cpp A toolbar hotkey closes its own front-most window, and Escape cancels placement before it closes windows. @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 3 (Q2, D10), docs/zz-reach2/architecture/gui/archi-gui.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_view_fixture.h"
#include "excavation_fixture.h"
#include "../hotkeys.h"
#include "../multilayer_content.h"
#include "../settings_type.h"
#include "../signs_func.h"
#include "../smallmap_gui.h"
#include "../toolbar_gui.h"
#include "../underground_gui.h"
#include "../widgets/underground_widget.h"
#include "../tilehighlight_func.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "../safeguards.h"

/**
 * The real main window's Escape binding ("reset_object_to_place"). The real main window needs the base graphics'
 * recolour sprites, so the tests put the same binding on the fixture's viewport instead. Focusing an edit box and
 * the game options both need a video driver, so the native "windows" scenario of demo_feedback.py checks F2 and
 * edit box precedence on the real main window instead.
 */
static HotkeyList _escape_test_hotkeys{"window_hotkey_toggle_test", {
	Hotkey(WKC_ESC, "reset_object_to_place", 0),
}};

/** Main view identity with the Escape binding; like the real main window it can never be closed. */
static WindowDesc _escape_main_view_desc(WindowPosition::Manual, {}, 640, 360, WindowClass::MainWindow, WindowClass::None,
	WindowDefaultFlag::NoClose, _underground_main_view_widgets, &_escape_test_hotkeys);

/** Main view that answers Escape exactly as `MainWindow::OnHotkey` answers `GHK_RESET_OBJECT_TO_PLACE`. */
class EscapeTestMainView : public UndergroundTestViewport {
public:
	EscapeTestMainView() : UndergroundTestViewport(_escape_main_view_desc) {}

	/**
	 * Handle the only bound key, Escape, through the production decision.
	 * @param hotkey Index in the test list; always the Escape entry.
	 * @return Always handled, as the real main window returns for this key.
	 */
	EventState OnHotkey([[maybe_unused]] int hotkey) override
	{
		CancelPlacementOrCloseWindow();
		return EventState::Handled;
	}
};

/** Shared world, windows and settings for the window-toggle tests; closes every test window before the map goes. */
class WindowToggleTest {
	ExcavationFixture fixture; ///< Company and multi-layer map the windows read.
	UndergroundToolbarTestUI ui{_current_company}; ///< Screen, local company and toolbar/viewport cleanup.
	AutoRestoreBackup<bool> toggles{_settings_client.gui.hotkey_toggles_window, true}; ///< The D10 defaults, set explicitly.
	AutoRestoreBackup<EscapeCloseWindow> escape{_settings_client.gui.escape_closes_window, EscapeCloseWindow::YesExceptSticky}; ///< See #toggles.

public:
	WindowToggleTest()
	{
		new EscapeTestMainView();
		AllocateToolbar();
		// The small map's legends are built once at game start; its hotkey assumes they exist.
		BuildIndustriesLegend();
		BuildLinkStatsLegend();
		BuildOwnerLegend();
	}

	/** Close the windows each test opened (pinned ones too) while the company and map still exist. */
	~WindowToggleTest()
	{
		CloseAllNonVitalWindows();
		Window::DeleteClosedWindows();
	}
};

/**
 * Press a key through the real dispatcher: a focused edit box, then windows front to back (the main view's Escape
 * binding among them), then the main toolbar's hotkeys.
 * @param keycode Key with modifiers, as the video driver reports it.
 */
static void PressKey(uint keycode)
{
	HandleKeypress(keycode, 0);
	Window::DeleteClosedWindows();
}

/**
 * D10: the second press of a window's toolbar key closes that window when it is in front. F2 and the game options
 * need a video driver (refresh rates), so the native "windows" scenario of demo_feedback.py presses F2 instead.
 */
TEST_CASE("A toolbar hotkey closes its own front-most window", "[demo-fix-window-toggle]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	WindowToggleTest test;

	// Every key bound to the same window toggles it: M opens the small map and F4 closes it.
	PressKey('M');
	REQUIRE(FindWindowById(WindowClass::SmallMap, 0) != nullptr);
	CHECK(FindFrontmostClosableWindow(false) == FindWindowById(WindowClass::SmallMap, 0));
	PressKey(WKC_F4);
	CHECK(FindWindowById(WindowClass::SmallMap, 0) == nullptr);

	// With the setting off the key only opens or raises, as before.
	_settings_client.gui.hotkey_toggles_window = false;
	PressKey('M');
	PressKey('M');
	CHECK(FindWindowById(WindowClass::SmallMap, 0) != nullptr);
}

/** D10: a key whose window is open but covered raises it; only the front-most window is closed. */
TEST_CASE("A toolbar hotkey raises a covered window before closing it", "[demo-fix-window-toggle]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	WindowToggleTest test;

	PressKey('M');
	Window *map = FindWindowById(WindowClass::SmallMap, 0);
	REQUIRE(map != nullptr);
	Window *signs = ShowSignList();
	REQUIRE(signs != nullptr);
	REQUIRE(FindFrontmostClosableWindow(false) == signs);
	CHECK_FALSE(IsWindowRaisedAsFarAsPossible(map));

	// The covered small map comes to the front and stays open.
	PressKey('M');
	CHECK(FindWindowById(WindowClass::SmallMap, 0) == map);
	CHECK(FindFrontmostClosableWindow(false) == map);
	CHECK(IsWindowRaisedAsFarAsPossible(map));
	// The sign list is now the covered one; the main toolbar in front of both never counts, as it always stays above.
	CHECK_FALSE(IsWindowRaisedAsFarAsPossible(signs));

	// Now it is in front, so the same key closes it and leaves the sign list alone.
	PressKey('M');
	CHECK(FindWindowById(WindowClass::SmallMap, 0) == nullptr);
	CHECK(FindWindowById(WindowClass::SignList, 0) == signs);
}

/** D10: Escape first aborts an active tool and leaves its palette open; the next Escape closes the palette. */
TEST_CASE("Escape aborts placement before it closes a window", "[demo-fix-window-toggle]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	WindowToggleTest test;

	Window *palette = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(palette != nullptr);
	palette->OnClick({}, WID_UC_SURVEY, 1);
	REQUIRE(_thd.GetCallbackWnd() == palette);
	REQUIRE(IsObjectPlacementActive());

	// The survey tool is waiting for a map click: Escape only cancels it.
	PressKey(WKC_ESC);
	CHECK_FALSE(IsObjectPlacementActive());
	CHECK(FindWindowById(WindowClass::UndergroundConstruction, MapLayer::Underground) == palette);

	// Without a tool the same key closes the front-most window.
	PressKey(WKC_ESC);
	CHECK(FindWindowById(WindowClass::UndergroundConstruction, MapLayer::Underground) == nullptr);

	// "No" restores the old key: it cancels placement and never closes a window.
	_settings_client.gui.escape_closes_window = EscapeCloseWindow::No;
	Window *signs = ShowSignList();
	REQUIRE(signs != nullptr);
	PressKey(WKC_ESC);
	CHECK(FindWindowById(WindowClass::SignList, 0) == signs);
}

/** D10: pinned windows survive Escape under "Yes except pinned" and close under "Yes". */
TEST_CASE("Escape closes pinned windows only when the setting allows it", "[demo-fix-window-toggle]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	WindowToggleTest test;

	Window *signs = ShowSignList();
	REQUIRE(signs != nullptr);
	PressKey('M');
	Window *map = FindWindowById(WindowClass::SmallMap, 0);
	REQUIRE(map != nullptr);
	map->flags.Set(WindowFlag::Sticky);

	// The pinned small map is in front, so Escape closes the unpinned sign list behind it.
	PressKey(WKC_ESC);
	CHECK(FindWindowById(WindowClass::SignList, 0) == nullptr);
	CHECK(FindWindowById(WindowClass::SmallMap, 0) == map);
	// Only pinned windows are left: Escape closes nothing.
	PressKey(WKC_ESC);
	CHECK(FindWindowById(WindowClass::SmallMap, 0) == map);

	_settings_client.gui.escape_closes_window = EscapeCloseWindow::Yes;
	PressKey(WKC_ESC);
	CHECK(FindWindowById(WindowClass::SmallMap, 0) == nullptr);
}
