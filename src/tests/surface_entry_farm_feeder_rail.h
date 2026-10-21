/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_farm_feeder_rail.h Read-only actual feeder-cohort rail pickup/final-delivery observation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGS.
 * Original52and native packet split/merge/payment owners carry the real
 * first22/Industry7cargo. No packet, route, position or income is supplied.
 */
#ifndef TEST_SURFACE_ENTRY_FARM_FEEDER_RAIL_H
#define TEST_SURFACE_ENTRY_FARM_FEEDER_RAIL_H

/** Read actual new-pickup cargo and preserved virtual feeder share on the original farm train.
 * @param train Exact original head52whose native consist/order contract already passed.
 * @return Whole first22/Industry7Livestock quantity and its actual fixed-money feeder share.
 */
static std::pair<uint, Money> SurfaceFarmFeederRailStock(const Train *train)
{
	uint quantity = 0; Money share{0}; const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"});
	// Native loading may split/merge packets or distribute them across real
	// wagons. Production identity and first station identify this cohort;
	// recyclable packet IDs cannot become a second shipment authority.
	for (auto *part = train; part != nullptr; part = part->Next()) {
		for (const auto *packet : *part->cargo.Packets()) {
			if (packet->GetFirstStation() != StationID{22}) continue;
			REQUIRE(part->cargo_type == livestock); REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == 7);
			quantity += packet->Count(); share += packet->GetFeederShare();
		}
	}
	return {quantity, share};
}

/** Earn one freshly funded2000tick original rail followthrough of the real transferred cohort. */
static void WorkSurfaceFarmFeederRail()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{58044}); REQUIRE(company->current_loan == Money{5000000});
	const auto *train = VerifySurfaceFarmRebalance(true); VerifySurfaceFarmFeederOrders(); VerifySurfaceIntertown();
	const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"}); const auto *source = Station::Get(StationID{15}), *receiver = Station::Get(StationID{13});
	REQUIRE(receiver->goods[livestock].status.Test(GoodsEntry::State::Acceptance)); uint waiting = 0; Money initial_share{0};
	// Confirm the literal receiving owner before admitting any further work.
	// Unpicked cargo still at22is not part of this transferred nine-unit fact.
	for (const auto &[hop, packets] : *source->goods[livestock].GetData().cargo.Packets()) for (const auto *packet : packets) {
		if (packet->GetFirstStation() != StationID{22}) continue;
		REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == 7);
		waiting += packet->Count(); initial_share += packet->GetFeederShare();
	}
	REQUIRE(waiting == 9); REQUIRE(initial_share == Money{3}); REQUIRE(SurfaceFarmFeederRailStock(train).first == 0);
	QuoteSurfaceFarmRoadWork("feeder-rail", Money{0}, false);
	fmt::print("SURFACE_RESUPPLY_FARM_FEEDER_RAIL_SOURCE vehicle=52 pickup=15 first_station=22 industry=7 receiver=13 waiting=9 feeder_share=3 initial_aboard=0 paid=0 changed_orders=0\n");
	uint previous = 0, boarded = 0, delivered = 0, maximum = 0; Money maximum_share{0};
	/** @return Whether actual source22cargo has finally unloaded under the original accepting13policy. */
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", 2000, [&] {
		REQUIRE(company->money >= Money{40000}); const auto [current, share] = SurfaceFarmFeederRailStock(train);
		if (current > previous) boarded += current - previous;
		// The exact unchanged consist can lose these real units only at its
		// original final receiver. NoLoad/UnloadIfPossible plus actual native
		// acceptance distinguishes payment from another virtual transfer.
		if (current < previous) {
			REQUIRE(train->last_station_visited == StationID{13});
			REQUIRE(receiver->goods[livestock].status.Test(GoodsEntry::State::Acceptance)); delivered += previous - current;
		}
		previous = current; maximum = std::max(maximum, current); maximum_share = std::max(maximum_share, share);
		return delivered > 0;
	});
	VerifySurfaceFarmRebalance(true); VerifySurfaceFarmFeederOrders(); VerifySurfaceIntertown();
	// Original company delivery counters and fare books corroborate the native
	// receiver-side quantity decrease; virtual feeder share alone proves none.
	if (delivered > 0) { REQUIRE(work.delivered[livestock] >= delivered); REQUIRE(work.income > Money{0}); }
	fmt::print("SURFACE_RESUPPLY_FARM_FEEDER_RAIL_SERVICE ticks={} vehicle=52 boarded={} delivered={} maximum_aboard={} aboard={} maximum_share={} cash={} restored={} partial={} feeder_credit_is_cash=false\n",
		work.ticks, boarded, delivered, maximum, previous, maximum_share.base(), company->money.base(), company->money >= Money{100000}, delivered == 0);
}
#endif /* TEST_SURFACE_ENTRY_FARM_FEEDER_RAIL_H */
