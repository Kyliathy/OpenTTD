/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_pod.cpp Paid passive shells with native pool identity and retained custody. */
#include "stdafx.h"
#include "space_pod.h"
#include "space_manifest.h"
#include "space_transition.h"
#include "space_vehicle.h"
#include "launch_operations.h"
#include "launch_center_cmd.h"
#include "multilayer_space_content.h"
#include "station_base.h"
#include "company_base.h"
#include "company_func.h"
#include "hub_cargo.h"
#include "command_func.h"
#include "map_func.h"
#include "window_func.h"
#include "core/pool_func.hpp"
#include <set>
#include "safeguards.h"

SpacePodPool _space_pod_pool("SpacePod");
INSTANTIATE_POOL_METHODS(SpacePod)
static uint64_t next_generation = 1;

SpacePodRef GetSpacePodReference(const SpacePod &pod) { return {pod.index, pod.generation}; }
SpacePod *ResolveSpacePod(SpacePodRef ref)
{
	auto *pod = SpacePod::GetIfValid(ref.pod);
	return pod != nullptr && ref.generation != 0 && pod->generation == ref.generation ? pod : nullptr;
}
uint64_t GetNextSpacePodGeneration() { return next_generation; }
void ResetSpacePods() { next_generation = 1; ResetSpacePodLoadStaging(); }
SpacePod::~SpacePod()
{
	if (SpacePod::CleaningPool()) { if (this->descent) this->descent->packets.OnCleanPool(); return; }
	const auto ref = GetSpacePodReference(*this);
	if (auto *center = ResolveLaunchCenter(this->state.home); center != nullptr && center->launch_operations && center->launch_operations->pod_workshop == ref) center->launch_operations->pod_workshop = {};
	if (auto *craft = ResolveSpaceCraft(this->state.craft); craft != nullptr && craft->manifest) std::erase(craft->manifest->pods, ref);
}
uint CountLaunchPods(LaunchCenterRef center)
{
	uint count = 0;
	for (const auto *pod : SpacePod::Iterate()) if (pod->state.home == center && (pod->state.custody == SpacePodCustody::Assembling || pod->state.custody == SpacePodCustody::CenterStored)) ++count;
	return count;
}
bool HasLaunchPods(LaunchCenterRef center) { return center.generation != 0 && CountLaunchPods(center) != 0; }
static bool WorkshopFree(const LaunchOperations &op)
{
	return op.workshop == SpaceCraftRef{} && op.service == SpaceCraftRef{} && op.pod_workshop == SpacePodRef{};
}
static void PlacePod(SpacePod &pod, const Station &center, bool assembling)
{
	auto &s = pod.state;
	s.home = GetLaunchCenterReference(center); s.craft = {}; s.custody = assembling ? SpacePodCustody::Assembling : SpacePodCustody::CenterStored;
	s.paused = false; s.position = *GetLaunchCenterPlaneReference(center.launch_center->tile);
	s.surface_tile = GetLaunchFacilityZone(center.launch_center->tile, center.launch_center->rotation, assembling ? LaunchFacilityZone::Workshop : LaunchFacilityZone::Storage)->tile;
}
CommandCost CmdAssembleLaunchPod(DoCommandFlags flags, StationID station, uint64_t generation)
{
	auto *center = ResolveLaunchCenter({station, generation}); const auto &d = *GetSpaceConstructionDefinition(SpaceBuildKind::Pod);
	if (center == nullptr || center->owner != _current_company || !center->launch_operations || !IsLaunchCenterTechnologyAvailable() ||
		TimerGameCalendar::year < d.introduction_year || !WorkshopFree(*center->launch_operations) ||
		CountLaunchPods(GetLaunchCenterReference(*center)) >= GetMultiLayerSpaceBudget().launch_pod_cap || next_generation == UINT64_MAX || !SpacePod::CanAllocateItem()) return CMD_ERROR;
	for (const auto &input : d.inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label);
		if (!IsValidCargoType(cargo) || !center->goods[cargo].HasData() || center->goods[cargo].GetData().cargo.AvailableCount() < input.quantity) return CMD_ERROR;
	}
	if (flags.Test(DoCommandFlag::Execute)) {
		auto *pod = SpacePod::Create(); pod->generation = next_generation++;
		pod->state.owner = center->owner; pod->state.origin = GetLaunchCenterReference(*center);
		pod->state.content_version = MULTILAYER_SPACE_CONTENT_VERSION; pod->state.capital_paid = d.capital; pod->state.remaining_ticks = static_cast<uint16_t>(d.work_ticks);
		PlacePod(*pod, *center, true);
		for (const auto &input : d.inputs) {
			const auto cargo = GetCargoTypeByLabel(input.label); const bool consumed = HubCargoTransfer::Consume(center->goods[cargo].GetOrCreateData().cargo, input.quantity);
			assert(consumed); (void)consumed;
		}
		center->launch_operations->pod_workshop = GetSpacePodReference(*pod); SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost(ExpensesType::NewVehicles, d.capital);
}
CommandCost CmdPauseLaunchPod(DoCommandFlags flags, SpacePodID id, uint64_t generation, bool pause)
{
	auto *pod = ResolveSpacePod({id, generation}); if (pod == nullptr || pod->state.owner != _current_company || pod->state.custody != SpacePodCustody::Assembling) return CMD_ERROR;
	auto *center = ResolveLaunchCenter(pod->state.home);
	if (center == nullptr || center->owner != pod->state.owner || !center->launch_operations || center->launch_operations->pod_workshop != GetSpacePodReference(*pod)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) { pod->state.paused = pause; SetWindowClassesDirty(WindowClass::LaunchAssembly); }
	return CommandCost{};
}
CommandCost CmdRecoverLaunchPod(DoCommandFlags flags, StationID station, uint64_t center_generation, SpacePodID id, uint64_t generation)
{
	auto *pod = ResolveSpacePod({id, generation}); auto *center = ResolveLaunchCenter({station, center_generation});
	if (pod == nullptr || center == nullptr || pod->state.owner != _current_company || center->owner != pod->state.owner || !center->launch_operations ||
		pod->state.custody != SpacePodCustody::Unhomed || CountLaunchPods(GetLaunchCenterReference(*center)) >= GetMultiLayerSpaceBudget().launch_pod_cap ||
		(pod->state.remaining_ticks != 0 && !WorkshopFree(*center->launch_operations))) return CMD_ERROR;
	const auto area = GetLaunchCenterFootprint(center->launch_center->tile, center->launch_center->rotation);
	if (!area || !area->Contains(pod->state.surface_tile)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		PlacePod(*pod, *center, pod->state.remaining_ticks != 0);
		if (pod->state.remaining_ticks != 0) center->launch_operations->pod_workshop = GetSpacePodReference(*pod);
		SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost{}; // Same paid shell and work; never debit the recipe again.
}
CommandCost CmdMoveLaunchPod(DoCommandFlags flags, StationID station, uint64_t center_generation, VehicleID vehicle, uint64_t craft_generation, SpacePodID id, uint64_t generation, bool unload)
{
	auto *pod = ResolveSpacePod({id, generation}); auto *center = ResolveLaunchCenter({station, center_generation}); auto *craft = ResolveSpaceCraft({vehicle, craft_generation});
	if (pod == nullptr || center == nullptr || craft == nullptr || pod->state.owner != _current_company || center->owner != pod->state.owner || craft->owner != pod->state.owner ||
		craft->construction.placement != SpacePlacement::SurfaceStored || craft->construction.home != GetLaunchCenterReference(*center) ||
		(craft->manifest && craft->manifest->service_ticks != 0)) return CMD_ERROR;
	const auto model = GetSpaceVehicleModel(*craft); if (model != 14 && model != 15 && model != 19 && model != 20) return CMD_ERROR;
	const auto ref = GetSpacePodReference(*pod);
	const auto craft_ref = GetSpaceCraftReference(*craft);
	if (unload) {
		if (pod->state.custody != SpacePodCustody::Craft || pod->state.craft != craft_ref || !craft->manifest ||
			std::count(craft->manifest->pods.begin(), craft->manifest->pods.end(), ref) != 1 || CountLaunchPods(GetLaunchCenterReference(*center)) >= GetMultiLayerSpaceBudget().launch_pod_cap) return CMD_ERROR;
	} else {
		const size_t cap = model == 14 ? GetMultiLayerSpaceBudget().launch_pod_cap : GetMultiLayerSpaceBudget().starting_pods;
		if (pod->state.custody != SpacePodCustody::CenterStored || pod->state.home != GetLaunchCenterReference(*center) || (craft->manifest && craft->manifest->pods.size() >= cap)) return CMD_ERROR;
		const auto mass = GetSpacePayloadMass(*craft);
		const auto extra = GetSpaceConstructionDefinition(SpaceBuildKind::Pod)->dry_mass_sixteenths;
		if (model == 14 && (mass > GetMultiLayerSpaceBudget().rocket_payload_sixteenths || extra > GetMultiLayerSpaceBudget().rocket_payload_sixteenths - mass)) return CMD_ERROR;
	}
	if (flags.Test(DoCommandFlag::Execute)) {
		auto &m = GetOrCreateSpaceManifest(*craft);
		if (unload) { std::erase(m.pods, ref); PlacePod(*pod, *center, false); }
		else { m.pods.push_back(ref); pod->state.custody = SpacePodCustody::Craft; pod->state.home = {}; pod->state.craft = craft_ref; SyncSpacePods(*craft); }
		SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost{};
}
void TickSpacePods()
{
	for (auto *pod : SpacePod::Iterate()) {
		auto &s = pod->state;
		if (s.home == LaunchCenterRef{}) continue;
		auto *center = ResolveLaunchCenter(s.home);
		if (center == nullptr || center->owner != s.owner || !center->launch_operations) { ForgetSpacePodCenter(s.home); continue; }
		if (s.custody != SpacePodCustody::Assembling || s.paused) continue;
		if (center->launch_operations->pod_workshop != GetSpacePodReference(*pod)) { s.paused = true; continue; }
		assert(s.remaining_ticks > 0);
		if (--s.remaining_ticks == 0) { center->launch_operations->pod_workshop = {}; PlacePod(*pod, *center, false); SetWindowClassesDirty(WindowClass::LaunchAssembly); }
	}
}
void SyncSpacePods(const SpaceVehicle &craft)
{
	if (!craft.manifest) return;
	for (const auto ref : craft.manifest->pods) if (auto *pod = ResolveSpacePod(ref); pod != nullptr && pod->state.craft == GetSpaceCraftReference(craft)) {
		pod->state.position = craft.position; pod->state.surface_tile = craft.construction.surface_tile;
	}
}
void ForgetSpacePodCraft(const SpaceVehicle &craft)
{
	if (!craft.manifest) return;
	for (const auto ref : craft.manifest->pods) if (auto *pod = ResolveSpacePod(ref); pod != nullptr && pod->state.craft == GetSpaceCraftReference(craft)) {
		auto &s = pod->state; s.craft = {}; s.home = {}; s.paused = true;
		s.custody = GetSpaceVehicleLayer(craft) == MapLayer::Space ? SpacePodCustody::Drifting : SpacePodCustody::Unhomed;
	}
}
void ForgetSpacePodCenter(LaunchCenterRef center)
{
	if (center == LaunchCenterRef{}) return;
	for (auto *pod : SpacePod::Iterate()) if (pod->state.home == center) { pod->state.home = {}; pod->state.custody = SpacePodCustody::Unhomed; pod->state.paused = true; }
	if (auto *s = ResolveLaunchCenter(center); s != nullptr && s->launch_operations) s->launch_operations->pod_workshop = {};
}
void ChangeSpacePodOwnership(Owner old_owner, Owner new_owner)
{
	for (auto *pod : SpacePod::Iterate()) if (pod->state.owner == old_owner) {
		pod->state.owner = new_owner == INVALID_OWNER ? OWNER_NONE : new_owner;
		if (new_owner == INVALID_OWNER) {
			if (auto *center = ResolveLaunchCenter(pod->state.home); center != nullptr && center->launch_operations && center->launch_operations->pod_workshop == GetSpacePodReference(*pod)) center->launch_operations->pod_workshop = {};
			pod->state.home = {}; pod->state.craft = {};
			if (pod->descent) { pod->descent->blocked = SpaceDescentBlock::CompanyRemoved; }
			else { pod->state.paused = true; if (pod->state.custody != SpacePodCustody::Drifting) pod->state.custody = SpacePodCustody::Unhomed; }
		}
	}
}

std::vector<SpacePodRecord> CaptureSpacePods()
{
	std::vector<SpacePodRecord> records;
	for (const auto *pod : SpacePod::Iterate()) records.push_back({GetSpacePodReference(*pod), pod->state});
	return records;
}
static bool ValidCenter(LaunchCenterRef ref) { return ref.station < StationID::End() && ref.generation != 0; }
bool ValidateSpacePodRecords(std::span<const SpacePodRecord> records, uint64_t next, uint32_t width, uint32_t height)
{
	if (records.size() > SpacePodID::End().base() || next == 0 || width < 64 || height < 64 || width > 4096 || height > 4096) return false;
	const auto &d = *GetSpaceConstructionDefinition(SpaceBuildKind::Pod);
	std::set<SpacePodID> ids; std::set<uint64_t> generations;
	for (const auto &r : records) {
		const auto &s = r.state;
		if (r.ref.pod >= SpacePodID::End() || r.ref.generation == 0 || r.ref.generation >= next || !ids.insert(r.ref.pod).second || !generations.insert(r.ref.generation).second ||
			s.custody >= SpacePodCustody::End || s.content_version != MULTILAYER_SPACE_CONTENT_VERSION || s.capital_paid != d.capital || !ValidCenter(s.origin) ||
			s.surface_tile.base() >= uint64_t{width} * height || s.remaining_ticks > d.work_ticks ||
			(s.owner >= MAX_COMPANIES && s.owner != OWNER_NONE)) return false;
		if (s.custody == SpacePodCustody::Assembling || s.custody == SpacePodCustody::CenterStored) {
			if (!ValidCenter(s.home) || s.craft != SpaceCraftRef{} || s.owner == OWNER_NONE ||
				(s.custody == SpacePodCustody::Assembling ? s.remaining_ticks == 0 : s.remaining_ticks != 0 || s.paused)) return false;
		} else if (s.custody == SpacePodCustody::Craft) {
			if (s.home != LaunchCenterRef{} || s.craft.vehicle >= VehicleID::End() || s.craft.generation == 0 || s.remaining_ticks != 0 || s.paused || s.owner == OWNER_NONE) return false;
		} else if (s.custody == SpacePodCustody::Descent) {
			if (s.home != LaunchCenterRef{} || s.craft != SpaceCraftRef{} || s.remaining_ticks != 0 || s.paused) return false;
		} else if (s.home != LaunchCenterRef{} || s.craft != SpaceCraftRef{} || !s.paused || (s.custody == SpacePodCustody::Drifting && s.remaining_ticks != 0)) return false;
		if (s.position.x < 0 || s.position.y < 0 || int64_t{s.position.x} >= int64_t{width} * SPACE_Q16 || int64_t{s.position.y} >= int64_t{height} * SPACE_Q16) return false;
	}
	return true;
}
bool ValidateSpacePodLinks(std::span<const SpacePodRecord> pods, std::span<const SpaceProductionRecord> craft,
	std::span<const LaunchOperationsRecord> operations, std::span<const SpaceManifestRecord> manifests)
{
	std::set<SpacePodID> assigned;
	auto find = [&](SpacePodRef ref) -> const SpacePodState * { for (const auto &pod : pods) if (pod.ref == ref) return &pod.state; return nullptr; };
	for (const auto &op : operations) {
		uint count = 0; for (const auto &pod : pods) if (pod.state.home == op.center) ++count;
		if (count > GetMultiLayerSpaceBudget().launch_pod_cap) return false;
		if (op.slots.pod_workshop != SpacePodRef{}) {
			const auto *s = find(op.slots.pod_workshop);
			if (s == nullptr || s->home != op.center || s->custody != SpacePodCustody::Assembling || !assigned.insert(op.slots.pod_workshop.pod).second) return false;
		}
	}
	for (const auto &m : manifests) {
		const SpaceConstructionState *s = nullptr; for (const auto &r : craft) if (r.craft == m.craft) s = &r.state;
		if (s == nullptr || m.pods.size() > (s->model == 14 ? GetMultiLayerSpaceBudget().launch_pod_cap : GetMultiLayerSpaceBudget().starting_pods)) return false;
		for (const auto ref : m.pods) {
			const auto *pod = find(ref);
			if (pod == nullptr || pod->custody != SpacePodCustody::Craft || pod->craft != m.craft || !assigned.insert(ref.pod).second) return false;
		}
	}
	for (const auto &r : pods) {
		const auto &s = r.state;
		if (s.custody == SpacePodCustody::Assembling || s.custody == SpacePodCustody::Craft) { if (!assigned.contains(r.ref.pod)) return false; }
		if (s.home != LaunchCenterRef{} && std::none_of(operations.begin(), operations.end(), [&](const auto &op) { return op.center == s.home; })) return false;
	}
	return true;
}
bool ValidateSpacePods()
{
	const auto records = CaptureSpacePods();
	if (!ValidateSpacePodRecords(records, next_generation, Map::SizeX(), Map::SizeY()) ||
		!ValidateSpacePodLinks(records, CaptureSpaceProduction(), CaptureLaunchOperations(), CaptureSpaceManifests())) return false;
	for (const auto &r : records) {
		const auto &s = r.state;
		if (s.origin.generation >= GetNextLaunchCenterGeneration() || (s.owner != OWNER_NONE && !Company::IsValidID(s.owner))) return false;
		if (s.home != LaunchCenterRef{}) {
			const auto *center = ResolveLaunchCenter(s.home); if (center == nullptr || center->owner != s.owner) return false;
			const auto zone = s.custody == SpacePodCustody::Assembling ? LaunchFacilityZone::Workshop : LaunchFacilityZone::Storage;
			if (s.surface_tile != GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, zone)->tile || s.position != *GetLaunchCenterPlaneReference(center->launch_center->tile)) return false;
		}
		if (s.custody == SpacePodCustody::Craft) {
			const auto *v = ResolveSpaceCraft(s.craft);
			if (v == nullptr || v->owner != s.owner || s.position != v->position || s.surface_tile != v->construction.surface_tile) return false;
		}
	}
	return true;
}
bool InstallLoadedSpacePods(std::span<const SpacePodRecord> records, uint64_t next)
{
	if (SpacePod::GetNumItems() != 0 || !ValidateSpacePodRecords(records, next, Map::SizeX(), Map::SizeY()) || !SpacePod::CanAllocateItem(records.size())) return false;
	for (const auto &r : records) { auto *pod = SpacePod::CreateAtIndex(r.ref.pod); pod->generation = r.ref.generation; pod->state = r.state; }
	next_generation = next; return true; // Cross-family/physical validation runs after the native center/manifest owners are installed.
}
uint64_t ComputeSpacePodDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	auto add = [&](uint64_t value) { for (uint i = 0; i < 8; ++i) { hash ^= (value >> (i * 8)) & 255; hash *= 1099511628211ULL; } };
	auto center = [&](LaunchCenterRef ref) { add(ref.station.base()); add(ref.generation); };
	add(next_generation);
	for (const auto &r : CaptureSpacePods()) {
		add(r.ref.pod.base()); add(r.ref.generation); const auto &s = r.state; add(s.owner.base()); add(to_underlying(s.custody)); center(s.home); center(s.origin);
		add(s.craft.vehicle.base()); add(s.craft.generation); add(s.surface_tile.base()); add(static_cast<uint32_t>(s.position.x)); add(static_cast<uint32_t>(s.position.y));
		add(s.remaining_ticks); add(s.content_version); add(s.capital_paid); add(s.paused);
	}
	return hash;
}
