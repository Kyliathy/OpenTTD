/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_portal_work_sl.cpp Bounded canonical work records, explicit legacy migration and independent preview. @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 2 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../excavation_state.h"
#include "../multilayer_state.h"
#include "../fios.h"

#include "../safeguards.h"

/** Exact 26-byte scalar transport; geometry and actor roles retain their real owners. */
struct PortalWorkWire {
	uint32_t key = UINT32_MAX; ///< Canonical physical endpoint reference.
	uint8_t state = 0, company = 255; ///< Checked work phase and actual company identity.
	uint32_t project = UINT32_MAX, cutter = VehicleID::Invalid().base(), origin = StationID::Invalid().base(); ///< Explicit actual owner references.
	uint64_t revision = 1; ///< Saved monotonic transition stamp.
};

static const SaveLoad _portal_work_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("key", SLE_OBJECT_ADDRESS(PortalWorkWire, key)),
	SaveLoad::Variable<VarFileType::U8>("state", SLE_OBJECT_ADDRESS(PortalWorkWire, state)),
	SaveLoad::Variable<VarFileType::U8>("company", SLE_OBJECT_ADDRESS(PortalWorkWire, company)),
	SaveLoad::Variable<VarFileType::U32>("project", SLE_OBJECT_ADDRESS(PortalWorkWire, project)),
	SaveLoad::Variable<VarFileType::U32>("cutter", SLE_OBJECT_ADDRESS(PortalWorkWire, cutter)),
	SaveLoad::Variable<VarFileType::U32>("origin", SLE_OBJECT_ADDRESS(PortalWorkWire, origin)),
	SaveLoad::Variable<VarFileType::U64>("revision", SLE_OBJECT_ADDRESS(PortalWorkWire, revision)),
};

static std::array<bool, 2> _portal_presence{}; ///< Unpublished header/table presence.
static uint32_t _portal_expected = 0; ///< Exact bounded transport cardinality.
static std::vector<LayerPortalWorkRecord> _portal_rows; ///< Unpublished wire authority, never a second live pair store.

/** Clear unpublished transport when the real world initializes. */
void ResetLayerPortalWorkLoadStaging()
{
	_portal_presence.fill(false);
	_portal_expected = 0;
	_portal_rows.clear();
}

/**
 * Validate copied key bounds without using current gameplay state during preview.
 * @param key Explicit saved world address.
 * @param preview Select copied map dimensions rather than live allocation.
 * @return Whether a canonical lower endpoint address fits an upper portal plane.
 */
static bool CheckPortalWireKey(uint32_t key, bool preview)
{
	const uint32_t width = preview ? _load_check_data.map_size_x : Map::SizeX();
	const uint32_t height = preview ? _load_check_data.map_size_y : Map::SizeY();
	const uint32_t layers = preview ? _load_check_data.map_layers : Map::LayerCount();
	// Bound dimensions before multiplication, then permit only Surface/Underground canonical keys; actual reciprocal geometry resolves later.
	if (width < 64 || height < 64 || width > 4096 || height > 4096 || layers < 2 || layers > MAX_MAP_LAYERS) return false;
	return key < static_cast<uint64_t>(width) * height * 2;
}

/**
 * Reconcile header/table/feature identity independently for load and preview.
 * @param preview Select independent copied counts.
 * @return Whether a complete family exists; legitimate old absence returns false.
 */
static bool CheckPortalWorkFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.portal_work_present : _portal_presence;
	const size_t expected = preview ? _load_check_data.portal_work_expected : _portal_expected;
	const size_t count = preview ? _load_check_data.portal_work_rows : _portal_rows.size();
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : (metadata == nullptr ? 0 : metadata->feature_flags);
	const bool required = (flags & MULTILAYER_FEATURE_PORTAL_WORK) != 0;
	if (!presence[0]) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::LayerPortalWork) || presence[1] || required) SlErrorCorrupt("Missing portal work header"); // New/explicit/partial families cannot migrate silently to usable legacy topology.
		return false;
	}
	if (!presence[1] || expected != count || required != (count != 0)) SlErrorCorrupt("Incomplete portal work family"); // Exact saved cardinality and explicit feature must agree before any record can authorize routing.
	return true;
}

/** Validate preview completeness without assigning map/vehicle/geological ownership. */
void ValidateLayerPortalWorkLoadPreview()
{
	CheckPortalWorkFamily(true);
}

/** Header owns the exact family bound and combined reference-completion step. */
struct LPWHChunkHandler : ChunkHandler {
	/** Register independent eight-byte header transport. */
	LPWHChunkHandler() : ChunkHandler("LPWH", ChunkType::Riff) {}
	/** Save only bounded actual cardinality and format revision. */
	void Save() const override
	{
		std::array<uint32_t, 2> header{1, static_cast<uint32_t>(CaptureLayerPortalWorkRecords().size())};
		SlSetLength(8);
		SlCopy<VarFileType::U32>(header);
	}
	/** @param preview Read copied bounds independently of the actual store. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::LayerPortalWork) || SlGetFieldLength() != 8) SlErrorCorrupt("Invalid portal work header version/length"); // Older formats cannot request new work authority.
		std::array<uint32_t, 2> header;
		SlCopy<VarFileType::U32>(header);
		if (header[0] != 1 || header[1] > MAX_LAYER_PORTAL_WORK_PAIRS) SlErrorCorrupt("Invalid portal work header bounds"); // Unknown/oversized records refuse before table allocation.
		auto &presence = preview ? _load_check_data.portal_work_present : _portal_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate portal work header");
		presence[0] = true;
		(preview ? _load_check_data.portal_work_expected : _portal_expected) = header[1];
	}
	/** Stage unpublished bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader owns exact length checks. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Validate complete actual topology/references after all chunks exist, without publishing accepted policy. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		const bool present = CheckPortalWorkFamily(false);
		const auto *metadata = GetPendingMultiLayerStateSchema();
		const auto profile = metadata == nullptr ? MultiLayerProfileSelection{} : metadata->profile;
		const auto *staged = GetStagedGeologyWorldState();
		const auto &geology = staged == nullptr ? GetGeologyWorldState() : *staged;
		auto records = _portal_rows;
		if (profile.id == MultiLayerGameplayProfileID::MultilayerIndustry) {
			// Native topology requires one actual record per pair. Older absence migrates existing geometry rather than generating rock or a project.
			for (uint32_t index = 0; index < Map::Size(); ++index) {
				const TileIndex tile{index};
				if (!IsLayerPortalTile(tile)) continue;
				const auto key = GetLayerPortalPairKey(tile);
				if (!key) SlErrorCorrupt("Invalid portal work topology");
				if (*key != tile) continue;
				if (present) {
					if (std::ranges::find(records, *key, &LayerPortalWorkRecord::pair_key) == records.end()) SlErrorCorrupt("Missing native portal work record");
				} else {
					if (records.size() >= MAX_LAYER_PORTAL_WORK_PAIRS) SlErrorCorrupt("Legacy native portal work exceeds bound");
					const auto pair = GetLayerPortalPairTile(tile);
					const std::array source_floor{tile, GetLayerPortalDoorTile(tile)};
					const std::array target_floor{pair, GetLayerPortalDoorTile(pair)};
					auto migration_policy = profile;
					migration_policy.editor_override = true; // Migration assesses only retained physical excavation; the saved real calendar policy is not changed.
					const bool open = CheckExcavatedFootprint(geology, migration_policy, TimerGameCalendar::date, source_floor).error == ExcavationError::None && CheckExcavatedFootprint(geology, migration_policy, TimerGameCalendar::date, target_floor).error == ExcavationError::None;
					const bool ownerless = GetTileOwner(tile) == OWNER_NONE && GetLayerPortalTransportType(tile) == TransportType::Road; // Pre-work-schema Native bankruptcy may retain road housing without a company; migrate it safely sealed rather than inventing an owner or traffic permission.
					records.push_back({*key, !ownerless && open ? LayerPortalWorkState::Connected : LayerPortalWorkState::Sealed, ownerless ? CompanyID::Invalid() : CompanyID{GetTileOwner(tile)}, ExcavationJobID{UINT32_MAX}, VehicleID::Invalid(), StationID::Invalid(), 1});
				}
			}
		}
		if (ValidateLayerPortalWorkRecords(records, profile, geology, true) != LayerPortalWorkError::None) SlErrorCorrupt("Invalid portal work authority");
		StageLoadedLayerPortalWork(std::move(records));
	}
};

/** Exact mandatory scalar rows, bounded before unresolved project/vehicle/station references are accepted. */
struct LPWKChunkHandler : ChunkHandler {
	/** Register canonical work-row table. */
	LPWKChunkHandler() : ChunkHandler("LPWK", ChunkType::Table) {}
	/** Save each actual pair once in canonical key order. */
	void Save() const override
	{
		SlTableHeader(_portal_work_desc);
		uint32_t ordinal = 0;
		for (const auto &record : CaptureLayerPortalWorkRecords()) {
			PortalWorkWire wire{record.pair_key.base(), to_underlying(record.state), record.company.base(), record.project.base(), record.cutter.base(), record.origin.base(), record.revision};
			SlSetArrayIndex(ordinal++);
			SlObject(&wire, _portal_work_desc);
		}
	}
	/** @param preview Validate independent copied rows without assigning actual map or actor state. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::LayerPortalWork)) SlErrorCorrupt("Unexpected portal work rows");
		const auto description = SlTableHeader(_portal_work_desc);
		if (description.size() != std::size(_portal_work_desc)) SlErrorCorrupt("Invalid portal work field count");
		// Missing descriptors cannot become default Connected work or fabricated unassigned actor references.
		for (const auto &field : _portal_work_desc) {
			if (std::ranges::find(description, field.name, &SaveLoad::name) == description.end()) SlErrorCorrupt("Missing portal work field");
		}
		uint32_t count = 0, previous = UINT32_MAX;
		// Explicit keys, phases and stamps are checked before any row enters unpublished transport.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_LAYER_PORTAL_WORK_PAIRS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid portal work ordinal/count");
			PortalWorkWire wire;
			SlObject(&wire, description);
			if (!CheckPortalWireKey(wire.key, preview) || (count != 0 && wire.key <= previous) || wire.state > 2 || (wire.company >= MAX_COMPANIES && wire.company != CompanyID::Invalid().base()) || wire.origin > StationID::Invalid().base() || wire.revision == 0 || wire.revision == UINT64_MAX) SlErrorCorrupt("Invalid portal work fields"); // Invalid company is an explicit ownerless-road candidate only; full actual topology/phase validation must prove that exception before publication.
			previous = wire.key;
			if (!preview) _portal_rows.push_back({TileIndex{wire.key}, static_cast<LayerPortalWorkState>(wire.state), CompanyID{wire.company}, ExcavationJobID{wire.project}, VehicleID{wire.cutter}, StationID{static_cast<StationID::BaseType>(wire.origin)}, wire.revision});
			++count;
		}
		auto &presence = preview ? _load_check_data.portal_work_present : _portal_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate portal work rows");
		presence[1] = true;
		if (preview) _load_check_data.portal_work_rows = count;
	}
	/** Stage copied unresolved records only. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns row-boundary validation. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const LPWHChunkHandler LPWH;
static const LPWKChunkHandler LPWK;
static const ChunkHandlerRef portal_work_handlers[] = {LPWH, LPWK};
extern const ChunkHandlerTable _portal_work_chunk_handlers(portal_work_handlers);
