/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_power_sl.cpp Bounded explicit physical inlet/path and fractional invoice transport; electrical accounts stay in UPLH/UPLD.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group5
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md schema ownership
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 2 (GCC-portable header bounds)
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../transport_tube.h"
#include "../multilayer_state.h"
#include "../fios.h"

#include "../safeguards.h"

/** Fixed discriminated scalar row; source IDs come exclusively from the common ledger. */
struct TubePowerWire {
	uint8_t kind = 0; ///< Zero connection, one subordinate path cell.
	uint32_t source = UINT32_MAX; ///< Exact original external producer identity.
	uint8_t owner = 0; ///< Actual invoice owner, zero for path rows.
	uint32_t portal = 0, interchange = 0; ///< Actual world endpoints, zero for path rows.
	uint64_t revision = 0; ///< Exact commissioning revision, zero for path rows.
	int32_t lease_day = 0; ///< Last invoiced economy day, zero for path rows.
	uint16_t remainder = 0; ///< Unpaid usage numerator, zero for path rows.
	uint64_t invoice = 0; ///< Paid Money history, zero for path rows.
	uint8_t active = 0; ///< Physical connection liveness, zero for path rows.
	uint32_t position = 0, tile = 0; ///< Exact ordered path index/world address, zero for connection rows.
};

static const SaveLoad _tube_power_desc[] = {
	SaveLoad::Variable<VarFileType::U8>("kind", SLE_OBJECT_ADDRESS(TubePowerWire, kind)),
	SaveLoad::Variable<VarFileType::U32>("source", SLE_OBJECT_ADDRESS(TubePowerWire, source)),
	SaveLoad::Variable<VarFileType::U8>("owner", SLE_OBJECT_ADDRESS(TubePowerWire, owner)),
	SaveLoad::Variable<VarFileType::U32>("portal", SLE_OBJECT_ADDRESS(TubePowerWire, portal)),
	SaveLoad::Variable<VarFileType::U32>("interchange", SLE_OBJECT_ADDRESS(TubePowerWire, interchange)),
	SaveLoad::Variable<VarFileType::U64>("revision", SLE_OBJECT_ADDRESS(TubePowerWire, revision)),
	SaveLoad::Variable<VarFileType::I32>("lease_day", SLE_OBJECT_ADDRESS(TubePowerWire, lease_day)),
	SaveLoad::Variable<VarFileType::U16>("remainder", SLE_OBJECT_ADDRESS(TubePowerWire, remainder)),
	SaveLoad::Variable<VarFileType::U64>("invoice", SLE_OBJECT_ADDRESS(TubePowerWire, invoice)),
	SaveLoad::Variable<VarFileType::U8>("active", SLE_OBJECT_ADDRESS(TubePowerWire, active)),
	SaveLoad::Variable<VarFileType::U32>("position", SLE_OBJECT_ADDRESS(TubePowerWire, position)),
	SaveLoad::Variable<VarFileType::U32>("tile", SLE_OBJECT_ADDRESS(TubePowerWire, tile)),
};

static std::array<bool, 2> _tube_power_presence{}; ///< Unpublished actual header/table presence.
static std::array<uint32_t, 2> _tube_power_expected{}; ///< Exact connection/path cardinality; checked before allocation.
static std::vector<TubePowerConnection> _tube_power_rows; ///< Parsed physical transport, never a live energy store.

/** @copydoc ResetTubePowerLoadStaging */
void ResetTubePowerLoadStaging()
{
	_tube_power_presence.fill(false); _tube_power_expected.fill(0); _tube_power_rows.clear();
}

/** @param preview Select copied independent preview bounds. @return Actual/candidate world tile count, with dimensions checked before multiplication. */
static uint64_t TubePowerWireTileCount(bool preview)
{
	if (!preview) return Map::Size();
	const auto &data = _load_check_data;
	// Hostile dimensions must refuse before multiplication or world-address
	// comparisons; preview never borrows the currently running game's map.
	if (data.map_size_x < 64 || data.map_size_y < 64 || data.map_size_x > 4096 || data.map_size_y > 4096 ||
		data.map_layers == 0 || data.map_layers > MAX_MAP_LAYERS) SlErrorCorrupt("Invalid tube power preview map");
	return static_cast<uint64_t>(data.map_size_x) * data.map_size_y * data.map_layers;
}

/** @param preview Select independent parsed transport. @return Complete family presence; legitimate pre386 absence has no paid service. */
static bool CheckTubePowerFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.tube_power_present : _tube_power_presence;
	const auto &expected = preview ? _load_check_data.tube_power_expected : _tube_power_expected;
	const auto &rows = preview ? _load_check_data.tube_power_rows : _tube_power_rows;
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : metadata == nullptr ? 0 : metadata->feature_flags;
	const bool required = (flags & MULTILAYER_FEATURE_TUBE_POWER) != 0;
	if (!presence[0]) {
		// A new/explicit/partial family cannot silently migrate to free external
		// service. Genuine older worlds retain no inlet or unpaid tariff.
		if (!IsSavegameVersionBefore(SaveLoadVersion::TubeExternalPower) || presence[1] || required) SlErrorCorrupt("Missing tube power header");
		return false;
	}
	size_t cells = 0;
	for (const auto &row : rows) cells += row.utility_path.size();
	// Missing rows/cells or a mismatched feature bit cannot delete unpaid
	// invoice history or supply an undeclared physical connection at load.
	if (!presence[1] || rows.size() != expected[0] || cells != expected[1] || required != !rows.empty()) SlErrorCorrupt("Incomplete tube power family");
	MultiLayerProfileSelection profile;
	profile.id = preview ? static_cast<MultiLayerGameplayProfileID>(_load_check_data.gameplay_profile_id) :
		metadata == nullptr ? MultiLayerGameplayProfileID::Classic : metadata->profile.id;
	const auto *utility = preview ? &_load_check_data.utility_power_state : nullptr;
	if (ValidateTubePowerConnections(rows, profile, false, utility) != TubePowerError::None) SlErrorCorrupt("Invalid tube power structure");
	return true;
}

/** @copydoc ValidateTubePowerLoadPreview */
void ValidateTubePowerLoadPreview() { CheckTubePowerFamily(true); }

/** Version386 header owns exact connection/path bounds and stages the complete family only. */
struct TUPHChunkHandler final : ChunkHandler {
	/** Register fixed twelve-byte physical service header. */
	TUPHChunkHandler() : ChunkHandler("TUPH", ChunkType::Riff) {}
	/** Save actual physical and invoice cardinality; no electrical balances are repeated. */
	void Save() const override
	{
		const auto rows = CaptureTubePowerConnections();
		uint32_t cells = 0;
		for (const auto &row : rows) cells += static_cast<uint32_t>(row.utility_path.size());
		std::array<uint32_t, 3> header{1, static_cast<uint32_t>(rows.size()), cells};
		SlSetLength(12); SlCopy<VarFileType::U32>(header);
	}
	/** @param preview Decode independent header bounds before any physical record can be allocated. */
	void Read(bool preview) const
	{
		// Only the explicit version/revision authorizes these invoices; oversized
		// counts refuse before wire tables or path storage are populated.
		if (IsSavegameVersionBefore(SaveLoadVersion::TubeExternalPower) || SlGetFieldLength() != 12) SlErrorCorrupt("Invalid tube power header version");
		std::array<uint32_t, 3> header;
		SlCopy<VarFileType::U32>(header);
		if (header[0] != 1 || header[1] > MAX_TUBE_POWER_CONNECTIONS || header[2] > MAX_TUBE_POWER_PATH_CELLS) SlErrorCorrupt("Invalid tube power counts");
		auto &presence = preview ? _load_check_data.tube_power_present : _tube_power_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate tube power header");
		presence[0] = true;
		// Bind the destination first: GCC 13 rejects a braced-list assignment to a conditional lvalue.
		auto &expected = preview ? _load_check_data.tube_power_expected : _tube_power_expected;
		expected = {header[1], header[2]};
	}
	/** Stage full-load header without publishing physical services. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader validates exact header size. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage only complete structural authority; actual portal and electrical references resolve in AfterLoadGame. */
	void FixPointers() const override
	{
		// Failed full reconstruction revokes partly published inlets, bindings
		// and their pending families; no old paid service survives a replaced world.
		if (!SlIsFixingPointers()) { ResetTubePowerState(); return; }
		if (CheckTubePowerFamily(false)) StageLoadedTubePowerConnections(_tube_power_rows);
	}
};

/** Discriminated connection/path table; missing/default fields cannot fabricate paid service. */
struct TUPDChunkHandler final : ChunkHandler {
	/** Register bounded physical service table. */
	TUPDChunkHandler() : ChunkHandler("TUPD", ChunkType::Table) {}
	/** Save each source-keyed connection followed by every ordered installed path cell. */
	void Save() const override
	{
		SlTableHeader(_tube_power_desc);
		uint32_t ordinal = 0;
		// Parent-first transport preserves invoice/source lineage and explicit
		// corridor ordering without pool indices or a second physical ID allocator.
		for (const auto &row : CaptureTubePowerConnections()) {
			TubePowerWire wire{0, row.producer.base(), row.owner.base(), row.portal.base(), row.interchange.base(),
				row.portal_revision, row.lease_day, row.usage_remainder_j, row.invoiced_money, row.active};
			SlSetArrayIndex(ordinal++); SlObject(&wire, _tube_power_desc);
			uint32_t position = 0;
			for (const auto tile : row.utility_path) {
				TubePowerWire cell; cell.kind = 1; cell.source = row.producer.base(); cell.position = position++; cell.tile = tile.base();
				SlSetArrayIndex(ordinal++); SlObject(&cell, _tube_power_desc);
			}
		}
	}
	/** @param preview Decode bounded copied rows without changing either live physical or electrical owner. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::TubeExternalPower)) SlErrorCorrupt("Unexpected tube power table");
		auto &presence = preview ? _load_check_data.tube_power_present : _tube_power_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate tube power table");
		auto &rows = preview ? _load_check_data.tube_power_rows : _tube_power_rows;
		const auto desc = SlTableHeader(_tube_power_desc);
		if (desc.size() != std::size(_tube_power_desc)) SlErrorCorrupt("Invalid tube power fields");
		// Every actual scalar is mandatory; missing descriptors cannot renew
		// a lease, discard a usage fraction or replace the commissioned revision.
		for (const auto &field : _tube_power_desc) {
			if (std::ranges::find(desc, field.name, &SaveLoad::name) == desc.end()) SlErrorCorrupt("Missing tube power field");
		}
		const auto tiles = TubePowerWireTileCount(preview);
		uint32_t ordinal = 0, cells = 0;
		// Stable bounded parent/child rows reject duplicate invoices, misplaced
		// corridor cells and hostile references before staged owner acceptance.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (ordinal >= MAX_TUBE_POWER_CONNECTIONS + MAX_TUBE_POWER_PATH_CELLS || index != static_cast<int>(ordinal++)) SlErrorCorrupt("Invalid tube power ordinal");
			TubePowerWire wire;
			SlObject(&wire, desc);
			if (wire.kind == 0) {
				// Increasing ledger identities keep physical invoice order canonical;
				// parent rows carry no hidden path position or unused address.
				if (rows.size() >= MAX_TUBE_POWER_CONNECTIONS || wire.position != 0 || wire.tile != 0 || wire.active > 1 ||
					wire.portal >= tiles || wire.interchange >= tiles || (!rows.empty() && wire.source <= rows.back().producer.base())) SlErrorCorrupt("Invalid tube power connection");
				TubePowerConnection row;
				row.producer = UtilityProducerID{wire.source}; row.owner = CompanyID{wire.owner};
				row.portal = TileIndex{wire.portal}; row.interchange = TileIndex{wire.interchange}; row.portal_revision = wire.revision;
				row.lease_day = wire.lease_day; row.usage_remainder_j = wire.remainder; row.invoiced_money = wire.invoice; row.active = wire.active != 0;
				rows.push_back(std::move(row));
			} else {
				// A path row must name its preceding parent and next exact position;
				// nonzero invoice fields cannot hide another service within a corridor.
				if (wire.kind != 1 || rows.empty() || wire.source != rows.back().producer.base() ||
					wire.position != rows.back().utility_path.size() || wire.position >= MAX_TUBE_POWER_PATH ||
					++cells > MAX_TUBE_POWER_PATH_CELLS || wire.tile >= tiles || wire.owner != 0 || wire.portal != 0 ||
					wire.interchange != 0 || wire.revision != 0 || wire.lease_day != 0 || wire.remainder != 0 || wire.invoice != 0 || wire.active != 0) SlErrorCorrupt("Invalid tube power path cell");
				rows.back().utility_path.push_back(TileIndex{wire.tile});
			}
		}
		presence[1] = true;
	}
	/** Decode unpublished full-load transport. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader validates row boundaries. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const TUPHChunkHandler TUPH;
static const TUPDChunkHandler TUPD;
static const ChunkHandlerRef tube_power_handlers[] = {TUPH, TUPD};
extern const ChunkHandlerTable _tube_power_chunk_handlers(tube_power_handlers);
