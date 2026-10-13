/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_orders.cpp Typed queue execution over the canonical Vehicle and mining owners; Plan8 Group6. */
#include "stdafx.h"
#include "space_orders.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_vehicle_cmd.h"
#include "asteroid.h"
#include "launch_center.h"
#include "station_base.h"
#include "engine_base.h"
#include "company_base.h"
#include "multilayer_space_content.h"
#include "command_func.h"
#include "map_func.h"
#include <set>
#include "safeguards.h"

static uint64_t _next_space_vehicle_generation = 1;
uint64_t GetNextSpaceVehicleGeneration() { return _next_space_vehicle_generation; }
void ResetSpaceOrders() { _next_space_vehicle_generation = 1; ResetSpaceOrderLoadStaging(); }
bool RegisterSpaceVehicleIdentity(SpaceVehicle &vehicle)
{
	if (vehicle.generation != 0 || _next_space_vehicle_generation == UINT64_MAX) return false;
	vehicle.generation = _next_space_vehicle_generation++;
	return true;
}

static bool ValidOrder(const SpaceOrder &order)
{
	if (order.action >= SpaceOrderAction::End || order.target.kind >= SpaceOrderTargetKind::End || order.target.id == UINT32_MAX) return false;
	if (order.target.kind == SpaceOrderTargetKind::Body ? order.target.generation != 0 : order.target.generation == 0) return false;
	if (order.target.kind == SpaceOrderTargetKind::LaunchCenter && order.target.id >= StationID::Invalid().base()) return false;
	if (order.target.kind == SpaceOrderTargetKind::Craft && order.target.id >= VehicleID::End().base()) return false;
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

bool IsSpaceOrderAllowed(uint16_t model, const SpaceOrder &order)
{
	if (!ValidOrder(order)) return false;
	if (model != 15 && model != 16 && model != 17 && model != 19 && model != 20) return false;
	switch (order.action) {
		case SpaceOrderAction::Survey: return model == 19;
		case SpaceOrderAction::Mine: return model == 15;
		case SpaceOrderAction::Construct: return model == 16 || model == 20;
		case SpaceOrderAction::Unload: return model != 19;
		case SpaceOrderAction::Visit:
		case SpaceOrderAction::ReturnService: return true;
		default: return false;
	}
}

bool ValidateSpaceOrderRecords(std::span<const SpaceOrderRecord> records, uint64_t next)
{
	if (next == 0 || records.size() > MAX_SPACE_VEHICLES) return false;
	std::set<VehicleID> vehicles; std::set<uint64_t> generations;
	for (const auto &record : records) {
		const auto &q = record.queue;
		if (record.vehicle == VehicleID::Invalid() || record.generation == 0 || record.generation >= next || !vehicles.insert(record.vehicle).second || !generations.insert(record.generation).second ||
			q.entries.size() > MAX_SPACE_ORDERS || q.cursor > q.entries.size() || q.blocked >= SpaceOrderBlock::End || q.work_ticks >= GetMultiLayerSpaceBudget().scan_ticks || q.timer_ticks > (1U << 24) ||
			(q.work_ticks != 0 && (!q.started || (q.timer_ticks == 0 && q.blocked != SpaceOrderBlock::Timeout) || q.cursor >= q.entries.size() || q.entries[q.cursor].action != SpaceOrderAction::Survey)) ||
			(q.cursor == q.entries.size() && (q.started || q.timer_ticks != 0)) || (!q.started && q.timer_ticks != 0) ||
			(q.blocked == SpaceOrderBlock::Completed && (q.entries.empty() || q.cursor != q.entries.size())) ||
			(q.entries.empty() && (q.cursor != 0 || q.started || q.blocked != SpaceOrderBlock::None))) return false;
		for (const auto &order : q.entries) if (!ValidOrder(order) || (order.target.kind == SpaceOrderTargetKind::Craft && order.target.generation >= next)) return false;
		if (q.blocked >= SpaceOrderBlock::ReturnsPending && !IsSpaceMiningServiceWait(q)) return false;
	}
	return true;
}

std::vector<SpaceOrderRecord> CaptureSpaceOrders()
{
	std::vector<SpaceOrderRecord> records;
	for (const auto *vehicle : SpaceVehicle::Iterate()) records.push_back({vehicle->index, vehicle->generation, vehicle->space_orders});
	return records;
}

bool ValidateNativeSpaceOrders()
{
	const auto records = CaptureSpaceOrders();
	if (!ValidateSpaceOrderRecords(records, _next_space_vehicle_generation)) return false;
	for (const auto *v : SpaceVehicle::Iterate()) {
		if (!Engine::IsValidID(v->engine_type) || v->GetEngine()->type != VehicleType::Space) return false;
		const auto &q = v->space_orders;
		const auto model = v->GetEngine()->VehInfo<SpaceVehicleInfo>().model;
		for (const auto &o : q.entries) if (!IsSpaceOrderAllowed(model, o)) return false;
		for (const auto &o : q.entries) if (o.target.kind == SpaceOrderTargetKind::Craft && q.blocked != SpaceOrderBlock::TargetRemoved) {
			const auto *target = ResolveSpaceCraft({VehicleID{o.target.id}, o.target.generation});
			if (target != nullptr && target->owner != v->owner) return false;
		}
		if (q.entries.empty()) continue;
		if (v->construction.placement != SpacePlacement::Orbital) return false;
		if (q.work_ticks != 0 && GetMutableAsteroidBody(OrbitalBodyID{q.entries[q.cursor].target.id}) == nullptr) return false;
		if (q.started && q.cursor < q.entries.size() && q.entries[q.cursor].action == SpaceOrderAction::Mine) {
			if (v->mission.target != OrbitalBodyID{q.entries[q.cursor].target.id}) return false;
		} else if (v->mission.phase != SpaceMissionPhase::Holding) return false;
	}
	return true;
}

bool InstallLoadedSpaceOrders(std::span<const SpaceOrderRecord> records, uint64_t next)
{
	if (!ValidateSpaceOrderRecords(records, next) || records.size() != GetSpaceVehicleCount()) return false;
	for (const auto &record : records) {
		const auto *v = SpaceVehicle::GetIfValid(record.vehicle);
		if (v == nullptr || !Engine::IsValidID(v->engine_type) || v->GetEngine()->type != VehicleType::Space) return false;
		const auto *info = &v->GetEngine()->VehInfo<SpaceVehicleInfo>();
		if (info == nullptr) return false;
		for (const auto &order : record.queue.entries) if (!IsSpaceOrderAllowed(info->model, order)) return false;
	}
	for (const auto &record : records) { auto *v = SpaceVehicle::Get(record.vehicle); v->generation = record.generation; v->space_orders = record.queue; }
	_next_space_vehicle_generation = next;
	return true;
}

void CancelSpaceOrders(SpaceVehicle &vehicle)
{
	ReleaseSpaceMiningSlot(vehicle); vehicle.space_orders = {};
	vehicle.mission.target = INVALID_ORBITAL_BODY; vehicle.mission.phase = SpaceMissionPhase::Holding; vehicle.mission.approach_ticks_left = 0;
}

/** Resolve delivered producers only. Static structures/blueprints gain no proxy pool here. */
SpaceOrderBlock ResolveSpaceNavigationTarget(const SpaceVehicle &vehicle, const SpaceOrderTarget &target, SpacePoint &point)
{
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
		if (center == nullptr || center->owner != vehicle.owner) return SpaceOrderBlock::TargetRemoved;
		const auto reference = GetLaunchCenterPlaneReference(center->launch_center->tile);
		if (!reference) return SpaceOrderBlock::TargetRemoved;
		point = *reference;
		return SpaceOrderBlock::None;
	}
	if (target.kind == SpaceOrderTargetKind::Craft) {
		const auto *other = ResolveSpaceCraft({VehicleID{target.id}, target.generation});
		if (other == nullptr || other == &vehicle || other->owner != vehicle.owner || other->construction.placement != SpacePlacement::Orbital) return SpaceOrderBlock::TargetRemoved;
		point = other->position; return SpaceOrderBlock::None;
	}
	return SpaceOrderBlock::ProducerUnavailable;
}

CommandCost CmdEditSpaceOrder(DoCommandFlags flags, VehicleID id, uint64_t generation, SpaceOrderEdit edit, uint8_t index, SpaceOrderAction action, SpaceOrderTargetKind kind, uint32_t target, uint64_t target_generation)
{
	auto *v = SpaceVehicle::GetIfValid(id);
	if (v == nullptr || v->construction.placement != SpacePlacement::Orbital || (v->manifest && v->manifest->service_ticks != 0) || generation == 0 || v->generation != generation || v->owner != _current_company || v->vehstatus.Test(VehState::Crashed) ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || edit >= SpaceOrderEdit::End) return CMD_ERROR;
	if (!Engine::IsValidID(v->engine_type) || v->GetEngine()->type != VehicleType::Space) return CMD_ERROR;
	const auto *info = &v->GetEngine()->VehInfo<SpaceVehicleInfo>();
	if (info == nullptr) return CMD_ERROR;
	const SpaceOrder order{action, {kind, target, target_generation}};
	const auto size = v->space_orders.entries.size();
	if (edit == SpaceOrderEdit::Insert || edit == SpaceOrderEdit::Replace) {
		if (!IsSpaceOrderAllowed(info->model, order) || index > size || (edit == SpaceOrderEdit::Replace && index == size) || (edit == SpaceOrderEdit::Insert && size == MAX_SPACE_ORDERS)) return CMD_ERROR;
		SpacePoint point;
		if (ResolveSpaceNavigationTarget(*v, order.target, point) != SpaceOrderBlock::None) return CMD_ERROR;
	} else if (edit == SpaceOrderEdit::Erase && index >= size) return CMD_ERROR;
	else if (edit == SpaceOrderEdit::MoveUp && (index == 0 || index >= size)) return CMD_ERROR;
	else if (edit == SpaceOrderEdit::MoveDown && size_t{index} + 1 >= size) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
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
		v->vehstatus.Reset(VehState::Stopped); v->MarkDirty();
	}
	return CommandCost{};
}

bool IsSpaceSurveyWorkTick(const SpaceVehicle &vehicle)
{
	const auto &q = vehicle.space_orders;
	if (GetSpaceVehicleModel(vehicle) != 19 || vehicle.construction.placement != SpacePlacement::Orbital || vehicle.vehstatus.Test(VehState::Stopped) ||
		(vehicle.manifest && vehicle.manifest->service_ticks != 0) || q.cursor >= q.entries.size() || q.blocked == SpaceOrderBlock::Timeout || (q.started && q.timer_ticks == 0)) return false;
	const auto &o = q.entries[q.cursor];
	if (o.action != SpaceOrderAction::Survey || o.target.kind != SpaceOrderTargetKind::Body) return false;
	const auto *body = GetMutableAsteroidBody(OrbitalBodyID{o.target.id}); if (body == nullptr) return false;
	const SpacePoint point{body->motion.position.x + vehicle.mission.attachment_offset.x, body->motion.position.y + vehicle.mission.attachment_offset.y};
	return SpaceDistanceSquared(vehicle.position, point) <= uint64_t{SPACE_Q16 / 4} * (SPACE_Q16 / 4);
}

bool IsSpaceMiningServiceWait(const SpaceOrderQueue &q)
{
	return !q.started && q.cursor < q.entries.size() && q.entries[q.cursor].action == SpaceOrderAction::Mine &&
		(q.blocked == SpaceOrderBlock::ReturnsPending || q.blocked == SpaceOrderBlock::ServiceRequired || q.blocked == SpaceOrderBlock::PodsMissing);
}

SpaceOrderBlock GetSpaceMiningBlock(const SpaceVehicle &v)
{
	// Retained P2 craft keep their old mining behavior; they gain no free return allowance.
	if (v.construction.content_version == 0 && (!v.manifest || v.manifest->services_completed == 0)) return SpaceOrderBlock::None;
	if (!v.manifest || v.manifest->service_ticks != 0 || v.manifest->returns_remaining == 0) return SpaceOrderBlock::ServiceRequired;
	if (v.manifest->returns_remaining <= v.manifest->pending_returns) return SpaceOrderBlock::ReturnsPending;
	if (v.manifest->pods.empty()) return SpaceOrderBlock::PodsMissing;
	return SpaceOrderBlock::None;
}

bool IsSpaceMiningWorkTick(const SpaceVehicle &v)
{
	const auto *body = GetMutableAsteroidBody(v.mission.target);
	return v.mission.phase == SpaceMissionPhase::Mining && !v.vehstatus.Test(VehState::Stopped) && GetSpaceMiningBlock(v) == SpaceOrderBlock::None &&
		body != nullptr && body->mining_slot == v.index && body->remaining_reserve != 0 && v.cargo.TotalCount() < v.cargo_cap;
}

bool TickSpaceOrders(SpaceVehicle &vehicle)
{
	auto &queue = vehicle.space_orders;
	if (queue.entries.empty()) return false;
	if (queue.cursor == queue.entries.size()) { queue.blocked = SpaceOrderBlock::Completed; return true; }
	const auto &order = queue.entries[queue.cursor];
	if (queue.blocked == SpaceOrderBlock::Timeout) return true; // Explicit edit is required to retry a spent frozen deadline.
	SpacePoint destination;
	queue.blocked = ResolveSpaceNavigationTarget(vehicle, order.target, destination);
	if (queue.blocked != SpaceOrderBlock::None) {
		ReleaseSpaceMiningSlot(vehicle); vehicle.mission.phase = SpaceMissionPhase::Holding; queue.started = false; queue.timer_ticks = 0; queue.work_ticks = 0; return true;
	}
	auto complete = [&] {
		ReleaseSpaceMiningSlot(vehicle); vehicle.mission.phase = SpaceMissionPhase::Holding; vehicle.mission.approach_ticks_left = 0;
		++queue.cursor; queue.started = false; queue.timer_ticks = 0; queue.work_ticks = 0;
		queue.blocked = queue.cursor == queue.entries.size() ? SpaceOrderBlock::Completed : SpaceOrderBlock::None;
	};
	if (order.action == SpaceOrderAction::Mine) {
		if (queue.started && (vehicle.mission.phase == SpaceMissionPhase::Return || vehicle.mission.phase == SpaceMissionPhase::Holding)) { complete(); return true; }
		if (const auto block = GetSpaceMiningBlock(vehicle); block != SpaceOrderBlock::None) {
			ReleaseSpaceMiningSlot(vehicle); vehicle.mission.phase = SpaceMissionPhase::Holding; vehicle.mission.approach_ticks_left = 0;
			queue.started = false; queue.timer_ticks = 0; queue.work_ticks = 0; queue.blocked = block; return true;
		}
		if (!queue.started) {
			if (AssignSpaceMiningMission(vehicle, order.target.id, true).Failed()) { queue.blocked = SpaceOrderBlock::HoldIncompatible; return true; }
			queue.started = true;
		}
		return false; // Existing native lease/reserve/packet implementation is the only extraction owner.
	}
	if (order.action == SpaceOrderAction::Visit || order.action == SpaceOrderAction::ReturnService || order.action == SpaceOrderAction::Survey) {
		if (!queue.started) {
			// A frozen bounded deadline at the same conservative 3-tile/day pursuit envelope.
			const uint64_t span = std::max<uint64_t>(std::abs(int64_t{vehicle.position.x} - destination.x), std::abs(int64_t{vehicle.position.y} - destination.y));
			queue.timer_ticks = static_cast<uint32_t>(std::max<uint64_t>(1480, (span * 2 * 74 + 3 * SPACE_Q16 - 1) / (3 * SPACE_Q16) + 148));
			queue.started = true;
		}
		if (queue.timer_ticks == 0) { queue.blocked = SpaceOrderBlock::Timeout; return true; }
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
			if (order.action == SpaceOrderAction::Visit) complete();
			else if (order.action == SpaceOrderAction::ReturnService) { queue.blocked = SpaceOrderBlock::ProducerUnavailable; queue.started = false; queue.timer_ticks = 0; } // Precise arrival cannot grant free service.
		}
		return true;
	}
	queue.blocked = SpaceOrderBlock::ProducerUnavailable; // Scan/build/descent owners are implemented by Plans9-11.
	return true;
}

uint64_t ComputeSpaceOrdersDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	auto hash = [&](uint64_t value) { for (int shift = 56; shift >= 0; shift -= 8) { digest ^= static_cast<uint8_t>(value >> shift); digest *= 1099511628211ULL; } };
	hash(_next_space_vehicle_generation);
	for (const auto &r : CaptureSpaceOrders()) {
		hash(r.vehicle.base()); hash(r.generation); hash(r.queue.entries.size()); hash(r.queue.cursor); hash(r.queue.started);
		hash(to_underlying(r.queue.blocked)); hash(r.queue.timer_ticks); hash(r.queue.work_ticks);
		for (const auto &o : r.queue.entries) { hash(to_underlying(o.action)); hash(to_underlying(o.target.kind)); hash(o.target.id); hash(o.target.generation); }
	}
	return digest;
}
