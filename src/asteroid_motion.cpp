/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_motion.cpp Publish actual body attachments before native vehicle decisions.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group21; opt-in operation diagnostics never govern simulation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group3
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md
 */
#include "stdafx.h"
#include "space_work_cost.h"
#include "asteroid.h"
#include "space_vehicle.h"
#include "safeguards.h"

/** Publish translation-only transforms for surviving native mining leases, including stopped hulls. */
void UpdateAttachedSpaceCraftTransforms()
{
	// The bounded native pool owns each craft; publish all leased hulls before pool ordering can affect a probe's decisions.
	for (auto *vehicle : SpaceVehicle::Iterate()) {
		RecordSpaceWork(SpaceWorkKind::AttachmentVisits);
		// A stored target alone grants no attachment: only an orbital Mining phase with its exact body lease follows drift.
		if (vehicle->construction.placement != SpacePlacement::Orbital || vehicle->mission.phase != SpaceMissionPhase::Mining) continue;
		const auto *body = GetMutableAsteroidBody(vehicle->mission.target);
		// Lost or foreign leases remain the mission controller's recovery responsibility; this transform cannot claim them.
		if (body == nullptr || body->mining_slot != vehicle->index) continue;
		vehicle->position = {body->motion.position.x + vehicle->mission.attachment_offset.x,
			body->motion.position.y + vehicle->mission.attachment_offset.y};
		vehicle->UpdateSpacePosition();
	}
}
