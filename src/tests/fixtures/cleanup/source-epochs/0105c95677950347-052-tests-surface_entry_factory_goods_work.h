/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_factory_goods_work.h Actual zero-price direct commercial Goods orders and native work.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CSGU/15CSGVC.
 * The actual paid3/25bodies retain every loaded unit. Native cargo/payment
 * owners alone supply delivery, movement and income during funded work.
 */
#ifndef TEST_SURFACE_ENTRY_FACTORY_GOODS_WORK_H
#define TEST_SURFACE_ENTRY_FACTORY_GOODS_WORK_H

/** Verify the actual direct non-stop policy on both original commercial carriers. */
static void VerifySurfaceFactoryGoodsOrders()
{
	for (const auto id : {VehicleID{3}, VehicleID{25}}) {
		const auto *truck = RoadVehicle::Get(id); REQUIRE(truck->owner == _current_company); REQUIRE(truck->engine_type == EngineID{140}); REQUIRE(truck->cargo_cap == 18);
		REQUIRE(truck->cargo_type == GetCargoTypeByLabel(CargoLabel{"GOOD"})); REQUIRE(truck->GetNumOrders() == 2); REQUIRE(!truck->vehstatus.Test(VehState::Stopped));
		for (uint8_t index = 0; index < 2; ++index) {
			const auto *order = truck->GetOrder(index); const auto expected = SurfaceFactoryGoodsOrder(index == 0);
			REQUIRE(order->IsType(OT_GOTO_STATION)); REQUIRE(order->GetDestination() == expected.GetDestination());
			REQUIRE(order->GetNonStopType() == expected.GetNonStopType()); REQUIRE(order->GetLoadType() == expected.GetLoadType()); REQUIRE(order->GetUnloadType() == expected.GetUnloadType());
		}
	}
}

/** Reassign only3/25while preserving loaded cargo, native positions and physical time. */
static void OrderSurfaceFactoryGoods()
{
	QuerySurfaceFactoryGoods(true); QuoteSurfaceFarmRoadWork("factory-goods-orders", Money{0}, false);
	const auto *company = Company::Get(_current_company); const auto cash = company->money;
	// Each independent old list may include native implicit stops. Replace
	// only this commercial role's list; loaded cargo must finish at20before
	// returning to13under the new explicit no-intermediate-stop policy.
	for (const auto id : {VehicleID{3}, VehicleID{25}}) {
		const auto *truck = RoadVehicle::Get(id); const auto held = truck->cargo.TotalCount(); const auto tile = truck->tile;
		while (truck->GetNumOrders() > 0) PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, id, VehicleOrderID{0});
		for (uint8_t index = 0; index < 2; ++index) PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, id, VehicleOrderID{index}, SurfaceFactoryGoodsOrder(index == 0));
		const VehicleOrderID target = held > 0 ? 1 : 0;
		// Native skipping requires a different current index. Reset through
		// the other real order when needed; no simulation tick is supplied.
		if (truck->cur_implicit_order_index == target) PaySurfaceResupply<Commands::SkipToOrder>(TileIndex{0}, id, VehicleOrderID{static_cast<uint8_t>(1 - target)});
		PaySurfaceResupply<Commands::SkipToOrder>(TileIndex{0}, id, target);
		REQUIRE(truck->cargo.TotalCount() == held); REQUIRE(truck->tile == tile);
		fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_ORDERED vehicle={} held={} first_destination={} invoice=0 elapsed=0\n", id.base(), held, held > 0 ? 20 : 13);
	}
	VerifySurfaceFactoryGoodsOrders(); REQUIRE(company->money == cash);
	fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_ROUTE_PAID vehicles=2 pickup=13 receiver=20 invoice=0 cash={} elapsed=0 new_vehicle=0\n", cash.base());
}

/** Earn only the newly funded direct Goods interval and read actual rail handoff.
 * @param limit Explicitly admitted native cap; historical default remains2000, while the separate liquidity phase may select a smaller complete funded interval.
 */
static void WorkSurfaceFactoryGoods(uint limit = 2000)
{
	VerifySurfaceFactoryGoodsOrders(); VerifySurfaceFarmFeederOrders(); const auto *train = VerifySurfaceFarmRebalance(true); VerifySurfaceIntertown();
	const auto *company = Company::Get(_current_company); const auto bill = QuoteSurfaceFixedPriceIncome(limit);
	const auto operating = bill.upkeep + bill.interest + bill.station; const auto target = Money{40000} + operating;
	fmt::print("SURFACE_RESUPPLY_FARM_ROAD_WORK_ADMISSION stage=factory-goods-service ticks={} unpaid=0 operating={} extra_running=0 reserve=40000 target={} cash={} funded={} future_income_counted=0 refunds_counted=0 credit=0\n",
		limit, operating.base(), target.base(), company->money.base(), company->money >= target);
	REQUIRE(company->money >= target); REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	std::set<VehicleID> loaded, picked_up, delivered, pickup_allowed; std::map<VehicleID, Money> profit;
	const auto year = TimerGameEconomy::year;
	for (const auto id : {VehicleID{3}, VehicleID{25}}) {
		const auto *truck = RoadVehicle::Get(id); profit[id] = truck->profit_this_year;
		if (truck->cargo.TotalCount() == 0) pickup_allowed.insert(id);
	}
	uint previous = SurfaceFarmFeederRailStock(train).first, boarded = 0, rail_delivered = 0;
	/** @return Whether actual company cash has restored the working reserve; cargo observations supply no forecast credit. */
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, [&] {
		REQUIRE(company->money >= Money{40000});
		// Distinguish a real new13pickup from25's preserved initial shipment.
		// Native emptying at the explicit accepting20proves a paid delivery.
		for (const auto id : {VehicleID{3}, VehicleID{25}}) {
			const auto *truck = RoadVehicle::Get(id);
			if (truck->cargo.TotalCount() == 0) pickup_allowed.insert(id);
			if (truck->cargo.TotalCount() > 0) loaded.insert(id);
			if (truck->cargo.TotalCount() > 0 && truck->last_station_visited == StationID{13} && pickup_allowed.contains(id)) picked_up.insert(id);
			if (loaded.contains(id) && truck->cargo.TotalCount() == 0 && truck->last_station_visited == StationID{20}) delivered.insert(id);
		}
		const auto current = SurfaceFarmFeederRailStock(train).first;
		if (current > previous) boarded += current - previous;
		// These same funded ticks may carry real first22/Industry7cargo on
		// original52. Only native accepting13unload counts as final delivery.
		if (current < previous) { REQUIRE(train->last_station_visited == StationID{13}); rail_delivered += previous - current; }
		previous = current; return company->money >= Money{100000};
	});
	VerifySurfaceFactoryGoodsOrders(); VerifySurfaceFarmRebalance(true); VerifySurfaceFarmFeederOrders(); VerifySurfaceIntertown();
	for (const auto id : {VehicleID{3}, VehicleID{25}}) {
		const auto *truck = RoadVehicle::Get(id); auto delta = truck->profit_this_year - profit.at(id);
		if (TimerGameEconomy::year != year) delta = truck->profit_last_year - profit.at(id) + truck->profit_this_year;
		fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_SERVICE_VEHICLE vehicle={} held={} picked_up={} delivered={} profit_delta_256={}\n", id.base(), truck->cargo.TotalCount(), picked_up.contains(id), delivered.contains(id), delta.base());
	}
	fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_SERVICE ticks={} cash={} restored={} picked_up={} delivered={} rail_boarded={} rail_delivered={} rail_aboard={} partial={} feeder_credit_is_cash=false\n",
		work.ticks, company->money.base(), company->money >= Money{100000}, picked_up.size(), delivered.size(), boarded, rail_delivered, previous, delivered.size() != 2 || picked_up.size() != 2);
}

/** Select one whole affordable interval without advancing time or counting future fares.
 * @param ceiling Explicit caller-owned maximum, at most3000; Group15CSGVCprovides1000.
 */
static void WorkSurfaceFactoryGoodsLiquidity(uint ceiling)
{
	REQUIRE(ceiling > 0); REQUIRE(ceiling <= 3000); const auto *company = Company::Get(_current_company);
	// Choose only among complete independently priced caps. A longer refused
	// interval cannot borrow its expected fare or implicitly authorize a later
	// window. The selected worker rechecks the same full native bill before ticks.
	for (const uint limit : {3000, 2000, 1000, 500, 250, 64}) {
		if (limit > ceiling) continue;
		const auto bill = QuoteSurfaceFixedPriceIncome(limit); const auto operating = bill.upkeep + bill.interest + bill.station;
		const auto target = Money{40000} + operating; const bool funded = company->money >= target;
		fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_LIQUID_CANDIDATE ticks={} operating={} target={} cash={} funded={} future_income_counted=0\n",
			limit, operating.base(), target.base(), company->money.base(), funded);
		if (!funded) continue;
		fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_LIQUID_SELECTION ceiling={} found=true ticks={} operating={} cash={}\n", ceiling, limit, operating.base(), company->money.base());
		WorkSurfaceFactoryGoods(limit); return;
	}
	fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_LIQUID_SELECTION ceiling={} found=false ticks=0 operating=0 cash={}\n", ceiling, company->money.base());
}
#endif /* TEST_SURFACE_ENTRY_FACTORY_GOODS_WORK_H */
