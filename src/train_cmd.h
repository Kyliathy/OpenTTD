/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file train_cmd.h Command definitions related to trains.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md (portal assembly)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md (restricted wagon exchange)
 */

#ifndef TRAIN_CMD_H
#define TRAIN_CMD_H

#include "command_type.h"
#include "engine_type.h"
#include "network/network_type.h"
#include "transport_type.h"
#include "vehicle_type.h"

struct LayerPortalCommissionRequest;
struct Train;

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

/**
 * Assemble a whole cutter consist on a validated portal source site in one synchronous step.
 * Validates the still-empty site once, builds the cutter then each hopper with CmdBuildVehicle's bookkeeping, and
 * couples each hopper through the shared rearrangement owner without requiring a depot tile. Charges nothing.
 * Like BuildRailVehicleForTBM it has no command trait; only the acquisition coordinator (tbm.h) calls it.
 * @param request Portal commissioning request; its source end is the assembly site.
 * @param cutter Native cutter definition for the request's model.
 * @param cutter_value Value recorded on the front engine.
 * @param unit_number Prepared unit number; it must still be the company's next free one.
 * @param wagons Hopper definitions in coupling order.
 * @param[out] front Receives the stopped, hidden front engine, or nullptr on failure.
 * @return Success, or a refusal after deleting every part this call built.
 */
CommandCost BuildTBMConsistAtSite(const LayerPortalCommissionRequest &request, const Engine *cutter, Money cutter_value, UnitID unit_number, std::span<const Engine *const> wagons, Train **front);

/**
 * Whether a train stands whole and stopped on a portal assembly site, in the depot state its assembly left it in.
 * A retired boring machine waits there until it can leave (r2mlvs4 Group 17.2); it may be sold where it stands.
 * @param front Front engine.
 * @return True when the train is stopped and every part is in the depot state on the same portal tile.
 */
bool IsTrainStoppedAtPortalSite(const Train &front);

/**
 * Exchange whole wagon units between two stationary consists that stand on one rearrangement tile, as one
 * transaction (Plan 5 Group 1.3/3). The final chains are tried first: the shared attachment rules (length and
 * coupling callbacks) and a capacity rule run on them. The capacity rule refuses any part whose rebuilt capacity
 * would be below its cargo, because the arrangement rebuild would truncate it. Without DoCommandFlag::Execute, or on
 * any refusal, both chains are restored exactly. With it, the same bookkeeping as an ordinary move follows (subtypes,
 * consist caches, groups, statistics, windows), which cannot fail; surplus cargo is never moved or truncated.
 * It has no command trait and no depot requirement; only the TBM exchange owner (tbm_exchange.h) calls it, after
 * admitting both trains to its bay. Ordinary CmdMoveRailVehicle keeps its depot guard unchanged.
 * @param flags Execute keeps the result; any other flags only try it.
 * @param first Front engine of the first consist; it stays that consist's head.
 * @param first_prefix Last part of the first consist's fixed prefix; units for it are inserted after this part.
 * @param to_first Unit heads now in the second consist, moved behind first_prefix in this order.
 * @param second Front engine of the second consist; it stays that consist's head.
 * @param to_second Unit heads now in the first consist, moved behind the second head's own unit in this order.
 * @return Success, or the first attachment, length or capacity refusal with both chains unchanged.
 */
CommandCost ExchangeRailWagonUnits(DoCommandFlags flags, Train *first, Train *first_prefix, std::span<Train *const> to_first, Train *second, std::span<Train *const> to_second);

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
