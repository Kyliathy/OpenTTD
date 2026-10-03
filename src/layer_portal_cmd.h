/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_portal_cmd.h Command that builds a pair of layer portal tiles. @see docs/zz-reach2/upgrades/2026-10/r2mlm7-layer-portal-foundation.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#ifndef LAYER_PORTAL_CMD_H
#define LAYER_PORTAL_CMD_H

#include "command_type.h"
#include "direction_type.h"
#include "map_layer_type.h"
#include "rail_type.h"
#include "road_type.h"
#include "transport_type.h"

/**
 * Infrastructure pieces one portal end counts as.
 * A rail end counts as one piece, the same as a rail depot.
 * A road end counts as two pieces, the same as a road depot.
 * @param transport Transport of the end.
 * @return Piece count for that one end.
 */
inline uint GetLayerPortalPieceCount(TransportType transport)
{
	return transport == TransportType::Rail ? 1 : 2;
}

CommandCost CmdBuildLayerPortal(DoCommandFlags flags, TileIndex tile, TransportType transport_type, MapLayer target_layer, DiagDirection shaft_dir, RailType railtype, RoadType roadtype);

DEF_CMD_TRAIT(Commands::BuildLayerPortal, CmdBuildLayerPortal, CommandFlags({CommandFlag::Auto, CommandFlag::NoWater}), CommandType::LandscapeConstruction)

void CcBuildLayerPortal(Commands cmd, const CommandCost &result, TileIndex tile, TransportType transport_type, MapLayer target_layer, DiagDirection shaft_dir, RailType railtype, RoadType roadtype);

/**
 * Move a vehicle that has stepped across a shaft edge onto the pair's entry edge.
 * @param portal End whose shaft was just crossed.
 * @param x World X, updated in place.
 * @param y World Y, updated in place.
 */
void TranslateIntoLayerPortalPair(TileIndex portal, int &x, int &y);

/**
 * Whether a vehicle at a point on a portal end is inside the shaft, so it is not drawn.
 * @param shaft Shaft direction of the end.
 * @param x World X of the point.
 * @param y World Y of the point.
 * @return True from the tunnel visibility frame of that shaft to the shaft edge.
 */
bool IsHiddenInLayerPortal(DiagDirection shaft, int x, int y);

#endif /* LAYER_PORTAL_CMD_H */
