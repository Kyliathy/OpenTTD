/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_operations.cpp Native center zones, paid assembly and identity-safe recovery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups2B/8B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups8/8BC; actual paid provenance and prepared Earth replacement settlement.
 */
#include "stdafx.h"
#include "launch_operations.h"
#include "launch_center_cmd.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_transition.h"
#include "space_replacement.h"
#include "multilayer_space_content.h"
#include "station_base.h"
#include "engine_base.h"
#include "company_base.h"
#include "company_func.h"
#include "group.h"
#include "hub_cargo.h"
#include "command_func.h"
#include "cargotype.h"
#include "map_func.h"
#include "vehicle_func.h"
#include "window_func.h"
#include <set>
#include "safeguards.h"

/** @copydoc GetLaunchFacilityZone */
std::optional<TileArea> GetLaunchFacilityZone(TileIndex anchor, uint8_t rotation, LaunchFacilityZone zone)
{
	if (zone >= LaunchFacilityZone::End || !GetLaunchCenterFootprint(anchor, rotation)) return std::nullopt;
	const uint x0 = zone == LaunchFacilityZone::Pad ? 0 : zone == LaunchFacilityZone::Workshop ? 1 : zone == LaunchFacilityZone::Recovery ? 3 : 0;
	const uint y0 = zone == LaunchFacilityZone::Storage ? 2 : 0;
	const uint w = zone == LaunchFacilityZone::Workshop ? 2 : zone == LaunchFacilityZone::Storage ? 4 : 1;
	const uint h = zone == LaunchFacilityZone::Storage ? 1 : 2;
	uint minx = 4, miny = 4, maxx = 0, maxy = 0;
	// Rotate every physical zone cell with the actual launch-center footprint; workshop/storage admission must agree for all four orientations.
	for (uint y = y0; y < y0 + h; ++y) for (uint x = x0; x < x0 + w; ++x) {
		const uint rx = rotation == 0 ? x : rotation == 1 ? 2 - y : rotation == 2 ? 3 - x : y;
		const uint ry = rotation == 0 ? y : rotation == 1 ? x : rotation == 2 ? 2 - y : 3 - x;
		minx = std::min(minx, rx); miny = std::min(miny, ry); maxx = std::max(maxx, rx); maxy = std::max(maxy, ry);
	}
	return TileArea{TileXYL(TileX(anchor) + minx, TilePlaneY(anchor) + miny, MapLayer::Surface), static_cast<uint16_t>(maxx - minx + 1), static_cast<uint16_t>(maxy - miny + 1)};
}
/** @copydoc GetLaunchServiceApproach */
std::optional<SpacePoint> GetLaunchServiceApproach(const Station &center)
{
	if (!center.launch_center) return std::nullopt;
	const auto zone = GetLaunchFacilityZone(center.launch_center->tile, center.launch_center->rotation, LaunchFacilityZone::Workshop);
	return zone ? GetLaunchCenterPlaneReference(zone->tile) : std::nullopt;
}
/** @copydoc IsAtLaunchServiceApproach */
bool IsAtLaunchServiceApproach(SpacePoint point, const Station &center)
{
	const auto approach = GetLaunchServiceApproach(center); if (!approach) return false;
	const int64_t dx = int64_t{point.x} - approach->x, dy = int64_t{point.y} - approach->y, tolerance = SPACE_Q16 / 4;
	return std::abs(dx) <= tolerance && std::abs(dy) <= tolerance && dx * dx + dy * dy <= tolerance * tolerance;
}
/** @param ref Actual original center incarnation. @return Native non-orbital craft still consuming its bounded physical storage, including unfinished paid bodies. */
static uint StoredCount(LaunchCenterRef ref)
{
	uint count = 0; for (const auto *v : SpaceVehicle::Iterate()) if (v->construction.home == ref && v->construction.placement != SpacePlacement::Orbital) ++count; return count;
}
/** @copydoc HasLaunchCraft */
bool HasLaunchCraft(LaunchCenterRef center)
{
	if (center.generation == 0) return false;
	for (const auto *v : SpaceVehicle::Iterate()) if (v->construction.home == center) return true;
	return false;
}
/** Place an existing paid body inside its already admitted native center zone; this commissions no hull or work.
 * @param v Actual existing native craft. @param center Current actual physical center owner. @param zone Admitted workshop/storage/pad custody.
 */
static void Place(SpaceVehicle &v, Station &center, LaunchFacilityZone zone)
{
	v.construction.surface_tile = GetLaunchFacilityZone(center.launch_center->tile, center.launch_center->rotation, zone)->tile;
	v.position = v.home = *GetLaunchCenterPlaneReference(center.launch_center->tile);
	v.UpdateSpacePosition(); SyncSpacePassenger(v); v.MarkDirty();
}
/** @copydoc RefreshLaunchPad */
void RefreshLaunchPad(Station &center)
{
	if (!center.launch_operations) return;
	const auto home = GetLaunchCenterReference(center);
	std::erase_if(center.launch_operations->pad_queue, [&](auto ref) {
		const auto *v = ResolveSpaceCraft(ref);
		return v == nullptr || v->owner != center.owner || v->construction.home != home || v->construction.placement != SpacePlacement::SurfaceStored || GetSpaceVehicleModel(*v) != 14;
	});
	for (size_t i = 0; i < center.launch_operations->pad_queue.size(); ++i) {
		auto &v = *ResolveSpaceCraft(center.launch_operations->pad_queue[i]);
		const auto zone = i == 0 && center.launch_operations->pad_active == SpaceCraftRef{} ? LaunchFacilityZone::Pad : LaunchFacilityZone::Storage;
		const auto tile = GetLaunchFacilityZone(center.launch_center->tile, center.launch_center->rotation, zone)->tile;
		if (v.construction.surface_tile != tile) Place(v, center, zone);
	}
}
/** @copydoc ForgetLaunchCraft */
void ForgetLaunchCraft(SpaceVehicle &vehicle)
{
	const auto ref = GetSpaceCraftReference(vehicle);
	for (auto *center : Station::Iterate()) if (center->launch_operations) {
		auto &s = *center->launch_operations;
		if (s.workshop == ref) s.workshop = {};
		if (s.service == ref) s.service = {};
		if (s.pad_active == ref) s.pad_active = {};
		std::erase(s.pad_queue, ref); RefreshLaunchPad(*center);
	}
}
/** @copydoc ForgetLaunchOperations */
void ForgetLaunchOperations(Station &center, bool cleaning_pool)
{
	if (!cleaning_pool && center.launch_center) {
		const auto ref = GetLaunchCenterReference(center);
		ForgetSpaceDescentCenter(ref);
		ForgetSpacePodCenter(ref);
		for (auto *v : SpaceVehicle::Iterate()) if (v->construction.home == ref) {
			if (IsSpaceFlightActive(*v)) { ForgetSpaceTransitionCenter(*v, ref); continue; }
			if (v->construction.placement == SpacePlacement::Orbital || (v->construction.placement == SpacePlacement::Carried && v->manifest && ResolveSpaceCraft(v->manifest->carrier) != nullptr && IsSpaceFlightActive(*ResolveSpaceCraft(v->manifest->carrier)))) continue;
			DetachSpaceManifestAtLostCenter(*v);
			v->construction.home = {}; v->construction.placement = SpacePlacement::Unhomed;
			v->construction.paused = true; v->MarkDirty(); // Preserve the one paid body and its incomplete work at this actual Surface site.
		}
	}
	center.launch_operations.reset();
}
/** @copydoc TickLaunchOperations */
void TickLaunchOperations()
{
	// Reconcile actual center ownership before advancing its exclusive paid workshop; a lost or foreign body retains its paid work as unhomed custody rather than borrowing a different center.
	for (const auto ref : GetLaunchCenterReferences()) {
		auto *center = ResolveLaunchCenter(ref); if (center == nullptr || !center->launch_operations) continue;
		auto &slots = *center->launch_operations;
		for (auto *v : SpaceVehicle::Iterate()) if (v->construction.home == ref && v->owner != center->owner) {
			if (IsSpaceFlightActive(*v)) { ForgetLaunchCraft(*v); ForgetSpaceTransitionCenter(*v, ref); continue; }
			if (v->construction.placement == SpacePlacement::Orbital || (v->construction.placement == SpacePlacement::Carried && v->manifest && ResolveSpaceCraft(v->manifest->carrier) != nullptr && IsSpaceFlightActive(*ResolveSpaceCraft(v->manifest->carrier)))) continue;
			DetachSpaceManifestAtLostCenter(*v);
			ForgetLaunchCraft(*v); v->construction.home = {}; v->construction.placement = SpacePlacement::Unhomed; v->construction.paused = true;
		}
		auto *v = ResolveSpaceCraft(slots.workshop);
		if (v != nullptr && v->construction.placement == SpacePlacement::Assembling && v->construction.home == ref && v->owner == center->owner && !v->construction.paused) {
			assert(v->construction.remaining_ticks > 0);
			std::optional<PreparedSpaceReplacement> replacement;
			// A real Earth body already embodies its paid materials. Hold its last work tick until the exact original source and all whole holdings can settle safely at this owned center.
			if (v->construction.remaining_ticks == 1 && v->construction.replacement) {
				replacement = PrepareSpaceReplacement(*v->construction.replacement, GetSpaceVehicleModel(*v), v->owner, {{}, {}, ref});
				if (!replacement) continue;
			}
			if (--v->construction.remaining_ticks == 0) {
				v->construction.placement = SpacePlacement::SurfaceStored; slots.workshop = {};
				if (GetSpaceVehicleModel(*v) == 14) { slots.pad_queue.push_back(GetSpaceCraftReference(*v)); RefreshLaunchPad(*center); }
				else Place(*v, *center, LaunchFacilityZone::Storage);
				if (replacement) CommitSpaceReplacement(*v->construction.replacement, *v, std::move(*replacement)); // Native slot/placement completion precedes retirement, so no unfinished hull can acquire the original assets or credit.
				SetWindowClassesDirty(WindowClass::StationView);
			}
		}
		RefreshLaunchPad(*center);
	}
}

/** Check the actual native Earth producer before quoting or mutating paid materials.
 * @param center Real owned workshop/storage and native assembly slots.
 * @param model Canonical delivered Earth recipe.
 * @return Whether all physical/native identity capacities and the entire material bill are available.
 */
static bool CanAssemble(const Station &center, uint16_t model)
{
	const auto *d = GetSpaceVehicleConstruction(model);
	// A complete paid Earth producer needs the canonical role/year, current company, every exclusive workshop owner and real native storage/pool/issuance capacity before quoting any recipe.
	if (d == nullptr || (model != 14 && model != 15 && model != 19 && model != 20) || !center.launch_operations || !IsLaunchCenterTechnologyAvailable() ||
		TimerGameCalendar::year < d->introduction_year || center.owner != _current_company || center.launch_operations->workshop != SpaceCraftRef{} ||
		center.launch_operations->service != SpaceCraftRef{} || center.launch_operations->pod_workshop != SpacePodRef{} || StoredCount(GetLaunchCenterReference(center)) >= MAX_CENTER_CRAFT ||
		GetSpaceVehicleCount() >= MAX_SPACE_VEHICLES || GetNextSpaceVehicleGeneration() == UINT64_MAX || !Vehicle::CanAllocateItem()) return false;
	if (GetFreeUnitNumber(VehicleType::Space) == UINT16_MAX) return false; // A replacement retains its original unit until final settlement; admission must still fit a genuine independently counted assembly body.
	if (!Engine::IsValidID(GetSpaceEngine(model)) && !Engine::CanAllocateItem()) return false;
	const auto mass = GetMultiLayerPayloadMass(d->inputs); if (!mass || *mass != d->dry_mass_sixteenths) return false;
	// Inspect the complete actual station bill without moving any earlier label; a missing last component cannot partially commission a hull or spend stock.
	for (const auto &input : d->inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label);
		if (!IsValidCargoType(cargo) || !center.goods[cargo].HasData() || center.goods[cargo].GetData().cargo.AvailableCount() < input.quantity) return false;
	}
	return true;
}
/** Admit one actual material-paid Earth hull through its original physical workshop owner.
 * @param flags Quote or execute native assembly. @param station Actual center index. @param generation Exact center incarnation. @param model Canonical supported Earth recipe.
 * @return Full native capital cost and, on execute, one real Assembling body with an embodied material bill; caller owns top-level financial accounting.
 */
CommandCost CmdAssembleLaunchCraft(DoCommandFlags flags, StationID station, uint64_t generation, uint16_t model)
{
	auto *center = ResolveLaunchCenter({station, generation});
	if (center == nullptr || !CanAssemble(*center, model)) return CMD_ERROR;
	const auto &d = *GetSpaceVehicleConstruction(model);
	if (flags.Test(DoCommandFlag::Execute)) {
		if (!EnsureSpaceEngineDefinition(model)) return CMD_ERROR;
		auto *v = SpaceVehicle::Create();
		if (!RegisterSpaceVehicleIdentity(*v)) { delete v; return CMD_ERROR; }
		v->owner = _current_company; v->engine_type = GetSpaceEngine(model);
		v->unitnumber = GetFreeUnitNumber(VehicleType::Space); Company::Get(v->owner)->freeunits[VehicleType::Space].UseID(v->unitnumber);
		v->cargo_type = v->GetEngine()->GetDefaultCargoType(); v->cargo_cap = v->GetEngine()->VehInfo<SpaceVehicleInfo>().capacity;
		v->vcache.cached_cargo_age_period = Ticks::CARGO_AGING_TICKS; v->value = d.capital;
		v->build_year = TimerGameCalendar::year; v->max_age = v->GetEngine()->GetLifeLengthInDays(); v->reliability = 0xFFFF;
		v->mission.phase = SpaceMissionPhase::Holding; v->vehstatus.Set({VehState::Stopped, VehState::Hidden});
		const auto ref = GetLaunchCenterReference(*center);
		v->construction = {SpacePlacement::Assembling, ref, ref, INVALID_TILE, static_cast<uint16_t>(d.work_ticks), MULTILAYER_SPACE_CONTENT_VERSION, model, d.capital, false};
		if (model != 14) GetOrCreateSpaceManifest(*v).returns_remaining = GetMultiLayerSpaceBudget().returns_per_service; // Paid new assembly grants this allowance once; old chunk absence never does.
		for (const auto &input : d.inputs) {
			const auto cargo = GetCargoTypeByLabel(input.label);
			const bool consumed = HubCargoTransfer::Consume(center->goods[cargo].GetOrCreateData().cargo, input.quantity);
			assert(consumed); (void)consumed;
		}
		center->launch_operations->workshop = GetSpaceCraftReference(*v);
		GroupStatistics::CountEngine(v, 1); GroupStatistics::CountVehicle(v, 1);
		Place(*v, *center, LaunchFacilityZone::Workshop); SetWindowClassesDirty(WindowClass::StationView);
	}
	return CommandCost(ExpensesType::NewVehicles, d.capital);
}
/** Control only the remaining paid work of the exact original Earth body.
 * @param flags Quote or execute. @param vehicle Existing paid body index. @param generation Exact body incarnation. @param pause Requested work pause.
 * @return Zero-cost control or unchanged refusal; no materials, hull or sale credit is recreated.
 */
CommandCost CmdPauseLaunchAssembly(DoCommandFlags flags, VehicleID vehicle, uint64_t generation, bool pause)
{
	auto *v = ResolveSpaceCraft({vehicle, generation});
	if (v == nullptr || v->owner != _current_company || v->construction.placement != SpacePlacement::Assembling) return CMD_ERROR;
	auto *center = ResolveLaunchCenter(v->construction.home);
	if (center == nullptr || center->owner != v->owner || !center->launch_operations || center->launch_operations->workshop != GetSpaceCraftReference(*v)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) { v->construction.paused = pause; SetWindowClassesDirty(WindowClass::StationView); }
	return CommandCost{};
}
/** Reattach an existing unhomed paid body to the actual owned physical recovery center without purchasing another.
 * @param flags Quote or execute. @param station Actual center index. @param center_generation Exact center incarnation.
 * @param vehicle Existing retained body index. @param vehicle_generation Exact retained body incarnation.
 * @return Whole native slot/placement recovery or unchanged refusal, preserving paid work and service custody.
 */
CommandCost CmdRecoverLaunchCraft(DoCommandFlags flags, StationID station, uint64_t center_generation, VehicleID vehicle, uint64_t vehicle_generation)
{
	auto *center = ResolveLaunchCenter({station, center_generation}); auto *v = ResolveSpaceCraft({vehicle, vehicle_generation});
	if (center == nullptr || v == nullptr || !center->launch_operations || center->owner != _current_company || v->owner != center->owner ||
		v->construction.placement != SpacePlacement::Unhomed || StoredCount(GetLaunchCenterReference(*center)) >= MAX_CENTER_CRAFT) return CMD_ERROR;
	const auto area = GetLaunchCenterFootprint(center->launch_center->tile, center->launch_center->rotation);
	const bool service = v->manifest && v->manifest->service_ticks != 0;
	if (!area || !area->Contains(v->construction.surface_tile) || ((v->construction.remaining_ticks != 0 || service) &&
		(center->launch_operations->workshop != SpaceCraftRef{} || center->launch_operations->service != SpaceCraftRef{} || center->launch_operations->pod_workshop != SpacePodRef{}))) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		v->construction.home = GetLaunchCenterReference(*center); v->construction.paused = false;
		v->construction.placement = v->construction.remaining_ticks != 0 ? SpacePlacement::Assembling : SpacePlacement::SurfaceStored;
		if (v->construction.remaining_ticks != 0) { center->launch_operations->workshop = GetSpaceCraftReference(*v); Place(*v, *center, LaunchFacilityZone::Workshop); }
		else if (service) { center->launch_operations->service = GetSpaceCraftReference(*v); v->manifest->service_paused = false; Place(*v, *center, LaunchFacilityZone::Workshop); }
		else if (GetSpaceVehicleModel(*v) == 14) { center->launch_operations->pad_queue.push_back(GetSpaceCraftReference(*v)); RefreshLaunchPad(*center); }
		else Place(*v, *center, LaunchFacilityZone::Storage);
	}
	return CommandCost{}; // Reattach the existing paid hull; service has its own undelivered cost/stock boundary.
}

/** @copydoc CaptureLaunchOperations */
std::vector<LaunchOperationsRecord> CaptureLaunchOperations()
{
	std::vector<LaunchOperationsRecord> result;
	for (const auto *s : Station::Iterate()) if (s->launch_center && s->launch_operations) result.push_back({GetLaunchCenterReference(*s), *s->launch_operations});
	return result;
}
/** @copydoc ValidateLaunchOperationRecords */
bool ValidateLaunchOperationRecords(std::span<const LaunchOperationsRecord> records)
{
	if (records.size() > MAX_LAUNCH_CENTERS) return false;
	std::set<StationID> centers; std::set<VehicleID> assigned; std::set<SpacePodID> pods;
	for (const auto &r : records) {
		if (r.center.station >= StationID::End() || r.center.generation == 0 || !centers.insert(r.center.station).second || r.slots.pad_queue.size() > MAX_CENTER_CRAFT ||
			(unsigned(r.slots.workshop != SpaceCraftRef{}) + unsigned(r.slots.service != SpaceCraftRef{}) + unsigned(r.slots.pod_workshop != SpacePodRef{}) > 1)) return false;
		if (r.slots.pod_workshop != SpacePodRef{} && (r.slots.pod_workshop.pod >= SpacePodID::End() || r.slots.pod_workshop.generation == 0 || !pods.insert(r.slots.pod_workshop.pod).second)) return false;
		auto add = [&](SpaceCraftRef ref) { return ref.vehicle != VehicleID::Invalid() && ref.generation != 0 && assigned.insert(ref.vehicle).second; };
		if (r.slots.workshop != SpaceCraftRef{} && !add(r.slots.workshop)) return false;
		if (r.slots.service != SpaceCraftRef{} && !add(r.slots.service)) return false;
		if (r.slots.pad_active != SpaceCraftRef{} && !add(r.slots.pad_active)) return false;
		for (const auto ref : r.slots.pad_queue) if (!add(ref)) return false;
	}
	return true;
}
/** @copydoc ValidateLaunchProductionLinks */
bool ValidateLaunchProductionLinks(std::span<const SpaceProductionRecord> craft, std::span<const LaunchOperationsRecord> operations,
	std::span<const SpaceCraftRef> identities, std::span<const LaunchCenterRef> centers)
{
	if (craft.size() != identities.size() || operations.size() != centers.size() || !ValidateLaunchOperationRecords(operations)) return false;
	auto find = [&](SpaceCraftRef ref) -> const SpaceProductionRecord * {
		for (const auto &r : craft) if (r.craft == ref) return &r;
		return nullptr;
	};
	std::set<VehicleID> assigned;
	for (const auto ref : identities) if (find(ref) == nullptr) return false;
	for (const auto &op : operations) {
		if (std::find(centers.begin(), centers.end(), op.center) == centers.end()) return false;
		uint count = 0;
		for (const auto &r : craft) if (r.state.home == op.center && r.state.placement != SpacePlacement::Orbital) ++count;
		if (count > MAX_CENTER_CRAFT) return false;
		auto add = [&](SpaceCraftRef ref, SpacePlacement placement, bool rocket) {
			const auto *r = find(ref);
			return r != nullptr && r->state.home == op.center && r->state.placement == placement &&
				(!rocket || r->state.model == 14) && assigned.insert(ref.vehicle).second;
		};
		if (op.slots.workshop != SpaceCraftRef{} && !add(op.slots.workshop, SpacePlacement::Assembling, false)) return false;
		if (op.slots.service != SpaceCraftRef{} && !add(op.slots.service, SpacePlacement::SurfaceStored, false)) return false;
		if (op.slots.pad_active != SpaceCraftRef{} && !add(op.slots.pad_active, SpacePlacement::InTransit, true)) return false;
		for (const auto ref : op.slots.pad_queue) if (!add(ref, SpacePlacement::SurfaceStored, true)) return false;
	}
	for (const auto &r : craft) {
		if (r.state.content_version == 0) continue;
		if (r.state.placement != SpacePlacement::Unhomed && r.state.placement != SpacePlacement::Orbital && r.state.placement != SpacePlacement::InTransit && r.state.placement != SpacePlacement::Carried && std::find(centers.begin(), centers.end(), r.state.home) == centers.end()) return false;
		if ((r.state.placement == SpacePlacement::Assembling || (r.state.placement == SpacePlacement::SurfaceStored && r.state.model == 14)) && !assigned.contains(r.craft.vehicle)) return false;
	}
	return true;
}
/** Resolve already structurally admitted native slots against their actual paid bodies and current company/physical custody.
 * @param records Complete original slot snapshot. @return Whether every real workshop/service/pad assignment has exactly its corresponding surviving native owner.
 */
static bool PhysicalOperations(std::span<const LaunchOperationsRecord> records)
{
	if (records.size() != GetLaunchCenterReferences().size()) return false;
	std::set<VehicleID> assigned;
	for (const auto &r : records) {
		const auto *center = ResolveLaunchCenter(r.center); if (center == nullptr || StoredCount(r.center) > MAX_CENTER_CRAFT) return false;
		auto check = [&](SpaceCraftRef ref, SpacePlacement mode, LaunchFacilityZone zone) {
			const auto *v = ResolveSpaceCraft(ref);
			if (v == nullptr || v->owner != center->owner || v->construction.home != r.center || v->construction.placement != mode ||
				v->construction.surface_tile != GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, zone)->tile || !assigned.insert(ref.vehicle).second) return false;
			return true;
		};
		if (r.slots.workshop != SpaceCraftRef{} && !check(r.slots.workshop, SpacePlacement::Assembling, LaunchFacilityZone::Workshop)) return false;
		if (r.slots.service != SpaceCraftRef{}) {
			if (!check(r.slots.service, SpacePlacement::SurfaceStored, LaunchFacilityZone::Workshop)) return false;
			const auto *v = ResolveSpaceCraft(r.slots.service);
			if (!v->manifest || v->manifest->service_ticks == 0 || GetSpaceVehicleModel(*v) == 14) return false;
		}
		if (r.slots.pad_active != SpaceCraftRef{}) {
			const auto *v = ResolveSpaceCraft(r.slots.pad_active);
			if (v == nullptr || v->owner != center->owner || v->construction.home != r.center || v->construction.placement != SpacePlacement::InTransit || !IsSpaceFlightActive(*v) || !assigned.insert(v->index).second) return false;
		}
		for (size_t i = 0; i < r.slots.pad_queue.size(); ++i) {
			if (!check(r.slots.pad_queue[i], SpacePlacement::SurfaceStored, i == 0 && r.slots.pad_active == SpaceCraftRef{} ? LaunchFacilityZone::Pad : LaunchFacilityZone::Storage) || GetSpaceVehicleModel(*ResolveSpaceCraft(r.slots.pad_queue[i])) != 14) return false;
		}
	}
	for (const auto *v : SpaceVehicle::Iterate()) {
		if ((v->construction.placement == SpacePlacement::Assembling || (v->construction.placement == SpacePlacement::SurfaceStored && GetSpaceVehicleModel(*v) == 14)) && !assigned.contains(v->index)) return false;
		if (v->construction.placement == SpacePlacement::SurfaceStored && GetSpaceVehicleModel(*v) != 14) {
			const auto *center = ResolveLaunchCenter(v->construction.home);
			if (center == nullptr) return false;
			const auto op = std::find_if(records.begin(), records.end(), [&](const auto &r) { return r.center == v->construction.home; });
			if (op == records.end()) return false;
			const auto zone = op->slots.service == GetSpaceCraftReference(*v) ? LaunchFacilityZone::Workshop : LaunchFacilityZone::Storage;
			if (v->construction.surface_tile != GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, zone)->tile) return false;
		}
	}
	return true;
}
/** @copydoc ValidateLaunchOperations */
bool ValidateLaunchOperations()
{
	const auto records = CaptureLaunchOperations(); return ValidateLaunchOperationRecords(records) && PhysicalOperations(records);
}
/** @copydoc InstallLoadedLaunchOperations */
bool InstallLoadedLaunchOperations(std::span<const LaunchOperationsRecord> records)
{
	if (!ValidateLaunchOperationRecords(records) || !PhysicalOperations(records)) return false;
	for (const auto &r : records) *ResolveLaunchCenter(r.center)->launch_operations = r.slots;
	return true;
}
/** @copydoc ComputeLaunchProductionDigest */
uint64_t ComputeLaunchProductionDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Canonical saved owner field included in the same deterministic paid-production digest. */
	auto add = [&](uint64_t value) { for (uint i = 0; i < 8; ++i) { hash ^= (value >> (i * 8)) & 255; hash *= 1099511628211ULL; } };
	/** @param ref Exact original Surface facility incarnation, including its generation. */
	auto center = [&](LaunchCenterRef ref) { add(ref.station.base()); add(ref.generation); };
	/** @param ref Exact original native hull incarnation, including its generation. */
	auto craft = [&](SpaceCraftRef ref) { add(ref.vehicle.base()); add(ref.generation); };
	// Include each existing hull's complete paid receipt in native order, so reload/retry comparisons detect changed work, placement or original factory provenance.
	for (const auto &r : CaptureSpaceProduction()) {
		craft(r.craft); const auto &s = r.state; add(to_underlying(s.placement)); center(s.home); center(s.origin); add(s.surface_tile.base());
		add(s.remaining_ticks); add(s.content_version); add(s.model); add(s.capital_paid); add(s.paused); add(s.deployed);
		// Actual orbital provenance joins the same paid production digest; changing factory/serial cannot hide behind an unchanged hull model.
		add(s.assembly.structure.base()); add(s.assembly.generation); add(s.assembly_serial);
		add(s.replacement.has_value());
		if (s.replacement) {
			const auto &intent = *s.replacement; craft(intent.source); add(intent.source_model); add(intent.resume); add(intent.settled); add(intent.sale_credit);
		}
	}
	// Physical center slots and their ordered pad waiters remain in the same digest; an identical hull receipt cannot conceal duplicate or reordered native reservations.
	for (const auto &r : CaptureLaunchOperations()) { center(r.center); craft(r.slots.workshop); craft(r.slots.service); add(r.slots.pod_workshop.pod.base()); add(r.slots.pod_workshop.generation); craft(r.slots.pad_active); add(r.slots.pad_queue.size()); for (auto ref : r.slots.pad_queue) craft(ref); }
	return hash;
}
