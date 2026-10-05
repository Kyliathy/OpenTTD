/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_cmd.h Command definitions for TBM project controls, temporary support conversion, the wagon exchange, the
 * project origin and the spoil receivers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 3 and 9
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Groups 1-8
 */

#ifndef TBM_CMD_H
#define TBM_CMD_H

#include "command_type.h"
#include "tbm.h"
#include "spoil_receiver.h"

/**
 * Pause, resume or (later) cancel one project owned by the command actor. The arguments are a typed project ID and
 * action, serialised by the ordinary command buffer; orders and vehicle state are not touched.
 * @param flags Command estimate/execute policy.
 * @param project Project to control.
 * @param action Requested control.
 * @return Success or a refusal with the store unchanged.
 */
CommandCost CmdControlTBMProject(DoCommandFlags flags, TBMProjectID project, TBMProjectAction action);

DEF_CMD_TRAIT(Commands::ControlTBMProject, CmdControlTBMProject, CommandFlags{}, CommandType::VehicleManagement)

/**
 * Convert released temporary support on one tile into permanent track (Group 9). The track stays; its support
 * metadata and traffic restriction go, for the recorded conversion price. Support still in use by an unfinished
 * project refuses.
 * @param flags Command estimate/execute policy.
 * @param tile Support tile owned by the command actor.
 * @return The conversion price, or a refusal with the registry unchanged.
 */
CommandCost CmdConvertTBMSupport(DoCommandFlags flags, TileIndex tile);

DEF_CMD_TRAIT(Commands::ConvertTBMSupport, CmdConvertTBMSupport, CommandFlags{}, CommandType::LandscapeConstruction)

/**
 * Assign a secondary locomotive to one project and register the empty set it brings (Plan 5 Groups 1.2 and 6.1).
 * @param flags Command estimate/execute policy.
 * @param project Project of the command actor.
 * @param hauler Non-TBM front engine of the same company.
 * @return Success or a refusal with the assignments unchanged.
 */
CommandCost CmdAssignTBMHauler(DoCommandFlags flags, TBMProjectID project, VehicleID hauler);

/**
 * End one hauler's assignment (cancellation); trains, wagons and packets are not touched, and its set can be recovered.
 * @param flags Command estimate/execute policy.
 * @param hauler Assigned hauler of the command actor.
 * @return Success or a refusal with the assignments unchanged.
 */
CommandCost CmdReleaseTBMHauler(DoCommandFlags flags, VehicleID hauler);

/**
 * Move a project's exchange to its source portal or to a validated excavated depot bay (Plan 5 Group 1.1).
 * @param flags Command estimate/execute policy.
 * @param project Project of the command actor, with no hauler assigned.
 * @param tile New rearrangement tile.
 * @return Success or a refusal with the project unchanged.
 */
CommandCost CmdSetTBMExchangeBay(DoCommandFlags flags, TBMProjectID project, TileIndex tile);

/**
 * Exchange the project's loaded wagons for one assigned hauler's empty set at the bay (Plan 5 Group 3).
 * @param flags Command estimate/execute policy; an estimate runs the full trial and restores everything.
 * @param project Project of the command actor.
 * @param hauler One of its assigned haulers, whose turn it is at the bay.
 * @return Success, the shared coupling refusal text, or CMD_ERROR, with nothing changed on refusal.
 */
CommandCost CmdExchangeTBMWagons(DoCommandFlags flags, TBMProjectID project, VehicleID hauler);

/**
 * Replace a project's origin with another company Surface railway station (Plan 5 Group 8.1). Sets that already
 * visited the old origin keep that visit; every other loaded set must reach the new one.
 * @param flags Command estimate/execute policy.
 * @param project Unfinished project of the command actor.
 * @param station Company-owned Surface railway station.
 * @return Success or a refusal with the project unchanged.
 */
CommandCost CmdSetTBMProjectOrigin(DoCommandFlags flags, TBMProjectID project, StationID station);

/**
 * Choose crushing or surplus disposal at a Rock crusher (Plan 5 Group 8.2).
 * @param flags Command estimate/execute policy.
 * @param industry Rock crusher founded by the command actor.
 * @param mode New mode.
 * @return Success or CMD_ERROR with the receiver unchanged.
 */
CommandCost CmdSetSpoilReceiverMode(DoCommandFlags flags, IndustryID industry, SpoilReceiverMode mode);

DEF_CMD_TRAIT(Commands::AssignTBMHauler, CmdAssignTBMHauler, CommandFlags{}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::ReleaseTBMHauler, CmdReleaseTBMHauler, CommandFlags{}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::SetTBMExchangeBay, CmdSetTBMExchangeBay, CommandFlags{}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::ExchangeTBMWagons, CmdExchangeTBMWagons, CommandFlags{}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::SetTBMProjectOrigin, CmdSetTBMProjectOrigin, CommandFlags{}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::SetSpoilReceiverMode, CmdSetSpoilReceiverMode, CommandFlags{}, CommandType::OtherManagement)

#endif /* TBM_CMD_H */
