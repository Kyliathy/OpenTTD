/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_approach.cpp Stable shared approach admission over actual native hulls and targets.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups10-10C/25B; shared clearance, native departure and contained endpoint occupancy.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group21; opt-in operation diagnostics never govern simulation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group11; shared homeward mining departure.
 */
#include "stdafx.h"
#include "space_work_cost.h"
#include "space_approach.h"
#include "space_departure.h"
#include "space_vehicle.h"
#include "space_mining_continuation.h"
#include "space_transition.h"
#include "space_structure.h"
#include "space_manifest.h"
#include "orbital_service.h"
#include "asteroid.h"
#include "map_func.h"
#include "company_base.h"
#include "openttd.h"
#include "timer/timer_game_tick.h"
#include <set>
#include <tuple>
#include "safeguards.h"

static constexpr int32_t BODY_APPROACH_CLEARANCE = 2 * SPACE_Q16; ///< Original shared body/attachment waiting envelope, also governing mining departure.

/** @copydoc GetSpaceBodyApproachClearance */
int32_t GetSpaceBodyApproachClearance() { return BODY_APPROACH_CLEARANCE; }

/** @param obstacle Detached footprint or exact approach point. @return Whether its inclusive geometry has the declared original meaning. */
static bool ValidAttachmentObstacle(const SpaceAttachmentObstacle &obstacle)
{
	// An inverted footprint or non-point berth must not quietly disappear from
	// a safety query and authorize passage through an accountable static owner.
	return obstacle.kind < SpaceAttachmentObstacleKind::End && obstacle.min.x <= obstacle.max.x && obstacle.min.y <= obstacle.max.y &&
		(obstacle.kind != SpaceAttachmentObstacleKind::Approach || obstacle.min == obstacle.max);
}

/** @copydoc IsSpaceAttachmentPointClear */
bool IsSpaceAttachmentPointClear(SpacePoint point, const SpacePlaneBounds &bounds, std::span<const SpaceAttachmentObstacle> obstacles)
{
	// Both the plane definition and proposed point must be valid; a malformed
	// copied boundary cannot authorize a staging or attachment slot.
	if (!bounds.IsValid() || !bounds.Contains(point)) return false;
	// Static clearance includes all original owners, not just the craft's company;
	// a projected body overlap cannot displace or borrow a module's paid berth.
	for (const auto &obstacle : obstacles) {
		if (!ValidAttachmentObstacle(obstacle)) return false;
		if (point.x >= obstacle.min.x && point.x <= obstacle.max.x && point.y >= obstacle.min.y && point.y <= obstacle.max.y) return false;
	}
	return true;
}

/** @copydoc GetSafeSpaceBodyApproachPoint */
std::optional<SpacePoint> GetSafeSpaceBodyApproachPoint(SpacePoint body_position, const SpacePlaneBounds &bounds,
	std::span<const SpaceAttachmentObstacle> obstacles)
{
	// A retired/outside body cannot supply a new approach, even if its translated
	// point happens to lie inside the map. Validate before adding the fixed offset.
	if (!bounds.IsValid() || !bounds.Contains(body_position)) return std::nullopt;
	const SpacePoint point{body_position.x + SPACE_Q16, body_position.y - SPACE_Q16 / 2};
	if (!IsSpaceAttachmentPointClear(point, bounds, obstacles)) return std::nullopt;
	return point;
}

/**
 * Solve the inclusive time interval during which one signed integer axis occupies an obstacle range.
 * @param position Original craft coordinate.
 * @param velocity Original signed body component; inequalities are mirrored without changing its value.
 * @param remainder Canonical unsigned fractional travel.
 * @param low Inclusive obstacle minimum.
 * @param high Inclusive obstacle maximum.
 * @param horizon Last opportunity worth inspecting, bounded by the body's first exit.
 * @return Intersection interval, or absence when this axis never reaches the range in time.
 */
static std::optional<std::pair<uint64_t, uint64_t>> AttachmentAxisInterval(int32_t position, int32_t velocity, uint8_t remainder,
	int32_t low, int32_t high, uint64_t horizon)
{
	if (velocity == 0) {
		// A stationary component either overlaps for the entire horizon or can
		// never enter; it does not create a fictitious zero-speed arrival time.
		if (position < low || position > high) return std::nullopt;
		return std::pair<uint64_t, uint64_t>{0, horizon};
	}
	const int64_t start = velocity < 0 ? -int64_t{position} : position;
	const int64_t minimum = velocity < 0 ? -int64_t{high} : low;
	const int64_t maximum = velocity < 0 ? -int64_t{low} : high;
	if (maximum < start) return std::nullopt;
	const uint64_t magnitude = static_cast<uint64_t>(std::abs(int64_t{velocity}));
	const uint64_t day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	uint64_t first = 0;
	if (minimum > start) {
		const uint64_t numerator = static_cast<uint64_t>(minimum - start) * day - remainder;
		first = numerator / magnitude + (numerator % magnitude != 0);
	}
	const uint64_t numerator = static_cast<uint64_t>(maximum - start + 1) * day - 1 - remainder;
	const uint64_t last = std::min(horizon, numerator / magnitude);
	if (first > last) return std::nullopt;
	return std::pair{first, last};
}

/** @copydoc ForecastSpaceAttachmentSafety */
std::optional<SpaceAttachmentSafety> ForecastSpaceAttachmentSafety(const SpaceMotion &motion, const SpaceMotionExitForecast &body_forecast,
	SpacePoint offset, const SpacePlaneBounds &bounds, std::span<const SpaceAttachmentObstacle> obstacles)
{
	const auto original = ForecastSpaceMotionExit(motion, bounds);
	// The copied forecast must describe this exact body/plane. A stale forecast
	// from another candidate cannot extend attachment rights past its own exit.
	if (!original || original->ticks_until_exit != body_forecast.ticks_until_exit || original->last_safe_motion != body_forecast.last_safe_motion ||
		original->crossed_edges != body_forecast.crossed_edges) return std::nullopt;
	// Reject the whole copied obstacle set before reporting a usable forecast;
	// a later malformed berth must not be hidden by an earlier easy hazard.
	for (const auto &obstacle : obstacles) if (!ValidAttachmentObstacle(obstacle)) return std::nullopt;
	const int64_t x = int64_t{motion.position.x} + offset.x, y = int64_t{motion.position.y} + offset.y;
	// Widen before adding the offset. An already unsafe point has no invented
	// last-safe position and cannot be repaired by clamping or teleportation.
	if (x < 0 || y < 0 || x > bounds.max.x || y > bounds.max.y) return SpaceAttachmentSafety{uint64_t{0}, std::nullopt, SpaceAttachmentHazard::MapEdge};
	auto craft = motion;
	craft.position = {static_cast<int32_t>(x), static_cast<int32_t>(y)};
	const auto craft_exit = ForecastSpaceMotionExit(craft, bounds);
	assert(craft_exit.has_value());
	SpaceAttachmentSafety result{body_forecast.ticks_until_exit, craft.position,
		body_forecast.ticks_until_exit ? SpaceAttachmentHazard::BodyExit : SpaceAttachmentHazard::None};
	// An outward attachment offset can become unsafe before its body leaves.
	// Equal times prefer the concrete craft edge over the body's retirement.
	if (craft_exit->ticks_until_exit && (!result.first_unsafe_tick || *craft_exit->ticks_until_exit <= *result.first_unsafe_tick)) {
		result.first_unsafe_tick = craft_exit->ticks_until_exit;
		result.hazard = SpaceAttachmentHazard::MapEdge;
	}
	const uint64_t horizon = body_forecast.ticks_until_exit.value_or(UINT64_MAX);
	// Solve each static rectangle/point analytically; runtime scales with actual
	// geometry, never with a slow body's millions of future simulation ticks.
	for (const auto &obstacle : obstacles) {
		const auto tx = AttachmentAxisInterval(craft.position.x, craft.velocity_x, craft.remainder_x, obstacle.min.x, obstacle.max.x, horizon);
		const auto ty = AttachmentAxisInterval(craft.position.y, craft.velocity_y, craft.remainder_y, obstacle.min.y, obstacle.max.y, horizon);
		if (!tx || !ty) continue;
		const uint64_t first = std::max(tx->first, ty->first), last = std::min(tx->second, ty->second);
		const auto hazard = obstacle.kind == SpaceAttachmentObstacleKind::Footprint ? SpaceAttachmentHazard::Footprint : SpaceAttachmentHazard::Approach;
		// Both axes must occupy the obstacle on the same native opportunity;
		// crossing its X and Y ranges at unrelated times is not a collision. Equal
		// static hazards prefer the same enum rank regardless of input enumeration.
		if (first > last || (result.first_unsafe_tick && (first > *result.first_unsafe_tick ||
			(first == *result.first_unsafe_tick && to_underlying(hazard) >= to_underlying(result.hazard))))) continue;
		result.first_unsafe_tick = first;
		result.hazard = hazard;
	}
	if (!result.first_unsafe_tick) return result;
	if (*result.first_unsafe_tick == 0) { result.last_safe_point.reset(); return result; }
	const auto last = ProjectSpaceMotionStraight(craft, bounds, *result.first_unsafe_tick - 1);
	assert(last.has_value() && last->crossed_edges.None());
	result.last_safe_point = last->proposed.position;
	return result;
}

/** @param target Typed requested destination. @return Its shared physical approach domain. */
static SpaceOrderTarget NormalizeApproachTarget(SpaceOrderTarget target)
{
	if (target.kind == SpaceOrderTargetKind::Blueprint) target.kind = SpaceOrderTargetKind::Structure;
	return target;
}

/** @param target Normalized target. @return Whether its native identity can name a supported approach producer. */
static bool ValidApproachTarget(const SpaceOrderTarget &target)
{
	if (target.id == UINT32_MAX) return false;
	if (target.kind == SpaceOrderTargetKind::Body) return target.generation == 0;
	return target.kind == SpaceOrderTargetKind::Structure && target.id < SpaceStructureID::End().base() && target.generation != 0;
}

/** @param craft Actual native hull. @param target Exact normalized destination.
 * @return Whether an orbital hull or this exact in-Space module flight may own the one shared request.
 */
static bool CanOwnSpaceApproach(const SpaceVehicle &craft, const SpaceOrderTarget &target)
{
	if (craft.construction.placement == SpacePlacement::Orbital) return true;
	// A rocket's InTransit placement is preserved throughout flight; only its unresolved Space/module arrival can borrow shared admission, never Surface preparation or recovery.
	return craft.construction.placement == SpacePlacement::InTransit && GetSpaceVehicleModel(craft) == 14 && craft.transition && IsSpaceModuleFreightApproach(*craft.transition, target);
}

/** @copydoc ValidateSpaceApproachRecords */
bool ValidateSpaceApproachRecords(std::span<const SpaceOrderRecord> records, uint64_t native_tick, uint32_t width, uint32_t height)
{
	std::set<std::tuple<SpaceOrderTargetKind, uint32_t, uint64_t, uint8_t>> occupied;
	// Every actual hull has at most one contained request; admitted target slots must have one exclusive saved owner.
	for (const auto &row : records) {
		if (!row.queue.approach) continue;
		const auto &r = *row.queue.approach;
		// Clock and coordinate bounds prevent a future-priority request or imported staging point becoming actionable after load.
		if (!ValidApproachTarget(r.target) || r.request_tick > native_tick || r.staging.x < 0 || r.staging.y < 0 ||
			r.staging.x > 4095 * SPACE_Q16 || r.staging.y > 4095 * SPACE_Q16 ||
			(width != 0 && r.staging.x > int64_t{width - 1} * SPACE_Q16) || (height != 0 && r.staging.y > int64_t{height - 1} * SPACE_Q16)) return false;
		if (r.slot == UINT8_MAX) continue;
		// Bodies and ordinary sites have one approach; an actual Dock may provide the second bounded slot after activation.
		if (r.slot > 1 || (r.target.kind == SpaceOrderTargetKind::Body && r.slot != 0) ||
			!occupied.emplace(r.target.kind, r.target.id, r.target.generation, r.slot).second) return false;
	}
	return true;
}

/** @copydoc PlanSpaceApproaches */
std::optional<std::vector<SpaceApproachRecord>> PlanSpaceApproaches(std::span<const SpaceApproachRecord> requests, std::span<const SpaceApproachSlot> slots)
{
	// Bound both copied request and derived-slot work by the published native fleet capacity.
	if (requests.size() > MAX_SPACE_VEHICLES || slots.size() > 2 * MAX_SPACE_VEHICLES) return std::nullopt;
	std::vector<SpaceApproachRecord> result(requests.begin(), requests.end());
	RecordSpaceWork(SpaceWorkKind::PlannerRequests, requests.size()); RecordSpaceWork(SpaceWorkKind::PlannerSlots, slots.size());
	std::set<VehicleID> crafts;
	std::set<std::tuple<SpaceOrderTargetKind, uint32_t, uint64_t, uint8_t>> definitions, retained;
	// Reject ambiguous physical slot definitions before considering any caller's priority or occupancy.
	for (const auto &s : slots) {
		if (!ValidApproachTarget(s.target) || s.slot > 1 || (s.target.kind == SpaceOrderTargetKind::Body && s.slot != 0) ||
			!definitions.emplace(s.target.kind, s.target.id, s.target.generation, s.slot).second) return std::nullopt;
	}
	// Surviving admissions retain their own slots; a later request cannot preempt a craft already approaching or being handled.
	for (auto &r : result) {
		if (r.craft == VehicleID::Invalid() || r.generation == 0 || !crafts.insert(r.craft).second || !ValidApproachTarget(r.request.target)) return std::nullopt;
		if (r.request.slot == UINT8_MAX) continue;
		const auto key = std::tuple{r.request.target.kind, r.request.target.id, r.request.target.generation, r.request.slot};
		if (!retained.insert(key).second) return std::nullopt;
		/** @param s Original physical slot. @return Whether its full domain/slot identity matches this existing request. */
		const auto found = std::ranges::find_if(slots, [&](const auto &s) { RecordSpaceWork(SpaceWorkKind::PlannerSlotTests); return s.target == r.request.target && s.slot == r.request.slot; });
		// Missing slots and physical foreign occupants block admission without moving either hull or replacing an actual body lease.
		if (found == slots.end() || found->blocked || (found->occupant != VehicleID::Invalid() && found->occupant != r.craft)) r.request.slot = UINT8_MAX;
	}
	std::vector<size_t> order(result.size());
	for (size_t i = 0; i < order.size(); ++i) order[i] = i;
	/** @param a First original request index. @param b Second original request index. @return Whether stable native tick/ID priority admits the first earlier. */
	std::ranges::sort(order, [&](size_t a, size_t b) {
		RecordSpaceWork(SpaceWorkKind::PlannerOrderTests);
		return std::tuple{result[a].request.request_tick, result[a].craft.base()} < std::tuple{result[b].request.request_tick, result[b].craft.base()};
	});
	// Older requests win first, with native hull ID resolving simultaneous arrivals independently of input iteration order.
	for (const auto index : order) {
		auto &r = result[index];
		if (r.request.slot != UINT8_MAX) continue;
		// Choose the lowest free physical slot, so derived slot enumeration cannot change deterministic admission.
		const SpaceApproachSlot *best = nullptr;
		for (const auto &s : slots) {
			RecordSpaceWork(SpaceWorkKind::PlannerSlotTests);
			if (s.target != r.request.target || s.blocked || (s.occupant != VehicleID::Invalid() && s.occupant != r.craft)) continue;
			/** @param other Previously admitted original request. @return Whether that exact shared slot already has an owner. */
			const bool used = std::ranges::any_of(result, [&](const auto &other) { RecordSpaceWork(SpaceWorkKind::PlannerOwnerTests); return other.request.target == s.target && other.request.slot == s.slot; });
			if (!used && (best == nullptr || s.slot < best->slot)) best = &s;
		}
		if (best != nullptr) r.request.slot = best->slot;
	}
	return result;
}

/** @copydoc GetSpaceStructureApproachCapacity */
uint8_t GetSpaceStructureApproachCapacity(const SpaceStructure &site)
{
	// Paid activated Dock geometry retains both accountable berths during utility/dependency outages; work admission still requires operational services.
	return site.state.kind == SpaceBuildKind::Dock && site.state.phase == SpaceStructurePhase::FrameComplete &&
		site.module != nullptr && site.module->activation_ticks == 0 ? GetOrbitalModuleServiceCapacity(site.state.kind) : 1;
}

/** @copydoc GetSpaceStructureApproachSlot */
std::optional<SpacePoint> GetSpaceStructureApproachSlot(const SpaceStructure &site, uint8_t slot)
{
	if (slot >= GetSpaceStructureApproachCapacity(site)) return std::nullopt;
	auto point = GetSpaceStructureApproach(site.state);
	// Two precise Dock berths fit inside its already reserved south approach cell, preserving retained inert blueprint geometry.
	point.x += int32_t{slot} * (SPACE_Q16 / 2);
	return point;
}

/** @copydoc GetSpaceAttachmentObstacles */
std::optional<std::vector<SpaceAttachmentObstacle>> GetSpaceAttachmentObstacles()
{
	std::vector<SpaceAttachmentObstacle> result;
	// Copy the original physical footprint and each actual paid approach slot;
	// cancelled retained records keep the same clearance as IsSafeStaging until
	// their native owner is removed. This vector owns no occupancy or reservation.
	for (const auto *site : SpaceStructure::Iterate()) {
		const auto *definition = GetSpaceConstructionDefinition(site->state.kind);
		// Refuse impossible native footprints before signed Q16 conversion;
		// clearance must not wrap malformed coordinates into the current plane.
		if (definition == nullptr || definition->width == 0 || definition->height == 0 ||
			uint32_t{site->state.x} + definition->width > Map::SizeX() || uint32_t{site->state.y} + definition->height >= Map::SizeY()) return std::nullopt;
		const SpacePoint minimum{int32_t{site->state.x} * SPACE_Q16, int32_t{site->state.y} * SPACE_Q16};
		const SpacePoint maximum{minimum.x + definition->width * SPACE_Q16 - 1, minimum.y + definition->height * SPACE_Q16 - 1};
		result.push_back({minimum, maximum, SpaceAttachmentObstacleKind::Footprint});
		for (uint8_t slot = 0; slot < GetSpaceStructureApproachCapacity(*site); ++slot) {
			const auto point = GetSpaceStructureApproachSlot(*site, slot);
			if (!point) return std::nullopt;
			result.push_back({*point, *point, SpaceAttachmentObstacleKind::Approach});
		}
	}
	return result;
}

/** Exact bounded segment parameter; denominators are positive native step magnitudes. */
struct DepartureRatio {
	int64_t numerator = 0, denominator = 1;
	/** @param other Segment bound to compare. @return Strict order without floating-point rounding of a berth intersection. */
	bool operator<(const DepartureRatio &other) const { return this->numerator * other.denominator < other.numerator * this->denominator; }
};

/** @param from Current proposed path point. @param to Next native movement point. @param obstacle Original static rectangle or exact berth. @return Whether the swept segment touches that original geometry. */
static bool DepartureSegmentTouches(SpacePoint from, SpacePoint to, const SpaceAttachmentObstacle &obstacle)
{
	DepartureRatio enter{0, 1}, leave{1, 1};
	// Clip the segment against each inclusive static axis. Cross products remain
	// bounded by native step magnitudes and 32-bit coordinates, protecting even
	// an exact berth that lies between two rendered/ticked craft positions.
	for (const auto [start, finish, low, high] : std::array<std::array<int32_t, 4>, 2>{{
		{from.x, to.x, obstacle.min.x, obstacle.max.x}, {from.y, to.y, obstacle.min.y, obstacle.max.y}}}) {
		const int64_t delta = int64_t{finish} - start;
		if (delta == 0) {
			if (start < low || start > high) return false;
			continue;
		}
		const DepartureRatio first{delta > 0 ? int64_t{low} - start : int64_t{start} - high, std::abs(delta)};
		const DepartureRatio last{delta > 0 ? int64_t{high} - start : int64_t{start} - low, std::abs(delta)};
		if (enter < first) enter = first;
		if (last < leave) leave = last;
		if (leave < enter) return false;
	}
	return true;
}

/** @copydoc IsSpaceMovementSegmentClear */
bool IsSpaceMovementSegmentClear(SpacePoint from, SpacePoint to, const SpacePlaneBounds &bounds, std::span<const SpaceAttachmentObstacle> obstacles, std::optional<SpacePoint> allowed_start_berth)
{
	// Copied malformed geometry cannot grant a path. Convex plane bounds make
	// both endpoints sufficient for the plane, but every static segment is swept.
	if (!bounds.IsValid() || !bounds.Contains(from) || !bounds.Contains(to) || (allowed_start_berth && *allowed_start_berth != from)) return false;
	for (const auto &obstacle : obstacles) {
		if (!ValidAttachmentObstacle(obstacle)) return false;
		// Leaving one's independently admitted precise berth is legal. This
		// exception belongs only to the starting endpoint and grants no passage
		// through its module footprint or any other berth along the segment.
		if (allowed_start_berth && obstacle.kind == SpaceAttachmentObstacleKind::Approach && obstacle.min == from) continue;
		if (DepartureSegmentTouches(from, to, obstacle)) return false;
	}
	return true;
}

/** @copydoc GetSpaceDepartureTravelTicks */
std::optional<uint32_t> GetSpaceDepartureTravelTicks(SpacePoint origin, SpacePoint destination, uint8_t remainder,
	const SpacePlaneBounds &bounds, std::span<const SpaceAttachmentObstacle> obstacles)
{
	const auto day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	const uint64_t dx = static_cast<uint64_t>(std::abs(int64_t{destination.x} - origin.x));
	const uint64_t dy = static_cast<uint64_t>(std::abs(int64_t{destination.y} - origin.y));
	// Departure is a local bounded route, not a second long-distance pathfinder.
	// Both endpoints and every static definition must be valid before any walk.
	if (remainder >= day || dx > SPACE_STAGING_MAX_RADIUS * SPACE_Q16 || dy > SPACE_STAGING_MAX_RADIUS * SPACE_Q16 ||
		!IsSpaceAttachmentPointClear(origin, bounds, obstacles) || !IsSpaceAttachmentPointClear(destination, bounds, obstacles)) return std::nullopt;
	if (origin == destination) return 0;
	const uint32_t minimum_step = GetMultiLayerMiningProbeTuning().travel_tiles_per_day * SPACE_Q16 / day;
	if (minimum_step <= 2) return std::nullopt;
	// Each integer component can lose less than one Q16 unit to truncation.
	// Manhattan distance bounds Euclidean distance, so this conservative budget
	// contains every local native route without a wall-clock timeout or long loop.
	const uint64_t guaranteed_progress = minimum_step - 2;
	const uint64_t limit = (dx + dy + guaranteed_progress - 1) / guaranteed_progress + 1;
	auto point = origin;
	for (uint32_t tick = 1; tick <= limit; ++tick) {
		const auto previous = point;
		const bool arrived = MoveSpaceSurveyTowards(point, destination, remainder);
		// A safe endpoint alone cannot admit a route through a paid footprint or
		// another static berth; no blocker or claimant is moved to make it clear.
		if (!IsSpaceMovementSegmentClear(previous, point, bounds, obstacles)) return std::nullopt;
		if (arrived) return tick;
	}
	return std::nullopt;
}

/** @copydoc GetSpaceDepartureReleaseDelay */
std::optional<uint64_t> GetSpaceDepartureReleaseDelay(const SpaceAttachmentSafety &safety, std::optional<uint32_t> travel_ticks)
{
	if (!safety.first_unsafe_tick) return std::nullopt;
	// One extra opportunity releases before body translation. With no admitted
	// route, retain the original valid point and release just before it becomes
	// unsafe; a failed route never permits teleportation or a delayed edge step.
	const uint64_t lead = travel_ticks ? uint64_t{*travel_ticks} + 1 : 1;
	return *safety.first_unsafe_tick > lead ? *safety.first_unsafe_tick - lead : 0;
}

/** Resolve target ownership without allocating a proxy station or changing an asteroid attachment lease.
 * @param craft Actual asking hull. @param target Normalized destination.
 * @return Whether the native target still belongs to this request's physical domain and owner.
 */
static bool HasApproachTarget(const SpaceVehicle &craft, const SpaceOrderTarget &target)
{
	if (!ValidApproachTarget(target)) return false;
	if (target.kind == SpaceOrderTargetKind::Body) return GetMutableAsteroidBody(OrbitalBodyID{target.id}) != nullptr;
	const auto *site = ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(target.id)}, target.generation});
	// Cancelled jobs with actual unconsumed material retain collection access; empty cancelled footprints cannot supply a berth.
	return site != nullptr && site->state.owner == craft.owner && (site->state.phase != SpaceStructurePhase::Cancelled ||
		site->job.inputs[0].TotalCount() != 0 || site->job.inputs[1].TotalCount() != 0);
}

/** @param point Candidate waiting point. @param craft Asking hull. @return Whether local physical occupancy permits safe staging. */
static bool IsSafeStaging(SpacePoint point, const SpaceVehicle &craft)
{
	// Supported map bounds apply to actual craft movement; a staging destination must never depend on the selected viewport.
	if (point.x < 0 || point.y < 0 || point.x > int64_t{Map::SizeX() - 1} * SPACE_Q16 || point.y > int64_t{Map::SizeY() - 1} * SPACE_Q16) return false;
	// Check bounded actual Space hulls and staging claims; rockets count physically, while carried passengers stay inside their original hull.
	for (const auto *other : SpaceVehicle::Iterate()) {
		if (other == &craft) continue;
		if (GetSpaceVehicleLayer(*other) != MapLayer::Space || other->construction.placement == SpacePlacement::Carried) continue;
		if (SpaceDistanceSquared(point, other->position) < uint64_t{SPACE_Q16} * SPACE_Q16) return false;
		if (other->space_orders.approach && other->space_orders.approach->staging == point) return false;
		// A saved local departure is already owned by its original mission.
		// Include it in common occupancy so sequential safety release cannot send
		// simultaneous miners to one endpoint without another berth registry.
		if (other->mission.continuation.departure_point && SpaceDistanceSquared(point, *other->mission.continuation.departure_point) < uint64_t{SPACE_Q16} * SPACE_Q16) return false;
	}
	// Static footprints and both reserved Dock approach cells remain unavailable for waiting craft.
	for (const auto *site : SpaceStructure::Iterate()) {
		const auto *d = GetSpaceConstructionDefinition(site->state.kind);
		if (d == nullptr) return false;
		const int64_t x = int64_t{site->state.x} * SPACE_Q16, y = int64_t{site->state.y} * SPACE_Q16;
		if (point.x >= x && point.x < x + d->width * SPACE_Q16 && point.y >= y && point.y < y + d->height * SPACE_Q16) return false;
		for (uint8_t slot = 0; slot < GetSpaceStructureApproachCapacity(*site); ++slot) if (point == *GetSpaceStructureApproachSlot(*site, slot)) return false;
	}
	// The derived body index limits exclusion checks to the local envelope, including a moving body's attached-craft margin.
	for (auto id : QueryAsteroidBodies({point.x - BODY_APPROACH_CLEARANCE, point.y - BODY_APPROACH_CLEARANCE}, {point.x + BODY_APPROACH_CLEARANCE, point.y + BODY_APPROACH_CLEARANCE})) {
		RecordSpaceWork(SpaceWorkKind::StagingBodyTests);
		const auto *body = GetMutableAsteroidBody(id);
		if (body != nullptr && SpaceDistanceSquared(point, body->motion.position) < uint64_t{BODY_APPROACH_CLEARANCE} * BODY_APPROACH_CLEARANCE) return false;
	}
	return true;
}

/** Find a real waiting/departure destination without changing position or saved priority.
 * @param craft Actual asking hull.
 * @param onward Optional actual onward route; only departing miners prefer its nearest clear candidate.
 * @param path_allowed Optional read-only additional route admission; absence preserves existing waiting/return behavior.
 * @param examined Optional count of bounded candidates inspected by this query; no gameplay counter is updated.
 * @return Safe current position or a bounded local alternative, if one exists.
 */
static std::optional<SpacePoint> FindSafeStaging(const SpaceVehicle &craft, std::optional<SpacePoint> onward = std::nullopt,
	const std::function<bool(SpacePoint)> &path_allowed = {}, uint8_t *examined = nullptr)
{
	if (examined != nullptr) *examined = 1;
	// An already clear point still needs any caller-specific route admission;
	// legacy callers provide no filter and retain their original immediate result.
	if (IsSafeStaging(craft.position, craft) && (!path_allowed || path_allowed(craft.position))) return craft.position;
	std::optional<SpacePoint> best;
	// The fixed local ring order bounds work and makes simultaneous staging choices independent of rendering or RNG.
	for (int radius = 1; radius <= SPACE_STAGING_MAX_RADIUS; ++radius) {
		for (const auto [dx, dy] : SPACE_STAGING_DIRECTIONS) {
			if (examined != nullptr) ++*examined;
			const SpacePoint point{craft.position.x + dx * radius * SPACE_Q16, craft.position.y + dy * radius * SPACE_Q16};
			if (!IsSafeStaging(point, craft)) continue;
			// Additional path checks may reject a geometrically clear endpoint;
			// they do not change priority, position or the fixed candidate order.
			if (path_allowed && !path_allowed(point)) continue;
			if (!onward) return point;
			// Original waiting requests retain their canonical first candidate. A mining departure instead prefers the homeward clear point so the next direct return step does not reverse into the body envelope.
			if (!best || SpaceDistanceSquared(point, *onward) < SpaceDistanceSquared(*best, *onward)) best = point;
		}
	}
	return best;
}

/** @copydoc IsSpaceDeparturePointAvailable */
bool IsSpaceDeparturePointAvailable(const SpaceVehicle &craft, SpacePoint point) { return IsSafeStaging(point, craft); }

/** @copydoc PlanSpaceDeparture */
SpaceDeparturePlan PlanSpaceDeparture(const SpaceVehicle &craft, std::optional<SpacePoint> onward)
{
	SpaceDeparturePlan plan;
	plan.retained_position = craft.position;
	const auto bounds = GetSpacePlaneBounds(Map::SizeX(), Map::SizeY());
	const auto obstacles = GetSpaceAttachmentObstacles();
	// A fallback must already be a valid clear point. Neither a malformed route
	// nor a selected off-map destination may manufacture a corrected position.
	if (!bounds || !obstacles || !IsSpaceAttachmentPointClear(craft.position, *bounds, *obstacles) ||
		(onward && !bounds->Contains(*onward)) || craft.mission.travel_remainder >= GetMultiLayerSpaceTuning().ticks_per_economy_day) return plan;
	const auto *body = GetMutableAsteroidBody(craft.mission.target);
	// Only this exact original live lease supplies attachment safety. A recalled
	// craft may still keep a historical target, but no longer needs lease release.
	if (body != nullptr && body->mining_slot == craft.index) {
		const auto forecast = ForecastAsteroidExit(*body, *bounds);
		if (!forecast) return plan;
		const auto offset = craft.mission.attachment_offset;
		// The actual hull must be at its saved attachment; a stale lease cannot
		// move it to the expected point or lend another mission a release deadline.
		if (craft.mission.phase != SpaceMissionPhase::Mining || int64_t{body->motion.position.x} + offset.x != craft.position.x ||
			int64_t{body->motion.position.y} + offset.y != craft.position.y) return plan;
		plan.attachment = ForecastSpaceAttachmentSafety(body->motion, *forecast, offset, *bounds, *obstacles);
		if (!plan.attachment) return plan;
	}
	/** @param point Bounded original staging candidate. @return Whether native movement can reach it without crossing original static/approach geometry. */
	const auto path_allowed = [&](SpacePoint point) { return GetSpaceDepartureTravelTicks(craft.position, point, craft.mission.travel_remainder, *bounds, *obstacles).has_value(); };
	plan.destination = FindSafeStaging(craft, onward, path_allowed, &plan.candidates_examined);
	if (plan.destination) {
		const auto ticks = GetSpaceDepartureTravelTicks(craft.position, *plan.destination, craft.mission.travel_remainder, *bounds, *obstacles);
		assert(ticks.has_value());
		plan.travel_ticks = *ticks;
		plan.status = SpaceDepartureStatus::Ready;
	} else {
		plan.status = SpaceDepartureStatus::NoClearPoint;
	}
	if (plan.attachment) plan.release_after_ticks = GetSpaceDepartureReleaseDelay(*plan.attachment, plan.destination ? std::optional{plan.travel_ticks} : std::nullopt);
	return plan;
}

/** @copydoc GetSpaceMiningDepartureDestination */
std::optional<SpacePoint> GetSpaceMiningDepartureDestination(const SpaceVehicle &craft, SpacePoint onward)
{
	if (craft.mission.phase != SpaceMissionPhase::Return) return std::nullopt;
	const auto *body = GetMutableAsteroidBody(craft.mission.target);
	// Departure is local to the original mining envelope. Once clear, ordinary travel retains its existing semantics; a removed body creates no artificial obstacle, waypoint or lease.
	if (body == nullptr || SpaceDistanceSquared(craft.position, body->motion.position) >= uint64_t{BODY_APPROACH_CLEARANCE} * BODY_APPROACH_CLEARANCE) return craft.position;
	return FindSafeStaging(craft, onward);
}

/** @copydoc RequestSpaceApproach */
bool RequestSpaceApproach(SpaceVehicle &craft, SpaceOrderTarget target, bool execute)
{
	target = NormalizeApproachTarget(target);
	// Estimates and repeated requests preserve physical owners, original queue priority and active service reservations.
	if (!Company::IsValidID(craft.owner) || !CanOwnSpaceApproach(craft, target) || craft.generation == 0 || craft.vehstatus.Test(VehState::Crashed) || !HasApproachTarget(craft, target)) return false;
	if (craft.space_orders.approach && craft.space_orders.approach->target == target) return true;
	if (HasOrbitalServiceAssignment(craft)) return false;
	const auto staging = FindSafeStaging(craft);
	if (!staging) return false;
	if (execute) craft.space_orders.approach = SpaceApproachRequest{target, TimerGameTick::counter, UINT8_MAX, *staging};
	return true;
}

/** @copydoc ReleaseSpaceApproach */
void ReleaseSpaceApproach(SpaceVehicle &craft) { craft.space_orders.approach.reset(); }

/** Derive the requested target's actual slots and foreign physical occupants.
 * @param craft Request owner. @param target Exact normalized target. @return Bounded slots for this actual target.
 */
static std::vector<SpaceApproachSlot> GetTargetSlots(const SpaceVehicle &craft, const SpaceOrderTarget &target)
{
	std::vector<SpaceApproachSlot> slots;
	if (!HasApproachTarget(craft, target)) return slots;
	if (target.kind == SpaceOrderTargetKind::Body) {
		const auto &body = *GetMutableAsteroidBody(OrbitalBodyID{target.id});
		const auto bounds = GetSpacePlaneBounds(Map::SizeX(), Map::SizeY());
		const auto obstacles = GetSpaceAttachmentObstacles();
		// A drifting body's slot may disappear without moving the body or an
		// existing static occupant. Both arbitration and destination queries use
		// this same boundary, so a retained request cannot bypass new geometry.
		if (!bounds || !obstacles) return slots;
		const auto point = GetSafeSpaceBodyApproachPoint(body.motion.position, *bounds, *obstacles);
		if (!point) return slots;
		slots.push_back({target, 0, *point, body.mining_slot, body.survey_slot != INVALID_SPACE_SURVEY});
	} else {
		const auto &site = *ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(target.id)}, target.generation});
		for (uint8_t i = 0; i < GetSpaceStructureApproachCapacity(site); ++i) slots.push_back({target, i, *GetSpaceStructureApproachSlot(site, i)});
		// A native manufacturing job reserves its planned output berth through this same authority; incoming craft wait without fake VehicleIDs or a parallel occupancy table.
		if (site.module && site.module->manufacture && site.module->manufacture->phase != OrbitalManufacturePhase::Complete) for (auto &slot : slots) slot.blocked = true;
		// Retained service jobs own their exact physical berth even when paused
		// or their hull is lost. A recycled pool ID cannot inherit the old job's
		// admission; a surviving exact owner remains the original occupant.
		if (site.module) for (auto &slot : slots) {
			const auto &job = *site.module->GetServiceJobs()[slot.slot];
			if (job.remaining_ticks == 0 && job.craft == SpaceCraftRef{}) continue;
			const auto *owner = ResolveSpaceCraft(job.craft);
			if (owner == nullptr) slot.blocked = true;
			else slot.occupant = owner->index;
		}
	}
	// Physical occupants and legacy body leases constrain reservations; admission never rewrites either physical authority.
	for (auto &slot : slots) {
		for (const auto *other : SpaceVehicle::Iterate()) {
			RecordSpaceWork(SpaceWorkKind::ApproachOccupantChecks);
			// An actual Space rocket occupies geometry through its original hull; a carried passenger is custody inside that hull and consumes no second berth.
			if (GetSpaceVehicleLayer(*other) != MapLayer::Space || other->construction.placement == SpacePlacement::Carried || other->position != slot.point) continue;
			if (slot.occupant != VehicleID::Invalid() && slot.occupant != other->index) slot.blocked = true;
			slot.occupant = other->index;
		}
	}
	return slots;
}

/** @copydoc CanAcquireSpaceApproach */
bool CanAcquireSpaceApproach(const SpaceVehicle &craft, SpaceOrderTarget target)
{
	target = NormalizeApproachTarget(target);
	// Automatic destinations require an actual owned physical target and a free
	// craft. A retained service job or a crashed/invalid hull grants no new slot.
	if (!Company::IsValidID(craft.owner) || !CanOwnSpaceApproach(craft, target) || craft.generation == 0 ||
		craft.vehstatus.Test(VehState::Crashed) || HasOrbitalServiceAssignment(craft) || !HasApproachTarget(craft, target)) return false;
	const bool retained = craft.space_orders.approach && craft.space_orders.approach->target == target;
	const auto staging = retained ? std::optional{craft.space_orders.approach->staging} : FindSafeStaging(craft);
	if (!staging) return false;
	std::vector<SpaceApproachRecord> requests;
	// Only the considered target competes for these derived slots. Existing
	// requests keep their native priority/incarnation; the asking hull replaces
	// only its own copied request, never a live request belonging to another hull.
	for (const auto *other : SpaceVehicle::Iterate()) {
		if (other->index == craft.index || !other->space_orders.approach || other->space_orders.approach->target != target) continue;
		requests.push_back({other->index, other->generation, *other->space_orders.approach});
	}
	requests.push_back({craft.index, craft.generation, retained ? *craft.space_orders.approach :
		SpaceApproachRequest{target, TimerGameTick::counter, UINT8_MAX, *staging}});
	const auto plan = PlanSpaceApproaches(requests, GetTargetSlots(craft, target));
	if (!plan) return false;
	const auto found = std::ranges::find(*plan, craft.index, &SpaceApproachRecord::craft);
	return found != plan->end() && found->request.slot != UINT8_MAX;
}

/** @copydoc PrepareSpaceApproachTick */
void PrepareSpaceApproachTick()
{
	// Only the synchronized running simulation may admit requests; paused/editor callbacks preserve saved priority and ownership.
	if (_game_mode != GameMode::Normal || _pause_mode.Any()) return;
	// Gather existing explicit orders before any hull earns upkeep/work or moves; simultaneous arrivals share this native tick's priority.
	for (auto *craft : SpaceVehicle::Iterate()) {
		RecordSpaceWork(SpaceWorkKind::ApproachCraftVisits);
		const auto &q = craft->space_orders;
		if (craft->transition) {
			const auto &flight = *craft->transition;
			// Gather module flights before normal-order stop filtering: native rockets retain Stopped for their dormant order controller, while their separate flight owns movement and shared priority.
			if (CanOwnSpaceApproach(*craft, flight.destination) && flight.destination.kind == SpaceOrderTargetKind::Structure && craft->construction.placement == SpacePlacement::InTransit) {
				RequestSpaceApproach(*craft, flight.destination, true); continue;
			}
		}
		if (craft->construction.placement != SpacePlacement::Orbital || craft->vehstatus.Test(VehState::Stopped) || craft->vehstatus.Test(VehState::Crashed) || HasOrbitalServiceAssignment(*craft)) continue;
		if (q.blocked == SpaceOrderBlock::Timeout) { ReleaseSpaceApproach(*craft); continue; }
		if (HasSpaceMiningContinuationOrder(*craft)) {
			const auto effective = ResolveEffectiveSpaceOrderTarget(*craft);
			// Detached/waiting miners acquire no historical body slot. A real
			// reached static berth remains held until departure selection uses its
			// original admission; remote failed requests confer no such right.
			if (!effective) {
				SpacePoint admitted;
				if (GetSpaceApproachDestination(*craft, admitted) != SpaceOrderBlock::None || admitted != craft->position) ReleaseSpaceApproach(*craft);
				continue;
			}
			// Initial or replacement body work still needs original service/pod
			// admission. Its receiver/service continuation keeps a separate target.
			if (effective->kind == SpaceOrderTargetKind::Body && GetSpaceMiningBlock(*craft) != SpaceOrderBlock::None) continue;
			if (effective->kind == SpaceOrderTargetKind::Body || effective->kind == SpaceOrderTargetKind::Structure || effective->kind == SpaceOrderTargetKind::Blueprint) RequestSpaceApproach(*craft, *effective, true);
			continue;
		}
		if (q.cursor < q.entries.size()) {
			const auto &o = q.entries[q.cursor];
			// Unsupported/finalized actions acquire no slot; live explicit static/body navigation uses this one shared producer.
			if (o.target.kind != SpaceOrderTargetKind::Body && o.target.kind != SpaceOrderTargetKind::Structure && o.target.kind != SpaceOrderTargetKind::Blueprint) continue;
			if (o.action == SpaceOrderAction::Mine && (GetSpaceMiningBlock(*craft) != SpaceOrderBlock::None ||
				(q.started && (craft->mission.phase == SpaceMissionPhase::Return || craft->mission.phase == SpaceMissionPhase::Holding)))) continue;
			RequestSpaceApproach(*craft, o.target, true);
		} else if (q.entries.empty() && craft->mission.phase == SpaceMissionPhase::Approach) {
			RequestSpaceApproach(*craft, {SpaceOrderTargetKind::Body, craft->mission.target.base(), 0}, true);
		}
	}
	std::vector<SpaceApproachRecord> requests;
	std::vector<SpaceApproachSlot> slots;
	// Gather every company before arbitration, preserving old priority and releasing only requests whose actual target disappeared.
	for (auto *craft : SpaceVehicle::Iterate()) {
		RecordSpaceWork(SpaceWorkKind::ApproachCraftVisits);
		if (!craft->space_orders.approach) continue;
		const auto target = craft->space_orders.approach->target;
		if (!CanOwnSpaceApproach(*craft, target) || craft->vehstatus.Test(VehState::Crashed) || !HasApproachTarget(*craft, target)) {
			ReleaseSpaceApproach(*craft);
			if (!craft->space_orders.entries.empty()) { craft->space_orders.blocked = SpaceOrderBlock::TargetRemoved; craft->space_orders.started = false; craft->space_orders.timer_ticks = 0; craft->space_orders.work_ticks = 0; }
			continue;
		}
		requests.push_back({craft->index, craft->generation, *craft->space_orders.approach});
		/** @param s Already gathered original slot. @return Whether this physical domain already contributes its native slots. */
		if (std::ranges::none_of(slots, [&](const auto &s) { RecordSpaceWork(SpaceWorkKind::ApproachTargetChecks); return s.target == target; })) {
			auto target_slots = GetTargetSlots(*craft, target);
			slots.insert(slots.end(), target_slots.begin(), target_slots.end());
		}
	}
	const auto plan = PlanSpaceApproaches(requests, slots);
	assert(plan.has_value());
	if (!plan) return;
	// Publish only planned slot ownership; no cargo, position, service timer or body lease changes in this common phase.
	for (const auto &row : *plan) {
		auto &craft = *SpaceVehicle::Get(row.craft);
		craft.space_orders.approach = row.request;
		// Moving bodies or a new physical occupant can invalidate a waiting point; refresh only staging, preserving priority and all positions.
		if (row.request.slot == UINT8_MAX && !IsSafeStaging(row.request.staging, craft)) {
			if (const auto staging = FindSafeStaging(craft)) craft.space_orders.approach->staging = *staging;
		}
	}
}

/** @copydoc GetSpaceApproachDestination */
SpaceOrderBlock GetSpaceApproachDestination(const SpaceVehicle &craft, SpacePoint &point)
{
	if (!craft.space_orders.approach) return SpaceOrderBlock::TargetRemoved;
	const auto &request = *craft.space_orders.approach;
	if (!HasApproachTarget(craft, request.target)) return SpaceOrderBlock::TargetRemoved;
	if (request.slot == UINT8_MAX) {
		// If bounded local staging is currently saturated, retain the actual hull position rather than moving into somebody else's claim.
		point = IsSafeStaging(request.staging, craft) ? request.staging : craft.position;
		return SpaceOrderBlock::ApproachBusy;
	}
	const auto slots = GetTargetSlots(craft, request.target);
	/** @param s Actual target slot. @return Whether its stable ordinal belongs to this original request. */
	const auto found = std::ranges::find_if(slots, [&](const auto &s) { return s.slot == request.slot; });
	// A lost/blocked slot remains a wait; a query cannot push an occupant away or grant a replacement slot outside arbitration.
	if (found == slots.end() || found->blocked || (found->occupant != VehicleID::Invalid() && found->occupant != craft.index)) {
		point = IsSafeStaging(request.staging, craft) ? request.staging : craft.position;
		return SpaceOrderBlock::ApproachBusy;
	}
	point = found->point;
	return SpaceOrderBlock::None;
}

/** @copydoc GetSpaceStructureBerth */
std::optional<uint8_t> GetSpaceStructureBerth(const SpaceVehicle &craft, const SpaceStructure &site)
{
	if (!craft.space_orders.approach) return std::nullopt;
	const auto &r = *craft.space_orders.approach;
	// A coincident coordinate or reused site ID cannot substitute for this hull's actual admitted reservation.
	if (r.target != SpaceOrderTarget{SpaceOrderTargetKind::Structure, site.index.base(), site.generation} || r.slot == UINT8_MAX) return std::nullopt;
	SpacePoint point;
	if (GetSpaceApproachDestination(craft, point) != SpaceOrderBlock::None || craft.position != point) return std::nullopt;
	return r.slot;
}

/** @copydoc ValidateNativeSpaceApproaches */
bool ValidateNativeSpaceApproaches()
{
	if (!ValidateSpaceApproachRecords(CaptureSpaceOrders(), TimerGameTick::counter, Map::SizeX(), Map::SizeY())) return false;
	// Saved requests must still resolve to their exact native producers; loading cannot silently retarget another company's site.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		if (!craft->space_orders.approach) continue;
		const auto &r = *craft->space_orders.approach;
		if (!CanOwnSpaceApproach(*craft, r.target) || !HasApproachTarget(*craft, r.target)) return false;
		if (r.target.kind == SpaceOrderTargetKind::Structure && r.slot != UINT8_MAX) {
			const auto *site = ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(r.target.id)}, r.target.generation});
			if (!GetSpaceStructureApproachSlot(*site, r.slot)) return false;
			// A pending paid batch owns the output slot. After completion, actual native hull admission/occupancy resumes, including a later visitor after the product departs.
			if (site->module && site->module->manufacture && site->module->manufacture->phase != OrbitalManufacturePhase::Complete) return false;
		}
	}
	return true;
}

/** @copydoc IsSpaceApproachSiteAvailable */
bool IsSpaceApproachSiteAvailable(const SpaceStructureState &state)
{
	const auto *d = GetSpaceConstructionDefinition(state.kind);
	if (d == nullptr) return false;
	/** @param point Actual hull or retained staging point. @return Whether it occupies the proposed footprint or reserved south approach. */
	auto contains = [&](SpacePoint point) {
		const int64_t x = int64_t{state.x} * SPACE_Q16, y = int64_t{state.y} * SPACE_Q16;
		// Dock's two precise berths stay within its one reserved approach cell; placement cannot displace either physical position.
		return (point.x >= x && point.x < x + d->width * SPACE_Q16 && point.y >= y && point.y < y + d->height * SPACE_Q16) ||
			(point.y == y + d->height * SPACE_Q16 && point.x >= x && point.x < x + SPACE_Q16);
	};
	// A blueprint cannot displace a real hull or its retained waiting claim, even when no facility reservation currently owns that cell.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		// Placement cannot displace an actual in-Space rocket, including its retained staging point; carried passengers remain inside the original physical hull.
		if (GetSpaceVehicleLayer(*craft) != MapLayer::Space || craft->construction.placement == SpacePlacement::Carried) continue;
		if (contains(craft->position) || (craft->space_orders.approach && contains(craft->space_orders.approach->staging))) return false;
	}
	return true;
}

/** @copydoc IsSpaceStructureManufactureBerthFree */
bool IsSpaceStructureManufactureBerthFree(const SpaceStructure &site)
{
	// Only a real Assembly's one physical output slot can admit manufacture; actual footprint, approach and staging occupants remain authoritative.
	if (site.state.kind != SpaceBuildKind::Assembly || GetSpaceStructureApproachCapacity(site) != 1 || !IsSpaceApproachSiteAvailable(site.state)) return false;
	const SpaceOrderTarget target{SpaceOrderTargetKind::Structure, site.index.base(), site.generation};
	// An already admitted incoming hull has priority even before reaching the precise point; a new order cannot steal its saved native slot.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		if (!craft->space_orders.approach) continue;
		const auto &request = *craft->space_orders.approach;
		if (request.target == target && request.slot != UINT8_MAX) return false;
	}
	return true;
}

/** @copydoc BindSpaceManufactureBerth */
bool BindSpaceManufactureBerth(SpaceVehicle &craft, const SpaceStructure &site)
{
	// Only the exact freshly completed paid product can inherit this preflighted output berth; a caller cannot use this boundary to admit an unrelated incoming hull.
	if (!site.module || !site.module->manufacture || site.module->manufacture->phase != OrbitalManufacturePhase::Complete ||
		site.module->manufacture->product != GetSpaceCraftReference(craft) || craft.owner != site.state.owner || craft.space_orders.approach ||
		craft.construction.assembly != GetSpaceStructureReference(site) || craft.construction.assembly_serial != site.module->manufacture->serial ||
		craft.position != GetSpaceStructureApproach(site.state) || craft.construction.placement != SpacePlacement::Orbital) return false;
	const SpaceOrderTarget target{SpaceOrderTargetKind::Structure, site.index.base(), site.generation};
	const auto slots = GetTargetSlots(craft, target);
	// The one derived physical slot must still belong only to this actual hull; no proxy VehicleID or alternate occupancy registry participates.
	if (slots.size() != 1 || slots[0].blocked || (slots[0].occupant != VehicleID::Invalid() && slots[0].occupant != craft.index)) return false;
	craft.space_orders.approach = SpaceApproachRequest{target, TimerGameTick::counter, 0, craft.position}; return true;
}
