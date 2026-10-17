/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_descent.cpp One paid pod owns each saved cargo descent and recovery reservation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups12-14.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1/1B; exclusive Core stock and service berth.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; saved sender/mass receipts and original paid heavy returns.
 */
#include "stdafx.h"
#include "launch_center.h"
#include "launch_center_cmd.h"
#include "space_pod.h"
#include "space_vehicle.h"
#include "space_structure.h"
#include "orbital_service.h"
#include "space_manifest.h"
#include "space_mining_tuning.h"
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
/** @copydoc GetNextSpaceDescentID */
uint64_t GetNextSpaceDescentID() { return next_descent_id; }
/** @copydoc ResetSpaceDescents */
void ResetSpaceDescents() { next_descent_id = 1; ResetSpaceDescentLoadStaging(); }
/** @copydoc GetSpaceDescentMassLimit */
uint16_t GetSpaceDescentMassLimit(uint16_t model)
{
	// Only the two explicitly delivered mining roles receive their shared hold allowance here; a later tuning-table entry cannot silently acquire a Surface descent producer.
	if (model == 15 || model == SPACE_HEAVY_MINER_MODEL) {
		const auto *miner = GetSpaceMiningTuning(model); return miner == nullptr ? 0 : miner->hold_sixteenths;
	}
	// A construction-probe or retained unknown old receipt keeps the original small mass bound; an unrelated role cannot borrow descent rights.
	return model == 20 || model == SPACE_DESCENT_LEGACY_SENDER_MODEL ? GetMultiLayerSpaceBudget().probe_hold_sixteenths : 0;
}
/** @copydoc IsSpaceDescentReceiptValid */
bool IsSpaceDescentReceiptValid(uint16_t model, uint16_t quantity, uint8_t unit_weight, uint16_t freight_mass)
{
	// The original fixed40-unit bound remains independent of cargo dimensions, keeping packet/work framing finite for every supported role.
	if (quantity == 0 || quantity > 40) return false;
	if (model == SPACE_DESCENT_LEGACY_SENDER_MODEL) return unit_weight == 0 && freight_mass == 0;
	const auto limit = GetSpaceDescentMassLimit(model);
	// Only an actual supported role and complete positive physical receipt can authorize a new return; redundant mass arithmetic remains checkable after sender loss and in an independent copied preview.
	return limit != 0 && unit_weight != 0 && freight_mass != 0 && uint64_t{quantity} * unit_weight == freight_mass && freight_mass <= limit;
}
/** @param cargo Actual native runtime cargo. @return Whether the shared paid return-shell capability accepts this label, without granting a carrier or stock. */
static bool ReturnCargo(CargoType cargo)
{
	if (!IsValidCargoType(cargo)) return false;
	// The canonical passive return payload owns label admission; adding a heavy sender never adds undeclared return cargo meanings.
	for (const auto &capability : GetMultiLayerVehicleCapabilities()) if (capability.model_id == 18) return
		std::ranges::find(capability.cargo_labels, CargoSpec::Get(cargo)->label) != capability.cargo_labels.end();
	return false;
}
/** @param ref Exact original recovery facility. @return Actual outstanding pod jobs counted against its existing bounded recovery capacity. */
static uint RecoveryJobs(LaunchCenterRef ref)
{
	// Count actual outstanding original jobs, including blocked ones, so another release cannot overbook the facility's saved recovery capacity.
	uint count = 0; for (const auto *pod : SpacePod::Iterate()) if (pod->descent && pod->descent->center == ref) ++count; return count;
}
/** @copydoc GetIncomingSpaceDescent */
uint GetIncomingSpaceDescent(LaunchCenterRef center, CargoType cargo)
{
	// Every retained physical packet reservation contributes to this exact center/label headroom even while its descent is blocked.
	uint count = 0; for (const auto *pod : SpacePod::Iterate()) if (pod->descent && pod->descent->center == center && pod->descent->cargo == cargo) count += pod->descent->quantity;
	return count;
}
/** @copydoc HasSpaceDescentForCenter */
bool HasSpaceDescentForCenter(LaunchCenterRef center) { return center.generation != 0 && RecoveryJobs(center) != 0; }
/** Admit the complete existing descent owner only when no Core retains this hull's paid service berth.
 * @param craft_ref Exact native sender incarnation. @param center_ref Explicit receiving center incarnation.
 * @param pod_ref Actual paid recoverable shell. @param quantity Positive physical cargo units.
 * @return Whether geometry, ownership, packet movement, receiver capacity and exclusive service state all permit release.
 */
bool CanReleaseOrbitalCargo(SpaceCraftRef craft_ref, LaunchCenterRef center_ref, SpacePodRef pod_ref, uint16_t quantity)
{
	const auto *v = ResolveSpaceCraft(craft_ref); const auto *center = ResolveLaunchCenter(center_ref); const auto *pod = ResolveSpacePod(pod_ref);
	// The Core's single service berth cannot simultaneously authorize a cargo return that changes this manifest's allowance.
	if (v != nullptr && HasOrbitalServiceAssignment(*v)) return false;
	const auto model = v == nullptr ? uint16_t{0} : GetSpaceVehicleModel(*v); const auto mass_limit = GetSpaceDescentMassLimit(model);
	// Complete actual custody, paid allowance, physical freight dimensions and destination capacity must admit together; a refusal retains the whole offered hold and shell on the original hull.
	if (v == nullptr || center == nullptr || pod == nullptr || !Company::IsValidID(v->owner) || center->owner != v->owner || pod->state.owner != v->owner ||
		v->construction.placement != SpacePlacement::Orbital || (model != 15 && model != 20 && model != SPACE_HEAVY_MINER_MODEL) || !v->manifest ||
		v->manifest->service_ticks != 0 || v->manifest->returns_remaining <= v->manifest->pending_returns || pod->descent ||
		pod->state.custody != SpacePodCustody::Craft || pod->state.craft != craft_ref || std::count(v->manifest->pods.begin(), v->manifest->pods.end(), pod_ref) != 1 ||
		!ReturnCargo(v->cargo_type) || quantity == 0 || quantity > 40 || CargoSpec::Get(v->cargo_type)->weight == 0 || uint64_t{quantity} * CargoSpec::Get(v->cargo_type)->weight > mass_limit ||
		next_descent_id == UINT64_MAX || RecoveryJobs(center_ref) >= GetMultiLayerSpaceBudget().launch_pod_cap || quantity > GetLaunchStockHeadroom(*center, v->cargo_type) ||
		!IsInsideLaunchCenterDropZone(v->position, *GetLaunchCenterPlaneReference(center->launch_center->tile))) return false;
	// The appended miner retains its immutable mineral refits even through a manual release; broad old passive-shell cargo admission grants it no construction/general freight role.
	if (model == SPACE_HEAVY_MINER_MODEL && quantity > GetSpaceFreightCapacity(model, v->cargo_type)) return false;
	return HubCargoTransfer::CanMove(v->cargo, quantity);
}
/** @copydoc CommitOrbitalCargoRelease */
bool CommitOrbitalCargoRelease(SpaceCraftRef craft_ref, LaunchCenterRef center_ref, SpacePodRef pod_ref, uint16_t quantity)
{
	if (!CanReleaseOrbitalCargo(craft_ref, center_ref, pod_ref, quantity)) return false;
	auto *v = ResolveSpaceCraft(craft_ref); auto *pod = ResolveSpacePod(pod_ref); auto job = std::make_unique<SpaceDescentJob>();
	job->id = next_descent_id; job->center = center_ref; job->release_point = v->position; job->sender = v->index; job->sender_generation = v->generation;
	job->sender_model = GetSpaceVehicleModel(*v); job->unit_weight = CargoSpec::Get(v->cargo_type)->weight; job->freight_mass = static_cast<uint16_t>(uint64_t{quantity} * job->unit_weight);
	job->cargo = v->cargo_type; job->quantity = quantity; job->remaining_ticks = GetMultiLayerSpaceBudget().descent_ticks;
	const bool moved = HubCargoTransfer::ReleaseOrbital(v->cargo, job->packets, quantity, v->tile); assert(moved); if (!moved) return false;
	++next_descent_id; ++v->manifest->pending_returns; std::erase(v->manifest->pods, pod_ref);
	pod->state.custody = SpacePodCustody::Descent; pod->state.craft = {}; pod->state.home = {}; pod->state.position = job->release_point;
	pod->descent = std::move(job); v->MarkDirty(); SetWindowClassesDirty(WindowClass::LaunchAssembly); return true;
}
/** Release through the original synchronized paid-pod owner, without payment for crossing layers.
 * @param flags Estimate or execute; mixed QueryCost retains query precedence.
 * @param vehicle Actual sender ID. @param generation Exact sender incarnation. @param station Explicit recovery station.
 * @param center_generation Exact recovery incarnation. @param id Actual paid shell ID. @param pod_generation Exact shell incarnation.
 * @param quantity Positive whole actual freight units. @return Complete atomic admission or unchanged refusal.
 */
CommandCost CmdReleaseOrbitalCargo(DoCommandFlags flags, VehicleID vehicle, uint64_t generation, StationID station, uint64_t center_generation, SpacePodID id, uint64_t pod_generation, uint16_t quantity)
{
	if (flags.Test(DoCommandFlag::QueryCost)) flags.Reset(DoCommandFlag::Execute);
	auto *v = ResolveSpaceCraft({vehicle, generation});
	// Acting ownership and the complete existing physical release gate precede any transfer; malformed/reused references cannot spend another hull's shell or allowance.
	if (v == nullptr || v->owner != _current_company || !CanReleaseOrbitalCargo({vehicle, generation}, {station, center_generation}, {id, pod_generation}, quantity)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) { const bool released = CommitOrbitalCargoRelease({vehicle, generation}, {station, center_generation}, {id, pod_generation}, quantity); assert(released); if (!released) return CMD_ERROR; }
	return CommandCost{}; // Materials/capital paid for this shell already; no freight income for crossing layers.
}
/** @copydoc CanRetargetSpaceDescent */
bool CanRetargetSpaceDescent(SpacePodRef ref, LaunchCenterRef center_ref)
{
	const auto *pod = ResolveSpacePod(ref); const auto *center = ResolveLaunchCenter(center_ref);
	// Retargeting moves only the recovery reservation of an actual same-owner original job; it creates no replacement shell or inventory.
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
/** Retarget the original retained job using its immutable release geometry, without changing cargo or sender rights.
 * @param flags Estimate or execute; mixed QueryCost never retargets.
 * @param id Original shell ID. @param generation Exact shell incarnation. @param station Explicit replacement recovery station.
 * @param center_generation Exact recovery incarnation. @return Owned physical recovery admission or unchanged refusal.
 */
CommandCost CmdRetargetSpaceDescent(DoCommandFlags flags, SpacePodID id, uint64_t generation, StationID station, uint64_t center_generation)
{
	if (flags.Test(DoCommandFlag::QueryCost)) flags.Reset(DoCommandFlag::Execute);
	auto *pod = ResolveSpacePod({id, generation});
	// Only the actual owner can re-admit this original job at an authorized target; failed quotes leave geometry, packets and reservations untouched.
	if (pod == nullptr || pod->state.owner != _current_company || !CanRetargetSpaceDescent({id, generation}, {station, center_generation})) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) { pod->descent->center = {station, center_generation}; pod->descent->blocked = SpaceDescentBlock::None; SetWindowClassesDirty(WindowClass::LaunchAssembly); }
	return CommandCost{};
}
/** @copydoc ForgetSpaceDescentCenter */
void ForgetSpaceDescentCenter(LaunchCenterRef center)
{
	// Retire only the exact lost incarnation; retaining the actual packet owner prevents reused station IDs from receiving old cargo implicitly.
	for (auto *pod : SpacePod::Iterate()) if (pod->descent && pod->descent->center == center) pod->descent->blocked = SpaceDescentBlock::TargetLost;
}
/** @copydoc TickSpaceDescents */
void TickSpaceDescents()
{
	// Each actual pod job ages and earns at most one native countdown opportunity; missing ownership/capacity retains the same physical packets and spent shell history.
	for (auto *pod : SpacePod::Iterate()) if (pod->descent) {
		auto &job = *pod->descent;
		if (++job.age_ticks == Ticks::CARGO_AGING_TICKS) { job.age_ticks = 0; HubCargoTransfer::Age(job.packets); }
		if (!Company::IsValidID(pod->state.owner)) { job.blocked = SpaceDescentBlock::CompanyRemoved; continue; }
		auto *center = ResolveLaunchCenter(job.center);
		// A lost or foreign recovery owner freezes this exact paid job; generation reuse never redirects packets automatically.
		if (center == nullptr || center->owner != pod->state.owner) { job.blocked = SpaceDescentBlock::TargetLost; continue; }
		if (job.blocked == SpaceDescentBlock::TargetLost || job.blocked == SpaceDescentBlock::CompanyRemoved) continue; // Explicit generation-checked retargeting resumes retained work.
		if (job.remaining_ticks != 0) --job.remaining_ticks;
		if (job.remaining_ticks != 0) continue;
		const uint cap = GetLaunchStockCapacity(*center, job.cargo), occupied = center->goods[job.cargo].TotalCount();
		// Recovery is whole-payload: retain the shell and all cargo until the actual Surface inventory can accept every reserved unit.
		if (occupied > cap || job.quantity > cap - occupied) { job.blocked = SpaceDescentBlock::StorageFull; continue; }
		const bool moved = HubCargoTransfer::Move(job.packets, center->goods[job.cargo].GetOrCreateData().cargo, job.quantity); assert(moved); if (!moved) continue;
		// Settle only the original surviving sender's reservation; a lost hull or reused numeric ID cannot spend another incarnation's allowance.
		if (auto *sender = ResolveSpaceCraft({job.sender, job.sender_generation}); sender != nullptr && sender->manifest) {
			assert(sender->manifest->pending_returns > 0 && sender->manifest->returns_remaining > 0); --sender->manifest->pending_returns; --sender->manifest->returns_remaining;
		}
		SetWindowClassesDirty(WindowClass::LaunchAssembly); SetWindowDirty(WindowClass::StationView, center->index); delete pod; // This exact paid shell is expended once.
	}
}
/** @copydoc CaptureSpaceDescents */
std::vector<SpaceDescentRecord> CaptureSpaceDescents()
{
	std::vector<SpaceDescentRecord> result;
	// Copy immutable receipt/work/packet references from each original job without moving or merging any physical cargo.
	for (const auto *pod : SpacePod::Iterate()) if (pod->descent) {
		const auto &j = *pod->descent; SpaceDescentRecord r;
		r.pod = GetSpacePodReference(*pod); r.id = j.id; r.center = j.center; r.release_point = j.release_point; r.sender = j.sender; r.sender_generation = j.sender_generation;
		r.sender_model = j.sender_model; r.unit_weight = j.unit_weight; r.freight_mass = j.freight_mass;
		r.cargo = j.cargo; r.quantity = j.quantity; r.remaining_ticks = j.remaining_ticks; r.age_ticks = j.age_ticks; r.blocked = j.blocked;
		// Preserve original routing and packet identity per compartment; a snapshot cannot merge, split or reassign the cargo owner.
		for (const auto &[hop, packets] : *j.packets.Packets()) for (const auto *packet : packets) r.packets.push_back({packet->index, hop});
		result.push_back(std::move(r));
	}
	return result;
}
/** @copydoc ValidateSpaceDescentRecords */
bool ValidateSpaceDescentRecords(std::span<const SpaceDescentRecord> records, uint64_t next, uint32_t width, uint32_t height)
{
	// Bound the whole family before accepting geometry, issuance or immutable dimension arithmetic from an untrusted source.
	if (next == 0 || records.size() > SpacePodID::End().base() || width < 64 || height < 64 || width > 4096 || height > 4096) return false;
	std::set<SpacePodID> pods; std::set<uint64_t> jobs; std::set<CargoPacketID> packets;
	// Every original shell/job/packet appears once, with finite work and actual in-map release geometry; structural validity grants no live sender or recovery owner.
	for (const auto &r : records) {
		if (!IsSpaceDescentReceiptValid(r.sender_model, r.quantity, r.unit_weight, r.freight_mass)) return false;
		if (r.pod.pod >= SpacePodID::End() || r.pod.generation == 0 || !pods.insert(r.pod.pod).second || r.id == 0 || r.id >= next || !jobs.insert(r.id).second ||
			r.center.station >= StationID::End() || r.center.generation == 0 || r.sender >= VehicleID::End() || r.sender_generation == 0 || r.cargo >= NUM_CARGO ||
			r.quantity == 0 || r.quantity > 40 || r.remaining_ticks > GetMultiLayerSpaceBudget().descent_ticks || r.age_ticks >= Ticks::CARGO_AGING_TICKS || r.blocked >= SpaceDescentBlock::End ||
			(r.blocked == SpaceDescentBlock::StorageFull && r.remaining_ticks != 0) ||
			r.packets.empty() || r.packets.size() > r.quantity || r.release_point.x < 0 || r.release_point.y < 0 ||
			r.release_point.x > int64_t{width - 1} * SPACE_Q16 || r.release_point.y > int64_t{height - 1} * SPACE_Q16) return false;
		// Reject missing-range identities, repeated ownership or invalid routing before native packet containers can be attached.
		for (const auto &[packet, hop] : r.packets) if (packet >= CargoPacketID::End() || !packets.insert(packet).second || (hop != StationID::Invalid() && hop >= StationID::End())) return false;
	}
	return true;
}
/** @copydoc ValidateSpaceDescentLinks */
bool ValidateSpaceDescentLinks(std::span<const SpaceDescentRecord> records, std::span<const SpacePodRecord> pods,
	std::span<const SpaceManifestRecord> manifests, std::span<const LaunchCenterRecord> centers, uint32_t width)
{
	if (width < 64 || width > 4096) return false;
	std::set<SpacePodID> assigned; std::set<CargoPacketID> packets; std::map<std::pair<VehicleID, uint64_t>, uint> pending; std::map<StationID, uint> per_center;
	// Compartment stock and descending stock cannot own the same physical packet; each job retains its paid shell, original circle and exact pending-return incarnation.
	for (const auto &m : manifests) for (const auto &slot : m.supplies) for (const auto &[packet, hop] : slot) { (void)hop; if (!packets.insert(packet).second) return false; }
	for (const auto &r : records) {
		for (const auto &[packet, hop] : r.packets) { (void)hop; if (!packets.insert(packet).second) return false; }
		/** @param p Same-file original shell. @return Whether its full paid incarnation owns this job. */
		const auto pod = std::find_if(pods.begin(), pods.end(), [&](const auto &p) { return p.ref == r.pod; });
		if (pod == pods.end() || pod->state.custody != SpacePodCustody::Descent || pod->state.position != r.release_point || !assigned.insert(r.pod.pod).second) return false;
		/** @param c Same-file recovery anchor. @return Whether its original station and generation match, excluding center reuse. */
		const auto center = std::find_if(centers.begin(), centers.end(), [&](const auto &c) { return c.station == r.center.station && c.anchor.generation == r.center.generation; });
		if (center == centers.end()) { if (r.blocked != SpaceDescentBlock::TargetLost && r.blocked != SpaceDescentBlock::CompanyRemoved) return false; }
		else {
			const auto tile = center->anchor.tile.base(); const SpacePoint anchor{static_cast<int32_t>((tile % width) * SPACE_Q16), static_cast<int32_t>((tile / width) * SPACE_Q16)};
			if (!IsInsideLaunchCenterDropZone(r.release_point, anchor) || ++per_center[center->station] > GetMultiLayerSpaceBudget().launch_pod_cap) return false;
		}
		/** @param m Same-file native quota owner. @return Whether its exact original sender incarnation retains this pending obligation. */
		const auto manifest = std::find_if(manifests.begin(), manifests.end(), [&](const auto &m) { return m.craft == SpaceCraftRef{r.sender, r.sender_generation}; });
		if (manifest != manifests.end()) ++pending[{r.sender, r.sender_generation}];
	}
	// Every descent shell requires exactly one job, and every surviving sender owes exactly its actual jobs; cached quota cannot create or erase custody.
	for (const auto &p : pods) if ((p.state.custody == SpacePodCustody::Descent) != assigned.contains(p.ref.pod)) return false;
	for (const auto &m : manifests) if (m.pending_returns != pending[{m.craft.vehicle, m.craft.generation}]) return false;
	return true;
}
/** Revalidate descent custody against every actual physical cargo owner.
 * @param records Copied descent transport pending acceptance or current validation.
 * @return Whether packets, cargo counts and generation-qualified recovery obligations are consistent.
 */
static bool PhysicalDescents(std::span<const SpaceDescentRecord> records)
{
	std::set<CargoPacketID> owned;
	// A descent can never own packets still held by either a construction job or a completed Core inventory.
	if (!InsertOrbitalPacketOwners(owned)) return false;
	/** @param packets One actual ordinary/native physical container. @return Whether its original packet identities remain exclusive across all earlier owners. */
	auto add = [&](const auto &packets) { for (const auto *cp : packets) if (!owned.insert(cp->index).second) return false; return true; };
	// Include all ordinary station/handling/vehicle stock and Space compartments, so loading a descent cannot duplicate an already owned packet elsewhere.
	for (const auto *station : Station::Iterate()) {
		for (const auto &goods : station->goods) if (goods.HasData()) for (const auto &[hop, packets] : *goods.GetData().cargo.Packets()) { (void)hop; if (!add(packets)) return false; }
		if (station->launch_hub) for (const auto &job : station->launch_hub->handling) for (const auto &[hop, packets] : *job->packets.Packets()) { (void)hop; if (!add(packets)) return false; }
	}
	for (const auto *v : Vehicle::Iterate()) if (!add(*v->cargo.Packets())) return false;
	for (const auto *v : SpaceVehicle::Iterate()) if (v->manifest) for (const auto &slot : v->manifest->supplies) for (const auto &[hop, packets] : *slot.Packets()) { (void)hop; if (!add(packets)) return false; }
	std::map<std::pair<VehicleID, uint64_t>, uint> pending;
	std::map<StationID, uint> jobs;
	// Join each immutable receipt to actual cargo quantities, paid shell and surviving generation-qualified sender/receiver; sender loss never erases its original role/mass bound.
	for (const auto &r : records) {
		const auto *pod = ResolveSpacePod(r.pod);
		// Actual physical quantity/weight must still match the immutable release receipt and exact role after a save or sender loss; a forged smaller cached mass cannot admit an overweight hold.
		if (pod == nullptr || pod->state.custody != SpacePodCustody::Descent || pod->state.position != r.release_point || !ReturnCargo(r.cargo) ||
			CargoSpec::Get(r.cargo)->weight == 0 || uint64_t{r.quantity} * CargoSpec::Get(r.cargo)->weight > GetSpaceDescentMassLimit(r.sender_model) ||
			(r.sender_model == SPACE_HEAVY_MINER_MODEL && r.quantity > GetSpaceFreightCapacity(r.sender_model, r.cargo)) ||
			(r.sender_model != SPACE_DESCENT_LEGACY_SENDER_MODEL && (r.unit_weight != CargoSpec::Get(r.cargo)->weight || r.freight_mass != uint64_t{r.quantity} * CargoSpec::Get(r.cargo)->weight)) ||
			r.center.generation >= GetNextLaunchCenterGeneration() || r.sender_generation >= GetNextSpaceVehicleGeneration()) return false;
		// Original physical packets supply the receipt quantity exactly once; invalid or already owned IDs cannot contribute fabricated cargo.
		uint count = 0; for (const auto &[id, hop] : r.packets) { (void)hop; const auto *cp = CargoPacket::GetIfValid(id); if (cp == nullptr || !owned.insert(id).second) return false; count += cp->Count(); }
		if (count != r.quantity) return false;
		const auto *center = ResolveLaunchCenter(r.center);
		// Only the exact surviving owned recovery center can admit its original circle/capacity; marked loss remains inert history.
		if (center != nullptr && center->owner == pod->state.owner) {
			if (!IsInsideLaunchCenterDropZone(r.release_point, *GetLaunchCenterPlaneReference(center->launch_center->tile)) || ++jobs[center->index] > GetMultiLayerSpaceBudget().launch_pod_cap) return false;
		} else if (r.blocked != SpaceDescentBlock::TargetLost && r.blocked != SpaceDescentBlock::CompanyRemoved) return false;
		if (auto *sender = ResolveSpaceCraft({r.sender, r.sender_generation})) {
			const auto model = GetSpaceVehicleModel(*sender);
			// Old unknown receipts retain only their genuine15/20 sender rights; a known receipt must identify the same surviving role and owner, never a reused hull's capability.
			if (sender->owner != pod->state.owner || !sender->manifest || (r.sender_model == SPACE_DESCENT_LEGACY_SENDER_MODEL ? model != 15 && model != 20 : model != r.sender_model)) return false;
			++pending[{sender->index, sender->generation}];
		}
	}
	// Reconcile every surviving manifest against actual generation-qualified jobs, preventing fabricated or silently discarded pending allowance.
	for (const auto *v : SpaceVehicle::Iterate()) if (v->manifest && v->manifest->pending_returns != pending[{v->index, v->generation}]) return false;
	return true;
}
/** @copydoc ValidateSpaceDescents */
bool ValidateSpaceDescents()
{
	const auto records = CaptureSpaceDescents();
	// Structural bounds, complete copied custody and actual physical packet ownership must all agree before saving or advancing the installed family.
	if (!ValidateSpaceDescentRecords(records, next_descent_id, Map::SizeX(), Map::SizeY()) ||
		!ValidateSpaceDescentLinks(records, CaptureSpacePods(), CaptureSpaceManifests(), CaptureLaunchCenters(), Map::SizeX()) || !PhysicalDescents(records)) return false;
	for (const auto *pod : SpacePod::Iterate()) if ((pod->state.custody == SpacePodCustody::Descent) != (pod->descent != nullptr)) return false;
	return true;
}
/** @copydoc InstallLoadedSpaceDescents */
bool InstallLoadedSpaceDescents(std::span<const SpaceDescentRecord> records, uint64_t next)
{
	// Whole-family preflight precedes any native attachment; replacing a live job could discard the only physical owner of its retained packets.
	if (!ValidateSpaceDescentRecords(records, next, Map::SizeX(), Map::SizeY()) || !PhysicalDescents(records)) return false;
	for (const auto *pod : SpacePod::Iterate()) if (pod->descent) return false;
	// Preflight succeeded for the complete family; bind each original packet to its sole saved pod without issuing new cargo or work.
	for (const auto &r : records) {
		auto *pod = ResolveSpacePod(r.pod); auto job = std::make_unique<SpaceDescentJob>();
		job->id = r.id; job->center = r.center; job->release_point = r.release_point; job->sender = r.sender; job->sender_generation = r.sender_generation;
		job->sender_model = r.sender_model; job->unit_weight = r.unit_weight; job->freight_mass = r.freight_mass;
		job->cargo = r.cargo; job->quantity = r.quantity; job->remaining_ticks = r.remaining_ticks; job->age_ticks = r.age_ticks; job->blocked = r.blocked;
		for (const auto &[id, hop] : r.packets) job->packets.Append(CargoPacket::Get(id), hop);
		pod->descent = std::move(job);
	}
	next_descent_id = next;
	const auto installed = CaptureSpaceDescents();
	for (const auto &r : records) {
		/** @param i Installed original shell/job snapshot. @return Whether this is the exact source row whose packet references must remain unchanged. */
		const auto found = std::find_if(installed.begin(), installed.end(), [&](const auto &i) { return i.pod == r.pod; });
		if (found == installed.end() || found->packets != r.packets) return false;
	}
	return true;
}
/** @copydoc ComputeSpaceDescentDigest */
uint64_t ComputeSpaceDescentDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Original scalar encoded in fixed byte order; digest equality retains actual paid history independently of host layout. */
	auto add = [&](uint64_t value) { for (uint i = 0; i < 8; ++i) { hash ^= (value >> (i * 8)) & 255; hash *= 1099511628211ULL; } }; add(next_descent_id);
	// Hash every original identity/receipt/work field and actual packet provenance; legacy unknown receipts retain their previously accepted stream.
	for (const auto &r : CaptureSpaceDescents()) {
		add(r.pod.pod.base()); add(r.pod.generation); add(r.id); add(r.center.station.base()); add(r.center.generation); add(static_cast<uint32_t>(r.release_point.x)); add(static_cast<uint32_t>(r.release_point.y));
		add(r.sender.base()); add(r.sender_generation); add(r.cargo); add(r.quantity); add(r.remaining_ticks); add(r.age_ticks); add(to_underlying(r.blocked));
		// Genuine older unknown receipts retain their original digest stream; new immutable provenance participates only when actually present.
		if (r.sender_model != SPACE_DESCENT_LEGACY_SENDER_MODEL) { add(r.sender_model); add(r.unit_weight); add(r.freight_mass); }
		for (const auto &[id, hop] : r.packets) { const auto *cp = CargoPacket::Get(id); add(id.base()); add(hop.base()); add(cp->Count()); add(cp->GetPeriodsInTransit()); add(static_cast<int64_t>(cp->GetFeederShare())); add(cp->GetSourceTile().base()); add(cp->GetFirstStation().base()); }
	}
	return hash;
}
