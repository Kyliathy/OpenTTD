/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_service_cmd.h Synchronized connection of actual geothermal machinery to one native town service.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups15BBB/15BBC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef GEOTHERMAL_SERVICE_CMD_H
#define GEOTHERMAL_SERVICE_CMD_H

#include "command_type.h"
#include "utility_power.h"

/**
 * Connect or recover already paid machinery and a real recipient through the single shared utility owner.
 * @param flags Pure complete estimate or synchronized execution.
 * @param generator Actual completed native generator and its original coupled electrical output.
 * @param first_riser Paid Deep sending machinery on the explicit physical route.
 * @param second_riser Paid Underground sender, or invalid for a direct Deep/Surface pair.
 * @param terminal Actual owned occupied Surface train/truck terminal.
 * @param town Actual native town authorized by the terminal's serving tiles.
 * @return Free logical connection or unchanged refusal; all capital, materials, installed cables and operating service remain with their existing owners.
 */
CommandCost CmdConnectGeothermalTownService(DoCommandFlags flags, IndustryID generator, IndustryID first_riser,
	IndustryID second_riser, StationID terminal, TownID town);

DEF_CMD_TRAIT(Commands::ConnectGeothermalTownService, CmdConnectGeothermalTownService, {}, CommandType::LandscapeConstruction)

#endif /* GEOTHERMAL_SERVICE_CMD_H */
