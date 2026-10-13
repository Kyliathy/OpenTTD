/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_production_sl.cpp Paid native hull receipts and referenced center slots; Plan9 Groups2B/8B. */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../launch_operations.h"
#include "../space_vehicle.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../map_func.h"
#include "../safeguards.h"

static std::optional<std::vector<SpaceProductionRecord>> _production;
static std::optional<std::vector<LaunchOperationsRecord>> _operations;
void ResetSpaceProductionLoadStaging() { _production.reset(); _operations.reset(); }

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
static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
static uint64_t Read(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
static void WriteCraft(SpaceCraftRef ref) { Write(ref.vehicle.base(), 4); Write(ref.generation, 8); }
static SpaceCraftRef ReadCraft() { return {VehicleID{static_cast<uint32_t>(Read(4))}, Read(8)}; }
static void WriteCenter(LaunchCenterRef ref) { Write(ref.station.base(), 2); Write(ref.generation, 8); }
static LaunchCenterRef ReadCenter() { return {StationID{static_cast<uint16_t>(Read(2))}, Read(8)}; }

struct SVSTChunkHandler : ChunkHandler {
	SVSTChunkHandler() : ChunkHandler("SVST", ChunkType::Riff) {}
	void Save() const override
	{
		if (!ValidateSpaceProduction()) SlErrorCorrupt("Invalid launch assembly ownership on save");
		const auto records = CaptureSpaceProduction(); SlSetLength(1 + 49 * records.size()); Write(records.size(), 1);
		for (const auto &r : records) {
			WriteCraft(r.craft); const auto &s = r.state; Write(to_underlying(s.placement), 1); WriteCenter(s.home); WriteCenter(s.origin);
			Write(s.surface_tile.base(), 4); Write(s.remaining_ticks, 2); Write(s.content_version, 2); Write(s.model, 2); Write(s.capital_paid, 4); Write(s.paused, 1); Write(s.deployed, 1);
		}
	}
	std::vector<SpaceProductionRecord> ReadRecords() const
	{
		const auto length = SlGetFieldLength();
		const size_t row_length = IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) ? 48 : 49;
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchAssembly) || length < 1 || length > 1 + row_length * MAX_SPACE_VEHICLES) SlErrorCorrupt("Invalid launch assembly length");
		const auto count = Read(1); if (length != 1 + row_length * count || count > MAX_SPACE_VEHICLES) SlErrorCorrupt("Invalid launch assembly count");
		std::vector<SpaceProductionRecord> records;
		for (uint i = 0; i < count; ++i) {
			SpaceProductionRecord r; r.craft = ReadCraft(); auto &s = r.state;
			s.placement = static_cast<SpacePlacement>(Read(1)); s.home = ReadCenter(); s.origin = ReadCenter(); s.surface_tile = TileIndex{static_cast<uint32_t>(Read(4))};
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) && (s.placement == SpacePlacement::Carried || s.placement == SpacePlacement::InTransit)) SlErrorCorrupt("Unexpected carried/transiting hull in old launch assembly");
			s.remaining_ticks = static_cast<uint16_t>(Read(2)); s.content_version = static_cast<uint16_t>(Read(2)); s.model = static_cast<uint16_t>(Read(2)); s.capital_paid = static_cast<uint32_t>(Read(4));
			const auto paused = Read(1); if (paused > 1) SlErrorCorrupt("Invalid launch assembly boolean"); s.paused = paused != 0;
			if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) { const auto deployed = Read(1); if (deployed > 1) SlErrorCorrupt("Invalid deployment boolean"); s.deployed = deployed != 0; }
			else if (s.content_version != 0 && s.placement == SpacePlacement::Orbital) SlErrorCorrupt("Unexpected deployed paid hull in old launch assembly");
			records.push_back(r);
		}
		return records;
	}
	void Load() const override { if (_production) SlErrorCorrupt("Repeated launch assembly family"); _production = this->ReadRecords(); }
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!_production && !IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchAssembly)) SlErrorCorrupt("Missing launch assembly family");
		if (_production && !ValidateSpaceProductionRecords(*_production, Map::SizeX(), Map::SizeY())) SlErrorCorrupt("Invalid launch assembly receipt");
	}
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_production_present) SlErrorCorrupt("Repeated launch assembly preview");
		_load_check_data.space_production_rows = this->ReadRecords(); _load_check_data.space_production_present = true;
	}
};
struct LCOPChunkHandler : ChunkHandler {
	LCOPChunkHandler() : ChunkHandler("LCOP", ChunkType::Riff) {}
	void Save() const override
	{
		if (!ValidateLaunchOperations()) SlErrorCorrupt("Invalid launch assembly slots on save");
		const auto records = CaptureLaunchOperations(); size_t length = 1;
		for (const auto &r : records) length += 57 + 12 * r.slots.pad_queue.size();
		SlSetLength(length); Write(records.size(), 1);
		for (const auto &r : records) { WriteCenter(r.center); WriteCraft(r.slots.workshop); WriteCraft(r.slots.service); Write(r.slots.pod_workshop.pod.base(), 2); Write(r.slots.pod_workshop.generation, 8); WriteCraft(r.slots.pad_active); Write(r.slots.pad_queue.size(), 1); for (auto ref : r.slots.pad_queue) WriteCraft(ref); }
	}
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
	void Load() const override { if (_operations) SlErrorCorrupt("Repeated launch assembly slots"); _operations = this->ReadRecords(); }
	void FixPointers() const override { if (SlIsFixingPointers() && !_operations && !IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchAssembly)) SlErrorCorrupt("Missing launch assembly slots"); }
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
