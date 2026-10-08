/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_collector_sl.cpp Independent389 actual collector construction transport; native industry/map/cargo and geology retain their quantities.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group1B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../geothermal_collector.h"
#include "../geothermal_collector_wire.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include <set>
#include "../safeguards.h"

static const SaveLoad _collector_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("industry", SLE_OBJECT_ADDRESS(GeothermalCollectorWire, industry)),
	SaveLoad::Variable<VarFileType::U32>("site", SLE_OBJECT_ADDRESS(GeothermalCollectorWire, site)),
	SaveLoad::Variable<VarFileType::U8>("owner", SLE_OBJECT_ADDRESS(GeothermalCollectorWire, owner)),
	SaveLoad::Variable<VarFileType::U16>("work_ticks", SLE_OBJECT_ADDRESS(GeothermalCollectorWire, work_ticks)),
	SaveLoad::Variable<VarFileType::U8>("materials_committed", SLE_OBJECT_ADDRESS(GeothermalCollectorWire, materials_committed)),
	SaveLoad::Variable<VarFileType::U8>("has_work_tick", SLE_OBJECT_ADDRESS(GeothermalCollectorWire, has_work_tick)),
	SaveLoad::Variable<VarFileType::U64>("last_work_tick", SLE_OBJECT_ADDRESS(GeothermalCollectorWire, last_work_tick)),
};

static std::array<bool, 2> _collector_presence{}; ///< Exact unpublished header/table presence.
static uint32_t _collector_expected = 0; ///< Header's bounded actual binding count.
static std::vector<GeothermalCollectorWire> _collector_rows; ///< Copied construction scalars, invisible to native work/source queries.

/** @copydoc ResetGeothermalCollectorLoadStaging */
void ResetGeothermalCollectorLoadStaging()
{
	_collector_presence.fill(false); _collector_expected = 0; _collector_rows.clear();
}

/** @param preview Inspect independent copied references rather than full-load staging. @return Whole exact family presence/scalar admission; genuine pre389 absence creates no actual collector. */
static bool CheckGeothermalCollectorFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.geothermal_collector_present : _collector_presence;
	const auto expected = preview ? _load_check_data.geothermal_collector_expected : _collector_expected;
	const auto &rows = preview ? _load_check_data.geothermal_collector_rows : _collector_rows;
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : metadata == nullptr ? 0 : metadata->feature_flags;
	const bool required = (flags & MULTILAYER_FEATURE_GEOTHERMAL_COLLECTORS) != 0;
	if (!presence[0]) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::GeothermalCollectors) || presence[1] || required) SlErrorCorrupt("Missing geothermal collector header"); // Current/explicit/partial construction transport cannot forget paid actual machinery.
		return false;
	}
	if (!presence[1] || rows.size() != expected || required != !rows.empty()) SlErrorCorrupt("Incomplete geothermal collector family"); // Header/table/metadata must describe exactly the same actual participant set.
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector);
	std::set<uint16_t> industries; std::set<uint32_t> sites, sources;
	// Refuse scalars before reconstructing runtime state. Preview additionally
	// resolves original consumed blueprints and native pool identities from this
	// file alone, without consulting the running map, company or cargo pools.
	for (const auto &row : rows) {
		if (row.industry >= 64000 || row.site >= MAX_GEOTHERMAL_SITE_CLAIMS || (row.owner >= MAX_COMPANIES && row.owner != CompanyID::Invalid().base()) ||
			row.work_ticks > terms->work_ticks || row.materials_committed > 1 || row.has_work_tick > 1 ||
			(row.materials_committed != 0) != (row.work_ticks != 0) || (row.has_work_tick != 0) != (row.work_ticks != 0) ||
			(row.has_work_tick == 0 && row.last_work_tick != 0) || !industries.insert(row.industry).second || !sites.insert(row.site).second) SlErrorCorrupt("Invalid geothermal collector scalars"); // Invalid booleans/work or duplicate native/planning IDs cannot fabricate receipts or completed construction.
		if (!preview) continue;
		const auto &cold = _load_check_data.geothermal_site_rows;
		if (row.site >= cold.size() || cold[row.site].active != 0 || cold[row.site].kind != to_underlying(terms->kind) ||
			!sources.insert(cold[row.site].vent).second || !std::ranges::binary_search(_load_check_data.native_industry_preview_ids, row.industry) ||
			(row.owner != CompanyID::Invalid().base() && !_load_check_data.companies.contains(row.owner))) SlErrorCorrupt("Invalid geothermal collector preview references"); // Only this save's native machinery, consumed original collector blueprint and company can establish actual source/control rights.
		for (const auto &claim : cold) {
			if (claim.active != 0 && claim.kind == to_underlying(terms->kind) && claim.vent == cold[row.site].vent) SlErrorCorrupt("Duplicate geothermal collector source reservation"); // Actual machinery remains exclusive even after the old cold claim was consumed.
		}
	}
	return true;
}

/** @copydoc ValidateGeothermalCollectorLoadPreview */
void ValidateGeothermalCollectorLoadPreview() { CheckGeothermalCollectorFamily(true); }

/** Exact8-byte revision/count header; the native industry chunk transports all actual delivered stocks/history. */
struct GCBHChunkHandler final : ChunkHandler {
	/** Register independent actual-construction header. */
	GCBHChunkHandler() : ChunkHandler("GCBH", ChunkType::Riff) {}
	/** Save the actual owner's exact bounded cardinality, including incomplete/abandoned machinery. */
	void Save() const override
	{
		std::array<uint32_t, 2> fields{1, static_cast<uint32_t>(CaptureGeothermalCollectors().size())}; SlSetLength(8); SlCopy<VarFileType::U32>(fields);
	}
	/** @param preview Decode unpublished full-load or independent copied bounds; no native participant is constructed. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::GeothermalCollectors) || SlGetFieldLength() != 8) SlErrorCorrupt("Invalid geothermal collector header version"); // Older formats cannot introduce this actual construction authority.
		std::array<uint32_t, 2> fields; SlCopy<VarFileType::U32>(fields);
		if (fields[0] != 1 || fields[1] > MAX_GEOTHERMAL_SITE_CLAIMS) SlErrorCorrupt("Invalid geothermal collector bounds"); // Bounded cardinality precedes any table allocation.
		auto &presence = preview ? _load_check_data.geothermal_collector_present : _collector_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate geothermal collector header");
		presence[0] = true; (preview ? _load_check_data.geothermal_collector_expected : _collector_expected) = fields[1];
	}
	/** Parse unpublished full-load cardinality. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader validates the header's exact size. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage checked scalar transport; actual native tile/stock/source/company acceptance runs after all existing owners resolve. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!CheckGeothermalCollectorFamily(false)) return;
		std::vector<GeothermalCollectorState> rows; rows.reserve(_collector_rows.size());
		for (const auto &row : _collector_rows) {
			rows.push_back({IndustryID{row.industry}, GeothermalSiteID{row.site}, CompanyID{row.owner}, row.work_ticks,
				row.materials_committed != 0, row.has_work_tick != 0, row.last_work_tick}); // Copy checked construction only; actual native references are never repaired from these scalars.
		}
		StageLoadedGeothermalCollectors(std::move(rows));
	}
};

/** Exact19-byte actual construction rows, independently bounded and ordered by native industry identity. */
struct GCBDChunkHandler final : ChunkHandler {
	/** Register actual-construction table. */
	GCBDChunkHandler() : ChunkHandler("GCBD", ChunkType::Table) {}
	/** Save actual work/receipt/control/tick fields without duplicating delivered stock or natural thermal capacity. */
	void Save() const override
	{
		SlTableHeader(_collector_desc); uint32_t ordinal = 0;
		for (const auto &state : CaptureGeothermalCollectors()) {
			GeothermalCollectorWire wire{state.industry.base(), state.site.base(), state.owner.base(), state.work_ticks, state.materials_committed, state.has_work_tick, state.last_work_tick};
			SlSetArrayIndex(ordinal++); SlObject(&wire, _collector_desc); // One row per actual native participant; no private material/heat ledger is serialized.
		}
	}
	/** @param preview Decode independent copied scalars, checking complete fields/ordinals before any actual participant publishes. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::GeothermalCollectors)) SlErrorCorrupt("Unexpected geothermal collector table");
		auto &presence = preview ? _load_check_data.geothermal_collector_present : _collector_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate geothermal collector table");
		const auto desc = SlTableHeader(_collector_desc);
		if (desc.size() != std::size(_collector_desc)) SlErrorCorrupt("Invalid geothermal collector fields");
		for (const auto &field : _collector_desc) {
			if (std::ranges::find(desc, field.name, &SaveLoad::name) == desc.end()) SlErrorCorrupt("Missing geothermal collector field"); // Missing native/source/work/receipt fields cannot receive implicit free construction.
		}
		auto &rows = preview ? _load_check_data.geothermal_collector_rows : _collector_rows;
		uint32_t ordinal = 0;
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (ordinal >= MAX_GEOTHERMAL_SITE_CLAIMS || index != static_cast<int>(ordinal++)) SlErrorCorrupt("Invalid geothermal collector ordinal"); // Dense bounded wire rows prevent sparse allocation and participant omission.
			GeothermalCollectorWire wire; SlObject(&wire, desc);
			if (!rows.empty() && wire.industry <= rows.back().industry) SlErrorCorrupt("Invalid geothermal collector industry order"); // Stable actual ID order forbids duplicate native owners before staging.
			rows.push_back(wire);
		}
		presence[1] = true;
	}
	/** Parse bounded unpublished actual-construction rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader validates each field and ordinal boundary. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const GCBHChunkHandler GCBH;
static const GCBDChunkHandler GCBD;
static const ChunkHandlerRef geothermal_collector_handlers[] = {GCBH, GCBD};
extern const ChunkHandlerTable _geothermal_collector_chunk_handlers(geothermal_collector_handlers);
