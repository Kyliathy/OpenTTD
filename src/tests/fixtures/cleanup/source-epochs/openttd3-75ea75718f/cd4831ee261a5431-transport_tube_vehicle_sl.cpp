/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file transport_tube_vehicle_sl.cpp Saved387 capsule bindings and common interval tick, also driven by391 collectors without capsules.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group2
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../transport_tube.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../safeguards.h"

/** Explicit physical binding transport; packets, invoices and electricity remain in their own saved families. */
struct TubeVehicleWire {
	uint32_t consumer = UINT32_MAX, source = UINT32_MAX, vehicle = UINT32_MAX; ///< Issued ledger identities and actual native front.
	uint16_t remainder = 0; ///< Admitted0..999 demand numerator.
	uint64_t prepared = UINT64_MAX, consumed = UINT64_MAX; ///< Global/front opportunity history; sentinel means no opportunity yet.
	uint8_t phase = 0, wait = 0, active = 0; ///< Checked native phase, separate electrical wait and physical binding liveness.
	uint32_t geothermal_source = UINT32_MAX, local_feed = UINT32_MAX; ///<434 original native provider/feed, default absent for genuine older external bindings.
};

static const SaveLoad _tube_vehicle_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("consumer", SLE_OBJECT_ADDRESS(TubeVehicleWire, consumer)),
	SaveLoad::Variable<VarFileType::U32>("source", SLE_OBJECT_ADDRESS(TubeVehicleWire, source)),
	SaveLoad::Variable<VarFileType::U32>("vehicle", SLE_OBJECT_ADDRESS(TubeVehicleWire, vehicle)),
	SaveLoad::Variable<VarFileType::U16>("remainder", SLE_OBJECT_ADDRESS(TubeVehicleWire, remainder)),
	SaveLoad::Variable<VarFileType::U64>("prepared", SLE_OBJECT_ADDRESS(TubeVehicleWire, prepared)),
	SaveLoad::Variable<VarFileType::U64>("consumed", SLE_OBJECT_ADDRESS(TubeVehicleWire, consumed)),
	SaveLoad::Variable<VarFileType::U8>("phase", SLE_OBJECT_ADDRESS(TubeVehicleWire, phase)),
	SaveLoad::Variable<VarFileType::U8>("wait", SLE_OBJECT_ADDRESS(TubeVehicleWire, wait)),
	SaveLoad::Variable<VarFileType::U8>("active", SLE_OBJECT_ADDRESS(TubeVehicleWire, active)),
	SaveLoad::Variable<VarFileType::U32>("geothermal_source", SLE_OBJECT_ADDRESS(TubeVehicleWire, geothermal_source), SaveLoadVersion::TubeGeothermalBindings),
	SaveLoad::Variable<VarFileType::U32>("local_feed", SLE_OBJECT_ADDRESS(TubeVehicleWire, local_feed), SaveLoadVersion::TubeGeothermalBindings),
};

static std::array<bool, 2> _tube_vehicle_presence{}; ///< Unpublished actual family presence.
static uint32_t _tube_vehicle_expected = 0; ///< Exact bounded physical row cardinality.
static uint64_t _tube_vehicle_tick = UINT64_MAX; ///< Unaccepted global request tick.
static std::vector<TubePowerVehicle> _tube_vehicle_rows; ///< Unpublished copied records, never a second movement owner.

/** @copydoc ResetTubePowerVehicleLoadStaging */
void ResetTubePowerVehicleLoadStaging()
{
	_tube_vehicle_presence.fill(false); _tube_vehicle_expected = 0; _tube_vehicle_tick = UINT64_MAX; _tube_vehicle_rows.clear();
}

/** @param preview Select independent copied transport. @return Complete presence; genuine older absence restores no implicit powered binding. */
static bool CheckTubeVehicleFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.tube_vehicle_present : _tube_vehicle_presence;
	const auto expected = preview ? _load_check_data.tube_vehicle_expected : _tube_vehicle_expected;
	const auto tick = preview ? _load_check_data.tube_vehicle_tick : _tube_vehicle_tick;
	const auto &rows = preview ? _load_check_data.tube_vehicle_rows : _tube_vehicle_rows;
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const auto flags = preview ? _load_check_data.multilayer_feature_flags : metadata == nullptr ? 0 : metadata->feature_flags;
	const bool required = (flags & MULTILAYER_FEATURE_TUBE_VEHICLES) != 0;
	if (!presence[0]) {
		// New/explicit/partial families cannot silently omit native metering.
		// Genuine386 and earlier files restore no implicit capsule consumer.
		if (!IsSavegameVersionBefore(SaveLoadVersion::TubeMovementPower) || presence[1] || required) SlErrorCorrupt("Missing capsule power header");
		return false;
	}
	if (!presence[1] || rows.size() != expected || required != !rows.empty()) SlErrorCorrupt("Incomplete capsule power family"); // Exact saved counts retain physical consumer lineage and demand carry.
	const auto *utility = preview ? &_load_check_data.utility_power_state : nullptr;
	const auto connections = preview ? std::span<const TubePowerConnection>{_load_check_data.tube_power_rows} : std::span<const TubePowerConnection>{};
	const auto *geothermal = preview ? &_load_check_data.tube_geothermal_state : nullptr;
	if (ValidateTubePowerVehicles(rows, false, utility, connections, geothermal) != TubePowerError::None) SlErrorCorrupt("Invalid capsule power structure");
	// Before391 only capsules could drive this clock. Collector-only worlds
	// and their retained post-demolition history now use the same saved driver.
	if (IsSavegameVersionBefore(SaveLoadVersion::GeothermalHeatSources) && rows.empty() && tick != UINT64_MAX) SlErrorCorrupt("Unexpected empty capsule power tick history");
	// Each front's prepared work belongs to the same or an earlier complete
	// global request tick; preview cannot borrow the live world's clock.
	for (const auto &row : rows) {
		if (row.prepared_tick != UINT64_MAX && (tick == UINT64_MAX || row.prepared_tick > tick)) SlErrorCorrupt("Invalid capsule global tick");
	}
	return true;
}

/** @copydoc ValidateTubePowerVehicleLoadPreview */
void ValidateTubePowerVehicleLoadPreview() { CheckTubeVehicleFamily(true); }

/** Fixed387 header owns exact physical cardinality and global tick history. */
struct TUVHChunkHandler final : ChunkHandler {
	/** Register the fixed16-byte physical header. */
	TUVHChunkHandler() : ChunkHandler("TUVH", ChunkType::Riff) {}
	/** Save real cardinality and tick without renewing grants or power. */
	void Save() const override
	{
		std::array<uint32_t, 2> counts{1, static_cast<uint32_t>(CaptureTubePowerVehicles().size())};
		std::array<uint64_t, 1> tick{GetTubePowerPreparedTick()};
		SlSetLength(16); SlCopy<VarFileType::U32>(counts); SlCopy<VarFileType::U64>(tick);
	}
	/** @param preview Decode copied exact bounds before physical records are allocated. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::TubeMovementPower) || SlGetFieldLength() != 16) SlErrorCorrupt("Invalid capsule power header version"); // Older formats cannot introduce a current native epoch.
		std::array<uint32_t, 2> counts; std::array<uint64_t, 1> tick;
		SlCopy<VarFileType::U32>(counts); SlCopy<VarFileType::U64>(tick);
		if (counts[0] != 1 || counts[1] > MAX_UTILITY_INTERVAL_ACCOUNTS) SlErrorCorrupt("Invalid capsule power bounds"); // Hostile retained counts refuse before table growth.
		auto &presence = preview ? _load_check_data.tube_vehicle_present : _tube_vehicle_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate capsule power header");
		presence[0] = true;
		(preview ? _load_check_data.tube_vehicle_expected : _tube_vehicle_expected) = counts[1];
		(preview ? _load_check_data.tube_vehicle_tick : _tube_vehicle_tick) = tick[0];
	}
	/** Decode unpublished full-load bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader validates exact length. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage complete transport for later actual vehicle/source acceptance. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (CheckTubeVehicleFamily(false)) StageLoadedTubePowerVehicles(_tube_vehicle_rows, _tube_vehicle_tick);
	}
};

/** Exact physical rows in ledger consumer order, independent of native vehicle pool insertion. */
struct TUVDChunkHandler final : ChunkHandler {
	/** Register bounded capsule binding table. */
	TUVDChunkHandler() : ChunkHandler("TUVD", ChunkType::Table) {}
	/** Save each actual retained association once, with no duplicated electrical balance. */
	void Save() const override
	{
		SlTableHeader(_tube_vehicle_desc); uint32_t ordinal = 0;
		// Source/capsule lineage, carry and opportunity history have one
		// physical record; packets and invoices keep their separate owners.
		for (const auto &row : CaptureTubePowerVehicles()) {
			TubeVehicleWire wire{row.consumer.base(), row.source.base(), row.vehicle.base(), row.remainder_mj,
				row.prepared_tick, row.consumed_tick, to_underlying(row.phase), to_underlying(row.wait), row.active,
				row.geothermal_source.base(), row.local_feed.base()};
			SlSetArrayIndex(ordinal++); SlObject(&wire, _tube_vehicle_desc);
		}
	}
	/** @param preview Decode bounded copied transport into independent preview/staging only. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::TubeMovementPower)) SlErrorCorrupt("Unexpected capsule power rows");
		auto &presence = preview ? _load_check_data.tube_vehicle_present : _tube_vehicle_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate capsule power table");
		const auto desc = SlTableHeader(_tube_vehicle_desc);
		const bool legacy = IsSavegameVersionBefore(SaveLoadVersion::TubeGeothermalBindings);
		if (desc.size() != std::size(_tube_vehicle_desc) - (legacy ? 2 : 0)) SlErrorCorrupt("Invalid capsule power fields");
		// Missing scalars cannot become a fresh allocation or silently discard
		// the previously paid native work's fractional demand numerator.
		for (const auto &field : _tube_vehicle_desc) {
			const std::string_view name{field.name};
			if (legacy && (name == "geothermal_source" || name == "local_feed")) continue; // Genuine older external rows have neither provider field; every current row must explicitly carry both original-role sentinels or identities.
			if (std::ranges::find(desc, field.name, &SaveLoad::name) == desc.end()) SlErrorCorrupt("Missing capsule power field");
		}
		auto &rows = preview ? _load_check_data.tube_vehicle_rows : _tube_vehicle_rows;
		uint32_t ordinal = 0;
		// Stable exact ordinals and issued consumers preserve one association
		// per actual capsule without default, duplicate or reordered identities.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (ordinal >= MAX_UTILITY_INTERVAL_ACCOUNTS || index != static_cast<int>(ordinal++)) SlErrorCorrupt("Invalid capsule power ordinal");
			TubeVehicleWire wire; SlObject(&wire, desc);
			if (wire.active > 1 || (!rows.empty() && wire.consumer <= rows.back().consumer.base())) SlErrorCorrupt("Invalid capsule power order"); // Checked wire booleans and source order reject duplicate/default bindings.
			rows.push_back({UtilityConsumerID{wire.consumer}, UtilityProducerID{wire.source}, VehicleID{wire.vehicle}, wire.remainder,
				wire.prepared, wire.consumed, static_cast<TubePowerPhase>(wire.phase), static_cast<TubePowerWait>(wire.wait), wire.active != 0,
				TubeGeothermalSourceID{wire.geothermal_source}, UtilityLinkID{wire.local_feed}});
		}
		presence[1] = true;
	}
	/** Decode copied full-load rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader validates row boundaries. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const TUVHChunkHandler TUVH;
static const TUVDChunkHandler TUVD;
static const ChunkHandlerRef tube_vehicle_handlers[] = {TUVH, TUVD};
extern const ChunkHandlerTable _tube_vehicle_chunk_handlers(tube_vehicle_handlers);
