/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file road_gui.h Functions/types related to the road GUIs. @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 3 (hotkey toggle) */

#ifndef ROAD_GUI_H
#define ROAD_GUI_H

#include "road_type.h"
#include "tile_type.h"
#include "direction_type.h"
#include "dropdown_type.h"

struct Window *ShowBuildRoadToolbar(RoadType roadtype);

/**
 * Road or tram type the build-road toolbar shows, so a toolbar hotkey can tell switching the type from closing it.
 * @return The current toolbar's road type (the last one opened).
 */
RoadType GetBuildRoadToolbarRoadType();
struct Window *ShowBuildRoadScenToolbar(RoadType roadtype);
void ConnectRoadToStructure(TileIndex tile, DiagDirection direction);
DropDownList GetRoadTypeDropDownList(RoadTramTypes rtts, bool for_replacement = false, bool all_option = false);
DropDownList GetScenRoadTypeDropDownList(RoadTramTypes rtts);
void SetDefaultRoadGui();

#endif /* ROAD_GUI_H */
