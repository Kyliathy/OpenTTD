/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_goods_trade.h Paid ordinary Goods trade on two actual empty reserves.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGB.
 * Native orders, factory offers, public construction, packets and company books
 * own the first measured delivery. No future cargo or income funds startup.
 */
#ifndef TEST_SURFACE_ENTRY_GOODS_TRADE_H
#define TEST_SURFACE_ENTRY_GOODS_TRADE_H

/** Verify the exact direct trade route on an actual reused reserve.
 * @param truck Real140/cap18Goods body, preserving its original paid identity/name.
 */
static void VerifySurfaceGoodsTradeOrders(const RoadVehicle &truck)
{
	REQUIRE(truck.owner == _current_company); REQUIRE(truck.engine_type == EngineID{140}); REQUIRE(truck.cargo_cap == 18);
	REQUIRE(truck.GetNumOrders() == 2);
	for (uint8_t index = 0; index < 2; ++index) {
		const auto *order = truck.GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_STATION));
		REQUIRE(order->GetDestination() == DestinationID{static_cast<uint16_t>(index == 0 ? 19 : 20)});
		REQUIRE(order->GetLoadType() == (index == 0 ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad));
		REQUIRE(order->GetUnloadType() == OrderUnloadType::UnloadIfPossible);
	}
}

/** Pay only the admitted native Goods receiver and reuse two genuine empty stopped vehicles. */
static void StartSurfaceGoodsTrade()
{
	QuerySurfaceGoodsIncome(); const TileIndex tile{731}; const auto *company = Company::Get(_current_company);
	const auto before = company->money; const auto bits = GetRoadBits(tile, RoadTramType::Road);
	const auto direction = bits == ROAD_X ? DiagDirection::NE : DiagDirection::SE;
	const auto work = QuoteSurfaceFixedPriceIncome(3000);
	REQUIRE(before >= Money{40000 + 150 + 252} + work.upkeep + work.interest + work.station);
	PaySurfaceResupply<Commands::BuildRoadStop>(tile, tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck,
		true, direction, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, StationID{20}, true);
	REQUIRE(GetStationIndex(tile) == StationID{20}); REQUIRE(GetRoadStopType(tile) == RoadStopType::Truck);
	// Empty original reserves alone change duty. Removing every old pickup/home
	// order prevents touching the historical fiftyGoods stored for launch.
	for (const auto id : {VehicleID{3}, VehicleID{25}}) {
		auto *truck = RoadVehicle::Get(id); REQUIRE(truck->cargo.TotalCount() == 0); REQUIRE(truck->vehstatus.Test(VehState::Stopped));
		while (truck->GetNumOrders() > 0) {
			PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, id, VehicleOrderID{0});
		}
		for (uint8_t index = 0; index < 2; ++index) {
			Order order; order.MakeGoToStation(StationID{static_cast<uint16_t>(index == 0 ? 19 : 20)});
			order.SetLoadType(index == 0 ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad);
			order.SetUnloadType(OrderUnloadType::UnloadIfPossible); order.SetStopLocation(OrderStopLocation::FarEnd);
			PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, id, VehicleOrderID{index}, order);
		}
		VerifySurfaceGoodsTradeOrders(*truck); PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, id, true);
		REQUIRE(!truck->vehstatus.Test(VehState::Stopped));
	}
	REQUIRE(company->money == before - Money{150}); REQUIRE(company->current_loan == Money{5000000});
	fmt::print("SURFACE_RESUPPLY_TRADE_PAID tile=731 station=20 invoice=150 vehicles=2 capacity=36 cash_before={} cash={} added_vehicles=0 credit=0\n", before.base(), company->money.base());
}

/** Earn one fully funded actual first trade interval and retain every genuine partial endpoint. */
static void WorkSurfaceGoodsTrade()
{
	const auto *company = Company::Get(_current_company); const auto limit = SurfaceChemicalWorkLimit();
	const auto work = QuoteSurfaceFixedPriceIncome(limit); const auto operating = work.upkeep + work.interest + work.station;
	REQUIRE(company->money >= Money{40000} + operating); REQUIRE(company->current_loan == Money{5000000});
	for (const auto id : {VehicleID{3}, VehicleID{25}}) VerifySurfaceGoodsTradeOrders(*RoadVehicle::Get(id));
	fmt::print("SURFACE_RESUPPLY_TRADE_ADMISSION ticks={} operating={} reserve=40000 target={} cash={} funded=true purchases=0 future_income_counted=0 refunds_counted=0 credit=0\n",
		limit, operating.base(), (Money{40000} + operating).base(), company->money.base());
	std::set<VehicleID> delivered;
	/** @return Whether both actual Goods deliveries and genuine working cash have been demonstrated. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, [&] {
		REQUIRE(company->money >= Money{40000});
		// Reaching the receiver empty with positive operating profit proves a
		// real first payment rather than an empty activation visit or forecast.
		for (const auto id : {VehicleID{3}, VehicleID{25}}) {
			const auto *truck = RoadVehicle::Get(id);
			if (truck->last_station_visited == StationID{20} && truck->cargo.TotalCount() == 0 && truck->profit_this_year > Money{0}) delivered.insert(id);
		}
		// Working cash alone cannot claim the new Goods route has delivered.
		return company->money >= Money{100000} && delivered.size() == 2;
	});
	for (const auto id : {VehicleID{3}, VehicleID{25}}) {
		const auto *truck = RoadVehicle::Get(id);
		fmt::print("SURFACE_RESUPPLY_TRADE_VEHICLE vehicle={} held={} profit256={} delivered={} tile={}\n", id.base(), truck->cargo.TotalCount(), truck->profit_this_year.base(), delivered.contains(id), truck->tile.base());
	}
	fmt::print("SURFACE_RESUPPLY_TRADE_PROGRESS ticks={} cash={} restored={} route_demonstrated={} partial={} working_shortfall={} module_base_shortfall={} module_capital_complete=false\n",
		receipt.ticks, company->money.base(), company->money >= Money{100000}, delivered.size() == 2, !receipt.reached,
		std::max(Money{0}, Money{100000} - company->money).base(), std::max(Money{0}, Money{510000} - company->money).base());
}
#endif /* TEST_SURFACE_ENTRY_GOODS_TRADE_H */
