/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_departure.cpp Stable native mining safety publication before body translation.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups25/25B/29.
 */
#include "stdafx.h"
#include "space_mining_departure.h"
#include "space_mining_continuation.h"
#include "space_mining_target.h"
#include "space_mining_tuning.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "orbital_service.h"
#include "company_base.h"
#include "map_func.h"
#include "safeguards.h"

/** @param craft Original hull. @return Whether its actual deployed mining pursuit/lease can participate without taking over another order or service owner. */
static bool IsMiningDepartureParticipant(const SpaceVehicle &craft)
{
	const auto &q = craft.space_orders;
	// Safety includes stopped miners but never revives a carried/crashed hull,
	// unrelated instruction, detached wait or separately accountable service.
	if (craft.construction.placement != SpacePlacement::Orbital || craft.vehstatus.Test(VehState::Crashed) ||
		!Company::IsValidID(craft.owner) || craft.generation == 0 || GetSpaceMiningTuning(GetSpaceVehicleModel(craft)) == nullptr ||
		(craft.mission.phase != SpaceMissionPhase::Mining && craft.mission.phase != SpaceMissionPhase::Approach) ||
		HasOrbitalServiceAssignment(craft) || (craft.manifest && craft.manifest->service_ticks != 0)) return false;
	// A future/non-Mine instruction cannot borrow a historical body target.
	// The empty original queue is the explicitly supported legacy/manual case.
	if (!q.entries.empty() && (q.cursor >= q.entries.size() || !q.started || q.entries[q.cursor].action != SpaceOrderAction::Mine)) return false;
	const auto *body = GetMutableAsteroidBody(craft.mission.target);
	if (body == nullptr) return false;
	// Only the original target request/lease may be relinquished. A distinct
	// static berth must remain with its original service/navigation owner.
	if (q.approach && q.approach->target != SpaceOrderTarget{SpaceOrderTargetKind::Body, body->id.base(), 0}) return false;
	return craft.mission.phase != SpaceMissionPhase::Mining || body->mining_slot == craft.index;
}

/** @copydoc GatherSpaceMiningDepartureParticipants */
std::vector<SpaceCraftRef> GatherSpaceMiningDepartureParticipants()
{
	std::vector<SpaceCraftRef> participants;
	// Native capacity bounds this temporary reference list. Stable IDs make
	// competing release/staging choices independent of field storage order.
	for (const auto *craft : SpaceVehicle::Iterate()) if (IsMiningDepartureParticipant(*craft)) participants.push_back(GetSpaceCraftReference(*craft));
	/** @param a Original participant. @param b Original competitor. @return Stable pool identity ordering; generations are revalidated at commit. */
	std::ranges::sort(participants, [](const auto &a, const auto &b) { return a.vehicle < b.vehicle; });
	return participants;
}

/** @copydoc PlanSpaceBodyDeparture */
std::optional<SpaceBodyDeparture> PlanSpaceBodyDeparture(SpaceCraftRef reference)
{
	const auto *craft = ResolveSpaceCraft(reference);
	if (craft == nullptr || !IsMiningDepartureParticipant(*craft)) return std::nullopt;
	const auto *body = GetMutableAsteroidBody(craft->mission.target);
	const auto plan = PlanSpaceDeparture(*craft);
	if (plan.status == SpaceDepartureStatus::InvalidPosition) return std::nullopt;
	if (craft->mission.phase == SpaceMissionPhase::Mining) {
		// Release when actual local travel plus one pre-translation margin has
		// consumed the attachment's remaining safe opportunities. Stop is not an
		// exemption; a blocked route releases at the last safe existing point.
		if (!plan.attachment || !plan.release_after_ticks || *plan.release_after_ticks != 0) return std::nullopt;
		return SpaceBodyDeparture{reference, body->id, SpaceBodyDepartureReason::UnsafeAttachment, plan};
	}
	const auto bounds = GetSpacePlaneBounds(Map::SizeX(), Map::SizeY());
	const auto obstacles = GetSpaceAttachmentObstacles();
	if (!bounds || !obstacles) return std::nullopt;
	const auto step = StepSpaceMotionStraight(body->motion, *bounds);
	if (!step) return std::nullopt;
	if (!step->crossed_edges.Any()) {
		auto moved = *body; moved.motion = step->proposed;
		// The native pursuit opportunity follows body translation. Forecast from
		// that actual next target point and honor this mission's remaining frozen
		// budget rather than granting a new deadline or another hull's lease.
		const auto intercept = ForecastSpaceMiningIntercept(craft->position, craft->mission.travel_remainder,
			GetSpaceVehicleModel(*craft), moved, *bounds, *obstacles);
		if (intercept && intercept->arrival_ticks <= craft->mission.approach_ticks_left) return std::nullopt;
	}
	return SpaceBodyDeparture{reference, body->id, SpaceBodyDepartureReason::UnsafeInterception, plan};
}

/** @copydoc BeginSpaceBodyDeparture */
bool BeginSpaceBodyDeparture(const SpaceBodyDeparture &proposal)
{
	const auto current = PlanSpaceBodyDeparture(proposal.craft);
	// A quote cannot outlive its exact incarnation, body, due reason, current
	// pose or occupancy. Earlier releases may have claimed a contained point.
	if (!current || *current != proposal) return false;
	auto *craft = ResolveSpaceCraft(proposal.craft);
	return BeginSpaceMiningContinuation(*craft, proposal.plan.destination);
}

/** @copydoc ReleaseUnsafeSpaceMiningAttachments */
uint32_t ReleaseUnsafeSpaceMiningAttachments()
{
	uint32_t released = 0;
	// Replan each stable participant after its predecessor commits. No batch of
	// stale endpoints can allocate the same point, and no second berth registry
	// is needed: the original contained mission supplies pending occupancy.
	for (const auto reference : GatherSpaceMiningDepartureParticipants()) {
		const auto proposal = PlanSpaceBodyDeparture(reference);
		if (proposal && BeginSpaceBodyDeparture(*proposal)) ++released;
	}
	return released;
}
