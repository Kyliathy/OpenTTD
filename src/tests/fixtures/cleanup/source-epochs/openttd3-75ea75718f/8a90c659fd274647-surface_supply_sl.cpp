/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_sl.cpp Complete426 paid-history transport; native CAPA and its physical containers alone own material.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md.
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../surface_supply.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include <set>
#include "../safeguards.h"

/** Explicit scalar wire values; invalid defaults cannot stand in for an omitted paid-history column. */
struct SurfaceSupplyOrderWire {
	uint32_t id = 0; ///< Complete positive native purchase identity.
	uint8_t owner = 255; ///< Exact company or retired invalid owner, never a local replacement company.
	uint16_t station = UINT16_MAX; ///< Original native source, invalid after actual removal.
	CargoLabel label{}; ///< Canonical four bytes via native LABEL conversion, independent of runtime slot ordering.
	int32_t economy_day = INT32_MIN, calendar_date = INT32_MIN; ///< Separate actual cap day and historical introduction date.
	uint16_t quantity = 0, refunded = UINT16_MAX, dispatched = UINT16_MAX; ///< Paid, already removed/refunded and once-only historical dispatched units.
	int64_t unit_price = 0; ///< Frozen positive actual Money tariff, checked before any future whole refund.
	uint8_t closed = 2; ///< Checked0/1 cancellation retirement; default is deliberately invalid.
};

static const SaveLoad _surface_supply_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("id", SLE_OBJECT_ADDRESS(SurfaceSupplyOrderWire, id)),
	SaveLoad::Variable<VarFileType::U8>("owner", SLE_OBJECT_ADDRESS(SurfaceSupplyOrderWire, owner)),
	SaveLoad::Variable<VarFileType::U16>("station", SLE_OBJECT_ADDRESS(SurfaceSupplyOrderWire, station)),
	SaveLoad::Variable<VarFileType::U32>("label", SLE_OBJECT_ADDRESS(SurfaceSupplyOrderWire, label)),
	SaveLoad::Variable<VarFileType::I32>("economy_day", SLE_OBJECT_ADDRESS(SurfaceSupplyOrderWire, economy_day)),
	SaveLoad::Variable<VarFileType::I32>("calendar_date", SLE_OBJECT_ADDRESS(SurfaceSupplyOrderWire, calendar_date)),
	SaveLoad::Variable<VarFileType::U16>("quantity", SLE_OBJECT_ADDRESS(SurfaceSupplyOrderWire, quantity)),
	SaveLoad::Variable<VarFileType::I64>("unit_price", SLE_OBJECT_ADDRESS(SurfaceSupplyOrderWire, unit_price)),
	SaveLoad::Variable<VarFileType::U16>("refunded", SLE_OBJECT_ADDRESS(SurfaceSupplyOrderWire, refunded)),
	SaveLoad::Variable<VarFileType::U16>("dispatched", SLE_OBJECT_ADDRESS(SurfaceSupplyOrderWire, dispatched)),
	SaveLoad::Variable<VarFileType::U8>("closed", SLE_OBJECT_ADDRESS(SurfaceSupplyOrderWire, closed)),
};

static std::array<bool, 2> _surface_supply_presence{}; ///< Unpublished full-load header/table presence only.
static uint32_t _surface_supply_expected = 0; ///< Exact bounded retained paid row count.
static SurfaceSupplyState _surface_supply_state; ///< Independent unpublished scalar transport; no cargo or purchase authority.

/** @copydoc ResetSurfaceSupplyLoadStaging */
void ResetSurfaceSupplyLoadStaging()
{
	_surface_supply_presence.fill(false); _surface_supply_expected = 0; _surface_supply_state = {};
}

/**
 * Check exact family presence and independent history before publication.
 * @param preview Select the current file's copied preview, never the live world's history/packets.
 * @return Whether a complete current family exists; genuine pre426 absence restores no purchase/cap rights.
 */
static bool CheckSurfaceSupplyFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.surface_supply_present : _surface_supply_presence;
	const uint32_t expected = preview ? _load_check_data.surface_supply_expected : _surface_supply_expected;
	const auto &state = preview ? _load_check_data.surface_supply_state : _surface_supply_state;
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : metadata == nullptr ? 0 : metadata->feature_flags;
	const bool required = (flags & MULTILAYER_FEATURE_SURFACE_SUPPLIES) != 0;
	if (!presence[0]) {
		// Current or partial families must not reset paid history/caps from
		// absence; legitimate older files contain no purchase purpose at all.
		if (!IsSavegameVersionBefore(SaveLoadVersion::SurfaceSupplyOrders) || presence[1] || required) SlErrorCorrupt("Missing paid Surface supply header");
		return false;
	}
	// Exact scalar cardinality/cursor and metadata history presence precede
	// native packet reference resolution at ordinary afterload acceptance.
	if (!presence[1] || state.orders.size() != expected || required != (state.next_order != 1)) SlErrorCorrupt("Incomplete paid Surface supply family");
	if (ValidateSurfaceSupplyState(state, false) != SurfaceSupplyError::None) SlErrorCorrupt("Invalid paid Surface supply history");
	return true;
}

/** @copydoc ValidateSurfaceSupplyLoadPreview */
void ValidateSurfaceSupplyLoadPreview() { CheckSurfaceSupplyFamily(true); }

/** Fixed12-byte426 header retains complete cardinality and the next never-used purchase identity. */
struct SSOHChunkHandler final : ChunkHandler {
	/** Register the bounded native paid-history header. */
	SSOHChunkHandler() : ChunkHandler("SSOH", ChunkType::Riff) {}
	/** Save scalar history bounds without issuing orders, reserving caps or touching material. */
	void Save() const override
	{
		const auto state = CaptureSurfaceSupplyState();
		std::array<uint32_t, 3> header{1, static_cast<uint32_t>(state.orders.size()), state.next_order};
		SlSetLength(12); SlCopy<VarFileType::U32>(header);
	}
	/** @param preview Decode exact version/bounds into independent file transport before row allocation. */
	void Read(bool preview) const
	{
		// Older formats cannot introduce paid rights, and a changed header
		// length cannot be interpreted as a convenient fresh purchase cursor.
		if (IsSavegameVersionBefore(SaveLoadVersion::SurfaceSupplyOrders) || SlGetFieldLength() != 12) SlErrorCorrupt("Invalid paid supply header version");
		std::array<uint32_t, 3> header;
		SlCopy<VarFileType::U32>(header);
		if (header[0] != 1 || header[1] > MAX_SURFACE_SUPPLY_ORDERS || header[2] != header[1] + 1) SlErrorCorrupt("Invalid paid supply header bounds");
		auto &presence = preview ? _load_check_data.surface_supply_present : _surface_supply_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate paid supply header");
		presence[0] = true;
		(preview ? _load_check_data.surface_supply_expected : _surface_supply_expected) = header[1];
		(preview ? _load_check_data.surface_supply_state : _surface_supply_state).next_order = header[2];
	}
	/** Decode unpublished full-load bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Native fixed reader enforces the exact header length. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage one complete scalar candidate; actual companies and packet owners resolve only at late afterload. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (CheckSurfaceSupplyFamily(false)) StageLoadedSurfaceSupplyState(_surface_supply_state);
	}
};

/** Complete ordered paid-history table; its rows contain no additional material inventory. */
struct SSORChunkHandler final : ChunkHandler {
	/** Register the fixed-schema native purchase table. */
	SSORChunkHandler() : ChunkHandler("SSOR", ChunkType::Table) {}
	/** Save each retained purchase/refund row once in its stable native identity order. */
	void Save() const override
	{
		SlTableHeader(_surface_supply_desc);
		uint32_t ordinal = 0;
		// Paid and refunded history survives shipment/consumption; no source
		// quantity or cursor is recomputed from whatever packets remain today.
		for (const auto &row : CaptureSurfaceSupplyState().orders) {
			SurfaceSupplyOrderWire wire{row.id.base(), row.owner.base(), row.station.base(), row.label, row.economy_day,
				row.calendar_date, row.quantity, row.refunded, row.dispatched, static_cast<int64_t>(row.unit_price), static_cast<uint8_t>(row.closed)};
			SlSetArrayIndex(ordinal++); SlObject(&wire, _surface_supply_desc);
		}
	}
	/** @param preview Decode bounded complete columns/rows into this file's independent history transport only. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::SurfaceSupplyOrders)) SlErrorCorrupt("Unexpected paid supply rows");
		auto &presence = preview ? _load_check_data.surface_supply_present : _surface_supply_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate paid supply table");
		const auto desc = SlTableHeader(_surface_supply_desc);
		std::set<std::string> columns;
		// A current schema must carry every paid field once. Missing/default,
		// duplicate or foreign columns cannot silently fabricate a valid owner/refund history.
		for (const auto &field : desc) if (!columns.insert(field.name).second) SlErrorCorrupt("Duplicate paid supply column");
		if (columns.size() != std::size(_surface_supply_desc)) SlErrorCorrupt("Incomplete paid supply columns");
		for (const auto &field : _surface_supply_desc) if (!columns.contains(field.name)) SlErrorCorrupt("Missing paid supply column");
		auto &state = preview ? _load_check_data.surface_supply_state : _surface_supply_state;
		int index;
		// Native ordinal order and the hard cap bound hostile file growth;
		// structural and actual reference checks still precede all live publication.
		while ((index = SlIterateArray()) != -1) {
			if (index != static_cast<int>(state.orders.size()) || state.orders.size() >= MAX_SURFACE_SUPPLY_ORDERS) SlErrorCorrupt("Invalid paid supply row order/count");
			SurfaceSupplyOrderWire wire;
			SlObject(&wire, desc);
			if (wire.closed > 1) SlErrorCorrupt("Invalid paid supply closed flag");
			SurfaceSupplyOrder row;
			row.id = SurfaceSupplyOrderID{wire.id}; row.owner = CompanyID{wire.owner}; row.station = StationID{wire.station}; row.label = wire.label;
			row.economy_day = wire.economy_day; row.calendar_date = wire.calendar_date; row.quantity = wire.quantity;
			row.unit_price = Money{wire.unit_price}; row.refunded = wire.refunded; row.dispatched = wire.dispatched; row.closed = wire.closed != 0;
			state.orders.push_back(row);
		}
		presence[1] = true;
	}
	/** Decode only unpublished full-load rows. */
	void Load() const override { this->Read(false); }
	/** @param len Native table machinery owns encoded field lengths; this reader enforces exact current columns/cardinality. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const SSOHChunkHandler SSOH;
static const SSORChunkHandler SSOR;
static const ChunkHandlerRef surface_supply_chunk_handlers[] = {SSOH, SSOR};
extern const ChunkHandlerTable _surface_supply_chunk_handlers(surface_supply_chunk_handlers);
