/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure_sl.cpp Bounded construction and operational Core transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-8BC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10B; processor buffers require same-file configured cargo dimensions.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC; versioned immutable contained service roles and same-file native part quantities.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../space_structure.h"
#include "../space_vehicle.h"
#include "../space_mining_tuning.h"
#include "../orbital_service.h"
#include "../orbital_manufacturing.h"
#include "../orbital_recipe.h"
#include "space_replacement_sl.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../map_func.h"
#include "../timer/timer_game_tick.h"
#include <set>
#include "../safeguards.h"

/** Detached records; decoding/preview never publishes native inventory or activation. */
struct StructureTransport {
	uint64_t next = 1;
	SpaceStructureRetirementHistory history;
	std::vector<SpaceStructureRecord> rows;
};
static std::optional<StructureTransport> candidate;
/** @copydoc ResetSpaceStructureLoadStaging */
void ResetSpaceStructureLoadStaging() { candidate.reset(); }
/** @copydoc AcceptLoadedSpaceStructures */
bool AcceptLoadedSpaceStructures()
{
	if (!candidate) return IsSavegameVersionBefore(SaveLoadVersion::SpaceConstructionJobs);
	if (!InstallLoadedSpaceStructures(candidate->rows, candidate->next, candidate->history)) return false;
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
	if ((!rows.empty() || data.next_space_structure_generation != 1 || data.space_structure_retirement_history != SpaceStructureRetirementHistory{}) &&
		(data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || data.map_layers < 3)) SlErrorCorrupt("Orbital construction requires selected Space profile");
	// Assembly receipts compare against this file's copied clock, never the currently open world's opportunity.
	if (!rows.empty() && !data.native_tick_present) SlErrorCorrupt("Missing orbital assembly clock preview");
	if (!ValidateSpaceStructureRecords(rows, data.next_space_structure_generation, data.map_size_x, data.map_size_y, data.native_tick_counter, data.space_structure_retirement_history)) SlErrorCorrupt("Invalid orbital construction preview");
	if (!ValidateOrbitalServiceUtilityRecords(rows, data.utility_power_state)) SlErrorCorrupt("Invalid orbital service utility preview");
	if (!ValidateOrbitalManufacturingProductionLinks(rows, data.space_production_rows, data.next_space_structure_generation, data.next_space_vehicle_generation)) SlErrorCorrupt("Invalid orbital manufacture product preview");
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
			if (r.module->manufacture) {
				// A manufactured batch remains an exclusive copied owner, including heavy components that do not belong to service/blueprint slots.
				for (const auto &slot : r.module->manufacture->inputs) for (const auto &[id, hop] : slot) { (void)hop; if (!packets.insert(id).second) SlErrorCorrupt("Repeated orbital manufacture packet preview"); }
				// A pending job owns its planned berth. Completed products and later actual visitors use ordinary same-file native admission instead.
				for (const auto &order : data.space_order_preview_rows) if (order.queue.approach) {
					const auto &request = *order.queue.approach;
					if (request.target == SpaceOrderTarget{SpaceOrderTargetKind::Structure, r.ref.structure.base(), r.ref.generation} && request.slot != UINT8_MAX &&
						r.module->manufacture->phase != OrbitalManufacturePhase::Complete) SlErrorCorrupt("Occupied orbital manufacture berth preview");
				}
			}
			const auto jobs = r.module->GetServiceJobs();
			// Both contained berths participate in same-file physical ownership; no second-job reference may hide from preview auditing.
			for (size_t berth = 0; berth < jobs.size(); ++berth) {
				const auto &job = *jobs[berth];
				// Service reservations join cross-family exclusivity before a detached preview can grant valid load status.
				for (const auto &slot : job.inputs) for (const auto &[id, hop] : slot) { (void)hop; if (!packets.insert(id).second) SlErrorCorrupt("Repeated orbital service packet preview"); }
				// Sources with detached native CAPA metadata must fund every claimed reserved unit from those exact same-file packet IDs; an open world's stock or a cached quantity cannot fill a missing part.
				if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerDescents)) {
					if (!data.native_cargo_packets_present) SlErrorCorrupt("Missing contained service packet quantities preview");
					const auto &native_packets = data.native_cargo_packet_rows;
					for (uint i = 0; i < 2; ++i) {
						uint64_t quantity = 0;
						for (const auto &[id, hop] : job.inputs[i]) {
							(void)hop; const auto packet = std::ranges::lower_bound(native_packets, id, {}, &NativeCargoPacketPreview::packet);
							// Only this original same-file native packet may finance its sole contained owner; a missing or different pool ID grants no work.
							if (packet == native_packets.end() || packet->packet != id) SlErrorCorrupt("Missing contained service packet preview");
							quantity += packet->quantity;
						}
						if (quantity != job.quantities[i]) SlErrorCorrupt("Invalid contained service packet quantity preview");
					}
				}
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
					if (live && (production == data.space_production_rows.end() || !IsSupportedSpaceVehicleContentVersion(production->state.content_version, production->state.model) ||
						production->state.placement != SpacePlacement::Orbital || !GetSpaceServiceTuning(production->state.model) ||
						(job.model == UINT16_MAX ? production->state.model == SPACE_HEAVY_MINER_MODEL : production->state.model != job.model))) SlErrorCorrupt("Invalid orbital service probe role preview");
					if (live) {
						const auto model = production->state.model;
						// The heavy's current contained producer requires exhausted allowance, no pending descent and actual holding pose. Genuine older role-unknown jobs cannot borrow this capability.
						if (model == SPACE_HEAVY_MINER_MODEL && (IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerContainedServices) || manifest->returns_remaining != 0 || manifest->pending_returns != 0 || craft->mining_phase != to_underlying(SpaceMissionPhase::Holding))) SlErrorCorrupt("Invalid heavy contained service state preview");
						auto point = GetSpaceStructureApproach(r.state);
						// Both heavy Core jobs and every Dock job retain their original precise shared berth; historical Core jobs preserve their older coordinate-only admission.
						if (r.state.kind == SpaceBuildKind::Dock || model == SPACE_HEAVY_MINER_MODEL) {
							point.x += static_cast<int32_t>(berth) * (SPACE_Q16 / 2);
							/** @param row Same-file native queue. @return Whether it belongs to this exact retained service craft. */
							const auto queue = std::ranges::find_if(data.space_order_preview_rows, [&](const auto &row) { return row.vehicle == job.craft.vehicle && row.generation == job.craft.generation; });
							// A current heavy Core or Dock job must retain its own exact admitted physical slot, including while power is paused; legacy Core jobs need no new implicit reservation.
							if (queue == data.space_order_preview_rows.end() || !queue->queue.approach || queue->queue.approach->target != SpaceOrderTarget{SpaceOrderTargetKind::Structure, r.ref.structure.base(), r.ref.generation} ||
								queue->queue.approach->slot != berth) SlErrorCorrupt("Invalid orbital Dock berth preview");
						}
						if (craft->position != point) SlErrorCorrupt("Invalid orbital service craft position preview");
					}
				}
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
/** Native SPST family:415 jobs,416 stock,417 service,418 exact work,419 extension activation and421 connected Dock jobs. */
struct SPSTChunkHandler : ChunkHandler {
	/** Register the single bounded native structure family. */
	SPSTChunkHandler() : ChunkHandler("SPST", ChunkType::Riff) {}
	/** @param job Contained service record whose state and real packet references are written in the canonical existing field order. */
	static void WriteService(const OrbitalServiceRecord &job)
	{
		Write(job.craft.vehicle.base(), 4); Write(job.craft.generation, 8); Write(job.remaining_ticks, 2); Write(job.paused, 1);
		Write(job.last_work_interval, 8); Write(job.services_at_start, 8);
		// Each reserved label retains exact packet IDs and routing, rather than replacing parts with aggregate quantities.
		for (uint i = 0; i < 2; ++i) {
			Write(job.quantities[i], 2); Write(job.inputs[i].size(), 2);
			for (const auto &[packet, hop] : job.inputs[i]) { Write(packet.base(), 4); Write(hop.base(), 2); }
		}
		Write(job.model, 2); // Append436 after the complete older job, preserving original fields and unknown legacy role without inference.
	}
	/** Serialize validated native owners, preserving exact packet identity and activation countdown. */
	void Save() const override
	{
		if (!ValidateSpaceStructures()) SlErrorCorrupt("Invalid orbital construction on save");
		const auto rows = CaptureSpaceStructures();
		// Validate copied clone/source/product epochs and bounded text before deriving lengths or writing presence bytes; serialization cannot publish malformed retained intent.
		if (!ValidateOrbitalManufacturingProductionLinks(rows, CaptureSpaceProduction(), GetNextSpaceStructureGeneration(), GetNextSpaceVehicleGeneration())) SlErrorCorrupt("Invalid orbital manufacture intent on save");
		size_t length = 42 + 62 * rows.size();
		// Exact transport length includes every retained packet reference, never cargo quantity as a packet surrogate.
		for (const auto &r : rows) for (const auto &slot : r.inputs) length += 6 * slot.size();
		for (const auto &r : rows) if (r.module) {
			length += 5 + 56 + 4 + 10 + 39 + 9 + 4; // Two contained jobs each append their original immutable role in436.
			if (r.module->manufacture) {
				length += 90; // Original50-byte reservation plus427 paid work, completion identity and four-label consumed receipt.
				for (const auto &slot : r.module->manufacture->inputs) length += 6 * slot.size();
				const auto &clone = r.module->manufacture->clone;
				length += 1; // Append428 records absence explicitly; ordinary purchases retain no inferred clone intent.
				if (clone) length += 14 + clone->name.size() + 14 * clone->orders.size();
				length += SpaceReplacementTransportLength(r.module->manufacture->replacement);
			}
			for (const auto &slot : r.module->stock) if (!slot.empty()) length += 5 + 6 * slot.size();
			for (const auto *job : r.module->GetServiceJobs()) for (const auto &slot : job->inputs) length += 6 * slot.size();
		}
		SlSetLength(length); Write(GetNextSpaceStructureGeneration(), 8); Write(rows.size(), 2);
		// Append422's bounded aggregates to the family header; no deleted native object or packet pool is reconstructed from spent receipts.
		const auto &history = GetSpaceStructureRetirementHistory();
		Write(history.structures_removed, 8); Write(history.consumed[0], 8); Write(history.consumed[1], 8); Write(history.services_completed, 8);
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
				WriteService(m.service);
				Write(m.assembly_consumer.base(), 4); // Append418 after the unchanged417 contained-service payload.
				// Append421 preserves the complete older module prefix; old modules acquire neither a source connection nor a second job.
				Write(m.utility_source.structure.base(), 2); Write(m.utility_source.generation, 8);
				WriteService(m.dock_service);
				// Append426 after the complete older facility payload; old saves acquire neither a request nor a new order serial.
				Write(m.manufacture_serial, 8); Write(m.manufacture.has_value(), 1);
				if (m.manufacture) {
					const auto &job = *m.manufacture;
					Write(to_underlying(job.phase), 1); Write(job.model, 2); Write(job.serial, 8);
					Write(job.source.structure.base(), 2); Write(job.source.generation, 8); Write(job.capital_paid, 4); Write(job.berth, 1); Write(job.reservation_interval, 8);
					// Every declared heavy/common input retains its own exact packet references; reserved quantities never become manufactured cargo on load.
					for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) {
						Write(job.quantities[input], 2); Write(job.inputs[input].size(), 2);
						for (const auto &[packet, hop] : job.inputs[input]) { Write(packet.base(), 4); Write(hop.base(), 2); }
					}
					// Append427 only after the complete426 packet owner; old reservations acquire zero work, consumption or product.
					Write(job.work_ticks, 4); Write(job.last_work_interval, 8); Write(job.work_energy_j, 8);
					Write(job.product.vehicle.base(), 4); Write(job.product.generation, 8);
					for (const auto quantity : job.consumed) Write(quantity, 2);
					Write(job.clone.has_value(), 1);
					if (job.clone) {
						const auto &clone = *job.clone;
						Write(clone.source.vehicle.base(), 4); Write(clone.source.generation, 8);
						Write(clone.name.size(), 1);
						// Serialize bounded captured text/instructions only: no source cargo, mutable action progress or shared queue identity can enter this owner.
						for (const auto byte : clone.name) Write(static_cast<uint8_t>(byte), 1);
						Write(clone.orders.size(), 1);
						for (const auto &order : clone.orders) { Write(to_underlying(order.action), 1); Write(to_underlying(order.target.kind), 1); Write(order.target.id, 4); Write(order.target.generation, 8); }
					}
					WriteSpaceReplacementTransport(job.replacement, Write);
				}
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
		const bool docks = !IsSavegameVersionBefore(SaveLoadVersion::OrbitalDockServices);
		const bool retired = !IsSavegameVersionBefore(SaveLoadVersion::OrbitalModuleRetirement);
		const bool manufacturing = !IsSavegameVersionBefore(SaveLoadVersion::OrbitalManufacturingReservations);
		const bool heavy_service = !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerContainedServices);
		const size_t header_length = retired ? 42 : 10;
		const size_t row_limit = 47 + 56 + 12 + 24 + 6 * 128 + NUM_CARGO * (5 + 6 * 64) + (docks ? 49 + 12 : 0) + (heavy_service ? 4 + 24 : 0) +
			(manufacturing ? 9 + 50 + (IsSavegameVersionBefore(SaveLoadVersion::OrbitalManufacturingMaterialization) ? 0 : 40) +
				(IsSavegameVersionBefore(SaveLoadVersion::OrbitalManufacturingClones) ? 0 : 15 + MAX_SPACE_CLONE_NAME_BYTES + 14 * MAX_SPACE_ORDERS) +
				(IsSavegameVersionBefore(SaveLoadVersion::SpaceReplacementSettlement) ? 0 : 21) + 6 * 64 * MAX_ORBITAL_MANUFACTURE_INPUTS : 0);
		// Bound the whole family before allocating records; older saves cannot smuggle newer facility payloads.
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceConstructionJobs) || length < header_length || length > header_length + SpaceStructureID::End().base() * row_limit) SlErrorCorrupt("Invalid orbital construction length");
		size_t consumed = 0;
		/** @param width Requested integer width. @return Value only after admitting its bytes within this family. */
		auto read = [&](uint width) { if (width > length - consumed) SlErrorCorrupt("Truncated orbital construction data"); consumed += width; return Read(width); };
		/** @param job Receives one bounded detached service job; no packet, craft or utility owner is resolved while decoding. */
		auto read_service = [&](OrbitalServiceRecord &job) {
			job.craft = {VehicleID{static_cast<uint32_t>(read(4))}, read(8)}; job.remaining_ticks = static_cast<uint16_t>(read(2));
			const auto paused = read(1); job.last_work_interval = read(8); job.services_at_start = read(8);
			if (paused > 1) SlErrorCorrupt("Invalid orbital service flags");
			job.paused = paused != 0;
			// Source-era bounds admit at most two real references per heavy input, while genuine older service keeps its original one-reference cap. Actual role/bill and native quantities validate after the complete row.
			for (uint i = 0; i < 2; ++i) {
				job.quantities[i] = static_cast<uint16_t>(read(2)); const auto packets = read(2);
				if (packets > (heavy_service ? 2 : 1)) SlErrorCorrupt("Invalid orbital service packet count");
				for (uint k = 0; k < packets; ++k) job.inputs[i].push_back({CargoPacketID{static_cast<uint32_t>(read(4))}, StationID{static_cast<uint16_t>(read(2))}});
			}
			// Genuine old jobs retain unknown instead of borrowing a surviving hull's current model; that conservative role and the complete original paid fields survive the first ordinary upgrade resave.
			if (heavy_service) job.model = static_cast<uint16_t>(read(2));
		};
		StructureTransport t; t.next = read(8); const auto count = read(2);
		if (retired) {
			// Old saves receive zero aggregates; current history is bounded together with the same file's issuance and live owners below.
			t.history.structures_removed = read(8); t.history.consumed[0] = read(8); t.history.consumed[1] = read(8); t.history.services_completed = read(8);
		}
		// Every native row has a mandatory construction prefix and, from416, one facility-presence byte.
		if (count > SpaceStructureID::End().base() || consumed + count * ((facilities ? 42 : 41) + (milestones ? 20 : 0)) > length) SlErrorCorrupt("Invalid orbital construction count");
		// Decode bounded native rows without allocating pools or resolving any live packet owner.
		for (uint j = 0; j < count; ++j) {
			SpaceStructureRecord r; r.ref = {SpaceStructureID{static_cast<uint16_t>(read(2))}, read(8)}; auto &s = r.state;
			s.owner = Owner{static_cast<uint8_t>(read(1))}; s.kind = static_cast<SpaceBuildKind>(read(1)); s.phase = static_cast<SpaceStructurePhase>(read(1));
			s.x = static_cast<uint16_t>(read(2)); s.y = static_cast<uint16_t>(read(2)); s.content_version = static_cast<uint16_t>(read(2)); s.capital_paid = static_cast<uint32_t>(read(4));
			// New content cannot be smuggled into an older SDEF2 file by editing only its global identity; genuine older construction receipts remain exact2.
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerCommissioning) && s.content_version > MULTILAYER_SPACE_LEGACY_CONTENT_VERSION) SlErrorCorrupt("Future orbital construction provenance in old save");
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
					// Older binaries only supported contained Core facilities; an older save cannot smuggle newly usable extension functions.
					if (IsSavegameVersionBefore(SaveLoadVersion::OrbitalModuleActivation) && s.kind != SpaceBuildKind::Core) SlErrorCorrupt("Invalid orbital extension activation version");
					if (!docks && s.kind == SpaceBuildKind::Dock) SlErrorCorrupt("Invalid orbital Dock activation version");
					// Passive processor buffers need this file's configured cargo meanings; earlier saves cannot supply the independently bounded role metadata.
					if (IsOrbitalRecipeKind(s.kind) && IsSavegameVersionBefore(SaveLoadVersion::SpaceCargoDimensions)) SlErrorCorrupt("Unsupported orbital processor buffer version");
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
						const auto enabled = read(1); m.services_completed = read(8);
						// Presence flags are strict booleans, rather than accepting malformed values through a cast.
						if (enabled > 1) SlErrorCorrupt("Invalid orbital service flags");
						m.power_enabled = enabled != 0; read_service(m.service);
					}
					if (milestones) m.assembly_consumer = UtilityConsumerID{static_cast<uint32_t>(read(4))};
					if (docks) {
						m.utility_source = {SpaceStructureID{static_cast<uint16_t>(read(2))}, read(8)};
						read_service(m.dock_service);
					}
					// Earlier runtimes could retain an inert Assembly frame but could never activate it or issue its work connection.
					if (s.kind == SpaceBuildKind::Assembly && IsSavegameVersionBefore(SaveLoadVersion::OrbitalAssemblyFacilities)) SlErrorCorrupt("Invalid orbital Assembly facility version");
					if (manufacturing) {
						m.manufacture_serial = read(8); const auto present_job = read(1);
						if (present_job > 1) SlErrorCorrupt("Invalid orbital manufacture presence");
						if (present_job != 0) {
							auto &job = m.manufacture.emplace(); job.phase = static_cast<OrbitalManufacturePhase>(read(1)); job.model = static_cast<uint16_t>(read(2)); job.serial = read(8);
							// Before431 no paid heavy-miner request existed, even while still queued without a hull receipt. Both native load and copied preview reject that future producer identity before publishing work or inputs.
							if (IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerCommissioning) && job.model >= 21) SlErrorCorrupt("Future orbital manufacture model in old save");
							job.source = {SpaceStructureID{static_cast<uint16_t>(read(2))}, read(8)}; job.capital_paid = static_cast<uint32_t>(read(4));
							job.berth = static_cast<uint8_t>(read(1)); job.reservation_interval = read(8);
							// Bound all four packet lists before allocating copied vectors; model/quantity/source semantics validate together after complete decoding.
							for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) {
								job.quantities[input] = static_cast<uint16_t>(read(2)); const auto packets = read(2);
								if (packets > 64 || packets * 6 > length - consumed) SlErrorCorrupt("Invalid orbital manufacture packet count");
								for (uint packet = 0; packet < packets; ++packet) job.inputs[input].push_back({CargoPacketID{static_cast<uint32_t>(read(4))}, StationID{static_cast<uint16_t>(read(2))}});
							}
							if (!IsSavegameVersionBefore(SaveLoadVersion::OrbitalManufacturingMaterialization)) {
								job.work_ticks = static_cast<uint32_t>(read(4)); job.last_work_interval = read(8); job.work_energy_j = read(8);
								job.product = {VehicleID{static_cast<uint32_t>(read(4))}, read(8)};
								for (auto &quantity : job.consumed) quantity = static_cast<uint16_t>(read(2));
							}
							if (!IsSavegameVersionBefore(SaveLoadVersion::OrbitalManufacturingClones)) {
								const auto present = read(1);
								if (present > 1) SlErrorCorrupt("Invalid orbital clone presence");
								if (present != 0) {
									auto &clone = job.clone.emplace();
									clone.source = {VehicleID{static_cast<uint32_t>(read(4))}, read(8)};
									const auto name_length = read(1);
									if (name_length > MAX_SPACE_CLONE_NAME_BYTES) SlErrorCorrupt("Invalid orbital clone name length");
									clone.name.reserve(name_length);
									// Admit byte/count bounds before allocating copied containers; full/preview semantics then validate UTF-8, roles and same-file issuance together.
									for (uint byte = 0; byte < name_length; ++byte) clone.name.push_back(static_cast<char>(read(1)));
									const auto orders = read(1);
									if (orders > MAX_SPACE_ORDERS) SlErrorCorrupt("Invalid orbital clone order count");
									for (uint order = 0; order < orders; ++order) clone.orders.push_back({static_cast<SpaceOrderAction>(read(1)), {static_cast<SpaceOrderTargetKind>(read(1)), static_cast<uint32_t>(read(4)), read(8)}});
								}
							}
							if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceReplacementSettlement)) job.replacement = ReadSpaceReplacementTransport(read);
						}
					}
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
		if ((!candidate->rows.empty() || candidate->next != 1 || candidate->history != SpaceStructureRetirementHistory{}) &&
			(schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Orbital construction requires selected profile");
		if (!ValidateSpaceStructureRecords(candidate->rows, candidate->next, Map::SizeX(), Map::SizeY(), TimerGameTick::counter, candidate->history)) SlErrorCorrupt("Invalid orbital construction receipt");
	}
	/** @param length Framework-admitted family length; the bounded decoder checks its exact contents again. */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_structures_present) SlErrorCorrupt("Repeated orbital construction preview");
		auto t = this->ReadRecords(); _load_check_data.space_structure_rows = std::move(t.rows); _load_check_data.next_space_structure_generation = t.next;
		_load_check_data.space_structure_retirement_history = t.history; _load_check_data.space_structures_present = true;
	}
};
static const SPSTChunkHandler SPST;
static const ChunkHandlerRef handlers[] = {SPST};
extern const ChunkHandlerTable _space_structure_chunk_handlers(handlers);
