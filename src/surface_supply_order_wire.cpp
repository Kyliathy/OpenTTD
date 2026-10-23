/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order_wire.cpp Fixed bounded original metadata codec shared by native/full and copied loading.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCC.
 * Bytes contain commitment/history only; native containers retain every physical cargo reference.
 */
#include "stdafx.h"
#include "surface_supply_order_wire.h"
#include <bit>
#include "safeguards.h"

/** Append one original fixed scalar; this transport helper creates no native authority.
 * @param bytes Canonical family output, already bounded by its complete record count.
 * @param value Original unsigned scalar representation.
 * @param width Fixed native little-endian byte width.
 */
static void WriteSurfaceSupplyScalar(std::vector<uint8_t> &bytes, uint64_t value, size_t width)
{
	// Original fixed field widths preserve every identity/price/date bit; byte emission cannot narrow a receipt into another purchase or stock account.
	for (size_t byte = 0; byte < width; ++byte) bytes.push_back(static_cast<uint8_t>(value >> (8 * byte)));
}

/** Reader over an already length-admitted fixed family; it cannot seek into another chunk or borrow live defaults. */
class SurfaceSupplyWireReader {
	std::span<const uint8_t> bytes; ///< Exact original bounded family.
	size_t cursor = 0; ///< Read-only offset; the admitted fixed record shape bounds every field.
public:
	/** @param bytes Exact structurally admitted original payload. */
	explicit SurfaceSupplyWireReader(std::span<const uint8_t> bytes) : bytes(bytes) {}
	/** @param width Fixed original scalar width. @return Exact original unsigned representation; caller has admitted the complete fixed field layout. */
	uint64_t Read(size_t width)
	{
		assert(width <= this->bytes.size() - this->cursor);
		uint64_t value = 0;
		// Decode each original fixed byte once; no data beyond this admitted family may complete a truncated price, identity or clock.
		for (size_t byte = 0; byte < width; ++byte) value |= uint64_t{this->bytes[this->cursor++]} << (8 * byte);
		return value;
	}
};

/** @copydoc EncodeSurfaceSupplyOrders */
std::optional<std::vector<uint8_t>> EncodeSurfaceSupplyOrders(const SurfaceSupplyOrderTransport &transport)
{
	const auto &snapshot = transport.snapshot;
	// Admit the whole bounded identity family before reserving output memory; no partial native family may publish a replacement issuance frontier.
	if (snapshot.records.size() > MAX_SURFACE_SUPPLY_RECORDS || snapshot.next_id == 0) return std::nullopt;
	std::vector<uint8_t> bytes; bytes.reserve(SURFACE_SUPPLY_WIRE_HEADER_SIZE + snapshot.records.size() * SURFACE_SUPPLY_WIRE_RECORD_SIZE);
	WriteSurfaceSupplyScalar(bytes, snapshot.next_id, 8); WriteSurfaceSupplyScalar(bytes, static_cast<uint32_t>(transport.saved_day.base()), 4); WriteSurfaceSupplyScalar(bytes, snapshot.records.size(), 2);
	// Emit original scalar receipts in their existing order. Cargo quantities remain history/commitments and cannot become a second serialized physical stock.
	for (const auto &record : snapshot.records) {
		const auto *evidence = record.Native();
		// A440 has no variant tag or legacy tariff fields. Refuse other receipts
		// instead of fabricating native payer/inflation/anchor facts for old bytes.
		if (evidence == nullptr || evidence->cost < 0) return std::nullopt;
		const auto &native = *evidence;
		WriteSurfaceSupplyScalar(bytes, record.id, 8); WriteSurfaceSupplyScalar(bytes, native.purchaser.base(), 1); WriteSurfaceSupplyScalar(bytes, record.owner.base(), 1);
		WriteSurfaceSupplyScalar(bytes, record.origin.base(), 2); WriteSurfaceSupplyScalar(bytes, native.origin_x, 2); WriteSurfaceSupplyScalar(bytes, native.origin_y, 2);
		// Preserve the actual four-byte canonical label, independent of the candidate world's configured cargo ordinal.
		for (const auto byte : record.label) bytes.push_back(byte);
		WriteSurfaceSupplyScalar(bytes, record.quantity, 2); WriteSurfaceSupplyScalar(bytes, static_cast<uint64_t>(native.cost), 8); WriteSurfaceSupplyScalar(bytes, native.inflation_prices, 8);
		WriteSurfaceSupplyScalar(bytes, static_cast<uint32_t>(native.purchase_year.base()), 4); WriteSurfaceSupplyScalar(bytes, static_cast<uint32_t>(record.purchase_day.base()), 4);
		WriteSurfaceSupplyScalar(bytes, native.packet_buyer, 2); WriteSurfaceSupplyScalar(bytes, to_underlying(native.state), 1); WriteSurfaceSupplyScalar(bytes, native.issued_packet.base(), 4);
	}
	assert(bytes.size() == SURFACE_SUPPLY_WIRE_HEADER_SIZE + snapshot.records.size() * SURFACE_SUPPLY_WIRE_RECORD_SIZE);
	return bytes;
}

/** @copydoc DecodeSurfaceSupplyOrders */
std::optional<SurfaceSupplyOrderTransport> DecodeSurfaceSupplyOrders(std::span<const uint8_t> bytes)
{
	// A complete bounded header precedes even record-count lookup; oversized/truncated families cannot allocate a native/copy vector or consume another chunk's bytes.
	if (bytes.size() < SURFACE_SUPPLY_WIRE_HEADER_SIZE || bytes.size() > MAX_SURFACE_SUPPLY_WIRE_SIZE) return std::nullopt;
	const uint16_t count = static_cast<uint16_t>(bytes[12] | (uint16_t{bytes[13]} << 8));
	if (count > MAX_SURFACE_SUPPLY_RECORDS || bytes.size() != SURFACE_SUPPLY_WIRE_HEADER_SIZE + count * SURFACE_SUPPLY_WIRE_RECORD_SIZE) return std::nullopt; // Exact whole shape excludes both invented trailing receipts and partial records before allocation.
	SurfaceSupplyWireReader reader(bytes); SurfaceSupplyOrderTransport transport;
	transport.snapshot.next_id = reader.Read(8); transport.saved_day = TimerGameEconomy::Date{std::bit_cast<int32_t>(static_cast<uint32_t>(reader.Read(4)))};
	(void)reader.Read(2); transport.snapshot.records.reserve(count);
	// Reconstruct detached original records only after exact length admission. Shared model/native-owner joins decide semantics; this loop issues neither packets nor payment rights.
	for (uint16_t index = 0; index < count; ++index) {
		SurfaceSupplyOrderRecord record;
		auto &native = *record.Native(); // The exact A440 wire layout supplies only NativeSupplyReceipt evidence.
		record.id = reader.Read(8); native.purchaser = CompanyID{static_cast<uint8_t>(reader.Read(1))}; record.owner = CompanyID{static_cast<uint8_t>(reader.Read(1))};
		record.origin = StationID{static_cast<uint16_t>(reader.Read(2))}; native.origin_x = static_cast<uint16_t>(reader.Read(2)); native.origin_y = static_cast<uint16_t>(reader.Read(2));
		// Each original label byte is retained without registering or resolving a runtime cargo slot during decoding.
		for (auto &byte : record.label) byte = static_cast<uint8_t>(reader.Read(1));
		record.quantity = static_cast<uint16_t>(reader.Read(2)); const auto cost = reader.Read(8); if (cost > static_cast<uint64_t>(INT64_MAX)) return std::nullopt;
		native.cost = Money{static_cast<int64_t>(cost)}; native.inflation_prices = reader.Read(8);
		native.purchase_year = TimerGameCalendar::Year{std::bit_cast<int32_t>(static_cast<uint32_t>(reader.Read(4)))};
		record.purchase_day = TimerGameEconomy::Date{std::bit_cast<int32_t>(static_cast<uint32_t>(reader.Read(4)))};
		native.packet_buyer = static_cast<SourceID>(reader.Read(2)); native.state = static_cast<SurfaceSupplyOrderState>(reader.Read(1)); native.issued_packet = CargoPacketID{static_cast<uint32_t>(reader.Read(4))};
		transport.snapshot.records.push_back(record);
	}
	return transport;
}

/** @copydoc ConvertLegacyBSupply */
std::optional<SurfaceSupplyOrderSnapshot> ConvertLegacyBSupply(const LegacyBSupplyTransport &original, const SurfaceSupplyRecordContext &same_file)
{
	// An absent/malformed header or partial table cannot reset paid history to
	// a fresh empty book. Admit the complete original bound before allocation.
	if (original.schema != 1 || original.count > MAX_LEGACY_SURFACE_SUPPLY_RECORDS ||
		original.count != original.orders.size() || original.next_order != original.count + 1) return std::nullopt;
	// Identity and raw closure are wire facts, not repairable canonical defaults.
	// Check every original ordinal before creating any replacement receipt.
	for (size_t index = 0; index < original.orders.size(); ++index) {
		if (original.orders[index].id != index + 1 || original.orders[index].closed > 1) return std::nullopt;
	}
	SurfaceSupplyOrderSnapshot candidate;
	candidate.next_id = original.next_order;
	candidate.records.reserve(original.count);
	// Preserve only facts B actually saved. Missing native payer, inflation,
	// anchor and issuance history remain structurally absent in the legacy tag.
	for (const auto &row : original.orders) {
		SurfaceSupplyOrderRecord record;
		record.id = row.id; record.owner = CompanyID{row.owner}; record.origin = StationID{row.station};
		record.label = row.label; record.quantity = row.quantity; record.purchase_day = TimerGameEconomy::Date{row.economy_day};
		record.receipt = LegacyBSupplyReceipt{Money{row.unit_price}, row.calendar_date, row.refunded, row.dispatched, row.closed != 0};
		candidate.records.push_back(record);
	}
	if (ValidateSurfaceSupplyRecords(candidate, same_file) != SurfaceSupplyRecordError::None) return std::nullopt;
	return candidate;
}

/** @copydoc ConvertLegacyASupply */
std::optional<SurfaceSupplyOrderSnapshot> ConvertLegacyASupply(const SurfaceSupplyOrderTransport &original, const SurfaceSupplyRecordContext &same_file)
{
	if (original.snapshot.records.size() > MAX_SURFACE_SUPPLY_RECORDS) return std::nullopt;
	// A440 has no receipt tag. A caller cannot pass an imported/unknown row
	// through this adapter to invent an original native payer or Pending right.
	for (const auto &record : original.snapshot.records) {
		if (record.Native() == nullptr) return std::nullopt;
	}
	if (ValidateSurfaceSupplyRecords(original.snapshot, same_file) != SurfaceSupplyRecordError::None) return std::nullopt;
	return original.snapshot;
}
