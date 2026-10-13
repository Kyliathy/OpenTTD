/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_transition.cpp Real native rocket round trips, single burns and probe activation. */
#include "stdafx.h"
#include "space_transition.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "launch_operations.h"
#include "launch_center_cmd.h"
#include "multilayer_space_content.h"
#include "asteroid.h"
#include "station_base.h"
#include "company_base.h"
#include "company_func.h"
#include "hub_cargo.h"
#include "command_func.h"
#include "map_func.h"
#include "window_func.h"
#include <set>
#include "safeguards.h"

bool IsSpaceFlightActive(const SpaceVehicle &v) { return v.transition && v.transition->phase != SpaceFlightPhase::GroundLoading; }
bool IsSpaceCraftAwaitingSurfaceRecovery(const SpaceVehicle &v)
{
	if (v.construction.placement == SpacePlacement::Unhomed) return true;
	const auto waiting = [](const SpaceVehicle &craft) { return craft.construction.home == LaunchCenterRef{} && craft.transition &&
		craft.transition->phase == SpaceFlightPhase::Holding && craft.transition->layer == MapLayer::Surface && craft.transition->blocked == SpaceFlightBlock::HomeLost; };
	if (v.construction.placement == SpacePlacement::InTransit) return waiting(v);
	if (v.construction.placement == SpacePlacement::Carried && v.manifest) { const auto *parent = ResolveSpaceCraft(v.manifest->carrier); return parent != nullptr && waiting(*parent); }
	return false;
}
MapLayer GetSpaceVehicleLayer(const SpaceVehicle &v)
{
	if (v.construction.placement == SpacePlacement::Orbital) return MapLayer::Space;
	if (v.construction.placement == SpacePlacement::InTransit && v.transition) return v.transition->layer;
	if (v.construction.placement == SpacePlacement::Carried && v.manifest) {
		const auto *parent = ResolveSpaceCraft(v.manifest->carrier);
		if (parent != nullptr && parent->transition && IsSpaceFlightActive(*parent)) return parent->transition->layer;
	}
	return MapLayer::Surface;
}
static bool ResolveDestination(const SpaceVehicle &rocket, const SpaceOrderTarget &target, SpacePoint &point)
{
	return (target.kind == SpaceOrderTargetKind::Body || target.kind == SpaceOrderTargetKind::LaunchCenter || target.kind == SpaceOrderTargetKind::Craft) &&
		ResolveSpaceNavigationTarget(rocket, target, point) == SpaceOrderBlock::None;
}
static Station *RecoveryCenter(const SpaceVehicle &v)
{
	if (!v.transition) return nullptr; auto *s = ResolveLaunchCenter(v.transition->recovery);
	return s != nullptr && s->owner == v.owner && s->launch_operations ? s : nullptr;
}
static void ResetFlight(SpaceVehicle &v, bool canceled)
{
	auto &f = *v.transition;
	const auto started = f.missions_started, completed = f.missions_completed + (canceled ? 0 : 1), aborted = f.missions_canceled + (canceled ? 1 : 0);
	const auto remainder = f.pad_cost_remainder;
	f = {}; f.missions_started = started; f.missions_completed = completed; f.missions_canceled = aborted; f.pad_cost_remainder = remainder;
}
static bool Burn(SpaceVehicle &v, uint16_t quantity)
{
	if (!v.manifest || v.manifest->fuel_burned > UINT64_MAX - quantity || !HubCargoTransfer::Consume(v.manifest->supplies[0], quantity)) return false;
	v.manifest->fuel_burned += quantity; return true;
}
static void StartReturn(SpaceVehicle &v)
{
	v.mission.travel_remainder = 0; // The completed/cancelled closing attempt has no remaining sub-step.
	auto &f = *v.transition; const bool burned = Burn(v, GetMultiLayerSpaceBudget().recovery_fuel); assert(burned); (void)burned;
	f.recovery_burned = true; f.phase = SpaceFlightPhase::ReturnRecovery; f.blocked = SpaceFlightBlock::None; f.remaining_ticks = GetMultiLayerSpaceBudget().flight_ticks; f.departure = v.position;
}
CommandCost CmdBeginLaunchFlight(DoCommandFlags flags, VehicleID vehicle, uint64_t generation, SpaceOrderTargetKind kind, uint32_t target, uint64_t target_generation)
{
	auto *v = ResolveSpaceCraft({vehicle, generation}); const auto &b = GetMultiLayerSpaceBudget(); const SpaceOrderTarget destination{kind, target, target_generation}; SpacePoint point;
	if (v == nullptr || v->owner != _current_company || GetSpaceVehicleModel(*v) != 14 || v->construction.placement != SpacePlacement::SurfaceStored ||
		IsSpaceFlightActive(*v) || !v->manifest ||
		v->manifest->supplies[0].AvailableCount() < uint{b.outbound_fuel} + b.recovery_fuel || v->manifest->fuel_burned > UINT64_MAX - b.outbound_fuel - b.recovery_fuel ||
		GetSpacePayloadMass(*v) > b.rocket_payload_sixteenths || (v->transition && v->transition->missions_started == UINT64_MAX) || !ResolveDestination(*v, destination, point)) return CMD_ERROR;
	auto *center = ResolveLaunchCenter(v->construction.home); const auto ref = GetSpaceCraftReference(*v);
	if (center == nullptr || center->owner != v->owner || !center->launch_operations || center->launch_operations->pad_active != SpaceCraftRef{} ||
		center->launch_operations->pad_queue.empty() || center->launch_operations->pad_queue.front() != ref) return CMD_ERROR;
	if (kind == SpaceOrderTargetKind::Craft) {
		if (GetSpaceResupplyBlock(*v, {VehicleID{target}, target_generation}) != SpaceFlightBlock::None) return CMD_ERROR;
	} else {
		auto *probe = ResolveSpaceCraft(v->manifest->passenger);
		if (v->cargo.TotalCount() != 0 || probe == nullptr || probe->owner != v->owner || !probe->manifest || probe->manifest->carrier != ref || probe->construction.placement != SpacePlacement::Carried ||
			probe->construction.content_version != MULTILAYER_SPACE_CONTENT_VERSION) return CMD_ERROR;
	}
	if (flags.Test(DoCommandFlag::Execute)) {
		if (!GetAsteroidField().surveys.empty()) { const bool retired = RetireSpaceSurveyPreview(); assert(retired); (void)retired; }
		if (!v->transition) v->transition = std::make_unique<SpaceTransition>(); auto &f = *v->transition;
		++f.missions_started; f.phase = SpaceFlightPhase::Preparing; f.source = f.recovery = GetLaunchCenterReference(*center); f.destination = destination;
		f.departure = v->position; f.remaining_ticks = b.prepare_ticks;
		center->launch_operations->pad_queue.erase(center->launch_operations->pad_queue.begin()); center->launch_operations->pad_active = ref;
		v->construction.placement = SpacePlacement::InTransit; RefreshLaunchPad(*center); SyncSpacePassenger(*v); v->MarkDirty(); SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost(ExpensesType::Other, b.mission_fee); // Accepted once; cancellation retains fuel and the already paid fee.
}
CommandCost CmdCancelLaunchFlight(DoCommandFlags flags, VehicleID vehicle, uint64_t generation)
{
	auto *v = ResolveSpaceCraft({vehicle, generation});
	if (v == nullptr || v->owner != _current_company || !IsSpaceFlightActive(*v) ||
		(v->transition->phase != SpaceFlightPhase::Preparing && v->transition->phase != SpaceFlightPhase::OrbitalDelivery)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		if (v->transition->phase == SpaceFlightPhase::Preparing) {
			auto *center = RecoveryCenter(*v); ResetFlight(*v, true);
			if (center != nullptr) {
				center->launch_operations->pad_active = {}; v->construction.placement = SpacePlacement::SurfaceStored;
				center->launch_operations->pad_queue.push_back(GetSpaceCraftReference(*v)); RefreshLaunchPad(*center);
			} else { v->construction.home = {}; v->construction.placement = SpacePlacement::Unhomed; v->construction.paused = true; DetachSpaceManifestAtLostCenter(*v); }
		} else StartReturn(*v); // The same dormant paid passenger comes back; activation has not occurred.
		SyncSpacePassenger(*v); v->MarkDirty(); SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost{};
}
CommandCost CmdDivertLaunchRecovery(DoCommandFlags flags, VehicleID vehicle, uint64_t generation, StationID station, uint64_t center_generation)
{
	auto *v = ResolveSpaceCraft({vehicle, generation}); auto *center = ResolveLaunchCenter({station, center_generation});
	if (v == nullptr || center == nullptr || v->owner != _current_company || center->owner != v->owner || !center->launch_operations || !IsSpaceFlightActive(*v) ||
		v->transition->phase != SpaceFlightPhase::Holding || center->launch_operations->pad_active != SpaceCraftRef{}) return CMD_ERROR;
	const auto area = GetLaunchCenterFootprint(center->launch_center->tile, center->launch_center->rotation);
	if (v->transition->layer == MapLayer::Space ? !IsInsideLaunchCenterDropZone(v->position, *GetLaunchCenterPlaneReference(center->launch_center->tile)) :
		!area || !area->Contains(v->construction.surface_tile)) return CMD_ERROR;
	const auto passenger = v->manifest ? v->manifest->passenger : SpaceCraftRef{};
	uint count = 0; for (const auto *other : SpaceVehicle::Iterate()) if (other != v && GetSpaceCraftReference(*other) != passenger && other->construction.home == GetLaunchCenterReference(*center) && other->construction.placement != SpacePlacement::Orbital) ++count;
	const uint needed = v->manifest && v->manifest->passenger != SpaceCraftRef{} ? 2 : 1;
	if (count + needed > MAX_CENTER_CRAFT) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		if (auto *old = RecoveryCenter(*v); old != nullptr && old->launch_operations->pad_active == GetSpaceCraftReference(*v)) { old->launch_operations->pad_active = {}; RefreshLaunchPad(*old); }
		v->construction.home = GetLaunchCenterReference(*center); v->home = *GetLaunchCenterPlaneReference(center->launch_center->tile);
		auto &f = *v->transition; f.recovery = v->construction.home; f.blocked = SpaceFlightBlock::None;
		if (f.layer == MapLayer::Space) { f.phase = SpaceFlightPhase::ReturnRecovery; f.remaining_ticks = GetMultiLayerSpaceBudget().flight_ticks; f.departure = v->position; }
		else { f.phase = SpaceFlightPhase::Turnaround; v->construction.surface_tile = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Recovery)->tile; }
		center->launch_operations->pad_active = GetSpaceCraftReference(*v);
		if (v->manifest) if (auto *probe = ResolveSpaceCraft(v->manifest->passenger)) probe->construction.home = v->construction.home;
		RefreshLaunchPad(*center); SyncSpacePassenger(*v);
	}
	return CommandCost{}; // Recovery fuel was burned for this leg already; no second hull, deployment or burn.
}
void ForgetSpaceTransitionCenter(SpaceVehicle &v, LaunchCenterRef center)
{
	if (!IsSpaceFlightActive(v)) return;
	if (v.transition->recovery != center) return;
	if (v.transition->phase == SpaceFlightPhase::Preparing) {
		ResetFlight(v, true); v.construction.placement = SpacePlacement::Unhomed; v.construction.home = {}; v.construction.paused = true; DetachSpaceManifestAtLostCenter(v);
	} else {
		v.construction.home = {}; v.transition->blocked = SpaceFlightBlock::HomeLost;
		if (v.transition->phase == SpaceFlightPhase::Turnaround) v.transition->phase = SpaceFlightPhase::Holding;
		if (v.manifest) if (auto *probe = ResolveSpaceCraft(v.manifest->passenger)) probe->construction.home = {};
	}
	v.MarkDirty();
}
void TickSpaceTransitions()
{
	const auto &b = GetMultiLayerSpaceBudget();
	for (auto *v : SpaceVehicle::Iterate()) if (IsSpaceFlightActive(*v)) {
		auto &f = *v->transition; auto *center = RecoveryCenter(*v);
		if ((f.phase == SpaceFlightPhase::Preparing || f.phase == SpaceFlightPhase::Turnaround) && center != nullptr) {
			const uint cost = f.pad_cost_remainder + b.pad_per_day; f.pad_cost_remainder = cost % 74;
			SubtractMoneyFromCompany(v->owner, CommandCost(ExpensesType::Other, cost / 74)); v->profit_this_year -= Money{cost / 74} * 256;
		}
		if (f.phase == SpaceFlightPhase::Preparing) {
			if (center == nullptr) { const auto lost = f.recovery; ForgetSpaceTransitionCenter(*v, lost); continue; }
			if (f.remaining_ticks != 0) --f.remaining_ticks;
			if (f.remaining_ticks == 0) {
				SpacePoint point; if (!ResolveDestination(*v, f.destination, point)) { f.blocked = SpaceFlightBlock::TargetLost; continue; }
				const bool burned = Burn(*v, b.outbound_fuel); assert(burned); (void)burned; f.outbound_burned = true;
				f.phase = SpaceFlightPhase::Ascent; f.remaining_ticks = b.flight_ticks; f.blocked = SpaceFlightBlock::None;
			}
		} else if (f.phase == SpaceFlightPhase::Ascent) {
			assert(f.remaining_ticks != 0);
			if (--f.remaining_ticks == 0) {
				f.layer = MapLayer::Space; f.phase = SpaceFlightPhase::OrbitalDelivery;
				v->vehstatus.Reset(VehState::Hidden);
				SpacePoint point; if (ResolveDestination(*v, f.destination, point)) v->position = point;
				else { v->position = f.departure; f.blocked = SpaceFlightBlock::TargetLost; }
			}
		} else if (f.phase == SpaceFlightPhase::OrbitalDelivery) {
			SpacePoint point; if (!ResolveDestination(*v, f.destination, point)) { f.blocked = SpaceFlightBlock::TargetLost; continue; }
			if (f.destination.kind == SpaceOrderTargetKind::Craft) {
				const SpaceCraftRef receiver{VehicleID{f.destination.id}, f.destination.generation};
				f.blocked = GetSpaceResupplyBlock(*v, receiver);
				if (f.blocked != SpaceFlightBlock::None) continue;
				if (SpaceDistanceSquared(v->position, point) > uint64_t{SPACE_Q16 / 4} * (SPACE_Q16 / 4)) {
					MoveSpaceSurveyTowards(v->position, point, v->mission.travel_remainder); f.blocked = SpaceFlightBlock::Approach;
				} else {
					v->position = point;
					const bool moved = CommitSpaceResupply(*v, receiver); assert(moved); if (!moved) continue;
					f.resupplied = true; StartReturn(*v);
				}
				v->UpdateSpacePosition(); v->MarkDirty(); continue;
			}
			auto *probe = v->manifest ? ResolveSpaceCraft(v->manifest->passenger) : nullptr;
			if (probe == nullptr || !probe->manifest || probe->manifest->carrier != GetSpaceCraftReference(*v)) { f.blocked = SpaceFlightBlock::TargetLost; continue; }
			v->position = point; probe->position = point; probe->home = v->home;
			probe->construction.placement = SpacePlacement::Orbital; probe->construction.paused = false; probe->construction.deployed = true; probe->manifest->carrier = {}; v->manifest->passenger = {};
			probe->vehstatus.Reset({VehState::Hidden, VehState::Stopped}); probe->UpdateSpacePosition(); SyncSpacePods(*probe); probe->MarkDirty();
			f.deployed = true; StartReturn(*v);
		} else if (f.phase == SpaceFlightPhase::ReturnRecovery) {
			assert(f.remaining_ticks != 0); --f.remaining_ticks;
			const uint elapsed = b.flight_ticks - f.remaining_ticks;
			v->position = {static_cast<int32_t>(int64_t{f.departure.x} + (int64_t{v->home.x} - f.departure.x) * elapsed / b.flight_ticks),
				static_cast<int32_t>(int64_t{f.departure.y} + (int64_t{v->home.y} - f.departure.y) * elapsed / b.flight_ticks)};
			if (f.remaining_ticks == 0) {
				if (center == nullptr || center->launch_operations->pad_active != GetSpaceCraftReference(*v)) { f.phase = SpaceFlightPhase::Holding; f.blocked = center == nullptr ? SpaceFlightBlock::HomeLost : SpaceFlightBlock::PadBusy; }
				else {
					f.layer = MapLayer::Surface; f.phase = SpaceFlightPhase::Turnaround; f.remaining_ticks = b.turnaround_ticks; f.blocked = SpaceFlightBlock::None;
					v->vehstatus.Set(VehState::Hidden);
					v->construction.surface_tile = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Recovery)->tile;
				}
			}
		} else if (f.phase == SpaceFlightPhase::Turnaround) {
			if (center == nullptr) { f.phase = SpaceFlightPhase::Holding; f.blocked = SpaceFlightBlock::HomeLost; v->construction.home = {}; continue; }
			assert(f.remaining_ticks != 0);
			if (--f.remaining_ticks == 0) {
				const bool canceled = !f.deployed && !f.resupplied; ResetFlight(*v, canceled); center->launch_operations->pad_active = {};
				v->construction.placement = SpacePlacement::SurfaceStored; center->launch_operations->pad_queue.push_back(GetSpaceCraftReference(*v)); RefreshLaunchPad(*center);
			}
		}
		v->UpdateSpacePosition(); v->MarkDirty();
	}
	// All parents move before their dormant passengers and pods are synchronized.
	for (auto *v : SpaceVehicle::Iterate()) if (v->manifest && v->manifest->passenger != SpaceCraftRef{}) SyncSpacePassenger(*v);
}

std::vector<SpaceTransitionRecord> CaptureSpaceTransitions()
{
	std::vector<SpaceTransitionRecord> records;
	for (const auto *v : SpaceVehicle::Iterate()) if (v->transition) records.push_back({GetSpaceCraftReference(*v), *v->transition});
	return records;
}
bool ValidateSpaceTransitionRecords(std::span<const SpaceTransitionRecord> records, uint32_t width, uint32_t height)
{
	if (records.size() > MAX_SPACE_VEHICLES || width < 64 || height < 64 || width > 4096 || height > 4096) return false;
	std::set<VehicleID> ids; const auto &b = GetMultiLayerSpaceBudget();
	for (const auto &r : records) {
		const auto &f = r.state;
		if (r.craft.vehicle >= VehicleID::End() || r.craft.generation == 0 || !ids.insert(r.craft.vehicle).second || f.phase >= SpaceFlightPhase::End || f.blocked >= SpaceFlightBlock::End ||
			(f.layer != MapLayer::Surface && f.layer != MapLayer::Space) || f.pad_cost_remainder >= 74 || f.missions_completed > f.missions_started || f.missions_canceled > f.missions_started - f.missions_completed) return false;
		const auto pending = f.missions_started - f.missions_completed - f.missions_canceled;
		if (f.phase == SpaceFlightPhase::GroundLoading) {
			auto expected = SpaceTransition{}; expected.pad_cost_remainder = f.pad_cost_remainder; expected.missions_started = f.missions_started; expected.missions_completed = f.missions_completed; expected.missions_canceled = f.missions_canceled;
			if (pending != 0 || f != expected) return false; continue;
		}
		if (pending != 1 || f.source.station >= StationID::End() || f.source.generation == 0 || f.recovery.station >= StationID::End() || f.recovery.generation == 0 ||
			(f.destination.kind != SpaceOrderTargetKind::Body && f.destination.kind != SpaceOrderTargetKind::LaunchCenter && f.destination.kind != SpaceOrderTargetKind::Craft) || f.destination.id == UINT32_MAX ||
			(f.destination.kind == SpaceOrderTargetKind::Body ? f.destination.generation != 0 : f.destination.generation == 0 ||
			f.destination.id >= (f.destination.kind == SpaceOrderTargetKind::Craft ? VehicleID::End().base() : uint32_t{StationID::End().base()}))) return false;
		if (f.destination.kind == SpaceOrderTargetKind::Craft ? f.deployed || f.destination.id == r.craft.vehicle.base() : f.resupplied) return false;
		if (f.resupplied && f.phase != SpaceFlightPhase::ReturnRecovery && f.phase != SpaceFlightPhase::Turnaround && f.phase != SpaceFlightPhase::Holding) return false;
		if (f.blocked >= SpaceFlightBlock::ReceiverBusy && (f.destination.kind != SpaceOrderTargetKind::Craft || f.phase != SpaceFlightPhase::OrbitalDelivery)) return false;
		if (f.departure.x < 0 || f.departure.y < 0 || f.departure.x > int64_t{width - 1} * SPACE_Q16 || f.departure.y > int64_t{height - 1} * SPACE_Q16 ||
			(f.destination.kind == SpaceOrderTargetKind::Body && f.destination.id >= MAX_ORBITAL_BODIES)) return false;
		if (f.phase == SpaceFlightPhase::Preparing) {
			if (f.layer != MapLayer::Surface || f.remaining_ticks > b.prepare_ticks || f.outbound_burned || f.recovery_burned || f.deployed) return false;
		} else if (f.phase == SpaceFlightPhase::Ascent) {
			if (f.layer != MapLayer::Surface || f.remaining_ticks == 0 || f.remaining_ticks > b.flight_ticks || !f.outbound_burned || f.recovery_burned || f.deployed) return false;
		} else if (f.phase == SpaceFlightPhase::OrbitalDelivery) {
			if (f.layer != MapLayer::Space || f.remaining_ticks != 0 || !f.outbound_burned || f.recovery_burned || f.deployed) return false;
		} else if (f.phase == SpaceFlightPhase::ReturnRecovery) {
			if (f.layer != MapLayer::Space || f.remaining_ticks == 0 || f.remaining_ticks > b.flight_ticks || !f.outbound_burned || !f.recovery_burned) return false;
		} else if (f.phase == SpaceFlightPhase::Turnaround) {
			if (f.layer != MapLayer::Surface || f.remaining_ticks == 0 || f.remaining_ticks > b.turnaround_ticks || !f.outbound_burned || !f.recovery_burned) return false;
		} else if (f.phase == SpaceFlightPhase::Holding && ((f.layer == MapLayer::Space ? f.remaining_ticks != 0 : f.remaining_ticks == 0 || f.remaining_ticks > b.turnaround_ticks) ||
			!f.outbound_burned || !f.recovery_burned || f.blocked == SpaceFlightBlock::None)) return false;
	}
	return true;
}
bool ValidateSpaceTransitionLinks(std::span<const SpaceTransitionRecord> flight, std::span<const SpaceProductionRecord> craft, std::span<const LaunchOperationsRecord> operations)
{
	for (const auto &r : flight) {
		const SpaceConstructionState *s = nullptr; for (const auto &c : craft) if (c.craft == r.craft) s = &c.state;
		if (s == nullptr || s->model != 14 || s->content_version != MULTILAYER_SPACE_CONTENT_VERSION) return false;
		if (r.state.phase != SpaceFlightPhase::GroundLoading) {
			if (s->placement != SpacePlacement::InTransit) return false;
			if (s->home != LaunchCenterRef{}) {
				bool reserved = false; for (const auto &op : operations) if (op.center == s->home && op.center == r.state.recovery && op.slots.pad_active == r.craft) reserved = true;
				if (!reserved) return false;
			}
		} else if (s->placement == SpacePlacement::InTransit) return false;
	}
	for (const auto &s : craft) if (s.state.placement == SpacePlacement::InTransit && std::none_of(flight.begin(), flight.end(), [&](const auto &r) { return r.craft == s.craft && r.state.phase != SpaceFlightPhase::GroundLoading; })) return false;
	for (const auto &op : operations) if (op.slots.pad_active != SpaceCraftRef{} && std::none_of(flight.begin(), flight.end(), [&](const auto &r) { return r.craft == op.slots.pad_active && r.state.recovery == op.center && r.state.phase != SpaceFlightPhase::GroundLoading; })) return false;
	return true;
}
bool ValidateSpaceTransitions()
{
	const auto records = CaptureSpaceTransitions();
	if (!ValidateSpaceTransitionRecords(records, Map::SizeX(), Map::SizeY()) || !ValidateSpaceTransitionLinks(records, CaptureSpaceProduction(), CaptureLaunchOperations())) return false;
	for (const auto &r : records) {
		const auto *v = ResolveSpaceCraft(r.craft); if (v == nullptr || GetSpaceVehicleModel(*v) != 14 || !v->manifest) return false;
		const auto &f = r.state; if (!IsSpaceFlightActive(*v)) continue;
		if (RecoveryCenter(*v) != nullptr && v->construction.home != f.recovery) return false;
		const uint fuel_needed = f.recovery_burned ? 0U : f.outbound_burned ? GetMultiLayerSpaceBudget().recovery_fuel : uint{GetMultiLayerSpaceBudget().outbound_fuel} + GetMultiLayerSpaceBudget().recovery_fuel;
		if (f.source.generation >= GetNextLaunchCenterGeneration() || f.recovery.generation >= GetNextLaunchCenterGeneration() ||
			(f.destination.kind == SpaceOrderTargetKind::Craft ? v->manifest->passenger != SpaceCraftRef{} || f.destination.generation >= GetNextSpaceVehicleGeneration() :
			f.deployed == (v->manifest->passenger != SpaceCraftRef{})) || v->manifest->supplies[0].TotalCount() < fuel_needed) return false;
		if (f.resupplied && (!v->manifest->pods.empty() || v->cargo.TotalCount() != 0 || v->manifest->supplies[1].TotalCount() != 0 || v->manifest->supplies[2].TotalCount() != 0)) return false;
		if (f.destination.kind == SpaceOrderTargetKind::Craft && f.blocked != SpaceFlightBlock::TargetLost &&
			(f.phase == SpaceFlightPhase::Preparing || f.phase == SpaceFlightPhase::Ascent || f.phase == SpaceFlightPhase::OrbitalDelivery)) {
			const auto *receiver = ResolveSpaceCraft({VehicleID{f.destination.id}, f.destination.generation});
			if (receiver != nullptr && receiver->owner != v->owner) return false;
		}
		if (GetSpaceVehicleLayer(*v) == MapLayer::Surface && f.phase != SpaceFlightPhase::Holding) {
			const auto *center = RecoveryCenter(*v); if (center == nullptr) { if (f.phase == SpaceFlightPhase::Ascent) continue; return false; }
			const auto zone = f.phase == SpaceFlightPhase::Turnaround ? LaunchFacilityZone::Recovery : LaunchFacilityZone::Pad;
			if (v->construction.surface_tile != GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, zone)->tile) return false;
		}
	}
	return true;
}
bool InstallLoadedSpaceTransitions(std::span<const SpaceTransitionRecord> records)
{
	if (!ValidateSpaceTransitionRecords(records, Map::SizeX(), Map::SizeY())) return false;
	for (const auto *v : SpaceVehicle::Iterate()) if (v->transition) return false;
	for (const auto &r : records) { const auto *v = ResolveSpaceCraft(r.craft); if (v == nullptr || GetSpaceVehicleModel(*v) != 14) return false; }
	for (const auto &r : records) ResolveSpaceCraft(r.craft)->transition = std::make_unique<SpaceTransition>(r.state);
	return true;
}
uint64_t ComputeSpaceTransitionDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	auto add = [&](uint64_t value) { for (uint i = 0; i < 8; ++i) { hash ^= (value >> (i * 8)) & 255; hash *= 1099511628211ULL; } };
	auto center = [&](LaunchCenterRef ref) { add(ref.station.base()); add(ref.generation); };
	for (const auto &r : CaptureSpaceTransitions()) {
		add(r.craft.vehicle.base()); add(r.craft.generation); const auto &f = r.state;
		add(to_underlying(f.phase)); add(to_underlying(f.blocked)); add(to_underlying(f.layer)); center(f.source); center(f.recovery);
		add(to_underlying(f.destination.kind)); add(f.destination.id); add(f.destination.generation); add(static_cast<uint32_t>(f.departure.x)); add(static_cast<uint32_t>(f.departure.y));
		add(f.remaining_ticks); add(f.pad_cost_remainder); add(f.outbound_burned); add(f.recovery_burned); add(f.deployed); add(f.missions_started); add(f.missions_completed); add(f.missions_canceled);
		if (f.resupplied) add(0x5253504C); // Preserve the old digest stream when no resupply receipt exists.
	}
	return hash;
}
