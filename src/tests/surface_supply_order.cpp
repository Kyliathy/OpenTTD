/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order.cpp Detached paid-order shape, canonical pricing and shared day-allowance boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB.
 * Every record/reference below is declared component metadata, not a paid native order or usable cargo.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../surface_supply_order.h"
#include "../multilayer_bootstrap.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/** Detached candidate facts; no native company/station/packet is created by this component fixture. */
struct SurfaceSupplyRecordRig {
	std::array<SurfaceSupplyStationView, 3> stations{{
		{StationID{3}, CompanyID{0}, 20, 20, MapLayer::Surface, true},
		{StationID{7}, CompanyID{0}, 22, 20, MapLayer::Surface, true},
		{StationID{9}, CompanyID{1}, 24, 20, MapLayer::Surface, true},
	}}; ///< Original strictly ordered candidate-only station facts.
	SurfaceSupplyRecordContext context; ///< Sole detached reference context, never a view of the running world.
	SurfaceSupplyOrderSnapshot snapshot; ///< Component declarations only; validation cannot activate them.

	/** Declare one isolated candidate profile/map/company/station context. */
	SurfaceSupplyRecordRig()
	{
		this->context.profile = MultiLayerGameplayProfileID::MultilayerIndustry;
		this->context.width = 64; this->context.height = 64;
		this->context.companies[0] = true; this->context.companies[1] = true;
		this->context.stations = this->stations;
	}

	/** Declare one whole receipt through the shared canonical price formula, never a purchase.
	 * @param id Original test intent identity.
	 * @param origin One explicitly declared candidate station.
	 * @param owner Original/current declared payer for this initial record.
	 * @param label Canonical construction meaning.
	 * @param quantity Entire declared original order.
	 * @param state Pending or terminal declaration; no stock is materialized.
	 * @param day Original economy day, independent of calendar introduction.
	 * @return Exact component record with immutable full-price inputs.
	 */
	SurfaceSupplyOrderRecord Record(SurfaceSupplyOrderID id, StationID origin = StationID{3}, CompanyID owner = CompanyID{0}, CargoLabel label = CargoLabel{"STEL"},
		uint16_t quantity = 2, SurfaceSupplyOrderState state = SurfaceSupplyOrderState::Pending, TimerGameEconomy::Date day = TimerGameEconomy::Date{100}) const
	{
		const auto station = std::ranges::find(this->stations, origin, &SurfaceSupplyStationView::station);
		REQUIRE(station != this->stations.end());
		SurfaceSupplyOrderRecord record;
		record.id = id; record.purchaser = owner; record.owner = owner; record.origin = origin;
		record.origin_x = station->x; record.origin_y = station->y; record.label = label; record.quantity = quantity;
		record.inflation_prices = uint64_t{1} << 16;
		const auto price = CalculateSurfaceSupplyCost(label, quantity, record.inflation_prices); REQUIRE(price.has_value()); record.cost = *price;
		record.purchase_year = TimerGameCalendar::Year{1980}; record.purchase_day = day; record.packet_buyer = owner.base(); record.state = state;
		record.issued_packet = state == SurfaceSupplyOrderState::Fulfilled ? CargoPacketID{17} : CargoPacketID::Invalid();
		return record;
	}
};

/** One original catalogue/checked formula prices both admission and frozen metadata without another label/price registry. */
TEST_CASE("Surface order metadata shares exactly the scheduled canonical prices", "[surface-supply-state]")
{
	const auto label = GENERATE(CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"});
	const auto *definition = GetSurfaceSupplyDefinition(label); REQUIRE(definition != nullptr);
	const auto all = GetMultiLayerSurfaceSupplies();
	CHECK(definition == &*std::ranges::find(all, label, &MultiLayerSurfaceSupply::label));
	CHECK(CalculateSurfaceSupplyCost(label, 2, uint64_t{1} << 16) == Money{definition->price_per_unit} * 2);
	CHECK(CalculateSurfaceSupplyCost(label, 2, uint64_t{3} << 15) == Money{definition->price_per_unit} * 3);
	CHECK_FALSE(CalculateSurfaceSupplyCost(label, 0, uint64_t{1} << 16));
	CHECK_FALSE(CalculateSurfaceSupplyCost(label, 2, 0)); CHECK_FALSE(CalculateSurfaceSupplyCost(label, 2, UINT64_MAX));
	CHECK(GetSurfaceSupplyDefinition(CargoLabel{"PROP"}) == nullptr); CHECK_FALSE(CalculateSurfaceSupplyCost(CargoLabel{"CHEM"}, 2, uint64_t{1} << 16));
}

/** Every origin contributes to the same owner/day/label allowance; detached inspection changes no input, native packet or RNG. */
TEST_CASE("Surface order day allowance combines pending and fulfilled across stations only once", "[surface-supply-state]")
{
	SurfaceSupplyRecordRig rig; rig.snapshot.next_id = 8;
	rig.snapshot.records = {
		rig.Record(1, StationID{3}, CompanyID{0}, CargoLabel{"STEL"}, 8),
		rig.Record(2, StationID{7}, CompanyID{0}, CargoLabel{"STEL"}, 4, SurfaceSupplyOrderState::Fulfilled),
		rig.Record(3, StationID{3}, CompanyID{0}, CargoLabel{"STEL"}, 2, SurfaceSupplyOrderState::Cancelled),
		rig.Record(4, StationID{9}, CompanyID{1}, CargoLabel{"STEL"}, 3),
		rig.Record(5, StationID{3}, CompanyID{0}, CargoLabel{"RFMT"}, 4),
		rig.Record(7, StationID{3}, CompanyID{0}, CargoLabel{"STEL"}, 7, SurfaceSupplyOrderState::Fulfilled, TimerGameEconomy::Date{99}),
	};
	const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
	REQUIRE(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"STEL"}, TimerGameEconomy::Date{100}) == 12);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"STEL"}, TimerGameEconomy::Date{99}) == 7);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{1}, CargoLabel{"STEL"}, TimerGameEconomy::Date{100}) == 3);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"RFMT"}, TimerGameEconomy::Date{100}) == 4);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"STEL"}, TimerGameEconomy::Date{101}) == 0);
	CHECK_FALSE(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID::Invalid(), CargoLabel{"STEL"}, TimerGameEconomy::Date{100}));
	CHECK_FALSE(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"PROP"}, TimerGameEconomy::Date{100}));
	CHECK(rig.snapshot.next_id == 8); CHECK(rig.snapshot.records.size() == 6); CHECK(rig.snapshot.records[0].cost == 800);
	CHECK(rig.snapshot.records[2].state == SurfaceSupplyOrderState::Cancelled); CHECK(rig.stations[0].owner == CompanyID{0});
	CHECK(CargoPacket::GetNumItems() == packets); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Whole-record faults refuse before any metadata is installed; absent candidate facts cannot be filled by an active native world. */
TEST_CASE("Surface order metadata rejects malformed identity price state and candidate references", "[surface-supply-state]")
{
	SurfaceSupplyRecordRig rig; rig.snapshot.next_id = 2; rig.snapshot.records.push_back(rig.Record(1, StationID{3}, CompanyID{0}, CargoLabel{"ELEC"}));
	REQUIRE(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29);
	auto &record = rig.snapshot.records.front(); SurfaceSupplyRecordError expected = SurfaceSupplyRecordError::Identity;
	switch (fault) {
		case 0: rig.snapshot.next_id = 0; break;
		case 1: record.id = 0; break;
		case 2: record.id = 2; break;
		case 3: rig.snapshot.records.push_back(record); rig.snapshot.next_id = 3; break;
		case 4: rig.context.profile = MultiLayerGameplayProfileID::Classic; expected = SurfaceSupplyRecordError::Profile; break;
		case 5: rig.context.width = 0; expected = SurfaceSupplyRecordError::Geometry; break;
		case 6: record.purchaser = CompanyID::Invalid(); expected = SurfaceSupplyRecordError::Company; break;
		case 7: record.owner = OWNER_NONE; expected = SurfaceSupplyRecordError::Company; break;
		case 8: record.packet_buyer = 1; expected = SurfaceSupplyRecordError::Company; break;
		case 9: record.origin = StationID::Invalid(); expected = SurfaceSupplyRecordError::Geometry; break;
		case 10: record.origin_x = 64; expected = SurfaceSupplyRecordError::Geometry; break;
		case 11: record.label = CargoLabel{"PROP"}; expected = SurfaceSupplyRecordError::Cargo; break;
		case 12: record.quantity = 0; expected = SurfaceSupplyRecordError::Quantity; break;
		case 13: record.quantity = 65; expected = SurfaceSupplyRecordError::Quantity; break;
		case 14: record.purchase_year = TimerGameCalendar::Year{1964}; expected = SurfaceSupplyRecordError::Date; break;
		case 15: record.inflation_prices = 0; expected = SurfaceSupplyRecordError::Price; break;
		case 16: record.cost = 999; expected = SurfaceSupplyRecordError::Price; break;
		case 17: rig.context.companies[0] = false; expected = SurfaceSupplyRecordError::Company; break;
		case 18: rig.context.stations = {}; expected = SurfaceSupplyRecordError::Station; break;
		case 19: rig.stations[0].owner = CompanyID{1}; expected = SurfaceSupplyRecordError::Station; break;
		case 20: rig.stations[0].layer = MapLayer::Underground; expected = SurfaceSupplyRecordError::Station; break;
		case 21: rig.stations[0].freight_facility = false; expected = SurfaceSupplyRecordError::Station; break;
		case 22: ++rig.stations[0].x; expected = SurfaceSupplyRecordError::Station; break;
		case 23: rig.stations[1].station = rig.stations[0].station; expected = SurfaceSupplyRecordError::Station; break;
		case 24: record.issued_packet = CargoPacketID{0}; expected = SurfaceSupplyRecordError::Packet; break;
		case 25: record.state = static_cast<SurfaceSupplyOrderState>(255); expected = SurfaceSupplyRecordError::State; break;
		case 26: record.state = SurfaceSupplyOrderState::Fulfilled; expected = SurfaceSupplyRecordError::Packet; break;
		case 27: record.state = SurfaceSupplyOrderState::Cancelled; record.issued_packet = CargoPacketID{0}; expected = SurfaceSupplyRecordError::Packet; break;
		case 28: record.state = SurfaceSupplyOrderState::Lost; expected = SurfaceSupplyRecordError::State; break;
		case 29: record.inflation_prices = UINT64_MAX; expected = SurfaceSupplyRecordError::Price; break;
	}
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == expected);
}

/** Native consumption/merging/deletion retires physical indices without resurrecting shipment/refund rights; takeover preserves original payment provenance. */
TEST_CASE("Surface order terminal history and transferred pending rights remain distinct", "[surface-supply-state]")
{
	SurfaceSupplyRecordRig rig; rig.snapshot.next_id = 3;
	auto done = rig.Record(1, StationID{3}, CompanyID{0}, CargoLabel{"STEL"}, 2, SurfaceSupplyOrderState::Fulfilled);
	done.owner = CompanyID::Invalid(); done.packet_buyer = Source::Invalid;
	auto lost = rig.Record(2); lost.state = SurfaceSupplyOrderState::Lost; lost.owner = CompanyID::Invalid(); lost.packet_buyer = Source::Invalid;
	rig.snapshot.records = {done, lost}; rig.context.stations = {}; rig.context.companies.fill(false);
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"STEL"}, TimerGameEconomy::Date{100}) == 0);
	rig.snapshot.records = {rig.Record(1)}; rig.snapshot.records[0].owner = CompanyID{1}; rig.snapshot.records[0].packet_buyer = Source::Invalid;
	rig.stations[0].owner = CompanyID{1}; rig.context.stations = rig.stations; rig.context.companies[1] = true;
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	CHECK(rig.snapshot.records[0].purchaser == CompanyID{0}); CHECK(rig.snapshot.records[0].cost == 200);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{1}, CargoLabel{"STEL"}, TimerGameEconomy::Date{100}) == 2);
}

/** Metadata remains bounded, and conservative acquired allowance may exceed a cap without being clamped into apparent new budget. */
TEST_CASE("Surface order snapshot bounds preserve issuance and conservative cap exhaustion", "[surface-supply-state]")
{
	SurfaceSupplyRecordRig rig; auto record = rig.Record(1, StationID{3}, CompanyID{0}, CargoLabel{"STEL"}, 1, SurfaceSupplyOrderState::Cancelled);
	rig.snapshot.next_id = MAX_SURFACE_SUPPLY_RECORDS + 1; rig.snapshot.records.reserve(MAX_SURFACE_SUPPLY_RECORDS + 1);
	// Every retained cancelled intent is a distinct sealed original receipt; reaching the metadata bound cannot wrap/reuse its identity or allocate physical cargo.
	for (size_t index = 0; index < MAX_SURFACE_SUPPLY_RECORDS; ++index) { record.id = index + 1; rig.snapshot.records.push_back(record); }
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	record.id = rig.snapshot.next_id++; rig.snapshot.records.push_back(record);
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::Size);
	CHECK_FALSE(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"STEL"}, TimerGameEconomy::Date{100}));
	rig.snapshot.next_id = 3; rig.snapshot.records = {
		rig.Record(1, StationID{3}, CompanyID{0}, CargoLabel{"STEL"}, 256, SurfaceSupplyOrderState::Fulfilled),
		rig.Record(2, StationID{9}, CompanyID{1}, CargoLabel{"STEL"}, 256, SurfaceSupplyOrderState::Fulfilled),
	};
	rig.snapshot.records[1].owner = CompanyID{0};
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
	CHECK(CountSurfaceSupplyDayUsage(rig.snapshot, CompanyID{0}, CargoLabel{"STEL"}, TimerGameEconomy::Date{100}) == 512);
	rig.snapshot.records.clear(); rig.snapshot.next_id = 1; rig.context.profile = MultiLayerGameplayProfileID::Classic;
	CHECK(ValidateSurfaceSupplyRecords(rig.snapshot, rig.context) == SurfaceSupplyRecordError::None);
}
