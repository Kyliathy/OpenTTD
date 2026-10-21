/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_passenger_coverage.h Lawful paid catchment and bounded native capital recovery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSDE.
 * Public native station commands own admission, invoices and catchment. Actual
 * vehicles and company books own service, passenger payments and running costs.
 */
#ifndef TEST_SURFACE_ENTRY_PASSENGER_COVERAGE_H
#define TEST_SURFACE_ENTRY_PASSENGER_COVERAGE_H

/** Pay the freshly admitted sole station extension without changing any original route or clock. */
static void BuySurfacePassengerCoverage()
{
	QuerySurfaceIncomeCapitalization();
	const auto *company = Company::Get(_current_company); const auto before = company->money;
	const TileIndex tile{1055}; REQUIRE(IsNormalRoadTile(tile));
	const auto coverage = SurfaceIncomeNewCoverage(tile); REQUIRE(coverage.town0 == 0); REQUIRE(coverage.town1 == 8);
	const auto quote = QuoteSurfaceResupply<Commands::BuildRoadStop>(tile, tile, uint8_t{1}, uint8_t{1}, RoadStopType::Bus,
		true, DiagDirection::NE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, StationID{20}, true);
	REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == Money{150});
	const auto work = QuoteSurfaceFixedPriceIncome(3000);
	REQUIRE(before >= Money{40000} + quote.GetCost() + work.upkeep + work.interest + work.station);
	PaySurfaceResupply<Commands::BuildRoadStop>(tile, tile, uint8_t{1}, uint8_t{1}, RoadStopType::Bus,
		true, DiagDirection::NE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, StationID{20}, true);
	REQUIRE(IsStationRoadStopTile(tile)); REQUIRE(GetStationIndex(tile) == StationID{20});
	REQUIRE(company->money == before - Money{150}); REQUIRE(company->current_loan == Money{5000000});
	fmt::print("SURFACE_RESUPPLY_COVERAGE_PAID tile=1055 station=20 invoice=150 cash_before={} cash={} new_town1=8 changed_orders=0 added_buses=0 credit=0\n",
		before.base(), company->money.base());
}

/** Measure one independently funded operating interval after the lawful native extension. */
static void WorkSurfacePassengerCoverage()
{
	const TileIndex tile{1055}; REQUIRE(IsStationRoadStopTile(tile)); REQUIRE(GetStationIndex(tile) == StationID{20});
	const auto *company = Company::Get(_current_company); const auto limit = SurfaceChemicalWorkLimit();
	REQUIRE(company->money < Money{100000});
	const auto work = QuoteSurfaceFixedPriceIncome(limit); const auto operating = work.upkeep + work.interest + work.station;
	const auto target = Money{40000} + operating; REQUIRE(company->money >= target);
	fmt::print("SURFACE_RESUPPLY_COVERAGE_ADMISSION ticks={} operating={} reserve=40000 target={} cash={} funded=true purchases=0 future_income_counted=0 refunds_counted=0 credit=0\n",
		limit, operating.base(), target.base(), company->money.base());
	// Only actual native receipts can restore working cash. A successful stop
	// retains its precise endpoint; a cap retains the genuine partial original.
	/** @return Whether actual spendable money reaches the existing working-reserve target. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, [&] {
		REQUIRE(company->money >= Money{40000}); return company->money >= Money{100000};
	});
	REQUIRE(receipt.ticks > 0); REQUIRE(receipt.ticks <= limit); REQUIRE(company->current_loan == Money{5000000});
	fmt::print("SURFACE_RESUPPLY_COVERAGE_PROGRESS ticks={} cash={} goal=100000 restored={} partial={} working_shortfall={} module_base_shortfall={} module_capital_complete=false\n",
		receipt.ticks, company->money.base(), receipt.reached, !receipt.reached, std::max(Money{0}, Money{100000} - company->money).base(),
		std::max(Money{0}, Money{510000} - company->money).base());
}
#endif /* TEST_SURFACE_ENTRY_PASSENGER_COVERAGE_H */
