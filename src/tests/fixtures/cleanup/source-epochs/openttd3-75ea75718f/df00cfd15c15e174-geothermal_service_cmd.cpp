/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_service_cmd.cpp Complete actual-player service admission before one shared-owner publication.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups15BBB/15BBC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "stdafx.h"
#include "geothermal_service_cmd.h"
#include "command_func.h"
#include "safeguards.h"

/** @copydoc CmdConnectGeothermalTownService */
CommandCost CmdConnectGeothermalTownService(DoCommandFlags flags, IndustryID generator, IndustryID first_riser,
	IndustryID second_riser, StationID terminal, TownID town)
{
	const auto result = ConnectGeothermalTownService({generator, first_riser, second_riser, terminal, town}, flags.Test(DoCommandFlag::Execute));
	return result.error == UtilityPowerError::None ? CommandCost(ExpensesType::Construction) : CMD_ERROR;
}
