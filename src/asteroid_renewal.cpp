/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_renewal.cpp Canonical single-ticket finite body and inward boundary proposals.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group27; publication/credit ownership remains Group28.
 */
#include "stdafx.h"
#include "asteroid_renewal.h"
#include "multilayer_asteroid_catalogue.h"
#include "space_vehicle.h"
#include "space_work_cost.h"
#include "safeguards.h"

/** @param motion Unissued canonical body centre/velocity. @param bounds Actual physical plane. @return Whether every boundary touched is entered strictly inward, including both axes at a corner. */
static bool IsInwardAsteroidEntry(const SpaceMotion &motion, const SpacePlaneBounds &bounds)
{
	const auto point = motion.position;
	// A corner must enter both touched edges. A tangential component there
	// cannot leave a replacement permanently stranded on an unusable edge.
	return bounds.Contains(point) &&
		(point.x != 0 || motion.velocity_x > 0) && (point.x != bounds.max.x || motion.velocity_x < 0) &&
		(point.y != 0 || motion.velocity_y > 0) && (point.y != bounds.max.y || motion.velocity_y < 0);
}

/** @param body Unissued proposed finite body. @param field Original live bodies. @param bounds Actual plane. @param obstacles Original static geometry. @param craft_positions Original physical hull centres. @return Whether its centre and first swept inward step respect existing native geometry/body clearance without acquiring a lease. */
static bool IsAsteroidEntryClear(const AsteroidBody &body, const AsteroidField &field, const SpacePlaneBounds &bounds,
	std::span<const SpaceAttachmentObstacle> obstacles, std::span<const SpacePoint> craft_positions)
{
	const auto step = StepSpaceMotionStraight(body.motion, bounds);
	// A clear endpoint cannot authorize an immediate corner exit or a first
	// segment crossing a real static footprint/approach. No attachment is granted.
	if (!IsInwardAsteroidEntry(body.motion, bounds) || !step || step->crossed_edges.Any() ||
		!IsSpaceMovementSegmentClear(body.motion.position, step->proposed.position, bounds, obstacles)) return false;
	const auto clearance = GetSpaceBodyApproachClearance(); const uint64_t squared = static_cast<uint64_t>(clearance) * clearance;
	// The same original body envelope protects native waiting/approach centres.
	// Existing resources and actual hulls cannot be displaced to make an entry.
	for (const auto &other : field.bodies) if (SpaceDistanceSquared(body.motion.position, other.motion.position) < squared) return false;
	for (const auto point : craft_positions) if (SpaceDistanceSquared(body.motion.position, point) < squared) return false;
	return true;
}

/** @copydoc PlanAsteroidRenewal */
AsteroidRenewalProposal PlanAsteroidRenewal(const AsteroidField &field, std::span<const SpaceAttachmentObstacle> obstacles,
	std::span<const SpacePoint> craft_positions)
{
	AsteroidRenewalProposal proposal;
	const auto bounds = GetSpacePlaneBounds(field.width, field.height);
	// A copied input supplies all authority. Malformed geometry or an invalid
	// original ledger cannot borrow another world's bounds or pending credits.
	if (!bounds || craft_positions.size() > MAX_SPACE_VEHICLES || !ValidateAsteroidField(field)) return proposal;
	// Actual hull snapshots must already be bounded. Refuse malformed copied
	// occupancy rather than ignoring an outside point or enlarging the fleet cap.
	for (const auto point : craft_positions) if (!bounds->Contains(point)) return proposal;
	proposal.source_renewal = field.renewal; proposal.source_next_body_id = field.next_body_id;
	proposal.attempt = BeginAsteroidRenewalAttempt(field.renewal, field.next_body_id);
	if (!proposal.attempt) { proposal.status = AsteroidRenewalProposalStatus::NotDue; return proposal; }
	if (!proposal.attempt->size) { proposal.status = AsteroidRenewalProposalStatus::NoEligibleSize; return proposal; }
	Randomizer local; local.SetSeed(proposal.attempt->ticket);
	AsteroidBody body; body.id = INVALID_ORBITAL_BODY; body.size = *proposal.attempt->size;
	uint32_t percentile = local.Next(100);
	// Dynamic creation samples the canonical class weights. Exact initial class
	// apportionment is generation history, never a refill/population target.
	for (const auto &share : GetMultiLayerAsteroidClassShares(MultiLayerGameplayProfileID::MultilayerIndustry)) {
		if (percentile < share.percent) { body.resource_class = static_cast<AsteroidClass>(share.class_index); break; }
		percentile -= share.percent;
	}
	body.initial_reserve = body.remaining_reserve = SampleMultiLayerAsteroidReserve(
		GetMultiLayerAsteroidDefinitions()[to_underlying(body.resource_class)], GetMultiLayerAsteroidSizeDefinitions()[to_underlying(body.size)], local);
	const auto headings = GetMultiLayerAsteroidHeadingDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto speeds = GetMultiLayerAsteroidSpeedDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto heading = headings[local.Next(static_cast<uint32_t>(headings.size()))];
	const int32_t speed = speeds[local.Next(static_cast<uint32_t>(speeds.size()))].distance_q16_per_day;
	body.motion.velocity_x = static_cast<int64_t>(speed) * heading.x_q16 / SPACE_Q16;
	body.motion.velocity_y = static_cast<int64_t>(speed) * heading.y_q16 / SPACE_Q16;
	std::array<SpacePlaneEdge, 2> sides{}; uint8_t side_count = 0;
	// Canonical cardinal/diagonal headings have one or two inward entry sides.
	// Choosing only those sides avoids random retries for outward/tangent draws.
	if (body.motion.velocity_x > 0) sides[side_count++] = SpacePlaneEdge::West;
	if (body.motion.velocity_x < 0) sides[side_count++] = SpacePlaneEdge::East;
	if (body.motion.velocity_y > 0) sides[side_count++] = SpacePlaneEdge::North;
	if (body.motion.velocity_y < 0) sides[side_count++] = SpacePlaneEdge::South;
	const auto first_side = local.Next(side_count); const auto coordinate_ticket = local.Next();
	// Consume the fixed local sequence before checking occupancy. Test at most
	// eight evenly separated coordinates on each inward side, rotating only by
	// the sampled side/coordinate. Blocked alternatives draw no new randomness.
	for (uint8_t side_offset = 0; side_offset < side_count; ++side_offset) {
		const auto side = sides[(first_side + side_offset) % side_count];
		const bool vertical = side == SpacePlaneEdge::West || side == SpacePlaneEdge::East;
		const uint32_t extent = vertical ? field.height : field.width; const auto start = ScaleToLimit(coordinate_ticket, extent);
		for (uint8_t alternative = 0; alternative < 8; ++alternative) {
			const int32_t coordinate = static_cast<int32_t>((start + uint32_t{alternative} * (extent / 8)) % extent) * SPACE_Q16;
			body.motion.position = vertical ? SpacePoint{side == SpacePlaneEdge::West ? 0 : bounds->max.x, coordinate} :
				SpacePoint{coordinate, side == SpacePlaneEdge::North ? 0 : bounds->max.y};
			++proposal.candidates_examined;
			RecordSpaceWork(SpaceWorkKind::RenewalEntryCandidates);
			if (!IsAsteroidEntryClear(body, field, *bounds, obstacles, craft_positions)) continue;
			proposal.status = AsteroidRenewalProposalStatus::Ready; proposal.body = body; return proposal;
		}
	}
	proposal.status = AsteroidRenewalProposalStatus::NoClearEntry;
	return proposal;
}
