/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_receipts.cpp Independent native/legacy provenance and finite-history refusals.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native custody boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-c-supply-receipts.md
 * These tests declare metadata only; no receipt manufactures cash or physical cargo.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "surface_supply_record_fixture.h"
#include "../surface_supply_order_wire.h"
#include "../timer/timer_game_calendar.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/** Declare B's original100-unit RFMT purchase at its frozen25-unit tariff.
 * @param id Original dense purchase identity, independent of any runtime packet index.
 * @return Legacy-only evidence with no native payer/anchor/inflation/issuance fields.
 */
static SurfaceSupplyOrderRecord LegacyRecord(SurfaceSupplyOrderID id = 1)
{
	SurfaceSupplyOrderRecord record;
	record.id = id; record.owner = CompanyID{0}; record.origin = StationID{3};
	record.label = CargoLabel{"RFMT"}; record.quantity = 100; record.purchase_day = TimerGameEconomy::Date{100};
	record.receipt = TableSupplyReceipt{Money{25}, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1).base(), 0, 0, false};
	return record;
}

/** Native and B receipts retain different provenance/cap rules in one uint64 lookup owner. */
TEST_CASE("Tagged Surface receipts preserve original evidence without inventing native facts", "[surface-supply-receipts]")
{
	SurfaceSupplyRecordRig rig;
	auto legacy = LegacyRecord(); legacy.Legacy()->dispatched = 40;
	auto native = rig.Record(uint64_t{1} << 40, StationID{3}, CompanyID{0}, CargoLabel{"RFMT"}, 2);
	rig.snapshot = {native.id + 1, {legacy, native}};
	const auto before = rig.snapshot; const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
	REQUIRE(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	CHECK(FindSurfaceSupplyOrderRecord(rig.snapshot, 1)->Native() == nullptr);
	CHECK(FindSurfaceSupplyOrderRecord(rig.snapshot, native.id)->Legacy() == nullptr);
	CHECK(FindSurfaceSupplyOrderRecord(rig.snapshot, 2) == nullptr);
	CHECK(FindSurfaceSupplyOrderRecord(rig.snapshot, rig.snapshot.next_id) == nullptr);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"RFMT"}, TimerGameEconomy::Date{100}) == 102);
	CHECK(rig.snapshot == before); CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	// B source stock may survive loss of the freight berth. Native Pending
	// admission still needs that berth; neither validator relaxes the other's rule.
	rig.snapshot.records.pop_back(); rig.stations[0].freight_facility = false;
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	rig.snapshot.records.push_back(native);
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::Station);
}

/** Cancellation cannot release B's consumed day allowance or restore a dispatched entitlement. */
TEST_CASE("Legacy Surface receipts keep refunded cap history and explicit loss sentinels", "[surface-supply-receipts]")
{
	SurfaceSupplyRecordRig rig; auto record = LegacyRecord();
	record.Legacy()->refunded = 60; record.Legacy()->dispatched = 40; record.Legacy()->closed = true;
	rig.snapshot = {2, {record}};
	REQUIRE(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"RFMT"}, TimerGameEconomy::Date{100}) == 100);
	rig.snapshot.records[0].origin = StationID::Invalid(); rig.context.stations = {};
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	CHECK(rig.snapshot.records[0].Native() == nullptr);
	rig.snapshot.records[0].owner = CompanyID::Invalid(); rig.context.companies.fill(false);
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"RFMT"}, TimerGameEconomy::Date{100}) == 0);
	CHECK(rig.snapshot.records[0].Legacy()->unit_price == 25);
}

/** Each independent scalar/reference fault must refuse before native ownership exists. */
TEST_CASE("Legacy Surface receipt validation rejects forged histories and unknown tags", "[surface-supply-receipts]")
{
	SurfaceSupplyRecordRig rig; rig.snapshot = {2, {LegacyRecord()}};
	REQUIRE(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	auto &record = rig.snapshot.records[0]; auto *legacy = record.Legacy();
	SurfaceSupplyRecordError expected = SurfaceSupplyRecordError::State;
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19);
	switch (fault) {
		case 0: record.quantity = 0; expected = SurfaceSupplyRecordError::Quantity; break;
		case 1: legacy->unit_price = 0; expected = SurfaceSupplyRecordError::Price; break;
		case 2: legacy->unit_price = Money{INT64_MAX / 100 + 1}; expected = SurfaceSupplyRecordError::Price; break;
		case 3: legacy->refunded = 101; break;
		case 4: legacy->refunded = 1; break;
		case 5: legacy->refunded = 60; legacy->dispatched = 41; legacy->closed = true; break;
		case 6: record.purchase_day = TimerGameEconomy::Date{-1}; expected = SurfaceSupplyRecordError::Date; break;
		case 7: legacy->calendar_date = CalendarTime::MAX_DATE.base() + 1; expected = SurfaceSupplyRecordError::Date; break;
		case 8: legacy->calendar_date = 0; expected = SurfaceSupplyRecordError::Date; break;
		case 9: record.owner = CompanyID{MAX_COMPANIES}; expected = SurfaceSupplyRecordError::Company; break;
		case 10: record.origin = StationID::Invalid(); break;
		case 11: record.owner = CompanyID::Invalid(); legacy->closed = true; expected = SurfaceSupplyRecordError::Company; break;
		case 12: record.id = 7; rig.snapshot.next_id = 8; expected = SurfaceSupplyRecordError::Identity; break;
		case 13: record.receipt = std::monostate{}; break;
		case 14: rig.snapshot.next_id = 0; expected = SurfaceSupplyRecordError::Identity; break;
		case 15: rig.context.companies[0] = false; expected = SurfaceSupplyRecordError::Company; break;
		case 16: rig.stations[0].layer = MapLayer::Underground; expected = SurfaceSupplyRecordError::Station; break;
		case 17: record.label = CargoLabel{"PROP"}; expected = SurfaceSupplyRecordError::Cargo; break;
		case 18: record.origin = StationID{7}; rig.stations[1].owner = CompanyID{1}; expected = SurfaceSupplyRecordError::Station; break;
		case 19: record.purchase_day = TimerGameEconomy::Date{EconomyTime::MAX_DATE.base() + 1}; expected = SurfaceSupplyRecordError::Date; break;
	}
	const auto before = rig.snapshot; const auto packets = CargoPacket::GetNumItems();
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == expected);
	CHECK(rig.snapshot == before); CHECK(CargoPacket::GetNumItems() == packets);
}

/** Complete old paid history may load above new-purchase admission limits without truncation. */
TEST_CASE("Canonical Surface metadata admits all65535 legacy rows but no extra history", "[surface-supply-receipts]")
{
	SurfaceSupplyRecordRig rig; rig.snapshot.next_id = MAX_LEGACY_SURFACE_SUPPLY_RECORDS + 1;
	rig.snapshot.records.reserve(MAX_LEGACY_SURFACE_SUPPLY_RECORDS + 1);
	// Every original dense B identity survives, even after refund/closure. This
	// deliberately tests the real retained bound rather than a small lookalike book.
	for (size_t index = 1; index <= MAX_LEGACY_SURFACE_SUPPLY_RECORDS; ++index) rig.snapshot.records.push_back(LegacyRecord(index));
	const auto packets = CargoPacket::GetNumItems();
	REQUIRE(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"RFMT"}, TimerGameEconomy::Date{100}) == 6553500);
	CHECK(FindSurfaceSupplyOrderRecord(rig.snapshot, 65535) == &rig.snapshot.records.back());
	CHECK_FALSE(EncodeSurfaceSupplyOrders({rig.snapshot, TimerGameEconomy::Date{100}}));
	rig.snapshot.records.push_back(LegacyRecord(65536)); ++rig.snapshot.next_id;
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::Size);
	CHECK_FALSE(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"RFMT"}, TimerGameEconomy::Date{100}));
	CHECK(CargoPacket::GetNumItems() == packets);
}

/** The untagged A440 transport cannot erase a legacy or unknown receipt's provenance. */
TEST_CASE("Original Surface wire refuses receipt variants it cannot represent", "[surface-supply-receipts]")
{
	SurfaceSupplyOrderTransport transport{{2, {LegacyRecord()}}, TimerGameEconomy::Date{100}};
	CHECK_FALSE(EncodeSurfaceSupplyOrders(transport));
	transport.snapshot.records[0].receipt = std::monostate{};
	CHECK_FALSE(EncodeSurfaceSupplyOrders(transport));
	CHECK_FALSE(CountSurfaceSupplyDayUsage(transport.snapshot, CompanyID{0}, CargoLabel{"RFMT"}, TimerGameEconomy::Date{100}));
}
