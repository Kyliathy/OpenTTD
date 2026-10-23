/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_site_cmd.cpp Player/script command boundary for the original bounded cold-site registry.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "stdafx.h"
#include "geothermal_site_cmd.h"
#include "command_func.h"
#include "company_func.h"
#include "openttd.h"
#include "viewport_func.h"
#include "safeguards.h"

/** @copydoc CmdPlanGeothermalSite */
CommandCost CmdPlanGeothermalSite(DoCommandFlags flags, TileIndex tile, GeothermalVentID vent,
	MultiLayerGeothermalConstructionKind kind, TileIndex endpoint, DiagDirection access)
{
	if (_game_mode != GameMode::Normal) return CMD_ERROR;
	const GeothermalSiteRequest request{vent, _current_company, kind, tile, endpoint, access};
	const auto result = ClaimGeothermalSite(request, flags.Test(DoCommandFlag::Execute));
	if (result.error != GeothermalError::None) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) MarkTileDirtyByTile(tile);
	return CommandCost(ExpensesType::Construction);
}

/** @copydoc CmdCancelGeothermalSite */
CommandCost CmdCancelGeothermalSite(DoCommandFlags flags, GeothermalSiteID site)
{
	if (_game_mode != GameMode::Normal) return CMD_ERROR;
	const auto *claim = GetGeothermalSiteClaim(site);
	const auto tile = claim == nullptr ? INVALID_TILE : claim->site.tile;
	if (CancelGeothermalSite(site, flags.Test(DoCommandFlag::Execute)) != GeothermalError::None) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) MarkTileDirtyByTile(tile);
	return CommandCost(ExpensesType::Construction);
}
