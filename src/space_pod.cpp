/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_pod.cpp Paid passive shells with native pool identity and retained custody.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups8C/13
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; preserve the actual shell's Surface history at a paid orbital factory product.
 */
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

/** @copydoc GetSpacePodReference */
SpacePodRef GetSpacePodReference(const SpacePod &pod) { return {pod.index, pod.generation}; }
/** @copydoc ResolveSpacePod */
SpacePod *ResolveSpacePod(SpacePodRef ref)
{
	auto *pod = SpacePod::GetIfValid(ref.pod);
	return pod != nullptr && ref.generation != 0 && pod->generation == ref.generation ? pod : nullptr;
}
/** @copydoc GetNextSpacePodGeneration */
uint64_t GetNextSpacePodGeneration() { return next_generation; }
/** @copydoc ResetSpacePods */
void ResetSpacePods() { next_generation = 1; ResetSpacePodLoadStaging(); }
/** Release this shell's exact original workshop/carrier references; native bulk teardown separately owns packet disposal. */
SpacePod::~SpacePod()
{
	if (SpacePod::CleaningPool()) { if (this->descent) this->descent->packets.OnCleanPool(); return; }
	const auto ref = GetSpacePodReference(*this);
	// Only the exact original slot and carrier lose this reference; a retired/reused native identity cannot revoke another shell's custody.
	if (auto *center = ResolveLaunchCenter(this->state.home); center != nullptr && center->launch_operations && center->launch_operations->pod_workshop == ref) center->launch_operations->pod_workshop = {};
	if (auto *craft = ResolveSpaceCraft(this->state.craft); craft != nullptr && craft->manifest) std::erase(craft->manifest->pods, ref);
}
/** @copydoc CountLaunchPods */
uint CountLaunchPods(LaunchCenterRef center)
{
	uint count = 0;
	// Stored and unfinished original shells both occupy real center capacity; packing an existing shell releases its Surface slot rather than cloning capacity.
	for (const auto *pod : SpacePod::Iterate()) if (pod->state.home == center && (pod->state.custody == SpacePodCustody::Assembling || pod->state.custody == SpacePodCustody::CenterStored)) ++count;
	return count;
}
/** @copydoc HasLaunchPods */
bool HasLaunchPods(LaunchCenterRef center) { return center.generation != 0 && CountLaunchPods(center) != 0; }
/** @param op Actual shared Surface work slots. @return Whether no hull, service or passive shell owns the original workshop. */
static bool WorkshopFree(const LaunchOperations &op)
{
	return op.workshop == SpaceCraftRef{} && op.service == SpaceCraftRef{} && op.pod_workshop == SpacePodRef{};
}
/** Set an admitted actual shell's real Surface custody without paying or issuing another shell.
 * @param pod Original paid passive asset. @param center Actual authorized owned facility. @param assembling Whether its unfinished recipe retains the workshop.
 */
static void PlacePod(SpacePod &pod, const Station &center, bool assembling)
{
	auto &s = pod.state;
	s.home = GetLaunchCenterReference(center); s.craft = {}; s.custody = assembling ? SpacePodCustody::Assembling : SpacePodCustody::CenterStored;
	s.paused = false; s.position = *GetLaunchCenterPlaneReference(center.launch_center->tile);
	s.surface_tile = GetLaunchFacilityZone(center.launch_center->tile, center.launch_center->rotation, assembling ? LaunchFacilityZone::Workshop : LaunchFacilityZone::Storage)->tile;
}
/** Quote or reserve one material-paid shell at the original Earth workshop.
 * @param flags Estimate or synchronized execution. @param station Actual center ID. @param generation Exact facility incarnation.
 * @return Canonical capital charge or complete refusal; the native command kernel owns company accounting.
 */
CommandCost CmdAssembleLaunchPod(DoCommandFlags flags, StationID station, uint64_t generation)
{
	auto *center = ResolveLaunchCenter({station, generation}); const auto &d = *GetSpaceConstructionDefinition(SpaceBuildKind::Pod);
	// Technology, physical capacity and the one actual workshop must all admit the entire paid recipe before reserving materials or issuing a passive asset.
	if (center == nullptr || center->owner != _current_company || !center->launch_operations || !IsLaunchCenterTechnologyAvailable() ||
		TimerGameCalendar::year < d.introduction_year || !WorkshopFree(*center->launch_operations) ||
		CountLaunchPods(GetLaunchCenterReference(*center)) >= GetMultiLayerSpaceBudget().launch_pod_cap || next_generation == UINT64_MAX || !SpacePod::CanAllocateItem()) return CMD_ERROR;
	// Preflight every actual input label before consuming any; a missing component cannot leave a partial shell or partial bill.
	for (const auto &input : d.inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label);
		// Every canonical construction input must exist as real available station cargo before a paid shell can reserve any material.
		if (!IsValidCargoType(cargo) || !center->goods[cargo].HasData() || center->goods[cargo].GetData().cargo.AvailableCount() < input.quantity) return CMD_ERROR;
	}
	if (flags.Test(DoCommandFlag::Execute)) {
		auto *pod = SpacePod::Create(); pod->generation = next_generation++;
		pod->state.owner = center->owner; pod->state.origin = GetLaunchCenterReference(*center);
		pod->state.content_version = MULTILAYER_SPACE_CONTENT_VERSION; pod->state.capital_paid = d.capital; pod->state.remaining_ticks = static_cast<uint16_t>(d.work_ticks);
		PlacePod(*pod, *center, true);
		// The new shell embodies exactly this original whole bill once; pausing/recovering its unfinished work cannot pay or reserve it again.
		for (const auto &input : d.inputs) {
			const auto cargo = GetCargoTypeByLabel(input.label); const bool consumed = HubCargoTransfer::Consume(center->goods[cargo].GetOrCreateData().cargo, input.quantity);
			assert(consumed); (void)consumed;
		}
		center->launch_operations->pod_workshop = GetSpacePodReference(*pod); SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost(ExpensesType::NewVehicles, d.capital);
}
/** Control only the existing shell's actual Earth workshop work.
 * @param flags Estimate or execute. @param id Original shell ID. @param generation Exact shell incarnation. @param pause True pauses, false resumes the same paid countdown.
 * @return Control admission or unchanged refusal; no stock, capital or new work is granted.
 */
CommandCost CmdPauseLaunchPod(DoCommandFlags flags, SpacePodID id, uint64_t generation, bool pause)
{
	// A surviving owned unfinished shell and its exact still-owned workshop must agree; another shell or reused center cannot supply a work slot.
	auto *pod = ResolveSpacePod({id, generation}); if (pod == nullptr || pod->state.owner != _current_company || pod->state.custody != SpacePodCustody::Assembling) return CMD_ERROR;
	auto *center = ResolveLaunchCenter(pod->state.home);
	// Only the exact original owned active workshop can pause this shell; a lost/foreign facility cannot manufacture work authority.
	if (center == nullptr || center->owner != pod->state.owner || !center->launch_operations || center->launch_operations->pod_workshop != GetSpacePodReference(*pod)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) { pod->state.paused = pause; SetWindowClassesDirty(WindowClass::LaunchAssembly); }
	return CommandCost{};
}
/** Recover the same unhomed Surface shell at its actual physical facility footprint.
 * @param flags Estimate or execute. @param station Actual receiving center. @param center_generation Exact facility incarnation.
 * @param id Original paid shell ID. @param generation Exact shell incarnation.
 * @return Recovery admission or unchanged refusal, without repaying or recreating the original recipe.
 */
CommandCost CmdRecoverLaunchPod(DoCommandFlags flags, StationID station, uint64_t center_generation, SpacePodID id, uint64_t generation)
{
	auto *pod = ResolveSpacePod({id, generation}); auto *center = ResolveLaunchCenter({station, center_generation});
	// Ownership, storage and unfinished workshop capacity must all admit this real unhomed Surface asset; orbital drifting history is not a teleport/recovery permission.
	if (pod == nullptr || center == nullptr || pod->state.owner != _current_company || center->owner != pod->state.owner || !center->launch_operations ||
		pod->state.custody != SpacePodCustody::Unhomed || CountLaunchPods(GetLaunchCenterReference(*center)) >= GetMultiLayerSpaceBudget().launch_pod_cap ||
		(pod->state.remaining_ticks != 0 && !WorkshopFree(*center->launch_operations))) return CMD_ERROR;
	const auto area = GetLaunchCenterFootprint(center->launch_center->tile, center->launch_center->rotation);
	// Recovery requires the actual shell inside the original facility storage geometry; historical Surface coordinates grant no remote retrieval.
	if (!area || !area->Contains(pod->state.surface_tile)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		PlacePod(*pod, *center, pod->state.remaining_ticks != 0);
		if (pod->state.remaining_ticks != 0) center->launch_operations->pod_workshop = GetSpacePodReference(*pod);
		SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost{}; // Same paid shell and work; never debit the recipe again.
}
/** Move one original paid shell between an actual Surface storage slot and an actual same-center hull.
 * @param flags Estimate or execute. @param station Exact facility ID. @param center_generation Original facility generation.
 * @param vehicle Actual carrier ID. @param craft_generation Exact carrier incarnation. @param id Original shell ID. @param generation Exact shell incarnation.
 * @param unload True returns this shell to the facility; false packs it into this same carrier.
 * @return Complete custody/mass/capacity admission or unchanged refusal; no shell or allowance is created.
 */
CommandCost CmdMoveLaunchPod(DoCommandFlags flags, StationID station, uint64_t center_generation, VehicleID vehicle, uint64_t craft_generation, SpacePodID id, uint64_t generation, bool unload)
{
	auto *pod = ResolveSpacePod({id, generation}); auto *center = ResolveLaunchCenter({station, center_generation}); auto *craft = ResolveSpaceCraft({vehicle, craft_generation});
	// Surface packing belongs only to actual same-owner stored hulls at their real center; an orbital factory product cannot borrow an invented Earth home.
	if (pod == nullptr || center == nullptr || craft == nullptr || pod->state.owner != _current_company || center->owner != pod->state.owner || craft->owner != pod->state.owner ||
		craft->construction.placement != SpacePlacement::SurfaceStored || craft->construction.home != GetLaunchCenterReference(*center) ||
		(craft->manifest && craft->manifest->service_ticks != 0)) return CMD_ERROR;
	// Surface packing is restricted to the original Earth-capable carriers; orbital-only factory roles cannot gain a fabricated Earth home.
	const auto model = GetSpaceVehicleModel(*craft); if (model != 14 && model != 15 && model != 19 && model != 20) return CMD_ERROR;
	const auto ref = GetSpacePodReference(*pod);
	const auto craft_ref = GetSpaceCraftReference(*craft);
	if (unload) {
		// Only the carrier's uniquely referenced actual shell can return to available Surface storage; missing custody leaves both physical owners unchanged.
		if (pod->state.custody != SpacePodCustody::Craft || pod->state.craft != craft_ref || !craft->manifest ||
			std::count(craft->manifest->pods.begin(), craft->manifest->pods.end(), ref) != 1 || CountLaunchPods(GetLaunchCenterReference(*center)) >= GetMultiLayerSpaceBudget().launch_pod_cap) return CMD_ERROR;
	} else {
		const size_t cap = model == 14 ? GetMultiLayerSpaceBudget().launch_pod_cap : GetMultiLayerSpaceBudget().starting_pods;
		// Packing consumes this exact stored shell and one real carrier slot; rocket payload headroom also includes every already loaded physical asset.
		if (pod->state.custody != SpacePodCustody::CenterStored || pod->state.home != GetLaunchCenterReference(*center) || (craft->manifest && craft->manifest->pods.size() >= cap)) return CMD_ERROR;
		const auto mass = GetSpacePayloadMass(*craft);
		const auto extra = GetSpaceConstructionDefinition(SpaceBuildKind::Pod)->dry_mass_sixteenths;
		// The entire added shell mass must fit the rocket payload alongside its existing paid assets before packing.
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
/** @copydoc TickSpacePods */
void TickSpacePods()
{
	// Each actual Surface job earns at most one original countdown opportunity; lost ownership/workshop and pauses retain the same paid shell without progress.
	for (auto *pod : SpacePod::Iterate()) {
		auto &s = pod->state;
		if (s.home == LaunchCenterRef{}) continue;
		auto *center = ResolveLaunchCenter(s.home);
		// Lost or foreign original facilities retire that binding without recreating paid shells or moving work to another center.
		if (center == nullptr || center->owner != s.owner || !center->launch_operations) { ForgetSpacePodCenter(s.home); continue; }
		// Only an actual unpaused construction owner earns work; stored shells and custody changes do not advance production.
		if (s.custody != SpacePodCustody::Assembling || s.paused) continue;
		if (center->launch_operations->pod_workshop != GetSpacePodReference(*pod)) { s.paused = true; continue; }
		assert(s.remaining_ticks > 0);
		if (--s.remaining_ticks == 0) { center->launch_operations->pod_workshop = {}; PlacePod(*pod, *center, false); SetWindowClassesDirty(WindowClass::LaunchAssembly); }
	}
}
/** @copydoc SyncSpacePods */
void SyncSpacePods(const SpaceVehicle &craft)
{
	if (!craft.manifest) return;
	// Only this exact incarnation's actual shells follow its Space position. A factory product never had an Earth workshop tile, so it preserves each shell's last real Surface custody instead of replacing history with INVALID_TILE.
	for (const auto ref : craft.manifest->pods) if (auto *pod = ResolveSpacePod(ref); pod != nullptr && pod->state.craft == GetSpaceCraftReference(craft)) {
		pod->state.position = craft.position;
		if (craft.construction.surface_tile != INVALID_TILE) pod->state.surface_tile = craft.construction.surface_tile;
	}
}
/** @copydoc IsSpacePodCarrierSurfaceTileValid */
bool IsSpacePodCarrierSurfaceTileValid(const SpacePodState &pod, const SpaceConstructionState &craft)
{
	if (craft.surface_tile != INVALID_TILE) return pod.surface_tile == craft.surface_tile;
	// Only genuine paid orbital factory provenance permits retained shell history; a missing Surface location on an arbitrary/unpaid hull is not a fallback home.
	return craft.placement == SpacePlacement::Orbital && craft.assembly != SpaceStructureRef{} && craft.assembly_serial != 0 &&
		IsSupportedSpaceVehicleContentVersion(craft.content_version, craft.model) && CanCommissionSpaceVehicle(craft.model, MultiLayerCommissioningFacility::OrbitalAssembly, true);
}
/** @copydoc ForgetSpacePodCraft */
void ForgetSpacePodCraft(const SpaceVehicle &craft)
{
	if (!craft.manifest) return;
	// Release only the disappearing incarnation's exact owned shells, retaining their actual Space pose/Surface history under explicit non-carried custody.
	for (const auto ref : craft.manifest->pods) if (auto *pod = ResolveSpacePod(ref); pod != nullptr && pod->state.craft == GetSpaceCraftReference(craft)) {
		auto &s = pod->state; s.craft = {}; s.home = {}; s.paused = true;
		s.custody = GetSpaceVehicleLayer(craft) == MapLayer::Space ? SpacePodCustody::Drifting : SpacePodCustody::Unhomed;
	}
}
/** @copydoc ForgetSpacePodCenter */
void ForgetSpacePodCenter(LaunchCenterRef center)
{
	if (center == LaunchCenterRef{}) return;
	// Every dependency follows the exact retired facility generation; retained shells and paid unfinished work never migrate implicitly to another center.
	for (auto *pod : SpacePod::Iterate()) if (pod->state.home == center) { pod->state.home = {}; pod->state.custody = SpacePodCustody::Unhomed; pod->state.paused = true; }
	if (auto *s = ResolveLaunchCenter(center); s != nullptr && s->launch_operations) s->launch_operations->pod_workshop = {};
}
/** @copydoc ChangeSpacePodOwnership */
void ChangeSpacePodOwnership(Owner old_owner, Owner new_owner)
{
	// Takeovers transfer the actual existing asset; company deletion retains ownerless paid history and blocks its real descent rather than recreating inventory.
	for (auto *pod : SpacePod::Iterate()) if (pod->state.owner == old_owner) {
		pod->state.owner = new_owner == INVALID_OWNER ? OWNER_NONE : new_owner;
		if (new_owner == INVALID_OWNER) {
			// Company removal clears only the exact original shell workshop slot; unrelated paid work remains owned by its own asset.
			if (auto *center = ResolveLaunchCenter(pod->state.home); center != nullptr && center->launch_operations && center->launch_operations->pod_workshop == GetSpacePodReference(*pod)) center->launch_operations->pod_workshop = {};
			pod->state.home = {}; pod->state.craft = {};
			if (pod->descent) { pod->descent->blocked = SpaceDescentBlock::CompanyRemoved; }
			else { pod->state.paused = true; if (pod->state.custody != SpacePodCustody::Drifting) pod->state.custody = SpacePodCustody::Unhomed; }
		}
	}
}

/** @copydoc CaptureSpacePods */
std::vector<SpacePodRecord> CaptureSpacePods()
{
	std::vector<SpacePodRecord> records;
	for (const auto *pod : SpacePod::Iterate()) records.push_back({GetSpacePodReference(*pod), pod->state});
	return records;
}
/** @param ref Saved original facility incarnation. @return Whether its identity is structurally bounded before any same-file/native lookup. */
static bool ValidCenter(LaunchCenterRef ref) { return ref.station < StationID::End() && ref.generation != 0; }
/** @copydoc ValidateSpacePodRecords */
bool ValidateSpacePodRecords(std::span<const SpacePodRecord> records, uint64_t next, uint32_t width, uint32_t height)
{
	// Bound the complete family before copied sets/geometry are used; invalid issuance or map dimensions cannot authorize shell custody.
	if (records.size() > SpacePodID::End().base() || next == 0 || width < 64 || height < 64 || width > 4096 || height > 4096) return false;
	const auto &d = *GetSpaceConstructionDefinition(SpaceBuildKind::Pod);
	std::set<SpacePodID> ids; std::set<uint64_t> generations;
	// Every original shell must retain one unique issued identity, actual paid recipe and a coherent physical/work domain.
	for (const auto &r : records) {
		const auto &s = r.state;
		// Unsupported provenance, unpaid capital, future/reused identity or impossible coordinates/work cannot become an asset after load.
		if (r.ref.pod >= SpacePodID::End() || r.ref.generation == 0 || r.ref.generation >= next || !ids.insert(r.ref.pod).second || !generations.insert(r.ref.generation).second ||
			s.custody >= SpacePodCustody::End || !IsSupportedSpaceConstructionVersion(s.content_version) || s.capital_paid != d.capital || !ValidCenter(s.origin) ||
			s.surface_tile.base() >= uint64_t{width} * height || s.remaining_ticks > d.work_ticks ||
			(s.owner >= MAX_COMPANIES && s.owner != OWNER_NONE)) return false;
		// Real Surface storage/work custody retains its exact center and excludes simultaneous carrier ownership.
		if (s.custody == SpacePodCustody::Assembling || s.custody == SpacePodCustody::CenterStored) {
			// Surface custody requires one real center and exactly the unfinished/finished work state appropriate to its original slot.
			if (!ValidCenter(s.home) || s.craft != SpaceCraftRef{} || s.owner == OWNER_NONE ||
				(s.custody == SpacePodCustody::Assembling ? s.remaining_ticks == 0 : s.remaining_ticks != 0 || s.paused)) return false;
		} else if (s.custody == SpacePodCustody::Craft) {
			// Carried custody cannot retain a second Surface workshop/home or unfinished construction claim.
			if (s.home != LaunchCenterRef{} || s.craft.vehicle >= VehicleID::End() || s.craft.generation == 0 || s.remaining_ticks != 0 || s.paused || s.owner == OWNER_NONE) return false;
		} else if (s.custody == SpacePodCustody::Descent) {
			// A released shell's original descent is its sole owner; no carrier, center-storage slot or construction countdown can overlap it.
			if (s.home != LaunchCenterRef{} || s.craft != SpaceCraftRef{} || s.remaining_ticks != 0 || s.paused) return false;
		} else if (s.home != LaunchCenterRef{} || s.craft != SpaceCraftRef{} || !s.paused || (s.custody == SpacePodCustody::Drifting && s.remaining_ticks != 0)) return false;
		// Every custody domain retains a bounded real Space projection; invalid coordinates cannot pass as an available shell location.
		if (s.position.x < 0 || s.position.y < 0 || int64_t{s.position.x} >= int64_t{width} * SPACE_Q16 || int64_t{s.position.y} >= int64_t{height} * SPACE_Q16) return false;
	}
	return true;
}
/** @copydoc ValidateSpacePodLinks */
bool ValidateSpacePodLinks(std::span<const SpacePodRecord> pods, std::span<const SpaceProductionRecord> craft,
	std::span<const LaunchOperationsRecord> operations, std::span<const SpaceManifestRecord> manifests)
{
	std::set<SpacePodID> assigned;
	/** @param ref Exact copied shell incarnation. @return Its same-file original state, never a live/open-world pool lookup. */
	auto find = [&](SpacePodRef ref) -> const SpacePodState * { for (const auto &pod : pods) if (pod.ref == ref) return &pod.state; return nullptr; };
	// Each actual Surface facility accounts both completed storage and its sole unfinished passive work slot against one capacity.
	for (const auto &op : operations) {
		uint count = 0; for (const auto &pod : pods) if (pod.state.home == op.center) ++count;
		if (count > GetMultiLayerSpaceBudget().launch_pod_cap) return false;
		if (op.slots.pod_workshop != SpacePodRef{}) {
			const auto *s = find(op.slots.pod_workshop);
			// The sole workshop slot must reference its actual unique unfinished shell at that exact center incarnation.
			if (s == nullptr || s->home != op.center || s->custody != SpacePodCustody::Assembling || !assigned.insert(op.slots.pod_workshop.pod).second) return false;
		}
	}
	// Each carried shell resolves through this same snapshot's actual paid hull and manifest, preserving one slot and one physical location per original asset.
	for (const auto &m : manifests) {
		const SpaceConstructionState *s = nullptr; for (const auto &r : craft) if (r.craft == m.craft) s = &r.state;
		// Only a real paid carrier with enough physical shell slots can own the complete copied manifest.
		if (s == nullptr || m.pods.size() > (s->model == 14 ? GetMultiLayerSpaceBudget().launch_pod_cap : GetMultiLayerSpaceBudget().starting_pods)) return false;
		for (const auto ref : m.pods) {
			const auto *pod = find(ref);
			// A copied carrier must retain actual Surface custody or genuine orbital-factory provenance; an invalid carrier tile cannot manufacture a replacement Earth location.
			if (pod == nullptr || pod->custody != SpacePodCustody::Craft || pod->craft != m.craft || !IsSpacePodCarrierSurfaceTileValid(*pod, *s) || !assigned.insert(ref.pod).second) return false;
		}
	}
	// Close the reverse custody join: no assembling/carried shell may be unreferenced, and a stored home must be the same actual Surface operations owner.
	for (const auto &r : pods) {
		const auto &s = r.state;
		// Every active construction/carrier shell must have its unique forward owner; detached references cannot create hidden stock.
		if (s.custody == SpacePodCustody::Assembling || s.custody == SpacePodCustody::Craft) { if (!assigned.contains(r.ref.pod)) return false; }
		// A stored shell keeps a same-file actual operations owner; an absent numeric center cannot act as an implicit fallback home.
		/** @param op Same-file actual Surface operations owner. @return Whether its exact center incarnation supplies this shell's retained home. */
		const auto home = [&](const auto &op) { return op.center == s.home; };
		if (s.home != LaunchCenterRef{} && std::none_of(operations.begin(), operations.end(), home)) return false;
	}
	return true;
}
/** @copydoc ValidateSpacePods */
bool ValidateSpacePods()
{
	const auto records = CaptureSpacePods();
	// Both paid individual receipts and complete exclusive custody joins must pass before checking live native geometry.
	if (!ValidateSpacePodRecords(records, next_generation, Map::SizeX(), Map::SizeY()) ||
		!ValidateSpacePodLinks(records, CaptureSpaceProduction(), CaptureLaunchOperations(), CaptureSpaceManifests())) return false;
	// Actual native ownership and physical projection supplement the detached graph; old numeric facility/carrier IDs never legitimize another incarnation.
	for (const auto &r : records) {
		const auto &s = r.state;
		// Paid origin issuance and surviving company identity must be real; future centers or nonexistent owners cannot legitimate custody.
		if (s.origin.generation >= GetNextLaunchCenterGeneration() || (s.owner != OWNER_NONE && !Company::IsValidID(s.owner))) return false;
		if (s.home != LaunchCenterRef{}) {
			// Lost or foreign original facilities retire that binding without recreating paid shells or moving work to another center.
			const auto *center = ResolveLaunchCenter(s.home); if (center == nullptr || center->owner != s.owner) return false;
			const auto zone = s.custody == SpacePodCustody::Assembling ? LaunchFacilityZone::Workshop : LaunchFacilityZone::Storage;
			// Surface custody must occupy the correct actual storage/workshop zone and that center's original Space projection.
			if (s.surface_tile != GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, zone)->tile || s.position != *GetLaunchCenterPlaneReference(center->launch_center->tile)) return false;
		}
		if (s.custody == SpacePodCustody::Craft) {
			const auto *v = ResolveSpaceCraft(s.craft);
			// The shell follows the exact native owner/pose; a real orbital factory product retains the shell's bounded original Surface history rather than an invented home tile.
			if (v == nullptr || v->owner != s.owner || s.position != v->position || !IsSpacePodCarrierSurfaceTileValid(s, v->construction)) return false;
		}
	}
	return true;
}
/** @copydoc InstallLoadedSpacePods */
bool InstallLoadedSpacePods(std::span<const SpacePodRecord> records, uint64_t next)
{
	// Reconstruction installs only a fully bounded whole original family into an empty pool; it cannot replace live custody or execute a producer.
	if (SpacePod::GetNumItems() != 0 || !ValidateSpacePodRecords(records, next, Map::SizeX(), Map::SizeY()) || !SpacePod::CanAllocateItem(records.size())) return false;
	for (const auto &r : records) { auto *pod = SpacePod::CreateAtIndex(r.ref.pod); pod->generation = r.ref.generation; pod->state = r.state; }
	next_generation = next; return true; // Cross-family/physical validation runs after the native center/manifest owners are installed.
}
/** @copydoc ComputeSpacePodDigest */
uint64_t ComputeSpacePodDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Original bounded scalar; fixed byte order makes custody/history comparisons independent of host layout. */
	auto add = [&](uint64_t value) { for (uint i = 0; i < 8; ++i) { hash ^= (value >> (i * 8)) & 255; hash *= 1099511628211ULL; } };
	/** @param ref Actual facility incarnation; generation prevents an identical numeric station ID hiding different custody. */
	auto center = [&](LaunchCenterRef ref) { add(ref.station.base()); add(ref.generation); };
	add(next_generation);
	// Include every actual asset's complete original identity, paid work and physical/custody history in stable native order.
	for (const auto &r : CaptureSpacePods()) {
		add(r.ref.pod.base()); add(r.ref.generation); const auto &s = r.state; add(s.owner.base()); add(to_underlying(s.custody)); center(s.home); center(s.origin);
		add(s.craft.vehicle.base()); add(s.craft.generation); add(s.surface_tile.base()); add(static_cast<uint32_t>(s.position.x)); add(static_cast<uint32_t>(s.position.y));
		add(s.remaining_ticks); add(s.content_version); add(s.capital_paid); add(s.paused);
	}
	return hash;
}
