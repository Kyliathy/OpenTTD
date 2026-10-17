/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file vehicle_cmd.h Command definitions for vehicles.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups8B/8BB; paid purchase/clone callbacks identify original physical facilities.
 */

#ifndef VEHICLE_CMD_H
#define VEHICLE_CMD_H

#include "command_type.h"
#include "engine_type.h"
#include "vehicle_type.h"
#include "vehiclelist.h"
#include "vehiclelist_cmd.h"
#include "cargo_type.h"
#include "network/network_type.h"

/** Build an ordinary native vehicle or admit one actual paid Space facility purchase.
 * @param flags Estimate/query or synchronized execution; Space replacement requires its separate old-hull settlement owner.
 * @param tile Actual ordinary depot, Surface workshop or orbital Assembly world tile.
 * @param eid Existing native EngineID, including a canonical Space mapping supplied by its real producer.
 * @param use_free_vehicles Ordinary reuse preference; Space requests always use their actual complete paid recipe.
 * @param cargo Requested ordinary refit, or default/INVALID_CARGO for currently supported Space purchases.
 * @param client_id Native requester, assigned by the command framework.
 * @return Cost, actual new VehicleID, cargo/mail capacities and per-cargo capacities. Successful queued orbital execution has Invalid VehicleID;
 *         its real factory owns the issued job and the callback receives the original facility/Engine arguments.
 */
std::tuple<CommandCost, VehicleID, uint, uint16_t, CargoArray> CmdBuildVehicle(DoCommandFlags flags, TileIndex tile, EngineID eid, bool use_free_vehicles, CargoType cargo, ClientID client_id);
CommandCost CmdSellVehicle(DoCommandFlags flags, VehicleID v_id, bool sell_chain, bool backup_order, ClientID client_id);
std::tuple<CommandCost, uint, uint16_t, CargoArray> CmdRefitVehicle(DoCommandFlags flags, VehicleID veh_id, CargoType new_cargo_type, uint8_t new_subtype, bool auto_refit, bool only_this, uint8_t num_vehicles);
CommandCost CmdSendVehicleToDepot(DoCommandFlags flags, VehicleID veh_id, DepotCommandFlags depot_cmd, const VehicleListIdentifier &vli);
CommandCost CmdChangeServiceInt(DoCommandFlags flags, VehicleID veh_id, uint16_t serv_int, bool is_custom, bool is_percent);
CommandCost CmdRenameVehicle(DoCommandFlags flags, VehicleID veh_id, const std::string &text);
/** Clone an ordinary fleet head or request one new actual material-paid Space product.
 * @param flags Native estimate/NoTest execution; Space query-cost inspection remains pure.
 * @param tile Actual ordinary depot, Surface workshop or orbital Assembly world tile.
 * @param veh_id Original real owned hull; its cargo, service owners and action progress remain solely with it.
 * @param share_orders Ordinary shared-list preference; native Space queues require independent copied instructions.
 * @return Actual cost and immediate new hull, or Invalid while a real orbital job owns the paid clone intent.
 */
std::tuple<CommandCost, VehicleID> CmdCloneVehicle(DoCommandFlags flags, TileIndex tile, VehicleID veh_id, bool share_orders);
CommandCost CmdStartStopVehicle(DoCommandFlags flags, VehicleID veh_id, bool evaluate_startstop_cb);
CommandCost CmdMassStartStopVehicle(DoCommandFlags flags, TileIndex tile, bool do_start, bool vehicle_list_window, const VehicleListIdentifier &vli);
CommandCost CmdDepotSellAllVehicles(DoCommandFlags flags, TileIndex tile, VehicleType vehicle_type);
CommandCost CmdDepotMassAutoReplace(DoCommandFlags flags, TileIndex tile, VehicleType vehicle_type);

DEF_CMD_TRAIT(Commands::BuildVehicle, CmdBuildVehicle, CommandFlag::ClientID, CommandType::VehicleConstruction)
DEF_CMD_TRAIT(Commands::SellVehicle, CmdSellVehicle, CommandFlags({CommandFlag::ClientID, CommandFlag::Location}), CommandType::VehicleConstruction)
DEF_CMD_TRAIT(Commands::RefitVehicle, CmdRefitVehicle, CommandFlag::Location, CommandType::VehicleConstruction)
DEF_CMD_TRAIT(Commands::SendVehicleToDepot, CmdSendVehicleToDepot, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::ChangeServiceInterval, CmdChangeServiceInt, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::RenameVehicle, CmdRenameVehicle, {}, CommandType::OtherManagement)
DEF_CMD_TRAIT(Commands::CloneVehicle, CmdCloneVehicle, CommandFlag::NoTest, CommandType::VehicleConstruction) // NewGRF callbacks influence building and refitting making it impossible to correctly estimate the cost
DEF_CMD_TRAIT(Commands::StartStopVehicle, CmdStartStopVehicle, CommandFlag::Location, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::MassStartStop, CmdMassStartStopVehicle, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::DepotMassSell, CmdDepotSellAllVehicles, {}, CommandType::VehicleConstruction)
DEF_CMD_TRAIT(Commands::DepotMassAutoreplace, CmdDepotMassAutoReplace, {}, CommandType::VehicleConstruction)

/** Present an actual purchased hull or the freshly admitted paid orbital job, using the existing callback-with-command-arguments contract.
 * @param command Native purchase command identity.
 * @param result Actual whole command cost/refusal.
 * @param new_veh_id Actual new hull, or Invalid for a queued orbital request which has not manufactured a vehicle yet.
 * @param capacity Quoted/default freight capacity; display handling does not grant cargo.
 * @param mail_capacity Ordinary aircraft mail capacity.
 * @param capacities Ordinary per-cargo capacity quote.
 * @param tile Original actual world facility tile, independent of camera selection.
 * @param engine Original canonical native Engine used by the admitted request.
 * @param use_free_vehicles Original ordinary purchase preference; never orbital recipe permission.
 * @param cargo Original requested/default cargo; no callback refit or temporary purchase occurs.
 * @param client_id Original native requester; callback changes presentation only.
 */
void CcBuildPrimaryVehicle(Commands command, const CommandCost &result, VehicleID new_veh_id, uint capacity, uint16_t mail_capacity, CargoArray capacities,
	TileIndex tile, EngineID engine, bool use_free_vehicles, CargoType cargo, ClientID client_id);
void CcStartStopVehicle(Commands, const CommandCost &result, VehicleID veh_id, bool);

template <typename Tcont, typename Titer>
inline EndianBufferWriter<Tcont, Titer> &operator <<(EndianBufferWriter<Tcont, Titer> &buffer, const CargoArray &cargo_array)
{
	for (const uint &amt : cargo_array) {
		buffer << amt;
	}
	return buffer;
}

inline EndianBufferReader &operator >>(EndianBufferReader &buffer, CargoArray &cargo_array)
{
	for (uint &amt : cargo_array) {
		buffer >> amt;
	}
	return buffer;
}

#endif /* VEHICLE_CMD_H */
