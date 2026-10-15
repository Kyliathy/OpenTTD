/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_site.cpp Sole launch terrain predicate, reusable before player-company initialization.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group3
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
 */
#include "stdafx.h"
#include "launch_site.h"
#include "launch_center.h"
#include "command_type.h"
#include "map_layer.h"
#include "bridge_map.h"
#include "vehicle_func.h"
#include "table/strings.h"
#include "safeguards.h"

/**
 * Apply the launch command's terrain rules without depending on a current company or clearing quote.
 * @param tile Explicit Surface footprint tile.
 * @param reference_height Flat elevation required for all footprint tiles.
 * @return Original native geometry/ground-vehicle refusal, or zero-cost success.
 */
CommandCost GetLaunchCenterTerrainTileBlock(TileIndex tile, int reference_height)
{
	if (!MultiLayerMap::IsTileInLayer(tile, MapLayer::Surface)) return CommandCost(STR_ERROR_LAUNCH_CENTER_SURFACE);
	// Every physical zone needs clear/tree land at one flat elevation; water, existing facilities or slopes cannot host it.
	if ((!IsTileType(tile, TileType::Clear) && !IsTileType(tile, TileType::Trees)) ||
		!IsTileFlat(tile) || GetTileZ(tile) != reference_height) return CommandCost(STR_ERROR_FLAT_LAND_REQUIRED);
	if (IsBridgeAbove(tile)) return CommandCost(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
	return EnsureNoVehicleOnGround(tile);
}

/**
 * Validate guarded site geometry without allocating a station or changing company policy.
 * @param anchor Surface build anchor.
 * @param rotation Native quarter-turn layout index.
 * @return First native geometry refusal, or zero-cost success.
 */
CommandCost GetLaunchCenterSiteGeometryBlock(TileIndex anchor, uint8_t rotation)
{
	const auto area = GetLaunchCenterFootprint(anchor, rotation);
	if (!area) return CommandCost(STR_ERROR_LAUNCH_CENTER_SURFACE);
	if (!HasLaunchCenterWaterAccess(*area)) return CommandCost(STR_ERROR_LAUNCH_CENTER_WATER);
	const int height = GetTileZ(anchor);
	// A starter candidate is usable only when every tile passes the actual placement terrain predicate.
	for (const auto tile : *area) {
		const auto result = GetLaunchCenterTerrainTileBlock(tile, height);
		if (result.Failed()) return result;
	}
	return CommandCost();
}
