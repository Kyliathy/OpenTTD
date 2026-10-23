/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_approach.cpp Stable shared approach admission over actual native hulls and targets.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11.
 */
#include "stdafx.h"
#include "space_approach.h"
#include "space_vehicle.h"
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
		const auto found = std::ranges::find_if(slots, [&](const auto &s) { return s.target == r.request.target && s.slot == r.request.slot; });
		// Missing slots and physical foreign occupants block admission without moving either hull or replacing an actual body lease.
		if (found == slots.end() || found->blocked || (found->occupant != VehicleID::Invalid() && found->occupant != r.craft)) r.request.slot = UINT8_MAX;
	}
	std::vector<size_t> order(result.size());
	for (size_t i = 0; i < order.size(); ++i) order[i] = i;
	std::ranges::sort(order, [&](size_t a, size_t b) {
		return std::tuple{result[a].request.request_tick, result[a].craft.base()} < std::tuple{result[b].request.request_tick, result[b].craft.base()};
	});
	// Older requests win first, with native hull ID resolving simultaneous arrivals independently of input iteration order.
	for (const auto index : order) {
		auto &r = result[index];
		if (r.request.slot != UINT8_MAX) continue;
		// Choose the lowest free physical slot, so derived slot enumeration cannot change deterministic admission.
		const SpaceApproachSlot *best = nullptr;
		for (const auto &s : slots) {
			if (s.target != r.request.target || s.blocked || (s.occupant != VehicleID::Invalid() && s.occupant != r.craft)) continue;
			const bool used = std::ranges::any_of(result, [&](const auto &other) { return other.request.target == s.target && other.request.slot == s.slot; });
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
	for (auto id : QueryAsteroidBodies({point.x - 2 * SPACE_Q16, point.y - 2 * SPACE_Q16}, {point.x + 2 * SPACE_Q16, point.y + 2 * SPACE_Q16})) {
		const auto *body = GetMutableAsteroidBody(id);
		if (body != nullptr && SpaceDistanceSquared(point, body->motion.position) < uint64_t{2 * SPACE_Q16} * (2 * SPACE_Q16)) return false;
	}
	return true;
}

/** Find a real waiting destination without changing position or saved priority.
 * @param craft Actual asking hull. @return Safe current position or a bounded local alternative, if one exists.
 */
static std::optional<SpacePoint> FindSafeStaging(const SpaceVehicle &craft)
{
	if (IsSafeStaging(craft.position, craft)) return craft.position;
	// The fixed local ring order bounds work and makes simultaneous staging choices independent of rendering or RNG.
	for (int radius = 1; radius <= 8; ++radius) {
		for (const auto [dx, dy] : std::array<std::pair<int, int>, 8>{{{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}}}) {
			const SpacePoint point{craft.position.x + dx * radius * SPACE_Q16, craft.position.y + dy * radius * SPACE_Q16};
			if (IsSafeStaging(point, craft)) return point;
		}
	}
	return std::nullopt;
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
		slots.push_back({target, 0, {body.motion.position.x + SPACE_Q16, body.motion.position.y - SPACE_Q16 / 2}, body.mining_slot, body.survey_slot != INVALID_SPACE_SURVEY});
	} else {
		const auto &site = *ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(target.id)}, target.generation});
		for (uint8_t i = 0; i < GetSpaceStructureApproachCapacity(site); ++i) slots.push_back({target, i, *GetSpaceStructureApproachSlot(site, i)});
	}
	// Physical occupants and legacy body leases constrain reservations; admission never rewrites either physical authority.
	for (auto &slot : slots) {
		for (const auto *other : SpaceVehicle::Iterate()) {
			// An actual Space rocket occupies geometry through its original hull; a carried passenger is custody inside that hull and consumes no second berth.
			if (GetSpaceVehicleLayer(*other) != MapLayer::Space || other->construction.placement == SpacePlacement::Carried || other->position != slot.point) continue;
			if (slot.occupant != VehicleID::Invalid() && slot.occupant != other->index) slot.blocked = true;
			slot.occupant = other->index;
		}
	}
	return slots;
}

/** @copydoc PrepareSpaceApproachTick */
void PrepareSpaceApproachTick()
{
	// Only the synchronized running simulation may admit requests; paused/editor callbacks preserve saved priority and ownership.
	if (_game_mode != GameMode::Normal || _pause_mode.Any()) return;
	// Gather existing explicit orders before any hull earns upkeep/work or moves; simultaneous arrivals share this native tick's priority.
	for (auto *craft : SpaceVehicle::Iterate()) {
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
		if (!craft->space_orders.approach) continue;
		const auto target = craft->space_orders.approach->target;
		if (!CanOwnSpaceApproach(*craft, target) || craft->vehstatus.Test(VehState::Crashed) || !HasApproachTarget(*craft, target)) {
			ReleaseSpaceApproach(*craft);
			if (!craft->space_orders.entries.empty()) { craft->space_orders.blocked = SpaceOrderBlock::TargetRemoved; craft->space_orders.started = false; craft->space_orders.timer_ticks = 0; craft->space_orders.work_ticks = 0; }
			continue;
		}
		requests.push_back({craft->index, craft->generation, *craft->space_orders.approach});
		if (std::ranges::none_of(slots, [&](const auto &s) { return s.target == target; })) {
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
