/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_portal_map.h Map accessors for a pair of tiles that connect two layers. */

#ifndef LAYER_PORTAL_MAP_H
#define LAYER_PORTAL_MAP_H

#include "tile_map.h"
#include "map_layer.h"
#include "rail_map.h"
#include "road_map.h"

/**
 * Whether this tile is one end of a layer portal.
 * @param t Tile to test.
 * @return True when the tile type is LayerPortal.
 */
inline bool IsLayerPortalTile(Tile t)
{
	return IsTileType(t, TileType::LayerPortal);
}

/**
 * Transport that uses this portal end.
 * m5 bits 3..2, the same field a tunnel uses.
 * @param t Portal end.
 * @return Rail or Road.
 */
inline TransportType GetLayerPortalTransportType(Tile t)
{
	assert(IsLayerPortalTile(t));
	return static_cast<TransportType>(GB(t.m5(), 2, 2));
}

/**
 * Heading of a vehicle that leaves this end down the shaft.
 * m5 bits 1..0. The pair's shaft is the reverse of this one.
 * @param t Portal end.
 * @return Shaft direction.
 */
inline DiagDirection GetLayerPortalShaftDirection(Tile t)
{
	assert(IsLayerPortalTile(t));
	return static_cast<DiagDirection>(GB(t.m5(), 0, 2));
}

/**
 * Direction the door of this end faces.
 * The door faces the reverse of the shaft, so a vehicle heading down the shaft arrives through the door.
 * @param t Portal end.
 * @return Door direction.
 */
inline DiagDirection GetLayerPortalDoorDirection(Tile t)
{
	return ReverseDiagDir(GetLayerPortalShaftDirection(t));
}

/**
 * Layer stored as the other end of this portal.
 * m5 bits 6..5.
 * @param t Portal end.
 * @return Target layer written when the pair was built.
 */
inline MapLayer GetLayerPortalTargetLayer(Tile t)
{
	assert(IsLayerPortalTile(t));
	return static_cast<MapLayer>(GB(t.m5(), 5, 2));
}

/**
 * The other end of this portal, at the same plane position in the target layer.
 * @param t Portal end.
 * @return The pair tile.
 */
inline TileIndex GetLayerPortalPairTile(Tile t)
{
	assert(IsLayerPortalTile(t));
	TileIndex pair = MultiLayerMap::TileInLayer(t, GetLayerPortalTargetLayer(t));
	//I-8: the other end is a portal and its shaft is the reverse of this one.
	assert(IsLayerPortalTile(pair));
	assert(GetLayerPortalShaftDirection(pair) == ReverseDiagDir(GetLayerPortalShaftDirection(t)));
	return pair;
}

/**
 * Whether a rail portal end is reserved.
 * m5 bit 4, the same bit a rail tunnel uses.
 * @param t Rail portal end.
 * @return True when the end is reserved.
 */
inline bool HasLayerPortalReservation(Tile t)
{
	assert(IsLayerPortalTile(t));
	assert(GetLayerPortalTransportType(t) == TransportType::Rail);
	return HasBit(t.m5(), 4);
}

/**
 * Set or clear the reservation on a rail portal end.
 * @param t Rail portal end.
 * @param reserved New reservation state.
 */
inline void SetLayerPortalReservation(Tile t, bool reserved)
{
	assert(IsLayerPortalTile(t));
	assert(GetLayerPortalTransportType(t) == TransportType::Rail);
	AssignBit(t.m5(), 4, reserved);
}

/**
 * Reserved track of a rail portal end.
 * The end offers one axis, so a reservation covers that axis.
 * @param t Rail portal end.
 * @return The axis track when reserved, otherwise no tracks.
 */
inline TrackBits GetLayerPortalReservationTrackBits(Tile t)
{
	return HasLayerPortalReservation(t) ? DiagDirToDiagTrack(GetLayerPortalShaftDirection(t)) : TrackBits{};
}

/**
 * Build a rail portal end.
 * Bytes follow MakeRailTunnel, plus the target layer in m5 bits 6..5.
 * @param t Tile to become the end.
 * @param o Owner of the end.
 * @param shaft_dir Heading down the shaft from this end.
 * @param target Layer of the other end.
 * @param rt Rail type on the end.
 */
inline void MakeRailLayerPortal(Tile t, Owner o, DiagDirection shaft_dir, MapLayer target, RailType rt)
{
	SetTileType(t, TileType::LayerPortal);
	SetTileOwner(t, o);
	t.m2() = 0;
	t.m3() = 0;
	t.m4() = 0;
	t.m5() = to_underlying(TransportType::Rail) << 2 | to_underlying(shaft_dir);
	SB(t.m5(), 5, 2, to_underlying(target));
	t.m6() = 0;
	t.m7() = 0;
	t.m8() = 0;
	SetRailType(t, rt);
}

/**
 * Build a road portal end.
 * Tram type stays invalid: version 1 elevators carry road vehicles only.
 * @param t Tile to become the end.
 * @param o Owner of the end.
 * @param shaft_dir Heading down the shaft from this end.
 * @param target Layer of the other end.
 * @param road_rt Road type on the end.
 */
inline void MakeRoadLayerPortal(Tile t, Owner o, DiagDirection shaft_dir, MapLayer target, RoadType road_rt)
{
	SetTileType(t, TileType::LayerPortal);
	SetTileOwner(t, o);
	t.m2() = 0;
	t.m3() = 0;
	t.m4() = 0;
	t.m5() = to_underlying(TransportType::Road) << 2 | to_underlying(shaft_dir);
	SB(t.m5(), 5, 2, to_underlying(target));
	t.m6() = 0;
	t.m7() = 0;
	t.m8() = 0;
	SetRoadOwner(t, RoadTramType::Road, o);
	if (o != OWNER_TOWN) SetRoadOwner(t, RoadTramType::Tram, o);
	SetRoadTypes(t, road_rt, INVALID_ROADTYPE);
}

/**
 * Whether leaving this tile in exitdir goes down the shaft.
 * @param tile Tile being left.
 * @param exitdir Direction the follower is leaving in.
 * @return True when the tile is a portal and exitdir is its shaft.
 */
inline bool IsLayerPortalShaftExit(TileIndex tile, DiagDirection exitdir)
{
	return IsLayerPortalTile(tile) && exitdir == GetLayerPortalShaftDirection(tile);
}

/**
 * Next tile for a vehicle or a follower that leaves tile in exitdir.
 * A shaft exit is the pair tile. Every other exit is the plain neighbour.
 * I-3: this is the only neighbour function that yields a tile in another layer.
 * @param tile Tile being left.
 * @param exitdir Direction of the step.
 * @return The tile entered by that step.
 */
inline TileIndex GetLayerPortalNextTile(TileIndex tile, DiagDirection exitdir)
{
	if (IsLayerPortalShaftExit(tile, exitdir)) return GetLayerPortalPairTile(tile);
	return TileAddByDiagDir(tile, exitdir);
}

#endif /* LAYER_PORTAL_MAP_H */
