/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geothermal_utility_cmd.h Shared synchronized estimate/build/removal boundary for paid Deep utility cells.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group3B
 */

#ifndef GEOTHERMAL_UTILITY_CMD_H
#define GEOTHERMAL_UTILITY_CMD_H

#include "command_type.h"
#include "multilayer_content.h"

/** @param flags Pure estimate or complete native execution. @param tile One fully validated Deep utility cell. @param domain Separate Heat/Electricity network. @return Whole frozen capital charge or unchanged refusal; underlying native transport metadata is preserved. */
CommandCost CmdBuildGeothermalUtilityLink(DoCommandFlags flags, TileIndex tile, UtilityEnergyDomain domain);
/** @param flags Pure estimate or complete native execution. @param tile Actual installed Deep cell. @param domain Exact owned network. @return Whole removal labour charge or unchanged refusal; no capital/energy refund or track removal occurs. */
CommandCost CmdRemoveGeothermalUtilityLink(DoCommandFlags flags, TileIndex tile, UtilityEnergyDomain domain);

DEF_CMD_TRAIT(Commands::BuildGeothermalUtilityLink, CmdBuildGeothermalUtilityLink, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::RemoveGeothermalUtilityLink, CmdRemoveGeothermalUtilityLink, {}, CommandType::LandscapeConstruction)

#endif /* GEOTHERMAL_UTILITY_CMD_H */
