/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_vehicle_cmd.h Native mission and generation-bound order command contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group20
 */
#ifndef SPACE_VEHICLE_CMD_H
#define SPACE_VEHICLE_CMD_H
#include "command_type.h"
#include "vehicle_type.h"
#include "space_orders.h"
/** Retain the historical argument layout for offline callers and replays.
 * @param flags Native query or execution flags.
 * @param vehicle Actual owned native miner ID; this legacy wire carries no incarnation.
 * @param body Current body ID, or UINT32_MAX for cargo-preserving recall.
 * @return Pure estimate or admitted offline mutation; multiplayer refuses this legacy command.
 */
CommandCost CmdSetSpaceMission(DoCommandFlags flags, VehicleID vehicle, uint32_t body);
DEF_CMD_TRAIT(Commands::SetSpaceMission, CmdSetSpaceMission, CommandFlags({CommandFlag::Offline}), CommandType::VehicleManagement)
/** Assign or recall only the incarnation originally selected by the player.
 * @param flags Native query or execution flags.
 * @param vehicle Original owned native miner ID.
 * @param generation Original native hull generation, checked again at agreed execution time.
 * @param body Current body ID, or UINT32_MAX for cargo-preserving recall.
 * @return Original mission admission, or unchanged refusal after stale/foreign/invalid input.
 */
CommandCost CmdSetSpaceMissionChecked(DoCommandFlags flags, VehicleID vehicle, uint64_t generation, uint32_t body);
DEF_CMD_TRAIT(Commands::SetSpaceMissionChecked, CmdSetSpaceMissionChecked, {}, CommandType::VehicleManagement)
/** Edit the original incarnation's typed order queue.
 * @param flags Native query or execution flags.
 * @param vehicle Original owned native hull ID.
 * @param generation Original hull incarnation.
 * @param edit Insert, remove, move, clear or running control.
 * @param index Existing or insertion queue position.
 * @param action Typed action for an inserted order.
 * @param kind Typed target domain for an inserted order.
 * @param target Original target ID in that domain.
 * @param target_generation Original target incarnation where the domain requires one.
 * @return Pure estimate or original synchronized order edit admission.
 */
CommandCost CmdEditSpaceOrder(DoCommandFlags flags, VehicleID vehicle, uint64_t generation, SpaceOrderEdit edit, uint8_t index, SpaceOrderAction action, SpaceOrderTargetKind kind, uint32_t target, uint64_t target_generation);
DEF_CMD_TRAIT(Commands::EditSpaceOrder, CmdEditSpaceOrder, {}, CommandType::VehicleManagement)
#endif
