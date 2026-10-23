/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_manifest_sl.cpp Bounded native spacecraft supply custody and independent preview; Plan9 Group2C. */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../space_manifest.h"
#include "../space_vehicle.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../safeguards.h"

static std::optional<std::vector<SpaceManifestRecord>> candidate;
void ResetSpaceManifestLoadStaging() { candidate.reset(); }
bool AcceptLoadedSpaceManifests()
{
	if (!candidate) return IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics);
	if (!InstallLoadedSpaceManifests(*candidate)) return false;
	candidate.reset(); return true;
}
void ValidateSpaceManifestLoadPreview()
{
	if (!_load_check_data.space_manifests_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing Space manifest family");
		return;
	}
	const auto &records = _load_check_data.space_manifest_rows;
	if (!ValidateSpaceManifestRecords(records) || (!records.empty() &&
		(_load_check_data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || _load_check_data.map_layers < 3))) SlErrorCorrupt("Invalid Space manifest preview profile");
	auto production = [&](SpaceCraftRef ref) -> const SpaceConstructionState * {
		for (const auto &r : _load_check_data.space_production_rows) if (r.craft == ref) return &r.state;
		return nullptr;
	};
	auto find = [&](SpaceCraftRef ref) -> const SpaceManifestRecord * { for (const auto &r : records) if (r.craft == ref) return &r; return nullptr; };
	for (const auto &r : records) {
		const auto *s = production(r.craft); if (s == nullptr) SlErrorCorrupt("Missing Space manifest craft identity");
		const auto model = s->content_version == 0 ? 15 : s->model;
		if (s->content_version == 0 && r.returns_remaining != 0 && r.services_completed == 0) SlErrorCorrupt("Unearned legacy service allowance in manifest preview");
		if ((model == 14 && (r.carrier != SpaceCraftRef{} || r.service_ticks != 0 || r.returns_remaining != 0 || r.pending_returns != 0 || r.services_completed != 0)) ||
			(model != 14 && (r.passenger != SpaceCraftRef{} || r.fuel_burned != 0))) SlErrorCorrupt("Invalid Space manifest preview role");
		for (uint i = 0; i < SPACE_SUPPLY_COUNT; ++i) if (r.supplies[i].size() > GetSpaceSupplyCapacity(model, static_cast<SpaceSupply>(i))) SlErrorCorrupt("Invalid Space manifest preview capacity");
		if (r.passenger != SpaceCraftRef{}) {
			const auto *child = find(r.passenger); const auto *state = production(r.passenger);
			if (child == nullptr || state == nullptr || child->carrier != r.craft || state->placement != SpacePlacement::Carried || state->home != s->home) SlErrorCorrupt("Invalid Space manifest passenger preview");
		}
		if (r.carrier != SpaceCraftRef{}) {
			const auto *parent = find(r.carrier); const auto *state = production(r.carrier);
			if (parent == nullptr || state == nullptr || parent->passenger != r.craft || state->model != 14 || (state->placement != SpacePlacement::SurfaceStored && state->placement != SpacePlacement::InTransit) || s->placement != SpacePlacement::Carried) SlErrorCorrupt("Invalid Space manifest carrier preview");
		} else if (s->placement == SpacePlacement::Carried) SlErrorCorrupt("Unowned carried hull in manifest preview");
		if (r.service_ticks != 0) {
			if (r.service_location == SpaceServiceLocation::Onboard) {
				if (s->placement != SpacePlacement::Orbital || r.returns_remaining != 0) SlErrorCorrupt("Invalid onboard service preview");
				const auto &queues = _load_check_data.space_order_preview_rows;
				const auto q = std::find_if(queues.begin(), queues.end(), [&](const auto &o) { return SpaceCraftRef{o.vehicle, o.generation} == r.craft; });
				if (q == queues.end() || (q->queue.cursor < q->queue.entries.size() && !IsSpaceMiningServiceWait(q->queue))) SlErrorCorrupt("Active order conflicts with onboard service preview");
			} else if (s->placement == SpacePlacement::Unhomed) { if (!r.service_paused) SlErrorCorrupt("Unpaused unhomed service preview"); }
			else {
				bool reserved = false;
				for (const auto &op : _load_check_data.launch_operations_rows) if (op.center == s->home && op.slots.service == r.craft) reserved = true;
				if (s->placement != SpacePlacement::SurfaceStored || (!r.service_paused && !reserved)) SlErrorCorrupt("Missing native service reservation preview");
			}
		}
	}
	for (const auto &op : _load_check_data.launch_operations_rows) if (op.slots.service != SpaceCraftRef{}) {
		const auto *r = find(op.slots.service); if (r == nullptr || r->service_ticks == 0 || r->service_location != SpaceServiceLocation::EarthWorkshop) SlErrorCorrupt("Service slot has no manifest work preview");
	}
	for (const auto &r : _load_check_data.space_production_rows) if (r.state.placement == SpacePlacement::Carried && find(r.craft) == nullptr) SlErrorCorrupt("Carried hull has no manifest preview");
}
static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
static uint64_t Read(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
static void WriteCraft(SpaceCraftRef ref) { Write(ref.vehicle.base(), 4); Write(ref.generation, 8); }
static SpaceCraftRef ReadCraft() { return {VehicleID{static_cast<uint32_t>(Read(4))}, Read(8)}; }
struct SPSUChunkHandler : ChunkHandler {
	SPSUChunkHandler() : ChunkHandler("SPSU", ChunkType::Riff) {}
	void Save() const override
	{
		if (!ValidateSpaceManifests()) SlErrorCorrupt("Invalid Space manifest ownership on save");
		const auto records = CaptureSpaceManifests(); size_t length = 1;
		for (const auto &r : records) { length += 64 + 10 * r.pods.size(); for (const auto &slot : r.supplies) length += 6 * slot.size(); }
		SlSetLength(length); Write(records.size(), 1);
		for (const auto &r : records) {
			WriteCraft(r.craft); WriteCraft(r.passenger); WriteCraft(r.carrier); Write(r.age_ticks, 2); Write(r.service_ticks, 2); Write(r.returns_remaining, 1); Write(r.service_paused, 1);
			Write(r.pending_returns, 1); Write(to_underlying(r.service_location), 1);
			Write(r.fuel_burned, 8); Write(r.services_completed, 8);
			Write(r.pods.size(), 1); for (const auto pod : r.pods) { Write(pod.pod.base(), 2); Write(pod.generation, 8); }
			for (const auto &slot : r.supplies) { Write(slot.size(), 1); for (const auto &[packet, hop] : slot) { Write(packet.base(), 4); Write(hop.base(), 2); } }
		}
	}
	std::vector<SpaceManifestRecord> ReadRecords() const
	{
		const auto length = SlGetFieldLength();
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) || length < 1 || length > 1 + MAX_SPACE_VEHICLES * (64 + 80 + 6 * 128 * SPACE_SUPPLY_COUNT)) SlErrorCorrupt("Invalid Space manifest length");
		const auto count = Read(1); if (count > MAX_SPACE_VEHICLES) SlErrorCorrupt("Invalid Space manifest count");
		std::vector<SpaceManifestRecord> records; size_t consumed = 1;
		for (uint i = 0; i < count; ++i) {
			if (consumed + 64 > length) SlErrorCorrupt("Truncated Space manifest row");
			SpaceManifestRecord r; r.craft = ReadCraft(); r.passenger = ReadCraft(); r.carrier = ReadCraft();
			r.age_ticks = static_cast<uint16_t>(Read(2)); r.service_ticks = static_cast<uint16_t>(Read(2)); r.returns_remaining = static_cast<uint8_t>(Read(1)); const auto paused = Read(1);
			if (paused > 1) SlErrorCorrupt("Invalid Space manifest boolean"); r.service_paused = paused != 0;
			r.pending_returns = static_cast<uint8_t>(Read(1)); r.service_location = static_cast<SpaceServiceLocation>(Read(1));
			r.fuel_burned = Read(8); r.services_completed = Read(8);
			const auto pods = Read(1); consumed += 61 + 10 * pods;
			if (pods > 8 || consumed + SPACE_SUPPLY_COUNT > length) SlErrorCorrupt("Invalid Space manifest pod count");
			for (uint j = 0; j < pods; ++j) r.pods.push_back({SpacePodID{static_cast<uint16_t>(Read(2))}, Read(8)});
			for (auto &slot : r.supplies) {
				if (consumed + 1 > length) SlErrorCorrupt("Truncated Space manifest slot");
				const auto packets = Read(1); consumed += 1 + 6 * packets;
				if (packets > 128 || consumed > length) SlErrorCorrupt("Invalid Space manifest packet count");
				for (uint j = 0; j < packets; ++j) slot.push_back({CargoPacketID{static_cast<uint32_t>(Read(4))}, StationID{static_cast<uint16_t>(Read(2))}});
			}
			records.push_back(std::move(r));
		}
		if (consumed != length || !ValidateSpaceManifestRecords(records)) SlErrorCorrupt("Invalid Space manifest transport");
		return records;
	}
	void Load() const override { if (candidate) SlErrorCorrupt("Repeated Space manifest family"); candidate = this->ReadRecords(); }
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!candidate && !IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing Space manifest family");
		const auto *schema = GetPendingMultiLayerStateSchema();
		if (candidate && !candidate->empty() && (schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Space manifests require selected profile");
	}
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_manifests_present) SlErrorCorrupt("Repeated Space manifest preview");
		_load_check_data.space_manifest_rows = this->ReadRecords(); _load_check_data.space_manifests_present = true;
	}
};
static const SPSUChunkHandler SPSU;
static const ChunkHandlerRef handlers[] = {SPSU};
extern const ChunkHandlerTable _space_manifest_chunk_handlers(handlers);
