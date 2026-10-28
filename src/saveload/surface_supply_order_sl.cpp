/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order_sl.cpp Original native/full/copied order-family transport and same-file reference admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCC.
 * Loading/preview never issues cargo, dispatches work or changes a company balance.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "space_cargo_preview.h"
#include "surface_supply_order_preview.h"
#include "../surface_supply_order_runtime.h"
#include "../surface_supply_order_wire.h"
#include "../surface_supply_merged_wire.h"
#include "../fios.h"
#include "../map_layer.h"
#include "../multilayer_state.h"
#include <set>
#include "../safeguards.h"

/** Original B TABLE scalars retain their exact names/types before conversion; they own no native packets. */
static const SaveLoad table_supply_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("id", SLE_OBJECT_ADDRESS(TableSupplyOrderWire, id)),
	SaveLoad::Variable<VarFileType::U8>("owner", SLE_OBJECT_ADDRESS(TableSupplyOrderWire, owner)),
	SaveLoad::Variable<VarFileType::U16>("station", SLE_OBJECT_ADDRESS(TableSupplyOrderWire, station)),
	SaveLoad::Variable<VarFileType::U32>("label", SLE_OBJECT_ADDRESS(TableSupplyOrderWire, label)),
	SaveLoad::Variable<VarFileType::I32>("economy_day", SLE_OBJECT_ADDRESS(TableSupplyOrderWire, economy_day)),
	SaveLoad::Variable<VarFileType::I32>("calendar_date", SLE_OBJECT_ADDRESS(TableSupplyOrderWire, calendar_date)),
	SaveLoad::Variable<VarFileType::U16>("quantity", SLE_OBJECT_ADDRESS(TableSupplyOrderWire, quantity)),
	SaveLoad::Variable<VarFileType::I64>("unit_price", SLE_OBJECT_ADDRESS(TableSupplyOrderWire, unit_price)),
	SaveLoad::Variable<VarFileType::U16>("refunded", SLE_OBJECT_ADDRESS(TableSupplyOrderWire, refunded)),
	SaveLoad::Variable<VarFileType::U16>("dispatched", SLE_OBJECT_ADDRESS(TableSupplyOrderWire, dispatched)),
	SaveLoad::Variable<VarFileType::U8>("closed", SLE_OBJECT_ADDRESS(TableSupplyOrderWire, closed)),
};

/** Admit only the Geothermal format's TABLE supply interpretation and its actual framing.
 * @param kind Exact RIFF header or TABLE rows expected by this particular component.
 */
static void RequireTableSupplyKind(ChunkType kind)
{
	const auto source = GetLoadCapabilities();
	// B's legacy components cannot be coerced into A or a merged canonical
	// family; preflight identity does not replace checking actual replayed kind.
	if (!source || source->format != SaveFormat::Geothermal || !SlHasSaveCapability(SaveCapability::SurfaceSupplyOrdersTable) || SlGetChunkType() != kind) SlErrorCorrupt("Wrong table supply format/chunk kind");
}

/** Decode B's exact original paid rows without creating a receipt, quota or cargo owner.
 * @param preview Route unpublished scalars to copied same-file data, otherwise the sole runtime load candidate.
 */
static void ReadTableSupplyRows(bool preview)
{
	RequireTableSupplyKind(ChunkType::Table);
	if (preview && _load_check_data.table_supply_present[1]) SlErrorCorrupt("Repeated copied legacy supply TABLE"); // Empty tables are unique original evidence too; a retry cannot replace paid history.
	const auto description = SlTableHeader(table_supply_desc);
	std::set<std::string> columns;
	// Missing, repeated or foreign columns cannot default a valid tariff,
	// closure or lifetime dispatch amount, even when this TABLE has no rows.
	for (const auto &field : description) {
		if (!columns.insert(field.name).second) SlErrorCorrupt("Repeated legacy supply column");
	}
	if (columns.size() != std::size(table_supply_desc)) SlErrorCorrupt("Changed legacy supply field count");
	// Matching cardinality alone cannot admit a foreign replacement column;
	// each original field must remain available before any paid row is decoded.
	for (const auto &field : table_supply_desc) {
		if (!columns.contains(field.name)) SlErrorCorrupt("Missing legacy supply field");
	}
	std::vector<TableSupplyOrderWire> rows;
	// Source ordinal order and the original65535history ceiling are admitted
	// before each allocation. An empty hole or repeated ID cannot become a new receipt.
	for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
		if (index != static_cast<int>(rows.size()) || rows.size() >= MAX_LEGACY_SURFACE_SUPPLY_RECORDS) SlErrorCorrupt("Invalid legacy supply row ordinal/count");
		TableSupplyOrderWire row; SlObject(&row, description);
		if (row.id != rows.size() + 1 || row.closed > 1) SlErrorCorrupt("Invalid legacy supply identity/closed encoding"); // Retained dense identities and literal closure must survive exactly, not a coercion/default.
		rows.push_back(row);
	}
	if (preview) {
		_load_check_data.table_supply.orders = std::move(rows);
		_load_check_data.table_supply_present[1] = true;
	} else if (!StageLoadedTableSupplyRows(std::move(rows))) {
		SlErrorCorrupt("Repeated/mismatched native legacy supply rows");
	}
}

/** Read-only legacy B header; merged writing emits neither this ID nor a second SSOR. */
struct SSOHChunkHandler final : ChunkHandler {
	/** Register one original header reader without activating a new writer. */
	SSOHChunkHandler() : ChunkHandler("SSOH", ChunkType::ReadOnly) {}
	/** @param preview Select copied transport, otherwise the sole unpublished full-file candidate. */
	void Read(bool preview) const
	{
		RequireTableSupplyKind(ChunkType::Riff);
		if (SlGetFieldLength() != 12) SlErrorCorrupt("Invalid legacy supply header length");
		std::array<uint32_t, 3> header{}; SlCopy<VarFileType::U32>(header);
		// Original schema/cardinality/frontier precede any row adoption; an
		// absent or forged cursor cannot replenish old purchase/refund rights.
		if (header[0] != 1 || header[1] > MAX_LEGACY_SURFACE_SUPPLY_RECORDS || header[2] != header[1] + 1) SlErrorCorrupt("Invalid legacy supply header bounds");
		if (preview) {
			if (_load_check_data.table_supply_present[0]) SlErrorCorrupt("Repeated copied legacy supply header");
			auto &original = _load_check_data.table_supply;
			original.schema = header[0]; original.count = header[1]; original.next_order = header[2];
			_load_check_data.table_supply_present[0] = true;
		} else if (!StageLoadedTableSupplyHeader(header[0], header[1], header[2])) {
			SlErrorCorrupt("Repeated/mismatched native legacy supply header");
		}
	}
	/** Read bounded original scalars without installing any refund rights. */
	void Load() const override { this->Read(false); }
	/** @param length Framework extent; the original fixed12-byte reader validates it independently. */
	void LoadCheck([[maybe_unused]] size_t length) const override { this->Read(true); }
};

/** Read one exact A440fixed or441tagged RIFFfamily; full and copied loading share the bound-lineage dispatch.
 * @return Detached original book/day, never installed rights or physical stock; malformed bytes raise the ordinary corrupt-save error.
 */
static SurfaceSupplyOrderTransport ReadSurfaceSupplyOrderFamily()
{
	const auto source = GetLoadCapabilities();
	// Original A440fixed and merged441tagged bytes share RIFFframing.
	// The admitted lineage selects exactly one codec; legacy B TABLEdispatch
	// remains separate and no mismatch may default to another paid-history era.
	if (!source || (source->format != SaveFormat::Orbital && source->format != SaveFormat::Unified) || SlGetChunkType() != ChunkType::Riff) SlErrorCorrupt("Wrong supply format/chunk kind");
	const auto length = SlGetFieldLength();
	const bool merged = source->format == SaveFormat::Unified;
	const auto minimum = merged ? MERGED_SURFACE_SUPPLY_HEADER_SIZE : SURFACE_SUPPLY_WIRE_HEADER_SIZE;
	const auto maximum = merged ? MAX_MERGED_SURFACE_SUPPLY_WIRE_SIZE : MAX_SURFACE_SUPPLY_WIRE_SIZE;
	// A new family requires its actual era and complete bounded bytes before allocation; old files cannot acquire imported stock or paid history by appending a header.
	if (IsSavegameVersionBefore(SaveLoadVersion::SurfaceSupplyOrders) || length < minimum || length > maximum) SlErrorCorrupt("Invalid Surface supply order family era/length");
	std::vector<uint8_t> bytes(length);
	// The framework admitted this exact family extent. Retaining each original byte cannot consume another chunk, allocate a native packet or supply absent references.
	for (auto &byte : bytes) byte = SlReadByte();
	auto transport = merged ? DecodeMergedSurfaceSupplyOrders(bytes) : DecodeSurfaceSupplyOrders(bytes);
	if (!transport) SlErrorCorrupt("Invalid Surface supply order transport");
	return std::move(*transport);
}

/** @copydoc GetSurfaceSupplyOrderLoadPreviewError */
std::optional<std::string> GetSurfaceSupplyOrderLoadPreviewError()
{
	auto &data = _load_check_data;
	const auto source = GetLoadCapabilities();
	const bool table_book = source && source->format == SaveFormat::Geothermal && SlHasSaveCapability(SaveCapability::SurfaceSupplyOrdersTable);
	if (table_book) {
		// Both original components are mandatory, even when no purchase ever
		// existed. Header absence cannot masquerade as a fresh empty book.
		if (!data.table_supply_present[0] || !data.table_supply_present[1]) return "Incomplete copied legacy supply family";
	} else if (!data.surface_supply_orders_present) {
		if (SlHasSaveCapability(SaveCapability::SurfaceSupplyOrdersFixed)) return "Missing Surface supply order family";
		return std::nullopt;
	}
	// Independent original DATE/MAPS and the actual normal-station family must
	// precede conversion; no live clock, map or same-numbered station fills them.
	if (!data.native_economy_date_present || !data.native_map_dimensions_present || !data.native_station_cargo_present) return "Missing copied supply map/date/station family";
	SurfaceSupplyRecordContext context;
	context.profile = static_cast<MultiLayerGameplayProfileID>(data.gameplay_profile_id);
	context.width = data.map_size_x; context.height = data.map_size_y;
	// Each pending controller must be an actual same-file PLYR identity; a current playable company never fills a missing or out-of-domain candidate slot.
	for (const auto &[company, properties] : data.companies) {
		(void)properties;
		if (company >= MAX_COMPANIES) return "Invalid copied Surface supply company domain";
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
	if (table_book) {
		// Same-file metadata must retain the exact original history declaration;
		// closed orders remain history and an omitted MLMD is never empty policy.
		if (data.multilayer_schema_version != 1 || ((data.multilayer_feature_flags & MULTILAYER_FEATURE_SURFACE_SUPPLIES) != 0) != (data.table_supply.next_order != 1)) return "Changed copied legacy supply metadata/history";
		auto converted = ConvertTableSupply(data.table_supply, context);
		if (!converted) return "Invalid copied table supply records";
		data.surface_supply_orders = {std::move(*converted), data.native_economy_date};
		data.surface_supply_orders_present = true;
	} else if (data.fixed_supply_wire) {
		auto converted = ConvertFixedSupply(data.surface_supply_orders, context);
		if (!converted) return "Invalid copied fixed supply records";
		data.surface_supply_orders.snapshot = std::move(*converted);
	}
	if (data.surface_supply_orders.saved_day != data.native_economy_date) return "Changed copied supply economy day";
	// A copied paid book cannot conceal omitted CAPA link/shipped fields even
	// when every retained order is closed or no physical packet remains.
	if (table_book && !data.table_supply_packet_fields_present) return "Missing copied table supply packet fields";
	if (ValidateSurfaceSupplyRecords(data.surface_supply_orders.snapshot, context) != SurfaceSupplyRecordError::None) return "Invalid copied Surface supply order ownership/history";
	// An empty paid book with entirely ordinary packets has no entitlement or
	// non-paying purpose to join. Preserve native Classic/custom-content preview
	// behavior through the existing complete exclusivity check; do not invent
	// cargo definitions merely to attach unrelated freight to an absent receipt.
	if (data.surface_supply_orders.snapshot.records.empty() && std::ranges::none_of(data.native_cargo_packet_rows,
		/** @param row Actual copied original packet. @return Whether any permanent paid-purpose/link evidence needs the stricter shared custody join. */
		[](const NativeCargoPacketPreview &row) { return row.source.IsCompanySupply() || row.company_supply_order != 0 || row.company_supply_shipped != 0; })) {
		if (!ValidateSpaceCargoPreviewOwners(data)) return "Invalid copied ordinary cargo ownership";
		return std::nullopt;
	}
	/** @param id Exact original CAPA index. @return Stable same-file raw or legacy-projected scalars; no native packet or active-world fallback is consulted. */
	const auto lookup = [&](CargoPacketID id) -> std::optional<SurfaceSupplyPacketFacts> {
		const auto row = std::ranges::lower_bound(data.native_cargo_packet_rows, id, {}, &NativeCargoPacketPreview::packet);
		// Missing original rows cannot be supplied by a matching live packet,
		// and the literal departure scalar must survive unchanged as0/1.
		if (row == data.native_cargo_packet_rows.end() || row->packet != id || row->company_supply_shipped > 1) return std::nullopt;
		SurfaceSupplyPacketFacts facts{id, row->quantity, row->source, row->feeder_share, row->company_supply_order, row->company_supply_shipped != 0};
		return table_book ? ProjectTableSupplyPacket(facts, data.surface_supply_orders.snapshot) : std::optional{facts};
	};
	SurfaceSupplyCustodyCheck check(data.surface_supply_orders.snapshot, context, lookup, data.native_cargo_packet_rows.size());
	if (!ValidateSpaceCargoPreviewOwners(data, &check)) return "Invalid copied supply container custody";
	// Actual ownership precedes the complete ordered CAPA pass: allocation
	// cannot stand in for stock, and paid orphan rows cannot finance refunds.
	for (const auto &row : data.native_cargo_packet_rows) {
		if (!check.ObserveAllocated(row.packet)) return "Invalid copied allocated supply packet";
	}
	if (!check.Valid()) return "Incomplete copied supply packet custody";
	return std::nullopt;
}

/** Validate copied supply evidence inside the native load-check error boundary.
 * The shared validator returns the original reason; only this framework-facing
 * wrapper invokes corrupt-save reporting and its action-specific cleanup.
 */
void ValidateSurfaceSupplyOrderLoadPreview()
{
	if (const auto error = GetSurfaceSupplyOrderLoadPreviewError()) SlErrorCorrupt(*error);
}

/** One SSORowner: original A440fixed RIFF, B TABLEor merged tagged RIFF; native containers retain sole physical stock. */
struct SSORChunkHandler : ChunkHandler {
	/** Register the unique native metadata family; construction allocates no order, cargo or financial right. */
	SSORChunkHandler() : ChunkHandler("SSOR", ChunkType::Riff) {}
	/** Serialize only the fully joined canonical tagged book and actual native economy-day stamp; never dispatch a pending order. */
	void Save() const override
	{
		if (!ValidateSurfaceSupplyOrderBook()) SlErrorCorrupt("Invalid native Surface supply order book on save");
		const auto bytes = EncodeMergedSurfaceSupplyOrders({CaptureSurfaceSupplyOrders(), TimerGameEconomy::date});
		if (!bytes) SlErrorCorrupt("Invalid native Surface supply order transport on save");
		SlSetLength(bytes->size());
		// Native/copy readers receive the same exact original commitment/history bytes; no scalar receipt becomes a serialized second inventory.
		for (const auto byte : *bytes) SlWriteByte(byte);
	}
	/** Stage one original full family; all native owners/date must reconstruct before adoption can expose any pending right. */
	void Load() const override
	{
		if (SlGetChunkType() == ChunkType::Table) { ReadTableSupplyRows(false); return; }
		auto transport = ReadSurfaceSupplyOrderFamily();
		if (!StageLoadedSurfaceSupplyOrders(std::move(transport.snapshot), transport.saved_day, GetLoadCapabilities()->format == SaveFormat::Orbital)) SlErrorCorrupt("Repeated/invalid native Surface supply order staging");
	}
	/** Copy only this candidate's original family; post-family admission resolves its own native facts without touching live owners.
	 * @param length Framework-admitted extent; the shared original decoder independently bounds its exact payload.
	 */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (SlGetChunkType() == ChunkType::Table) { ReadTableSupplyRows(true); return; }
		if (_load_check_data.surface_supply_orders_present) SlErrorCorrupt("Repeated copied Surface supply order family");
		_load_check_data.surface_supply_orders = ReadSurfaceSupplyOrderFamily();
		_load_check_data.surface_supply_orders_present = true;
		_load_check_data.fixed_supply_wire = GetLoadCapabilities()->format == SaveFormat::Orbital;
	}
	/** Failed native loading revokes partly adopted rights; successful pointer fixups leave adoption to the complete afterload join. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) ResetSurfaceSupplyOrders();
	}
};

static const SSOHChunkHandler SSOH;
static const SSORChunkHandler SSOR;
static const ChunkHandlerRef supply_order_handlers[] = {SSOH, SSOR};
extern const ChunkHandlerTable _surface_supply_order_chunk_handlers(supply_order_handlers);
