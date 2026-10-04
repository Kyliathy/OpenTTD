/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file map_layer_gui.h Names, icons and view-layer switching. @see docs/zz-reach2/upgrades/2026-10/r2mlm1-layer-toolbar-button.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#ifndef MAP_LAYER_GUI_H
#define MAP_LAYER_GUI_H

#include "map_layer_type.h"
#include "direction_type.h"
#include "gfx_type.h"
#include "strings_type.h"
#include "dropdown_type.h"

struct Window;
struct Vehicle;
enum class LayerPortalKind : uint8_t;

/**
 * Player-facing name of one map layer.
 * @param layer Layer to name.
 * @return String id for that layer.
 */
StringID GetMapLayerName(MapLayer layer);

/**
 * Toolbar icon for one map layer.
 * @param layer Layer whose icon is needed.
 * @return Generated toolbar icon.
 */
SpriteID GetMapLayerIcon(MapLayer layer);

/**
 * Draw generated portal art in an orientation picker for the main view's Layer.
 * @param x Tile north-corner X in the current GUI drawing region.
 * @param y Tile north-corner Y in the current GUI drawing region.
 * @param kind Kind of portal being placed.
 * @param door Door direction selected by the preview button.
 */
void DrawLayerPortalPreview(int x, int y, LayerPortalKind kind, DiagDirection door);

/**
 * Layer that follows this one in GUI display order, skipping unallocated Layers.
 * @param layer Layer currently shown.
 * @param layer_count How many layers the cycle includes.
 * @return The next layer, wrapping to the surface after the last one.
 */
MapLayer GetNextMapLayer(MapLayer layer, uint layer_count);

/**
 * Build the shared Layer selector in GUI display order.
 * @return One icon/name entry for each allocated Layer, with its stable MapLayer value.
 */
DropDownList BuildMapLayerDropDown();

/**
 * Layer that owns a stacked world Y.
 * @param y World Y in pixels, measured from the north of the surface.
 * @return The allocated layer that contains y, clamped to the first or last layer.
 */
MapLayer GetLayerOfWorldY(int y);

/**
 * Layer whose views draw a vehicle and accept clicks on it.
 * FD-8: aircraft, ships and disasters belong to the surface. Effects use their world Y. Every other vehicle uses its tile.
 * @param v Vehicle to place.
 * @return The layer that shows the vehicle.
 */
MapLayer GetVehicleDisplayLayer(const Vehicle *v);

/**
 * Make one viewport show a map layer.
 * The same plane position stays on screen: the scroll moves by that layer's band.
 * @param w Window whose viewport changes.
 * @param layer Layer that viewport should show.
 */
void SetViewportLayer(Window *w, MapLayer layer);

/** Cycle the main viewport to the next map layer. */
void CycleMainViewportLayer();

#endif /* MAP_LAYER_GUI_H */
