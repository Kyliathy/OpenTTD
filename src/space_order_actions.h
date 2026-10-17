/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_order_actions.h Typed freight/service actions over existing physical packet and paid-work owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B
 * These adapters own no stock, service timer, allowance or second order queue.
 */
#ifndef SPACE_ORDER_ACTIONS_H
#define SPACE_ORDER_ACTIONS_H
#include "space_orders.h"

/** Original native launch anchor incarnation; service adapters never create a replacement station identity. */
struct LaunchCenterRef;

/** Recognize a reached Return/Service instruction's saved wait, without granting physical admission.
 * @param queue Actual or detached native queue.
 * @return Whether the current started bounded action can wait for its existing service/return owner.
 */
bool IsSpaceReturnServiceWait(const SpaceOrderQueue &queue);

/** Resolve physical service admission, or retain already admitted onboard work after its explicit center was retired.
 * @param craft Actual paid hull with its original native queue and position.
 * @return Whether the reached owned center admits work, or the existing actual onboard producer retains a marked lost-target instruction without granting new service.
 */
bool IsSpaceOnboardServiceOrder(const SpaceVehicle &craft);

/** Preserve existing onboard work and queue history when the actual explicit target center is removed.
 * @param center Original retiring native center incarnation, before its anchor metadata disappears.
 */
void ForgetSpaceServiceOrderCenter(const LaunchCenterRef &center);

/** Offer an entire reached orbital freight hold to the existing packet-preserving module exchange command.
 * @param craft Actual hull at its admitted Structure approach; caller owns navigation and queue completion.
 * @return True when its hold is empty after the real exchange; false retains the whole load and an explicit saved wait.
 */
bool ExecuteSpaceOrderUnload(SpaceVehicle &craft);

/** Request due service through the original contained Core/Dock or onboard producer after actual arrival.
 * @param craft Actual reached hull; existing service jobs/timers alone advance and renew its allowance.
 * @return True when no returns remain pending and its existing allowance permits continuation; false retains its exact current instruction.
 */
bool ExecuteSpaceOrderService(SpaceVehicle &craft);

#endif
