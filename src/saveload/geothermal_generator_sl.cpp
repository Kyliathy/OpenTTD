/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md; original B adapters use explicit lineage capabilities.
 * @file geothermal_generator_sl.cpp Bounded427 construction with appended429 original utility/service transport and independent copied preview admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8C/8DC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../geothermal_generator.h"
#include "../geothermal_generator_wire.h"
#include "../multilayer_state.h"
#include "../map_layer.h"
#include "../fios.h"
#include <set>
#include "../safeguards.h"

static const SaveLoad _generator_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("industry", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, industry)),
	SaveLoad::Variable<VarFileType::U32>("site", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, site)),
	SaveLoad::Variable<VarFileType::U8>("owner", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, owner)),
	SaveLoad::Variable<VarFileType::U8>("model", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, model)),
	SaveLoad::Variable<VarFileType::U16>("work_ticks", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, work_ticks)),
	SaveLoad::Variable<VarFileType::U8>("materials_committed", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, materials_committed)),
	SaveLoad::Variable<VarFileType::U8>("has_work_tick", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, has_work_tick)),
	SaveLoad::Variable<VarFileType::U64>("last_work_tick", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, last_work_tick)),
	SaveLoad::Variable<VarFileType::U32>("heat_consumer", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, heat_consumer)).WithCapability(SaveCapability::GeothermalGeneratorOperation),
	SaveLoad::Variable<VarFileType::U32>("electrical_producer", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, electrical_producer)).WithCapability(SaveCapability::GeothermalGeneratorOperation),
	SaveLoad::Variable<VarFileType::U8>("service_remainder", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, service_remainder)).WithCapability(SaveCapability::GeothermalGeneratorOperation),
	SaveLoad::Variable<VarFileType::U8>("has_operating_tick", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, has_operating_tick)).WithCapability(SaveCapability::GeothermalGeneratorOperation),
	SaveLoad::Variable<VarFileType::U64>("last_operating_tick", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, last_operating_tick)).WithCapability(SaveCapability::GeothermalGeneratorOperation),
	SaveLoad::Variable<VarFileType::U64>("active_service_ticks", SLE_OBJECT_ADDRESS(GeothermalGeneratorWire, active_service_ticks)).WithCapability(SaveCapability::GeothermalGeneratorOperation),
};

static std::array<bool, 2> _generator_presence{}; ///< Exact unpublished header/table presence for the current full load.
static uint32_t _generator_expected = 0; ///< Bounded copied header count, never native allocation authority.
static std::vector<GeothermalGeneratorWire> _generator_rows; ///< Raw unpublished exact scalars; Industry transports all physical stock.

/** @copydoc ResetGeothermalGeneratorLoadStaging */
void ResetGeothermalGeneratorLoadStaging()
{
	_generator_presence.fill(false); _generator_expected = 0; _generator_rows.clear();
}

/** @param preview Use only this preview's copied references. @return Complete scalar/presence admission; genuine pre427 absence creates no machinery or progress. */
static bool CheckGeothermalGeneratorFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.geothermal_generator_present : _generator_presence;
	const auto expected = preview ? _load_check_data.geothermal_generator_expected : _generator_expected;
	const auto &rows = preview ? _load_check_data.geothermal_generator_rows : _generator_rows;
	const auto *metadata = preview ? nullptr : GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : metadata == nullptr ? 0 : metadata->feature_flags;
	const bool required = (flags & MULTILAYER_FEATURE_GEOTHERMAL_GENERATORS) != 0;
	if (!presence[0]) {
		// Current files always carry both family components, including an empty
		// one. Only genuine older absence can mean no actual paid powerhouse.
		if (SlHasSaveCapability(SaveCapability::GeothermalGenerators) || presence[1] || required) SlErrorCorrupt("Missing geothermal generator header");
		return false;
	}
	if (!presence[1] || rows.size() != expected || required != !rows.empty()) SlErrorCorrupt("Incomplete geothermal generator family"); // Header, table and metadata must name exactly the same retained participant set.
	std::set<uint16_t> industries; std::set<uint32_t> sites, inputs, outputs;
	// Several actual powerhouses may share a vent's future bounded allocation,
	// but each must retain its own unique native machinery and consumed blueprint.
	for (const auto &row : rows) {
		// Previous native formats preserve their exact first-model semantics;
		// a new model byte cannot grant2000 machinery under an older version.
		if (row.model == to_underlying(GeothermalGeneratorModel::MagmaLoop) && !SlHasSaveCapability(SaveCapability::GeothermalMagmaLoop)) SlErrorCorrupt("Unexpected older Magma-Loop generator");
		// A malformed or reused industry/site cannot become a second paid powerhouse.
		if (!ValidateGeothermalGeneratorWire(row) || !industries.insert(row.industry).second || !sites.insert(row.site).second) SlErrorCorrupt("Invalid geothermal generator scalars");
		if (!preview) continue;
		const auto &cold = _load_check_data.geothermal_site_rows;
		const auto *terms = GetMultiLayerGeothermalGeneratorTerms(static_cast<GeothermalGeneratorModel>(row.model));
		// Independent preview resolves only this file's copied consumed site,
		// native industry identity and company, without borrowing live inventory.
		if (row.site >= cold.size() || cold[row.site].active != 0 || cold[row.site].kind != to_underlying(terms->construction) ||
			!std::ranges::binary_search(_load_check_data.native_industry_preview_ids, row.industry) ||
			(row.owner != CompanyID::Invalid().base() && !_load_check_data.companies.contains(row.owner))) SlErrorCorrupt("Invalid geothermal generator preview references");
		// Copied preview resolves the complete immutable native/shared pair and
		// declared physical endpoint using only this file's copied families.
		if (!ValidateGeothermalGeneratorOperatingReferences(DecodeGeothermalGenerator(row), _load_check_data.utility_power_state) ||
			(row.heat_consumer != UINT32_MAX && !inputs.insert(row.heat_consumer).second) ||
			(row.electrical_producer != UINT32_MAX && !outputs.insert(row.electrical_producer).second)) SlErrorCorrupt("Invalid geothermal generator operating references");
		if (row.heat_consumer != UINT32_MAX) {
			const auto &site = cold[row.site];
			/** @param port This file's copied physical Heat row. @return Whether native candidate geometry and identity bind this original input to its declared boundary. */
			const auto count = std::ranges::count_if(_load_check_data.geothermal_heat_route_rows, [&row, &site](const auto &port) {
				const auto endpoint = MultiLayerMap::DecodeStoredTile(TileIndex{port.reference}, _load_check_data.map_size_x, _load_check_data.map_size_y, _load_check_data.map_layers);
				return port.kind == 0 && port.consumer == row.heat_consumer && endpoint.has_value() &&
					endpoint->layer == MapLayer::DeepUnderground && endpoint->x == site.endpoint_x && endpoint->y == site.endpoint_y;
			});
			if (count != 1) SlErrorCorrupt("Invalid geothermal generator physical input");
		}
	}
	if (preview) {
		// A local feed's original plant, converted source and boundary must all
		// come from this file. A running world's coincident native ID is irrelevant.
		for (const auto &link : _load_check_data.utility_power_state.links) {
			// Only live local feeds confer current electrical permissions; retired history stays inert.
			if (!link.active || link.kind != UtilityLinkKind::ElectricalFeed) continue;
			const auto plant = std::ranges::find(rows, link.generator.base(), &GeothermalGeneratorWire::industry);
			// Resolve both original participants before borrowing the shared forwarding reservation.
			if (plant == rows.end() || link.source.base() >= _load_check_data.utility_power_state.consumers.size()) SlErrorCorrupt("Missing copied local-feed plant");
			const auto &source = _load_check_data.utility_power_state.consumers[link.source.base()];
			const auto &site = _load_check_data.geothermal_site_rows[plant->site];
			const auto endpoint = MultiLayerMap::DecodeStoredTile(link.source_endpoint, _load_check_data.map_size_x, _load_check_data.map_size_y, _load_check_data.map_layers);
			// Completed original machinery owns one converted output. Another
			// plant's source, a wrong port or foreign reservation cannot adopt it.
			if (source.owner.base() != plant->owner || source.linked_producers != std::vector{UtilityProducerID{plant->electrical_producer}} ||
				!endpoint.has_value() || endpoint->layer != MapLayer::DeepUnderground || endpoint->x != site.endpoint_x || endpoint->y != site.endpoint_y || plant->materials_committed == 0) SlErrorCorrupt("Invalid copied local-feed source");
		}
	}
	return true;
}

/** @copydoc ValidateGeothermalGeneratorLoadPreview */
void ValidateGeothermalGeneratorLoadPreview() { CheckGeothermalGeneratorFamily(true); }

/** Fixed8-byte schema/count for actual retained construction, with no extra stock or utility ledger. */
struct GPGHChunkHandler final : ChunkHandler {
	/** Register independent actual-powerhouse header. */
	GPGHChunkHandler() : ChunkHandler("GPGH", ChunkType::Riff) {}
	/** Save exactly the actual owner's schema and bounded cardinality, including unfinished/abandoned work. */
	void Save() const override
	{
		std::array<uint32_t, 2> fields{2, static_cast<uint32_t>(CaptureGeothermalGenerators().size())}; SlSetLength(8); SlCopy<VarFileType::U32>(fields);
	}
	/** @param preview Decode full-load staging or independent copied bounds before allocating any actual participant. */
	void Read(bool preview) const
	{
		if (!SlHasSaveCapability(SaveCapability::GeothermalGenerators) || SlGetChunkType() != ChunkType::Riff || SlGetFieldLength() != 8) SlErrorCorrupt("Invalid geothermal generator header version"); // An older format cannot acquire this new paid-work authority.
		std::array<uint32_t, 2> fields; SlCopy<VarFileType::U32>(fields);
		const uint32_t schema = !SlHasSaveCapability(SaveCapability::GeothermalGeneratorOperation) ? 1 : 2;
		if (fields[0] != schema || fields[1] > MAX_GEOTHERMAL_SITE_CLAIMS) SlErrorCorrupt("Invalid geothermal generator bounds"); // Exact revision/count prevent implicit future models or unbounded allocations.
		auto &presence = preview ? _load_check_data.geothermal_generator_present : _generator_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate geothermal generator header");
		presence[0] = true; (preview ? _load_check_data.geothermal_generator_expected : _generator_expected) = fields[1];
	}
	/** Parse unpublished current native header. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader validates its exact8-byte length. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage the complete scalar-validated family; actual native floor/stock/control acceptance remains afterload's responsibility. */
	void FixPointers() const override
	{
		// A later physical owner may refuse after this native family accepts.
		// Failed-load Null cleanup must revoke those partly published original
		// construction/operating references before returning ReInit to its caller.
		if (!SlIsFixingPointers()) { ResetGeothermalGenerators(); ResetGeothermalGeneratorLoadStaging(); return; }
		if (!CheckGeothermalGeneratorFamily(false)) return;
		std::vector<GeothermalGeneratorState> states; states.reserve(_generator_rows.size());
		for (const auto &row : _generator_rows) states.push_back(DecodeGeothermalGenerator(row)); // Copy references/work only; actual stock is already owned by native Industry transport.
		StageLoadedGeothermalGenerators(std::move(states));
	}
};

/** Dense20-byte427/428 construction or46-byte429 construction/operating rows, ordered by actual native identity. */
struct GPGDChunkHandler final : ChunkHandler {
	/** Register original-powerhouse construction table. */
	GPGDChunkHandler() : ChunkHandler("GPGD", ChunkType::Table) {}
	/** Serialize only actual work/control/receipt references; native Industry transports materials exactly once. */
	void Save() const override
	{
		SlTableHeader(_generator_desc); uint32_t ordinal = 0;
		for (const auto &state : CaptureGeothermalGenerators()) {
			auto wire = EncodeGeothermalGenerator(state); SlSetArrayIndex(ordinal++); SlObject(&wire, _generator_desc);
		}
	}
	/** @param preview Decode bounded exact copied fields/ordinals without publishing native work, stock or energy. */
	void Read(bool preview) const
	{
		// Only the original TABLE family can restore paid machinery; accepting another framing could discard participant identity.
		if (!SlHasSaveCapability(SaveCapability::GeothermalGenerators) || SlGetChunkType() != ChunkType::Table) SlErrorCorrupt("Unexpected geothermal generator table");
		auto &presence = preview ? _load_check_data.geothermal_generator_present : _generator_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate geothermal generator table");
		const auto desc = SlTableHeader(_generator_desc);
		const bool operating = SlHasSaveCapability(SaveCapability::GeothermalGeneratorOperation);
		if (desc.size() != std::size(_generator_desc) - (operating ? 0 : 6)) SlErrorCorrupt("Invalid geothermal generator fields");
		// Every original identity/receipt field is mandatory exactly once. A
		// missing owner or work flag must never default into abandonment/free labour.
		for (size_t index = 0; index < std::size(_generator_desc) - (operating ? 0 : 6); ++index) {
			const auto &field = _generator_desc[index];
			if (std::ranges::count(desc, field.name, &SaveLoad::name) != 1) SlErrorCorrupt("Missing geothermal generator field");
		}
		auto &rows = preview ? _load_check_data.geothermal_generator_rows : _generator_rows;
		uint32_t ordinal = 0;
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (ordinal >= MAX_GEOTHERMAL_SITE_CLAIMS || index != static_cast<int>(ordinal++)) SlErrorCorrupt("Invalid geothermal generator ordinal"); // Dense bounded rows prevent sparse allocation and silently omitted participants.
			GeothermalGeneratorWire wire; SlObject(&wire, desc);
			if (!rows.empty() && wire.industry <= rows.back().industry) SlErrorCorrupt("Invalid geothermal generator industry order"); // Canonical native ID order rejects duplicates before staging any paid work.
			rows.push_back(wire);
		}
		presence[1] = true;
	}
	/** Parse bounded unpublished native construction. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader validates every explicit field and dense ordinal. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const GPGHChunkHandler GPGH;
static const GPGDChunkHandler GPGD;
static const ChunkHandlerRef geothermal_generator_handlers[] = {GPGH, GPGD};
extern const ChunkHandlerTable _geothermal_generator_chunk_handlers(geothermal_generator_handlers);
