/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_capital_recovery.h Pure current freight and bounded working-reserve recovery admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CR/15CS.
 * Existing native consists, physical cargo, calendar costs and company cash own
 * the proposal. A future payment target never contributes to current funding.
 */
#ifndef TEST_SURFACE_ENTRY_CAPITAL_RECOVERY_H
#define TEST_SURFACE_ENTRY_CAPITAL_RECOVERY_H

/** Quote only one actual next bounded existing-service interval, with no purchase.
 * @param limit Freshly proposed native cap, at most3000ticks.
 * @return Conservative full cost ceiling, payable from current cash above the explicit40000recovery floor.
 */
static Money QuoteSurfaceCapitalRecovery(uint limit)
{
	REQUIRE(limit > 0); REQUIRE(limit <= 3000);
	const auto *company = Company::Get(_current_company); const auto work = QuoteSurfaceModuleCapitalWork(0, limit);
	REQUIRE(!work.year_boundary); const auto operating = work.upkeep + work.interest + work.station;
	const auto target = Money{40000} + operating; const bool funded = company->money >= target;
	fmt::print("SURFACE_RESUPPLY_RECOVERY_ADMISSION ticks={} days={} months={} upkeep={} interest={} station_charge={} operating={} reserve=40000 target={} cash={} funded={} purchases=0 future_income_counted=0 refunds_counted=0 credit=0 goal=100000 module_base=510000 module_reserve=150000 working_shortfall={} module_base_shortfall={}\n",
		limit, work.days, work.months, work.upkeep.base(), work.interest.base(), work.station.base(), operating.base(), target.base(), company->money.base(), funded,
		std::max(Money{0}, Money{100000} - company->money).base(), std::max(Money{0}, Money{510000} - company->money).base());
	REQUIRE(funded); REQUIRE(company->current_loan == Money{5000000}); return operating;
}

/** Characterize original paid consists and real waiting demand before admitting reserve recovery. */
static void QuerySurfaceCapitalRecovery()
{
	uint bodies = 0, heads = 0, wagons = 0;
	// Account for every actually owned rail body before treating this existing
	// service as a funded recovery route; no spare or vanished wagon is assumed.
	for (const auto *vehicle : Train::Iterate()) {
		REQUIRE(vehicle->owner == _current_company); ++bodies;
		if (!vehicle->IsFrontEngine()) { ++wagons; continue; }
		++heads; REQUIRE(vehicle->name.starts_with("Surface Income "));
		CargoArray capacity{}, held{}; uint parts = 0;
		// Sum actual linked native bodies; no spare wagon, inherited aggregate
		// capacity or station production history can become available rolling stock.
		for (const Train *part = vehicle; part != nullptr; part = part->Next()) {
			capacity[part->cargo_type] += part->cargo_cap; held[part->cargo_type] += part->cargo.TotalCount(); ++parts;
		}
		const auto depot = vehicle->name == "Surface Income goods" ? TileIndex{3224} : vehicle->name == "Surface Income coal" ? TileIndex{3161} : TileIndex{1638};
		REQUIRE(vehicle->GetNumOrders() == 2);
		for (uint8_t index = 0; index < 2; ++index) {
			const auto *order = vehicle->GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_STATION));
			const auto station = StationID{order->GetDestination().base()}; const auto *stop = Station::Get(station);
			const bool connected = SurfaceResupplyRailConnected(depot, stop->xy, vehicle->compatible_railtypes);
			fmt::print("SURFACE_RESUPPLY_RECOVERY_ROUTE vehicle={} order={} station={} load={} unload={} depot={} connected={} remaining={} speed={} parts={}\n",
				vehicle->index.base(), index, station.base(), to_underlying(order->GetLoadType()), to_underlying(order->GetUnloadType()), depot.base(), connected,
				vehicle->max_age.base() - vehicle->age.base(), vehicle->cur_speed, parts);
		}
		for (CargoType cargo = CargoType{0}; cargo < NUM_CARGO; ++cargo) {
			if (capacity[cargo] == 0) continue;
			fmt::print("SURFACE_RESUPPLY_RECOVERY_CONSIST vehicle={} label={} capacity={} held={} parts={}\n", vehicle->index.base(), CargoSpec::Get(cargo)->label.AsString(), capacity[cargo], held[cargo], parts);
		}
	}
	REQUIRE(bodies == 35); REQUIRE(heads == 3); REQUIRE(wagons == 32);
	// Observe actual current cargo queues at existing paid stations. Packet
	// ownership remains native; demand may justify work but cannot pay its quote.
	for (const auto id : {StationID{11}, StationID{12}, StationID{13}, StationID{14}, StationID{15}, StationID{16}, StationID{17}, StationID{18}, StationID{20}}) {
		const auto *station = Station::Get(id); REQUIRE(station->owner == _current_company);
		for (CargoType cargo = CargoType{0}; cargo < NUM_CARGO; ++cargo) {
			const auto waiting = station->goods[cargo].AvailableCount(); if (waiting == 0) continue;
			fmt::print("SURFACE_RESUPPLY_RECOVERY_DEMAND station={} label={} waiting={}\n", id.base(), CargoSpec::Get(cargo)->label.AsString(), waiting);
		}
	}
	QuoteSurfaceCapitalRecovery(3000);
	fmt::print("SURFACE_RESUPPLY_RECOVERY_PROPOSAL heads=3 wagons=32 bodies=35 added_vehicles=0 changed_orders=0 purchase_cost=0 goal=100000 guaranteed_income=false module_capital_complete=false\n");
}

/** Earn one freshly funded actual interval and stop only on native cash or its explicit at-most3000tick cap.
 * Future freight fares are not included in the current cost ceiling; the
 * actual endpoint remains resumable if the working reserve is not restored.
 */
static void WorkSurfaceCapitalRecovery()
{
	const auto limit = SurfaceChemicalWorkLimit(); const auto *company = Company::Get(_current_company);
	REQUIRE(company->money < Money{100000}); QuoteSurfaceCapitalRecovery(limit);
	/** @return Whether genuine native payments restored the working reserve; ongoing costs retain their protected floor. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, [&] {
		REQUIRE(company->money >= Money{40000}); return company->money >= Money{100000};
	});
	REQUIRE(receipt.ticks > 0); REQUIRE(receipt.ticks <= limit); REQUIRE(company->current_loan == Money{5000000});
	fmt::print("SURFACE_RESUPPLY_RECOVERY_PROGRESS ticks={} cash={} debt=5000000 goal=100000 restored={} partial={} working_shortfall={} reserve=40000 purchases=0 credit=0 module_base_shortfall={} module_capital_complete=false\n",
		receipt.ticks, company->money.base(), receipt.reached, !receipt.reached, std::max(Money{0}, Money{100000} - company->money).base(),
		std::max(Money{0}, Money{510000} - company->money).base());
}
#endif /* TEST_SURFACE_ENTRY_CAPITAL_RECOVERY_H */
