/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_pod_sl.cpp Bounded passive pod identity/custody and independent preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups8C/13
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9B; conserve original paid2 shell provenance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; copied carrier authority/pose comes only from this candidate file.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../space_pod.h"
#include "../space_manifest.h"
#include "../launch_operations.h"
#include "../multilayer_state.h"
#include "../multilayer_space_content.h"
#include "../fios.h"
#include "../map_func.h"
#include "../safeguards.h"

/** Detached bounded file transport; only the existing native pod owner may install custody after cross-family validation. */
struct PodTransport { uint64_t next_generation = 1; std::vector<SpacePodRecord> records; };
static std::optional<PodTransport> candidate;
/** Discard unpublished file staging without deleting a real shell or changing its current carrier/recovery claim. */
void ResetSpacePodLoadStaging() { candidate.reset(); }
/** @return Whether complete staged rows were admitted by the native pod owner, or a genuine pre-pod source needs no family. */
bool AcceptLoadedSpacePods()
{
	if (!candidate) return IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics);
	if (!InstallLoadedSpacePods(candidate->records, candidate->next_generation)) return false;
	candidate.reset(); return true;
}
/** Validate this file's copied profile, identities and carrier/center/manifest links without resolving or mutating the open world. */
void ValidateSpacePodLoadPreview()
{
	if (!_load_check_data.space_pods_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing passive Space pod family");
		return;
	}
	const auto &records = _load_check_data.space_pod_rows;
	// Any issued pod namespace requires the selected multilayer Space profile; empty Classic metadata cannot lend custody authority to a forged saved shell.
	if ((!records.empty() || _load_check_data.next_space_pod_generation != 1) &&
		(_load_check_data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || _load_check_data.map_layers < 3)) SlErrorCorrupt("Space pod preview requires selected Space profile");
	// Both bounded individual receipts and the complete same-file custody graph must agree before any pod preview is accepted.
	if (!ValidateSpacePodRecords(records, _load_check_data.next_space_pod_generation, _load_check_data.map_size_x, _load_check_data.map_size_y) ||
		!ValidateSpacePodLinks(records, _load_check_data.space_production_rows, _load_check_data.launch_operations_rows, _load_check_data.space_manifest_rows)) SlErrorCorrupt("Invalid passive Space pod preview custody");
	// A copied carried shell follows the exact native owner and physical point in this file; an open-world hull or coincident foreign carrier cannot validate it.
	for (const auto &row : records) {
		if (row.state.custody != SpacePodCustody::Craft) continue;
		/** @param candidate Same-file native hull. @return Whether its actual pool identity is the already generation-qualified carrier. */
		const auto carrier = std::ranges::find_if(_load_check_data.native_space_vehicle_rows, [&](const auto &candidate) { return candidate.vehicle == row.state.craft.vehicle; });
		if (carrier == _load_check_data.native_space_vehicle_rows.end() || carrier->owner != row.state.owner || carrier->position != row.state.position) SlErrorCorrupt("Foreign or displaced passive pod carrier preview");
	}
}
/** @param value Original unsigned scalar. @param width Exact fixed network-order width, preserving the original source format. */
static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
/** @param width Exact already bounded network-order scalar width. @return Decoded unsigned value; field validators retain range/custody authority. */
static uint64_t Read(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
/** @param ref Original generation-qualified center identity, never a replacement station selected by numeric pool order. */
static void WriteCenter(LaunchCenterRef ref) { Write(ref.station.base(), 2); Write(ref.generation, 8); }
/** @return Copied exact center identity; same-file validators resolve its custody rights after complete decoding. */
static LaunchCenterRef ReadCenter() { return {StationID{static_cast<uint16_t>(Read(2))}, Read(8)}; }
/** Original fixed65-byte shell receipts with independently validated native and copied-preview publication paths. */
struct SPODChunkHandler : ChunkHandler {
	/** Register passive asset transport without allocating a shell, carrier or recovery owner. */
	SPODChunkHandler() : ChunkHandler("SPOD", ChunkType::Riff) {}
	/** Serialize only currently valid actual shell identities, paid provenance and exact existing custody. */
	void Save() const override
	{
		if (!ValidateSpacePods()) SlErrorCorrupt("Invalid passive Space pod ownership on save");
		const auto records = CaptureSpacePods(); SlSetLength(10 + 65 * records.size()); Write(GetNextSpacePodGeneration(), 8); Write(records.size(), 2);
		// Each shell has one original paid receipt and one custody owner; capture their exact fields rather than creating another routed vehicle or payload packet.
		for (const auto &r : records) {
			Write(r.ref.pod.base(), 2); Write(r.ref.generation, 8); const auto &s = r.state;
			Write(s.owner.base(), 1); Write(to_underlying(s.custody), 1); WriteCenter(s.home); WriteCenter(s.origin);
			Write(s.craft.vehicle.base(), 4); Write(s.craft.generation, 8); Write(s.surface_tile.base(), 4);
			Write(static_cast<uint32_t>(s.position.x), 4); Write(static_cast<uint32_t>(s.position.y), 4);
			Write(s.remaining_ticks, 2); Write(s.content_version, 2); Write(s.capital_paid, 4); Write(s.paused, 1);
		}
	}
	/** @return Complete detached source snapshot, with exact framing and source-version provenance; decoding allocates no native pod. */
	PodTransport ReadRecords() const
	{
		const auto length = SlGetFieldLength();
		// Only a genuine pod-era family inside its bounded original extent may supply receipts; oversized or older forged families cannot allocate staging indefinitely.
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics) || length < 10 || length > 10 + 65 * SpacePodID::End().base()) SlErrorCorrupt("Invalid passive Space pod length");
		PodTransport transport; transport.next_generation = Read(8); const auto count = Read(2);
		// Exact count/extent agreement prevents a missing/extra row from being interpreted as another shell's generation or paid material history.
		if (count > SpacePodID::End().base() || length != 10 + 65 * count) SlErrorCorrupt("Invalid passive Space pod count");
		// Decode every original row before any owner can publish custody; current content cannot retroactively create a new paid receipt in an older source.
		for (uint i = 0; i < count; ++i) {
			SpacePodRecord r; r.ref = {SpacePodID{static_cast<uint16_t>(Read(2))}, Read(8)}; auto &s = r.state;
			s.owner = Owner{static_cast<uint8_t>(Read(1))}; s.custody = static_cast<SpacePodCustody>(Read(1)); s.home = ReadCenter(); s.origin = ReadCenter();
			s.craft = {VehicleID{static_cast<uint32_t>(Read(4))}, Read(8)}; s.surface_tile = TileIndex{static_cast<uint32_t>(Read(4))};
			s.position = {static_cast<int32_t>(Read(4)), static_cast<int32_t>(Read(4))};
			s.remaining_ticks = static_cast<uint16_t>(Read(2)); s.content_version = static_cast<uint16_t>(Read(2)); s.capital_paid = static_cast<uint32_t>(Read(4));
			// Paid shell provenance follows the same source-version rule as hulls/frames; supporting retained2 shells does not legitimize new3 receipts in a pre431 file.
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerCommissioning) && s.content_version > MULTILAYER_SPACE_LEGACY_CONTENT_VERSION) SlErrorCorrupt("Future Space pod provenance in old save");
			const auto paused = Read(1); if (paused > 1) SlErrorCorrupt("Invalid passive Space pod boolean"); s.paused = paused != 0;
			transport.records.push_back(r);
		}
		return transport;
	}
	/** Stage the sole original family; repeated transport never replaces already decoded provenance. */
	void Load() const override { if (candidate) SlErrorCorrupt("Repeated passive Space pod family"); candidate = this->ReadRecords(); }
	/** Check copied profile/geometry after map staging; actual cross-family installation remains with AcceptLoadedSpacePods. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!candidate) { if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchLogistics)) SlErrorCorrupt("Missing passive Space pod family"); return; }
		const auto *schema = GetPendingMultiLayerStateSchema();
		// Any issued shell namespace belongs to the selected Space gameplay profile, even if all of its previous shells were already spent.
		if ((!candidate->records.empty() || candidate->next_generation != 1) &&
			(schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Space pods require selected profile");
		if (!ValidateSpacePodRecords(candidate->records, candidate->next_generation, Map::SizeX(), Map::SizeY())) SlErrorCorrupt("Invalid passive Space pod receipt");
	}
	/** @param length Framework family extent; ReadRecords independently verifies exact bounded framing before copying preview rows. */
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
