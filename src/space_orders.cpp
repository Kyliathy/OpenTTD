/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_orders.cpp Typed queue execution over the canonical Vehicle, mining and construction owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups17/23/26; checked policy edits, intent cancellation and current natural Survey/Visit loss.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group6.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group3.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; genuine Visit arrivals feed the native automatic replacement queue.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B; reached freight/service instructions delegate to their actual physical producers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group14; explicit paid-pod return inside the owned center's circle.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9C; appended heavy mining retains the original queue/mission owner.
 */
#include "stdafx.h"
#include "space_orders.h"
#include "space_order_actions.h"
#include "space_replacement.h"
#include "space_approach.h"
#include "space_vehicle.h"
#include "space_mining_tuning.h"
#include "space_mining_continuation.h"
#include "space_return_target.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_structure.h"
#include "orbital_service.h"
#include "space_vehicle_cmd.h"
#include "asteroid.h"
#include "launch_center.h"
#include "station_base.h"
#include "engine_base.h"
#include "company_base.h"
#include "multilayer_space_content.h"
#include "command_func.h"
#include "map_func.h"
#include "timer/timer_game_tick.h"
#include <set>
#include "safeguards.h"

static uint64_t _next_space_vehicle_generation = 1;
/** @copydoc GetNextSpaceVehicleGeneration */
uint64_t GetNextSpaceVehicleGeneration() { return _next_space_vehicle_generation; }
/** @copydoc ResetSpaceOrders */
void ResetSpaceOrders() { _next_space_vehicle_generation = 1; ResetSpaceOrderLoadStaging(); }
/** @copydoc RegisterSpaceVehicleIdentity */
bool RegisterSpaceVehicleIdentity(SpaceVehicle &vehicle)
{
	// Commissioning issues one new incarnation only; reusing an issued or exhausted identity would retarget other vehicles orders.
	if (vehicle.generation != 0 || _next_space_vehicle_generation == UINT64_MAX) return false;
	vehicle.generation = _next_space_vehicle_generation++;
	return true;
}

/** @param order Copied typed instruction. @return Whether its action/domain/identity bounds are meaningful before any producer lookup. */
static bool ValidOrder(const SpaceOrder &order)
{
	// Reject unknown roles/domains and absent identities before consulting any physical producer.
	if (order.action >= SpaceOrderAction::End || order.target.kind >= SpaceOrderTargetKind::End || order.target.id == UINT32_MAX) return false;
	// Only Mine instructions may carry a configured continuation choice; unknown
	// policies and policy payloads on navigation/work actions cannot acquire mining.
	if (order.mining_policy >= SpaceMiningPolicy::End || (order.action != SpaceOrderAction::Mine && order.mining_policy != SpaceMiningPolicy::LegacyReturn)) return false;
	if (order.target.kind == SpaceOrderTargetKind::Body ? order.target.generation != 0 : order.target.generation == 0) return false;
	// Launch destinations use actual station IDs; another domain cannot bypass that bounded pool.
	if (order.target.kind == SpaceOrderTargetKind::LaunchCenter && order.target.id >= StationID::Invalid().base()) return false;
	// Craft targets remain within the native vehicle pool, preserving generation-qualified delivery identity.
	if (order.target.kind == SpaceOrderTargetKind::Craft && order.target.id >= VehicleID::End().base()) return false;
	// Static targets share the one bounded structure pool while retaining distinct blueprint/action meaning.
	if ((order.target.kind == SpaceOrderTargetKind::Structure || order.target.kind == SpaceOrderTargetKind::Blueprint) && order.target.id >= SpaceStructureID::End().base()) return false;
	switch (order.action) {
		case SpaceOrderAction::Visit: return order.target.kind != SpaceOrderTargetKind::Blueprint;
		case SpaceOrderAction::Survey:
		case SpaceOrderAction::Mine: return order.target.kind == SpaceOrderTargetKind::Body;
		case SpaceOrderAction::Construct: return order.target.kind == SpaceOrderTargetKind::Blueprint;
		case SpaceOrderAction::Unload:
		case SpaceOrderAction::ReturnService: return order.target.kind == SpaceOrderTargetKind::Structure || order.target.kind == SpaceOrderTargetKind::LaunchCenter;
		default: return false;
	}
}

/** @copydoc IsSpaceOrderAllowed */
bool IsSpaceOrderAllowed(uint16_t model, const SpaceOrder &order)
{
	if (!ValidOrder(order)) return false;
	// Only commissioned Space roles enter this controller; ordinary vehicles cannot acquire orbital actions.
	if (model != 15 && model != 16 && model != 17 && model != 19 && model != 20 && model != SPACE_HEAVY_MINER_MODEL) return false;
	switch (order.action) {
		case SpaceOrderAction::Survey: return model == 19;
		case SpaceOrderAction::Mine: return GetSpaceMiningTuning(model) != nullptr;
		case SpaceOrderAction::Construct: return model == 16 || model == 20;
		case SpaceOrderAction::Unload: return model != 19;
		case SpaceOrderAction::Visit:
		case SpaceOrderAction::ReturnService: return true;
		default: return false;
	}
}

/** @copydoc ValidateSpaceOrderRecords */
bool ValidateSpaceOrderRecords(std::span<const SpaceOrderRecord> records, uint64_t next)
{
	// Saved issuance and total hull count bound every queue before per-vehicle validation allocates copied sets.
	if (next == 0 || records.size() > MAX_SPACE_VEHICLES) return false;
	std::set<VehicleID> vehicles; std::set<uint64_t> generations;
	for (const auto &record : records) {
		const auto &q = record.queue;
		// Saved incarnations, cursor positions and action-owned timers must agree so a stale wait cannot become paid work.
		if (record.vehicle == VehicleID::Invalid() || record.generation == 0 || record.generation >= next || !vehicles.insert(record.vehicle).second || !generations.insert(record.generation).second ||
			q.entries.size() > MAX_SPACE_ORDERS || q.cursor > q.entries.size() || q.blocked >= SpaceOrderBlock::End || q.work_ticks >= GetMultiLayerSpaceBudget().scan_ticks || q.timer_ticks > (1U << 24) ||
			(q.work_ticks != 0 && (!q.started || (q.timer_ticks == 0 && q.blocked != SpaceOrderBlock::Timeout) || q.cursor >= q.entries.size() || q.entries[q.cursor].action != SpaceOrderAction::Survey)) ||
			(q.cursor == q.entries.size() && (q.started || q.timer_ticks != 0)) || (!q.started && q.timer_ticks != 0) ||
			(q.blocked == SpaceOrderBlock::Completed && (q.entries.empty() || q.cursor != q.entries.size())) ||
			(q.entries.empty() && (q.cursor != 0 || q.started || q.blocked != SpaceOrderBlock::None))) return false;
		// Every queued craft target must be an already issued incarnation; future targets cannot inherit later hulls.
		for (const auto &order : q.entries) if (!ValidOrder(order) || (order.target.kind == SpaceOrderTargetKind::Craft && order.target.generation >= next)) return false;
		// Paid return/service waits belong to a held Mine or the reached explicit Return/Service action; unrelated actions cannot inherit another owner's allowance or service state.
		if (q.blocked >= SpaceOrderBlock::ReturnsPending && q.blocked <= SpaceOrderBlock::PodsMissing && !IsSpaceMiningServiceWait(q) && !IsSpaceReturnServiceWait(q)) return false;
		// Construction waits require the same live started action/deadline rather than hiding progress in another role.
		if (q.blocked >= SpaceOrderBlock::MaterialsMissing && q.blocked <= SpaceOrderBlock::BuilderBusy && (!q.started || q.timer_ticks == 0 || q.cursor >= q.entries.size() || q.entries[q.cursor].action != SpaceOrderAction::Construct)) return false;
		// A queued approach wait cannot hide work or lose the hull's sole physical request.
		if (q.blocked == SpaceOrderBlock::ApproachBusy && !q.approach) return false;
	}
	return ValidateSpaceApproachRecords(records);
}

/** @copydoc CaptureSpaceOrders */
std::vector<SpaceOrderRecord> CaptureSpaceOrders()
{
	std::vector<SpaceOrderRecord> records;
	for (const auto *vehicle : SpaceVehicle::Iterate()) records.push_back({vehicle->index, vehicle->generation, vehicle->space_orders});
	return records;
}

/** @copydoc ValidateNativeSpaceOrders */
bool ValidateNativeSpaceOrders()
{
	const auto records = CaptureSpaceOrders();
	if (!ValidateSpaceOrderRecords(records, _next_space_vehicle_generation)) return false;
	// Cross-check every actual hull and typed target against copied queue bounds without acquiring a new work or mining lease.
	for (const auto *v : SpaceVehicle::Iterate()) {
		// Queue ownership requires an actual Space engine; an ordinary vehicle cannot inherit these orbital controls.
		if (!Engine::IsValidID(v->engine_type) || v->GetEngine()->type != VehicleType::Space) return false;
		const auto &q = v->space_orders;
		if (!ValidateNativeSpaceMiningContinuation(*v)) return false;
		const auto model = v->GetEngine()->VehInfo<SpaceVehicleInfo>().model;
		// Revalidate each instruction against the actual hull role after content and engine reconstruction.
		for (const auto &o : q.entries) if (!IsSpaceOrderAllowed(model, o)) return false;
		// Original natural targets may outlive retirement as inert history, but
		// no instruction may reserve a body identity that has never been issued.
		for (const auto &o : q.entries) if (o.target.kind == SpaceOrderTargetKind::Body && o.target.id >= GetAsteroidField().next_body_id) return false;
		// Resolve only structure-domain incarnations here; other target families keep their own native authority.
		for (const auto &o : q.entries) if (o.target.kind == SpaceOrderTargetKind::Structure || o.target.kind == SpaceOrderTargetKind::Blueprint) {
			if (o.target.generation >= GetNextSpaceStructureGeneration()) return false;
			const auto *site = ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(o.target.id)}, o.target.generation});
			// Removed/unowned sites may retain history, but a live foreign module cannot silently supply another company.
			if (site != nullptr && site->state.owner != OWNER_NONE && site->state.owner != v->owner) return false;
		}
		// A retained removed-target wait remains inert; other live craft targets must still belong to this fleet.
		for (const auto &o : q.entries) if (o.target.kind == SpaceOrderTargetKind::Craft && q.blocked != SpaceOrderBlock::TargetRemoved) {
			const auto *target = ResolveSpaceCraft({VehicleID{o.target.id}, o.target.generation});
			// A surviving delivery target must retain the asking fleet owner before its approach can be used.
			if (target != nullptr && target->owner != v->owner) return false;
		}
		if (q.entries.empty()) continue;
		if (v->construction.placement != SpacePlacement::Orbital) return false;
		// Partial survey work requires its actual natural body; losing the producer cannot preserve actionable scan progress.
		if (q.work_ticks != 0 && GetMutableAsteroidBody(OrbitalBodyID{q.entries[q.cursor].target.id}) == nullptr) return false;
		// An active Mine queue must retain the same native mission target/lease instead of acquiring extraction through another action.
		if (q.started && q.cursor < q.entries.size() && q.entries[q.cursor].action == SpaceOrderAction::Mine) {
			const auto effective = ResolveEffectiveSpaceOrderTarget(*v);
			// A native completion publishes Return after queue dispatch. The next
			// continuation tick owns departure; no historical body request should
			// be required during this real between-opportunity checkpoint.
			const bool completing = HasSpaceMiningContinuationOrder(*v) &&
				(v->mission.phase == SpaceMissionPhase::Return || v->mission.phase == SpaceMissionPhase::Holding);
			// Replacement mining retains the player's original Mine target as
			// history; only the effective current body may own active native work.
			if (!completing && (!effective || effective->kind != SpaceOrderTargetKind::Body || v->mission.target != OrbitalBodyID{effective->id})) return false;
		} else if (v->mission.phase != SpaceMissionPhase::Holding) return false;
	}
	return ValidateNativeSpaceApproaches();
}

/** @copydoc InstallLoadedSpaceOrders */
bool InstallLoadedSpaceOrders(std::span<const SpaceOrderRecord> records, uint64_t next)
{
	// All native hulls must be accounted for before queue installation can mutate any vehicle.
	if (!ValidateSpaceOrderRecords(records, next) || records.size() != GetSpaceVehicleCount() ||
		!ValidateSpaceApproachRecords(records, TimerGameTick::counter, Map::SizeX(), Map::SizeY())) return false;
	for (const auto &record : records) {
		const auto *v = SpaceVehicle::GetIfValid(record.vehicle);
		// Every copied queue must resolve to the exact native Space hull family before the all-row installation commit.
		if (v == nullptr || !Engine::IsValidID(v->engine_type) || v->GetEngine()->type != VehicleType::Space) return false;
		const auto *info = &v->GetEngine()->VehInfo<SpaceVehicleInfo>();
		if (info == nullptr) return false;
		// Preflight all instructions against the restored hull role before installing any copied queue.
		for (const auto &order : record.queue.entries) if (!IsSpaceOrderAllowed(info->model, order)) return false;
	}
	// After the whole preflight, install only the existing hulls generations and queues in stable saved order.
	for (const auto &record : records) { auto *v = SpaceVehicle::Get(record.vehicle); v->generation = record.generation; v->space_orders = record.queue; }
	_next_space_vehicle_generation = next;
	return true;
}

/** @copydoc CancelSpaceOrders */
void CancelSpaceOrders(SpaceVehicle &vehicle)
{
	ReleaseSpaceMiningSlot(vehicle); vehicle.space_orders = {};
	vehicle.mission.continuation = {}; // This queue owns the sole active intent; cancelling it never disposes the original freight.
	vehicle.mission.target = INVALID_ORBITAL_BODY; vehicle.mission.phase = SpaceMissionPhase::Holding; vehicle.mission.approach_ticks_left = 0;
}

/** @copydoc ForgetSpaceBodySurveyOrders */
void ForgetSpaceBodySurveyOrders(std::span<const OrbitalBodyID> bodies)
{
	// Native capacity bounds this pass. Only a current affected natural Survey
	// or Visit loses its own approach/work; future orders and paid static jobs
	// retain their exact original owners and completed company disclosure.
	for (auto *craft : SpaceVehicle::Iterate()) {
		auto &q = craft->space_orders;
		if (q.cursor >= q.entries.size()) continue;
		const auto &order = q.entries[q.cursor];
		if ((order.action != SpaceOrderAction::Survey && order.action != SpaceOrderAction::Visit) || order.target.kind != SpaceOrderTargetKind::Body ||
			order.target.generation != 0 || std::ranges::find(bodies, OrbitalBodyID{order.target.id}) == bodies.end()) continue;
		ReleaseSpaceApproach(*craft); q.started = false; q.timer_ticks = 0; q.work_ticks = 0; q.blocked = SpaceOrderBlock::TargetRemoved;
		craft->mission.phase = SpaceMissionPhase::Holding; craft->mission.approach_ticks_left = 0;
	}
}

/** @copydoc ResolveSpaceNavigationTarget */
SpaceOrderBlock ResolveSpaceNavigationTarget(const SpaceVehicle &vehicle, const SpaceOrderTarget &target, SpacePoint &point)
{
	// Unknown target domains and absent IDs cannot resolve to coordinates or manufacture a navigation producer.
	if (target.kind >= SpaceOrderTargetKind::End || target.id == UINT32_MAX ||
		(target.kind == SpaceOrderTargetKind::Body ? target.generation != 0 : target.generation == 0) ||
		(target.kind == SpaceOrderTargetKind::LaunchCenter && target.id >= StationID::End().base()) ||
		(target.kind == SpaceOrderTargetKind::Craft && target.id >= VehicleID::End().base())) return SpaceOrderBlock::TargetRemoved;
	if (target.kind == SpaceOrderTargetKind::Body) {
		const auto *body = GetMutableAsteroidBody(OrbitalBodyID{target.id});
		if (body == nullptr) return SpaceOrderBlock::TargetRemoved;
		point = {body->motion.position.x + vehicle.mission.attachment_offset.x, body->motion.position.y + vehicle.mission.attachment_offset.y};
		return SpaceOrderBlock::None;
	}
	if (target.kind == SpaceOrderTargetKind::LaunchCenter) {
		const auto *center = ResolveLaunchCenter(LaunchCenterRef{StationID{static_cast<uint16_t>(target.id)}, target.generation});
		// An exact surviving owned launch anchor supplies the destination; foreign or reused station IDs are removed targets.
		if (center == nullptr || center->owner != vehicle.owner) return SpaceOrderBlock::TargetRemoved;
		const auto reference = GetLaunchCenterPlaneReference(center->launch_center->tile);
		if (!reference) return SpaceOrderBlock::TargetRemoved;
		point = *reference;
		return SpaceOrderBlock::None;
	}
	if (target.kind == SpaceOrderTargetKind::Craft) {
		const auto *other = ResolveSpaceCraft({VehicleID{target.id}, target.generation});
		// A craft destination must be another owned deployed hull, preventing self-delivery and foreign cargo exchange.
		if (other == nullptr || other == &vehicle || other->owner != vehicle.owner || other->construction.placement != SpacePlacement::Orbital) return SpaceOrderBlock::TargetRemoved;
		point = other->position; return SpaceOrderBlock::None;
	}
	// Static frames and blueprints resolve through one native site owner, without a proxy station or footprint.
	if (target.kind == SpaceOrderTargetKind::Structure || target.kind == SpaceOrderTargetKind::Blueprint) {
		if (target.id >= SpaceStructureID::End().base()) return SpaceOrderBlock::TargetRemoved;
		const auto *site = ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(target.id)}, target.generation});
		// Cancellation removes construction admission while retaining the owned Structure approach for explicit material collection.
		if (site == nullptr || site->state.owner != vehicle.owner || (site->state.phase == SpaceStructurePhase::Cancelled &&
			(target.kind == SpaceOrderTargetKind::Blueprint || (site->job.inputs[0].TotalCount() == 0 && site->job.inputs[1].TotalCount() == 0)))) return SpaceOrderBlock::TargetRemoved;
		point = GetSpaceStructureApproach(site->state); return SpaceOrderBlock::None;
	}
	return SpaceOrderBlock::ProducerUnavailable;
}

/** Edit one exact owned native queue without granting work or commissioning a hull.
 * @param flags Estimate or execute. @param id Native hull ID. @param generation Exact hull incarnation.
 * @param edit Explicit bounded edit. @param index Existing/insertion queue position. @param action Requested native role.
 * @param kind Target domain. @param target Actual target ID. @param target_generation Exact reusable-target incarnation.
 * @return Unchanged refusal or synchronized queue edit, with no material/payment/work side effects.
 */
CommandCost CmdEditSpaceOrder(DoCommandFlags flags, VehicleID id, uint64_t generation, SpaceOrderEdit edit, uint8_t index, SpaceOrderAction action, SpaceOrderTargetKind kind, uint32_t target, uint64_t target_generation)
{
	auto *v = SpaceVehicle::GetIfValid(id);
	// Only the exact owned deployed unstolen hull may edit its queue; onboard service retains physical control of its craft.
	if (v == nullptr || v->construction.placement != SpacePlacement::Orbital || (v->manifest && v->manifest->service_ticks != 0) || HasOrbitalServiceAssignment(*v) || generation == 0 || v->generation != generation || v->owner != _current_company || v->vehstatus.Test(VehState::Crashed) ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || edit >= SpaceOrderEdit::End) return CMD_ERROR;
	// Queue ownership requires an actual Space engine; an ordinary vehicle cannot inherit these orbital controls.
	if (!Engine::IsValidID(v->engine_type) || v->GetEngine()->type != VehicleType::Space) return CMD_ERROR;
	const auto *info = &v->GetEngine()->VehInfo<SpaceVehicleInfo>();
	if (info == nullptr) return CMD_ERROR;
	const SpaceOrder order{action, {kind, target, target_generation}};
	const auto size = v->space_orders.entries.size();
	// New/replaced instructions require role, bounded position and actual target admission before the queue changes.
	if (edit == SpaceOrderEdit::Insert || edit == SpaceOrderEdit::Replace) {
		// Insertion cannot overbook the fixed queue or substitute an action unsupported by the paid hull.
		if (!IsSpaceOrderAllowed(info->model, order) || index > size || (edit == SpaceOrderEdit::Replace && index == size) || (edit == SpaceOrderEdit::Insert && size == MAX_SPACE_ORDERS)) return CMD_ERROR;
		SpacePoint point;
		if (ResolveSpaceNavigationTarget(*v, order.target, point) != SpaceOrderBlock::None) return CMD_ERROR;
	// Erasure requires an existing entry so it cannot change a later incarnation or an absent instruction.
	} else if (edit == SpaceOrderEdit::Erase && index >= size) return CMD_ERROR;
	// Moving upward preserves existing queue bounds and cannot move before the first instruction.
	else if (edit == SpaceOrderEdit::MoveUp && (index == 0 || index >= size)) return CMD_ERROR;
	// Moving downward preserves existing queue bounds and cannot append an unintended instruction.
	else if (edit == SpaceOrderEdit::MoveDown && size_t{index} + 1 >= size) return CMD_ERROR;
	// Mixed Execute/QueryCost is still an estimate; inspecting a valid instruction must not restart work, release a berth or alter pending replacement running intent.
	if (flags.Test(DoCommandFlag::Execute) && !flags.Test(DoCommandFlag::QueryCost)) {
		auto queue = v->space_orders.entries;
		switch (edit) {
			case SpaceOrderEdit::Insert: queue.insert(queue.begin() + index, order); break;
			case SpaceOrderEdit::Replace: queue[index] = order; break;
			case SpaceOrderEdit::Erase: queue.erase(queue.begin() + index); break;
			case SpaceOrderEdit::Clear: queue.clear(); break;
			case SpaceOrderEdit::MoveUp: std::iter_swap(queue.begin() + index, queue.begin() + index - 1); break;
			case SpaceOrderEdit::MoveDown: std::iter_swap(queue.begin() + index, queue.begin() + index + 1); break;
			default: return CMD_ERROR;
		}
		CancelSpaceOrders(*v); v->space_orders.entries = std::move(queue);
		v->vehstatus.Reset(VehState::Stopped);
		SetPendingSpaceReplacementResume(GetSpaceCraftReference(*v), true); // Editing native instructions explicitly starts this original hull; its actual pending producer must preserve that later running choice at handover.
		v->MarkDirty();
	}
	return CommandCost{};
}

/** @copydoc CmdSetSpaceMiningPolicy */
CommandCost CmdSetSpaceMiningPolicy(DoCommandFlags flags, VehicleID id, uint64_t generation, uint8_t index, SpaceMiningPolicy policy)
{
	auto *craft = SpaceVehicle::GetIfValid(id);
	// Only the exact owned deployed miner may change its Mine intent. Actual
	// onboard/module service retains physical control even when it is paused.
	if (craft == nullptr || generation == 0 || craft->generation != generation || craft->owner != _current_company ||
		craft->construction.placement != SpacePlacement::Orbital || craft->vehstatus.Test(VehState::Crashed) ||
		(craft->manifest && craft->manifest->service_ticks != 0) || HasOrbitalServiceAssignment(*craft) ||
		GetSpaceMiningTuning(GetSpaceVehicleModel(*craft)) == nullptr || policy >= SpaceMiningPolicy::End ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return CMD_ERROR;
	auto &queue = craft->space_orders;
	// This command configures one existing Mine; it cannot insert work, select a
	// different body or attach mining behavior to another role's instruction.
	if (index >= queue.entries.size() || queue.entries[index].action != SpaceOrderAction::Mine) return CMD_ERROR;
	const bool current = index == queue.cursor;
	auto intent = craft->mission.continuation;
	// Opting an already active legacy Mine into a new policy pins its actual
	// admitted body now, including an empty hold. A missing source is never
	// reconstructed from a cargo slot or from the original GUI selection.
	if (current && queue.started && policy != SpaceMiningPolicy::LegacyReturn && intent.mineral.Empty()) {
		const auto *body = FindAsteroidBody(GetAsteroidField(), craft->mission.target);
		if (body == nullptr) return CMD_ERROR;
		intent.mineral = GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo;
		intent.previous_body = body->id;
	}
	// A live safe-departure/service action keeps its physical progress when the
	// player changes preference. An inactive legacy choice needs no pinned intent.
	if (current && policy == SpaceMiningPolicy::LegacyReturn && intent.phase == MiningContinuationPhase::None) intent = {};
	if (current && intent.phase == MiningContinuationPhase::Waiting) intent.retry_ticks = 0;
	// Estimates, including mixed flags, cannot change policy, mineral, wait or
	// ongoing work. Registration/UI exposure remains gated on Group23's reader.
	if (flags.Test(DoCommandFlag::Execute) && !flags.Test(DoCommandFlag::QueryCost)) {
		queue.entries[index].mining_policy = policy;
		if (current) craft->mission.continuation = intent;
		craft->MarkDirty();
	}
	return CommandCost{};
}

/** @copydoc IsSpaceSurveyWorkTick */
bool IsSpaceSurveyWorkTick(const SpaceVehicle &vehicle)
{
	const auto &q = vehicle.space_orders;
	// Survey work/upkeep share actual deployed unstopped hull and deadline admission; waiting or serviced craft earn no scan.
	if (GetSpaceVehicleModel(vehicle) != 19 || vehicle.construction.placement != SpacePlacement::Orbital || vehicle.vehstatus.Test(VehState::Stopped) ||
		(vehicle.manifest && vehicle.manifest->service_ticks != 0) || q.cursor >= q.entries.size() || q.blocked == SpaceOrderBlock::Timeout || (q.started && q.timer_ticks == 0)) return false;
	const auto &o = q.entries[q.cursor];
	// Only an explicit natural-body Survey action may accrue action-owned scan work.
	if (o.action != SpaceOrderAction::Survey || o.target.kind != SpaceOrderTargetKind::Body) return false;
	const auto *body = GetMutableAsteroidBody(OrbitalBodyID{o.target.id}); if (body == nullptr) return false;
	SpacePoint admitted;
	if (GetSpaceApproachDestination(vehicle, admitted) != SpaceOrderBlock::None) return false;
	const SpacePoint point{body->motion.position.x + vehicle.mission.attachment_offset.x, body->motion.position.y + vehicle.mission.attachment_offset.y};
	const uint64_t tolerance = GetMultiLayerMiningProbeTuning().approach_tolerance_q16;
	return SpaceDistanceSquared(vehicle.position, point) <= tolerance * tolerance;
}

/** @copydoc IsSpaceMiningServiceWait */
bool IsSpaceMiningServiceWait(const SpaceOrderQueue &q)
{
	return !q.started && q.cursor < q.entries.size() && q.entries[q.cursor].action == SpaceOrderAction::Mine &&
		(q.blocked == SpaceOrderBlock::ReturnsPending || q.blocked == SpaceOrderBlock::ServiceRequired || q.blocked == SpaceOrderBlock::PodsMissing);
}

/** Prove a podless heavy miner has an explicit physical orbital return route.
 * @param craft Actual paid hull whose current Mine may acquire the finite-resource lease.
 * @return Whether its first subsequent handling action is an owned operational module Unload for the target's mineral.
 */
static bool HasHeavyMiningModuleReturn(const SpaceVehicle &craft)
{
	const auto &queue = craft.space_orders;
	// Only the appended station-built miner may replace a shell requirement with its own explicit module route; old15 and legacy unordered missions retain their delivered pod rule.
	if (GetSpaceVehicleModel(craft) != SPACE_HEAVY_MINER_MODEL || queue.cursor >= queue.entries.size() || queue.entries[queue.cursor].action != SpaceOrderAction::Mine) return false;
	CargoLabel mineral = craft.mission.continuation.mineral;
	// Configured continuation retains its originally admitted mineral after body
	// retirement. Legacy/unpinned instructions still require their actual source;
	// an empty label or current cargo slot cannot invent a module return route.
	if ((queue.entries[queue.cursor].mining_policy == SpaceMiningPolicy::LegacyReturn && craft.mission.continuation.phase == MiningContinuationPhase::None) || mineral.Empty()) {
		const auto *body = GetMutableAsteroidBody(OrbitalBodyID{queue.entries[queue.cursor].target.id});
		if (body == nullptr) return false;
		mineral = GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo;
	}
	const auto cargo = GetCargoTypeByLabel(mineral);
	if (!IsValidCargoType(cargo)) return false;
	// Configured nearest returns retain their allowed module domain after the
	// originally specified site disappears. Another real owned compatible module
	// supplies the return function; no shell, capacity or unload is granted here.
	if (queue.entries[queue.cursor].mining_policy != SpaceMiningPolicy::LegacyReturn) {
		const auto route = GetSpaceReturnRoute(craft);
		if (!route || route->domain != SpaceOrderTargetKind::Structure || route->declared_action != SpaceOrderAction::Unload) return false;
		for (const auto *site : SpaceStructure::Iterate()) {
			if (site->state.owner == craft.owner && IsOrbitalModuleOperational(*site) && GetOrbitalModuleCargoCapacity(*site, cargo) != 0) return true;
		}
		return false;
	}
	// Visits may navigate between the mine and receiver. Another work/handling action cannot hide an Earth return, unavailable processor or intervening mining trip behind a later valid module.
	for (size_t index = queue.cursor + 1; index < queue.entries.size(); ++index) {
		const auto &order = queue.entries[index];
		if (order.action == SpaceOrderAction::Visit) continue;
		// The first actual unload must identify a real module incarnation; coordinates and Surface launch regions grant no orbital receiving function.
		if (order.action != SpaceOrderAction::Unload || order.target.kind != SpaceOrderTargetKind::Structure) return false;
		const auto *site = ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(order.target.id)}, order.target.generation});
		return site != nullptr && site->state.owner == craft.owner && GetOrbitalModuleCargoCapacity(*site, cargo) != 0;
	}
	return false;
}

/** @copydoc GetSpaceMiningBlock */
SpaceOrderBlock GetSpaceMiningBlock(const SpaceVehicle &v)
{
	// Retained P2 craft keep their old mining behavior; they gain no free return allowance.
	// Historical unpaid probes keep their bounded old mining behavior without receiving new paid return/service inventory.
	if (v.construction.content_version == 0 && (!v.manifest || v.manifest->services_completed == 0)) return SpaceOrderBlock::None;
	// Paid mining requires an actual idle serviced manifest and remaining allowance; no query creates replacement supplies.
	if (!v.manifest || v.manifest->service_ticks != 0 || v.manifest->returns_remaining == 0) return SpaceOrderBlock::ServiceRequired;
	if (v.manifest->returns_remaining <= v.manifest->pending_returns) return SpaceOrderBlock::ReturnsPending;
	// A separately paid shell remains mandatory for Earth recovery. Only21's explicit operational module route can use its actual orbital freight owner instead.
	if (v.manifest->pods.empty() && !HasHeavyMiningModuleReturn(v)) return SpaceOrderBlock::PodsMissing;
	return SpaceOrderBlock::None;
}

/** @copydoc IsSpaceMiningWorkTick */
bool IsSpaceMiningWorkTick(const SpaceVehicle &v)
{
	const auto *body = GetMutableAsteroidBody(v.mission.target);
	return v.mission.phase == SpaceMissionPhase::Mining && !v.vehstatus.Test(VehState::Stopped) && GetSpaceMiningBlock(v) == SpaceOrderBlock::None &&
		body != nullptr && body->mining_slot == v.index && body->remaining_reserve != 0 && v.cargo.TotalCount() < v.cargo_cap;
}

/** @copydoc TickSpaceOrders */
bool TickSpaceOrders(SpaceVehicle &vehicle)
{
	auto &queue = vehicle.space_orders;
	// Mission-owned legacy local departure has no order to index. Configured
	// Mine continuation also runs before missing-original-target completion.
	if (HasSpaceMiningContinuationOrder(vehicle)) return TickSpaceMiningContinuation(vehicle);
	if (queue.entries.empty()) return false;
	if (queue.cursor == queue.entries.size()) { queue.blocked = SpaceOrderBlock::Completed; return true; }
	const auto &order = queue.entries[queue.cursor];
	if (queue.blocked == SpaceOrderBlock::Timeout) return true; // Explicit edit is required to retry a spent frozen deadline.
	SpacePoint destination;
	queue.blocked = ResolveSpaceNavigationTarget(vehicle, order.target, destination);
	if (queue.blocked != SpaceOrderBlock::None) {
		ReleaseSpaceMiningSlot(vehicle); ReleaseSpaceApproach(vehicle); vehicle.mission.phase = SpaceMissionPhase::Holding; queue.started = false; queue.timer_ticks = 0; queue.work_ticks = 0; return true;
	}
	/** Finish only the current action, releasing its actual lease and clearing its action-owned partial timers. */
	auto complete = [&] {
		ReleaseSpaceMiningSlot(vehicle); vehicle.mission.phase = SpaceMissionPhase::Holding; vehicle.mission.approach_ticks_left = 0;
		vehicle.mission.continuation = {}; // The completed instruction cannot lend its pinned mineral or pending destination to the next action.
		++queue.cursor; queue.started = false; queue.timer_ticks = 0; queue.work_ticks = 0;
		queue.blocked = queue.cursor == queue.entries.size() ? SpaceOrderBlock::Completed : SpaceOrderBlock::None;
	};
	if (order.action == SpaceOrderAction::Mine) {
		// A returning/held completed mining mission releases its own action rather than restarting extraction on the same lease.
		if (queue.started && (vehicle.mission.phase == SpaceMissionPhase::Return || vehicle.mission.phase == SpaceMissionPhase::Holding)) { complete(); return true; }
		if (const auto block = GetSpaceMiningBlock(vehicle); block != SpaceOrderBlock::None) {
			ReleaseSpaceMiningSlot(vehicle); vehicle.mission.phase = SpaceMissionPhase::Holding; vehicle.mission.approach_ticks_left = 0;
			queue.started = false; queue.timer_ticks = 0; queue.work_ticks = 0; queue.blocked = block; return true;
		}
		if (!queue.started) {
			if (AssignSpaceMiningMission(vehicle, order.target.id, true, order.mining_policy).Failed()) { queue.blocked = SpaceOrderBlock::HoldIncompatible; return true; }
			queue.started = true;
		}
		return false; // Existing native lease/reserve/packet implementation is the only extraction owner.
	}
	// Cargo descent has a circular receiving region and its own paid owner; it must not inherit Visit's exact-anchor rendezvous or orbital-module unloading authority.
	if (order.action == SpaceOrderAction::Unload && order.target.kind == SpaceOrderTargetKind::LaunchCenter) {
		const LaunchCenterRef center{StationID{static_cast<uint16_t>(order.target.id)}, order.target.generation};
		if (vehicle.cargo.TotalCount() == 0) { complete(); return true; }
		queue.started = true; queue.timer_ticks = 0; queue.work_ticks = 0;
		if (!IsInsideLaunchCenterDropZone(vehicle.position, destination)) {
			vehicle.cur_speed = 4;
			MoveSpaceSurveyTowards(vehicle.position, destination, vehicle.mission.travel_remainder);
			vehicle.UpdateSpacePosition(); // Release provenance must use this tick's actual physical tile.
		}
		if (!IsInsideLaunchCenterDropZone(vehicle.position, destination)) return true;
		// No shell, service quota or recovery headroom may be invented by an order; an unavailable producer retains the original hold for explicit recovery/edit.
		if (!vehicle.manifest || vehicle.manifest->pods.empty()) { queue.blocked = SpaceOrderBlock::ProducerUnavailable; return true; }
		const auto pod = vehicle.manifest->pods.front(); const uint16_t quantity = static_cast<uint16_t>(std::min<uint>(40, vehicle.cargo.TotalCount()));
		if (!CanReleaseOrbitalCargo(GetSpaceCraftReference(vehicle), center, pod, quantity)) { queue.blocked = SpaceOrderBlock::ProducerUnavailable; return true; }
		const bool released = CommitOrbitalCargoRelease(GetSpaceCraftReference(vehicle), center, pod, quantity); assert(released);
		if (!released) { queue.blocked = SpaceOrderBlock::ProducerUnavailable; return true; }
		if (vehicle.cargo.TotalCount() == 0) complete();
		return true;
	}
	// These physical approach actions share native pursuit and a frozen bounded deadline; each feature still owns its work.
	if (order.action == SpaceOrderAction::Visit || order.action == SpaceOrderAction::ReturnService || order.action == SpaceOrderAction::Survey || order.action == SpaceOrderAction::Construct || order.action == SpaceOrderAction::Unload) {
		// Static sites and bodies share one saved admission owner; craft/Surface navigation retains its separate producer contract.
		if (order.target.kind == SpaceOrderTargetKind::Body || order.target.kind == SpaceOrderTargetKind::Structure || order.target.kind == SpaceOrderTargetKind::Blueprint) {
			if (!RequestSpaceApproach(vehicle, order.target, true)) { queue.blocked = SpaceOrderBlock::ProducerUnavailable; return true; }
			const auto admission = GetSpaceApproachDestination(vehicle, destination);
			if (admission != SpaceOrderBlock::None) {
				// Waiting preserves the original action deadline without earning work; pursue staging normally rather than moving an occupant.
				queue.blocked = admission; vehicle.cur_speed = 4;
				MoveSpaceSurveyTowards(vehicle.position, destination, vehicle.mission.travel_remainder);
				return true;
			}
		} else ReleaseSpaceApproach(vehicle);
		if (!queue.started) {
			// A frozen bounded deadline at the same conservative 3-tile/day pursuit envelope.
			const uint64_t span = std::max<uint64_t>(std::abs(int64_t{vehicle.position.x} - destination.x), std::abs(int64_t{vehicle.position.y} - destination.y));
			queue.timer_ticks = static_cast<uint32_t>(std::max<uint64_t>(1480, (span * 2 * 74 + 3 * SPACE_Q16 - 1) / (3 * SPACE_Q16) + 148));
			queue.started = true;
		}
		if (queue.timer_ticks == 0) { queue.blocked = SpaceOrderBlock::Timeout; return true; }
		// Reaching a physical handler ends travel's deadline. Missing receiving capacity, materials or power then waits at that actual berth without spending partial freight or recreating a service timer in this queue.
		if (vehicle.position == destination && (order.action == SpaceOrderAction::Unload || order.action == SpaceOrderAction::ReturnService)) {
			queue.timer_ticks = 1; vehicle.cur_speed = 0;
			const bool finished = order.action == SpaceOrderAction::Unload ? ExecuteSpaceOrderUnload(vehicle) : ExecuteSpaceOrderService(vehicle);
			if (finished) complete();
			return true;
		}
		// Construction starts only at the exact paid site approach, using its exclusive material and current utility work admission.
		if (order.action == SpaceOrderAction::Construct && vehicle.position == destination) {
			auto &site = *ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(order.target.id)}, order.target.generation});
			vehicle.cur_speed = 0;
			if (site.state.phase == SpaceStructurePhase::FrameComplete) { complete(); return true; }
			const bool working = IsSpaceConstructionWorkTick(vehicle); // Same pre-delivery eligibility as the upkeep charge.
			ReserveOrbitalConstructionInputs(site, vehicle, true);
			if (working) { const bool progressed = CommitOrbitalConstructionStep(vehicle); assert(progressed); (void)progressed; }
			if (site.state.phase == SpaceStructurePhase::FrameComplete) complete();
			// The committed grant is spent now; retain this step's actual work result instead of treating its zero remaining balance as a busy slot.
			else queue.blocked = !HasOrbitalConstructionInputs(site) ? SpaceOrderBlock::MaterialsMissing : (working ? SpaceOrderBlock::None : SpaceOrderBlock::BuilderBusy);
			return true;
		}
		// A scan advances only through the same physical body/role admission that authorizes its active upkeep.
		if (order.action == SpaceOrderAction::Survey && IsSpaceSurveyWorkTick(vehicle)) {
			vehicle.position = destination; // Follow the current body without taking its mining lease.
			vehicle.cur_speed = 0;
			if (++queue.work_ticks == GetMultiLayerSpaceBudget().scan_ticks) {
				const bool surveyed = CompleteAsteroidSurvey(vehicle); assert(surveyed);
				if (surveyed) complete(); else { --queue.work_ticks; queue.blocked = SpaceOrderBlock::ProducerUnavailable; }
			}
			return true;
		}
		--queue.timer_ticks; vehicle.cur_speed = 4;
		if (MoveSpaceSurveyTowards(vehicle.position, destination, vehicle.mission.travel_remainder)) {
			if (order.action == SpaceOrderAction::Construct) queue.timer_ticks = std::max<uint32_t>(queue.timer_ticks, 1);
			if (order.action == SpaceOrderAction::Visit) {
				complete();
				QueueSpaceReplacementOnArrival(vehicle); // Only this genuine reached Visit event may offer automatic paid replacement; the existing native queue accounts its full capital later this tick.
			}
			// Actual freight/service admission occurs on the next reached tick; keep a bounded live deadline so arrival on its last travel opportunity cannot turn into a false timeout.
			else if (order.action == SpaceOrderAction::ReturnService || order.action == SpaceOrderAction::Unload) queue.timer_ticks = std::max<uint32_t>(queue.timer_ticks, 1);
		}
		return true;
	}
	queue.blocked = SpaceOrderBlock::ProducerUnavailable; // Scan/build/descent owners are implemented by Plans9-11.
	return true;
}

/** @copydoc ComputeSpaceOrdersDigest */
uint64_t ComputeSpaceOrdersDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	/** @param value Canonical integer entering the saved queue digest in fixed byte order. */
	auto hash = [&](uint64_t value) { for (int shift = 56; shift >= 0; shift -= 8) { digest ^= static_cast<uint8_t>(value >> shift); digest *= 1099511628211ULL; } };
	hash(_next_space_vehicle_generation);
	// Hash every native queue in stable pool order so interrupted continuation cannot hide a retargeted job or changed wait.
	for (const auto &r : CaptureSpaceOrders()) {
		hash(r.vehicle.base()); hash(r.generation); hash(r.queue.entries.size()); hash(r.queue.cursor); hash(r.queue.started);
		hash(to_underlying(r.queue.blocked)); hash(r.queue.timer_ticks); hash(r.queue.work_ticks);
		hash(r.queue.approach.has_value());
		if (r.queue.approach) {
			const auto &a = *r.queue.approach;
			hash(to_underlying(a.target.kind)); hash(a.target.id); hash(a.target.generation); hash(a.request_tick); hash(a.slot);
			hash(static_cast<uint32_t>(a.staging.x)); hash(static_cast<uint32_t>(a.staging.y));
		}
		// Include each ordered typed target and incarnation; coincident numeric IDs in different domains remain distinct.
		for (const auto &o : r.queue.entries) {
			hash(to_underlying(o.action)); hash(to_underlying(o.target.kind)); hash(o.target.id); hash(o.target.generation);
			// Preserve old/default queue digests while including every explicitly
			// configured policy in modern intent and quote-purity comparisons.
			if (o.mining_policy != SpaceMiningPolicy::LegacyReturn) { hash(0x4D494E504F4C31ULL); hash(to_underlying(o.mining_policy)); }
		}
	}
	return digest;
}
