/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file depot_map.h Native depot identities, including registered portal magazines. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group8; docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#ifndef DEPOT_MAP_H
#define DEPOT_MAP_H

#include "order_type.h"
#include "station_map.h"
#include "layer_portal_map.h"

/**
 * Check if a tile is a depot and it is a depot of the given type.
 * @param tile The tile to check.
 * @param type The type of transport.
 * @return \c true iff the given tile is a depot for the given transport type.
 */
inline bool IsDepotTypeTile(Tile tile, TransportType type)
{
	switch (type) {
		default: NOT_REACHED();
		case TransportType::Rail:
			return IsRailDepotTile(tile) || IsLayerPortalHangarTile(tile);

		case TransportType::Road:
			return IsRoadDepotTile(tile);

		case TransportType::Water:
			return IsShipDepotTile(tile);

		case TransportType::Air:
			return IsHangarTile(tile);
	}
}

/**
 * Is the given tile a tile with a depot on it?
 * @param tile the tile to check
 * @return true if and only if there is a depot on the tile.
 */
inline bool IsDepotTile(Tile tile)
{
	return IsRailDepotTile(tile) || IsRoadDepotTile(tile) || IsShipDepotTile(tile) || IsHangarTile(tile) || IsLayerPortalHangarTile(tile);
}

/**
 * Get the index of which depot is attached to the tile.
 * @param t the tile
 * @pre IsRailDepotTile(t) || IsRoadDepotTile(t) || IsShipDepotTile(t) || IsLayerPortalHangarTile(t)
 * @return DepotID
 */
inline DepotID GetDepotIndex(Tile t)
{
	/* Aircraft hangars use station IDs; a registered TBM magazine has a real Depot. */
	assert(IsRailDepotTile(t) || IsRoadDepotTile(t) || IsShipDepotTile(t) || IsLayerPortalHangarTile(t));
	return DepotID{t.m2()};
}

/**
 * Get the destination index of a 'depot'. For hangars that's the station index, for the rest a depot index.
 * @param t the tile
 * @pre IsDepotTile(t)
 * @return DepotID
 */
inline DestinationID GetDepotDestinationIndex(Tile t)
{
	if (IsHangarTile(t)) return GetStationIndex(t);
	return GetDepotIndex(t);
}

/**
 * Get the type of vehicles that can use a depot
 * @param t The tile
 * @pre IsDepotTile(t)
 * @return the type of vehicles that can use the depot
 */
inline VehicleType GetDepotVehicleType(Tile t)
{
	switch (GetTileType(t)) {
		default: NOT_REACHED();
		case TileType::Railway: return VehicleType::Train;
		case TileType::LayerPortal: assert(IsLayerPortalHangarTile(t)); return VehicleType::Train;
		case TileType::Road: return VehicleType::Road;
		case TileType::Water: return VehicleType::Ship;
		case TileType::Station: return VehicleType::Aircraft;
	}
}

#endif /* DEPOT_MAP_H */
