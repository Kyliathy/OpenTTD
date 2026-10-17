/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_production_sl.cpp Paid Earth/orbital native hull receipts and original production slots.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups2B/8B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups8/8BC
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "space_replacement_sl.h"
#include "../launch_operations.h"
#include "../space_vehicle.h"
#include "../multilayer_space_content.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../map_func.h"
#include "../safeguards.h"

static std::optional<std::vector<SpaceProductionRecord>> _production;
static std::optional<std::vector<LaunchOperationsRecord>> _operations;
/** @copydoc ResetSpaceProductionLoadStaging */
void ResetSpaceProductionLoadStaging() { _production.reset(); _operations.reset(); }

/** @copydoc AcceptLoadedSpaceProduction */
bool AcceptLoadedSpaceProduction()
{
	if (!_production || !_operations) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchAssembly) || _production || _operations) return false;
		// Absence upgrades transport only: no paid hulls, materials, work or operational reservations.
		return InstallLoadedSpaceProduction(CaptureSpaceProduction()) && InstallLoadedLaunchOperations(CaptureLaunchOperations());
	}
	const bool accepted = InstallLoadedSpaceProduction(*_production) && InstallLoadedLaunchOperations(*_operations);
	if (accepted) ResetSpaceProductionLoadStaging();
	return accepted;
}
/** @copydoc ValidateSpaceProductionLoadPreview */
void ValidateSpaceProductionLoadPreview()
{
	if (!_load_check_data.space_production_present || !_load_check_data.launch_operations_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchAssembly) || _load_check_data.space_production_present || _load_check_data.launch_operations_present) SlErrorCorrupt("Missing launch assembly family");
		return;
	}
	if ((!_load_check_data.space_production_rows.empty() || !_load_check_data.launch_operations_rows.empty()) &&
		(_load_check_data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || _load_check_data.map_layers < 3)) SlErrorCorrupt("Launch assembly preview requires selected Space geometry");
	if (!ValidateSpaceProductionRecords(_load_check_data.space_production_rows, _load_check_data.map_size_x, _load_check_data.map_size_y) ||
		!ValidateLaunchProductionLinks(_load_check_data.space_production_rows, _load_check_data.launch_operations_rows,
			_load_check_data.space_order_preview_refs, _load_check_data.launch_center_preview_refs)) SlErrorCorrupt("Invalid launch assembly preview ownership");
}
/** @param value Canonical bounded integer. @param width Exact big-endian byte width. */
static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
/** @param width Exact width admitted by native framing. @return Decoded unsigned integer. */
static uint64_t Read(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
/** @param ref Original native hull incarnation. */
static void WriteCraft(SpaceCraftRef ref) { Write(ref.vehicle.base(), 4); Write(ref.generation, 8); }
/** @return Copied native hull incarnation without allocating a vehicle. */
static SpaceCraftRef ReadCraft() { return {VehicleID{static_cast<uint32_t>(Read(4))}, Read(8)}; }
/** @param ref Original native Surface facility incarnation. */
static void WriteCenter(LaunchCenterRef ref) { Write(ref.station.base(), 2); Write(ref.generation, 8); }
/** @return Copied original Surface facility incarnation. */
static LaunchCenterRef ReadCenter() { return {StationID{static_cast<uint16_t>(Read(2))}, Read(8)}; }

/** Bounded paid native commissioning transport; copying restores existing hull receipts without executing either producer. */
struct SVSTChunkHandler : ChunkHandler {
	/** Register the sole paid hull receipt family. */
	SVSTChunkHandler() : ChunkHandler("SVST", ChunkType::Riff) {}
	/** Serialize complete validated existing owners; this grants no materials, work, slots or hulls. */
	void Save() const override
	{
		if (!ValidateSpaceProduction()) SlErrorCorrupt("Invalid launch assembly ownership on save");
		const auto records = CaptureSpaceProduction(); size_t length = 1;
		// Each actual hull retains its own optional retirement receipt; ordinary products carry an explicit absence byte, never an inferred source.
		for (const auto &r : records) length += 67 + SpaceReplacementTransportLength(r.state.replacement);
		SlSetLength(length); Write(records.size(), 1);
		// Serialize the unchanged original paid receipt followed by its bounded429 extension; no source assets or extra identity are reconstructed here.
		for (const auto &r : records) {
			WriteCraft(r.craft); const auto &s = r.state; Write(to_underlying(s.placement), 1); WriteCenter(s.home); WriteCenter(s.origin);
			Write(s.surface_tile.base(), 4); Write(s.remaining_ticks, 2); Write(s.content_version, 2); Write(s.model, 2); Write(s.capital_paid, 4); Write(s.paused, 1); Write(s.deployed, 1);
			// Append427 preserves the full original Earth receipt; orbital products carry their actual factory/job rather than an invented launch station.
			Write(s.assembly.structure.base(), 2); Write(s.assembly.generation, 8); Write(s.assembly_serial, 8);
			WriteSpaceReplacementTransport(s.replacement, Write);
		}
	}
	/** @return Detached exact-width paid receipts; native framing bounds every field before publication. */
	std::vector<SpaceProductionRecord> ReadRecords() const
	{
		const auto length = SlGetFieldLength();
		const bool replacement = !IsSavegameVersionBefore(SaveLoadVersion::SpaceReplacementSettlement);
		const size_t row_length = !IsSavegameVersionBefore(SaveLoadVersion::OrbitalManufacturingMaterialization) ? 67 :
			(IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) ? 48 : 49);
		const size_t minimum_row = row_length + (replacement ? 1 : 0), maximum_row = row_length + (replacement ? 21 : 0);
		// Admit the whole versioned family before decoding any receipt; truncated rows or a newer layout disguised as an older save cannot grant paid hull ownership.
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchAssembly) || length < 1 || length > 1 + maximum_row * MAX_SPACE_VEHICLES) SlErrorCorrupt("Invalid launch assembly length");
		size_t consumed = 0;
		/** @param width Exact integer extent. @return Detached value after admitting all its bytes within this one family. */
		const auto read = [&](uint width) { if (width > length - consumed) SlErrorCorrupt("Truncated launch assembly data"); consumed += width; return Read(width); };
		/** @return Original native craft incarnation without resolving or allocating a hull. */
		const auto read_craft = [&]() { return SpaceCraftRef{VehicleID{static_cast<uint32_t>(read(4))}, read(8)}; };
		/** @return Original Surface center incarnation without consulting current-world station state. */
		const auto read_center = [&]() { return LaunchCenterRef{StationID{static_cast<uint16_t>(read(2))}, read(8)}; };
		const auto count = read(1);
		// Count and byte bounds are independent; older fixed rows remain exact, while429 permits only the bounded optional receipt on each real row.
		if (count > MAX_SPACE_VEHICLES || length < 1 + minimum_row * count || length > 1 + maximum_row * count) SlErrorCorrupt("Invalid launch assembly count");
		std::vector<SpaceProductionRecord> records;
		// Preserve each existing hull's original producer and work fields in detached staging; this pass reconstructs receipts and never commissions a vehicle.
		for (uint i = 0; i < count; ++i) {
			SpaceProductionRecord r; r.craft = read_craft(); auto &s = r.state;
			s.placement = static_cast<SpacePlacement>(read(1)); s.home = read_center(); s.origin = read_center(); s.surface_tile = TileIndex{static_cast<uint32_t>(read(4))};
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) && (s.placement == SpacePlacement::Carried || s.placement == SpacePlacement::InTransit)) SlErrorCorrupt("Unexpected carried/transiting hull in old launch assembly");
			s.remaining_ticks = static_cast<uint16_t>(read(2)); s.content_version = static_cast<uint16_t>(read(2)); s.model = static_cast<uint16_t>(read(2)); s.capital_paid = static_cast<uint32_t>(read(4));
			// Older files never commissioned content3 or heavy21. Full load and copied preview decode the same source boundary before either may install paid provenance.
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerCommissioning) && (s.content_version > MULTILAYER_SPACE_LEGACY_CONTENT_VERSION || s.model == 21)) SlErrorCorrupt("Future Space hull provenance in old save");
			const auto paused = read(1); if (paused > 1) SlErrorCorrupt("Invalid launch assembly boolean"); s.paused = paused != 0;
			if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) { const auto deployed = read(1); if (deployed > 1) SlErrorCorrupt("Invalid deployment boolean"); s.deployed = deployed != 0; }
			else if (s.content_version != 0 && s.placement == SpacePlacement::Orbital) SlErrorCorrupt("Unexpected deployed paid hull in old launch assembly");
			// Only427 has a real orbital-factory receipt; older records retain canonical empty provenance rather than acquiring a fictitious producer during migration.
			if (!IsSavegameVersionBefore(SaveLoadVersion::OrbitalManufacturingMaterialization)) {
				s.assembly = {SpaceStructureID{static_cast<uint16_t>(read(2))}, read(8)}; s.assembly_serial = read(8);
			}
			if (replacement) s.replacement = ReadSpaceReplacementTransport(read);
			records.push_back(r);
		}
		if (consumed != length) SlErrorCorrupt("Trailing launch assembly data");
		return records;
	}
	/** Decode one whole native family into detached load staging. */
	void Load() const override { if (_production) SlErrorCorrupt("Repeated launch assembly family"); _production = this->ReadRecords(); }
	/** Require the exact staged family during native reference reconstruction. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!_production && !IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchAssembly)) SlErrorCorrupt("Missing launch assembly family");
		if (_production && !ValidateSpaceProductionRecords(*_production, Map::SizeX(), Map::SizeY())) SlErrorCorrupt("Invalid launch assembly receipt");
	}
	/** @param length Native preview field extent; the complete decoder performs its own exact-length admission. */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_production_present) SlErrorCorrupt("Repeated launch assembly preview");
		_load_check_data.space_production_rows = this->ReadRecords(); _load_check_data.space_production_present = true;
	}
};
/** Original native launch-slot transport, independent of orbital factory ownership. */
struct LCOPChunkHandler : ChunkHandler {
	/** Register the original native Surface slot family. */
	LCOPChunkHandler() : ChunkHandler("LCOP", ChunkType::Riff) {}
	/** Serialize complete validated existing owners; this grants no materials, work, slots or hulls. */
	void Save() const override
	{
		if (!ValidateLaunchOperations()) SlErrorCorrupt("Invalid launch assembly slots on save");
		const auto records = CaptureLaunchOperations(); size_t length = 1;
		for (const auto &r : records) length += 57 + 12 * r.slots.pad_queue.size();
		SlSetLength(length); Write(records.size(), 1);
		for (const auto &r : records) { WriteCenter(r.center); WriteCraft(r.slots.workshop); WriteCraft(r.slots.service); Write(r.slots.pod_workshop.pod.base(), 2); Write(r.slots.pod_workshop.generation, 8); WriteCraft(r.slots.pad_active); Write(r.slots.pad_queue.size(), 1); for (auto ref : r.slots.pad_queue) WriteCraft(ref); }
	}
	/** @return Detached bounded original launch slots, without installing references. */
	std::vector<LaunchOperationsRecord> ReadRecords() const
	{
		const auto length = SlGetFieldLength();
		const size_t row_length = IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) ? 35 : 57;
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchAssembly) || length < 1 || length > 1 + MAX_LAUNCH_CENTERS * (row_length + 12 * MAX_CENTER_CRAFT)) SlErrorCorrupt("Invalid launch assembly slots length");
		const auto count = Read(1); if (count > MAX_LAUNCH_CENTERS) SlErrorCorrupt("Invalid launch assembly center count");
		std::vector<LaunchOperationsRecord> records; size_t consumed = 1;
		for (uint i = 0; i < count; ++i) {
			if (consumed + row_length > length) SlErrorCorrupt("Truncated launch assembly slots");
			LaunchOperationsRecord r; r.center = ReadCenter(); r.slots.workshop = ReadCraft(); r.slots.service = ReadCraft();
			if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) r.slots.pod_workshop = {SpacePodID{static_cast<uint16_t>(Read(2))}, Read(8)};
			if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) r.slots.pad_active = ReadCraft();
			const auto queued = Read(1);
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) && r.slots.service != SpaceCraftRef{}) SlErrorCorrupt("Unexpected service reservation in old launch assembly");
			consumed += row_length + 12 * queued; if (queued > MAX_CENTER_CRAFT || consumed > length) SlErrorCorrupt("Invalid launch assembly queue count");
			for (uint j = 0; j < queued; ++j) r.slots.pad_queue.push_back(ReadCraft()); records.push_back(std::move(r));
		}
		if (consumed != length || !ValidateLaunchOperationRecords(records)) SlErrorCorrupt("Invalid launch assembly slots");
		return records;
	}
	/** Decode one whole native family into detached load staging. */
	void Load() const override { if (_operations) SlErrorCorrupt("Repeated launch assembly slots"); _operations = this->ReadRecords(); }
	/** Require the exact staged family during native reference reconstruction. */
	void FixPointers() const override { if (SlIsFixingPointers() && !_operations && !IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchAssembly)) SlErrorCorrupt("Missing launch assembly slots"); }
	/** @param length Native preview field extent; the complete decoder performs its own exact-length admission. */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.launch_operations_present) SlErrorCorrupt("Repeated launch assembly slots preview");
		_load_check_data.launch_operations_rows = this->ReadRecords(); _load_check_data.launch_operations_present = true;
	}
};
static const SVSTChunkHandler SVST;
static const LCOPChunkHandler LCOP;
static const ChunkHandlerRef handlers[] = {SVST, LCOP};
extern const ChunkHandlerTable _space_production_chunk_handlers(handlers);
