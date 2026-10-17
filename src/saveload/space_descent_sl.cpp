/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_descent_sl.cpp Bounded paid-pod cargo custody, reservations and independent preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups12-14
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; immutable sender/mass receipts and same-file packet quantities.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../launch_center.h"
#include "../space_pod.h"
#include "../space_manifest.h"
#include "../space_mining_tuning.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../map_func.h"
#include "../safeguards.h"

/** Complete unpublished file snapshot; installation attaches its references to the sole original pod/packet owners. */
struct DescentTransport { uint64_t next = 1; std::vector<SpaceDescentRecord> records; };
static std::optional<DescentTransport> candidate;
/** @copydoc ResetSpaceDescentLoadStaging */
void ResetSpaceDescentLoadStaging() { candidate.reset(); }
/** @copydoc AcceptLoadedSpaceDescents */
bool AcceptLoadedSpaceDescents()
{
	if (!candidate) return IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics);
	if (!InstallLoadedSpaceDescents(candidate->records, candidate->next)) return false;
	candidate.reset(); return true;
}
/** @copydoc ValidateSpaceDescentLoadPreview */
void ValidateSpaceDescentLoadPreview()
{
	if (!_load_check_data.space_descents_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing Space descent family");
		return;
	}
	const auto &records = _load_check_data.space_descent_rows;
	if ((!records.empty() || _load_check_data.next_space_descent_id != 1) &&
		(_load_check_data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || _load_check_data.map_layers < 3)) SlErrorCorrupt("Space descent preview requires selected profile");
	if (!ValidateSpaceDescentRecords(records, _load_check_data.next_space_descent_id, _load_check_data.map_size_x, _load_check_data.map_size_y) ||
		!ValidateSpaceDescentLinks(records, _load_check_data.space_pod_rows, _load_check_data.space_manifest_rows, _load_check_data.launch_center_preview_rows, _load_check_data.map_size_x)) SlErrorCorrupt("Invalid Space descent preview custody");
	// Resolve original role/company and real packet quantities only within this file. A removed sender retains immutable provenance; a newer incarnation with the same numeric ID cannot replace it.
	for (const auto &r : records) {
		if (r.sender_generation >= _load_check_data.next_space_vehicle_generation) SlErrorCorrupt("Future descent sender generation preview");
		/** @param p Same-file paid shell. @return Whether its exact incarnation supplies this sole descent owner. */
		const auto pod = std::ranges::find_if(_load_check_data.space_pod_rows, [&](const auto &p) { return p.ref == r.pod; });
		if (pod == _load_check_data.space_pod_rows.end()) SlErrorCorrupt("Missing original descent shell preview");
		/** @param p Same-file paid native receipt. @return Whether it names the original sender generation, excluding numeric ID reuse. */
		const auto receipt = std::ranges::find_if(_load_check_data.space_production_rows, [&](const auto &p) { return p.craft == SpaceCraftRef{r.sender, r.sender_generation}; });
		if (receipt != _load_check_data.space_production_rows.end()) {
			const auto model = receipt->state.content_version == 0 ? 15 : receipt->state.model;
			/** @param v Same-file native Space hull. @return Whether this is the already generation-qualified paid sender. */
			const auto sender = std::ranges::find_if(_load_check_data.native_space_vehicle_rows, [&](const auto &v) { return v.vehicle == r.sender; });
			/** @param m Same-file actual compartment owner. @return Whether this exact sender retains its original pending-return quota. */
			const auto manifest = std::ranges::find_if(_load_check_data.space_manifest_rows, [&](const auto &m) { return m.craft == receipt->craft; });
			// Legacy unknown provenance grants only old15/20 rights. Every known receipt must retain the same surviving role, company and manifest rather than borrowing another hull's allowance.
			if ((r.sender_model == SPACE_DESCENT_LEGACY_SENDER_MODEL ? model != 15 && model != 20 : model != r.sender_model) ||
				sender == _load_check_data.native_space_vehicle_rows.end() || sender->owner != pod->state.owner || manifest == _load_check_data.space_manifest_rows.end()) SlErrorCorrupt("Foreign or mismatched descent sender preview");
		}
		/** @param c Same-file immutable launch anchor. @return Whether the original recovery incarnation still exists. */
		const auto center = std::ranges::find_if(_load_check_data.launch_center_preview_rows, [&](const auto &c) { return c.station == r.center.station && c.anchor.generation == r.center.generation; });
		if (center != _load_check_data.launch_center_preview_rows.end()) {
			/** @param s Same-file native normal Station authority. @return Whether it owns this surviving original launch anchor. */
			const auto authority = std::ranges::find_if(_load_check_data.native_station_owner_rows, [&](const auto &s) { return s.station == r.center.station; });
			// Geometry cannot authorize a foreign recovery. Explicitly marked lost/company-removed history retains its packets until actual authorized retargeting.
			if ((authority == _load_check_data.native_station_owner_rows.end() || authority->owner != pod->state.owner) &&
				r.blocked != SpaceDescentBlock::TargetLost && r.blocked != SpaceDescentBlock::CompanyRemoved) SlErrorCorrupt("Foreign descent recovery preview");
		}
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerDescents)) continue;
		if (!_load_check_data.native_cargo_packets_present) SlErrorCorrupt("Missing copied native descent packet quantities");
		uint64_t quantity = 0; const auto &packets = _load_check_data.native_cargo_packet_rows;
		// Actual CAPA identities/counts must supply the complete receipt. An arithmetic-only snapshot cannot invent a missing packet or hide a partial/duplicated quantity.
		for (const auto &[id, hop] : r.packets) {
			(void)hop; const auto packet = std::ranges::lower_bound(packets, id, {}, &NativeCargoPacketPreview::packet);
			if (packet == packets.end() || packet->packet != id) SlErrorCorrupt("Missing native descent packet preview");
			quantity += packet->quantity;
			// Ordinary native SLE freight refs encode index+1; SPDJ encodes raw indices. No same-file Space hold may simultaneously own this original descent packet.
			for (const auto &hull : _load_check_data.native_space_vehicle_rows) if (std::ranges::find(hull.cargo_packet_refs, id.base() + 1) != hull.cargo_packet_refs.end()) SlErrorCorrupt("Repeated native Space descent freight preview");
		}
		if (quantity != r.quantity) SlErrorCorrupt("Changed native descent quantity preview");
	}
}
/** @param value Original bounded receipt scalar. @param width Exact network-order field width, independent of host object layout. */
static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
/** @param width Exact already admitted network-order field width. @return Detached scalar, awaiting whole receipt/owner validation. */
static uint64_t Read(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
/** Original pod-owned SPDJ transport;434 appends immutable provenance inside the same job rather than creating another ticking owner. */
struct SPDJChunkHandler : ChunkHandler {
	/** Register saved descent transport without allocating cargo, a shell or a recovery reservation. */
	SPDJChunkHandler() : ChunkHandler("SPDJ", ChunkType::Riff) {}
	/** Serialize actual validated jobs, preserving explicit unknown provenance for genuine upgraded old descents. */
	void Save() const override
	{
		if (!ValidateSpaceDescents()) SlErrorCorrupt("Invalid Space descent ownership on save");
		const auto records = CaptureSpaceDescents(); size_t length = 10; for (const auto &r : records) length += 62 + 6 * r.packets.size();
		SlSetLength(length); Write(GetNextSpaceDescentID(), 8); Write(records.size(), 2);
		for (const auto &r : records) {
			Write(r.pod.pod.base(), 2); Write(r.pod.generation, 8); Write(r.id, 8); Write(r.center.station.base(), 2); Write(r.center.generation, 8);
			Write(static_cast<uint32_t>(r.release_point.x), 4); Write(static_cast<uint32_t>(r.release_point.y), 4); Write(r.sender.base(), 4); Write(r.sender_generation, 8);
			Write(r.cargo, 1); Write(r.quantity, 2); Write(r.remaining_ticks, 2); Write(r.age_ticks, 2); Write(to_underlying(r.blocked), 1);
			Write(r.sender_model, 2); Write(r.unit_weight, 1); Write(r.freight_mass, 2); Write(r.packets.size(), 1);
			for (const auto &[packet, hop] : r.packets) { Write(packet.base(), 4); Write(hop.base(), 2); }
		}
	}
	/** @return Complete bounded source snapshot; pre434 rows retain unknown role/zero new dimensions without guessing or rewriting old cargo/work history. */
	DescentTransport ReadRecords() const
	{
		const auto length = SlGetFieldLength(); const uint fixed = IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerDescents) ? 57 : 62;
		// Source-era fixed width and complete packet framing precede scalar reads; a downgraded header cannot reinterpret new physical provenance as an old packet list.
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) || length < 10 || length > 10 + SpacePodID::End().base() * (fixed + 6 * 40)) SlErrorCorrupt("Invalid Space descent length");
		DescentTransport transport; transport.next = Read(8); const auto count = Read(2); if (count > SpacePodID::End().base()) SlErrorCorrupt("Invalid Space descent count");
		size_t consumed = 10;
		for (uint i = 0; i < count; ++i) {
			if (consumed + fixed > length) SlErrorCorrupt("Truncated Space descent row");
			SpaceDescentRecord r; r.pod = {SpacePodID{static_cast<uint16_t>(Read(2))}, Read(8)}; r.id = Read(8); r.center = {StationID{static_cast<uint16_t>(Read(2))}, Read(8)};
			r.release_point = {static_cast<int32_t>(Read(4)), static_cast<int32_t>(Read(4))}; r.sender = VehicleID{static_cast<uint32_t>(Read(4))}; r.sender_generation = Read(8); r.cargo = static_cast<CargoType>(Read(1));
			r.quantity = static_cast<uint16_t>(Read(2)); r.remaining_ticks = static_cast<uint16_t>(Read(2)); r.age_ticks = static_cast<uint16_t>(Read(2)); r.blocked = static_cast<SpaceDescentBlock>(Read(1));
			if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerDescents)) { r.sender_model = static_cast<uint16_t>(Read(2)); r.unit_weight = static_cast<uint8_t>(Read(1)); r.freight_mass = static_cast<uint16_t>(Read(2)); }
			const auto packets = Read(1); consumed += fixed + 6 * packets; if (packets == 0 || packets > 40 || consumed > length) SlErrorCorrupt("Invalid Space descent packet count");
			for (uint j = 0; j < packets; ++j) r.packets.push_back({CargoPacketID{static_cast<uint32_t>(Read(4))}, StationID{static_cast<uint16_t>(Read(2))}});
			transport.records.push_back(std::move(r));
		}
		if (consumed != length) SlErrorCorrupt("Invalid Space descent transport length");
		return transport;
	}
	/** Stage one complete family; repeated transport cannot overwrite an original packet owner's candidate. */
	void Load() const override { if (candidate) SlErrorCorrupt("Repeated Space descent family"); candidate = this->ReadRecords(); }
	/** Validate profile/geometry after normal map staging, before publishing any native descent owner. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!candidate) { if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing Space descent family"); return; }
		const auto *schema = GetPendingMultiLayerStateSchema();
		if ((!candidate->records.empty() || candidate->next != 1) && (schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Space descent requires selected profile");
		if (!ValidateSpaceDescentRecords(candidate->records, candidate->next, Map::SizeX(), Map::SizeY())) SlErrorCorrupt("Invalid Space descent transport");
	}
	/** @param length Framework extent; ReadRecords validates complete source-era framing before copying preview rows. */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_descents_present) SlErrorCorrupt("Repeated Space descent preview");
		auto data = this->ReadRecords(); _load_check_data.space_descent_rows = std::move(data.records); _load_check_data.next_space_descent_id = data.next; _load_check_data.space_descents_present = true;
	}
};
static const SPDJChunkHandler SPDJ;
static const ChunkHandlerRef handlers[] = {SPDJ};
extern const ChunkHandlerTable _space_descent_chunk_handlers(handlers);
