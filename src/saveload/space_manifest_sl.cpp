/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_manifest_sl.cpp Bounded native spacecraft supply custody and independent preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group2C
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B; actual onboard service can coexist only with its reached owned-center instruction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; older heavy sources cannot contain future paid supply/custody.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CC; source-gated heavy onboard work joins actual same-file part quantities.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../space_manifest.h"
#include "../space_mining_tuning.h"
#include "../space_vehicle.h"
#include "../space_order_actions.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../safeguards.h"

static std::optional<std::vector<SpaceManifestRecord>> candidate; ///< Detached candidate; native installation waits for the original hull/order/launch owners.
/** Discard unpublished transport during world teardown without changing installed physical compartments. */
void ResetSpaceManifestLoadStaging() { candidate.reset(); }
/** Publish only existing native manifests after queue reconstruction and explicit service-version admission.
 * @return Whether the complete saved manifest set installed without granting material, work or allowance.
 */
bool AcceptLoadedSpaceManifests()
{
	if (!candidate) return IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics);
	// Older saves never owned onboard work under an explicit Return/Service instruction. Do not infer the new action/producer contract from a forged old-version header.
	for (const auto &row : *candidate) {
		const auto *owner = ResolveSpaceCraft(row.craft);
		// Real source-era hull identity precedes installation; a commissioning/mining-only heavy save cannot acquire later shell/part/return/service history through a forged header.
		if (owner != nullptr && !IsSpaceHeavyResupplyStateSupported(GetSpaceVehicleModel(*owner), row, !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerSupplies), !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerDescents), !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerOnboardServices))) return false;
		if (row.service_ticks == 0 || row.service_location != SpaceServiceLocation::Onboard) continue;
		const auto *craft = ResolveSpaceCraft(row.craft);
		if (craft != nullptr && IsSpaceReturnServiceWait(craft->space_orders) && IsSavegameVersionBefore(SaveLoadVersion::SpaceOrderedActions)) return false;
	}
	if (!InstallLoadedSpaceManifests(*candidate)) return false;
	candidate.reset(); return true;
}
/** Validate a reached onboard service order against copied same-file hull and launch-anchor geometry.
 * @param row Actual copied manifest identity; this helper never resolves a live hull or station.
 * @param queue Same-file current queue already admitted by its role/identity validator.
 * @return Whether this version's explicit reached LaunchCenter action can coexist with the saved onboard work.
 */
static bool ValidOnboardServiceOrderPreview(const SpaceManifestRecord &row, const SpaceOrderQueue &queue)
{
	// Current action shape and version must precede all target lookups; a future queue entry or unrelated work cannot authorize service.
	if (!IsSpaceReturnServiceWait(queue) || IsSavegameVersionBefore(SaveLoadVersion::SpaceOrderedActions)) return false;
	const auto &target = queue.entries[queue.cursor].target;
	if (target.kind != SpaceOrderTargetKind::LaunchCenter) return false;
	const auto &centers = _load_check_data.launch_center_preview_rows;
	/** @param center Same-file launch anchor. @return Whether its exact original incarnation is this instruction's target. */
	const auto center = std::ranges::find_if(centers, [&](const auto &center) { return center.station.base() == target.id && center.anchor.generation == target.generation; });
	const auto &hulls = _load_check_data.native_space_vehicle_rows;
	/** @param hull Same-file native hull. @return Whether its pool identity is this already generation-qualified manifest's owner. */
	const auto hull = std::ranges::find_if(hulls, [&](const auto &hull) { return hull.vehicle == row.craft.vehicle; });
	const auto &stations = _load_check_data.native_station_owner_rows;
	/** @param station Same-file native Station authority. @return Whether its original index supplies this launch anchor. */
	const auto station = std::ranges::find_if(stations, [&](const auto &station) { return station.station.base() == target.id; });
	if (hull == hulls.end() || _load_check_data.map_size_x == 0) return false;
	// A retired explicit target can retain only marked history under this already existing onboard timer. An unmarked foreign/missing center cannot authorize a new physical service action in preview.
	if (center == centers.end() || station == stations.end() || station->owner != hull->owner) return queue.blocked == SpaceOrderBlock::TargetRemoved;
	// Resolve the file's own surviving Surface geometry rather than the open map; even marked history cannot claim arrival at a different still-owned physical anchor.
	const auto tile = center->anchor.tile.base();
	return hull->position == SpacePoint{static_cast<int32_t>(tile % _load_check_data.map_size_x) * SPACE_Q16,
		static_cast<int32_t>(tile / _load_check_data.map_size_x) * SPACE_Q16};
}
/** Validate copied compartment, passenger and service references using only the complete candidate snapshot; preview never publishes work or accesses live inventories. */
void ValidateSpaceManifestLoadPreview()
{
	if (!_load_check_data.space_manifests_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing Space manifest family");
		return;
	}
	const auto &records = _load_check_data.space_manifest_rows;
	// Populated compartments belong to the same-file Space profile and complete bounded custody family; the open world's profile grants no preview authority.
	if (!ValidateSpaceManifestRecords(records, !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerOnboardServices)) || (!records.empty() &&
		(_load_check_data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || _load_check_data.map_layers < 3))) SlErrorCorrupt("Invalid Space manifest preview profile");
	/** @param ref Exact copied hull incarnation. @return Its same-file paid receipt, without looking up a live hull. */
	auto production = [&](SpaceCraftRef ref) -> const SpaceConstructionState * {
		for (const auto &r : _load_check_data.space_production_rows) if (r.craft == ref) return &r.state;
		return nullptr;
	};
	/** @param ref Exact passenger/carrier/service owner. @return Its same-file manifest, never another world's compartment. */
	auto find = [&](SpaceCraftRef ref) -> const SpaceManifestRecord * { for (const auto &r : records) if (r.craft == ref) return &r; return nullptr; };
	// Join each sole compartment owner to its original paid role, service source boundary and reciprocal same-file carrier/work state.
	for (const auto &r : records) {
		const auto *s = production(r.craft); if (s == nullptr) SlErrorCorrupt("Missing Space manifest craft identity");
		const auto model = s->content_version == 0 ? 15 : s->model;
		if (!IsSpaceHeavyResupplyStateSupported(model, r, !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerSupplies), !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerDescents), !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerOnboardServices))) SlErrorCorrupt("Unsupported heavy supply or future work history in source preview");
		const auto service = GetSpaceServiceTuning(model);
		// Source-wide structural headroom never lends a legacy or unrelated role the heavy's296-opportunity service or completed history.
		if ((r.service_ticks != 0 || r.services_completed != 0) && (!service || r.service_ticks > service->ticks)) SlErrorCorrupt("Invalid per-model service history preview");
		// Legacy unpaid placement cannot gain initial allowance, and role-specific flight/service histories cannot migrate between rockets and probes.
		if (s->content_version == 0 && r.returns_remaining != 0 && r.services_completed == 0) SlErrorCorrupt("Unearned legacy service allowance in manifest preview");
		if ((model == 14 && (r.carrier != SpaceCraftRef{} || r.service_ticks != 0 || r.returns_remaining != 0 || r.pending_returns != 0 || r.services_completed != 0)) ||
			(model != 14 && (r.passenger != SpaceCraftRef{} || r.fuel_burned != 0))) SlErrorCorrupt("Invalid Space manifest preview role");
		// Each reference already represents a positive physical packet; even that lower bound must fit the exact role's compartment before later packet-quantity joins.
		for (uint i = 0; i < SPACE_SUPPLY_COUNT; ++i) if (r.supplies[i].size() > GetSpaceSupplyCapacity(model, static_cast<SpaceSupply>(i))) SlErrorCorrupt("Invalid Space manifest preview capacity");
		// New heavy onboard work requires actual same-file packet quantities. Two references cannot impersonate two parts, and one undersized packet cannot finance a two-part completion.
		if (model == SPACE_HEAVY_MINER_MODEL && !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerOnboardServices)) {
			if (!_load_check_data.native_cargo_packets_present) SlErrorCorrupt("Missing native heavy service part quantities preview");
			const auto &packets = _load_check_data.native_cargo_packet_rows;
			// Each fixed native compartment keeps its original bounded physical quantity; missing packet IDs and underfunded service retain no admission even when another open world has enough parts.
			for (uint i = 0; i < SPACE_SUPPLY_COUNT; ++i) {
				uint64_t count = 0;
				for (const auto &[id, hop] : r.supplies[i]) {
					(void)hop; const auto packet = std::ranges::lower_bound(packets, id, {}, &NativeCargoPacketPreview::packet);
					if (packet == packets.end() || packet->packet != id) SlErrorCorrupt("Missing native heavy service packet preview");
					count += packet->quantity;
				}
				if (count > GetSpaceSupplyCapacity(model, static_cast<SpaceSupply>(i))) SlErrorCorrupt("Overfull native heavy service compartment preview");
				if (r.service_ticks != 0 && i != 0 && count < service->inputs[i - 1]) SlErrorCorrupt("Insufficient native heavy service parts preview");
			}
		}
		if (r.passenger != SpaceCraftRef{}) {
			const auto *child = find(r.passenger); const auto *state = production(r.passenger);
			// The original same-file paid passenger must retain reciprocal carrier custody and the same real Earth facility.
			if (child == nullptr || state == nullptr || child->carrier != r.craft || state->placement != SpacePlacement::Carried || state->home != s->home) SlErrorCorrupt("Invalid Space manifest passenger preview");
		}
		if (r.carrier != SpaceCraftRef{}) {
			const auto *parent = find(r.carrier); const auto *state = production(r.carrier);
			// A carried paid hull requires the original rocket and its actual Surface/in-flight custody; copied IDs alone cannot create a passenger.
			if (parent == nullptr || state == nullptr || parent->passenger != r.craft || state->model != 14 || (state->placement != SpacePlacement::SurfaceStored && state->placement != SpacePlacement::InTransit) || s->placement != SpacePlacement::Carried) SlErrorCorrupt("Invalid Space manifest carrier preview");
		} else if (s->placement == SpacePlacement::Carried) SlErrorCorrupt("Unowned carried hull in manifest preview");
		if (r.service_ticks != 0) {
			if (r.service_location == SpaceServiceLocation::Onboard) {
				// Onboard work requires real orbital placement and exhausted allowance; supply alone never starts service.
				if (s->placement != SpacePlacement::Orbital || r.returns_remaining != 0) SlErrorCorrupt("Invalid onboard service preview");
				const auto &queues = _load_check_data.space_order_preview_rows;
				/** @param o Same-file native order owner. @return Whether its exact incarnation belongs to this retained service manifest. */
				const auto q = std::find_if(queues.begin(), queues.end(), [&](const auto &o) { return SpaceCraftRef{o.vehicle, o.generation} == r.craft; });
				// Existing held Mine and the new precisely reached Return/Service are the only active instructions compatible with this same-file onboard producer.
				if (q == queues.end() || (q->queue.cursor < q->queue.entries.size() && !IsSpaceMiningServiceWait(q->queue) && !ValidOnboardServiceOrderPreview(r, q->queue))) SlErrorCorrupt("Active order conflicts with onboard service preview");
			} else if (s->placement == SpacePlacement::Unhomed) { if (!r.service_paused) SlErrorCorrupt("Unpaused unhomed service preview"); }
			else {
				bool reserved = false;
				// Unpaused Earth service must retain the original facility's exact sole service slot; a reused or unrelated center cannot lend its workshop.
				for (const auto &op : _load_check_data.launch_operations_rows) if (op.center == s->home && op.slots.service == r.craft) reserved = true;
				if (s->placement != SpacePlacement::SurfaceStored || (!r.service_paused && !reserved)) SlErrorCorrupt("Missing native service reservation preview");
			}
		}
	}
	// Close the reverse workshop join: a saved slot cannot exist without that exact original manifest's paid Earth work.
	for (const auto &op : _load_check_data.launch_operations_rows) if (op.slots.service != SpaceCraftRef{}) {
		const auto *r = find(op.slots.service); if (r == nullptr || r->service_ticks == 0 || r->service_location != SpaceServiceLocation::EarthWorkshop) SlErrorCorrupt("Service slot has no manifest work preview");
	}
	// Every carried native receipt must have a reciprocal manifest owner, preventing a detached hull from claiming a second carrier domain.
	for (const auto &r : _load_check_data.space_production_rows) if (r.state.placement == SpacePlacement::Carried && find(r.craft) == nullptr) SlErrorCorrupt("Carried hull has no manifest preview");
}
/** @param value Canonical saved unsigned field. @param width Exact big-endian byte width. */
static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
/** @param width Exact field width within the caller's admitted manifest row. @return Canonical unsigned field. */
static uint64_t Read(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
/** @param ref Actual generation-qualified owner; saving does not issue another hull identity. */
static void WriteCraft(SpaceCraftRef ref) { Write(ref.vehicle.base(), 4); Write(ref.generation, 8); }
/** @return Detached native index/incarnation; complete same-file validation follows before publication. */
static SpaceCraftRef ReadCraft() { return {VehicleID{static_cast<uint32_t>(Read(4))}, Read(8)}; }
/** Native SPSU compartment/manifest transport; all physical packet objects and paid-service work remain owned by the original hull and producers. */
struct SPSUChunkHandler : ChunkHandler {
	/** Register the existing bounded manifest family without adding an inventory or service timer. */
	SPSUChunkHandler() : ChunkHandler("SPSU", ChunkType::Riff) {}
	/** Serialize only already validated actual manifest/packet references, including their original service and allowance receipts. */
	void Save() const override
	{
		if (!ValidateSpaceManifests()) SlErrorCorrupt("Invalid Space manifest ownership on save");
		const auto records = CaptureSpaceManifests(); size_t length = 1;
		// Every actual shell and compartment reference contributes once to the declared framing extent; no quantity-only inventory is synthesized.
		for (const auto &r : records) { length += 64 + 10 * r.pods.size(); for (const auto &slot : r.supplies) length += 6 * slot.size(); }
		SlSetLength(length); Write(records.size(), 1);
		// Stable native owner order preserves exact links and service history across ordinary save/reload.
		for (const auto &r : records) {
			WriteCraft(r.craft); WriteCraft(r.passenger); WriteCraft(r.carrier); Write(r.age_ticks, 2); Write(r.service_ticks, 2); Write(r.returns_remaining, 1); Write(r.service_paused, 1);
			Write(r.pending_returns, 1); Write(to_underlying(r.service_location), 1);
			Write(r.fuel_burned, 8); Write(r.services_completed, 8);
			Write(r.pods.size(), 1); for (const auto pod : r.pods) { Write(pod.pod.base(), 2); Write(pod.generation, 8); }
			for (const auto &slot : r.supplies) { Write(slot.size(), 1); for (const auto &[packet, hop] : slot) { Write(packet.base(), 4); Write(hop.base(), 2); } }
		}
	}
	/** @return Detached bounded rows; every variable shell/compartment extent is admitted before its references are read. */
	std::vector<SpaceManifestRecord> ReadRecords() const
	{
		const auto length = SlGetFieldLength();
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) || length < 1 || length > 1 + MAX_SPACE_VEHICLES * (64 + 80 + 6 * 128 * SPACE_SUPPLY_COUNT)) SlErrorCorrupt("Invalid Space manifest length");
		const auto count = Read(1); if (count > MAX_SPACE_VEHICLES) SlErrorCorrupt("Invalid Space manifest count");
		std::vector<SpaceManifestRecord> records; size_t consumed = 1;
		// Reject each row's complete shell/compartment extent before reading it so malformed owner counts cannot cross native family framing.
		for (uint i = 0; i < count; ++i) {
			if (consumed + 64 > length) SlErrorCorrupt("Truncated Space manifest row");
			SpaceManifestRecord r; r.craft = ReadCraft(); r.passenger = ReadCraft(); r.carrier = ReadCraft();
			r.age_ticks = static_cast<uint16_t>(Read(2)); r.service_ticks = static_cast<uint16_t>(Read(2)); r.returns_remaining = static_cast<uint8_t>(Read(1)); const auto paused = Read(1);
			if (paused > 1) SlErrorCorrupt("Invalid Space manifest boolean"); r.service_paused = paused != 0;
			r.pending_returns = static_cast<uint8_t>(Read(1)); r.service_location = static_cast<SpaceServiceLocation>(Read(1));
			r.fuel_burned = Read(8); r.services_completed = Read(8);
			const auto pods = Read(1); consumed += 61 + 10 * pods;
			if (pods > 8 || consumed + SPACE_SUPPLY_COUNT > length) SlErrorCorrupt("Invalid Space manifest pod count");
			for (uint j = 0; j < pods; ++j) r.pods.push_back({SpacePodID{static_cast<uint16_t>(Read(2))}, Read(8)});
			for (auto &slot : r.supplies) {
				if (consumed + 1 > length) SlErrorCorrupt("Truncated Space manifest slot");
				const auto packets = Read(1); consumed += 1 + 6 * packets;
				if (packets > 128 || consumed > length) SlErrorCorrupt("Invalid Space manifest packet count");
				for (uint j = 0; j < packets; ++j) slot.push_back({CargoPacketID{static_cast<uint32_t>(Read(4))}, StationID{static_cast<uint16_t>(Read(2))}});
			}
			records.push_back(std::move(r));
		}
		if (consumed != length || !ValidateSpaceManifestRecords(records, !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerOnboardServices))) SlErrorCorrupt("Invalid Space manifest transport");
		return records;
	}
	/** Stage one complete detached candidate; duplicate families cannot replace the first owner's references. */
	void Load() const override { if (candidate) SlErrorCorrupt("Repeated Space manifest family"); candidate = this->ReadRecords(); }
	/** Check candidate/profile presence during native pointer reconstruction; physical publication remains the later all-owner acceptance step. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!candidate && !IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing Space manifest family");
		const auto *schema = GetPendingMultiLayerStateSchema();
		if (candidate && !candidate->empty() && (schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Space manifests require selected profile");
	}
	/** Copy the same bounded rows for independent preview without allocating native hulls, packets or service grants.
	 * @param length Framework-provided family extent; ReadRecords independently checks the actual current field length.
	 */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_manifests_present) SlErrorCorrupt("Repeated Space manifest preview");
		_load_check_data.space_manifest_rows = this->ReadRecords(); _load_check_data.space_manifests_present = true;
	}
};
static const SPSUChunkHandler SPSU;
static const ChunkHandlerRef handlers[] = {SPSU};
extern const ChunkHandlerTable _space_manifest_chunk_handlers(handlers);
