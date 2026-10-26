/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file map_layer_gui.h Names, icons, layer switching and shared portal UI choices. @see docs/zz-reach2/upgrades/2026-10/r2mlm1-layer-toolbar-button.md, r2mlvs7-underground-construction-and-fleet.md and docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group3.
 */

#ifndef MAP_LAYER_GUI_H
#define MAP_LAYER_GUI_H

#include "map_layer_type.h"
#include "direction_type.h"
#include "gfx_type.h"
#include "strings_type.h"
#include "dropdown_type.h"
#include "station_type.h"
#include "company_type.h"

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
 * @param target Explicit selected target layer, or Invalid for the existing default choice.
 * @param preview_tile Actual existing endpoint for a saved-state preview; Invalid shows a fresh Sealed placement.
 */
void DrawLayerPortalPreview(int x, int y, LayerPortalKind kind, DiagDirection door, MapLayer target = MapLayer::Invalid, TileIndex preview_tile = INVALID_TILE);

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

/**
 * Select one allocated main-view layer through the shared viewport owner.
 * @param layer Explicit layer identity, independent of display-order ordinals.
 * Unallocated layers are ignored; other viewports and gameplay remain unchanged.
 */
void SelectMainViewportLayer(MapLayer layer);

/**
 * Open native portal status and validated connection controls for either housing.
 * @param tile Actual clicked endpoint; window identity derives from its canonical reciprocal pair.
 */
void ShowLayerPortalWindow(TileIndex tile);

/**
 * Choose the canonical available cutter for one actual rail pair's two layers.
 * @param tile Either real endpoint; pair identity and calendar/model capabilities are resolved by their owners.
 * @return Profile-owned model identity, or UINT16_MAX when no eligible model exists.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, area tools
 */
uint16_t GetLayerPortalDefaultTBMModel(TileIndex tile);

/**
 * Choose an owned Surface origin for the shared portal/area controls.
 * @param tile Actual pair endpoint; distance is measured using plane geometry.
 * @param company Actual requesting company, never selected from the camera.
 * @return Nearest owned railway station accepting both spoil cargos, otherwise nearest owned railway station or Invalid.
 */
StationID GetLayerPortalDefaultTBMOrigin(TileIndex tile, CompanyID company);

#endif /* MAP_LAYER_GUI_H */
