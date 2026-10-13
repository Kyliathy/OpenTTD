/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_descent.cpp One paid pod owns each saved cargo descent and recovery reservation. */
#include "stdafx.h"
#include "launch_center.h"
#include "launch_center_cmd.h"
#include "space_pod.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_transition.h"
#include "multilayer_space_content.h"
#include "hub_cargo.h"
#include "station_base.h"
#include "company_base.h"
#include "company_func.h"
#include "command_func.h"
#include "window_func.h"
#include <set>
#include <map>
#include "safeguards.h"

static uint64_t next_descent_id = 1;
uint64_t GetNextSpaceDescentID() { return next_descent_id; }
void ResetSpaceDescents() { next_descent_id = 1; ResetSpaceDescentLoadStaging(); }
static bool ReturnCargo(CargoType cargo)
{
	if (!IsValidCargoType(cargo)) return false;
	for (const auto &capability : GetMultiLayerVehicleCapabilities()) if (capability.model_id == 18) return
		std::ranges::find(capability.cargo_labels, CargoSpec::Get(cargo)->label) != capability.cargo_labels.end();
	return false;
}
static uint RecoveryJobs(LaunchCenterRef ref)
{
	uint count = 0; for (const auto *pod : SpacePod::Iterate()) if (pod->descent && pod->descent->center == ref) ++count; return count;
}
uint GetIncomingSpaceDescent(LaunchCenterRef center, CargoType cargo)
{
	uint count = 0; for (const auto *pod : SpacePod::Iterate()) if (pod->descent && pod->descent->center == center && pod->descent->cargo == cargo) count += pod->descent->quantity;
	return count;
}
bool HasSpaceDescentForCenter(LaunchCenterRef center) { return center.generation != 0 && RecoveryJobs(center) != 0; }
bool CanReleaseOrbitalCargo(SpaceCraftRef craft_ref, LaunchCenterRef center_ref, SpacePodRef pod_ref, uint16_t quantity)
{
	const auto *v = ResolveSpaceCraft(craft_ref); const auto *center = ResolveLaunchCenter(center_ref); const auto *pod = ResolveSpacePod(pod_ref);
	if (v == nullptr || center == nullptr || pod == nullptr || !Company::IsValidID(v->owner) || center->owner != v->owner || pod->state.owner != v->owner ||
		v->construction.placement != SpacePlacement::Orbital || (GetSpaceVehicleModel(*v) != 15 && GetSpaceVehicleModel(*v) != 20) || !v->manifest ||
		v->manifest->service_ticks != 0 || v->manifest->returns_remaining <= v->manifest->pending_returns || pod->descent ||
		pod->state.custody != SpacePodCustody::Craft || pod->state.craft != craft_ref || std::count(v->manifest->pods.begin(), v->manifest->pods.end(), pod_ref) != 1 ||
		!ReturnCargo(v->cargo_type) || quantity == 0 || quantity > 40 || CargoSpec::Get(v->cargo_type)->weight == 0 || uint64_t{quantity} * CargoSpec::Get(v->cargo_type)->weight > GetMultiLayerSpaceBudget().probe_hold_sixteenths ||
		next_descent_id == UINT64_MAX || RecoveryJobs(center_ref) >= GetMultiLayerSpaceBudget().launch_pod_cap || quantity > GetLaunchStockHeadroom(*center, v->cargo_type) ||
		!IsInsideLaunchCenterDropZone(v->position, *GetLaunchCenterPlaneReference(center->launch_center->tile))) return false;
	return HubCargoTransfer::CanMove(v->cargo, quantity);
}
bool CommitOrbitalCargoRelease(SpaceCraftRef craft_ref, LaunchCenterRef center_ref, SpacePodRef pod_ref, uint16_t quantity)
{
	if (!CanReleaseOrbitalCargo(craft_ref, center_ref, pod_ref, quantity)) return false;
	auto *v = ResolveSpaceCraft(craft_ref); auto *pod = ResolveSpacePod(pod_ref); auto job = std::make_unique<SpaceDescentJob>();
	job->id = next_descent_id; job->center = center_ref; job->release_point = v->position; job->sender = v->index; job->sender_generation = v->generation;
	job->cargo = v->cargo_type; job->quantity = quantity; job->remaining_ticks = GetMultiLayerSpaceBudget().descent_ticks;
	const bool moved = HubCargoTransfer::ReleaseOrbital(v->cargo, job->packets, quantity, v->tile); assert(moved); if (!moved) return false;
	++next_descent_id; ++v->manifest->pending_returns; std::erase(v->manifest->pods, pod_ref);
	pod->state.custody = SpacePodCustody::Descent; pod->state.craft = {}; pod->state.home = {}; pod->state.position = job->release_point;
	pod->descent = std::move(job); v->MarkDirty(); SetWindowClassesDirty(WindowClass::LaunchAssembly); return true;
}
CommandCost CmdReleaseOrbitalCargo(DoCommandFlags flags, VehicleID vehicle, uint64_t generation, StationID station, uint64_t center_generation, SpacePodID id, uint64_t pod_generation, uint16_t quantity)
{
	auto *v = ResolveSpaceCraft({vehicle, generation});
	if (v == nullptr || v->owner != _current_company || !CanReleaseOrbitalCargo({vehicle, generation}, {station, center_generation}, {id, pod_generation}, quantity)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) { const bool released = CommitOrbitalCargoRelease({vehicle, generation}, {station, center_generation}, {id, pod_generation}, quantity); assert(released); if (!released) return CMD_ERROR; }
	return CommandCost{}; // Materials/capital paid for this shell already; no freight income for crossing layers.
}
bool CanRetargetSpaceDescent(SpacePodRef ref, LaunchCenterRef center_ref)
{
	const auto *pod = ResolveSpacePod(ref); const auto *center = ResolveLaunchCenter(center_ref);
	if (pod == nullptr || center == nullptr || !pod->descent || !Company::IsValidID(pod->state.owner) || center->owner != pod->state.owner) return false;
	const auto &job = *pod->descent;
	if (!IsInsideLaunchCenterDropZone(job.release_point, *GetLaunchCenterPlaneReference(center->launch_center->tile))) return false;
	const auto jobs = RecoveryJobs(center_ref); const auto own = job.center == center_ref ? 1U : 0U;
	if (jobs - own >= GetMultiLayerSpaceBudget().launch_pod_cap) return false;
	const uint headroom = GetLaunchStockHeadroom(*center, job.cargo);
	if (own == 0) return job.quantity <= headroom;
	const uint64_t occupied = uint64_t{center->goods[job.cargo].TotalCount()} + GetIncomingSpaceDescent(center_ref, job.cargo);
	return occupied <= GetLaunchStockCapacity(*center, job.cargo); // Existing reservation remains attached to this exact pod.
}
CommandCost CmdRetargetSpaceDescent(DoCommandFlags flags, SpacePodID id, uint64_t generation, StationID station, uint64_t center_generation)
{
	auto *pod = ResolveSpacePod({id, generation});
	if (pod == nullptr || pod->state.owner != _current_company || !CanRetargetSpaceDescent({id, generation}, {station, center_generation})) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) { pod->descent->center = {station, center_generation}; pod->descent->blocked = SpaceDescentBlock::None; SetWindowClassesDirty(WindowClass::LaunchAssembly); }
	return CommandCost{};
}
void ForgetSpaceDescentCenter(LaunchCenterRef center)
{
	for (auto *pod : SpacePod::Iterate()) if (pod->descent && pod->descent->center == center) pod->descent->blocked = SpaceDescentBlock::TargetLost;
}
void TickSpaceDescents()
{
	for (auto *pod : SpacePod::Iterate()) if (pod->descent) {
		auto &job = *pod->descent;
		if (++job.age_ticks == Ticks::CARGO_AGING_TICKS) { job.age_ticks = 0; HubCargoTransfer::Age(job.packets); }
		if (!Company::IsValidID(pod->state.owner)) { job.blocked = SpaceDescentBlock::CompanyRemoved; continue; }
		auto *center = ResolveLaunchCenter(job.center);
		if (center == nullptr || center->owner != pod->state.owner) { job.blocked = SpaceDescentBlock::TargetLost; continue; }
		if (job.blocked == SpaceDescentBlock::TargetLost || job.blocked == SpaceDescentBlock::CompanyRemoved) continue; // Explicit generation-checked retargeting resumes retained work.
		if (job.remaining_ticks != 0) --job.remaining_ticks;
		if (job.remaining_ticks != 0) continue;
		const uint cap = GetLaunchStockCapacity(*center, job.cargo), occupied = center->goods[job.cargo].TotalCount();
		if (occupied > cap || job.quantity > cap - occupied) { job.blocked = SpaceDescentBlock::StorageFull; continue; }
		const bool moved = HubCargoTransfer::Move(job.packets, center->goods[job.cargo].GetOrCreateData().cargo, job.quantity); assert(moved); if (!moved) continue;
		if (auto *sender = ResolveSpaceCraft({job.sender, job.sender_generation}); sender != nullptr && sender->manifest) {
			assert(sender->manifest->pending_returns > 0 && sender->manifest->returns_remaining > 0); --sender->manifest->pending_returns; --sender->manifest->returns_remaining;
		}
		SetWindowClassesDirty(WindowClass::LaunchAssembly); SetWindowDirty(WindowClass::StationView, center->index); delete pod; // This exact paid shell is expended once.
	}
}
std::vector<SpaceDescentRecord> CaptureSpaceDescents()
{
	std::vector<SpaceDescentRecord> result;
	for (const auto *pod : SpacePod::Iterate()) if (pod->descent) {
		const auto &j = *pod->descent; SpaceDescentRecord r;
		r.pod = GetSpacePodReference(*pod); r.id = j.id; r.center = j.center; r.release_point = j.release_point; r.sender = j.sender; r.sender_generation = j.sender_generation;
		r.cargo = j.cargo; r.quantity = j.quantity; r.remaining_ticks = j.remaining_ticks; r.age_ticks = j.age_ticks; r.blocked = j.blocked;
		for (const auto &[hop, packets] : *j.packets.Packets()) for (const auto *packet : packets) r.packets.push_back({packet->index, hop});
		result.push_back(std::move(r));
	}
	return result;
}
bool ValidateSpaceDescentRecords(std::span<const SpaceDescentRecord> records, uint64_t next, uint32_t width, uint32_t height)
{
	if (next == 0 || records.size() > SpacePodID::End().base() || width < 64 || height < 64 || width > 4096 || height > 4096) return false;
	std::set<SpacePodID> pods; std::set<uint64_t> jobs; std::set<CargoPacketID> packets;
	for (const auto &r : records) {
		if (r.pod.pod >= SpacePodID::End() || r.pod.generation == 0 || !pods.insert(r.pod.pod).second || r.id == 0 || r.id >= next || !jobs.insert(r.id).second ||
			r.center.station >= StationID::End() || r.center.generation == 0 || r.sender >= VehicleID::End() || r.sender_generation == 0 || r.cargo >= NUM_CARGO ||
			r.quantity == 0 || r.quantity > 40 || r.remaining_ticks > GetMultiLayerSpaceBudget().descent_ticks || r.age_ticks >= Ticks::CARGO_AGING_TICKS || r.blocked >= SpaceDescentBlock::End ||
			(r.blocked == SpaceDescentBlock::StorageFull && r.remaining_ticks != 0) ||
			r.packets.empty() || r.packets.size() > r.quantity || r.release_point.x < 0 || r.release_point.y < 0 ||
			r.release_point.x > int64_t{width - 1} * SPACE_Q16 || r.release_point.y > int64_t{height - 1} * SPACE_Q16) return false;
		for (const auto &[packet, hop] : r.packets) if (packet >= CargoPacketID::End() || !packets.insert(packet).second || (hop != StationID::Invalid() && hop >= StationID::End())) return false;
	}
	return true;
}
bool ValidateSpaceDescentLinks(std::span<const SpaceDescentRecord> records, std::span<const SpacePodRecord> pods,
	std::span<const SpaceManifestRecord> manifests, std::span<const LaunchCenterRecord> centers, uint32_t width)
{
	if (width < 64 || width > 4096) return false;
	std::set<SpacePodID> assigned; std::set<CargoPacketID> packets; std::map<std::pair<VehicleID, uint64_t>, uint> pending; std::map<StationID, uint> per_center;
	for (const auto &m : manifests) for (const auto &slot : m.supplies) for (const auto &[packet, hop] : slot) { (void)hop; if (!packets.insert(packet).second) return false; }
	for (const auto &r : records) {
		for (const auto &[packet, hop] : r.packets) { (void)hop; if (!packets.insert(packet).second) return false; }
		const auto pod = std::find_if(pods.begin(), pods.end(), [&](const auto &p) { return p.ref == r.pod; });
		if (pod == pods.end() || pod->state.custody != SpacePodCustody::Descent || pod->state.position != r.release_point || !assigned.insert(r.pod.pod).second) return false;
		const auto center = std::find_if(centers.begin(), centers.end(), [&](const auto &c) { return c.station == r.center.station && c.anchor.generation == r.center.generation; });
		if (center == centers.end()) { if (r.blocked != SpaceDescentBlock::TargetLost && r.blocked != SpaceDescentBlock::CompanyRemoved) return false; }
		else {
			const auto tile = center->anchor.tile.base(); const SpacePoint anchor{static_cast<int32_t>((tile % width) * SPACE_Q16), static_cast<int32_t>((tile / width) * SPACE_Q16)};
			if (!IsInsideLaunchCenterDropZone(r.release_point, anchor) || ++per_center[center->station] > GetMultiLayerSpaceBudget().launch_pod_cap) return false;
		}
		const auto manifest = std::find_if(manifests.begin(), manifests.end(), [&](const auto &m) { return m.craft == SpaceCraftRef{r.sender, r.sender_generation}; });
		if (manifest != manifests.end()) ++pending[{r.sender, r.sender_generation}];
	}
	for (const auto &p : pods) if ((p.state.custody == SpacePodCustody::Descent) != assigned.contains(p.ref.pod)) return false;
	for (const auto &m : manifests) if (m.pending_returns != pending[{m.craft.vehicle, m.craft.generation}]) return false;
	return true;
}
static bool PhysicalDescents(std::span<const SpaceDescentRecord> records)
{
	std::set<CargoPacketID> owned;
	auto add = [&](const auto &packets) { for (const auto *cp : packets) if (!owned.insert(cp->index).second) return false; return true; };
	for (const auto *station : Station::Iterate()) {
		for (const auto &goods : station->goods) if (goods.HasData()) for (const auto &[hop, packets] : *goods.GetData().cargo.Packets()) { (void)hop; if (!add(packets)) return false; }
		if (station->launch_hub) for (const auto &job : station->launch_hub->handling) for (const auto &[hop, packets] : *job->packets.Packets()) { (void)hop; if (!add(packets)) return false; }
	}
	for (const auto *v : Vehicle::Iterate()) if (!add(*v->cargo.Packets())) return false;
	for (const auto *v : SpaceVehicle::Iterate()) if (v->manifest) for (const auto &slot : v->manifest->supplies) for (const auto &[hop, packets] : *slot.Packets()) { (void)hop; if (!add(packets)) return false; }
	std::map<std::pair<VehicleID, uint64_t>, uint> pending;
	std::map<StationID, uint> jobs;
	for (const auto &r : records) {
		const auto *pod = ResolveSpacePod(r.pod);
		if (pod == nullptr || pod->state.custody != SpacePodCustody::Descent || pod->state.position != r.release_point || !ReturnCargo(r.cargo) ||
			uint64_t{r.quantity} * CargoSpec::Get(r.cargo)->weight > GetMultiLayerSpaceBudget().probe_hold_sixteenths || r.center.generation >= GetNextLaunchCenterGeneration() || r.sender_generation >= GetNextSpaceVehicleGeneration()) return false;
		uint count = 0; for (const auto &[id, hop] : r.packets) { (void)hop; const auto *cp = CargoPacket::GetIfValid(id); if (cp == nullptr || !owned.insert(id).second) return false; count += cp->Count(); }
		if (count != r.quantity) return false;
		const auto *center = ResolveLaunchCenter(r.center);
		if (center != nullptr && center->owner == pod->state.owner) {
			if (!IsInsideLaunchCenterDropZone(r.release_point, *GetLaunchCenterPlaneReference(center->launch_center->tile)) || ++jobs[center->index] > GetMultiLayerSpaceBudget().launch_pod_cap) return false;
		} else if (r.blocked != SpaceDescentBlock::TargetLost && r.blocked != SpaceDescentBlock::CompanyRemoved) return false;
		if (auto *sender = ResolveSpaceCraft({r.sender, r.sender_generation})) { if (sender->owner != pod->state.owner || !sender->manifest) return false; ++pending[{sender->index, sender->generation}]; }
	}
	for (const auto *v : SpaceVehicle::Iterate()) if (v->manifest && v->manifest->pending_returns != pending[{v->index, v->generation}]) return false;
	return true;
}
bool ValidateSpaceDescents()
{
	const auto records = CaptureSpaceDescents();
	if (!ValidateSpaceDescentRecords(records, next_descent_id, Map::SizeX(), Map::SizeY()) ||
		!ValidateSpaceDescentLinks(records, CaptureSpacePods(), CaptureSpaceManifests(), CaptureLaunchCenters(), Map::SizeX()) || !PhysicalDescents(records)) return false;
	for (const auto *pod : SpacePod::Iterate()) if ((pod->state.custody == SpacePodCustody::Descent) != (pod->descent != nullptr)) return false;
	return true;
}
bool InstallLoadedSpaceDescents(std::span<const SpaceDescentRecord> records, uint64_t next)
{
	if (!ValidateSpaceDescentRecords(records, next, Map::SizeX(), Map::SizeY()) || !PhysicalDescents(records)) return false;
	for (const auto *pod : SpacePod::Iterate()) if (pod->descent) return false;
	for (const auto &r : records) {
		auto *pod = ResolveSpacePod(r.pod); auto job = std::make_unique<SpaceDescentJob>();
		job->id = r.id; job->center = r.center; job->release_point = r.release_point; job->sender = r.sender; job->sender_generation = r.sender_generation;
		job->cargo = r.cargo; job->quantity = r.quantity; job->remaining_ticks = r.remaining_ticks; job->age_ticks = r.age_ticks; job->blocked = r.blocked;
		for (const auto &[id, hop] : r.packets) job->packets.Append(CargoPacket::Get(id), hop);
		pod->descent = std::move(job);
	}
	next_descent_id = next;
	const auto installed = CaptureSpaceDescents();
	for (const auto &r : records) {
		const auto found = std::find_if(installed.begin(), installed.end(), [&](const auto &i) { return i.pod == r.pod; });
		if (found == installed.end() || found->packets != r.packets) return false;
	}
	return true;
}
uint64_t ComputeSpaceDescentDigest()
{
	uint64_t hash = 14695981039346656037ULL; auto add = [&](uint64_t value) { for (uint i = 0; i < 8; ++i) { hash ^= (value >> (i * 8)) & 255; hash *= 1099511628211ULL; } }; add(next_descent_id);
	for (const auto &r : CaptureSpaceDescents()) {
		add(r.pod.pod.base()); add(r.pod.generation); add(r.id); add(r.center.station.base()); add(r.center.generation); add(static_cast<uint32_t>(r.release_point.x)); add(static_cast<uint32_t>(r.release_point.y));
		add(r.sender.base()); add(r.sender_generation); add(r.cargo); add(r.quantity); add(r.remaining_ticks); add(r.age_ticks); add(to_underlying(r.blocked));
		for (const auto &[id, hop] : r.packets) { const auto *cp = CargoPacket::Get(id); add(id.base()); add(hop.base()); add(cp->Count()); add(cp->GetPeriodsInTransit()); add(static_cast<int64_t>(cp->GetFeederShare())); add(cp->GetSourceTile().base()); add(cp->GetFirstStation().base()); }
	}
	return hash;
}
