/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_continuation.h Effective native mining targets and progress within the original typed queue.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups20/23/25; the same mission also retains local unqueued legacy departure.
 */
#ifndef SPACE_MINING_CONTINUATION_H
#define SPACE_MINING_CONTINUATION_H
#include "space_orders.h"
#include "space_mining_state.h"
#include "space_return_target.h"
#include "asteroid.h"
struct SpaceVehicle;

/** @param craft Original native hull. @return Whether its current Mine has configured/unfinished intent, or its original empty queue accompanies a saved local legacy departure. */
bool HasSpaceMiningContinuationOrder(const SpaceVehicle &craft);
/** Resolve the target consumed by the original queue, approach owner and action adapters.
 * @param craft Actual hull; the player's original entries remain unchanged.
 * @return Effective body/Visit/receiver, or absence while safely detached/waiting. This read-only view grants no movement, lease or admission.
 */
std::optional<SpaceOrderTarget> ResolveEffectiveSpaceOrderTarget(const SpaceVehicle &craft);
/** Publish continuation after ordinary mining completion or an explicitly admitted safety transition.
 * @param craft Original actual miner with a current Mine or an unqueued legacy mining mission. Derives a missing legacy pin only from its actual live body; releases only its own body lease/work and preserves stopped state, pose, packets and actual static service ownership.
 * @param departure Previously admitted local departure point, if available; absence retains a bounded departure wait.
 * @return Whether this original instruction owns the transition. Group25B supplies actual pre-exit safety admission; this function never starts a stopped hull.
 */
bool BeginSpaceMiningContinuation(SpaceVehicle &craft, std::optional<SpacePoint> departure);
/** @param craft Running original miner; caller already established HasSpaceMiningContinuationOrder. @return True when continuation handled its one local/navigation/wait/handling opportunity; false delegates only actual Approach/Mining to the original native controller. Unqueued departure finishes into ordinary native Return. */
bool TickSpaceMiningContinuation(SpaceVehicle &craft);
/** @param craft Native hull with its sole original queue, field and mission. @return Whether contained intent, effective identity, pinned mineral and physical work phase agree; no target selection or state publication occurs. */
bool ValidateNativeSpaceMiningContinuation(const SpaceVehicle &craft);
/** Read-only native or same-file candidate facts for one original mission; contains no resource, cargo, lease or work owner. */
struct SpaceMiningContinuationContext {
	const SpaceOrderQueue &queue;
	std::span<const AsteroidIdentityRecord> bodies;
	uint32_t next_body_id;
	std::optional<SpacePlaneBounds> bounds;
	uint16_t model;
	SpaceMissionPhase mission_phase;
	OrbitalBodyID mission_target;
	uint16_t work_ticks;
	bool has_freight, mineral_defined;
	std::optional<CargoLabel> freight_mineral;
	std::optional<SpaceReturnRoute> route;
	uint64_t next_structure_generation, next_center_generation;
};
/** @param intent Sole original/copied contained intent. @param context Facts from the matching native owner or candidate file only. @return Whether history, mineral, effective target and queue/phase agree, without consulting another world's pools or publishing a decision. */
bool ValidateSpaceMiningContinuation(const SpaceMiningContinuationState &intent, const SpaceMiningContinuationContext &context);
#endif /* SPACE_MINING_CONTINUATION_H */
