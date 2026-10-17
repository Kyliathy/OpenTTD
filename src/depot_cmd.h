/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file depot_cmd.h Command definitions related to depots.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BB; original arguments identify queued clone callbacks.
 */

#ifndef DEPOT_CMD_H
#define DEPOT_CMD_H

#include "command_type.h"
#include "depot_type.h"
#include "vehicle_type.h"

CommandCost CmdRenameDepot(DoCommandFlags flags, DepotID depot_id, const std::string &text);

DEF_CMD_TRAIT(Commands::RenameDepot, CmdRenameDepot, {}, CommandType::OtherManagement)

/** Present one actual new hull or the original physical factory's newly admitted paid clone request.
 * @param command Original native clone command identity.
 * @param result Whole actual paid clone cost/refusal.
 * @param veh_id Actual new hull; Invalid identifies an admitted orbital job which has not manufactured its product yet.
 * @param tile Original actual destination world tile.
 * @param source Original source VehicleID; the saved job retains its full original incarnation independently.
 * @param share_orders Original clone sharing preference; native Space lists explicitly refuse unsupported sharing.
 */
void CcCloneVehicle(Commands command, const CommandCost &result, VehicleID veh_id, TileIndex tile, VehicleID source, bool share_orders);

#endif /* DEPOT_CMD_H */
