/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file map_layer.cpp The multi-layered map controller. */

#include "stdafx.h"
#include "map_layer.h"
#include "map_layer_gui.h"
#include "error_func.h"
#include "clear_map.h"
#include "void_map.h"
#include "bridge_map.h"
#include "company_func.h"
#include "core/random_func.hpp"
#include "strings_func.h"
#include "table/strings.h"

#include "safeguards.h"

static_assert(to_underlying(MapLayerConstruction::End) <= 32);

/**
 * Every construction kind. Surface uses this.
 * @return A bitset with every real kind set.
 */
static MapLayerConstructions AllConstructions()
{
	MapLayerConstructions allowed;
	allowed.Set();
	return allowed;
}

/**
 * What Underground accepts in version 1: rail and road, not tram, plus the buildings that go with them, signs and portals.
 * @return That bitset.
 */
static MapLayerConstructions UndergroundConstructions()
{
	MapLayerConstructions allowed;
	allowed.Set(MapLayerConstruction::Rail);
	allowed.Set(MapLayerConstruction::Road);
	allowed.Set(MapLayerConstruction::RailStation);
	allowed.Set(MapLayerConstruction::RoadStop);
	allowed.Set(MapLayerConstruction::Depot);
	allowed.Set(MapLayerConstruction::Signal);
	allowed.Set(MapLayerConstruction::Sign);
	allowed.Set(MapLayerConstruction::LayerPortal);
	return allowed;
}

/**
 * What Space accepts in version 1: signs only.
 * @return That bitset.
 */
static MapLayerConstructions SpaceConstructions()
{
	MapLayerConstructions allowed;
	allowed.Set(MapLayerConstruction::Sign);
	return allowed;
}

/**
 * One row per layer, in MapLayer order.
 * Surface is the historical map. Underground is flat bedrock. Space is flat vacuum.
 */
static const MapLayerRules _map_layer_rules[] = {
	{ AllConstructions(), MapLayerGround::Natural, 0, true, true, true },
	{ UndergroundConstructions(), MapLayerGround::Bedrock, 0, false, false, false },
	{ SpaceConstructions(), MapLayerGround::Vacuum, 0, false, false, false },
};

/**
 * Stop the game when a map asks for a layer count this build cannot store.
 * @param layer_count How many layers the caller wants to allocate.
 */
/* static */ void MultiLayerMap::ValidateLayerCount(uint layer_count)
{
	//Live games allocate every layer. Tests may ask for one.
	if (layer_count < 1 || layer_count > MAX_MAP_LAYERS) {
		FatalError("Invalid map layer count");
	}
}

/**
 * Rules for one allocated layer.
 * @param layer Layer to describe.
 * @return That layer's rules.
 */
/* static */ const MapLayerRules &MultiLayerMap::GetRules(MapLayer layer)
{
	assert(MultiLayerMap::IsValidLayer(layer));
	return _map_layer_rules[to_underlying(layer)];
}

/**
 * Rules for the layer that stores a tile.
 * @param tile Tile whose layer is described.
 * @return That layer's rules.
 */
/* static */ const MapLayerRules &MultiLayerMap::GetRules(TileIndex tile)
{
	return MultiLayerMap::GetRules(GetTileLayer(tile));
}

/**
 * Fill the inner rectangle of one layer and void its fence.
 * @param layer Layer being filled.
 * @param ground Clear ground for the inner tiles.
 * @param height Height of the inner tiles.
 * @param void_north Also void the north edges. Surface leaves that to the generator.
 */
static void FillLayer(MapLayer layer, ClearGround ground, uint height, bool void_north)
{
	//The south fence is always void. The north fence is void when this layer asks for it.
	for (uint y = 0; y < Map::SizeY(); ++y) {
		for (uint x = 0; x < Map::SizeX(); ++x) {
			TileIndex tile = TileXYL(x, y, layer);
			bool south_fence = x == Map::MaxX() || y == Map::MaxY();
			bool north_fence = void_north && (x == 0 || y == 0);
			if (south_fence || north_fence) {
				MakeVoid(tile);
				continue;
			}
			MakeClear(tile, ground, 3);
			SetTileHeight(tile, height);
			SetTropicZone(tile, TropicZone::Normal);
			ClearBridgeMiddle(tile);
		}
	}
}

/**
 * Fill one layer with its default ground and void fence.
 * Surface keeps the historical grass fill. Other layers never call Random().
 * @param layer Layer to fill.
 */
/* static */ void MultiLayerMap::InitialiseLayer(MapLayer layer)
{
	assert(MultiLayerMap::IsValidLayer(layer));

	if (layer == MapLayer::Surface) {
		//The historical surface fill. North edges stay for the generator to void when freeform edges are on.
		for (uint y = _settings_game.construction.freeform_edges ? 1 : 0; y < Map::MaxY(); y++) {
			for (uint x = _settings_game.construction.freeform_edges ? 1 : 0; x < Map::MaxX(); x++) {
				TileIndex tile = TileXYL(x, y, MapLayer::Surface);
				MakeClear(tile, ClearGround::Grass, 3);
				SetTileHeight(tile, 0);
				SetTropicZone(tile, TropicZone::Normal);
				ClearBridgeMiddle(tile);
			}
		}
		for (uint x = 0; x < Map::SizeX(); x++) MakeVoid(TileXYL(x, Map::MaxY(), MapLayer::Surface));
		for (uint y = 0; y < Map::SizeY(); y++) MakeVoid(TileXYL(Map::MaxX(), y, MapLayer::Surface));
		return;
	}

	//Underground and Space must not consume random numbers. The surface seed stays bit-identical.
#ifdef _DEBUG
	uint32_t random_before_0 = _random.state[0];
	uint32_t random_before_1 = _random.state[1];
#endif

	const MapLayerRules &rules = MultiLayerMap::GetRules(layer);
	FillLayer(layer, ClearGround::Rocks, rules.base_height, _settings_game.construction.freeform_edges);

#ifdef _DEBUG
	assert(_random.state[0] == random_before_0);
	assert(_random.state[1] == random_before_1);
#endif
}

/**
 * Refuse a construction the tile's layer does not allow.
 * @param tile Tile the command wants to change.
 * @param kind Construction being attempted.
 * @return An empty cost when the layer allows it, otherwise the layer error.
 */
CommandCost CheckLayerAllowsConstruction(TileIndex tile, MapLayerConstruction kind)
{
	MapLayer layer = GetTileLayer(tile);
	//An unknown layer, or a kind this layer does not list, is refused before the command spends money.
	if (!MultiLayerMap::IsValidLayer(layer) || !MultiLayerMap::GetRules(layer).allowed.Test(kind)) {
		CommandCost error(STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER);
		//The name is only known for a real layer. The local player is the one who sees the message.
		if (MultiLayerMap::IsValidLayer(layer) && IsLocalCompany()) {
			error.SetEncodedMessage(GetEncodedString(STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER, GetMapLayerName(layer)));
		}
		return error;
	}
	return {};
}
