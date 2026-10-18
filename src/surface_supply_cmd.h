/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_cmd.h Synchronized paid Surface supply placement and unshipped cancellation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups13CCC/13CCD.
 * The original command kernel settles complete frozen expenses/refunds; estimates never publish rights or cargo.
 */
#ifndef SURFACE_SUPPLY_CMD_H
#define SURFACE_SUPPLY_CMD_H

#include "command_type.h"
#include "station_type.h"
#include "surface_supply_order.h"

/** Place one explicitly identified complete paid intent, or acknowledge an identical retained pending/fulfilled retry without another charge.
 * @param flags Native estimate/execute policy; execution recaptures real admission and publishes the commitment.
 * @param origin Actual owned Surface freight station identity.
 * @param label_value Canonical four-byte label value in big-endian order, not a local cargo ordinal/registry.
 * @param quantity Entire requested purchase.
 * @param intent Original proposed monotonic intent ID; retries must never issue a different purchase.
 * @return Complete Property expense for the original command kernel, zero for an exact retained retry, or unchanged refusal.
 */
CommandCost CmdPlaceSurfaceSupplyOrder(DoCommandFlags flags, StationID origin, uint32_t label_value, uint16_t quantity, SurfaceSupplyOrderID intent);

/** Cancel only a retained undispatched paid intent; native custody permanently ends refund permission.
 * @param flags Native estimate/execute policy; only execution releases the commitment.
 * @param intent Original native intent; a terminal/retired identity can never produce another refund.
 * @return The frozen negative Property expense for the original command kernel, or unchanged refusal for a foreign/terminal/retired intent.
 */
CommandCost CmdCancelSurfaceSupplyOrder(DoCommandFlags flags, SurfaceSupplyOrderID intent);

DEF_CMD_TRAIT(Commands::PlaceSurfaceSupplyOrder, CmdPlaceSurfaceSupplyOrder, {}, CommandType::OtherManagement)
DEF_CMD_TRAIT(Commands::CancelSurfaceSupplyOrder, CmdCancelSurfaceSupplyOrder, {}, CommandType::OtherManagement)

#endif /* SURFACE_SUPPLY_CMD_H */
