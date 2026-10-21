/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_productive_capital.h Pure complete cost of the remaining paid first-delivery observation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGC.
 * Existing native offers, trucks, trains and financial owners remain unchanged.
 * A whole cost bound never turns a possible fare into present capitalization.
 */
#ifndef TEST_SURFACE_ENTRY_PRODUCTIVE_CAPITAL_H
#define TEST_SURFACE_ENTRY_PRODUCTIVE_CAPITAL_H

/** Price all remaining proposed work on current paid assets, without weakening any existing guard. */
static void QuerySurfaceProductiveCapital()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{84595});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	const TileIndex receiver{731}; REQUIRE(IsStationRoadStopTile(receiver)); REQUIRE(GetStationIndex(receiver) == StationID{20});
	REQUIRE(GetRoadStopType(receiver) == RoadStopType::Truck);
	for (const auto id : {VehicleID{3}, VehicleID{25}}) VerifySurfaceGoodsTradeOrders(*RoadVehicle::Get(id));
	const auto component = SurfaceGoodsRoadComponent(TileIndex{2443});
	REQUIRE(component.contains(Station::Get(StationID{19})->xy)); REQUIRE(component.contains(receiver));
	const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	fmt::print("SURFACE_RESUPPLY_PRODUCTIVE_SOURCE waiting={} receiver=20 receiver_tile=731 connected=true trucks=2 goods_head=49 goods_capacity=300 purchases=0 changed_orders=0\n",
		Station::Get(StationID{19})->goods[goods].AvailableCount());
	// The existing short guard proves current fixed-price callback/queue
	// prerequisites. The longer shared quote independently protects every
	// useful body's life and every crossed native month/year for the whole bill.
	QuoteSurfaceFixedPriceIncome(3000);
	const auto whole = QuoteSurfaceModuleCapitalWork(0, 6000);
	const auto operating = whole.upkeep + whole.interest + whole.station;
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto target = reserve + operating;
		fmt::print("SURFACE_RESUPPLY_PRODUCTIVE_ADMISSION ticks=6000 intervals=2 interval_limit=3000 days={} months={} year_boundary={} upkeep={} interest={} station_charge={} operating={} reserve={} target={} cash={} funded={} purchases=0 future_income_counted=0 refunds_counted=0 credit=0 working_shortfall=15405 module_base_shortfall=425405 module_capital_complete=false\n",
			whole.days, whole.months, whole.year_boundary, whole.upkeep.base(), whole.interest.base(), whole.station.base(), operating.base(), reserve.base(), target.base(), company->money.base(), company->money >= target);
	}
}
#endif /* TEST_SURFACE_ENTRY_PRODUCTIVE_CAPITAL_H */
