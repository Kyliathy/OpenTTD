/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_merged_wire.cpp Independent frozen441tagged bytes, streaming inspection and malformed evidence boundaries.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md
 * The literal below is authored from the wire contract, never today's encoder.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../surface_supply_merged_wire.h"
#include "save_envelope_fixture.h"
#include "../safeguards.h"

/** Return independent two-era bytes: LegacyBid2followed by Nativeid4294967297.
 * @return Frozen20-byte header,39-byte B row and54-byte Native row; no current writer or world supplies expectations.
 */
static std::vector<uint8_t> OriginalMergedSupplyBytes()
{
	return EnvelopeHex(
		"53534d31" "0001" "0000000100000002" "00000028" "0002"
		"02" "0000000000000002" "00" "0001" "52464d54" "0008" "00000028"
		"0000000000000013" "000aae60" "0002" "0003" "00"
		"01" "0000000100000001" "00" "0001" "52464d54" "0004" "00000028"
		"00" "000a" "0014" "000000000000006c" "0000000000010000" "000007d0" "0000" "01" "00000006");
}

/** A single explicit tagged transport preserves both original evidence variants and identities beyond U32. */
TEST_CASE("Merged supply retains independently authored two-era receipts", "[surface-supply-merged-wire]")
{
	const auto bytes = OriginalMergedSupplyBytes(); REQUIRE(bytes.size() == 113);
	const auto transport = DecodeMergedSurfaceSupplyOrders(bytes); REQUIRE(transport.has_value());
	CHECK(transport->snapshot.next_id == 0x100000002ULL); CHECK(transport->saved_day.base() == 40);
	REQUIRE(transport->snapshot.records.size() == 2);
	const auto &legacy = transport->snapshot.records[0], &native = transport->snapshot.records[1];
	CHECK(legacy.id == 2); CHECK(legacy.quantity == 8); REQUIRE(legacy.Legacy() != nullptr); CHECK(legacy.Native() == nullptr);
	CHECK(legacy.Legacy()->unit_price == Money{19}); CHECK(legacy.Legacy()->calendar_date == 700000);
	CHECK(legacy.Legacy()->refunded == 2); CHECK(legacy.Legacy()->dispatched == 3); CHECK_FALSE(legacy.Legacy()->closed);
	CHECK(native.id == 0x100000001ULL); CHECK(native.quantity == 4); REQUIRE(native.Native() != nullptr); CHECK(native.Legacy() == nullptr);
	CHECK(native.Native()->origin_x == 10); CHECK(native.Native()->origin_y == 20); CHECK(native.Native()->cost == Money{108});
	CHECK(native.Native()->inflation_prices == 65536); CHECK(native.Native()->purchase_year.base() == 2000);
	CHECK(native.Native()->state == SurfaceSupplyOrderState::Fulfilled); CHECK(native.Native()->issued_packet == CargoPacketID{6});
	const auto encoded = EncodeMergedSurfaceSupplyOrders(*transport); REQUIRE(encoded.has_value()); CHECK(*encoded == bytes);
	CHECK_FALSE(DecodeSurfaceSupplyOrders(bytes).has_value());
	size_t cursor = 0;
	/** @param width Frozen scalar width requested by the allocation-free inspector. @return Independently read original network-order bits. */
	const auto read = [&](size_t width) {
		REQUIRE(width <= bytes.size() - cursor); uint64_t value = 0;
		for (size_t index = 0; index < width; ++index) value = (value << 8) | bytes[cursor++];
		return value;
	};
	const auto summary = InspectMergedSurfaceSupplyOrders(bytes.size(), read); REQUIRE(summary.has_value());
	CHECK(summary->next_id == 0x100000002ULL); CHECK(summary->records == 2); CHECK(summary->saved_day.base() == 40); CHECK(cursor == bytes.size());
}

/** Unknown provenance, unsafe money/counters and contradictory extents refuse before they can become native refund or stock rights. */
TEST_CASE("Merged supply refuses altered tags counts money and original evidence", "[surface-supply-merged-wire]")
{
	const auto fault = GENERATE(range(0, 17)); INFO(fault); auto bytes = OriginalMergedSupplyBytes();
	// Every mutation changes one independently located wire fact. No native
	// owner or today's encoder rebuilds a convenient alternative expected row.
	switch (fault) {
		case 0: bytes[0] = 0; break;
		case 1: bytes[5] = 2; break;
		case 2: bytes[19] = 3; break;
		case 3: bytes[19] = 1; break;
		case 4: bytes[59] = 3; break;
		case 5: bytes[58] = 2; break;
		case 6: bytes[86] = 0x80; break;
		case 7: bytes[42] = 0x80; break;
		case 8: std::fill(bytes.begin() + 60, bytes.begin() + 68, 0); bytes[67] = 2; break;
		case 9: std::fill(bytes.begin() + 6, bytes.begin() + 14, 0); break;
		case 10: bytes[108] = 255; break;
		case 11: bytes.push_back(0); break;
		case 12: bytes.pop_back(); break;
		case 13: bytes[19] = 0; break;
		case 14: bytes[18] = bytes[19] = 255; break;
		case 15: bytes[54] = bytes[55] = 255; break;
		case 16: bytes[56] = bytes[57] = 255; break;
	}
	CHECK_FALSE(DecodeMergedSurfaceSupplyOrders(bytes).has_value());
	size_t cursor = 0;
	/** @param width Shared scanner's bounded scalar request. @return Independent malformed-source bits, without a raw payload copy or native record allocation. */
	const auto read = [&](size_t width) {
		REQUIRE(width <= bytes.size() - cursor); uint64_t value = 0;
		for (size_t index = 0; index < width; ++index) value = (value << 8) | bytes[cursor++];
		return value;
	};
	CHECK_FALSE(InspectMergedSurfaceSupplyOrders(bytes.size(), read).has_value());
}

/** The wider imported-history transport cannot enlarge Native purchase policy or accept an invalid variant. */
TEST_CASE("Merged history bounds preserve original native admission limit", "[surface-supply-merged-wire]")
{
	auto transport = *DecodeMergedSurfaceSupplyOrders(OriginalMergedSupplyBytes());
	const auto native = transport.snapshot.records.back(); transport.snapshot.records.clear();
	transport.snapshot.next_id = MAX_SURFACE_SUPPLY_RECORDS + 2;
	// Distinct issued Native IDs deliberately exceed only their existing policy
	// bound; the larger B history allowance cannot turn them into legal purchases.
	for (size_t index = 0; index <= MAX_SURFACE_SUPPLY_RECORDS; ++index) {
		auto row = native; row.id = index + 1; transport.snapshot.records.push_back(row);
	}
	CHECK_FALSE(EncodeMergedSurfaceSupplyOrders(transport).has_value());
	transport.snapshot.records.resize(1); transport.snapshot.records[0].receipt = std::monostate{};
	CHECK_FALSE(EncodeMergedSurfaceSupplyOrders(transport).has_value());
}


/** Every persisted receipt fact affects diagnostics, including original evidence unavailable in the other era. */
TEST_CASE("Merged supply diagnostics retain every original variant field", "[surface-supply-merged-digest]")
{
	const auto original = DecodeMergedSurfaceSupplyOrders(OriginalMergedSupplyBytes()); REQUIRE(original);
	const auto before = ComputeSurfaceSupplyOrderDigest(original->snapshot); auto changed = original->snapshot;
	const auto fault = GENERATE(range(0, 29)); INFO(fault);
	auto &legacy = changed.records[0]; auto &native = changed.records[1];
	// A single original fact changes per case. The digest must distinguish each
	// paid/refunded/dispatched history without asking the encoder for expected data.
	switch (fault) {
		case 0: ++changed.next_id; break;
		case 1: ++legacy.id; break;
		case 2: legacy.owner = CompanyID{1}; break;
		case 3: legacy.origin = StationID{2}; break;
		case 4: legacy.label = CargoLabel{"STEL"}; break;
		case 5: ++legacy.quantity; break;
		case 6: legacy.purchase_day = TimerGameEconomy::Date{41}; break;
		case 7: legacy.Legacy()->unit_price = Money{20}; break;
		case 8: ++legacy.Legacy()->calendar_date; break;
		case 9: ++legacy.Legacy()->refunded; break;
		case 10: ++legacy.Legacy()->dispatched; break;
		case 11: legacy.Legacy()->closed = true; break;
		case 12: native.Native()->purchaser = CompanyID{1}; break;
		case 13: ++native.Native()->origin_x; break;
		case 14: ++native.Native()->origin_y; break;
		case 15: native.Native()->cost = Money{109}; break;
		case 16: ++native.Native()->inflation_prices; break;
		case 17: native.Native()->purchase_year = TimerGameCalendar::Year{2001}; break;
		case 18: ++native.Native()->packet_buyer; break;
		case 19: native.Native()->state = SurfaceSupplyOrderState::Lost; break;
		case 20: native.Native()->issued_packet = CargoPacketID{7}; break;
		case 21: native.id = 1; break;
		case 22: native.owner = CompanyID{1}; break;
		case 23: native.origin = StationID{2}; break;
		case 24: native.label = CargoLabel{"STEL"}; break;
		case 25: ++native.quantity; break;
		case 26: native.purchase_day = TimerGameEconomy::Date{41}; break;
		case 27: changed.records.pop_back(); break;
		case 28: native.receipt = *legacy.Legacy(); break;
	}
	CHECK(ComputeSurfaceSupplyOrderDigest(changed) != before);
	CHECK(ComputeSurfaceSupplyOrderDigest(original->snapshot) == before);
}
