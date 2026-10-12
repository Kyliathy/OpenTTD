/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
#ifndef SPACE_VEHICLE_CMD_H
#define SPACE_VEHICLE_CMD_H
#include "command_type.h"
#include "vehicle_type.h"
/** Assign a current body ID, or UINT32_MAX to recall with cargo intact. Estimate is pure. */
CommandCost CmdSetSpaceMission(DoCommandFlags flags, VehicleID vehicle, uint32_t body);
DEF_CMD_TRAIT(Commands::SetSpaceMission, CmdSetSpaceMission, {}, CommandType::VehicleManagement)
#endif
