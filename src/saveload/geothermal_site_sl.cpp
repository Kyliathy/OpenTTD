/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_site_sl.cpp Bounded388 cold-site identity/coordinate transport, with independent copied natural-source preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../geothermal.h"
#include "../geothermal_site_wire.h"
#include "../excavation_state.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include <set>
#include "../safeguards.h"

static const SaveLoad _geothermal_site_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("id", SLE_OBJECT_ADDRESS(GeothermalSiteWire, id)),
	SaveLoad::Variable<VarFileType::U32>("vent", SLE_OBJECT_ADDRESS(GeothermalSiteWire, vent)),
	SaveLoad::Variable<VarFileType::U8>("owner", SLE_OBJECT_ADDRESS(GeothermalSiteWire, owner)),
	SaveLoad::Variable<VarFileType::U8>("kind", SLE_OBJECT_ADDRESS(GeothermalSiteWire, kind)),
	SaveLoad::Variable<VarFileType::U16>("x", SLE_OBJECT_ADDRESS(GeothermalSiteWire, x)),
	SaveLoad::Variable<VarFileType::U16>("y", SLE_OBJECT_ADDRESS(GeothermalSiteWire, y)),
	SaveLoad::Variable<VarFileType::U16>("endpoint_x", SLE_OBJECT_ADDRESS(GeothermalSiteWire, endpoint_x)),
	SaveLoad::Variable<VarFileType::U16>("endpoint_y", SLE_OBJECT_ADDRESS(GeothermalSiteWire, endpoint_y)),
	SaveLoad::Variable<VarFileType::U8>("access", SLE_OBJECT_ADDRESS(GeothermalSiteWire, access)),
	SaveLoad::Variable<VarFileType::U8>("active", SLE_OBJECT_ADDRESS(GeothermalSiteWire, active)),
};

static std::array<bool, 2> _geothermal_site_presence{}; ///< Unpublished exact header/table presence.
static uint32_t _geothermal_site_expected = 0; ///< Fixed header's retained identity count.
static std::vector<GeothermalSiteWire> _geothermal_site_rows; ///< Copied coordinate transport only; no live blueprint is granted during parsing.

/** @copydoc ResetGeothermalSiteLoadStaging */
void ResetGeothermalSiteLoadStaging()
{
	_geothermal_site_presence.fill(false); _geothermal_site_expected = 0; _geothermal_site_rows.clear();
}

/** @param preview Select independent copied map/source data. @return Complete presence/reference validation; genuine pre388 absence creates no implicit blueprint. */
static bool CheckGeothermalSiteFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.geothermal_site_present : _geothermal_site_presence;
	const auto expected = preview ? _load_check_data.geothermal_site_expected : _geothermal_site_expected;
	const auto &rows = preview ? _load_check_data.geothermal_site_rows : _geothermal_site_rows;
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : metadata == nullptr ? 0 : metadata->feature_flags;
	const bool required = (flags & MULTILAYER_FEATURE_GEOTHERMAL_SITES) != 0;
	if (!presence[0]) {
		// New/explicit/partial cold families cannot forget retained reservation
		// identity; actual older worlds remain empty rather than receiving a plan.
		if (!IsSavegameVersionBefore(SaveLoadVersion::GeothermalSiteClaims) || presence[1] || required) SlErrorCorrupt("Missing geothermal site header");
		return false;
	}
	if (!presence[1] || rows.size() != expected || required != !rows.empty()) SlErrorCorrupt("Incomplete geothermal site family"); // Exact counts and feature presence preserve cancellation/ID history.
	if (rows.empty()) return true;
	const auto *geology = preview ? nullptr : GetStagedGeologyWorldState();
	if (!preview && geology == nullptr) SlErrorCorrupt("Missing geothermal site source owner"); // Full loading cannot borrow the running world's natural sources.
	const auto vents = preview ? std::span<const GeothermalVentField>{_load_check_data.geology_vent_fields} : std::span<const GeothermalVentField>{geology->vents};
	const uint32_t width = preview ? _load_check_data.map_size_x : Map::SizeX();
	const uint32_t height = preview ? _load_check_data.map_size_y : Map::SizeY();
	const uint layers = preview ? _load_check_data.map_layers : Map::LayerCount();
	// Saved geometry, never current camera/map dimensions, bounds local wire
	// coordinates before any actual tile is reconstructed or looked up.
	if (width < 64 || height < 64 || width > 4096 || height > 4096 || layers != 4) SlErrorCorrupt("Invalid geothermal site geometry");
	std::set<GeothermalVentID> collectors;
	std::set<std::pair<uint16_t, uint16_t>> floors;
	// Every retained identity references the copied original natural field;
	// active cold floors/collectors are exclusive, including between companies.
	for (size_t index = 0; index < rows.size(); ++index) {
		const auto &row = rows[index];
		const auto *terms = GetMultiLayerGeothermalConstruction(static_cast<MultiLayerGeothermalConstructionKind>(row.kind));
		const auto vent = std::ranges::find(vents, GeothermalVentID{row.vent}, &GeothermalVentField::id);
		// Hostile scalars cannot turn a blueprint into a riser, forget cancelled
		// identity or acquire a source absent from this file's actual geology.
		if (row.id != index || row.owner >= MAX_COMPANIES || row.active > 1 || row.access >= to_underlying(DiagDirection::End) ||
			terms == nullptr || terms->kind == MultiLayerGeothermalConstructionKind::Riser || vent == vents.end()) SlErrorCorrupt("Invalid geothermal site references");
		if (static_cast<uint32_t>(row.x) + terms->width >= width || static_cast<uint32_t>(row.y) + terms->height >= height) SlErrorCorrupt("Invalid geothermal site floor bounds"); // The complete facility cannot cross the saved map's far void fence.
		// The utility endpoint is explicitly within this same saved Deep floor,
		// on its boundary; arbitrary remote/interior coordinates supply no link.
		if (row.endpoint_x < row.x || row.endpoint_y < row.y || row.endpoint_x >= row.x + terms->width || row.endpoint_y >= row.y + terms->height ||
			(row.endpoint_x != row.x && row.endpoint_y != row.y && row.endpoint_x + 1 != row.x + terms->width && row.endpoint_y + 1 != row.y + terms->height)) SlErrorCorrupt("Invalid geothermal site endpoint");
		if (!row.active) continue;
		if (terms->kind == MultiLayerGeothermalConstructionKind::Collector) {
			// Collectors need real natural-field contact. Cold generators retain
			// explicit source identity but need later actual utility admission.
			if (row.x >= vent->x + vent->width || row.y >= vent->y + vent->height || row.x + terms->width <= vent->x || row.y + terms->height <= vent->y ||
				!collectors.insert(vent->id).second) SlErrorCorrupt("Invalid geothermal collector claim");
		}
		// Bound work by256 retained sites and the frozen maximum16 floor cells;
		// no world-plane iteration or overlapping partial permission is allowed.
		for (uint16_t y = row.y; y < row.y + terms->height; ++y) {
			for (uint16_t x = row.x; x < row.x + terms->width; ++x) {
				if (!floors.insert({x, y}).second) SlErrorCorrupt("Overlapping geothermal site claims");
			}
		}
	}
	return true;
}

/** @copydoc ValidateGeothermalSiteLoadPreview */
void ValidateGeothermalSiteLoadPreview() { CheckGeothermalSiteFamily(true); }

/** Exact388 revision/count header; no work, stock, heat or electrical offer is serialized here. */
struct GCSHChunkHandler final : ChunkHandler {
	/** Register fixed8-byte cold identity header. */
	GCSHChunkHandler() : ChunkHandler("GCSH", ChunkType::Riff) {}
	/** Save retained identity cardinality without completing or renewing any plan. */
	void Save() const override
	{
		std::array<uint32_t, 2> fields{1, static_cast<uint32_t>(CaptureGeothermalSiteClaims().size())};
		SlSetLength(8); SlCopy<VarFileType::U32>(fields);
	}
	/** @param preview Decode exact bounded header into independent transport only. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::GeothermalSiteClaims) || SlGetFieldLength() != 8) SlErrorCorrupt("Invalid geothermal site header version"); // Older formats cannot introduce new planning identity.
		std::array<uint32_t, 2> fields; SlCopy<VarFileType::U32>(fields);
		if (fields[0] != 1 || fields[1] > MAX_GEOTHERMAL_SITE_CLAIMS) SlErrorCorrupt("Invalid geothermal site bounds"); // Counts refuse before table growth.
		auto &presence = preview ? _load_check_data.geothermal_site_present : _geothermal_site_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate geothermal site header");
		presence[0] = true; (preview ? _load_check_data.geothermal_site_expected : _geothermal_site_expected) = fields[1];
	}
	/** Decode unpublished full-load bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader validates exact header length. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Resolve actual saved map addresses and stage cold identities only after the original geological source family exists. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!CheckGeothermalSiteFamily(false)) return;
		std::vector<GeothermalSiteClaim> claims; claims.reserve(_geothermal_site_rows.size());
		// The map controller reconstructs actual world addresses once saved
		// geometry exists; preview never performs this conversion through live Map.
		for (const auto &row : _geothermal_site_rows) {
			GeothermalSiteRequest site;
			site.vent = GeothermalVentID{row.vent}; site.owner = CompanyID{row.owner}; site.kind = static_cast<MultiLayerGeothermalConstructionKind>(row.kind);
			site.tile = TileXYL(row.x, row.y, MapLayer::DeepUnderground); site.endpoint = TileXYL(row.endpoint_x, row.endpoint_y, MapLayer::DeepUnderground);
			site.access = static_cast<DiagDirection>(row.access); claims.push_back({GeothermalSiteID{row.id}, site, row.active != 0});
		}
		StageLoadedGeothermalSiteClaims(std::move(claims));
	}
};

/** Exact20-byte cold coordinate rows in retained planning identity order. */
struct GCSDChunkHandler final : ChunkHandler {
	/** Register bounded cold planning table. */
	GCSDChunkHandler() : ChunkHandler("GCSD", ChunkType::Table) {}
	/** Save actual cold geometry/history, without duplicating natural capacity or any construction/energy owner. */
	void Save() const override
	{
		SlTableHeader(_geothermal_site_desc); uint32_t ordinal = 0;
		// Retained original identity/source and cancellation are each emitted
		// once; local coordinates keep the descriptor independent of live preview geometry.
		for (const auto &claim : CaptureGeothermalSiteClaims()) {
			const auto &site = claim.site;
			GeothermalSiteWire wire{claim.id.base(), site.vent.base(), site.owner.base(), to_underlying(site.kind),
				static_cast<uint16_t>(TileX(site.tile)), static_cast<uint16_t>(TilePlaneY(site.tile)),
				static_cast<uint16_t>(TileX(site.endpoint)), static_cast<uint16_t>(TilePlaneY(site.endpoint)), to_underlying(site.access), claim.active};
			SlSetArrayIndex(ordinal++); SlObject(&wire, _geothermal_site_desc);
		}
	}
	/** @param preview Parse bounded independent coordinate transport without claiming a floor or touching live natural sources. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::GeothermalSiteClaims)) SlErrorCorrupt("Unexpected geothermal site rows");
		auto &presence = preview ? _load_check_data.geothermal_site_present : _geothermal_site_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate geothermal site table");
		const auto desc = SlTableHeader(_geothermal_site_desc);
		if (desc.size() != std::size(_geothermal_site_desc)) SlErrorCorrupt("Invalid geothermal site fields");
		// Missing original geometry/identity scalars cannot create a fresh plan
		// or discard a previously cancelled reservation's monotonic history.
		for (const auto &field : _geothermal_site_desc) {
			if (std::ranges::find(desc, field.name, &SaveLoad::name) == desc.end()) SlErrorCorrupt("Missing geothermal site field");
		}
		auto &rows = preview ? _load_check_data.geothermal_site_rows : _geothermal_site_rows;
		uint32_t ordinal = 0;
		// Dense exact ordinals and the shared retained cap reject sparse,
		// duplicate or oversized planning transport before unbounded allocation.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (ordinal >= MAX_GEOTHERMAL_SITE_CLAIMS || index != static_cast<int>(ordinal++)) SlErrorCorrupt("Invalid geothermal site ordinal");
			GeothermalSiteWire wire; SlObject(&wire, desc); rows.push_back(wire);
		}
		presence[1] = true;
	}
	/** Parse copied full-load rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader validates every row boundary. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const GCSHChunkHandler GCSH;
static const GCSDChunkHandler GCSD;
static const ChunkHandlerRef geothermal_site_handlers[] = {GCSH, GCSD};
extern const ChunkHandlerTable _geothermal_site_chunk_handlers(geothermal_site_handlers);
