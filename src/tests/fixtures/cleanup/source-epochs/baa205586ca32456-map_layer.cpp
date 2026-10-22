/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file map_layer.cpp The multi-layered map controller. @see docs/zz-reach2/upgrades/2026-10/r2mlm2-layer-addressing-and-controller.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md, docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md */

#include "stdafx.h"
#include "map_layer.h"
#include "excavation.h"
#include "multilayer_content.h"
#include "map_layer_gui.h"
#include "error_func.h"
#include "clear_map.h"
#include "void_map.h"
#include "bridge_map.h"
#include "company_func.h"
#include "command_func.h"
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
	allowed.Set(MapLayerConstruction::TrainDepot);
	allowed.Set(MapLayerConstruction::RoadDepot);
	allowed.Set(MapLayerConstruction::Signal);
	allowed.Set(MapLayerConstruction::Sign);
	allowed.Set(MapLayerConstruction::LayerPortal);
	return allowed;
}

/**
 * D-2: Deep Underground accepts roads, road stops, road depots, signs and portals, with no rail.
 * @return The road-only construction bitset.
 */
static MapLayerConstructions DeepUndergroundConstructions()
{
	MapLayerConstructions allowed;
	allowed.Set(MapLayerConstruction::Road);
	allowed.Set(MapLayerConstruction::RoadStop);
	allowed.Set(MapLayerConstruction::RoadDepot);
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
 * Surface is the historical Map, Underground is bedrock, Space is vacuum, and Deep Underground is dark bedrock.
 */
static const MapLayerRules _map_layer_rules[] = {
	{ AllConstructions(), MapLayerGround::Natural, 0, true, true, true },
	{ UndergroundConstructions(), MapLayerGround::Bedrock, 0, false, false, false },
	{ SpaceConstructions(), MapLayerGround::Vacuum, 0, false, false, false },
	{ DeepUndergroundConstructions(), MapLayerGround::DeepBedrock, 0, false, false, false },
};
static_assert(std::size(_map_layer_rules) == MAX_MAP_LAYERS);

/**
 * Native Deep infrastructure consumes the existing Underground rail/road set with the Deep ground palette.
 * Date and excavation remain operation checks, independently of this profile's structural permissions.
 * @return Immutable new-profile rules; Classic retains its historical road-only row.
 */
static const MapLayerRules &GetNativeDeepRules()
{
	static const MapLayerRules rules{UndergroundConstructions(), MapLayerGround::DeepBedrock, 0, false, false, false};
	return rules;
}

/**
 * Whether a portal kind may join two Layers in either order.
 * @param kind Kind of portal.
 * @param a Layer of one end.
 * @param b Layer of the other end.
 * @return True for Surface/Underground tunnels or an upper-Layer/Deep Underground elevator.
 */
/* static */ bool MultiLayerMap::IsPortalPairAllowed(LayerPortalKind kind, MapLayer a, MapLayer b)
{
	return IsPortalPairAllowed(kind, a, b, GetCurrentMultiLayerProfileSelection().id);
}

/**
 * Resolve the authoritative pair matrix for actual accepted or validated staged profile policy.
 * @param kind Physical portal kind.
 * @param a First layer.
 * @param b Second layer.
 * @param profile Explicit profile identity, independent of loader activation or viewport state.
 * @return Supported topology; operation date/terrain checks remain separate.
 */
/* static */ bool MultiLayerMap::IsPortalPairAllowed(LayerPortalKind kind, MapLayer a, MapLayer b, MultiLayerGameplayProfileID profile)
{
	//Pairs are unordered, so compare their stable storage values lower first.
	if (a > b) std::swap(a, b);
	switch (kind) {
		case LayerPortalKind::RailTunnel:
			// AD-4 adds only Underground/Deep rail for the explicit industry profile; Surface/Deep rail stays outside the connection matrix.
			if (profile == MultiLayerGameplayProfileID::MultilayerIndustry && a == MapLayer::Underground && b == MapLayer::DeepUnderground) return true;
			return a == MapLayer::Surface && b == MapLayer::Underground;
		case LayerPortalKind::RoadTunnel:
			return a == MapLayer::Surface && b == MapLayer::Underground;
		case LayerPortalKind::RoadElevator:
			return b == MapLayer::DeepUnderground && (a == MapLayer::Surface || a == MapLayer::Underground);
	}
	NOT_REACHED();
}

/**
 * The Layer a new portal connects to from its clicked Tile.
 * @param from Layer of the clicked Tile.
 * @param kind Kind of portal being built.
 * @param elevator_upper Upper destination selected when building from Deep Underground.
 * @return Target Layer, or MapLayer::Invalid for an unsupported start or destination.
 */
/* static */ MapLayer MultiLayerMap::GetPortalTargetLayer(MapLayer from, LayerPortalKind kind, MapLayer elevator_upper)
{
	switch (kind) {
		case LayerPortalKind::RailTunnel:
			if (from == MapLayer::Surface) return MapLayer::Underground;
			if (from == MapLayer::Underground) {
				// Existing callers omit a selection and retain Surface; only an explicit eligible Deep choice adds the new descent.
				if (elevator_upper == MapLayer::DeepUnderground && GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry) return MapLayer::DeepUnderground;
				if (elevator_upper == MapLayer::Invalid || elevator_upper == MapLayer::Surface) return MapLayer::Surface;
				return MapLayer::Invalid;
			}
			if (from == MapLayer::DeepUnderground && GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry) return MapLayer::Underground; // Deep rail returns through Underground; direct Surface ascent is deferred.
			return MapLayer::Invalid;
		case LayerPortalKind::RoadTunnel:
			//Tunnels retain the original Surface and Underground pair in both directions.
			if (from == MapLayer::Surface) return MapLayer::Underground;
			if (from == MapLayer::Underground) return MapLayer::Surface;
			return MapLayer::Invalid;
		case LayerPortalKind::RoadElevator:
			//Either upper Layer can descend; the picker chooses the ascent destination at the bottom.
			if (from == MapLayer::Surface || from == MapLayer::Underground) return MapLayer::DeepUnderground;
			if (from == MapLayer::DeepUnderground && (elevator_upper == MapLayer::Surface || elevator_upper == MapLayer::Underground)) return elevator_upper;
			return MapLayer::Invalid;
	}
	NOT_REACHED();
}

/**
 * Stop the game when a map asks for a layer count this build cannot store.
 * @param layer_count How many layers the caller wants to allocate.
 */
/* static */ void MultiLayerMap::ValidateLayerCount(uint layer_count)
{
	//Live games allocate every layer. Tests may ask for one.
	if (!MultiLayerMap::IsStorableLayerCount(layer_count)) {
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
	if (layer == MapLayer::DeepUnderground && GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry) return GetNativeDeepRules(); // Native rail permission is profile-specific; opening the camera cannot change legacy construction policy.
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

	//Every Layer other than the Surface must not consume random numbers. The surface seed stays bit-identical.
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
	return CheckExcavationConstruction(std::array{tile});
}

/**
 * Refuse a command whose two corner or end tiles lie on different layers.
 * FD-10: an area or a drag stays in one layer, so its walk never has to cross a void fence to find out.
 * @param a First corner or end tile.
 * @param b Second corner or end tile.
 * @return An empty cost when both tiles are on the map and share a layer, otherwise CMD_ERROR.
 */
CommandCost CheckTilesShareLayer(TileIndex a, TileIndex b)
{
	//A tile past the map has no layer, so it can never share one.
	if (a >= Map::Size() || b >= Map::Size()) return CMD_ERROR;
	if (GetTileLayer(a) != GetTileLayer(b)) return CMD_ERROR;
	return CommandCost();
}
