/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_custody_policy.cpp Independent same-candidate custody, completeness and finite-entitlement cases.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md actual ownership boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-h-cargo-custody.md detached policy versus native publication.
 * All packet facts are original-row declarations; tests allocate no native stock or live purchase book.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "surface_supply_record_fixture.h"
#include "../surface_supply_order_wire.h"
#include "../timer/timer_game_calendar.h"
#include "../core/random_func.hpp"
#include <map>
#include "../safeguards.h"

/** Immutable candidate packet rows and an independently declared100-unit legacy purchase. */
struct SupplyCustodyPolicyRig : SurfaceSupplyRecordRig {
	std::map<CargoPacketID, SurfaceSupplyPacketFacts> packets; ///< Explicit original table, not native physical stock or a replacement owner registry.
	/** Declare60 never-shipped source units and40 previously shipped units at an original25-unit tariff. */
	SupplyCustodyPolicyRig()
	{
		LegacyBSupplyOrderWire row{1, 0, 3, CargoLabel{"RFMT"}, 100,
			TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1).base(), 100, 0, 40, 25, 0};
		const auto converted = ConvertLegacyBSupply({1, 1, 2, {row}}, this->context); REQUIRE(converted.has_value()); this->snapshot = *converted;
		this->packets.emplace(CargoPacketID{10}, SurfaceSupplyPacketFacts{CargoPacketID{10}, 60, {Source::Invalid, SourceType::CompanySupply}, Money{0}, 1, false});
		this->packets.emplace(CargoPacketID{20}, SurfaceSupplyPacketFacts{CargoPacketID{20}, 40, {Source::Invalid, SourceType::CompanySupply}, Money{0}, 1, true});
	}
	/** Resolve one original declared row without inspecting the active world's pools.
	 * @param id Original key requested by the checker.
	 * @return Exact original value, including deliberately contradictory negative-case data, or absence.
	 */
	std::optional<SurfaceSupplyPacketFacts> Lookup(CargoPacketID id) const
	{
		const auto row = this->packets.find(id); return row == this->packets.end() ? std::nullopt : std::optional{row->second};
	}
	/** @return One read-only lookup bound to this unchanged candidate for both observation passes. */
	SurfaceSupplyPacketLookup Reader() const
	{
		/** @param id Original candidate identity. @return Exact same-file facts or absence, never native fallback. */
		return [this](CargoPacketID id) { return this->Lookup(id); };
	}
};

/** Source and shipped observations can arrive in either traversal order; native historical issuance never becomes a live packet link. */
TEST_CASE("Streaming supply custody joins exact legacy fragments beside ordinary and unlinked native cargo", "[surface-supply-custody-policy]")
{
	SupplyCustodyPolicyRig rig; const auto high = uint64_t{1} << 40;
	rig.snapshot.records.push_back(rig.Record(high, StationID{3}, CompanyID{0}, CargoLabel{"STEL"}, 2, SurfaceSupplyOrderState::Fulfilled)); rig.snapshot.next_id = high + 1;
	rig.packets.emplace(CargoPacketID{30}, SurfaceSupplyPacketFacts{CargoPacketID{30}, 2, {CompanyID{0}, SourceType::CompanySupply}, Money{0}, 0, false});
	rig.packets.emplace(CargoPacketID{40}, SurfaceSupplyPacketFacts{CargoPacketID{40}, 5, {SourceID{0}, SourceType::Town}, Money{7}, 0, false});
	REQUIRE(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	const auto book = rig.snapshot; const auto rows = rig.packets; const auto pool = CargoPacket::GetNumItems(); const auto random = _random;
	SurfaceSupplyCustodyCheck check(rig.snapshot, rig.context, rig.Reader(), rig.packets.size());
	CHECK_FALSE(check.Valid()); const bool shipped_first = GENERATE(false, true);
	if (shipped_first) REQUIRE(check.AddOwned(CargoPacketID{20}, CargoLabel{"RFMT"}, StationID::Invalid()));
	REQUIRE(check.AddOwned(CargoPacketID{10}, CargoLabel{"RFMT"}, StationID{3}));
	if (!shipped_first) REQUIRE(check.AddOwned(CargoPacketID{20}, CargoLabel{"RFMT"}, StationID::Invalid()));
	REQUIRE(check.AddOwned(CargoPacketID{30}, CargoLabel{"STEL"}, StationID{3})); REQUIRE(check.AddOwned(CargoPacketID{40}, CargoLabel{"FOOD"}, StationID{9}));
	CHECK_FALSE(check.Valid());
	// Enumerate the complete original table only after actual owner evidence.
	// Its independent cardinality, rather than visited totals, proves completion.
	for (const auto &[id, facts] : rig.packets) { (void)facts; REQUIRE(check.ObserveAllocated(id)); }
	CHECK(check.Valid()); CHECK(rig.snapshot == book); CHECK(rig.packets == rows); CHECK(CargoPacket::GetNumItems() == pool);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Each source, identity, tag or quantity contradiction refuses without changing input history or packet facts. */
TEST_CASE("Streaming supply custody refuses contradictory purpose history and original source claims", "[surface-supply-custody-policy]")
{
	SupplyCustodyPolicyRig rig;
	CargoPacketID first{10}; CargoLabel first_label{"RFMT"}; StationID first_queue{3};
	CargoLabel second_label{"RFMT"}; StationID second_queue = StationID::Invalid();
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19);
	switch (fault) {
		case 0: first = CargoPacketID{99}; break;
		case 1: rig.packets.at(CargoPacketID{10}).packet = CargoPacketID{99}; break;
		case 2: rig.packets.at(CargoPacketID{10}).order = 77; break;
		case 3:
			rig.snapshot.records.push_back(rig.Record(2)); rig.snapshot.next_id = 3; rig.packets.at(CargoPacketID{10}).order = 2; break;
		case 4: first_label = CargoLabel{"STEL"}; break;
		case 5: rig.packets.at(CargoPacketID{10}).source = {Source::Invalid, SourceType::Industry}; break;
		case 6: rig.packets.at(CargoPacketID{10}).source.id = MAX_COMPANIES; break;
		case 7: rig.packets.at(CargoPacketID{10}).feeder_share = Money{1}; break;
		case 8: rig.packets.at(CargoPacketID{20}).order = 0; break;
		case 9: rig.packets.at(CargoPacketID{10}).source.type = static_cast<SourceType>(255); break;
		case 10: rig.packets.at(CargoPacketID{10}).quantity = 0; break;
		case 11: rig.packets.at(CargoPacketID{10}).quantity = 61; break;
		case 12: rig.packets.at(CargoPacketID{20}).quantity = 41; break;
		case 13: rig.packets.at(CargoPacketID{10}).quantity = 50; rig.packets.at(CargoPacketID{20}).quantity = 41; break;
		case 14: rig.packets.at(CargoPacketID{20}).shipped = false; break;
		case 15: first_queue = StationID{9}; break;
		case 16: rig.snapshot.records[0].Legacy()->closed = true; break;
		case 17: rig.snapshot.records[0].Legacy()->refunded = 101; break;
		case 18: rig.snapshot.records[0].receipt = std::monostate{}; break;
		case 19: rig.packets.at(CargoPacketID{10}).source.id = 0; break; // B never recorded this claimed original payer.
	}
	const auto book = rig.snapshot; const auto rows = rig.packets;
	SurfaceSupplyCustodyCheck check(rig.snapshot, rig.context, rig.Reader(), rig.packets.size());
	const bool first_ok = check.AddOwned(first, first_label, first_queue);
	const bool second_ok = check.AddOwned(CargoPacketID{20}, second_label, second_queue);
	CHECK_FALSE((first_ok && second_ok)); CHECK_FALSE(check.Valid());
	CHECK_FALSE(check.ObserveAllocated(CargoPacketID{10})); // A later favorable original row cannot repair a prior refusal.
	CHECK(rig.snapshot == book); CHECK(rig.packets == rows);
}

/** Completeness and unique ownership are independent of matching total quantities or an otherwise valid lookup. */
TEST_CASE("Streaming supply custody requires every allocated row once and refuses duplicate or late owners", "[surface-supply-custody-policy]")
{
	SupplyCustodyPolicyRig rig;
	SECTION("duplicate actual owner") {
		SurfaceSupplyCustodyCheck check(rig.snapshot, rig.context, rig.Reader(), 2);
		REQUIRE(check.AddOwned(CargoPacketID{10}, CargoLabel{"RFMT"}, StationID{3}));
		CHECK_FALSE(check.AddOwned(CargoPacketID{10}, CargoLabel{"RFMT"}, StationID{3})); CHECK_FALSE(check.Valid());
	}
	SECTION("incomplete and duplicate allocated enumeration") {
		SurfaceSupplyCustodyCheck check(rig.snapshot, rig.context, rig.Reader(), 2);
		REQUIRE(check.AddOwned(CargoPacketID{10}, CargoLabel{"RFMT"}, StationID{3})); REQUIRE(check.AddOwned(CargoPacketID{20}, CargoLabel{"RFMT"}, StationID::Invalid()));
		REQUIRE(check.ObserveAllocated(CargoPacketID{10})); CHECK_FALSE(check.Valid());
		CHECK_FALSE(check.ObserveAllocated(CargoPacketID{10})); CHECK_FALSE(check.ObserveAllocated(CargoPacketID{20})); CHECK_FALSE(check.Valid());
	}
	SECTION("missing independent original cardinality") {
		SurfaceSupplyCustodyCheck check(rig.snapshot, rig.context, rig.Reader(), 3);
		REQUIRE(check.AddOwned(CargoPacketID{10}, CargoLabel{"RFMT"}, StationID{3})); REQUIRE(check.AddOwned(CargoPacketID{20}, CargoLabel{"RFMT"}, StationID::Invalid()));
		REQUIRE(check.ObserveAllocated(CargoPacketID{10})); REQUIRE(check.ObserveAllocated(CargoPacketID{20})); CHECK_FALSE(check.Valid());
	}
	SECTION("paid orphan and forbidden late repair") {
		SurfaceSupplyCustodyCheck check(rig.snapshot, rig.context, rig.Reader(), 2);
		CHECK_FALSE(check.ObserveAllocated(CargoPacketID{10})); CHECK_FALSE(check.AddOwned(CargoPacketID{10}, CargoLabel{"RFMT"}, StationID{3})); CHECK_FALSE(check.Valid());
	}
	SECTION("unsupported original lookup or cardinality") {
		CHECK_FALSE(SurfaceSupplyCustodyCheck(rig.snapshot, rig.context, {}, 2).Valid());
		CHECK_FALSE(SurfaceSupplyCustodyCheck(rig.snapshot, rig.context, rig.Reader(), static_cast<size_t>(CargoPacketID::End().base()) + 1).Valid());
	}
}

/** Loss retains real queue stock with no refund owner; it cannot disguise never-shipped cargo in a carrier or job. */
TEST_CASE("Streaming supply custody preserves closed owner-loss queues without inventing rights", "[surface-supply-custody-policy]")
{
	SupplyCustodyPolicyRig rig; auto &record = rig.snapshot.records[0];
	record.owner = CompanyID::Invalid(); record.origin = StationID::Invalid(); record.Legacy()->closed = true;
	REQUIRE(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	const auto location = GENERATE(0, 1, 2, 3);
	rig.stations[1].owner = OWNER_NONE;
	if (location == 2) rig.stations[1].layer = MapLayer::Underground;
	const bool physical_queue = location == 1;
	SurfaceSupplyCustodyCheck check(rig.snapshot, rig.context, rig.Reader(), 2);
	const StationID queue = location == 0 ? StationID::Invalid() : location == 3 ? StationID{99} : StationID{7};
	CHECK(check.AddOwned(CargoPacketID{10}, CargoLabel{"RFMT"}, queue) == physical_queue);
	if (physical_queue) {
		REQUIRE(check.AddOwned(CargoPacketID{20}, CargoLabel{"RFMT"}, StationID::Invalid()));
		REQUIRE(check.ObserveAllocated(CargoPacketID{10})); REQUIRE(check.ObserveAllocated(CargoPacketID{20})); CHECK(check.Valid());
	} else {
		CHECK_FALSE(check.Valid());
	}
	CHECK(record.owner == CompanyID::Invalid()); CHECK(record.origin == StationID::Invalid()); CHECK(record.Legacy()->closed);
}

/** The paid policy does not invent an owner for ordinary orphans; unlinked A purchased material still requires physical custody. */
TEST_CASE("Streaming supply custody keeps ordinary policy separate from paid orphan refusal", "[surface-supply-custody-policy]")
{
	SurfaceSupplyOrderSnapshot empty;
	SurfaceSupplyRecordRig references;
	const bool paid = GENERATE(false, true);
	const SurfaceSupplyPacketFacts row{CargoPacketID{4}, 2, {Source::Invalid, paid ? SourceType::CompanySupply : SourceType::Industry}, paid ? Money{0} : Money{-7}, 0, false};
	/** @param id Original row identity. @return The same immutable candidate row for both observation phases. */
	const SurfaceSupplyPacketLookup lookup = [&](CargoPacketID id) { return id == row.packet ? std::optional{row} : std::nullopt; };
	SurfaceSupplyCustodyCheck check(empty, references.context, lookup, 1);
	CHECK(check.ObserveAllocated(row.packet) == !paid); CHECK(check.Valid() == !paid);
	SurfaceSupplyCustodyCheck owned(empty, references.context, lookup, 1);
	REQUIRE(owned.AddOwned(row.packet, CargoLabel{"STEL"}, StationID{3})); REQUIRE(owned.ObserveAllocated(row.packet)); CHECK(owned.Valid());
	CHECK_FALSE(owned.AddOwned(row.packet, CargoLabel{"STEL"}, StationID{3})); CHECK_FALSE(owned.Valid());
}
