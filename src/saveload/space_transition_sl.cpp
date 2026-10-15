/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_transition_sl.cpp Saved native rocket round trips and independent preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups10B/11.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../space_transition.h"
#include "../space_vehicle.h"
#include "../launch_operations.h"
#include "../space_structure.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../safeguards.h"

static std::optional<std::vector<SpaceTransitionRecord>> candidate;
/** @copydoc ResetSpaceTransitionLoadStaging */
void ResetSpaceTransitionLoadStaging() { candidate.reset(); }
/** @copydoc AcceptLoadedSpaceTransitions */
bool AcceptLoadedSpaceTransitions()
{
	if (!candidate) return IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics);
	if (!InstallLoadedSpaceTransitions(*candidate)) return false;
	candidate.reset(); return true;
}
/** @copydoc ValidateSpaceTransitionLoadPreview */
void ValidateSpaceTransitionLoadPreview()
{
	if (!_load_check_data.space_transitions_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing native Space transition family");
		return;
	}
	const auto &records = _load_check_data.space_transition_rows;
	// Same-file profile/geometry, bounded phase receipts and original pad reservations must all agree before a copied preview can grant flight authority.
	if ((!records.empty() && (_load_check_data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || _load_check_data.map_layers < 3)) ||
		!ValidateSpaceTransitionRecords(records, _load_check_data.map_size_x, _load_check_data.map_size_y) || !ValidateSpaceTransitionLinks(records, _load_check_data.space_production_rows, _load_check_data.launch_operations_rows)) SlErrorCorrupt("Invalid native Space transition preview");
	// Resolve each freight role only against this snapshot's original hull, manifest, issued target and paid module rows; preview never borrows active-world inventory.
	for (const auto &r : records) {
		const auto &f = r.state;
		// Only freight roles require destination inventory checks; ordinary passenger deployment retains its separate manifest validation.
		if (f.destination.kind != SpaceOrderTargetKind::Craft && f.destination.kind != SpaceOrderTargetKind::Structure) continue;
		const auto &native = _load_check_data.native_space_vehicle_rows;
		/** @param v Same-file native hull. @return Whether its identity names this freight sender. */
		const auto sender = std::find_if(native.begin(), native.end(), [&](const auto &v) { return v.vehicle == r.craft.vehicle; });
		if (sender == native.end()) SlErrorCorrupt("Missing native Space freight sender preview");
		/** @param m Same-file manifest. @return Whether both original hull identity components agree. */
		const auto manifest = std::find_if(_load_check_data.space_manifest_rows.begin(), _load_check_data.space_manifest_rows.end(), [&](const auto &m) { return m.craft == r.craft; });
		// A direct freight sender cannot also promise a carried passenger; successful receipts require every actual deliverable compartment to be empty.
		if (manifest == _load_check_data.space_manifest_rows.end() || manifest->passenger != SpaceCraftRef{}) SlErrorCorrupt("Invalid native Space freight passenger preview");
		// A successful whole delivery cannot coexist with retained cargo/pods/service parts; rejecting this receipt prevents a second restored unload.
		if ((f.resupplied || f.module_delivered) && (!sender->cargo_packet_refs.empty() || !manifest->pods.empty() || !manifest->supplies[1].empty() || !manifest->supplies[2].empty())) SlErrorCorrupt("Repeated native Space freight payload preview");
		const bool unresolved = f.phase == SpaceFlightPhase::Preparing || f.phase == SpaceFlightPhase::Ascent || f.phase == SpaceFlightPhase::OrbitalDelivery;
		if (f.destination.kind == SpaceOrderTargetKind::Structure) {
			// An issued module incarnation is separate from station/hull identity; pods remain their original paid shell/cargo owner instead of silently becoming station freight.
			if (f.destination.generation >= _load_check_data.next_space_structure_generation || !manifest->pods.empty()) SlErrorCorrupt("Invalid native Space module freight identity preview");
			const auto &sites = _load_check_data.space_structure_rows;
			/** @param site Same-file paid structure. @return Whether both issued identity components name this module destination. */
			const auto site = std::ranges::find_if(sites, [&](const auto &site) { return site.ref.structure.base() == f.destination.id && site.ref.generation == f.destination.generation; });
			if (unresolved) {
				// Removed targets are explicitly lost already for paused saving. Dependency outages may retain a paid empty module without creating any new operation or stock.
				if (site == sites.end() && f.blocked != SpaceFlightBlock::TargetLost) SlErrorCorrupt("Missing native Space module freight target preview");
				// A surviving target must retain its actual completed/activated stock role and company; a paid blueprint or foreign module grants no delivery authority.
				if (site != sites.end() && (site->state.phase != SpaceStructurePhase::FrameComplete || !site->module || site->module->activation_ticks != 0 || GetOrbitalModuleStockLimit(site->state.kind) == 0 ||
					(f.blocked != SpaceFlightBlock::TargetLost && site->state.owner != sender->owner))) SlErrorCorrupt("Invalid owned native Space module freight target preview");
			}
		} else {
			if (f.destination.generation >= _load_check_data.next_space_vehicle_generation) SlErrorCorrupt("Invalid future Space resupply target preview");
			const SpaceCraftRef target{VehicleID{f.destination.id}, f.destination.generation};
			const auto identity = std::find(_load_check_data.space_order_preview_refs.begin(), _load_check_data.space_order_preview_refs.end(), target);
			// Retained lost history grants no operation; a surviving active resupply receiver must still belong to the exact sender fleet.
			if (identity != _load_check_data.space_order_preview_refs.end() && f.blocked != SpaceFlightBlock::TargetLost && unresolved) {
				/** @param v Same-file native hull. @return Whether it is the explicit original resupply receiver. */
				const auto receiver = std::find_if(native.begin(), native.end(), [&](const auto &v) { return v.vehicle == target.vehicle; });
				// A retained valid target incarnation cannot authorize another company's physical receiver.
				if (receiver == native.end() || receiver->owner != sender->owner) SlErrorCorrupt("Foreign native Space resupply receiver preview");
			}
		}
	}
}
/** @param value Canonical field value. @param width Big-endian byte width, bounded by the unchanged84-byte flight row. */
static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
/** @param width Canonical big-endian byte width. @return Detached integer; decoding never creates a native physical owner. */
static uint64_t Read(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
/** @param ref Exact physical Surface facility incarnation; identity is distinct from a module destination. */
static void WriteCenter(LaunchCenterRef ref) { Write(ref.station.base(), 2); Write(ref.generation, 8); }
/** @return Detached physical Surface facility reference, including its issued generation. */
static LaunchCenterRef ReadCenter() { return {StationID{static_cast<uint16_t>(Read(2))}, Read(8)}; }
/** Existing native flight family owns only detached rows; restoration reattaches them to the original paid Vehicle. */
struct SPFLChunkHandler : ChunkHandler {
	/** Register the existing84-byte native flight transport;423 adds explicit role/receipt semantics without another family. */
	SPFLChunkHandler() : ChunkHandler("SPFL", ChunkType::Riff) {}
	/** Save original hull identity, phase, packet-delivery receipts and mission history only after full physical validation. */
	void Save() const override
	{
		if (!ValidateSpaceTransitions()) SlErrorCorrupt("Invalid native Space transition ownership on save");
		const auto records = CaptureSpaceTransitions(); SlSetLength(1 + 84 * records.size()); Write(records.size(), 1);
		// Stable native pool order preserves exact replay while each row keeps its original rocket and role-specific once-only receipt.
		for (const auto &r : records) {
			Write(r.craft.vehicle.base(), 4); Write(r.craft.generation, 8); const auto &f = r.state;
			Write(to_underlying(f.phase), 1); Write(to_underlying(f.blocked), 1); Write(to_underlying(f.layer), 1); WriteCenter(f.source); WriteCenter(f.recovery);
			Write(to_underlying(f.destination.kind), 1); Write(f.destination.id, 4); Write(f.destination.generation, 8);
			Write(static_cast<uint32_t>(f.departure.x), 4); Write(static_cast<uint32_t>(f.departure.y), 4); Write(f.remaining_ticks, 2); Write(f.pad_cost_remainder, 1);
			Write(unsigned(f.outbound_burned) | (unsigned(f.recovery_burned) << 1) | (unsigned(f.deployed) << 2) | (unsigned(f.resupplied) << 3) | (unsigned(f.module_delivered) << 4), 1);
			Write(f.missions_started, 8); Write(f.missions_completed, 8); Write(f.missions_canceled, 8);
		}
	}
	/** @return Fully bounded detached transport with explicit old-schema domain/flag refusals; this decoder creates no hull or module. */
	std::vector<SpaceTransitionRecord> ReadRecords() const
	{
		const auto length = SlGetFieldLength();
		// A flight family requires its historical introduction and exact bounded framing before any row can influence native state.
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) || length < 1 || length > 1 + 84 * MAX_SPACE_VEHICLES) SlErrorCorrupt("Invalid native Space transition length");
		// Declared count and payload length must agree; truncated or extra rows cannot smuggle a second ticking flight.
		const auto count = Read(1); if (count > MAX_SPACE_VEHICLES || length != 1 + 84 * count) SlErrorCorrupt("Invalid native Space transition count");
		std::vector<SpaceTransitionRecord> records;
		// Decode all physical identity and receipt fields before installing any owner; version checks preserve the historical meaning of each role bit.
		for (uint i = 0; i < count; ++i) {
			SpaceTransitionRecord r; r.craft = {VehicleID{static_cast<uint32_t>(Read(4))}, Read(8)}; auto &f = r.state;
			f.phase = static_cast<SpaceFlightPhase>(Read(1)); f.blocked = static_cast<SpaceFlightBlock>(Read(1)); f.layer = static_cast<MapLayer>(Read(1)); f.source = ReadCenter(); f.recovery = ReadCenter();
			f.destination = {static_cast<SpaceOrderTargetKind>(Read(1)), static_cast<uint32_t>(Read(4)), Read(8)};
			// Unchanged84-byte framing gains module semantics only in423; older files cannot gain a facility through a new domain byte.
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalModuleFreight) && f.destination.kind == SpaceOrderTargetKind::Structure) SlErrorCorrupt("Unexpected module freight in old Space transition");
			// Older deployment-only saves cannot acquire craft resupply or its later resumable capacity waits.
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply) && (f.destination.kind == SpaceOrderTargetKind::Craft || f.blocked >= SpaceFlightBlock::ReceiverBusy)) SlErrorCorrupt("Unexpected resupply in old Space transition");
			f.departure = {static_cast<int32_t>(Read(4)), static_cast<int32_t>(Read(4))}; f.remaining_ticks = static_cast<uint16_t>(Read(2)); f.pad_cost_remainder = static_cast<uint8_t>(Read(1));
			const auto flags = Read(1); if (flags > (IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply) ? 7 : IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalModuleFreight) ? 15 : 31)) SlErrorCorrupt("Invalid native Space transition flags");
			f.outbound_burned = (flags & 1) != 0; f.recovery_burned = (flags & 2) != 0; f.deployed = (flags & 4) != 0;
			f.resupplied = (flags & 8) != 0;
			f.module_delivered = (flags & 16) != 0;
			f.missions_started = Read(8); f.missions_completed = Read(8); f.missions_canceled = Read(8); records.push_back(r);
		}
		if (!ValidateSpaceTransitionRecords(records)) SlErrorCorrupt("Invalid native Space transition transport");
		return records;
	}
	/** Stage one complete detached family; repeated transport cannot overwrite original flight history. */
	void Load() const override { if (candidate) SlErrorCorrupt("Repeated native Space transition family"); candidate = this->ReadRecords(); }
	/** Bind only after the normal pointer-fixing/profile phase; unresolved staged rows remain non-ticking transport. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		// Files at/after flight introduction must supply the family, even when no live rocket exists.
		if (!candidate && !IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing native Space transition family");
		const auto *schema = GetPendingMultiLayerStateSchema();
		// A populated flight owner requires the same-file Space profile; Classic cannot inherit paid Space operations from another active world.
		if (candidate && !candidate->empty() && (schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Space transitions require selected profile");
	}
	/** @param length Native family payload length; ReadRecords validates the actual bounded framing. Copy only same-file authority for independent preview. */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_transitions_present) SlErrorCorrupt("Repeated native Space transition preview");
		_load_check_data.space_transition_rows = this->ReadRecords(); _load_check_data.space_transitions_present = true;
	}
};
static const SPFLChunkHandler SPFL;
static const ChunkHandlerRef handlers[] = {SPFL};
extern const ChunkHandlerTable _space_transition_chunk_handlers(handlers);
