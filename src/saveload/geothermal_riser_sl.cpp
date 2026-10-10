/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_riser_sl.cpp Version400 paid machinery transport and401 original paid-link preview against this file's native owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../geothermal_riser.h"
#include "../geothermal_riser_wire.h"
#include "../layer_portal_cmd.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include <set>
#include "../safeguards.h"

static const SaveLoad _riser_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("industry", SLE_OBJECT_ADDRESS(GeothermalRiserWire, industry)),
	SaveLoad::Variable<VarFileType::U8>("owner", SLE_OBJECT_ADDRESS(GeothermalRiserWire, owner)),
	SaveLoad::Variable<VarFileType::U8>("layer", SLE_OBJECT_ADDRESS(GeothermalRiserWire, layer)),
	SaveLoad::Variable<VarFileType::U16>("x", SLE_OBJECT_ADDRESS(GeothermalRiserWire, x)),
	SaveLoad::Variable<VarFileType::U16>("y", SLE_OBJECT_ADDRESS(GeothermalRiserWire, y)),
	SaveLoad::Variable<VarFileType::U16>("portal_x", SLE_OBJECT_ADDRESS(GeothermalRiserWire, portal_x)),
	SaveLoad::Variable<VarFileType::U16>("portal_y", SLE_OBJECT_ADDRESS(GeothermalRiserWire, portal_y)),
	SaveLoad::Variable<VarFileType::U16>("port_x", SLE_OBJECT_ADDRESS(GeothermalRiserWire, port_x)),
	SaveLoad::Variable<VarFileType::U16>("port_y", SLE_OBJECT_ADDRESS(GeothermalRiserWire, port_y)),
	SaveLoad::Variable<VarFileType::U8>("destination_layer", SLE_OBJECT_ADDRESS(GeothermalRiserWire, destination_layer)),
	SaveLoad::Variable<VarFileType::U16>("destination_x", SLE_OBJECT_ADDRESS(GeothermalRiserWire, destination_x)),
	SaveLoad::Variable<VarFileType::U16>("destination_y", SLE_OBJECT_ADDRESS(GeothermalRiserWire, destination_y)),
	SaveLoad::Variable<VarFileType::U8>("access", SLE_OBJECT_ADDRESS(GeothermalRiserWire, access)),
	SaveLoad::Variable<VarFileType::U64>("portal_revision", SLE_OBJECT_ADDRESS(GeothermalRiserWire, portal_revision)),
	SaveLoad::Variable<VarFileType::U8>("portal_retired", SLE_OBJECT_ADDRESS(GeothermalRiserWire, portal_retired)),
	SaveLoad::Variable<VarFileType::U16>("work_ticks", SLE_OBJECT_ADDRESS(GeothermalRiserWire, work_ticks)),
	SaveLoad::Variable<VarFileType::U8>("materials_committed", SLE_OBJECT_ADDRESS(GeothermalRiserWire, materials_committed)),
	SaveLoad::Variable<VarFileType::U8>("has_work_tick", SLE_OBJECT_ADDRESS(GeothermalRiserWire, has_work_tick)),
	SaveLoad::Variable<VarFileType::U64>("last_work_tick", SLE_OBJECT_ADDRESS(GeothermalRiserWire, last_work_tick)),
};

static std::array<bool, 2> _riser_presence{}; ///< Unpublished exact header/table presence.
static uint32_t _riser_expected = 0; ///< Header's bounded actual machinery count.
static std::vector<GeothermalRiserWire> _riser_rows; ///< Parsed original geometry/work/loss references; no native stock or utility identity is published.

/** @copydoc ResetGeothermalRiserLoadStaging */
void ResetGeothermalRiserLoadStaging() { _riser_presence.fill(false); _riser_expected = 0; _riser_rows.clear(); }

/** @param x Copied local column. @param y Copied local row. @param layer Copied plane. @param width Copied plane width. @param height Copied plane height. @return Existing wire-format linear address, calculated solely for comparison with this file's native INDY/LPWK fields. */
static uint32_t RiserPreviewAddress(uint16_t x, uint16_t y, uint8_t layer, uint32_t width, uint32_t height)
{
	return static_cast<uint32_t>((static_cast<uint64_t>(layer) * height + y) * width + x); // Serialization comparison only; gameplay and full-load conversion use the native map controller.
}

/** @param preview Select independent copied references instead of full-load staging. @return Whole exact family admission; genuine pre400 absence gives no constructed riser. */
static bool CheckGeothermalRiserFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.geothermal_riser_present : _riser_presence;
	const auto expected = preview ? _load_check_data.geothermal_riser_expected : _riser_expected;
	const auto &rows = preview ? _load_check_data.geothermal_riser_rows : _riser_rows;
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const auto flags = preview ? _load_check_data.multilayer_feature_flags : metadata == nullptr ? 0 : metadata->feature_flags;
	const bool required = (flags & MULTILAYER_FEATURE_GEOTHERMAL_RISERS) != 0;
	if (!presence[0]) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::GeothermalRisers) || presence[1] || required) SlErrorCorrupt("Missing geothermal riser header"); // Current, explicit or partial transport cannot forget paid original machinery.
		return false;
	}
	if (!presence[1] || rows.size() != expected || required != !rows.empty()) SlErrorCorrupt("Incomplete geothermal riser family"); // Metadata/header/table must name exactly the same paid participants, including retired ones.
	const auto width = preview ? _load_check_data.map_size_x : Map::SizeX();
	const auto height = preview ? _load_check_data.map_size_y : Map::SizeY();
	const auto layers = preview ? _load_check_data.map_layers : Map::LayerCount();
	std::set<uint16_t> identities;
	std::set<std::tuple<uint8_t, uint16_t, uint16_t>> senders;
	// Every bounded row keeps its original full geometry/receipt/loss marker.
	// Preview resolves only copied companies, industry definitions and portals;
	// it neither reads nor repairs the running map, cargo or utility owner.
	for (const auto &row : rows) {
		if (!ValidateGeothermalRiserWire(row, width, height, layers) || !identities.insert(row.industry).second ||
			!senders.insert({row.layer, row.portal_x, row.portal_y}).second) SlErrorCorrupt("Invalid geothermal riser scalars");
		if (!preview) continue;
		if (!_load_check_data.native_tick_present || (row.has_work_tick != 0 && row.last_work_tick > _load_check_data.native_tick_counter)) SlErrorCorrupt("Invalid geothermal riser preview work tick"); // Paid work references this file's synchronized clock; a missing/future stamp cannot earn labour on reload.
		const auto &native = _load_check_data.native_industry_preview_rows;
		const auto found = std::ranges::find(native, row.industry, &NativeIndustryPreview::id);
		const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser);
		// The same file's actual machinery type, full floor and control company
		// must exist. Neither a generic native ID nor live registry supplies them.
		if (found == native.end() || std::ranges::find(_load_check_data.native_riser_preview_types, found->type) == _load_check_data.native_riser_preview_types.end() ||
			found->tile != RiserPreviewAddress(row.x, row.y, row.layer, width, height) || found->width != terms->width || found->height != terms->height ||
			(row.owner != CompanyID::Invalid().base() && !_load_check_data.companies.contains(row.owner))) SlErrorCorrupt("Invalid geothermal riser native preview references");
		if (row.portal_retired != 0) continue;
		const auto source = RiserPreviewAddress(row.portal_x, row.portal_y, row.layer, width, height);
		const auto destination = RiserPreviewAddress(row.destination_x, row.destination_y, row.destination_layer, width, height);
		const auto &portals = _load_check_data.portal_work_records;
		const auto portal = std::ranges::find(portals, std::min(source, destination), &PortalWorkWire::key);
		// Retained stale stamps remain unavailable; a future stamp or missing
		// original native pair is corrupt, never repaired into a new attachment.
		if (portal == portals.end() || portal->company != row.owner || portal->revision < row.portal_revision) SlErrorCorrupt("Invalid geothermal riser portal preview reference");
	}
	if (preview) {
		// Native-family coverage also rejects an omitted construction row when
		// an attacker clears both its count and metadata bit. This uses IIDS/INDY
		// copied from this file, not the running native manager or industry pool.
		for (const auto &native : _load_check_data.native_industry_preview_rows) {
			const bool riser = std::ranges::find(_load_check_data.native_riser_preview_types, native.type) != _load_check_data.native_riser_preview_types.end();
			if (riser != identities.contains(native.id)) SlErrorCorrupt("Missing native riser construction preview");
		}
		const auto &utility = _load_check_data.utility_power_state;
		// Active paid edges must resolve complete original machinery from this
		// file. A live world's buildings or a cleared feature/count cannot stand
		// in for the saved paid receipt, native floor or accepted incarnation.
		for (const auto &link : utility.links) {
			if (!link.active) continue;
			if (link.kind == UtilityLinkKind::ElectricalPortal) {
				// A canonical-looking old role cannot downgrade an actual native
				// machinery attachment and evade its future retirement policy.
				for (const auto &row : rows) {
					if (link.source_endpoint.base() == RiserPreviewAddress(row.portal_x, row.portal_y, row.layer, width, height)) SlErrorCorrupt("Generic portal role bypasses paid riser preview");
				}
				continue;
			}
			if (link.kind != UtilityLinkKind::ElectricalRiser) continue;
			const auto found = std::ranges::find(rows, link.riser.base(), &GeothermalRiserWire::industry);
			if (found == rows.end() || link.source.base() >= utility.consumers.size() || link.destination.base() >= utility.consumers.size()) SlErrorCorrupt("Missing paid riser link preview machinery");
			const auto &source = utility.consumers[link.source.base()], &destination = utility.consumers[link.destination.base()];
			const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser);
			const auto portal_key = std::min(link.source_endpoint.base(), link.destination_endpoint.base());
			const auto portal = std::ranges::find(_load_check_data.portal_work_records, portal_key, &PortalWorkWire::key);
			if (portal == _load_check_data.portal_work_records.end() || portal->state != to_underlying(LayerPortalWorkState::Connected) || portal->revision != link.portal_revision ||
				_load_check_data.current_date < TimerGameCalendar::ConvertYMDToDate(terms->introduction_year, TimerGameCalendar::Month{0}, TimerGameCalendar::Day{1})) SlErrorCorrupt("Unavailable paid riser link preview portal/date"); // Actual saved Connected state and technology date must agree; the live world's commission/date cannot authorize this file.
			// Exact company, actual sending/receiving planes and original portal
			// stamp bind this paid direction; retired or unfinished stock/work
			// cannot become an operational electrical edge during preview.
			if (found->portal_retired != 0 || found->materials_committed != 1 || found->work_ticks != terms->work_ticks ||
				found->owner != source.owner.base() || found->layer != to_underlying(source.meter.layer) || found->destination_layer != to_underlying(destination.meter.layer) ||
				link.source_endpoint.base() != RiserPreviewAddress(found->portal_x, found->portal_y, found->layer, width, height) ||
				link.destination_endpoint.base() != RiserPreviewAddress(found->destination_x, found->destination_y, found->destination_layer, width, height) ||
				link.portal_revision != found->portal_revision) SlErrorCorrupt("Invalid paid riser link preview attachment");
			// Copied physical rights must finish at this file's exact declared
			// machinery port. No live industry manager or utility cache may supply
			// a missing port, even when another cell is cardinally adjacent.
			for (const auto &consumer : utility.consumers) {
				for (const auto &share : consumer.shares) {
					for (size_t position = 0; position < share.segment.size(); ++position) {
						if (share.segment[position] != link.id) continue;
						if (position == 0 || share.segment[position - 1].base() >= utility.links.size()) SlErrorCorrupt("Missing paid riser preview inlet");
						const auto &inlet = utility.links[share.segment[position - 1].base()];
						if (inlet.kind != UtilityLinkKind::LocalCell || inlet.source_endpoint.base() != RiserPreviewAddress(found->port_x, found->port_y, found->layer, width, height)) SlErrorCorrupt("Invalid paid riser preview inlet"); // The original funded machinery declares one exact physical cable attachment, not every neighboring industry tile.
					}
				}
			}
		}
	}
	return true;
}

/** @copydoc ValidateGeothermalRiserLoadPreview */
void ValidateGeothermalRiserLoadPreview() { CheckGeothermalRiserFamily(true); }

/** Exact revision/count header, with native stock/history retained in INDY. */
struct GRSHChunkHandler final : ChunkHandler {
	/** Register the independent paid riser construction header. */
	GRSHChunkHandler() : ChunkHandler("GRSH", ChunkType::Riff) {}
	/** Save exact bounded machinery cardinality, including abandoned/retired records. */
	void Save() const override
	{
		std::array<uint32_t, 2> header{1, static_cast<uint32_t>(CaptureGeothermalRisers().size())}; SlSetLength(8); SlCopy<VarFileType::U32>(header);
	}
	/** @param preview Decode unpublished full-load or copied preview bounds; no machinery is constructed. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::GeothermalRisers) || SlGetFieldLength() != 8) SlErrorCorrupt("Invalid geothermal riser header version"); // Genuine older formats cannot acquire this later actual machinery authority.
		std::array<uint32_t, 2> header; SlCopy<VarFileType::U32>(header);
		if (header[0] != 1 || header[1] > MAX_GEOTHERMAL_SITE_CLAIMS) SlErrorCorrupt("Invalid geothermal riser bounds");
		auto &presence = preview ? _load_check_data.geothermal_riser_present : _riser_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate geothermal riser header");
		presence[0] = true; (preview ? _load_check_data.geothermal_riser_expected : _riser_expected) = header[1];
	}
	/** Parse unpublished full-load bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader owns exact header-size checks. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage complete checked references after all chunks parse; actual native floor/stock/company acceptance follows native afterload fixup. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) { ResetGeothermalRisers(); return; } // Null error fixup clears unpublished or newly accepted bindings before a failed world's native pools are reused; Industry still owns its serialized stock.
		if (!CheckGeothermalRiserFamily(false)) return;
		std::vector<GeothermalRiserState> rows; rows.reserve(_riser_rows.size());
		// Convert only scalar-validated addresses through the saved native map
		// controller. This creates no industry, inventory, paid work or utility ID.
		for (const auto &row : _riser_rows) rows.push_back(DecodeGeothermalRiser(row));
		StageLoadedGeothermalRisers(std::move(rows));
	}
};

/** Exact43-byte local-coordinate construction rows in stable native ID order. */
struct GRSDChunkHandler final : ChunkHandler {
	/** Register the independent paid machinery table. */
	GRSDChunkHandler() : ChunkHandler("GRSD", ChunkType::Table) {}
	/** Save original geometry/work/receipt/loss references without duplicating native material or energy balances. */
	void Save() const override
	{
		SlTableHeader(_riser_desc); uint32_t ordinal = 0;
		// Stable native IDs keep the whole bounded construction family canonical.
		for (const auto &state : CaptureGeothermalRisers()) {
			auto row = EncodeGeothermalRiser(state); SlSetArrayIndex(ordinal++); SlObject(&row, _riser_desc);
		}
	}
	/** @param preview Decode independent copied or unpublished full-load scalars, refusing missing fields and noncanonical ordinals. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::GeothermalRisers)) SlErrorCorrupt("Unexpected geothermal riser rows");
		auto &presence = preview ? _load_check_data.geothermal_riser_present : _riser_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate geothermal riser table");
		const auto desc = SlTableHeader(_riser_desc);
		if (desc.size() != std::size(_riser_desc)) SlErrorCorrupt("Invalid geothermal riser fields");
		// Missing original geometry, paid receipt or permanent-loss fields cannot
		// default to free completed/revived machinery on load or in preview.
		for (const auto &field : _riser_desc) {
			if (std::ranges::find(desc, field.name, &SaveLoad::name) == desc.end()) SlErrorCorrupt("Missing geothermal riser field");
		}
		auto &rows = preview ? _load_check_data.geothermal_riser_rows : _riser_rows;
		uint32_t ordinal = 0;
		// Dense bounded rows reject duplicate native IDs and sparse allocation
		// before any actual construction participant can become visible.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (ordinal >= MAX_GEOTHERMAL_SITE_CLAIMS || index != static_cast<int>(ordinal++)) SlErrorCorrupt("Invalid geothermal riser ordinal");
			GeothermalRiserWire row; SlObject(&row, desc);
			if (!rows.empty() && row.industry <= rows.back().industry) SlErrorCorrupt("Invalid geothermal riser industry order");
			rows.push_back(row);
		}
		presence[1] = true;
	}
	/** Parse unpublished bounded construction rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns exact field/ordinal boundaries. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const GRSHChunkHandler GRSH;
static const GRSDChunkHandler GRSD;
static const ChunkHandlerRef geothermal_riser_handlers[] = {GRSH, GRSD};
extern const ChunkHandlerTable _geothermal_riser_chunk_handlers(geothermal_riser_handlers);
