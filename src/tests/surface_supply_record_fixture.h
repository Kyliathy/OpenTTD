/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_record_fixture.h Detached candidate facts shared by receipt and conversion tests.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native owner invariants.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Groups10-11.
 */
#ifndef TESTS_SURFACE_SUPPLY_RECORD_FIXTURE_H
#define TESTS_SURFACE_SUPPLY_RECORD_FIXTURE_H
#include "../surface_supply_order.h"
#include "../3rdparty/catch2/catch.hpp"

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
		record.id = id; record.Native()->purchaser = owner; record.owner = owner; record.origin = origin;
		record.Native()->origin_x = station->x; record.Native()->origin_y = station->y; record.label = label; record.quantity = quantity;
		record.Native()->inflation_prices = uint64_t{1} << 16;
		const auto price = CalculateSurfaceSupplyCost(label, quantity, record.Native()->inflation_prices); REQUIRE(price.has_value()); record.Native()->cost = *price;
		record.Native()->purchase_year = TimerGameCalendar::Year{1980}; record.purchase_day = day; record.Native()->packet_buyer = owner.base(); record.Native()->state = state;
		record.Native()->issued_packet = state == SurfaceSupplyOrderState::Fulfilled ? CargoPacketID{17} : CargoPacketID::Invalid();
		return record;
	}
};

#endif /* TESTS_SURFACE_SUPPLY_RECORD_FIXTURE_H */
