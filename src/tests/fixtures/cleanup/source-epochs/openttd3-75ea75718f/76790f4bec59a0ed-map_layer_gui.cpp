/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file map_layer_gui.cpp Names, icons and view-layer switching. @see docs/zz-reach2/upgrades/2026-10/r2mlm1-layer-toolbar-button.md, r2mlvs7-underground-construction-and-fleet.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md and docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 2 (small map follows the main view, per-layer hotkeys) */

#include "stdafx.h"
#include "map_layer_gui.h"
#include "map_layer.h"
#include "multilayer_sprite.h"
#include "multilayer_content.h"
#include "window_func.h"
#include "window_gui.h"
#include "window_type.h"
#include "landscape.h"
#include "vehicle_base.h"
#include "linkgraph/linkgraph_gui.h"
#include "dropdown_func.h"
#include "gfx_func.h"
#include "tilehighlight_func.h"
#include "smallmap_gui.h"

#include "table/sprites.h"
#include "table/strings.h"

#include "safeguards.h"

/** Display order is separate from the immutable storage order (r2mlm12 D-1). */
static constexpr MapLayer _map_layer_display_order[] = {MapLayer::Surface, MapLayer::Underground, MapLayer::DeepUnderground, MapLayer::Space};
static_assert(std::size(_map_layer_display_order) == MAX_MAP_LAYERS);

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
		case MapLayer::DeepUnderground: return STR_MAP_LAYER_DEEP_UNDERGROUND;
		default: NOT_REACHED();
	}
}

/**
 * Toolbar icon for one map layer.
 * @param layer Layer whose icon is needed.
 * @return Sprite from the Map Layer graphics block in OpenTTD's extra GRF.
 */
SpriteID GetMapLayerIcon(MapLayer layer)
{
	switch (layer) {
		case MapLayer::Surface: return SPR_IMG_MAP_LAYER_SURFACE;
		case MapLayer::Underground: return SPR_IMG_MAP_LAYER_UNDERGROUND;
		case MapLayer::Space: return SPR_IMG_MAP_LAYER_SPACE;
		case MapLayer::DeepUnderground: return SPR_IMG_MAP_LAYER_DEEP_UNDERGROUND;
		default: NOT_REACHED();
	}
}

/**
 * Draw generated portal art in an orientation picker for the main view's Layer.
 * Placement still derives its Layer from the clicked Tile, including clicks in extra viewports (FD-7).
 * @param x Tile north-corner X in the current GUI drawing region.
 * @param y Tile north-corner Y in the current GUI drawing region.
 * @param kind Kind of portal being placed.
 * @param door Door direction selected by the preview button.
 * @param target Explicit selected target layer, or Invalid for the existing default choice.
 */
void DrawLayerPortalPreview(int x, int y, LayerPortalKind kind, DiagDirection door, MapLayer target, TileIndex preview_tile)
{
	const MapLayer layer = IsValidTile(preview_tile) ? GetTileLayer(preview_tile) : GetMainWindow()->viewport->layer;
	bool surface = layer == MapLayer::Surface;
	SpriteID base;
	switch (kind) {
		case LayerPortalKind::RailTunnel: base = surface ? SPR_MAP_LAYER_RAIL_PORTAL_SURFACE : SPR_MAP_LAYER_RAIL_PORTAL_UNDERGROUND; break;
		case LayerPortalKind::RoadTunnel: base = surface ? SPR_MAP_LAYER_ROAD_TUNNEL_SURFACE : SPR_MAP_LAYER_ROAD_TUNNEL_UNDERGROUND; break;
		case LayerPortalKind::RoadElevator: base = surface ? SPR_MAP_LAYER_ROAD_ELEVATOR_SURFACE : SPR_MAP_LAYER_ROAD_ELEVATOR_UNDERGROUND; break;
		default: NOT_REACHED();
	}
	SpriteID back = base + to_underlying(door) * 2;
	SpriteID front = back + 1;
	// A native rail preview shows the same Sealed source housing it would build.
	// The picker still owns availability/date checks and the destination choice.
	if (kind == LayerPortalKind::RailTunnel && GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry) {
		if (const auto existing = GetMultiLayerPortalSprite(preview_tile, false)) {
			// Existing endpoints display their actual Sealed/Commissioning/Connected
			// owner state; they do not reuse the new-build picker's default phase.
			DrawSprite(*existing, PAL_NONE, x, y);
			DrawSprite(*GetMultiLayerPortalSprite(preview_tile, true), PAL_NONE, x, y);
			return;
		}
		// A Deep housing stays distinct on either endpoint, including an existing
		// lower-end picker. The chosen destination never supplies excavation rights.
		const bool deep = layer == MapLayer::DeepUnderground || target == MapLayer::DeepUnderground;
		const auto family = deep ? MultiLayerSpriteAsset::FacilityPortalDeepRail : MultiLayerSpriteAsset::FacilityPortalSealed;
		// Surface and Underground-to-Deep are upper endpoints; their reciprocal
		// Underground and Deep views use the independently authored lower housing.
		const uint16_t endpoint = layer == MapLayer::Surface || (layer == MapLayer::Underground && deep) ? 0 : 1;
		back = *GetMultiLayerSprite(family, {0, 0, static_cast<uint8_t>(door), endpoint, 0});
		front = *GetMultiLayerSprite(family, {0, 0, static_cast<uint8_t>(door), endpoint, 1});
	}
	DrawSprite(back, PAL_NONE, x, y);
	DrawSprite(front, PAL_NONE, x, y);
}

/**
 * Layer that follows this one in GUI display order, skipping unallocated Layers.
 * @param layer Layer currently shown.
 * @param layer_count How many layers the cycle includes.
 * @return The next layer, wrapping to the surface after the last one.
 */
MapLayer GetNextMapLayer(MapLayer layer, uint layer_count)
{
	//One layer has nowhere to go.
	if (layer_count <= 1) return MapLayer::Surface;

	auto current = std::find(std::begin(_map_layer_display_order), std::end(_map_layer_display_order), layer);
	size_t start = current == std::end(_map_layer_display_order) ? std::size(_map_layer_display_order) - 1
		: static_cast<size_t>(std::distance(std::begin(_map_layer_display_order), current));
	//One walk through display order wraps to Surface while skipping Layers this Map has not allocated.
	for (size_t step = 1; step <= std::size(_map_layer_display_order); ++step) {
		MapLayer next = _map_layer_display_order[(start + step) % std::size(_map_layer_display_order)];
		if (to_underlying(next) < layer_count) return next;
	}
	return MapLayer::Surface;
}

/**
 * Build the shared Layer selector in GUI display order.
 * @return One icon/name entry per allocated Layer, with its stable MapLayer value.
 */
DropDownList BuildMapLayerDropDown()
{
	DropDownList list;
	//Storage values identify entries; the GUI order places Deep Underground before Outer Space.
	for (MapLayer layer : _map_layer_display_order) {
		//A partially allocated Map must not offer a Layer its views cannot show.
		if (!MultiLayerMap::IsValidLayer(layer)) continue;
		list.push_back(MakeDropDownListIconItem(GetMapLayerIcon(layer), PAL_NONE, GetMapLayerName(layer), layer));
	}
	return list;
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
	// A missing viewport has no displayed layer or active selection to remap.
	if (w == nullptr || w->viewport == nullptr) return;

	//A viewport may only show a layer this game allocated. Release builds fall back to the surface.
	assert(MultiLayerMap::IsValidLayer(layer));
	if (!MultiLayerMap::IsValidLayer(layer)) layer = MapLayer::Surface;

	ViewportData *vp = w->viewport.get();
	MapLayer old = vp->layer;
	if (old != layer) {
		// Cancel the visible cursor/drag before its world-coordinate band changes, including extra viewports.
		// Already posted commands retain their captured world tiles; this only ends the unfinished GUI selection.
		ResetObjectToPlace();
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
	// r2df D1: the small map follows the main view's layer, whichever input changed it; extra viewports never move it.
	// The main window is looked up without GetMainWindow's assertion: a viewport may change layer while no main window
	// exists, as a test's own viewport does (r2mlvs3mp master finalization, 2026-10-16).
	if (old != layer && w == FindWindowById(WindowClass::MainWindow, 0)) InvalidateWindowClassesData(WindowClass::SmallMap, SMALLMAP_INVALIDATE_MAIN_VIEW_LAYER);
	//Both the game toolbar and the scenario editor toolbar use this class.
	InvalidateWindowData(WindowClass::MainToolbar, 0);
	//Build toolbars refresh their buttons. Since FD-7 the portal buttons depend on the layer count only.
	InvalidateWindowData(WindowClass::BuildToolbar, 0);
	InvalidateWindowData(WindowClass::ScenarioBuildToolbar, 0);
}

/** @copydoc ShowMainViewportLayerInDisplayOrder */
bool ShowMainViewportLayerInDisplayOrder(uint display_index)
{
	// A key for a layer this game never allocated does nothing, as the layer dropdown never offers it.
	if (display_index >= std::size(_map_layer_display_order)) return false;
	const MapLayer layer = _map_layer_display_order[display_index];
	if (!MultiLayerMap::IsValidLayer(layer)) return false;
	SetViewportLayer(GetMainWindow(), layer);
	return true;
}

/** Cycle the main viewport to the next map layer. */
void CycleMainViewportLayer()
{
	Window *main = GetMainWindow();
	//The cycle length is the number of layers this game allocated (AD-7).
	MapLayer current = main->viewport->layer;
	SetViewportLayer(main, GetNextMapLayer(current, MultiLayerMap::GetLayerCount()));
}
