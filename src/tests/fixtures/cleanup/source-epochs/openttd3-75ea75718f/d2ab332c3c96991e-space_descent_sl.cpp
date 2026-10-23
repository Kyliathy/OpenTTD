/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_descent_sl.cpp Bounded paid-pod cargo custody, reservations and independent preview. */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../launch_center.h"
#include "../space_pod.h"
#include "../space_manifest.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../map_func.h"
#include "../safeguards.h"

struct DescentTransport { uint64_t next = 1; std::vector<SpaceDescentRecord> records; };
static std::optional<DescentTransport> candidate;
void ResetSpaceDescentLoadStaging() { candidate.reset(); }
bool AcceptLoadedSpaceDescents()
{
	if (!candidate) return IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics);
	if (!InstallLoadedSpaceDescents(candidate->records, candidate->next)) return false;
	candidate.reset(); return true;
}
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
}
static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
static uint64_t Read(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
struct SPDJChunkHandler : ChunkHandler {
	SPDJChunkHandler() : ChunkHandler("SPDJ", ChunkType::Riff) {}
	void Save() const override
	{
		if (!ValidateSpaceDescents()) SlErrorCorrupt("Invalid Space descent ownership on save");
		const auto records = CaptureSpaceDescents(); size_t length = 10; for (const auto &r : records) length += 57 + 6 * r.packets.size();
		SlSetLength(length); Write(GetNextSpaceDescentID(), 8); Write(records.size(), 2);
		for (const auto &r : records) {
			Write(r.pod.pod.base(), 2); Write(r.pod.generation, 8); Write(r.id, 8); Write(r.center.station.base(), 2); Write(r.center.generation, 8);
			Write(static_cast<uint32_t>(r.release_point.x), 4); Write(static_cast<uint32_t>(r.release_point.y), 4); Write(r.sender.base(), 4); Write(r.sender_generation, 8);
			Write(r.cargo, 1); Write(r.quantity, 2); Write(r.remaining_ticks, 2); Write(r.age_ticks, 2); Write(to_underlying(r.blocked), 1); Write(r.packets.size(), 1);
			for (const auto &[packet, hop] : r.packets) { Write(packet.base(), 4); Write(hop.base(), 2); }
		}
	}
	DescentTransport ReadRecords() const
	{
		const auto length = SlGetFieldLength();
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) || length < 10 || length > 10 + SpacePodID::End().base() * (57 + 6 * 40)) SlErrorCorrupt("Invalid Space descent length");
		DescentTransport transport; transport.next = Read(8); const auto count = Read(2); if (count > SpacePodID::End().base()) SlErrorCorrupt("Invalid Space descent count");
		size_t consumed = 10;
		for (uint i = 0; i < count; ++i) {
			if (consumed + 57 > length) SlErrorCorrupt("Truncated Space descent row");
			SpaceDescentRecord r; r.pod = {SpacePodID{static_cast<uint16_t>(Read(2))}, Read(8)}; r.id = Read(8); r.center = {StationID{static_cast<uint16_t>(Read(2))}, Read(8)};
			r.release_point = {static_cast<int32_t>(Read(4)), static_cast<int32_t>(Read(4))}; r.sender = VehicleID{static_cast<uint32_t>(Read(4))}; r.sender_generation = Read(8); r.cargo = static_cast<CargoType>(Read(1));
			r.quantity = static_cast<uint16_t>(Read(2)); r.remaining_ticks = static_cast<uint16_t>(Read(2)); r.age_ticks = static_cast<uint16_t>(Read(2)); r.blocked = static_cast<SpaceDescentBlock>(Read(1));
			const auto packets = Read(1); consumed += 57 + 6 * packets; if (packets == 0 || packets > 40 || consumed > length) SlErrorCorrupt("Invalid Space descent packet count");
			for (uint j = 0; j < packets; ++j) r.packets.push_back({CargoPacketID{static_cast<uint32_t>(Read(4))}, StationID{static_cast<uint16_t>(Read(2))}});
			transport.records.push_back(std::move(r));
		}
		if (consumed != length) SlErrorCorrupt("Invalid Space descent transport length");
		return transport;
	}
	void Load() const override { if (candidate) SlErrorCorrupt("Repeated Space descent family"); candidate = this->ReadRecords(); }
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!candidate) { if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing Space descent family"); return; }
		const auto *schema = GetPendingMultiLayerStateSchema();
		if ((!candidate->records.empty() || candidate->next != 1) && (schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Space descent requires selected profile");
		if (!ValidateSpaceDescentRecords(candidate->records, candidate->next, Map::SizeX(), Map::SizeY())) SlErrorCorrupt("Invalid Space descent transport");
	}
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_descents_present) SlErrorCorrupt("Repeated Space descent preview");
		auto data = this->ReadRecords(); _load_check_data.space_descent_rows = std::move(data.records); _load_check_data.next_space_descent_id = data.next; _load_check_data.space_descents_present = true;
	}
};
static const SPDJChunkHandler SPDJ;
static const ChunkHandlerRef handlers[] = {SPDJ};
extern const ChunkHandlerTable _space_descent_chunk_handlers(handlers);
