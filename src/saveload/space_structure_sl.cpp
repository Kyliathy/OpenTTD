/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure_sl.cpp Bounded construction and operational Core transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-6.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../space_structure.h"
#include "../orbital_service.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../map_func.h"
#include "../timer/timer_game_tick.h"
#include <set>
#include "../safeguards.h"

/** Detached records; decoding/preview never publishes native inventory or activation. */
struct StructureTransport { uint64_t next = 1; std::vector<SpaceStructureRecord> rows; };
static std::optional<StructureTransport> candidate;
/** @copydoc ResetSpaceStructureLoadStaging */
void ResetSpaceStructureLoadStaging() { candidate.reset(); }
/** @copydoc AcceptLoadedSpaceStructures */
bool AcceptLoadedSpaceStructures()
{
	if (!candidate) return IsSavegameVersionBefore(SaveLoadVersion::SpaceConstructionJobs);
	if (!InstallLoadedSpaceStructures(candidate->rows, candidate->next)) return false;
	candidate.reset(); return true;
}
/** @copydoc ValidateSpaceStructureLoadPreview */
void ValidateSpaceStructureLoadPreview()
{
	const auto &data = _load_check_data;
	if (!data.space_structures_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceConstructionJobs)) SlErrorCorrupt("Missing orbital construction family");
		return;
	}
	const auto &rows = data.space_structure_rows;
	// Nonempty native orbital state requires the same file's selected Space profile, rather than the current camera/world.
	if ((!rows.empty() || data.next_space_structure_generation != 1) && (data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || data.map_layers < 3)) SlErrorCorrupt("Orbital construction requires selected Space profile");
	// Assembly receipts compare against this file's copied clock, never the currently open world's opportunity.
	if (!rows.empty() && !data.native_tick_present) SlErrorCorrupt("Missing orbital assembly clock preview");
	if (!ValidateSpaceStructureRecords(rows, data.next_space_structure_generation, data.map_size_x, data.map_size_y, data.native_tick_counter)) SlErrorCorrupt("Invalid orbital construction preview");
	if (!ValidateOrbitalServiceUtilityRecords(rows, data.utility_power_state)) SlErrorCorrupt("Invalid orbital service utility preview");
	std::set<CargoPacketID> packets;
	// Cross-family copied owners must remain exclusive even when no live native pool has been installed.
	for (const auto &m : data.space_manifest_rows) for (const auto &slot : m.supplies) for (const auto &[id, hop] : slot) { (void)hop; if (!packets.insert(id).second) SlErrorCorrupt("Repeated orbital cargo preview owner"); }
	for (const auto &d : data.space_descent_rows) for (const auto &[id, hop] : d.packets) { (void)hop; if (!packets.insert(id).second) SlErrorCorrupt("Repeated orbital cargo preview owner"); }
	for (const auto &r : rows) {
		// A saved owned site must resolve to this file's company catalogue; unowned retained cargo is allowed.
		if (r.state.owner != OWNER_NONE && !data.companies.contains(r.state.owner.base())) SlErrorCorrupt("Missing orbital construction company preview");
		for (const auto &slot : r.inputs) for (const auto &[id, hop] : slot) { (void)hop; if (!packets.insert(id).second) SlErrorCorrupt("Repeated orbital construction packet preview"); }
		if (r.module) for (const auto &slot : r.module->stock) for (const auto &[id, hop] : slot) { (void)hop; if (!packets.insert(id).second) SlErrorCorrupt("Repeated operational Core packet preview"); }
		if (r.module) {
			const auto &job = r.module->service;
			// Service reservations join cross-family exclusivity before a detached preview can grant valid load status.
			for (const auto &slot : job.inputs) for (const auto &[id, hop] : slot) { (void)hop; if (!packets.insert(id).second) SlErrorCorrupt("Repeated orbital service packet preview"); }
			if (job.remaining_ticks != 0) {
				if (job.craft.generation >= data.next_space_vehicle_generation) SlErrorCorrupt("Unissued orbital service craft preview");
				/** @param m Copied native manifest. @return Whether it owns the exact job's hull incarnation. */
				const auto manifest = std::find_if(data.space_manifest_rows.begin(), data.space_manifest_rows.end(), [&](const auto &m) { return m.craft == job.craft; });
				/** @param v Copied native hull. @return Whether its pool ID matches the exact incarnation admitted by the order catalogue below. */
				const auto craft = std::find_if(data.native_space_vehicle_rows.begin(), data.native_space_vehicle_rows.end(), [&](const auto &v) { return v.vehicle == job.craft.vehicle; });
				const bool live = std::find(data.space_order_preview_refs.begin(), data.space_order_preview_refs.end(), job.craft) != data.space_order_preview_refs.end();
				/** @param p Copied paid construction receipt. @return Whether it belongs to this exact berth incarnation. */
				const auto production = std::find_if(data.space_production_rows.begin(), data.space_production_rows.end(), [&](const auto &p) { return p.craft == job.craft; });
				// Removed craft may retain a waiting reservation; a surviving craft must own the same service serial and company.
				if (live && (craft == data.native_space_vehicle_rows.end() || craft->owner != r.state.owner || manifest == data.space_manifest_rows.end() || manifest->service_ticks != 0 || manifest->services_completed != job.services_at_start)) SlErrorCorrupt("Foreign or changed orbital service craft preview");
				// A live occupant must be the same paid deployed probe accepted by the physical service command, never a rocket or preview-only hull.
				if (live && (production == data.space_production_rows.end() || production->state.content_version != MULTILAYER_SPACE_CONTENT_VERSION ||
					production->state.placement != SpacePlacement::Orbital || (production->state.model != 15 && production->state.model != 19 && production->state.model != 20))) SlErrorCorrupt("Invalid orbital service probe role preview");
			}
		}
	}
	// Typed navigation targets may remain removed, but cannot refer to a future incarnation or another company's live site.
	for (const auto &q : data.space_order_preview_rows) for (const auto &o : q.queue.entries) {
		// Only static site targets belong to this owner; other target families retain their own validation.
		if (o.target.kind != SpaceOrderTargetKind::Structure && o.target.kind != SpaceOrderTargetKind::Blueprint) continue;
		if (o.target.id >= SpaceStructureID::End().base() || o.target.generation >= data.next_space_structure_generation) SlErrorCorrupt("Unissued orbital construction target preview");
		/** @param r Copied site. @return Whether it is the exact referenced incarnation. */
		const auto site = std::find_if(rows.begin(), rows.end(), [&](const auto &r) { return r.ref.structure.base() == o.target.id && r.ref.generation == o.target.generation; });
		const auto v = std::find_if(data.native_space_vehicle_rows.begin(), data.native_space_vehicle_rows.end(), [&](const auto &r) { return r.vehicle == q.vehicle; });
		// A live site target cannot silently borrow another company's operational facilities.
		if (site != rows.end() && site->state.owner != OWNER_NONE && v != data.native_space_vehicle_rows.end() && site->state.owner != v->owner) SlErrorCorrupt("Foreign orbital construction target preview");
	}
}
/** @param value Canonical integer. @param width Big-endian byte width. */
static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
/** @param width Big-endian byte width already admitted by the caller. @return Decoded integer. */
static uint64_t Read(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
/** Native SPST family:415 job prefix,416 stock,417 service and418 exact assembly work/consumer receipts. */
struct SPSTChunkHandler : ChunkHandler {
	/** Register the single bounded native structure family. */
	SPSTChunkHandler() : ChunkHandler("SPST", ChunkType::Riff) {}
	/** Serialize validated native owners, preserving exact packet identity and activation countdown. */
	void Save() const override
	{
		if (!ValidateSpaceStructures()) SlErrorCorrupt("Invalid orbital construction on save");
		const auto rows = CaptureSpaceStructures(); size_t length = 10 + 62 * rows.size();
		// Exact transport length includes every retained packet reference, never cargo quantity as a packet surrogate.
		for (const auto &r : rows) for (const auto &slot : r.inputs) length += 6 * slot.size();
		for (const auto &r : rows) if (r.module) {
			length += 5 + 56 + 4;
			for (const auto &slot : r.module->stock) if (!slot.empty()) length += 5 + 6 * slot.size();
			for (const auto &slot : r.module->service.inputs) length += 6 * slot.size();
		}
		SlSetLength(length); Write(GetNextSpaceStructureGeneration(), 8); Write(rows.size(), 2);
		// Write the unchanged construction prefix before the append-only contained facility payload.
		for (const auto &r : rows) {
			Write(r.ref.structure.base(), 2); Write(r.ref.generation, 8); const auto &s = r.state;
			Write(s.owner.base(), 1); Write(to_underlying(s.kind), 1); Write(to_underlying(s.phase), 1); Write(s.x, 2); Write(s.y, 2); Write(s.content_version, 2); Write(s.capital_paid, 4);
			Write(r.work_ticks, 4); Write(r.age_ticks, 2);
			for (uint i = 0; i < 2; ++i) { Write(r.consumed[i], 2); Write(r.quantities[i], 2); Write(r.inputs[i].size(), 2); for (const auto &[packet, hop] : r.inputs[i]) { Write(packet.base(), 4); Write(hop.base(), 2); } }
			Write(r.module.has_value(), 1);
			if (r.module) {
				const auto &m = *r.module; Write(m.activation_ticks, 2); Write(m.age_ticks, 2);
				Write(std::count_if(m.stock.begin(), m.stock.end(), [](const auto &slot) { return !slot.empty(); }), 1);
				// Sparse cargo slots remain tied to their resolved saved cargo IDs and real packet next hops.
				for (CargoType cargo : EnumRange(NUM_CARGO)) if (!m.stock[cargo].empty()) {
					Write(cargo, 1); Write(m.quantities[cargo], 2); Write(m.stock[cargo].size(), 2);
					for (const auto &[packet, hop] : m.stock[cargo]) { Write(packet.base(), 4); Write(hop.base(), 2); }
				}
				// Append417 fields after the unchanged416 sparse stock payload; old saves receive no implicit commissioned IDs.
				Write(m.producer.base(), 4); Write(m.consumer.base(), 4); Write(m.power_enabled, 1); Write(m.services_completed, 8);
				const auto &job = m.service;
				Write(job.craft.vehicle.base(), 4); Write(job.craft.generation, 8); Write(job.remaining_ticks, 2); Write(job.paused, 1); Write(job.last_work_interval, 8); Write(job.services_at_start, 8);
				for (uint i = 0; i < 2; ++i) {
					Write(job.quantities[i], 2); Write(job.inputs[i].size(), 2);
					for (const auto &[packet, hop] : job.inputs[i]) { Write(packet.base(), 4); Write(hop.base(), 2); }
				}
				Write(m.assembly_consumer.base(), 4); // Append418 after the unchanged417 contained-service payload.
			}
			// Every native row carries the exact fractional milestone and tick receipt, including portable Core and cancelled jobs.
			Write(r.work_fraction, 2); Write(r.work_remainder, 2); Write(r.last_work_tick, 8); Write(r.last_work_interval, 8);
		}
	}
	/** @return Detached, length-bounded records; old415 frames stay inert until explicitly activated. */
	StructureTransport ReadRecords() const
	{
		const auto length = SlGetFieldLength();
		const bool facilities = !IsSavegameVersionBefore(SaveLoadVersion::OrbitalCoreFacilities);
		const bool services = !IsSavegameVersionBefore(SaveLoadVersion::OrbitalCoreServices);
		const bool milestones = !IsSavegameVersionBefore(SaveLoadVersion::OrbitalAssemblyMilestones);
		const size_t row_limit = 47 + 56 + 12 + 24 + 6 * 128 + NUM_CARGO * (5 + 6 * 64);
		// Bound the whole family before allocating records; older saves cannot smuggle newer facility payloads.
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceConstructionJobs) || length < 10 || length > 10 + SpaceStructureID::End().base() * row_limit) SlErrorCorrupt("Invalid orbital construction length");
		size_t consumed = 0;
		/** @param width Requested integer width. @return Value only after admitting its bytes within this family. */
		auto read = [&](uint width) { if (width > length - consumed) SlErrorCorrupt("Truncated orbital construction data"); consumed += width; return Read(width); };
		StructureTransport t; t.next = read(8); const auto count = read(2);
		// Every native row has a mandatory construction prefix and, from416, one facility-presence byte.
		if (count > SpaceStructureID::End().base() || consumed + count * ((facilities ? 42 : 41) + (milestones ? 20 : 0)) > length) SlErrorCorrupt("Invalid orbital construction count");
		// Decode bounded native rows without allocating pools or resolving any live packet owner.
		for (uint j = 0; j < count; ++j) {
			SpaceStructureRecord r; r.ref = {SpaceStructureID{static_cast<uint16_t>(read(2))}, read(8)}; auto &s = r.state;
			s.owner = Owner{static_cast<uint8_t>(read(1))}; s.kind = static_cast<SpaceBuildKind>(read(1)); s.phase = static_cast<SpaceStructurePhase>(read(1));
			s.x = static_cast<uint16_t>(read(2)); s.y = static_cast<uint16_t>(read(2)); s.content_version = static_cast<uint16_t>(read(2)); s.capital_paid = static_cast<uint32_t>(read(4));
			r.work_ticks = static_cast<uint32_t>(read(4)); r.age_ticks = static_cast<uint16_t>(read(2));
			// Preserve each recipe slot's consumed receipt, current count and exact physical packet references.
			for (uint i = 0; i < 2; ++i) {
				r.consumed[i] = static_cast<uint16_t>(read(2)); r.quantities[i] = static_cast<uint16_t>(read(2)); const auto packets = read(2);
				if (packets > 64 || packets * 6 > length - consumed) SlErrorCorrupt("Invalid orbital construction packet count");
				for (uint k = 0; k < packets; ++k) r.inputs[i].push_back({CargoPacketID{static_cast<uint32_t>(read(4))}, StationID{static_cast<uint16_t>(read(2))}});
			}
			if (facilities) {
				const auto present = read(1);
				if (present > 1) SlErrorCorrupt("Invalid orbital Core presence");
				if (present != 0) {
					r.module.emplace(); auto &m = *r.module;
					m.activation_ticks = static_cast<uint16_t>(read(2)); m.age_ticks = static_cast<uint16_t>(read(2)); const auto slots = read(1);
					if (slots > NUM_CARGO) SlErrorCorrupt("Invalid orbital Core cargo count");
					// Sparse slots must be strictly increasing, bounded and nonempty; repeats cannot create two owners of a label.
					int previous = -1;
					for (uint i = 0; i < slots; ++i) {
						const auto cargo = read(1); const auto quantity = read(2); const auto packets = read(2);
						if (cargo >= NUM_CARGO || static_cast<int>(cargo) <= previous || quantity == 0 || quantity > GetMultiLayerSpaceBudget().stock_cap || packets == 0 || packets > 64 || packets * 6 > length - consumed) SlErrorCorrupt("Invalid orbital Core cargo slot");
						previous = static_cast<int>(cargo); m.quantities[cargo] = static_cast<uint16_t>(quantity);
						for (uint k = 0; k < packets; ++k) m.stock[cargo].push_back({CargoPacketID{static_cast<uint32_t>(read(4))}, StationID{static_cast<uint16_t>(read(2))}});
					}
					if (services) {
						m.producer = UtilityProducerID{static_cast<uint32_t>(read(4))}; m.consumer = UtilityConsumerID{static_cast<uint32_t>(read(4))};
						const auto enabled = read(1); m.services_completed = read(8); auto &job = m.service;
						job.craft = {VehicleID{static_cast<uint32_t>(read(4))}, read(8)}; job.remaining_ticks = static_cast<uint16_t>(read(2));
						const auto paused = read(1); job.last_work_interval = read(8); job.services_at_start = read(8);
						// Presence flags are strict booleans, rather than accepting malformed values through a cast.
						if (enabled > 1 || paused > 1) SlErrorCorrupt("Invalid orbital service flags");
						m.power_enabled = enabled != 0; job.paused = paused != 0;
						// Each active service label is exactly one part, so more than one packet cannot be a legitimate owner.
						for (uint i = 0; i < 2; ++i) {
							job.quantities[i] = static_cast<uint16_t>(read(2)); const auto packets = read(2);
							if (packets > 1) SlErrorCorrupt("Invalid orbital service packet count");
							for (uint k = 0; k < packets; ++k) job.inputs[i].push_back({CargoPacketID{static_cast<uint32_t>(read(4))}, StationID{static_cast<uint16_t>(read(2))}});
						}
					}
					if (milestones) m.assembly_consumer = UtilityConsumerID{static_cast<uint32_t>(read(4))};
				}
			} else if (s.kind != SpaceBuildKind::Core) {
				SlErrorCorrupt("Old orbital construction cannot contain later module kinds");
			}
			if (milestones) {
				r.work_fraction = static_cast<uint16_t>(read(2)); r.work_remainder = static_cast<uint16_t>(read(2));
				r.last_work_tick = read(8); r.last_work_interval = read(8);
			}
			t.rows.push_back(std::move(r));
		}
		if (consumed != length) SlErrorCorrupt("Trailing orbital construction data");
		return t;
	}
	/** Stage one detached family, refusing duplicate authoritative transport. */
	void Load() const override { if (candidate) SlErrorCorrupt("Repeated orbital construction family"); candidate = this->ReadRecords(); }
	/** Validate copied transport/profile before native packet-owner installation. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!candidate) { if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceConstructionJobs)) SlErrorCorrupt("Missing orbital construction family"); return; }
		const auto *schema = GetPendingMultiLayerStateSchema();
		// Nonempty orbital owners require the pending save's selected gameplay profile.
		if ((!candidate->rows.empty() || candidate->next != 1) && (schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Orbital construction requires selected profile");
		if (!ValidateSpaceStructureRecords(candidate->rows, candidate->next, Map::SizeX(), Map::SizeY(), TimerGameTick::counter)) SlErrorCorrupt("Invalid orbital construction receipt");
	}
	/** @param length Framework-admitted family length; the bounded decoder checks its exact contents again. */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_structures_present) SlErrorCorrupt("Repeated orbital construction preview");
		auto t = this->ReadRecords(); _load_check_data.space_structure_rows = std::move(t.rows); _load_check_data.next_space_structure_generation = t.next; _load_check_data.space_structures_present = true;
	}
};
static const SPSTChunkHandler SPST;
static const ChunkHandlerRef handlers[] = {SPST};
extern const ChunkHandlerTable _space_structure_chunk_handlers(handlers);
