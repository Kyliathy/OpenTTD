/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_site_cmd.h Synchronized cold-site planning over the original geothermal claim owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef GEOTHERMAL_SITE_CMD_H
#define GEOTHERMAL_SITE_CMD_H

#include "command_type.h"
#include "geothermal.h"

/**
 * Reserve one fully excavated cold site; funding and actual transported construction remain separate commands.
 * @param flags Pure estimate or synchronized execution.
 * @param tile Explicit world machinery anchor, never a view-relative coordinate.
 * @param vent Original natural source selected by the player.
 * @param kind Immutable collector or dated generator construction family.
 * @param endpoint Explicit boundary utility cell within the complete floor.
 * @param access Excavated outward construction approach.
 * @return Free whole reservation or unchanged refusal; no machinery, stock, heat or electricity is created.
 */
CommandCost CmdPlanGeothermalSite(DoCommandFlags flags, TileIndex tile, GeothermalVentID vent,
	MultiLayerGeothermalConstructionKind kind, TileIndex endpoint, DiagDirection access);

/**
 * Release only an owned active cold reservation, retaining its original identity and all natural geology.
 * @param flags Pure cancellation quote or synchronized execution.
 * @param site Exact retained planning identity; funded or cancelled sites cannot be released again.
 * @return Free complete cancellation or unchanged refusal; actual native machinery is never demolished here.
 */
CommandCost CmdCancelGeothermalSite(DoCommandFlags flags, GeothermalSiteID site);

DEF_CMD_TRAIT(Commands::PlanGeothermalSite, CmdPlanGeothermalSite, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::CancelGeothermalSite, CmdCancelGeothermalSite, {}, CommandType::OtherManagement)

#endif /* GEOTHERMAL_SITE_CMD_H */
