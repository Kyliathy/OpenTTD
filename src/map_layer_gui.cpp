/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file map_layer_gui.cpp Names, icons and view-layer switching. @see docs/zz-reach2/upgrades/2026-10/r2mlm1-layer-toolbar-button.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#include "stdafx.h"
#include "map_layer_gui.h"
#include "map_layer.h"
#include "window_func.h"
#include "window_gui.h"
#include "window_type.h"
#include "landscape.h"
#include "vehicle_base.h"
#include "linkgraph/linkgraph_gui.h"

#include "table/sprites.h"
#include "table/strings.h"

#include "safeguards.h"

/**
 * Player-facing name of one map layer.
 * @param layer Layer to name.
 * @return String id for that layer.
 */
StringID GetMapLayerName(MapLayer layer)
{
	switch (layer) {
		case MapLayer::Surface: return STR_MAP_LAYER_SURFACE;
		case MapLayer::Underground: return STR_MAP_LAYER_UNDERGROUND;
		case MapLayer::Space: return STR_MAP_LAYER_SPACE;
		default: NOT_REACHED();
	}
}

/**
 * Toolbar icon for one map layer.
 * @param layer Layer whose icon is needed.
 * @return Stand-in sprite until the layer art exists.
 */
SpriteID GetMapLayerIcon(MapLayer layer)
{
	switch (layer) {
		case MapLayer::Surface: return SPR_IMG_PLANTTREES;
		case MapLayer::Underground: return SPR_IMG_TUNNEL_RAIL;
		case MapLayer::Space: return SPR_IMG_AIRPLANESLIST;
		default: NOT_REACHED();
	}
}

/**
 * Layer that follows this one in the cycle.
 * @param layer Layer currently shown.
 * @param layer_count How many layers the cycle includes.
 * @return The next layer, wrapping to the surface after the last one.
 */
MapLayer GetNextMapLayer(MapLayer layer, uint layer_count)
{
	//One layer has nowhere to go.
	if (layer_count <= 1) return MapLayer::Surface;

	uint next = to_underlying(layer) + 1;
	//Past the last live layer, the cycle returns to the surface.
	if (next >= layer_count) return MapLayer::Surface;
	return static_cast<MapLayer>(next);
}

/**
 * Layer that owns a stacked world Y.
 * @param y World Y in pixels, measured from the north of the surface.
 * @return The allocated layer that contains y, clamped to the first or last layer.
 */
MapLayer GetLayerOfWorldY(int y)
{
	int tile_y = y / static_cast<int>(TILE_SIZE);
	//North of the surface belongs to the surface. South of the last layer belongs to that layer.
	if (tile_y < 0) return MapLayer::Surface;
	uint stacked = static_cast<uint>(tile_y);
	uint layer_index = stacked >> Map::LogY();
	if (layer_index >= Map::LayerCount()) layer_index = Map::LayerCount() - 1;
	return static_cast<MapLayer>(layer_index);
}

/**
 * Layer whose views draw a vehicle and accept clicks on it.
 * FD-8: aircraft, ships and disasters belong to the surface. Effects use their world Y. Every other vehicle uses its tile.
 * @param v Vehicle to place.
 * @return The layer that shows the vehicle.
 */
MapLayer GetVehicleDisplayLayer(const Vehicle *v)
{
	switch (v->type) {
		//L-6: flying and sailing vehicles stay on the surface, even when their position lies south of the surface band.
		case VehicleType::Aircraft:
		case VehicleType::Ship:
		case VehicleType::Disaster:
			return MapLayer::Surface;

		//An effect has no tile of its own that matters. Its world point decides.
		case VehicleType::Effect:
			return GetLayerOfWorldY(v->y_pos);

		default:
			//A vehicle that is not on the map yet has no tile, so its world point decides.
			if (v->tile == INVALID_TILE) return GetLayerOfWorldY(v->y_pos);
			return GetTileLayer(v->tile);
	}
}

/**
 * Make one viewport show a map layer.
 * The same plane position stays on screen: the scroll moves by that layer's band.
 * @param w Window whose viewport changes.
 * @param layer Layer that viewport should show.
 */
void SetViewportLayer(Window *w, MapLayer layer)
{
	if (w == nullptr || w->viewport == nullptr) return;

	//A viewport may only show a layer this game allocated. Release builds fall back to the surface.
	assert(MultiLayerMap::IsValidLayer(layer));
	if (!MultiLayerMap::IsValidLayer(layer)) layer = MapLayer::Surface;

	ViewportData *vp = w->viewport.get();
	MapLayer old = vp->layer;
	if (old != layer) {
		//One layer is Map::SizeY() tiles tall. RemapCoords turns that world shift into a view shift.
		int world_dy = (static_cast<int>(to_underlying(layer)) - static_cast<int>(to_underlying(old))) * static_cast<int>(Map::SizeY()) * static_cast<int>(TILE_SIZE);
		Point delta = RemapCoords(0, world_dy, 0);
		vp->scrollpos_x += delta.x;
		vp->scrollpos_y += delta.y;
		vp->dest_scrollpos_x += delta.x;
		vp->dest_scrollpos_y += delta.y;

		//A followed vehicle in another layer would drag the view back.
		if (vp->follow_vehicle != VehicleID::Invalid()) {
			const Vehicle *veh = Vehicle::Get(vp->follow_vehicle);
			if (GetVehicleDisplayLayer(veh) != layer) vp->CancelFollow(*w);
		}
		//FD-8: the link overlay cached the stations of the old layer.
		if (vp->overlay != nullptr) vp->overlay->SetDirty();
	}

	vp->layer = layer;
	w->SetDirty();
	//Both the game toolbar and the scenario editor toolbar use this class.
	InvalidateWindowData(WindowClass::MainToolbar, 0);
	//Build toolbars refresh their buttons. Since FD-7 the portal buttons depend on the layer count only.
	InvalidateWindowData(WindowClass::BuildToolbar, 0);
	InvalidateWindowData(WindowClass::ScenarioBuildToolbar, 0);
}

/** Cycle the main viewport to the next map layer. */
void CycleMainViewportLayer()
{
	Window *main = GetMainWindow();
	//The cycle length is the number of layers this game allocated (AD-7).
	MapLayer current = main->viewport->layer;
	SetViewportLayer(main, GetNextMapLayer(current, MultiLayerMap::GetLayerCount()));
}
