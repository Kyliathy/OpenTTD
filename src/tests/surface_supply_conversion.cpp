/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_conversion.cpp Independent legacy receipt conversion and unchanged-world boundaries.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native custody and finance ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-c-supply-receipts.md conversion examples.
 * These are declared same-file scalar candidates, not saved-world or physical procurement acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "surface_supply_record_fixture.h"
#include "../surface_supply_order_wire.h"
#include "../surface_supply_order_runtime.h"
#include "../company_base.h"
#include "../core/random_func.hpp"
#include "../timer/timer_game_calendar.h"
#include "../safeguards.h"

/** Declare one independently specified original B purchase, at a tariff distinct from today's native price.
 * @param id Original dense row identity; no packet or current-company lookup supplies it.
 * @return Raw historical fields, including distinct calendar and economy dates and a checked0/1 closure byte.
 */
static TableSupplyOrderWire OriginalLegacyRow(uint32_t id)
{
	return {id, 0, 3, CargoLabel{"RFMT"}, 100,
		TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1).base(), 100, 0, 0, 25, 0};
}

/** Read only actual native cash, so scalar conversion cannot hide a direct or indirect settlement.
 * @return Current company identities and exact wallets in native pool order; no company is created.
 */
static std::vector<std::pair<CompanyID, Money>> ConversionCash()
{
	std::vector<std::pair<CompanyID, Money>> cash;
	// Observe every existing wallet, including any company unrelated to these
	// declared candidates; a converter has no authority to change any account.
	for (const auto *company : Company::Iterate()) cash.emplace_back(company->index, company->money);
	return cash;
}

/** Observe the actual native owners without installing the detached test candidates. */
struct ConversionWitness {
	SurfaceSupplyOrderSnapshot installed = CaptureSurfaceSupplyOrders(); ///< Original sole live book, including current cap history.
	std::vector<std::pair<CompanyID, Money>> cash = ConversionCash(); ///< Exact original real wallets, possibly an empty pool.
	size_t packets = CargoPacket::GetNumItems(); ///< Physical pool cardinality; detached conversion cannot materialize stock.
	Randomizer random = _random; ///< Simulation RNG must remain independent of validation.
	TimerGameEconomy::Date day = TimerGameEconomy::date; ///< Actual running clock cannot be replaced with candidate history.
	/** Require unchanged native cash, packets, history, clock and RNG after success or refusal. */
	void CheckUnchanged() const
	{
		CHECK(CaptureSurfaceSupplyOrders() == this->installed);
		CHECK(ConversionCash() == this->cash); CHECK(CargoPacket::GetNumItems() == this->packets);
		CHECK(_random.state[0] == this->random.state[0]); CHECK(_random.state[1] == this->random.state[1]);
		CHECK(TimerGameEconomy::date == this->day);
	}
};

/** Preserve all original facts across shipment, refunds, station loss, takeover and company loss. */
TEST_CASE("Legacy B conversion preserves exact finite history without inventing native purchase facts", "[surface-supply-conversion]")
{
	SurfaceSupplyRecordRig rig;
	TableSupplyTransport original{1, 7, 8, {}};
	// These seven independent histories cover the saved refund/loss examples;
	// their shared old day may exceed today's cap after history combines.
	for (uint32_t id = 1; id <= 7; ++id) original.orders.push_back(OriginalLegacyRow(id));
	original.orders[1].dispatched = 40;
	original.orders[2].dispatched = 40; original.orders[2].refunded = 60; original.orders[2].closed = 1;
	original.orders[3].dispatched = 100;
	original.orders[4].station = UINT16_MAX; original.orders[4].closed = 1;
	original.orders[5].owner = 2; original.orders[5].station = 9;
	original.orders[6].owner = UINT8_MAX; original.orders[6].station = UINT16_MAX; original.orders[6].closed = 1;
	rig.context.companies[2] = true; rig.stations[2].owner = CompanyID{2}; rig.stations[0].freight_facility = false;
	const auto before = original; const ConversionWitness witness;
	const auto converted = ConvertTableSupply(original, rig.context);
	REQUIRE(converted.has_value()); CHECK(converted->next_id == 8); REQUIRE(converted->records.size() == 7);
	// Compare each preserved scalar to its original row, including loss
	// sentinels. There is no expected-data round trip through the converter.
	for (const auto &row : original.orders) {
		const auto *record = FindSurfaceSupplyOrderRecord(*converted, row.id); REQUIRE(record != nullptr);
		CHECK(record->id == row.id); CHECK(record->owner.base() == row.owner); CHECK(record->origin.base() == row.station);
		CHECK(record->label == row.label); CHECK(record->quantity == row.quantity); CHECK(record->purchase_day.base() == row.economy_day);
		CHECK(record->Native() == nullptr); const auto *legacy = record->Legacy(); REQUIRE(legacy != nullptr);
		CHECK(legacy->unit_price == Money{row.unit_price}); CHECK(legacy->calendar_date == row.calendar_date);
		CHECK(legacy->refunded == row.refunded); CHECK(legacy->dispatched == row.dispatched); CHECK(legacy->closed == (row.closed != 0));
	}
	CHECK(FindSurfaceSupplyOrderRecord(*converted, 8) == nullptr);
	CHECK(CountSurfaceSupplyDayUsage(*converted, CompanyID{0}, CargoLabel{"RFMT"}, TimerGameEconomy::Date{100}) == 500);
	CHECK(CountSurfaceSupplyDayUsage(*converted, CompanyID{2}, CargoLabel{"RFMT"}, TimerGameEconomy::Date{100}) == 100);
	CHECK(converted->records[2].Legacy()->unit_price * 60 == Money{1500}); // Original already-refunded price remains25, never today's180.
	CHECK(original == before); witness.CheckUnchanged();
}

/** Missing or contradictory original structure refuses without partial candidates or repairs. */
TEST_CASE("Legacy B conversion refuses malformed headers dense identities and raw closure", "[surface-supply-conversion]")
{
	SurfaceSupplyRecordRig rig; TableSupplyTransport original{1, 2, 3, {OriginalLegacyRow(1), OriginalLegacyRow(2)}};
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
	switch (fault) {
		case 0: original.schema = 0; break;
		case 1: original.schema = 2; break;
		case 2: original.count = 1; break;
		case 3: original.count = 65536; break;
		case 4: original.next_order = 2; break;
		case 5: original.next_order = 4; break;
		case 6: original.orders[0].id = 0; break;
		case 7: original.orders[1].id = 1; break;
		case 8: original.orders[1].id = 3; break;
		case 9: std::swap(original.orders[0], original.orders[1]); break;
		case 10: original.orders[1].closed = 2; break;
		case 11: original.orders[1] = {}; break;
	}
	const auto before = original; const ConversionWitness witness;
	CHECK_FALSE(ConvertTableSupply(original, rig.context)); CHECK(original == before); witness.CheckUnchanged();
}

/** Candidate-owned references, not a live world or convenient replacement source, decide admission. */
TEST_CASE("Legacy B conversion refuses missing foreign or contradictory same-file evidence", "[surface-supply-conversion]")
{
	SurfaceSupplyRecordRig rig; TableSupplyTransport original{1, 1, 2, {OriginalLegacyRow(1)}};
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
	switch (fault) {
		case 0: rig.context.stations = {}; break;
		case 1: rig.context.companies[0] = false; break;
		case 2: rig.stations[0].owner = CompanyID{1}; break;
		case 3: rig.stations[0].layer = MapLayer::Underground; break;
		case 4: original.orders[0].station = UINT16_MAX; break;
		case 5: original.orders[0].owner = UINT8_MAX; original.orders[0].closed = 1; break;
		case 6: original.orders[0].refunded = 1; break;
		case 7: original.orders[0].refunded = 20; original.orders[0].dispatched = 81; original.orders[0].closed = 1; break;
		case 8: original.orders[0].unit_price = INT64_MAX / 100 + 1; break;
		case 9: rig.context.profile = MultiLayerGameplayProfileID::Classic; break;
	}
	const auto before = original; const ConversionWitness witness;
	CHECK_FALSE(ConvertTableSupply(original, rig.context)); CHECK(original == before); witness.CheckUnchanged();
}

/** All65535 original rows and the never-issued65536 frontier survive; absent transport is not empty history. */
TEST_CASE("Legacy B conversion keeps the full original bound and admits only explicit empty history", "[surface-supply-conversion]")
{
	SurfaceSupplyRecordRig rig; const ConversionWitness witness;
	CHECK_FALSE(ConvertTableSupply({}, rig.context));
	const auto empty = ConvertTableSupply({1, 0, 1, {}}, rig.context);
	REQUIRE(empty.has_value()); CHECK(empty->records.empty()); CHECK(empty->next_id == 1);
	TableSupplyTransport original{1, 65535, 65536, {}}; original.orders.reserve(65535);
	// Exercise the real maximum historical cardinality, not a reduced
	// substitute that could conceal uint16 truncation or id-minus-one lookup.
	for (uint32_t id = 1; id <= 65535; ++id) original.orders.push_back(OriginalLegacyRow(id));
	const auto converted = ConvertTableSupply(original, rig.context);
	REQUIRE(converted.has_value()); CHECK(converted->next_id == 65536); REQUIRE(converted->records.size() == 65535);
	CHECK(FindSurfaceSupplyOrderRecord(*converted, 65535)->Legacy()->unit_price == 25);
	CHECK(CountSurfaceSupplyDayUsage(*converted, CompanyID{0}, CargoLabel{"RFMT"}, TimerGameEconomy::Date{100}) == 6553500);
	witness.CheckUnchanged();
}

/** A440 projection keeps sparse uint64 intent identity, native price and terminal issuance history. */
TEST_CASE("Legacy A conversion preserves native Pending and fulfilled receipts exactly", "[surface-supply-conversion]")
{
	SurfaceSupplyRecordRig rig; const auto high = uint64_t{1} << 40;
	SurfaceSupplyOrderTransport original{{high + 1, {rig.Record(1), rig.Record(high, StationID{3}, CompanyID{0}, CargoLabel{"RFMT"}, 2, SurfaceSupplyOrderState::Fulfilled)}}, TimerGameEconomy::Date{100}};
	const auto before = original.snapshot; const ConversionWitness witness;
	const auto converted = ConvertFixedSupply(original, rig.context);
	REQUIRE(converted.has_value()); CHECK(*converted == before); CHECK(original.snapshot == before);
	CHECK(original.saved_day == TimerGameEconomy::Date{100}); CHECK(converted->records[0].Native()->state == SurfaceSupplyOrderState::Pending);
	CHECK(converted->records[1].Native()->issued_packet == CargoPacketID{17}); CHECK(converted->records[1].Legacy() == nullptr);
	CHECK(FindSurfaceSupplyOrderRecord(*converted, high) == &converted->records[1]);
	witness.CheckUnchanged();
}

/** Old A projection cannot launder legacy provenance or weaken its original native validation. */
TEST_CASE("Legacy A conversion refuses nonnative and invalid native purchase evidence", "[surface-supply-conversion]")
{
	SurfaceSupplyRecordRig rig; SurfaceSupplyOrderTransport original{{2, {rig.Record(1)}}, TimerGameEconomy::Date{100}};
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5);
	switch (fault) {
		case 0: original.snapshot.records[0].receipt = TableSupplyReceipt{}; break;
		case 1: original.snapshot.records[0].receipt = std::monostate{}; break;
		case 2: original.snapshot.records[0].Native()->cost += Money{1}; break;
		case 3: original.snapshot.records[0].Native()->origin_x += 1; break;
		case 4: rig.context.companies[0] = false; break;
		case 5: original.snapshot.records.resize(MAX_SURFACE_SUPPLY_RECORDS + 1); break;
	}
	const auto before = original.snapshot; const ConversionWitness witness;
	CHECK_FALSE(ConvertFixedSupply(original, rig.context)); CHECK(original.snapshot == before); witness.CheckUnchanged();
}

/** Only original invalid-Industry linked facts project to non-paying purpose; no active-world buyer or same-label inference is permitted. */
TEST_CASE("Legacy B packet projection preserves original paid evidence and ordinary cargo", "[surface-supply-conversion]")
{
	SurfaceSupplyRecordRig rig;
	const auto snapshot = ConvertTableSupply({1, 1, 2, {OriginalLegacyRow(1)}}, rig.context); REQUIRE(snapshot.has_value());
	const SurfaceSupplyPacketFacts raw{CargoPacketID{17}, 40, {Source::Invalid, SourceType::Industry}, Money{0}, 1, true};
	const ConversionWitness witness;
	const auto projected = ProjectTableSupplyPacket(raw, *snapshot); REQUIRE(projected.has_value());
	CHECK(projected->source == (Source{Source::Invalid, SourceType::CompanySupply}));
	CHECK(projected->order == 1); CHECK(projected->shipped); CHECK(projected->quantity == 40); CHECK(projected->feeder_share == Money{0});
	auto ordinary = raw; ordinary.order = 0; ordinary.shipped = false;
	CHECK(ProjectTableSupplyPacket(ordinary, *snapshot) == ordinary);
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5);
	auto malformed = raw;
	switch (fault) {
		case 0: malformed.source.id = 0; break; // Today's owner is not the unavailable historical buyer.
		case 1: malformed.source.type = SourceType::Town; break; // Matching material cannot change ordinary provenance.
		case 2: malformed.source.type = SourceType::CompanySupply; break; // Original B did not define A's tag.
		case 3: malformed.feeder_share = Money{1}; break; // Paid packets cannot acquire transfer income.
		case 4: malformed.order = 2; break; // The exact unissued identity supplies no receipt.
		case 5: malformed.order = 0; break; // Shipped-without-link cannot become ordinary or bought cargo.
	}
	CHECK_FALSE(ProjectTableSupplyPacket(malformed, *snapshot));
	witness.CheckUnchanged();
}
