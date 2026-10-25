/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tilehighlight_func.h Functions related to tile highlights. @see docs/zz-reach2/architecture/gui/archi-viewport.md, docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 3 (Escape cancels placement first) */

#ifndef TILEHIGHLIGHT_FUNC_H
#define TILEHIGHLIGHT_FUNC_H

#include "gfx_type.h"
#include "tilehighlight_type.h"

void PlaceProc_DemolishArea(TileIndex tile);
bool GUIPlaceProcDragXY(ViewportDragDropSelectionProcess proc, TileIndex start_tile, TileIndex end_tile);

bool HandlePlacePushButton(Window *w, WidgetID widget, CursorID cursor, HighLightStyle mode);
void SetObjectToPlaceWnd(CursorID icon, PaletteID pal, HighLightStyle mode, Window *w);
void SetObjectToPlace(CursorID icon, PaletteID pal, HighLightStyle mode, WindowClass window_class, WindowNumber window_num);
void ResetObjectToPlace();

/**
 * Whether the player is in the middle of placing something: a tool with a highlight, a drag-and-drop, or a window
 * that waits for a map click. Escape cancels such a placement before it may close a window (r2df decision D10).
 * @return True while ResetObjectToPlace would cancel something.
 */
bool IsObjectPlacementActive();

void VpSelectTilesWithMethod(int x, int y, ViewportPlaceMethod method);
void VpStartDragging(ViewportDragDropSelectionProcess process);
void VpStartPlaceSizing(TileIndex tile, ViewportPlaceMethod method, ViewportDragDropSelectionProcess process);
void VpSetPresizeRange(TileIndex from, TileIndex to);
void VpSetPlaceSizingLimit(int limit);

void UpdateTileSelection();

extern TileHighlightData _thd;

#endif /* TILEHIGHLIGHT_FUNC_H */
