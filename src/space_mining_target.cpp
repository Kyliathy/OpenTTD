/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_target.cpp Complete distance ordering before native mining/approach eligibility.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group18.
 * Queries own no reserve, packet, request, lease or alternative body registry. Event notification only wakes existing mission-owned retry state.
 */
#include "stdafx.h"
#include "space_mining_target.h"
#include "space_vehicle.h"
#include "space_approach.h"
#include "space_mining_tuning.h"
#include "map_func.h"
#include "space_work_cost.h"
#include "safeguards.h"

/** @copydoc ForecastSpaceMiningIntercept */
std::optional<SpaceMiningInterceptForecast> ForecastSpaceMiningIntercept(SpacePoint position, uint8_t remainder, uint16_t model,
	const AsteroidBody &body, const SpacePlaneBounds &bounds, std::span<const SpaceAttachmentObstacle> obstacles, std::optional<SpacePoint> allowed_start_berth)
{
	RecordSpaceWork(SpaceWorkKind::InterceptForecasts);
	// Only an original mining role with canonical copied motion/position may
	// borrow the native pursuit clock; no role, cargo or lease is created here.
	if (GetSpaceMiningTuning(model) == nullptr || remainder >= GetMultiLayerSpaceTuning().ticks_per_economy_day ||
		!IsSpaceMovementSegmentClear(position, position, bounds, obstacles, allowed_start_berth)) return std::nullopt;
	const auto initial = GetSafeSpaceBodyApproachPoint(body.motion.position, bounds, obstacles);
	const auto exit = ForecastAsteroidExit(body, bounds);
	if (!initial || !exit) return std::nullopt;
	const auto safety = ForecastSpaceAttachmentSafety(body.motion, *exit, {SPACE_Q16, -SPACE_Q16 / 2}, bounds, obstacles);
	if (!safety) return std::nullopt;
	const auto deadline = GetSpaceMiningApproachTicks(position, *initial);
	const uint64_t tolerance = GetMultiLayerMiningProbeTuning().approach_tolerance_q16;
	const uint64_t clearance = GetSpaceBodyApproachClearance();
	const uint64_t day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	const uint64_t craft_step = (uint64_t{GetMultiLayerMiningProbeTuning().travel_tiles_per_day} * SPACE_Q16 + day - 1) / day;
	uint64_t earliest_arrival = 1;
	// Even perfect closing on each axis cannot beat this lower bound. Refuse
	// obviously too-late bodies before walking a long pursuit; rounding upward
	// on both step capacities deliberately overestimates possible closing speed.
	for (const auto [delta, velocity] : std::array<std::pair<int64_t, int64_t>, 2>{{
		{int64_t{initial->x} - position.x, body.motion.velocity_x}, {int64_t{initial->y} - position.y, body.motion.velocity_y}}}) {
		const uint64_t gap = static_cast<uint64_t>(std::abs(delta)) > tolerance ? static_cast<uint64_t>(std::abs(delta)) - tolerance : 0;
		const uint64_t closing = craft_step + (static_cast<uint64_t>(std::abs(velocity)) + day - 1) / day;
		earliest_arrival = std::max(earliest_arrival, (gap + closing - 1) / closing);
	}
	// A legal departure must move outside the two-tile envelope, requiring at
	// least one further opportunity. This bound filters only impossible timings.
	if (safety->first_unsafe_tick && *safety->first_unsafe_tick <= earliest_arrival + 1) return std::nullopt;
	auto motion = body.motion;
	// Walk only this candidate's bounded original pursuit deadline. The first
	// opportunity sees the already-moved field; subsequent opportunities advance
	// copied straight motion first. Arrival can occur far before the minimum
	// deadline, preserving genuinely reachable bodies near a physical edge.
	for (uint32_t tick = 1; tick <= deadline; ++tick) {
		if (safety->first_unsafe_tick && *safety->first_unsafe_tick <= tick) return std::nullopt;
		const auto attachment = GetSafeSpaceBodyApproachPoint(motion.position, bounds, obstacles);
		if (!attachment) return std::nullopt;
		const auto previous = position;
		MoveSpaceSurveyTowards(position, *attachment, remainder);
		if (!IsSpaceMovementSegmentClear(previous, position, bounds, obstacles, tick == 1 ? allowed_start_berth : std::nullopt)) return std::nullopt;
		if (SpaceDistanceSquared(position, *attachment) <= tolerance * tolerance) {
			if (!IsSpaceMovementSegmentClear(position, *attachment, bounds, obstacles)) return std::nullopt;
			// Reuse the native bounded staging ring order and swept local mover.
			// This copied endpoint clears the target's own envelope and statics;
			// actual native craft/body occupancy is checked again before departure.
			for (int radius = 1; radius <= SPACE_STAGING_MAX_RADIUS; ++radius) for (const auto [dx, dy] : SPACE_STAGING_DIRECTIONS) {
				const SpacePoint candidate{attachment->x + dx * radius * SPACE_Q16, attachment->y + dy * radius * SPACE_Q16};
				if (SpaceDistanceSquared(candidate, motion.position) < clearance * clearance) continue;
				const auto travel = GetSpaceDepartureTravelTicks(*attachment, candidate, remainder, bounds, obstacles);
				if (!travel) continue;
				// Attachment must leave one extra release opportunity beyond actual
				// departure travel. A safe arrival alone cannot authorize a doomed lease.
				if (safety->first_unsafe_tick && *safety->first_unsafe_tick <= uint64_t{tick} + *travel) continue;
				return SpaceMiningInterceptForecast{tick, motion, remainder, candidate, *travel};
			}
			return std::nullopt;
		}
		const auto step = StepSpaceMotionStraight(motion, bounds);
		if (!step || step->crossed_edges.Any()) return std::nullopt;
		motion = step->proposed;
	}
	return std::nullopt;
}

/** @copydoc GetSpaceMiningTargetRetryDelay */
uint16_t GetSpaceMiningTargetRetryDelay() { return GetMultiLayerSpaceTuning().ticks_per_economy_day; }

/** @copydoc NotifySpaceMiningTargetChange */
void NotifySpaceMiningTargetChange(CargoLabel mineral)
{
	if (mineral.Empty()) return;
	// Native fleet capacity bounds event work. Wake only the original active
	// FullLoadNearest Mine waiting for this mineral; another action, receiver
	// wait or service obligation cannot be restarted by a resource event.
	for (auto *craft : SpaceVehicle::Iterate()) {
		auto &intent = craft->mission.continuation;
		const auto &queue = craft->space_orders;
		if (intent.phase != MiningContinuationPhase::Waiting || intent.waiting != MiningContinuationWait::NoCompatibleAsteroid ||
			intent.mineral != mineral || queue.cursor >= queue.entries.size() || queue.entries[queue.cursor].action != SpaceOrderAction::Mine ||
			queue.entries[queue.cursor].mining_policy != SpaceMiningPolicy::FullLoadNearest) continue;
		intent.retry_ticks = 0;
	}
}

/** @copydoc RankMiningTargetCandidates */
std::vector<OrbitalBodyID> RankMiningTargetCandidates(std::span<const AsteroidBody> bodies, SpacePoint position, CargoLabel mineral)
{
	std::vector<std::pair<uint64_t, OrbitalBodyID>> keys;
	if (mineral.Empty()) return {};
	const auto &definitions = GetMultiLayerAsteroidDefinitions();
	// Every live matching body participates before exact sorting. Storage order,
	// old dense counts and arbitrary nearest-N cutoffs cannot hide a better ID.
	for (const auto &body : bodies) {
		const auto resource = to_underlying(body.resource_class);
		if (resource >= definitions.size() || body.remaining_reserve == 0 || definitions[resource].cargo != mineral) continue;
		keys.emplace_back(SpaceDistanceSquared(position, body.motion.position), body.id);
	}
	std::ranges::sort(keys);
	std::vector<OrbitalBodyID> result; result.reserve(keys.size());
	for (const auto &[distance, id] : keys) { (void)distance; result.push_back(id); }
	return result;
}

/** @copydoc FindNearestEligibleMiningTarget */
std::optional<OrbitalBodyID> FindNearestEligibleMiningTarget(SpaceVehicle &craft, CargoLabel mineral)
{
	RecordSpaceWork(SpaceWorkKind::MiningSelections);
	// A caller cannot substitute another resource for the mission's already
	// pinned intent, including while the original freight hold happens to be empty.
	if (mineral.Empty() || (!craft.mission.continuation.mineral.Empty() && craft.mission.continuation.mineral != mineral)) return std::nullopt;
	const auto candidates = RankMiningTargetCandidates(GetAsteroidField().bodies, craft.position, mineral);
	if (candidates.empty()) return std::nullopt;
	const auto bounds = GetSpacePlaneBounds(Map::SizeX(), Map::SizeY());
	const auto obstacles = GetSpaceAttachmentObstacles();
	if (!bounds || !obstacles) return std::nullopt;
	std::optional<SpacePoint> source_berth;
	// A real retained static admission may authorize departure from its own
	// starting point. Selection cannot borrow another berth or move a temporary
	// hull to fabricate this proof; body approaches confer no such exception.
	if (craft.space_orders.approach && craft.space_orders.approach->target.kind != SpaceOrderTargetKind::Body) {
		SpacePoint admitted;
		if (GetSpaceApproachDestination(craft, admitted) == SpaceOrderBlock::None && admitted == craft.position) source_berth = admitted;
	}
	// Ordered traversal may stop only at a genuinely admitted candidate. Every
	// closer candidate first passes the same native hold, role, service and shared
	// slot checks used at assignment; querying neither saves requests nor starts work.
	for (const auto id : candidates) {
		if (AssignSpaceMiningMission(craft, id.base(), false, SpaceMiningPolicy::FullLoadNearest).Failed()) continue;
		const auto *body = FindAsteroidBody(GetAsteroidField(), id);
		// The exact body must still supply enough time for native interception and
		// a bounded clear departure; a nearer soon-exiting body is not eligible.
		if (body == nullptr || !ForecastSpaceMiningIntercept(craft.position, craft.mission.travel_remainder,
			GetSpaceVehicleModel(craft), *body, *bounds, *obstacles, source_berth)) continue;
		return id;
	}
	return std::nullopt;
}
