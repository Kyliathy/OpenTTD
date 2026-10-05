/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file train_cmd.h Command definitions related to trains. @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md */

#ifndef TRAIN_CMD_H
#define TRAIN_CMD_H

#include "command_type.h"
#include "engine_type.h"
#include "network/network_type.h"
#include "vehicle_type.h"

struct LayerPortalCommissionRequest;

/**
 * Construct one native railway unit at a portal-owner-validated assembly site.
 * This primitive constructs stopped unpublished parts. Its caller must preflight
 * complete-consist funds/route/capacity and own rollback before publishing a project.
 * It has no command trait; whole-project commissioning remains unavailable until that coordinator exists.
 * @param flags Pure estimate or actual construction policy.
 * @param request Complete portal commissioning proposal.
 * @param engine Actual native cutter/hopper definition.
 * @param ret Receives the actual new unit on successful execution.
 * @return Part construction result, excluding the controller-owned purchase charge; portal metadata remains intact.
 */
CommandCost BuildRailVehicleForTBM(DoCommandFlags flags, const LayerPortalCommissionRequest &request, const Engine *engine, Vehicle **ret);

CommandCost CmdBuildRailVehicle(DoCommandFlags flags, TileIndex tile, const Engine *e, Vehicle **ret);
CommandCost CmdSellRailWagon(DoCommandFlags flags, Vehicle *t, bool sell_chain, bool backup_order, ClientID user);

CommandCost CmdMoveRailVehicle(DoCommandFlags flags, VehicleID src_veh, VehicleID dest_veh, bool move_chain);
CommandCost CmdForceTrainProceed(DoCommandFlags flags, VehicleID veh_id);
CommandCost CmdReverseTrainDirection(DoCommandFlags flags, VehicleID veh_id, bool reverse_single_veh);

DEF_CMD_TRAIT(Commands::MoveRailVehicle, CmdMoveRailVehicle, CommandFlag::Location, CommandType::VehicleConstruction)
DEF_CMD_TRAIT(Commands::ForceTrainProceed, CmdForceTrainProceed, CommandFlag::Location, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::ReverseTrainDirection, CmdReverseTrainDirection, CommandFlag::Location, CommandType::VehicleManagement)

void CcBuildWagon(Commands, const CommandCost &result, VehicleID new_veh_id, uint, uint16_t, CargoArray, TileIndex tile, EngineID, bool, CargoType, ClientID);

#endif /* TRAIN_CMD_H */
