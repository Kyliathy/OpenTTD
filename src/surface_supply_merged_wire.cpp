/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_merged_wire.cpp One pure streaming441codec for preserved native and original B receipt evidence.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md
 * No callback here resolves a live company, cargo slot, station, packet or wallet.
 */
#include "stdafx.h"
#include "surface_supply_merged_wire.h"
#include <bit>
#include "safeguards.h"

/** Frozen merged receipt discriminants; unavailable original B facts are never fabricated as Native evidence. */
enum class MergedSupplyReceiptTag : uint8_t { Native = 1, LegacyB = 2 };

/** Check raw-representable evidence without borrowing current price or gameplay rights.
 * @param record One exact canonical receipt after raw boolean/tag admission.
 * @return Whether its immutable money/state/counters are safe to serialize or inspect; reference validation remains the shared owner's job.
 */
static bool ValidMergedSupplyScalars(const SurfaceSupplyOrderRecord &record)
{
	if (record.quantity == 0) return false;
	if (const auto *native = record.Native()) {
		// Negative charges and future state discriminants cannot become a saved
		// refund promise or reinterpret another native settlement outcome.
		return native->cost >= 0 && to_underlying(native->state) <= to_underlying(SurfaceSupplyOrderState::Lost);
	}
	if (const auto *legacy = record.Legacy()) {
		const auto price = static_cast<int64_t>(legacy->unit_price);
		// Retained B counters and exact tariff must fit one original purchase;
		// inspection cannot hide overflow or turn already removed units into stock.
		return price > 0 && price <= INT64_MAX / record.quantity && legacy->refunded <= record.quantity &&
			legacy->dispatched <= record.quantity - legacy->refunded;
	}
	return false;
}

/** Walk the complete pure merged family using bounded unsigned scalar reads.
 * @param length Exact original family extent, admitted before header/count reads.
 * @param input Caller-owned network-order scalar reader; this function bounds every requested width.
 * @param retain True builds detached native-load records; false inspects on the stack without a record vector or payload copy.
 * @param summary Header facts published to the caller only when the whole walk succeeds.
 * @return Complete detached transport or refusal; no gameplay owner or quantity is installed.
 */
static std::optional<SurfaceSupplyOrderTransport> ReadMergedSupplyWire(size_t length, const std::function<uint64_t(size_t)> &input,
	bool retain, MergedSurfaceSupplyWireSummary &summary)
{
	// Bound the complete source before even reading a count. Mixed retained
	// history may exceed new-purchase policy, but never the existing legacy cap.
	if (length < MERGED_SURFACE_SUPPLY_HEADER_SIZE || length > MAX_MERGED_SURFACE_SUPPLY_WIRE_SIZE) return std::nullopt;
	size_t remaining = length; bool valid = true;
	/** @param width Exact frozen field width. @return Original bits, or inert zero after recording truncation; no read crosses this family. */
	const auto read = [&](size_t width) -> uint64_t {
		if (width > remaining) { valid = false; return 0; }
		remaining -= width; return input(width);
	};
	const auto magic = read(4), schema = read(2); SurfaceSupplyOrderTransport transport;
	transport.snapshot.next_id = read(8); transport.saved_day = TimerGameEconomy::Date{std::bit_cast<int32_t>(static_cast<uint32_t>(read(4)))};
	const auto count = static_cast<uint16_t>(read(2));
	// A wrong schema/magic/frontier or impossible minimum row budget cannot
	// allocate a candidate or disguise an original A440fixed family as merged.
	if (!valid || magic != MERGED_SURFACE_SUPPLY_MAGIC || schema != MERGED_SURFACE_SUPPLY_SCHEMA || transport.snapshot.next_id == 0 ||
		remaining < size_t{count} * MERGED_SURFACE_SUPPLY_LEGACY_SIZE) return std::nullopt;
	if (retain) transport.snapshot.records.reserve(count);
	uint64_t previous = 0; size_t native_count = 0;
	// Read every tagged receipt exactly once. Historical counters remain metadata;
	// neither variant carries a second packet inventory or reconstructed payer.
	for (uint32_t index = 0; index < count; ++index) {
		const auto tag = static_cast<MergedSupplyReceiptTag>(read(1));
		const auto size = tag == MergedSupplyReceiptTag::Native ? MERGED_SURFACE_SUPPLY_NATIVE_SIZE : MERGED_SURFACE_SUPPLY_LEGACY_SIZE;
		// Unknown tags and truncated variant payloads refuse before any tag can
		// default to Native or erase unavailable original B evidence.
		if ((tag != MergedSupplyReceiptTag::Native && tag != MergedSupplyReceiptTag::LegacyB) || remaining < size - 1) return std::nullopt;
		SurfaceSupplyOrderRecord record; record.id = read(8); record.owner = CompanyID{static_cast<uint8_t>(read(1))};
		record.origin = StationID{static_cast<uint16_t>(read(2))};
		for (auto &byte : record.label) byte = static_cast<uint8_t>(read(1));
		record.quantity = static_cast<uint16_t>(read(2)); record.purchase_day = TimerGameEconomy::Date{std::bit_cast<int32_t>(static_cast<uint32_t>(read(4)))};
		if (tag == MergedSupplyReceiptTag::Native) {
			auto &native = *record.Native(); native.purchaser = CompanyID{static_cast<uint8_t>(read(1))};
			native.origin_x = static_cast<uint16_t>(read(2)); native.origin_y = static_cast<uint16_t>(read(2));
			const auto charge = read(8); if (charge > INT64_MAX) return std::nullopt;
			native.cost = Money{static_cast<int64_t>(charge)}; native.inflation_prices = read(8);
			native.purchase_year = TimerGameCalendar::Year{std::bit_cast<int32_t>(static_cast<uint32_t>(read(4)))};
			native.packet_buyer = static_cast<SourceID>(read(2)); native.state = static_cast<SurfaceSupplyOrderState>(read(1));
			native.issued_packet = CargoPacketID{static_cast<uint32_t>(read(4))};
			if (++native_count > MAX_SURFACE_SUPPLY_RECORDS) return std::nullopt;
		} else {
			LegacyBSupplyReceipt legacy; const auto price = read(8); if (price > INT64_MAX) return std::nullopt;
			legacy.unit_price = Money{static_cast<int64_t>(price)}; legacy.calendar_date = std::bit_cast<int32_t>(static_cast<uint32_t>(read(4)));
			legacy.refunded = static_cast<uint16_t>(read(2)); legacy.dispatched = static_cast<uint16_t>(read(2)); const auto closed = read(1);
			if (closed > 1) return std::nullopt;
			legacy.closed = closed != 0; record.receipt = legacy;
		}
		// Ordered issued identities and safe scalar evidence must agree for
		// every row, including rows the detached inspector deliberately discards.
		if (!valid || record.id <= previous || record.id >= transport.snapshot.next_id || !ValidMergedSupplyScalars(record)) return std::nullopt;
		previous = record.id; if (retain) transport.snapshot.records.push_back(std::move(record));
	}
	if (remaining != 0) return std::nullopt;
	summary = {transport.snapshot.next_id, transport.saved_day, count}; return transport;
}

/** @copydoc DecodeMergedSurfaceSupplyOrders */
std::optional<SurfaceSupplyOrderTransport> DecodeMergedSurfaceSupplyOrders(std::span<const uint8_t> bytes)
{
	size_t cursor = 0; MergedSurfaceSupplyWireSummary summary;
	/** @param width Admitted frozen scalar width. @return Original network-order bits; the shared walker enforces the complete span boundary. */
	const auto read = [&](size_t width) {
		uint64_t value = 0;
		for (size_t byte = 0; byte < width; ++byte) value = (value << 8) | bytes[cursor++];
		return value;
	};
	return ReadMergedSupplyWire(bytes.size(), read, true, summary);
}

/** @copydoc InspectMergedSurfaceSupplyOrders */
std::optional<MergedSurfaceSupplyWireSummary> InspectMergedSurfaceSupplyOrders(size_t length, const std::function<uint64_t(size_t)> &read)
{
	MergedSurfaceSupplyWireSummary summary;
	if (!ReadMergedSupplyWire(length, read, false, summary)) return std::nullopt;
	return summary;
}

/** @copydoc EncodeMergedSurfaceSupplyOrders */
std::optional<std::vector<uint8_t>> EncodeMergedSurfaceSupplyOrders(const SurfaceSupplyOrderTransport &transport)
{
	const auto &snapshot = transport.snapshot;
	// Retain the existing mixed-history limit and monotonic issuer before any
	// output allocation; serialization cannot widen new-purchase admission.
	if (snapshot.next_id == 0 || snapshot.records.size() > MAX_LEGACY_SURFACE_SUPPLY_RECORDS) return std::nullopt;
	std::vector<uint8_t> bytes; bytes.reserve(MERGED_SURFACE_SUPPLY_HEADER_SIZE + snapshot.records.size() * MERGED_SURFACE_SUPPLY_NATIVE_SIZE);
	/** @param value Frozen original unsigned bits. @param width Exact network-order scalar width. Emit metadata only, never stock or new identity. */
	const auto write = [&](uint64_t value, size_t width) {
		for (size_t byte = 0; byte < width; ++byte) bytes.push_back(static_cast<uint8_t>(value >> (8 * (width - byte - 1))));
	};
	write(MERGED_SURFACE_SUPPLY_MAGIC, 4); write(MERGED_SURFACE_SUPPLY_SCHEMA, 2); write(snapshot.next_id, 8);
	write(static_cast<uint32_t>(transport.saved_day.base()), 4); write(snapshot.records.size(), 2);
	uint64_t previous = 0; size_t native_count = 0;
	// Serialize exact variant evidence and common identity history. The Native
	// tag retains Q16charge/payer facts; LegacyBretains only its real tariff/counters.
	for (const auto &record : snapshot.records) {
		// Invalid variants, duplicate/reissued identities and malformed scalar
		// history must refuse the whole output rather than publish partial bytes.
		if (record.id <= previous || record.id >= snapshot.next_id || !ValidMergedSupplyScalars(record)) return std::nullopt;
		previous = record.id; write(to_underlying(record.Native() != nullptr ? MergedSupplyReceiptTag::Native : MergedSupplyReceiptTag::LegacyB), 1);
		write(record.id, 8); write(record.owner.base(), 1); write(record.origin.base(), 2);
		for (const auto byte : record.label) write(byte, 1);
		write(record.quantity, 2); write(static_cast<uint32_t>(record.purchase_day.base()), 4);
		if (const auto *native = record.Native()) {
			if (++native_count > MAX_SURFACE_SUPPLY_RECORDS) return std::nullopt;
			write(native->purchaser.base(), 1); write(native->origin_x, 2); write(native->origin_y, 2); write(static_cast<uint64_t>(native->cost), 8);
			write(native->inflation_prices, 8); write(static_cast<uint32_t>(native->purchase_year.base()), 4); write(native->packet_buyer, 2);
			write(to_underlying(native->state), 1); write(native->issued_packet.base(), 4);
		} else {
			const auto &legacy = *record.Legacy(); write(static_cast<uint64_t>(legacy.unit_price), 8); write(static_cast<uint32_t>(legacy.calendar_date), 4);
			write(legacy.refunded, 2); write(legacy.dispatched, 2); write(legacy.closed, 1);
		}
	}
	return bytes;
}
