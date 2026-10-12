/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_center_sl.cpp Bounded native launch-anchor transport and independent preview. */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../launch_center.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../safeguards.h"

struct LaunchCenterTransport {
	uint64_t next = 1;
	std::vector<LaunchCenterRecord> records;
};
static std::optional<LaunchCenterTransport> _staged_launch_centers;
struct LaunchHubTransport {
	uint64_t next = 1;
	std::vector<LaunchHubRecord> records;
};
static std::optional<LaunchHubTransport> _staged_launch_hubs;
void ResetLaunchCenterLoadStaging() { _staged_launch_centers.reset(); _staged_launch_hubs.reset(); }

static void WriteUnsigned(uint64_t value, uint8_t width)
{
	for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift));
}
static uint64_t ReadUnsigned(uint8_t width)
{
	uint64_t value = 0;
	while (width-- != 0) value = value << 8 | SlReadByte();
	return value;
}
static bool Active(const LaunchCenterTransport &data) { return data.next != 1 || !data.records.empty(); }

static void CheckTransport(const LaunchCenterTransport *data, uint32_t width, uint32_t height, uint32_t features)
{
	if (data == nullptr) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::LaunchCenters) || (features & MULTILAYER_FEATURE_LAUNCH_CENTERS) != 0) SlErrorCorrupt("Missing launch-center family");
		return;
	}
	if (!ValidateLaunchCenterRecords(data->records, data->next, width, height) ||
		Active(*data) != ((features & MULTILAYER_FEATURE_LAUNCH_CENTERS) != 0)) SlErrorCorrupt("Invalid launch-center transport authority");
}

bool AcceptLoadedLaunchCenters()
{
	if (!_staged_launch_centers) {
		return IsSavegameVersionBefore(SaveLoadVersion::LaunchCenters) && InstallLoadedLaunchCenters({}, 1);
	}
	if (!InstallLoadedLaunchCenters(std::move(_staged_launch_centers->records), _staged_launch_centers->next)) return false;
	_staged_launch_centers.reset();
	return true;
}

void ValidateLaunchCenterLoadPreview()
{
	if (!_load_check_data.launch_centers_present && !IsSavegameVersionBefore(SaveLoadVersion::LaunchCenters)) SlErrorCorrupt("Missing launch-center preview family");
	if (_load_check_data.launch_centers_active != ((_load_check_data.multilayer_feature_flags & MULTILAYER_FEATURE_LAUNCH_CENTERS) != 0)) SlErrorCorrupt("Launch-center preview feature mismatch");
	if (!_load_check_data.launch_hubs_present && !IsSavegameVersionBefore(SaveLoadVersion::LaunchCollectionHubs)) SlErrorCorrupt("Missing launch-hub preview family");
	if (_load_check_data.launch_hubs_active != ((_load_check_data.multilayer_feature_flags & MULTILAYER_FEATURE_LAUNCH_HUBS) != 0)) SlErrorCorrupt("Launch-hub preview feature mismatch");
}

struct LCTRChunkHandler : ChunkHandler {
	LCTRChunkHandler() : ChunkHandler("LCTR", ChunkType::Riff) {}
	void Save() const override
	{
		if (!ValidateLaunchCenters()) SlErrorCorrupt("Invalid native launch-center state");
		const LaunchCenterTransport data{GetNextLaunchCenterGeneration(), CaptureLaunchCenters()};
		CheckTransport(&data, Map::SizeX(), Map::SizeY(), CaptureMultiLayerStateSchema().feature_flags);
		SlSetLength(10 + 15 * data.records.size());
		WriteUnsigned(data.next, 8); WriteUnsigned(data.records.size(), 2);
		for (const auto &record : data.records) {
			WriteUnsigned(record.station.base(), 2); WriteUnsigned(record.anchor.generation, 8);
			WriteUnsigned(record.anchor.tile.base(), 4); WriteUnsigned(record.anchor.rotation, 1);
		}
	}
	LaunchCenterTransport Read() const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::LaunchCenters)) SlErrorCorrupt("Unexpected launch centers in old save");
		const auto length = SlGetFieldLength();
		if (length < 10 || length > 10 + 15 * MAX_LAUNCH_CENTERS) SlErrorCorrupt("Invalid launch-center family length");
		LaunchCenterTransport data;
		data.next = ReadUnsigned(8);
		const auto count = ReadUnsigned(2);
		if (count > MAX_LAUNCH_CENTERS || length != 10 + 15 * count) SlErrorCorrupt("Invalid launch-center record count");
		data.records.reserve(static_cast<size_t>(count));
		for (size_t i = 0; i < count; ++i) {
			const auto station = StationID{static_cast<uint16_t>(ReadUnsigned(2))};
			const auto generation = ReadUnsigned(8);
			const auto tile = TileIndex{static_cast<uint32_t>(ReadUnsigned(4))};
			const auto rotation = static_cast<uint8_t>(ReadUnsigned(1));
			data.records.push_back({station, {tile, generation, rotation}});
		}
		return data;
	}
	void Load() const override { _staged_launch_centers = this->Read(); }
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		const auto *metadata = GetPendingMultiLayerStateSchema();
		CheckTransport(_staged_launch_centers ? &*_staged_launch_centers : nullptr,
			Map::SizeX(), Map::SizeY(), metadata == nullptr ? 0 : metadata->feature_flags);
	}
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		const auto data = this->Read();
		if (!ValidateLaunchCenterRecords(data.records, data.next, _load_check_data.map_size_x, _load_check_data.map_size_y) ||
			(Active(data) && _load_check_data.map_layers < 3)) SlErrorCorrupt("Invalid launch-center preview geometry");
		_load_check_data.launch_centers_present = true;
		_load_check_data.launch_centers_active = Active(data);
	}
};
static const LCTRChunkHandler LCTR;

bool AcceptLoadedLaunchHubs()
{
	if (!_staged_launch_hubs) return IsSavegameVersionBefore(SaveLoadVersion::LaunchCollectionHubs) && InstallLoadedLaunchHubs({}, 1);
	if (!InstallLoadedLaunchHubs(std::move(_staged_launch_hubs->records), _staged_launch_hubs->next)) return false;
	_staged_launch_hubs.reset();
	return true;
}

static bool ActiveHub(const LaunchHubTransport &data) { return data.next != 1 || !data.records.empty(); }

struct LCHBChunkHandler : ChunkHandler {
	LCHBChunkHandler() : ChunkHandler("LCHB", ChunkType::Riff) {}
	void Save() const override
	{
		if (!ValidateLaunchHubs()) SlErrorCorrupt("Invalid native launch-hub state");
		const LaunchHubTransport data{GetNextLaunchHubMemberGeneration(), CaptureLaunchHubs()};
		size_t length = 10;
		for (const auto &record : data.records) length += 11 + 10 * record.members.size();
		SlSetLength(length);
		WriteUnsigned(data.next, 8); WriteUnsigned(data.records.size(), 2);
		for (const auto &record : data.records) {
			WriteUnsigned(record.center.station.base(), 2); WriteUnsigned(record.center.generation, 8); WriteUnsigned(record.members.size(), 1);
			for (const auto member : record.members) { WriteUnsigned(member.station.base(), 2); WriteUnsigned(member.generation, 8); }
		}
	}
	LaunchHubTransport Read() const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::LaunchCollectionHubs)) SlErrorCorrupt("Unexpected launch hubs in old save");
		const auto length = SlGetFieldLength();
		if (length < 10 || length > 10 + MAX_LAUNCH_CENTERS * (11 + 10 * MAX_LAUNCH_HUB_MEMBERS)) SlErrorCorrupt("Invalid launch-hub family length");
		LaunchHubTransport data;
		data.next = ReadUnsigned(8);
		const auto count = ReadUnsigned(2);
		if (count > MAX_LAUNCH_CENTERS) SlErrorCorrupt("Invalid launch-hub center count");
		data.records.reserve(static_cast<size_t>(count));
		size_t consumed = 10;
		for (size_t i = 0; i < count; ++i) {
			if (consumed + 11 > length) SlErrorCorrupt("Truncated launch-hub record");
			const auto station = StationID{static_cast<uint16_t>(ReadUnsigned(2))};
			const auto generation = ReadUnsigned(8);
			const auto members = ReadUnsigned(1);
			consumed += 11 + 10 * members;
			if (members == 0 || members > MAX_LAUNCH_HUB_MEMBERS || consumed > length) SlErrorCorrupt("Invalid launch-hub member count");
			LaunchHubRecord record{{station, generation}, {}};
			record.members.reserve(static_cast<size_t>(members));
			for (size_t j = 0; j < members; ++j) {
				const auto member = StationID{static_cast<uint16_t>(ReadUnsigned(2))};
				const auto identity = ReadUnsigned(8);
				record.members.push_back({member, identity});
			}
			data.records.push_back(std::move(record));
		}
		if (consumed != length || !ValidateLaunchHubRecords(data.records, data.next)) SlErrorCorrupt("Invalid launch-hub transport");
		return data;
	}
	void Load() const override { _staged_launch_hubs = this->Read(); }
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		const auto *metadata = GetPendingMultiLayerStateSchema();
		const bool expected = metadata != nullptr && (metadata->feature_flags & MULTILAYER_FEATURE_LAUNCH_HUBS) != 0;
		if (!_staged_launch_hubs) {
			if (!IsSavegameVersionBefore(SaveLoadVersion::LaunchCollectionHubs) || expected) SlErrorCorrupt("Missing launch-hub family");
		} else if (ActiveHub(*_staged_launch_hubs) != expected) SlErrorCorrupt("Launch-hub feature mismatch");
	}
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		const auto data = this->Read();
		if (ActiveHub(data) && _load_check_data.map_layers < 3) SlErrorCorrupt("Launch-hub preview needs Surface/Space geometry");
		_load_check_data.launch_hubs_present = true;
		_load_check_data.launch_hubs_active = ActiveHub(data);
	}
};
static const LCHBChunkHandler LCHB;
static const ChunkHandlerRef launch_center_handlers[] = {LCTR, LCHB};
extern const ChunkHandlerTable _launch_center_chunk_handlers(launch_center_handlers);
