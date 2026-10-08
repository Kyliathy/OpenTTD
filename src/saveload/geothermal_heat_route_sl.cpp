/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_heat_route_sl.cpp Exact physical Heat ports and source paths; shared native utility chunks retain every quantity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Groups3-4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md independent preview and complete acceptance
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../geothermal_heat.h"
#include "../geothermal_collector.h"
#include "../fios.h"
#include "../multilayer_state.h"
#include "../safeguards.h"

static const SaveLoad _heat_route_desc[] = {
	SaveLoad::Variable<VarFileType::U8>("kind", SLE_OBJECT_ADDRESS(GeothermalHeatRouteWire, kind)),
	SaveLoad::Variable<VarFileType::U32>("consumer", SLE_OBJECT_ADDRESS(GeothermalHeatRouteWire, consumer)),
	SaveLoad::Variable<VarFileType::U32>("producer", SLE_OBJECT_ADDRESS(GeothermalHeatRouteWire, producer)),
	SaveLoad::Variable<VarFileType::U32>("position", SLE_OBJECT_ADDRESS(GeothermalHeatRouteWire, position)),
	SaveLoad::Variable<VarFileType::U32>("reference", SLE_OBJECT_ADDRESS(GeothermalHeatRouteWire, reference)),
};

static std::array<bool, 2> _heat_route_presence{}; ///< Exact unpublished header/table presence.
static std::array<uint32_t, 3> _heat_route_expected{}; ///< Header's bounded port/path/cell cardinalities.
static uint64_t _heat_route_interval = 0; ///< Exact original shared reservation epoch, never a tick query.
static std::vector<GeothermalHeatRouteWire> _heat_route_rows; ///< Unpublished physical scalars; no source or stock account is constructed.

/** @copydoc ResetGeothermalHeatRouteLoadStaging */
void ResetGeothermalHeatRouteLoadStaging()
{
	_heat_route_presence.fill(false); _heat_route_expected.fill(0); _heat_route_interval = 0; _heat_route_rows.clear();
}

/** Checked physical transport awaiting native owner resolution; it owns no heat or callback. */
struct HeatRouteDecoded {
	std::vector<GeothermalHeatConsumerBinding> bindings; ///< Exact physical port references.
	std::vector<GeothermalHeatRoute> routes; ///< Exact original source path references.
};

/** @param preview Decode copied independent scalars instead of full-load staging. @return Complete reference family, or absence for genuine pre393 transport; no world participant is published. */
static std::optional<HeatRouteDecoded> CheckHeatRouteFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.geothermal_heat_route_present : _heat_route_presence;
	const auto &expected = preview ? _load_check_data.geothermal_heat_route_expected : _heat_route_expected;
	const auto interval = preview ? _load_check_data.geothermal_heat_route_interval : _heat_route_interval;
	const auto &rows = preview ? _load_check_data.geothermal_heat_route_rows : _heat_route_rows;
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const auto flags = preview ? _load_check_data.multilayer_feature_flags : metadata == nullptr ? 0 : metadata->feature_flags;
	const bool required = (flags & MULTILAYER_FEATURE_GEOTHERMAL_HEAT_ROUTES) != 0;
	// Genuine older absence supplies no port or inferred route. Current,
	// explicitly marked or partial families must transport every reference.
	if (!presence[0]) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::GeothermalHeatRoutes) || presence[1] || required) SlErrorCorrupt("Missing geothermal heat route header");
		return std::nullopt;
	}
	if (!presence[1] || rows.size() != static_cast<size_t>(expected[0]) + expected[1] + expected[2] || required != (expected[0] != 0)) SlErrorCorrupt("Incomplete geothermal heat route family"); // Physical authority cannot hide ports/paths by changing only metadata or cardinality.
	HeatRouteDecoded decoded;
	decoded.bindings.reserve(expected[0]); decoded.routes.reserve(expected[1]);
	uint32_t cells = 0; bool paths_started = false;
	// Ports precede paths; each path declaration is immediately followed by
	// dense cardinal cells. Exact order prevents ambiguous slice reconstruction
	// and bounds all allocation before actual native owner acceptance.
	for (const auto &row : rows) {
		if (row.kind == 0) {
			if (paths_started || row.producer != UINT32_MAX || row.position != 0 || decoded.bindings.size() >= expected[0] ||
				(!decoded.bindings.empty() && decoded.bindings.back().consumer.base() >= row.consumer)) SlErrorCorrupt("Invalid geothermal heat port order"); // Issued ports have one fixed role and increasing original identity.
			decoded.bindings.push_back({UtilityConsumerID{row.consumer}, TileIndex{row.reference}});
		} else if (row.kind == 1) {
			paths_started = true;
			if (row.position != 0 || row.reference != UINT32_MAX || decoded.routes.size() >= expected[1] ||
				(!decoded.routes.empty() && (decoded.routes.back().cells.empty() || std::tie(decoded.routes.back().consumer, decoded.routes.back().producer) >= std::tuple{UtilityConsumerID{row.consumer}, UtilityProducerID{row.producer}}))) SlErrorCorrupt("Invalid geothermal heat path order"); // A declared source slice is unique and cannot omit its original installed cells.
			decoded.routes.push_back({UtilityConsumerID{row.consumer}, UtilityProducerID{row.producer}, {}});
		} else if (row.kind == 2) {
			if (decoded.routes.empty() || ++cells > expected[2]) SlErrorCorrupt("Invalid geothermal heat cell count");
			auto &route = decoded.routes.back();
			if (route.consumer.base() != row.consumer || route.producer.base() != row.producer || row.position != route.cells.size()) SlErrorCorrupt("Invalid geothermal heat cell position"); // Each issued cell belongs to the immediately preceding exact shared slice, in original path order.
			route.cells.push_back(UtilityLinkID{row.reference});
		} else {
			SlErrorCorrupt("Invalid geothermal heat row kind");
		}
	}
	if (decoded.bindings.size() != expected[0] || decoded.routes.size() != expected[1] || cells != expected[2] ||
		(!decoded.routes.empty() && decoded.routes.back().cells.empty()) || (interval == 0 && !decoded.routes.empty())) SlErrorCorrupt("Invalid geothermal heat family counts"); // Omitted final cells or an epochless path cannot acquire native rights.
	if (preview) {
		std::map<UtilityProducerID, TileIndex> ports;
		const uint64_t plane = static_cast<uint64_t>(_load_check_data.map_size_x) * _load_check_data.map_size_y;
		// Original constructed source ports resolve from this file's accepted
		// copied collector/cold rows and copied map geometry, never the live map.
		for (const auto &collector : _load_check_data.geothermal_collector_rows) {
			if (collector.heat_producer == UINT32_MAX) continue;
			if (collector.site >= _load_check_data.geothermal_site_rows.size()) SlErrorCorrupt("Invalid geothermal heat source port");
			const auto &site = _load_check_data.geothermal_site_rows[collector.site];
			const auto tile = TileIndex{static_cast<uint32_t>(to_underlying(MapLayer::DeepUnderground) * plane + static_cast<uint64_t>(site.endpoint_y) * _load_check_data.map_size_x + site.endpoint_x)};
			ports.emplace(UtilityProducerID{collector.heat_producer}, tile);
		}
		if (!ValidateGeothermalHeatRouteState(decoded.bindings, decoded.routes, interval, _load_check_data.utility_power_state,
			_load_check_data.map_size_x, _load_check_data.map_size_y, _load_check_data.map_layers, ports)) SlErrorCorrupt("Invalid geothermal heat preview references");
	}
	return decoded;
}

/** @copydoc ValidateGeothermalHeatRouteLoadPreview */
void ValidateGeothermalHeatRouteLoadPreview() { CheckHeatRouteFamily(true); }

/** Exact24-byte revision/count/epoch header; native UPLH/UPLD transport every authoritative quantity. */
struct GHTHChunkHandler final : ChunkHandler {
	/** Register independent physical reference header. */
	GHTHChunkHandler() : ChunkHandler("GHTH", ChunkType::Riff) {}
	/** Save physical bounds and the exact existing native epoch, without allocating or generating. */
	void Save() const override
	{
		const auto bindings = CaptureGeothermalHeatConsumers(); const auto routes = CaptureGeothermalHeatRoutes();
		uint32_t cells = 0;
		for (const auto &route : routes) cells += static_cast<uint32_t>(route.cells.size()); // The complete bounded path-reference family determines exact transport cardinality.
		std::array<uint32_t, 4> counts{1, static_cast<uint32_t>(bindings.size()), static_cast<uint32_t>(routes.size()), cells};
		std::array<uint64_t, 1> interval{GetGeothermalHeatRouteInterval()};
		SlSetLength(24); SlCopy<VarFileType::U32>(counts); SlCopy<VarFileType::U64>(interval);
	}
	/** @param preview Decode independent copied bounds instead of full-load staging; no physical owner is published. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::GeothermalHeatRoutes) || SlGetFieldLength() != 24) SlErrorCorrupt("Invalid geothermal heat route version"); // Older numerical grants cannot introduce a current physical reference family.
		std::array<uint32_t, 4> counts; std::array<uint64_t, 1> interval;
		SlCopy<VarFileType::U32>(counts); SlCopy<VarFileType::U64>(interval);
		if (counts[0] != 1 || counts[1] > MAX_UTILITY_INTERVAL_ACCOUNTS || counts[2] > MAX_UTILITY_POWER_SHARES || counts[3] > MAX_UTILITY_POWER_SHARES) SlErrorCorrupt("Invalid geothermal heat route bounds"); // Hostile counts refuse before any table growth or native identity changes.
		auto &presence = preview ? _load_check_data.geothermal_heat_route_present : _heat_route_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate geothermal heat route header");
		presence[0] = true;
		(preview ? _load_check_data.geothermal_heat_route_expected : _heat_route_expected) = {counts[1], counts[2], counts[3]};
		(preview ? _load_check_data.geothermal_heat_route_interval : _heat_route_interval) = interval[0];
	}
	/** Decode unpublished whole-load bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader validates the exact header length. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage exact checked physical references; complete native acceptance follows original source and utility geometry resolution. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		auto decoded = CheckHeatRouteFamily(false);
		if (decoded.has_value()) StageLoadedGeothermalHeatState(std::move(decoded->bindings), std::move(decoded->routes), _heat_route_interval);
	}
};

/** Exact17-byte reference rows, independent of native pool insertion order or presentation. */
struct GHTDChunkHandler final : ChunkHandler {
	/** Register physical ports/path/cell table. */
	GHTDChunkHandler() : ChunkHandler("GHTD", ChunkType::Table) {}
	/** Serialize only issued physical references; source quantities remain exclusively in their original native records. */
	void Save() const override
	{
		SlTableHeader(_heat_route_desc); uint32_t ordinal = 0;
		/** @param wire One fixed physical row, already in stable consumer/source/path order. */
		const auto write = [&ordinal](GeothermalHeatRouteWire wire) { SlSetArrayIndex(ordinal++); SlObject(&wire, _heat_route_desc); };
		for (const auto &binding : CaptureGeothermalHeatConsumers()) write({0, binding.consumer.base(), UINT32_MAX, 0, binding.endpoint.base()}); // Retained original ports precede current source path declarations.
		for (const auto &route : CaptureGeothermalHeatRoutes()) {
			write({1, route.consumer.base(), route.producer.base(), 0, UINT32_MAX}); uint32_t position = 0;
			for (const auto cell : route.cells) write({2, route.consumer.base(), route.producer.base(), position++, cell.base()}); // Exact cardinal cell order cannot be rebuilt from endpoint proximity after reload.
		}
	}
	/** @param preview Decode copied physical scalars instead of whole-load staging, with exact fields and bounded dense rows. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::GeothermalHeatRoutes)) SlErrorCorrupt("Unexpected geothermal heat route table");
		auto &presence = preview ? _load_check_data.geothermal_heat_route_present : _heat_route_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate geothermal heat route table");
		const auto desc = SlTableHeader(_heat_route_desc);
		for (const auto &required : _heat_route_desc) {
			if (std::ranges::find(desc, required.name, &SaveLoad::name) == desc.end()) SlErrorCorrupt("Missing geothermal heat route field"); // Every exact physical reference is mandatory, with no default cell or endpoint.
		}
		auto &rows = preview ? _load_check_data.geothermal_heat_route_rows : _heat_route_rows;
		uint32_t ordinal = 0;
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (ordinal >= MAX_UTILITY_INTERVAL_ACCOUNTS + 2 * MAX_UTILITY_POWER_SHARES || index != static_cast<int>(ordinal++)) SlErrorCorrupt("Invalid geothermal heat route ordinal"); // Dense bounded scalars prevent sparse allocations and hidden duplicate families.
			GeothermalHeatRouteWire wire; SlObject(&wire, desc); rows.push_back(wire);
		}
		presence[1] = true;
	}
	/** Parse unpublished bounded physical scalars. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader validates exact field and dense ordinal boundaries. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const GHTHChunkHandler GHTH;
static const GHTDChunkHandler GHTD;
static const ChunkHandlerRef heat_route_handlers[] = {GHTH, GHTD};
extern const ChunkHandlerTable _geothermal_heat_route_chunk_handlers(heat_route_handlers);
