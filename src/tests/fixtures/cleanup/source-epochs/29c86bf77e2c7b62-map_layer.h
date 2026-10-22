/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file map_layer.h The multi-layered map controller. @see docs/zz-reach2/upgrades/2026-10/r2mlm2-layer-addressing-and-controller.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#ifndef MAP_LAYER_H
#define MAP_LAYER_H

#include "map_func.h"
#include "command_type.h"

/** Stable content-profile identity, defined by multilayer_content.h rather than the map geometry owner. */
enum class MultiLayerGameplayProfileID : uint8_t;

/**
 * How the ground of a layer is drawn.
 * r2mlm5 paints Bedrock and Vacuum with stand-in sprites until the layer art exists.
 */
enum class MapLayerGround : uint8_t {
	Natural, ///< Grass, snow and desert, drawn by the existing landscape.
	Bedrock, ///< Flat rock. r2mlm5 draws this as the underground ground.
	Vacuum, ///< Empty space. r2mlm5 draws this as the space ground.
	DeepBedrock, ///< Dark Deep Underground basalt, drawn with SPR_MAP_LAYER_DEEP_FLOOR variants.
};

/**
 * A kind of construction a layer may allow.
 * The numeric values are bit positions in MapLayerConstructions.
 */
enum class MapLayerConstruction : uint8_t {
	Rail, ///< Railway track.
	Road, ///< Road, not tram.
	Tram, ///< Tram track.
	RailStation, ///< Rail station or rail waypoint.
	RoadStop, ///< Bus stop, truck stop or road waypoint.
	TrainDepot, ///< Train depot.
	RoadDepot, ///< Road depot.
	Signal, ///< Rail signal.
	Bridge, ///< Bridge.
	Tunnel, ///< Tunnel.
	Airport, ///< Airport.
	Dock, ///< Dock or buoy.
	Water, ///< Canal, lock or ship depot.
	Terraform, ///< Raising or levelling land.
	Trees, ///< Planted trees.
	Object, ///< Objects.
	Town, ///< Founding a town.
	Industry, ///< Funding an industry.
	House, ///< Placing a house.
	Sign, ///< A sign.
	LayerPortal, ///< A portal between layers. Built by a later plan.
	End, ///< Not a kind. One past the last bit.
};

/** Which constructions one layer allows. */
using MapLayerConstructions = EnumBitSet<MapLayerConstruction, uint32_t, MapLayerConstruction::End>;

/**
 * What one layer is allowed to be.
 */
struct MapLayerRules {
	MapLayerConstructions allowed; ///< Constructions this layer accepts.
	MapLayerGround ground; ///< Ground style drawn for this layer.
	uint8_t base_height; ///< Height of a freshly initialised tile.
	bool has_climate; ///< Snow, desert, grass growth and town paving run here.
	bool has_water_flow; ///< Flooding and water tile loops run here.
	bool has_ambient_sound; ///< Ambient tile sounds play here.
};

/** Portal behaviour derived from transport and the two Layers, never stored in Tile bits. */
enum class LayerPortalKind : uint8_t {
	RailTunnel, ///< Instant rail passage between Surface and Underground.
	RoadTunnel, ///< Instant road passage between Surface and Underground.
	RoadElevator, ///< Timed road passage from an upper Layer to Deep Underground.
};

/**
 * Layer meaning for the stacked map.
 *
 * Map owns the tile storage and the plane geometry. MultiLayerMap owns what a
 * layer is: which layers exist, which plane a tile belongs to, and how to walk
 * one layer. Gameplay asks this controller. It does not add Map::PlaneSize()
 * to a tile by hand.
 */
struct MultiLayerMap {
	/**
	 * Walk every tile of one layer.
	 * Same shape as Map::Iterator, bounded to a single plane.
	 */
	struct Iterator {
		using value_type = Tile;
		using pointer = Tile *;
		using reference = Tile &;
		using difference_type = size_t;
		using iterator_category = std::forward_iterator_tag;

		/**
		 * Start on one tile.
		 * @param index First tile of the walk.
		 */
		explicit Iterator(TileIndex index) : index(index) {}

		/**
		 * Whether two walks are on the same tile.
		 * @param other Walk to compare.
		 * @return True when both are on the same tile.
		 */
		bool operator==(const Iterator &other) const { return this->index == other.index; }

		/**
		 * Tile the walk is on.
		 * @return That tile.
		 */
		Tile operator*() const { return this->index; }

		/**
		 * Step to the next stored tile.
		 * @return This walk, moved one tile forward.
		 */
		Iterator &operator++() { this->index++; return *this; }

	private:
		TileIndex index; ///< Tile the walk is on.
	};

	/**
	 * One layer as a begin/end pair.
	 */
	struct IterateWrapper {
		MapLayer layer; ///< Layer this pair walks.

		/**
		 * First tile of the layer.
		 * @return Iterator at the layer base.
		 */
		Iterator begin() const { return Iterator(MultiLayerMap::GetLayerBase(this->layer)); }

		/**
		 * Tile just past the layer.
		 * @return Iterator at the next layer's base, or the storage end for the last layer.
		 */
		Iterator end() const
		{
			return Iterator(TileIndex{MultiLayerMap::GetLayerBase(this->layer).base() + Map::PlaneSize()});
		}

		/**
		 * Whether the layer stores no tiles.
		 * An allocated layer always has tiles.
		 * @return False.
		 */
		bool empty() const { return false; }
	};

	/**
	 * How many layers this game allocated.
	 * @return Map::LayerCount().
	 */
	static inline uint GetLayerCount()
	{
		return Map::LayerCount();
	}

	/**
	 * Whether a layer is one of the allocated layers.
	 * @param layer Layer to test.
	 * @return True when the layer was stored by the current Allocate.
	 */
	static inline bool IsValidLayer(MapLayer layer)
	{
		return to_underlying(layer) < Map::LayerCount();
	}

	/**
	 * First tile of one layer.
	 * @param layer Layer to locate.
	 * @return Tile (0, 0) of that layer.
	 */
	static inline TileIndex GetLayerBase(MapLayer layer)
	{
		assert(MultiLayerMap::IsValidLayer(layer));
		return TileXYL(0, 0, layer);
	}

	/**
	 * Whether a tile is stored in one layer.
	 * @param tile Tile to test.
	 * @param layer Layer it should belong to.
	 * @return True when the tile is inside the map and on that layer.
	 */
	static inline bool IsTileInLayer(TileIndex tile, MapLayer layer)
	{
		return tile < Map::Size() && GetTileLayer(tile) == layer;
	}

	/**
	 * The same plane position in another layer.
	 * @param tile Tile whose column and row are kept.
	 * @param layer Layer that should own the result.
	 * @return That column and row on the requested layer.
	 */
	static inline TileIndex TileInLayer(TileIndex tile, MapLayer layer)
	{
		assert(MultiLayerMap::IsValidLayer(layer));
		return TileXYL(TileX(tile), TilePlaneY(tile), layer);
	}

	/**
	 * The surface tile directly above or below a tile.
	 * @param tile Tile to project.
	 * @return The same column and row on the surface.
	 */
	static inline TileIndex ProjectToSurface(TileIndex tile)
	{
		return MultiLayerMap::TileInLayer(tile, MapLayer::Surface);
	}

	/**
	 * Walk exactly one layer.
	 * @param layer Layer to walk.
	 * @return Begin/end pair covering that plane and nothing else.
	 */
	static inline IterateWrapper IterateLayer(MapLayer layer)
	{
		assert(MultiLayerMap::IsValidLayer(layer));
		return IterateWrapper{layer};
	}

	/**
	 * Pick a tile in one layer from a caller-supplied random value.
	 * Callers pass Random() so the random trace stays at the call site.
	 * @param layer Layer to pick from.
	 * @param r Random value, usually Random().
	 * @return A tile in that layer.
	 */
	static inline TileIndex RandomTileInLayer(MapLayer layer, uint32_t r)
	{
		assert(MultiLayerMap::IsValidLayer(layer));
		return Map::WrapToLayer(TileIndex{r}, layer);
	}

	/**
	 * Whether a layer count can be stored by this build.
	 * FD-11: a savegame value outside this range is corrupt, never a reason to read past the planes.
	 * @param layer_count Layer count to test.
	 * @return True for 1 to MAX_MAP_LAYERS.
	 */
	static constexpr bool IsStorableLayerCount(uint layer_count)
	{
		return layer_count >= 1 && layer_count <= MAX_MAP_LAYERS;
	}

	/**
	 * Stop the game when a map asks for a layer count this build cannot store.
	 * @param layer_count How many layers the caller wants to allocate.
	 */
	static void ValidateLayerCount(uint layer_count);

	/**
	 * Rules for one allocated layer.
	 * @param layer Layer to describe.
	 * @return That layer's rules.
	 */
	static const MapLayerRules &GetRules(MapLayer layer);

	/**
	 * Rules for the layer that stores a tile.
	 * @param tile Tile whose layer is described.
	 * @return That layer's rules.
	 */
	static const MapLayerRules &GetRules(TileIndex tile);

	/**
	 * Fill one layer with its default ground and void fence.
	 * Surface keeps the historical grass fill. Other layers never call Random().
	 * @param layer Layer to fill.
	 */
	static void InitialiseLayer(MapLayer layer);

	/**
	 * Whether a portal kind may join two Layers in either order.
	 * Surface/Underground tunnels and existing road elevators retain legacy pairs; the industry profile additionally permits Underground/Deep rail.
	 * @param kind Kind of portal.
	 * @param a Layer of one end.
	 * @param b Layer of the other end.
	 * @return True for a supported pair.
	 */
	static bool IsPortalPairAllowed(LayerPortalKind kind, MapLayer a, MapLayer b);

	/**
	 * Check the same pair matrix against a validated saved profile before activation.
	 * @param kind Physical portal kind.
	 * @param a First allocated layer.
	 * @param b Second allocated layer.
	 * @param profile Explicit accepted/staged content-profile identity.
	 * @return Whether that profile supports this pair; date/occupancy remain operation checks.
	 */
	static bool IsPortalPairAllowed(LayerPortalKind kind, MapLayer a, MapLayer b, MultiLayerGameplayProfileID profile);

	/**
	 * The Layer a new portal connects to from its clicked Tile.
	 * @param from Layer of the clicked Tile.
	 * @param kind Kind of portal being built.
	 * @param elevator_upper Elevator ascent destination, or the explicitly selected Surface/Deep destination for Underground rail. Invalid preserves the default Surface rail target.
	 * @return Target Layer, or MapLayer::Invalid when this kind cannot start here.
	 */
	static MapLayer GetPortalTargetLayer(MapLayer from, LayerPortalKind kind, MapLayer elevator_upper);
};

/**
 * Refuse a construction the tile's layer does not allow.
 * @param tile Tile the command wants to change.
 * @param kind Construction being attempted.
 * @return An empty cost when the layer allows it, otherwise the layer error.
 */
CommandCost CheckLayerAllowsConstruction(TileIndex tile, MapLayerConstruction kind);

/**
 * Refuse a command whose two corner or end tiles lie on different layers.
 * @param a First corner or end tile.
 * @param b Second corner or end tile.
 * @return An empty cost when both tiles are on the map and share a layer, otherwise CMD_ERROR.
 */
CommandCost CheckTilesShareLayer(TileIndex a, TileIndex b);

#endif /* MAP_LAYER_H */
