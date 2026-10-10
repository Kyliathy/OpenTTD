/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geothermal_utility_cmd.h Shared synchronized estimate/build/removal boundary for paid rock-layer utility cells.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Groups3B-5
 */

#ifndef GEOTHERMAL_UTILITY_CMD_H
#define GEOTHERMAL_UTILITY_CMD_H

#include "command_type.h"
#include "multilayer_content.h"

/** @param flags Pure estimate or complete native execution. @param tile One fully validated permitted rock-layer utility cell. @param domain Separate network: Heat stays Deep; electricity also supports Underground relays. @return Whole frozen capital charge or unchanged refusal; underlying native transport metadata is preserved. */
CommandCost CmdBuildGeothermalUtilityLink(DoCommandFlags flags, TileIndex tile, UtilityEnergyDomain domain);
/** @param flags Pure estimate or complete native execution. @param tile Actual installed rock-layer cell. @param domain Exact owned Heat/Electricity network. @return Whole removal labour charge or unchanged refusal; no capital/energy refund or track removal occurs. */
CommandCost CmdRemoveGeothermalUtilityLink(DoCommandFlags flags, TileIndex tile, UtilityEnergyDomain domain);

DEF_CMD_TRAIT(Commands::BuildGeothermalUtilityLink, CmdBuildGeothermalUtilityLink, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::RemoveGeothermalUtilityLink, CmdRemoveGeothermalUtilityLink, {}, CommandType::LandscapeConstruction)

#endif /* GEOTHERMAL_UTILITY_CMD_H */
