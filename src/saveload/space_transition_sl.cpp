/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_transition_sl.cpp Saved native rocket round trips and independent preview. */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../space_transition.h"
#include "../space_vehicle.h"
#include "../launch_operations.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../safeguards.h"

static std::optional<std::vector<SpaceTransitionRecord>> candidate;
void ResetSpaceTransitionLoadStaging() { candidate.reset(); }
bool AcceptLoadedSpaceTransitions()
{
	if (!candidate) return IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics);
	if (!InstallLoadedSpaceTransitions(*candidate)) return false;
	candidate.reset(); return true;
}
void ValidateSpaceTransitionLoadPreview()
{
	if (!_load_check_data.space_transitions_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing native Space transition family");
		return;
	}
	const auto &records = _load_check_data.space_transition_rows;
	if ((!records.empty() && (_load_check_data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || _load_check_data.map_layers < 3)) ||
		!ValidateSpaceTransitionRecords(records, _load_check_data.map_size_x, _load_check_data.map_size_y) || !ValidateSpaceTransitionLinks(records, _load_check_data.space_production_rows, _load_check_data.launch_operations_rows)) SlErrorCorrupt("Invalid native Space transition preview");
	for (const auto &r : records) if (r.state.destination.kind == SpaceOrderTargetKind::Craft) {
		if (r.state.destination.generation >= _load_check_data.next_space_vehicle_generation) SlErrorCorrupt("Invalid future Space resupply target preview");
		const auto &native = _load_check_data.native_space_vehicle_rows;
		const auto sender = std::find_if(native.begin(), native.end(), [&](const auto &v) { return v.vehicle == r.craft.vehicle; });
		if (sender == native.end()) SlErrorCorrupt("Missing native Space resupply sender preview");
		const SpaceCraftRef target{VehicleID{r.state.destination.id}, r.state.destination.generation};
		const auto identity = std::find(_load_check_data.space_order_preview_refs.begin(), _load_check_data.space_order_preview_refs.end(), target);
		if (identity != _load_check_data.space_order_preview_refs.end() && r.state.blocked != SpaceFlightBlock::TargetLost &&
			(r.state.phase == SpaceFlightPhase::Preparing || r.state.phase == SpaceFlightPhase::Ascent || r.state.phase == SpaceFlightPhase::OrbitalDelivery)) {
			const auto receiver = std::find_if(native.begin(), native.end(), [&](const auto &v) { return v.vehicle == target.vehicle; });
			if (receiver == native.end() || receiver->owner != sender->owner) SlErrorCorrupt("Foreign native Space resupply receiver preview");
		}
		const auto manifest = std::find_if(_load_check_data.space_manifest_rows.begin(), _load_check_data.space_manifest_rows.end(), [&](const auto &m) { return m.craft == r.craft; });
		if (manifest == _load_check_data.space_manifest_rows.end() || manifest->passenger != SpaceCraftRef{}) SlErrorCorrupt("Invalid native Space resupply passenger preview");
		if (r.state.resupplied && (!manifest->pods.empty() || !manifest->supplies[1].empty() || !manifest->supplies[2].empty())) SlErrorCorrupt("Repeated native Space resupply payload preview");
	}
}
static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
static uint64_t Read(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
static void WriteCenter(LaunchCenterRef ref) { Write(ref.station.base(), 2); Write(ref.generation, 8); }
static LaunchCenterRef ReadCenter() { return {StationID{static_cast<uint16_t>(Read(2))}, Read(8)}; }
struct SPFLChunkHandler : ChunkHandler {
	SPFLChunkHandler() : ChunkHandler("SPFL", ChunkType::Riff) {}
	void Save() const override
	{
		if (!ValidateSpaceTransitions()) SlErrorCorrupt("Invalid native Space transition ownership on save");
		const auto records = CaptureSpaceTransitions(); SlSetLength(1 + 84 * records.size()); Write(records.size(), 1);
		for (const auto &r : records) {
			Write(r.craft.vehicle.base(), 4); Write(r.craft.generation, 8); const auto &f = r.state;
			Write(to_underlying(f.phase), 1); Write(to_underlying(f.blocked), 1); Write(to_underlying(f.layer), 1); WriteCenter(f.source); WriteCenter(f.recovery);
			Write(to_underlying(f.destination.kind), 1); Write(f.destination.id, 4); Write(f.destination.generation, 8);
			Write(static_cast<uint32_t>(f.departure.x), 4); Write(static_cast<uint32_t>(f.departure.y), 4); Write(f.remaining_ticks, 2); Write(f.pad_cost_remainder, 1);
			Write(unsigned(f.outbound_burned) | (unsigned(f.recovery_burned) << 1) | (unsigned(f.deployed) << 2) | (unsigned(f.resupplied) << 3), 1);
			Write(f.missions_started, 8); Write(f.missions_completed, 8); Write(f.missions_canceled, 8);
		}
	}
	std::vector<SpaceTransitionRecord> ReadRecords() const
	{
		const auto length = SlGetFieldLength();
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) || length < 1 || length > 1 + 84 * MAX_SPACE_VEHICLES) SlErrorCorrupt("Invalid native Space transition length");
		const auto count = Read(1); if (count > MAX_SPACE_VEHICLES || length != 1 + 84 * count) SlErrorCorrupt("Invalid native Space transition count");
		std::vector<SpaceTransitionRecord> records;
		for (uint i = 0; i < count; ++i) {
			SpaceTransitionRecord r; r.craft = {VehicleID{static_cast<uint32_t>(Read(4))}, Read(8)}; auto &f = r.state;
			f.phase = static_cast<SpaceFlightPhase>(Read(1)); f.blocked = static_cast<SpaceFlightBlock>(Read(1)); f.layer = static_cast<MapLayer>(Read(1)); f.source = ReadCenter(); f.recovery = ReadCenter();
			f.destination = {static_cast<SpaceOrderTargetKind>(Read(1)), static_cast<uint32_t>(Read(4)), Read(8)};
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply) && (f.destination.kind == SpaceOrderTargetKind::Craft || f.blocked >= SpaceFlightBlock::ReceiverBusy)) SlErrorCorrupt("Unexpected resupply in old Space transition");
			f.departure = {static_cast<int32_t>(Read(4)), static_cast<int32_t>(Read(4))}; f.remaining_ticks = static_cast<uint16_t>(Read(2)); f.pad_cost_remainder = static_cast<uint8_t>(Read(1));
			const auto flags = Read(1); if (flags > (IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply) ? 7 : 15)) SlErrorCorrupt("Invalid native Space transition flags");
			f.outbound_burned = (flags & 1) != 0; f.recovery_burned = (flags & 2) != 0; f.deployed = (flags & 4) != 0;
			f.resupplied = (flags & 8) != 0;
			f.missions_started = Read(8); f.missions_completed = Read(8); f.missions_canceled = Read(8); records.push_back(r);
		}
		if (!ValidateSpaceTransitionRecords(records)) SlErrorCorrupt("Invalid native Space transition transport");
		return records;
	}
	void Load() const override { if (candidate) SlErrorCorrupt("Repeated native Space transition family"); candidate = this->ReadRecords(); }
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!candidate && !IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing native Space transition family");
		const auto *schema = GetPendingMultiLayerStateSchema();
		if (candidate && !candidate->empty() && (schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Space transitions require selected profile");
	}
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_transitions_present) SlErrorCorrupt("Repeated native Space transition preview");
		_load_check_data.space_transition_rows = this->ReadRecords(); _load_check_data.space_transitions_present = true;
	}
};
static const SPFLChunkHandler SPFL;
static const ChunkHandlerRef handlers[] = {SPFL};
extern const ChunkHandlerTable _space_transition_chunk_handlers(handlers);
