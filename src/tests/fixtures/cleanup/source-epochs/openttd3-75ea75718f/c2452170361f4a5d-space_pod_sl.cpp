/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_pod_sl.cpp Bounded passive pod identity/custody and independent preview. */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../space_pod.h"
#include "../space_manifest.h"
#include "../launch_operations.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../map_func.h"
#include "../safeguards.h"

struct PodTransport { uint64_t next_generation = 1; std::vector<SpacePodRecord> records; };
static std::optional<PodTransport> candidate;
void ResetSpacePodLoadStaging() { candidate.reset(); }
bool AcceptLoadedSpacePods()
{
	if (!candidate) return IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics);
	if (!InstallLoadedSpacePods(candidate->records, candidate->next_generation)) return false;
	candidate.reset(); return true;
}
void ValidateSpacePodLoadPreview()
{
	if (!_load_check_data.space_pods_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing passive Space pod family");
		return;
	}
	const auto &records = _load_check_data.space_pod_rows;
	if ((!records.empty() || _load_check_data.next_space_pod_generation != 1) &&
		(_load_check_data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || _load_check_data.map_layers < 3)) SlErrorCorrupt("Space pod preview requires selected Space profile");
	if (!ValidateSpacePodRecords(records, _load_check_data.next_space_pod_generation, _load_check_data.map_size_x, _load_check_data.map_size_y) ||
		!ValidateSpacePodLinks(records, _load_check_data.space_production_rows, _load_check_data.launch_operations_rows, _load_check_data.space_manifest_rows)) SlErrorCorrupt("Invalid passive Space pod preview custody");
}
static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
static uint64_t Read(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
static void WriteCenter(LaunchCenterRef ref) { Write(ref.station.base(), 2); Write(ref.generation, 8); }
static LaunchCenterRef ReadCenter() { return {StationID{static_cast<uint16_t>(Read(2))}, Read(8)}; }
struct SPODChunkHandler : ChunkHandler {
	SPODChunkHandler() : ChunkHandler("SPOD", ChunkType::Riff) {}
	void Save() const override
	{
		if (!ValidateSpacePods()) SlErrorCorrupt("Invalid passive Space pod ownership on save");
		const auto records = CaptureSpacePods(); SlSetLength(10 + 65 * records.size()); Write(GetNextSpacePodGeneration(), 8); Write(records.size(), 2);
		for (const auto &r : records) {
			Write(r.ref.pod.base(), 2); Write(r.ref.generation, 8); const auto &s = r.state;
			Write(s.owner.base(), 1); Write(to_underlying(s.custody), 1); WriteCenter(s.home); WriteCenter(s.origin);
			Write(s.craft.vehicle.base(), 4); Write(s.craft.generation, 8); Write(s.surface_tile.base(), 4);
			Write(static_cast<uint32_t>(s.position.x), 4); Write(static_cast<uint32_t>(s.position.y), 4);
			Write(s.remaining_ticks, 2); Write(s.content_version, 2); Write(s.capital_paid, 4); Write(s.paused, 1);
		}
	}
	PodTransport ReadRecords() const
	{
		const auto length = SlGetFieldLength();
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) || length < 10 || length > 10 + 65 * SpacePodID::End().base()) SlErrorCorrupt("Invalid passive Space pod length");
		PodTransport transport; transport.next_generation = Read(8); const auto count = Read(2);
		if (count > SpacePodID::End().base() || length != 10 + 65 * count) SlErrorCorrupt("Invalid passive Space pod count");
		for (uint i = 0; i < count; ++i) {
			SpacePodRecord r; r.ref = {SpacePodID{static_cast<uint16_t>(Read(2))}, Read(8)}; auto &s = r.state;
			s.owner = Owner{static_cast<uint8_t>(Read(1))}; s.custody = static_cast<SpacePodCustody>(Read(1)); s.home = ReadCenter(); s.origin = ReadCenter();
			s.craft = {VehicleID{static_cast<uint32_t>(Read(4))}, Read(8)}; s.surface_tile = TileIndex{static_cast<uint32_t>(Read(4))};
			s.position = {static_cast<int32_t>(Read(4)), static_cast<int32_t>(Read(4))};
			s.remaining_ticks = static_cast<uint16_t>(Read(2)); s.content_version = static_cast<uint16_t>(Read(2)); s.capital_paid = static_cast<uint32_t>(Read(4));
			const auto paused = Read(1); if (paused > 1) SlErrorCorrupt("Invalid passive Space pod boolean"); s.paused = paused != 0;
			transport.records.push_back(r);
		}
		return transport;
	}
	void Load() const override { if (candidate) SlErrorCorrupt("Repeated passive Space pod family"); candidate = this->ReadRecords(); }
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!candidate) { if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing passive Space pod family"); return; }
		const auto *schema = GetPendingMultiLayerStateSchema();
		if ((!candidate->records.empty() || candidate->next_generation != 1) &&
			(schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Space pods require selected profile");
		if (!ValidateSpacePodRecords(candidate->records, candidate->next_generation, Map::SizeX(), Map::SizeY())) SlErrorCorrupt("Invalid passive Space pod receipt");
	}
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_pods_present) SlErrorCorrupt("Repeated passive Space pod preview");
		auto transport = this->ReadRecords(); _load_check_data.space_pod_rows = std::move(transport.records);
		_load_check_data.next_space_pod_generation = transport.next_generation; _load_check_data.space_pods_present = true;
	}
};
static const SPODChunkHandler SPOD;
static const ChunkHandlerRef handlers[] = {SPOD};
extern const ChunkHandlerTable _space_pod_chunk_handlers(handlers);
