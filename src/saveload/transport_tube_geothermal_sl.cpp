/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md; original B adapters use explicit lineage capabilities.
 * @file transport_tube_geothermal_sl.cpp Explicit433 original geothermal inlet references; shared utility transport alone owns every quantity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BBC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../transport_tube_geothermal.h"
#include "../geothermal_generator.h"
#include "../multilayer_state.h"
#include "../map_layer.h"
#include "../fios.h"
#include "../safeguards.h"

/** Explicit28-byte reference row; raw active is checked before boolean conversion. */
struct TubeGeothermalWire {
	uint32_t id = UINT32_MAX; ///< Original nonreused physical inlet.
	uint16_t generator = UINT16_MAX; ///< Original native plant.
	uint32_t producer = UINT32_MAX, supply = UINT32_MAX; ///< Original shared conversion output and sole forwarding root.
	uint8_t owner = UINT8_MAX; ///< Actual retained controller.
	uint32_t port = UINT32_MAX, inlet = UINT32_MAX; ///< Original plant/native tube boundaries.
	uint16_t depot = UINT16_MAX, station = UINT16_MAX; ///< Exactly one original native facility.
	uint8_t active = 0; ///< Canonical0/1 permission, not runtime readiness.
};

static const SaveLoad _tube_geothermal_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("id", SLE_OBJECT_ADDRESS(TubeGeothermalWire, id)),
	SaveLoad::Variable<VarFileType::U16>("generator", SLE_OBJECT_ADDRESS(TubeGeothermalWire, generator)),
	SaveLoad::Variable<VarFileType::U32>("producer", SLE_OBJECT_ADDRESS(TubeGeothermalWire, producer)),
	SaveLoad::Variable<VarFileType::U32>("supply", SLE_OBJECT_ADDRESS(TubeGeothermalWire, supply)),
	SaveLoad::Variable<VarFileType::U8>("owner", SLE_OBJECT_ADDRESS(TubeGeothermalWire, owner)),
	SaveLoad::Variable<VarFileType::U32>("port", SLE_OBJECT_ADDRESS(TubeGeothermalWire, port)),
	SaveLoad::Variable<VarFileType::U32>("inlet", SLE_OBJECT_ADDRESS(TubeGeothermalWire, inlet)),
	SaveLoad::Variable<VarFileType::U16>("depot", SLE_OBJECT_ADDRESS(TubeGeothermalWire, depot)),
	SaveLoad::Variable<VarFileType::U16>("station", SLE_OBJECT_ADDRESS(TubeGeothermalWire, station)),
	SaveLoad::Variable<VarFileType::U8>("active", SLE_OBJECT_ADDRESS(TubeGeothermalWire, active)),
};

static std::array<bool, 2> _tube_geothermal_presence{}; ///< Unpublished complete-family presence.
static uint32_t _tube_geothermal_expected = 0; ///< Exact bounded retained row count.
static TubeGeothermalPowerState _tube_geothermal_state; ///< Reference-only load staging, never live electrical authority.

/** @copydoc ResetTubeGeothermalPowerLoadStaging */
void ResetTubeGeothermalPowerLoadStaging()
{
	_tube_geothermal_presence.fill(false); _tube_geothermal_expected = 0; _tube_geothermal_state = {};
}

/** @param preview Select independent copied family. @return Whether a complete current family exists; genuine pre433 absence has no original geothermal inlet. */
static bool CheckTubeGeothermalFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.tube_geothermal_present : _tube_geothermal_presence;
	const auto expected = preview ? _load_check_data.tube_geothermal_expected : _tube_geothermal_expected;
	const auto &state = preview ? _load_check_data.tube_geothermal_state : _tube_geothermal_state;
	const auto *metadata = preview ? nullptr : GetPendingMultiLayerStateSchema();
	const auto flags = preview ? _load_check_data.multilayer_feature_flags : metadata == nullptr ? 0 : metadata->feature_flags;
	const bool required = (flags & MULTILAYER_FEATURE_TUBE_GEOTHERMAL) != 0;
	if (!presence[0]) {
		if (SlHasSaveCapability(SaveCapability::TubeGeothermalSources) || presence[1] || required) SlErrorCorrupt("Missing geothermal tube source header"); // Current/partial families cannot silently discard original permissions or history; only genuine older absence migrates empty.
		return false;
	}
	if (!presence[1] || state.sources.size() != expected || state.next_source != expected || state.sequence < expected ||
		state.sequence == UINT64_MAX || (expected == 0 && state.sequence != 0) || required != !state.sources.empty()) SlErrorCorrupt("Incomplete geothermal tube source family"); // Dense bounded history, receipt and metadata must describe exactly the same retained incarnations.
	if (!preview) return true;
	if (!ValidateTubeGeothermalPowerState(state, _load_check_data.utility_power_state, false)) SlErrorCorrupt("Invalid copied geothermal tube supply");
	// Copied original plant/output/consumed-site references resolve only against
	// this file. Native depot/platform geometry resolves later in full afterload.
	for (const auto &source : state.sources) {
		if (!source.active) continue;
		const auto &plants = _load_check_data.geothermal_generator_rows;
		const auto plant = std::ranges::find(plants, source.generator.base(), &GeothermalGeneratorWire::industry);
		if (plant == plants.end()) SlErrorCorrupt("Missing copied geothermal tube plant");
		const auto *terms = GetMultiLayerGeothermalGeneratorTerms(static_cast<GeothermalGeneratorModel>(plant->model));
		const auto &sites = _load_check_data.geothermal_site_rows;
		if (terms == nullptr || plant->site >= sites.size()) SlErrorCorrupt("Invalid copied geothermal tube blueprint"); // Invalid model/site must refuse before deriving any original endpoint or bill.
		const auto &site = sites[plant->site];
		const auto port = MultiLayerMap::DecodeStoredTile(source.port, _load_check_data.map_size_x, _load_check_data.map_size_y, _load_check_data.map_layers);
		if (plant->owner != source.owner.base() || plant->electrical_producer != source.producer.base() || plant->materials_committed != 1 ||
			plant->heat_consumer != _load_check_data.utility_power_state.producers[source.producer.base()].conversion_input.base() ||
			plant->work_ticks != GetMultiLayerGeothermalConstruction(terms->construction)->work_ticks || !port.has_value() || port->layer != MapLayer::DeepUnderground || port->x != site.endpoint_x || port->y != site.endpoint_y) SlErrorCorrupt("Invalid copied geothermal tube plant"); // A different plant/output, incomplete bill or substituted endpoint cannot authorize the saved original source.
	}
	return true;
}

/** @copydoc ValidateTubeGeothermalPowerLoadPreview */
void ValidateTubeGeothermalPowerLoadPreview() { CheckTubeGeothermalFamily(true); }

/** Fixed20-byte source header; it owns only original identity/cardinality/reference receipts. */
struct TGSHChunkHandler final : ChunkHandler {
	/** Register the explicit original source header. */
	TGSHChunkHandler() : ChunkHandler("TGSH", ChunkType::Riff) {}
	/** Save exact issuer and history bounds, without a balance or cached readiness. */
	void Save() const override
	{
		const auto state = CaptureTubeGeothermalPowerState();
		std::array<uint32_t, 5> header{1, state.next_source, static_cast<uint32_t>(state.sequence >> 32), static_cast<uint32_t>(state.sequence), static_cast<uint32_t>(state.sources.size())};
		SlSetLength(20); SlCopy<VarFileType::U32>(header);
	}
	/** @param preview Decode bounded copied/full header before reference allocation. */
	void Read(bool preview) const
	{
		if (!SlHasSaveCapability(SaveCapability::TubeGeothermalSources) || SlGetChunkType() != ChunkType::Riff || SlGetFieldLength() != 20) SlErrorCorrupt("Invalid geothermal tube source header"); // Only explicit433 transport can declare original inlet incarnations.
		std::array<uint32_t, 5> header; SlCopy<VarFileType::U32>(header);
		const auto schema = header[0], next = header[1], count = header[4]; const uint64_t sequence = (uint64_t{header[2]} << 32) | header[3];
		if (schema != 1 || count > MAX_TUBE_GEOTHERMAL_SOURCES || next != count || sequence < count || sequence == UINT64_MAX ||
			(count == 0 && sequence != 0)) SlErrorCorrupt("Invalid geothermal tube source bounds"); // Hostile or omitted dense history refuses before any row is allocated.
		auto &presence = preview ? _load_check_data.tube_geothermal_present : _tube_geothermal_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate geothermal tube source header");
		presence[0] = true;
		(preview ? _load_check_data.tube_geothermal_expected : _tube_geothermal_expected) = count;
		auto &state = preview ? _load_check_data.tube_geothermal_state : _tube_geothermal_state;
		state.next_source = next; state.sequence = sequence;
	}
	/** Decode unpublished original identity bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader checks the exact header length. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage complete transport; native/shared references resolve after original generators accept. */
	void FixPointers() const override
	{
		// A later native owner may refuse after source adoption. ReInit cleanup
		// must revoke its provider and unpublished family rather than retain power rights.
		if (!SlIsFixingPointers()) { ResetTubeGeothermalPowerState(); ResetTubeGeothermalPowerLoadStaging(); return; }
		if (CheckTubeGeothermalFamily(false)) StageLoadedTubeGeothermalPowerState(_tube_geothermal_state);
	}
};

/** Bounded explicit original-reference rows; omitted fields cannot fabricate an active permission. */
struct TGSDChunkHandler final : ChunkHandler {
	/** Register original source table. */
	TGSDChunkHandler() : ChunkHandler("TGSD", ChunkType::Table) {}
	/** Save every retained original incarnation in issued order, including retired history. */
	void Save() const override
	{
		SlTableHeader(_tube_geothermal_desc);
		// Dense issuance keeps retired identity history independent of reusable
		// native pool slots and of the original generator's electrical accounts.
		for (const auto &source : CaptureTubeGeothermalPowerState().sources) {
			TubeGeothermalWire wire{source.id.base(), source.generator.base(), source.producer.base(), source.supply.base(), source.owner.base(),
				source.port.base(), source.inlet.base(), source.depot.base(), source.station.base(), source.active};
			SlSetArrayIndex(source.id.base()); SlObject(&wire, _tube_geothermal_desc);
		}
	}
	/** @param preview Decode this candidate's bounded exact fields without publishing physical or electrical authority. */
	void Read(bool preview) const
	{
		// A provider requires its original TABLE framing; alternate records cannot confer forwarding permission.
		if (!SlHasSaveCapability(SaveCapability::TubeGeothermalSources) || SlGetChunkType() != ChunkType::Table) SlErrorCorrupt("Unexpected geothermal tube source table");
		auto &presence = preview ? _load_check_data.tube_geothermal_present : _tube_geothermal_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate geothermal tube source table");
		auto &state = preview ? _load_check_data.tube_geothermal_state : _tube_geothermal_state;
		const auto desc = SlTableHeader(_tube_geothermal_desc);
		if (desc.size() != std::size(_tube_geothermal_desc)) SlErrorCorrupt("Invalid geothermal tube source fields");
		// Every scalar is mandatory: descriptor defaults cannot remove original
		// history, substitute a pool slot or silently change active permissions.
		for (const auto &field : _tube_geothermal_desc) {
			if (std::ranges::count(desc, field.name, &SaveLoad::name) != 1) SlErrorCorrupt("Missing geothermal tube source field");
		}
		const uint32_t width = preview ? _load_check_data.map_size_x : Map::SizeX(), height = preview ? _load_check_data.map_size_y : Map::SizeY();
		const auto layers = preview ? _load_check_data.map_layers : Map::LayerCount();
		if (width < 64 || height < 64 || width > 4096 || height > 4096 || layers == 0 || layers > MAX_MAP_LAYERS) SlErrorCorrupt("Invalid geothermal tube source map"); // Candidate dimensions must be bounded before multiplication or Deep endpoint checks; preview never borrows the live world.
		// Bound all original addresses and exact raw booleans before decoding;
		// the family and afterload validators then resolve original owner roles.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (state.sources.size() >= MAX_TUBE_GEOTHERMAL_SOURCES || index != static_cast<int>(state.sources.size())) SlErrorCorrupt("Invalid geothermal tube source ordinal");
			TubeGeothermalWire wire; SlObject(&wire, desc);
			const auto port = MultiLayerMap::DecodeStoredTile(TileIndex{wire.port}, width, height, layers);
			const auto inlet = MultiLayerMap::DecodeStoredTile(TileIndex{wire.inlet}, width, height, layers);
			if (wire.id != state.sources.size() || wire.active > 1 || wire.generator >= 64000 || !port.has_value() || !inlet.has_value() ||
				port->layer != MapLayer::DeepUnderground || inlet->layer != MapLayer::DeepUnderground || (wire.depot == UINT16_MAX) == (wire.station == UINT16_MAX)) SlErrorCorrupt("Invalid geothermal tube source row"); // Wrong-plane/replaced reference shapes cannot become native original inlet authority through scalar coercion.
			state.sources.push_back({TubeGeothermalSourceID{wire.id}, IndustryID{wire.generator}, UtilityProducerID{wire.producer}, UtilityConsumerID{wire.supply},
				CompanyID{wire.owner}, TileIndex{wire.port}, TileIndex{wire.inlet}, DepotID{wire.depot}, StationID{wire.station}, wire.active != 0});
		}
		presence[1] = true;
	}
	/** Decode unpublished native reference rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader validates exact descriptors and row boundaries. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const TGSHChunkHandler TGSH;
static const TGSDChunkHandler TGSD;
static const ChunkHandlerRef tube_geothermal_handlers[] = {TGSH, TGSD};
extern const ChunkHandlerTable _tube_geothermal_chunk_handlers(tube_geothermal_handlers);
