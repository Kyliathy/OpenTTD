/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_farm_cycle.h Actual retained farm-to-factory cycle and full cost admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGI.
 * Waiting products, recent profits and partial production histories are
 * observations. Only present cash admits the complete remaining work bill.
 */
#ifndef TEST_SURFACE_ENTRY_FARM_CYCLE_H
#define TEST_SURFACE_ENTRY_FARM_CYCLE_H

/** Publish actual active rail loads and cash books without creating any work.
 * @param boundary Current query, before-work or after-work diagnostic boundary.
 */
static void ObserveSurfaceFarmCycle(std::string_view boundary)
{
	VerifySurfaceFarmRebalance(true);
	// Each real service reports its whole linked load. Profit is native fixed
	// 256 accounting and stays separate from company interest and startup costs.
	for (const auto id : {VehicleID{42}, VehicleID{49}, VehicleID{52}}) {
		const auto *head = Train::Get(id); uint held = 0;
		for (const Train *part = head; part != nullptr; part = part->Next()) held += part->cargo.TotalCount();
		fmt::print("SURFACE_RESUPPLY_CYCLE_RAIL boundary={} head={} tile={} speed={} held={} last_station={} order={} load_ticks={} breakdown={} profit_current_256={} profit_previous_256={} remaining_days={}\n",
			boundary, id.base(), head->tile.base(), head->cur_speed, held, head->last_station_visited.base(), to_underlying(head->current_order.GetType()),
			head->load_unload_ticks, head->breakdown_ctr, head->profit_this_year.base(), head->profit_last_year.base(), head->max_age.base() - head->age.base());
	}
	// These are actual station-owned offers, never future fares or an admission
	// credit. The independent packet observer verifies exact saved quantities.
	for (const auto &[station, label] : std::array<std::pair<StationID, CargoLabel>, 4>{{{StationID{12}, CargoLabel{"GOOD"}},
		{StationID{15}, CargoLabel{"LVST"}}, {StationID{15}, CargoLabel{"GRAI"}}, {StationID{19}, CargoLabel{"GOOD"}}}}) {
		const auto cargo = GetCargoTypeByLabel(label); const auto *source = Station::Get(station);
		fmt::print("SURFACE_RESUPPLY_CYCLE_OFFER boundary={} station={} cargo={} waiting={} rating={}\n",
			boundary, station.base(), cargo, source->goods[cargo].AvailableCount(), source->goods[cargo].rating);
	}
}

/** Price the entire next two-interval native producer cycle before admitting any work. */
static void QuerySurfaceFarmCycle()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{72259});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	ObserveSurfaceFarmCycle("query"); QuoteSurfaceFixedPriceIncome(3000); const auto work = QuoteSurfaceModuleCapitalWork(0, 6000);
	const auto operating = work.upkeep + work.interest + work.station;
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto target = reserve + operating;
		fmt::print("SURFACE_RESUPPLY_CYCLE_ADMISSION ticks=6000 interval_limit=3000 days={} months={} year_boundary={} upkeep={} interest={} station_charge={} operating={} reserve={} target={} cash={} funded={} purchases=0 future_income_counted=0 refunds_counted=0 credit=0\n",
			work.days, work.months, work.year_boundary, work.upkeep.base(), work.interest.base(), work.station.base(), operating.base(), reserve.base(), target.base(), company->money.base(), company->money >= target);
	}
}

/** Earn one freshly priced actual interval, retaining genuine partial service/cash endpoints. */
static void WorkSurfaceFarmCycle()
{
	ObserveSurfaceFarmCycle("before"); WorkSurfaceFarmRebalance(); ObserveSurfaceFarmCycle("after");
}
#endif /* TEST_SURFACE_ENTRY_FARM_CYCLE_H */
