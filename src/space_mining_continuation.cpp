/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_continuation.cpp Mining continuation under the original mission, queue, approach and physical handling owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups20/23/25.
 * Native unsafe-step publication belongs to Group25B and normal-loop activation to Group29.
 */
#include "stdafx.h"
#include "space_mining_continuation.h"
#include "space_vehicle.h"
#include "space_mining_target.h"
#include "space_return_target.h"
#include "space_manifest.h"
#include "space_approach.h"
#include "space_departure.h"
#include "space_mining_tuning.h"
#include "space_structure.h"
#include "orbital_service.h"
#include "launch_center.h"
#include "space_replacement.h"
#include "asteroid.h"
#include "map_func.h"
#include "safeguards.h"

/** @copydoc HasSpaceMiningContinuationOrder */
bool HasSpaceMiningContinuationOrder(const SpaceVehicle &craft)
{
	const auto &q = craft.space_orders;
	// An unordered legacy miner may own only the saved local departure. Its
	// original empty queue never becomes an automatic Mine or receiver policy.
	if (q.entries.empty()) return craft.mission.continuation.phase == MiningContinuationPhase::Detaching;
	// Only the current original Mine owns this progress. A queued future Mine
	// or a stale intent after editing another action cannot commandeer navigation.
	return q.cursor < q.entries.size() && q.entries[q.cursor].action == SpaceOrderAction::Mine &&
		(q.entries[q.cursor].mining_policy != SpaceMiningPolicy::LegacyReturn || craft.mission.continuation.phase != MiningContinuationPhase::None);
}

/** @copydoc ResolveEffectiveSpaceOrderTarget */
std::optional<SpaceOrderTarget> ResolveEffectiveSpaceOrderTarget(const SpaceVehicle &craft)
{
	const auto &q = craft.space_orders;
	if (q.cursor >= q.entries.size()) return std::nullopt;
	if (!HasSpaceMiningContinuationOrder(craft)) return q.entries[q.cursor].target;
	const auto &intent = craft.mission.continuation;
	// Native finite extraction may finish after this tick's queue dispatch.
	// Its released Return/Holding state must not reacquire the old body in the
	// next common gather while continuation publication is still pending.
	if (q.started && (craft.mission.phase == SpaceMissionPhase::Return || craft.mission.phase == SpaceMissionPhase::Holding)) return std::nullopt;
	if (intent.phase == MiningContinuationPhase::None) return q.started ?
		SpaceOrderTarget{SpaceOrderTargetKind::Body, craft.mission.target.base(), 0} : q.entries[q.cursor].target;
	// A detached/waiting hull cannot reacquire its historical body's approach.
	if (intent.phase == MiningContinuationPhase::Detaching || intent.phase == MiningContinuationPhase::Waiting) return std::nullopt;
	// Intervening Visits remain the actual original entries, executed once in
	// order while the Mine cursor retains its mineral/full-load intent.
	if ((intent.phase == MiningContinuationPhase::ReturnCargo || intent.phase == MiningContinuationPhase::NeedService) &&
		intent.route_cursor < q.entries.size() && q.entries[intent.route_cursor].action == SpaceOrderAction::Visit) return q.entries[intent.route_cursor].target;
	return intent.effective_target;
}

/** @param craft Original miner. @return Whether its actual freight has reached its current native physical capacity. */
static bool HasFullMiningHold(const SpaceVehicle &craft) { return craft.cargo_cap != 0 && craft.cargo.TotalCount() >= craft.cargo_cap; }

/** @param craft Original miner. @return Next continuation purpose derived only from the current policy, actual freight and original service obligation. */
static MiningContinuationPhase NextMiningPhase(const SpaceVehicle &craft)
{
	const auto policy = craft.space_orders.entries[craft.space_orders.cursor].mining_policy;
	if (GetSpaceMiningBlock(craft) == SpaceOrderBlock::ServiceRequired) return MiningContinuationPhase::NeedService;
	// An edit back to legacy preserves physical departure, then returns through
	// its exact declared route instead of acquiring another automatic source.
	if (policy == SpaceMiningPolicy::LegacyReturn) return MiningContinuationPhase::ReturnCargo;
	return SelectSpaceMiningContinuationPhase(policy, HasFullMiningHold(craft), false);
}

/** Retain a named bounded retry without reserving a remote target.
 * @param craft Original miner whose packets and static source berth remain accountable.
 * @param reason Explicit mission wait. @param block Original queue's operational disclosure.
 */
static void WaitForMiningContinuation(SpaceVehicle &craft, MiningContinuationWait reason, SpaceOrderBlock block)
{
	auto &intent = craft.mission.continuation;
	intent.phase = MiningContinuationPhase::Waiting; intent.waiting = reason; intent.retry_ticks = GetSpaceMiningTargetRetryDelay();
	intent.effective_target.reset(); intent.travel_ticks = 0;
	craft.space_orders.blocked = block;
	// Keep only an independently reached source berth. A failed remote target
	// must not monopolize a slot while this hull waits elsewhere for reselection.
	SpacePoint admitted;
	if (GetSpaceApproachDestination(craft, admitted) != SpaceOrderBlock::None || admitted != craft.position) ReleaseSpaceApproach(craft);
}

/** @copydoc BeginSpaceMiningContinuation */
bool BeginSpaceMiningContinuation(SpaceVehicle &craft, std::optional<SpacePoint> departure)
{
	const auto &queue = craft.space_orders;
	const bool unordered = queue.entries.empty();
	const bool mine = queue.cursor < queue.entries.size() && queue.entries[queue.cursor].action == SpaceOrderAction::Mine;
	const auto *body = GetMutableAsteroidBody(craft.mission.target);
	// Legacy unordered intent can start only from its actual existing mining
	// pursuit/lease. A held unrelated craft or another action cannot acquire it.
	if (SpaceVehicle::GetIfValid(craft.index) != &craft || craft.generation == 0 || (!mine && !unordered) || GetSpaceMiningTuning(GetSpaceVehicleModel(craft)) == nullptr ||
		(unordered && (craft.mission.continuation != SpaceMiningContinuationState{} || body == nullptr ||
			(craft.mission.phase != SpaceMissionPhase::Approach && craft.mission.phase != SpaceMissionPhase::Mining) ||
			(craft.mission.phase == SpaceMissionPhase::Mining && body->mining_slot != craft.index) ||
			(queue.approach && queue.approach->target.kind != SpaceOrderTargetKind::Body)))) return false;
	auto intent = craft.mission.continuation;
	if (intent.mineral.Empty()) {
		// Only genuine legacy/manual admission lacks a pin. Derive it from this
		// actual still-live source, never a missing history ID or another craft.
		if (body == nullptr || (!unordered && queue.entries[queue.cursor].mining_policy != SpaceMiningPolicy::LegacyReturn)) return false;
		intent.mineral = GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo;
	}
	const auto bounds = GetSpacePlaneBounds(Map::SizeX(), Map::SizeY());
	// Safety publication cannot invent an unpinned mineral or commandeer paid
	// service. It preserves both stopped/running choice and the current pose.
	if (intent.mineral.Empty() || !bounds || !bounds->Contains(craft.position) || (departure && !bounds->Contains(*departure)) || HasOrbitalServiceAssignment(craft) ||
		(craft.manifest && craft.manifest->service_ticks != 0)) return false;
	intent.previous_body = craft.mission.target;
	ReleaseSpaceMiningSlot(craft); craft.mission.phase = SpaceMissionPhase::Holding; craft.mission.approach_ticks_left = 0;
	auto &q = craft.space_orders; q.started = false; q.timer_ticks = 0; q.work_ticks = 0; q.blocked = SpaceOrderBlock::None;
	intent.phase = MiningContinuationPhase::Detaching; intent.departure_point = departure; intent.effective_target.reset();
	intent.retry_ticks = 0; intent.travel_ticks = 0; intent.waiting = MiningContinuationWait::None;
	if (!unordered && intent.route_cursor == UINT8_MAX) intent.route_cursor = q.cursor + 1;
	craft.mission.continuation = intent;
	return true;
}

/** Move only one clear ordinary native step toward the saved local departure.
 * @param craft Original detached hull; actual bounds/static owners revalidate each segment and common occupancy revalidates its retained endpoint.
 * @return Whether the local departure has actually completed. Refusal retains its exact point and cargo.
 */
static bool TickMiningDeparture(SpaceVehicle &craft)
{
	auto &intent = craft.mission.continuation;
	if (intent.retry_ticks != 0) { --intent.retry_ticks; return false; }
	if (!intent.departure_point) {
		const auto plan = PlanSpaceDeparture(craft);
		if (plan.status == SpaceDepartureStatus::Ready) intent.departure_point = plan.destination;
	}
	if (intent.departure_point) {
		const auto bounds = GetSpacePlaneBounds(Map::SizeX(), Map::SizeY()); const auto obstacles = GetSpaceAttachmentObstacles();
		auto proposed = craft.position; auto remainder = craft.mission.travel_remainder;
		const bool arrived = MoveSpaceSurveyTowards(proposed, *intent.departure_point, remainder);
		// A saved endpoint grants no passage through newly occupied geometry.
		// Copy only a point/carry for this swept check; never move a Vehicle copy
		// to bypass an arrival-sensitive native command or berth owner.
		if (bounds && obstacles && IsSpaceDeparturePointAvailable(craft, *intent.departure_point) && IsSpaceMovementSegmentClear(craft.position, proposed, *bounds, *obstacles)) {
			craft.position = proposed; craft.mission.travel_remainder = remainder; craft.cur_speed = 4;
			if (arrived) { intent.departure_point.reset(); intent.waiting = MiningContinuationWait::None; return true; }
			return false;
		}
	}
	intent.departure_point.reset(); intent.waiting = MiningContinuationWait::DepartureBlocked; intent.retry_ticks = GetSpaceMiningTargetRetryDelay();
	// There is deliberately no approach request for a local departure wait.
	// Use the existing producer wait instead of claiming a queued berth.
	// An empty original queue remains canonical empty. Its named mission wait
	// carries the block instead of inventing an order-owned queue status.
	if (!craft.space_orders.entries.empty()) craft.space_orders.blocked = SpaceOrderBlock::ProducerUnavailable;
	return false;
}

/** @param craft Original travelling hull. @param target Effective original Visit or generation-qualified receiver. @param circle True retains original positive Surface cargo drop-circle arrival. @return Whether ordinary native movement actually reached the authorized destination; unavailable targets/slots retain visible waits. */
static bool TravelMiningContinuation(SpaceVehicle &craft, const SpaceOrderTarget &target, bool circle)
{
	SpacePoint destination; auto &q = craft.space_orders; auto &intent = craft.mission.continuation;
	const auto resolved = ResolveSpaceNavigationTarget(craft, target, destination);
	if (resolved != SpaceOrderBlock::None) { q.blocked = resolved; return false; }
	// Bodies and static modules share the original sole request. Surface center
	// travel retains its separate exact-anchor/drop-circle physical contract.
	if (target.kind == SpaceOrderTargetKind::Body || target.kind == SpaceOrderTargetKind::Structure || target.kind == SpaceOrderTargetKind::Blueprint) {
		if (!RequestSpaceApproach(craft, target, true)) { q.blocked = SpaceOrderBlock::ProducerUnavailable; return false; }
		const auto admitted = GetSpaceApproachDestination(craft, destination);
		if (admitted != SpaceOrderBlock::None) { q.blocked = admitted; return false; }
	} else ReleaseSpaceApproach(craft);
	/** @return Whether this current physical point satisfies the original destination geometry. */
	const auto arrived = [&] { return circle ? IsInsideLaunchCenterDropZone(craft.position, destination) : craft.position == destination; };
	if (arrived()) { intent.travel_ticks = 0; return true; }
	if (intent.travel_ticks == 0) {
		const uint64_t span = std::max<uint64_t>(std::abs(int64_t{craft.position.x} - destination.x), std::abs(int64_t{craft.position.y} - destination.y));
		intent.travel_ticks = static_cast<uint32_t>(std::max<uint64_t>(1480, (span * 2 * 74 + 3 * SPACE_Q16 - 1) / (3 * SPACE_Q16) + 148));
	}
	--intent.travel_ticks; craft.cur_speed = 4; MoveSpaceSurveyTowards(craft.position, destination, craft.mission.travel_remainder);
	if (arrived()) { intent.travel_ticks = 0; return true; }
	if (intent.travel_ticks == 0) q.blocked = SpaceOrderBlock::Timeout;
	return false;
}

/** Complete only instructions actually visited/handled under the held Mine.
 * @param craft Original hull. @param next First unexecuted original instruction; legacy departure completes only its Mine.
 */
static void CompleteMiningContinuation(SpaceVehicle &craft, uint8_t next)
{
	auto &q = craft.space_orders;
	ReleaseSpaceMiningSlot(craft); craft.mission.phase = SpaceMissionPhase::Holding; craft.mission.approach_ticks_left = 0;
	q.cursor = next;
	q.started = false; q.timer_ticks = 0; q.work_ticks = 0; q.blocked = q.cursor == q.entries.size() ? SpaceOrderBlock::Completed : SpaceOrderBlock::None;
	craft.mission.continuation = {};
}

/** Advance actual original Visits and receiver work while Mine retains its cursor.
 * @param craft Original hull already detached from extraction.
 */
static void TickMiningReturn(SpaceVehicle &craft)
{
	auto &intent = craft.mission.continuation; auto &q = craft.space_orders;
	const auto route = GetSpaceReturnRoute(craft);
	if (!route) { WaitForMiningContinuation(craft, MiningContinuationWait::NoReceiver, SpaceOrderBlock::ProducerUnavailable); return; }
	// Pending descents retain their original allowance claims. Missing pods with
	// positive allowance cannot be repaired by purchasing redundant service.
	if (craft.manifest && craft.manifest->pending_returns != 0 &&
		(craft.manifest->returns_remaining <= craft.manifest->pending_returns || route->declared_action == SpaceOrderAction::ReturnService)) {
		WaitForMiningContinuation(craft, MiningContinuationWait::ServiceUnavailable, SpaceOrderBlock::ReturnsPending); return;
	}
	if (intent.route_cursor < route->handling_index) {
		const auto target = ResolveEffectiveSpaceOrderTarget(craft);
		if (!target) { WaitForMiningContinuation(craft, MiningContinuationWait::NoReceiver, SpaceOrderBlock::TargetRemoved); return; }
		q.blocked = SpaceOrderBlock::None;
		if (TravelMiningContinuation(craft, *target, false)) { ++intent.route_cursor; QueueSpaceReplacementOnArrival(craft); }
		return;
	}
	// Completed real service renews allowance in its original work owner. Resume
	// the held full-load task only after that actual receipt exists.
	if (intent.phase == MiningContinuationPhase::NeedService && craft.manifest && craft.manifest->returns_remaining != 0) {
		intent.phase = NextMiningPhase(craft); intent.effective_target.reset(); intent.travel_ticks = 0; return;
	}
	const auto purpose = intent.phase == MiningContinuationPhase::NeedService ? SpaceReturnPurpose::Service :
		route->declared_action == SpaceOrderAction::ReturnService ? SpaceReturnPurpose::ServiceVisit : SpaceReturnPurpose::Cargo;
	std::optional<SpaceReturnCandidate> receiver;
	if (intent.effective_target) receiver = GetSpaceReturnCandidateAdmission(craft, *intent.effective_target, purpose);
	if (!receiver) {
		receiver = FindNearestEligibleSpaceReturn(craft, purpose); intent.travel_ticks = 0;
		if (!receiver) {
			const auto block = GetSpaceMiningBlock(craft);
			WaitForMiningContinuation(craft, purpose == SpaceReturnPurpose::Service ? MiningContinuationWait::ServiceUnavailable : MiningContinuationWait::NoReceiver,
				block == SpaceOrderBlock::None ? SpaceOrderBlock::ProducerUnavailable : block); return;
		}
		intent.effective_target = receiver->target;
	}
	q.blocked = purpose == SpaceReturnPurpose::Service ? SpaceOrderBlock::ServiceRequired : SpaceOrderBlock::None;
	// Only positive Surface freight uses circle entry. Visits, empty returns and
	// onboard service reach their exact original anchor, as before continuation.
	const bool circle = purpose == SpaceReturnPurpose::Cargo && receiver->quantity != 0 && receiver->target.kind == SpaceOrderTargetKind::LaunchCenter;
	if (!TravelMiningContinuation(craft, receiver->target, circle)) return;
	craft.cur_speed = 0; craft.UpdateSpacePosition(); // Original packet release needs this opportunity's actual production/projection tile.
	if (!ExecuteSpaceReturnArrival(craft, receiver->target, purpose)) {
		WaitForMiningContinuation(craft, purpose == SpaceReturnPurpose::Service ? MiningContinuationWait::ServiceUnavailable : MiningContinuationWait::NoReceiver,
			purpose == SpaceReturnPurpose::Service ? SpaceOrderBlock::ServiceRequired : SpaceOrderBlock::ProducerUnavailable); return;
	}
	// A service start retains this instruction and its original physical job.
	// Surface cargo may require another real shell/batch before the hold empties.
	if (purpose == SpaceReturnPurpose::Service || (purpose == SpaceReturnPurpose::Cargo && craft.cargo.TotalCount() != 0)) return;
	CompleteMiningContinuation(craft, route->handling_index < q.entries.size() ? route->handling_index + 1 : q.cursor + 1);
}

/** @copydoc TickSpaceMiningContinuation */
bool TickSpaceMiningContinuation(SpaceVehicle &craft)
{
	auto &q = craft.space_orders; auto &intent = craft.mission.continuation;
	if (q.entries.empty()) {
		// The normal native Tick already honored Stopped and charged one upkeep
		// opportunity. This sole local step consumes that opportunity; ordinary
		// legacy home Return resumes on a later tick, without a second movement.
		if (intent.phase == MiningContinuationPhase::Detaching && TickMiningDeparture(craft)) {
			intent = {}; craft.mission.phase = SpaceMissionPhase::Return; craft.mission.approach_ticks_left = 0;
		}
		return true;
	}
	const auto &order = q.entries[q.cursor];
	if (q.blocked == SpaceOrderBlock::Timeout) return true;
	if (intent.phase == MiningContinuationPhase::None) {
		if (!q.started) {
			SpacePoint source;
			q.blocked = ResolveSpaceNavigationTarget(craft, order.target, source);
			if (q.blocked != SpaceOrderBlock::None) return true;
			q.blocked = GetSpaceMiningBlock(craft);
			if (q.blocked != SpaceOrderBlock::None) return true;
			if (AssignSpaceMiningMission(craft, order.target.id, true, order.mining_policy).Failed()) { q.blocked = SpaceOrderBlock::HoldIncompatible; return true; }
			q.started = true; return false;
		}
		// Working on the first body delegates solely to the existing extraction
		// owner until actual completion, loss or an original service obligation.
		const auto *body = GetMutableAsteroidBody(craft.mission.target);
		if (body != nullptr && body->remaining_reserve != 0 && !HasFullMiningHold(craft) && GetSpaceMiningBlock(craft) == SpaceOrderBlock::None &&
			(craft.mission.phase == SpaceMissionPhase::Approach || craft.mission.phase == SpaceMissionPhase::Mining)) return false;
		const auto plan = PlanSpaceDeparture(craft);
		if (!BeginSpaceMiningContinuation(craft, plan.status == SpaceDepartureStatus::Ready ? plan.destination : std::nullopt)) return true;
	}
	if (intent.phase == MiningContinuationPhase::Detaching) {
		if (TickMiningDeparture(craft)) {
			// Legacy Mine originally finishes on Return/Holding. Only the local
			// departure delays that completion; subsequent Visits/work stay under
			// their own original queue dispatch, even without a handling route.
			if (order.mining_policy == SpaceMiningPolicy::LegacyReturn) CompleteMiningContinuation(craft, q.cursor + 1);
			else intent.phase = NextMiningPhase(craft);
		}
		return true;
	}
	if (intent.phase == MiningContinuationPhase::Waiting) {
		if (intent.retry_ticks != 0) { --intent.retry_ticks; return true; }
		intent.phase = NextMiningPhase(craft); intent.waiting = MiningContinuationWait::None;
	}
	if (intent.phase == MiningContinuationPhase::SeekMineral) {
		// A replacement keeps the same Mine cursor while its native controller
		// alone pursues, attaches and earns finite output on that actual body.
		if (q.started) {
			const auto *body = GetMutableAsteroidBody(craft.mission.target);
			if (body != nullptr && body->remaining_reserve != 0 && !HasFullMiningHold(craft) && GetSpaceMiningBlock(craft) == SpaceOrderBlock::None &&
				(craft.mission.phase == SpaceMissionPhase::Approach || craft.mission.phase == SpaceMissionPhase::Mining)) return false;
			const auto plan = PlanSpaceDeparture(craft); BeginSpaceMiningContinuation(craft, plan.status == SpaceDepartureStatus::Ready ? plan.destination : std::nullopt); return true;
		}
		const auto block = GetSpaceMiningBlock(craft);
		if (block != SpaceOrderBlock::None) {
			if (block == SpaceOrderBlock::ServiceRequired) { intent.phase = MiningContinuationPhase::NeedService; return true; }
			WaitForMiningContinuation(craft, MiningContinuationWait::ServiceUnavailable, block); return true;
		}
		const auto selected = FindNearestEligibleMiningTarget(craft, intent.mineral);
		if (!selected || AssignSpaceMiningMission(craft, selected->base(), true, order.mining_policy).Failed()) {
			WaitForMiningContinuation(craft, MiningContinuationWait::NoCompatibleAsteroid, SpaceOrderBlock::ProducerUnavailable); return true;
		}
		intent.effective_target = SpaceOrderTarget{SpaceOrderTargetKind::Body, selected->base(), 0}; q.started = true; q.blocked = SpaceOrderBlock::None;
		return false;
	}
	TickMiningReturn(craft); return true;
}

/** @copydoc ValidateNativeSpaceMiningContinuation */
bool ValidateNativeSpaceMiningContinuation(const SpaceVehicle &craft)
{
	const auto &intent = craft.mission.continuation;
	const auto rows = GetAsteroidIdentityRecords(GetAsteroidField());
	const bool freight = craft.cargo.TotalCount() != 0;
	const auto label = IsValidCargoType(craft.cargo_type) ? std::optional{CargoSpec::Get(craft.cargo_type)->label} : std::nullopt;
	return ValidateSpaceMiningContinuation(intent, {craft.space_orders, rows, GetAsteroidField().next_body_id,
		GetSpacePlaneBounds(Map::SizeX(), Map::SizeY()), GetSpaceVehicleModel(craft), craft.mission.phase, craft.mission.target, craft.mission.work_ticks,
		freight, IsValidCargoType(GetCargoTypeByLabel(intent.mineral)), label, GetSpaceReturnRoute(craft), GetNextSpaceStructureGeneration(), GetNextLaunchCenterGeneration()});
}

/** @copydoc ValidateSpaceMiningContinuation */
bool ValidateSpaceMiningContinuation(const SpaceMiningContinuationState &intent, const SpaceMiningContinuationContext &context)
{
	const auto &queue = context.queue;
	const bool unordered_departure = queue.entries.empty() && intent.phase == MiningContinuationPhase::Detaching;
	// Only this current original Mine may retain configured or already-active
	// intent. A future instruction cannot acquire navigation through this DTO.
	const bool configured = queue.cursor < queue.entries.size() && queue.entries[queue.cursor].action == SpaceOrderAction::Mine &&
		(queue.entries[queue.cursor].mining_policy != SpaceMiningPolicy::LegacyReturn || intent.phase != MiningContinuationPhase::None);

	// An unstarted configured Mine may wait without a pin. Actual configured
	// mining must already retain the mineral from its admitted first assignment.
	if (intent == SpaceMiningContinuationState{}) return !configured || !context.queue.started;
	const auto &q = context.queue;
	const auto &bounds = context.bounds;
	// Contained intent needs one original supported Mine, issued body history,
	// immutable mineral meaning and bounded physical/retry/travel discriminators.
	if ((!configured && !unordered_departure) || GetSpaceMiningTuning(context.model) == nullptr || !bounds ||
		intent.phase >= MiningContinuationPhase::End || intent.waiting >= MiningContinuationWait::End || intent.mineral.Empty() ||
		intent.previous_body.base() >= context.next_body_id || intent.retry_ticks > GetSpaceMiningTargetRetryDelay() || intent.travel_ticks > (1U << 24) ||
		(intent.departure_point && !bounds->Contains(*intent.departure_point)) ||
		(intent.route_cursor != UINT8_MAX && (intent.route_cursor <= q.cursor || intent.route_cursor > q.entries.size()))) return false;

	// A stable mineral label must name an original asteroid class and cannot
	// relabel an actual loaded hold or active replacement's finite source.
	if (!context.mineral_defined || (context.has_freight && context.freight_mineral != intent.mineral) ||
		std::ranges::none_of(GetMultiLayerAsteroidDefinitions(), [&](const auto &definition) { return definition.cargo == intent.mineral; })) return false;
	const auto *previous_body = FindAsteroidIdentity(context.bodies, intent.previous_body);
	// A surviving immutable source still proves the original pin, including
	// empty-hold departure/waiting. Missing retired history never borrows a class.
	if (previous_body != nullptr && GetMultiLayerAsteroidDefinitions()[to_underlying(previous_body->resource_class)].cargo != intent.mineral) return false;
	if (unordered_departure) {
		// No unqueued seek, receiver, hidden route or work is permitted. Only its
		// original saved local departure/retry may precede ordinary legacy Return.
		return context.mission_phase == SpaceMissionPhase::Holding && context.mission_target == intent.previous_body && context.work_ticks == 0 &&
			!queue.started && queue.timer_ticks == 0 && queue.work_ticks == 0 && queue.blocked == SpaceOrderBlock::None && !queue.approach &&
			!intent.effective_target && intent.route_cursor == UINT8_MAX && intent.travel_ticks == 0 &&
			(intent.waiting == MiningContinuationWait::None || intent.waiting == MiningContinuationWait::DepartureBlocked);
	}
	const auto *active_body = FindAsteroidIdentity(context.bodies, context.mission_target);
	// An empty hold still keeps the originally admitted mineral; active work
	// cannot replace that pin with a different otherwise-valid mineral label.
	if ((context.mission_phase == SpaceMissionPhase::Approach || context.mission_phase == SpaceMissionPhase::Mining) &&
		(active_body == nullptr || GetMultiLayerAsteroidDefinitions()[to_underlying(active_body->resource_class)].cargo != intent.mineral)) return false;
	// A pin before departure belongs to the actually started Mine, including
	// its real Return/Holding completion checkpoint. Idle cannot claim that work.
	if (intent.phase == MiningContinuationPhase::None) return q.started && context.mission_phase != SpaceMissionPhase::Idle && !intent.effective_target && !intent.departure_point && intent.retry_ticks == 0 &&
		intent.waiting == MiningContinuationWait::None && intent.route_cursor == UINT8_MAX && intent.travel_ticks == 0;
	if (intent.route_cursor == UINT8_MAX) return false;
	// Each phase has canonical intent. Forged waiting/departure targets cannot
	// acquire a berth or retain hidden travel/service progress after load.
	if ((intent.phase == MiningContinuationPhase::Waiting && (intent.waiting == MiningContinuationWait::None || intent.effective_target || intent.departure_point || intent.travel_ticks != 0)) ||
		(intent.phase == MiningContinuationPhase::Detaching && (intent.effective_target || intent.travel_ticks != 0)) ||
		(intent.phase != MiningContinuationPhase::Detaching && intent.departure_point)) return false;
	const auto original_route = context.route;
	if (original_route && intent.route_cursor > original_route->handling_index) return false;
	if (intent.effective_target) {
		const auto &target = *intent.effective_target;
		if (target.kind == SpaceOrderTargetKind::Body) {
			const auto *body = FindAsteroidIdentity(context.bodies, OrbitalBodyID{target.id});
			// Only a real currently pursued/mined replacement may carry a body
			// effective target; detached historical bodies confer no active lease.
			if (intent.phase != MiningContinuationPhase::SeekMineral || !q.started || target.generation != 0 || body == nullptr ||
				context.mission_target != body->id || GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo != intent.mineral) return false;
		} else {
			const auto route = context.route;
			// Capacity/availability may change after selection; identity and domain
			// remain meaningful while the controller revalidates before any work.
			if (!route || target.kind != route->domain || target.generation == 0 ||
				(intent.phase != MiningContinuationPhase::ReturnCargo && intent.phase != MiningContinuationPhase::NeedService)) return false;
			// A removed receiver may remain historical, but an unissued future
			// incarnation or out-of-domain numeric ID cannot acquire later rights.
			if (target.kind == SpaceOrderTargetKind::Structure ? target.id >= SpaceStructureID::End().base() || target.generation >= context.next_structure_generation :
				target.id >= StationID::End().base() || target.generation >= context.next_center_generation) return false;
		}
	}
	if (q.started) return intent.phase == MiningContinuationPhase::SeekMineral && intent.effective_target.has_value();
	return context.mission_phase == SpaceMissionPhase::Holding && context.work_ticks == 0;
}
