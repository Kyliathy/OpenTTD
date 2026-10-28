/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file window_func.h %Window functions not directly related to making/drawing windows.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md, docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 3
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group3.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 6; consolidated layer hotkeys, window toggle and settings.
 */

#ifndef WINDOW_FUNC_H
#define WINDOW_FUNC_H

#include "window_type.h"
#include "company_type.h"
#include "core/geometry_type.hpp"
#include "strings_type.h"

Window *FindWindowById(WindowClass cls, WindowNumber number);
Window *FindWindowByClass(WindowClass cls);
Window *GetMainWindow();
void ChangeWindowOwner(Owner old_owner, Owner new_owner);

void ResizeWindow(Window *w, int x, int y, bool clamp_to_screen = true, bool schedule_resize = true);
int PositionMainToolbar(Window *w);
int PositionStatusbar(Window *w);
int PositionNewsMessage(Window *w);
int PositionNetworkChatWindow(Window *w);

int GetMainViewTop();
int GetMainViewBottom();

void InitWindowSystem();
void UnInitWindowSystem();
void ResetWindowSystem();
void SetupColoursAndInitialWindow();
void InputLoop();

void InvalidateWindowData(WindowClass cls, WindowNumber number, int data = 0, bool gui_scope = false);
/** @copydoc InvalidateWindowData */
void InvalidateWindowData(WindowClass cls, WindowNumber number, ConvertibleThroughBase auto data, bool gui_scope = false) { InvalidateWindowData(cls, number, data.base(), gui_scope); }
/** @copydoc InvalidateWindowData */
template <typename T> requires is_scoped_enum_v<T>
void InvalidateWindowData(WindowClass cls, WindowNumber number, T data, bool gui_scope = false) { InvalidateWindowData(cls, number, to_underlying(data), gui_scope); }
void InvalidateWindowClassesData(WindowClass cls, int data = 0, bool gui_scope = false);
/** @copydoc InvalidateWindowClassesData */
void InvalidateWindowClassesData(WindowClass cls, ConvertibleThroughBase auto data, bool gui_scope = false) { InvalidateWindowClassesData(cls, data.base(), gui_scope); }
/** @copydoc InvalidateWindowClassesData */
template <typename T> requires is_scoped_enum_v<T>
void InvalidateWindowClassesData(WindowClass cls, T data, bool gui_scope = false) { InvalidateWindowClassesData(cls, to_underlying(data), gui_scope); }

void CloseNonVitalWindows();

void CloseAllNonVitalWindows();

/**
 * Find the window a player would close first: the front-most window that is not the main view, a vital window
 * (toolbar, status bar, news, chat), a window that cannot be closed or, if asked, a pinned window.
 * @param except_sticky Whether pinned windows are skipped.
 * @return That window, or nullptr when no window qualifies.
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md decision D10
 */
Window *FindFrontmostClosableWindow(bool except_sticky);

/**
 * Whether bringing a window to the front would change nothing: every window in front of it always stays above it
 * (a higher z-priority, as the game options, query windows, toolbars and tooltips have over a map or list window).
 * The D10 hotkey toggle closes its window in that case, so a key whose window is already as high as it can go
 * never does nothing.
 * @param w Open window.
 * @return Whether \a w is already raised as far as the window stack allows.
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md decision D10
 */
bool IsWindowRaisedAsFarAsPossible(const Window *w);

/**
 * Close the window #FindFrontmostClosableWindow names (Escape).
 * @param except_sticky Whether pinned windows are skipped.
 * @return Whether a window was closed.
 */
bool CloseFrontmostClosableWindow(bool except_sticky);

/**
 * The Escape hotkey (D10): cancel an active placement first, exactly as before; only without one, and only when
 * the "close window with Escape" setting allows it, close the front-most closable window. Focused edit boxes,
 * queries and other windows that handle Escape themselves receive the key before this hotkey runs.
 */
void CancelPlacementOrCloseWindow();
void DeleteAllMessages();
void CloseConstructionWindows();
void HideVitalWindows();
void ShowVitalWindows();

/**
 * Re-initialize all windows.
 * @param zoom_changed Set if windows are being re-initialized due to a zoom level changed.
 */
void ReInitAllWindows(bool zoom_changed);

void SetWindowWidgetDirty(WindowClass cls, WindowNumber number, WidgetID widget_index);
void SetWindowDirty(WindowClass cls, WindowNumber number);
void SetWindowClassesDirty(WindowClass cls);

void CloseWindowById(WindowClass cls, WindowNumber number, bool force = true, int data = 0);
void CloseWindowByClass(WindowClass cls, int data = 0);

bool EditBoxInGlobalFocus();
bool FocusedWindowIsConsole();
Point GetCaretPosition();

/**
 * Adding a window number to a string is a common occurence to get the caption for a vehicle type.
 * @param string The base string.
 * @param window_number The window number to add.
 * @return The resulting \c StringID.
 */
constexpr StringID operator+(StringID string, WindowNumber window_number) noexcept { return string + static_cast<int32_t>(window_number); }

#endif /* WINDOW_FUNC_H */
