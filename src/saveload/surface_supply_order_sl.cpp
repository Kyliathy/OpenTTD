/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order_sl.cpp Original native/full/copied order-family transport and same-file reference admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCC.
 * Loading/preview never issues cargo, dispatches work or changes a company balance.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../surface_supply_order_runtime.h"
#include "../surface_supply_order_wire.h"
#include "../fios.h"
#include "../map_layer.h"
#include "../safeguards.h"

/** Read only one exact original bounded family; full and copied loading share this decoder.
 * @return Detached original book/day, never installed rights or physical stock; malformed bytes raise the ordinary corrupt-save error.
 */
static SurfaceSupplyOrderTransport ReadSurfaceSupplyOrderFamily()
{
	const auto length = SlGetFieldLength();
	// A new family requires its actual era and complete bounded bytes before allocation; old files cannot acquire imported stock or paid history by appending a header.
	if (IsSavegameVersionBefore(SaveLoadVersion::SurfaceSupplyOrders) || length < SURFACE_SUPPLY_WIRE_HEADER_SIZE || length > MAX_SURFACE_SUPPLY_WIRE_SIZE) SlErrorCorrupt("Invalid Surface supply order family era/length");
	std::vector<uint8_t> bytes(length);
	// The framework admitted this exact family extent. Retaining each original byte cannot consume another chunk, allocate a native packet or supply absent references.
	for (auto &byte : bytes) byte = SlReadByte();
	auto transport = DecodeSurfaceSupplyOrders(bytes);
	if (!transport) SlErrorCorrupt("Invalid Surface supply order transport");
	return std::move(*transport);
}

/** Validate only this copied file's complete original order/map/date/company/station facts.
 * No native company, station, packet, clock or financial owner is read from the active world.
 */
void ValidateSurfaceSupplyOrderLoadPreview()
{
	const auto &data = _load_check_data;
	if (!data.surface_supply_orders_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SurfaceSupplyOrders)) SlErrorCorrupt("Missing Surface supply order family");
		return; // Genuine old absence grants no imports, commitments or refund history.
	}
	// Original economic time and explicit native map geometry must come from this candidate. Initialized/default/previous-file values cannot replenish allowance or locate a pending receiver.
	if (!data.native_economy_date_present || !data.native_map_dimensions_present || data.surface_supply_orders.saved_day != data.native_economy_date) SlErrorCorrupt("Missing/changed copied Surface supply map/economy day");
	SurfaceSupplyRecordContext context;
	context.profile = static_cast<MultiLayerGameplayProfileID>(data.gameplay_profile_id);
	context.width = data.map_size_x; context.height = data.map_size_y;
	// Each pending controller must be an actual same-file PLYR identity; a current playable company never fills a missing or out-of-domain candidate slot.
	for (const auto &[company, properties] : data.companies) {
		(void)properties;
		if (company >= MAX_COMPANIES) SlErrorCorrupt("Invalid copied Surface supply company domain");
		context.companies[company] = true;
	}
	std::vector<SurfaceSupplyStationView> stations; stations.reserve(data.native_station_owner_rows.size());
	// Original packed storage is decoded by Map/controller against this file's dimensions/layers. A matching active-world tile, another layer or a waypoint cannot lend station authority.
	for (const auto &station : data.native_station_owner_rows) {
		const auto position = MultiLayerMap::DecodeStoredTile(station.tile, data.map_size_x, data.map_size_y, data.map_layers);
		SurfaceSupplyStationView view;
		view.station = station.station; view.owner = station.owner;
		view.layer = position ? position->layer : MapLayer::Invalid;
		if (position) { view.x = position->x; view.y = position->y; }
		view.freight_facility = station.facilities.Any({StationFacility::Train, StationFacility::TruckStop});
		stations.push_back(view);
	}
	context.stations = stations;
	if (ValidateSurfaceSupplyRecords(data.surface_supply_orders.snapshot, context) != SurfaceSupplyRecordError::None) SlErrorCorrupt("Invalid copied Surface supply order ownership/history");
}

/** Required current native family; original station/carrier/industry containers remain the sole physical stock owners. */
struct SSORChunkHandler : ChunkHandler {
	/** Register the unique native metadata family; construction allocates no order, cargo or financial right. */
	SSORChunkHandler() : ChunkHandler("SSOR", ChunkType::Riff) {}
	/** Serialize only the fully joined original private book and actual native economy-day stamp; do not dispatch a pending order. */
	void Save() const override
	{
		if (!ValidateSurfaceSupplyOrderBook()) SlErrorCorrupt("Invalid native Surface supply order book on save");
		const auto bytes = EncodeSurfaceSupplyOrders({CaptureSurfaceSupplyOrders(), TimerGameEconomy::date});
		if (!bytes) SlErrorCorrupt("Invalid native Surface supply order transport on save");
		SlSetLength(bytes->size());
		// Native/copy readers receive the same exact original commitment/history bytes; no scalar receipt becomes a serialized second inventory.
		for (const auto byte : *bytes) SlWriteByte(byte);
	}
	/** Stage one original full family; all native owners/date must reconstruct before adoption can expose any pending right. */
	void Load() const override
	{
		auto transport = ReadSurfaceSupplyOrderFamily();
		if (!StageLoadedSurfaceSupplyOrders(std::move(transport.snapshot), transport.saved_day)) SlErrorCorrupt("Repeated/invalid native Surface supply order staging");
	}
	/** Copy only this candidate's original family; post-family admission resolves its own native facts without touching live owners.
	 * @param length Framework-admitted extent; the shared original decoder independently bounds its exact payload.
	 */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.surface_supply_orders_present) SlErrorCorrupt("Repeated copied Surface supply order family");
		_load_check_data.surface_supply_orders = ReadSurfaceSupplyOrderFamily();
		_load_check_data.surface_supply_orders_present = true;
	}
};

static const SSORChunkHandler SSOR;
static const ChunkHandlerRef supply_order_handlers[] = {SSOR};
extern const ChunkHandlerTable _surface_supply_order_chunk_handlers(supply_order_handlers);
