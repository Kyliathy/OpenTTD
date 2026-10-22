/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_factory_goods.h Pure direct natural-factory trade on actual paid Goods carriers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGT.
 * Physical offers and native order/road/cost owners decide admission. No
 * existing cargo, company balance, destination or simulation time is changed.
 */
#ifndef TEST_SURFACE_ENTRY_FACTORY_GOODS_H
#define TEST_SURFACE_ENTRY_FACTORY_GOODS_H

/** Build the proposed explicit commercial pickup or final-delivery order.
 * @param pickup Whether to serve actualFactory5at13or acceptingTown1at20.
 * @return Native no-intermediate-stop order, retaining strict pickup/final cargo policy.
 */
static Order SurfaceFactoryGoodsOrder(bool pickup)
{
	Order order; order.MakeGoToStation(pickup ? StationID{13} : StationID{20}); order.SetStopLocation(OrderStopLocation::FarEnd);
	order.SetNonStopType(OrderNonStopFlag::NonStop);
	order.SetLoadType(pickup ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad);
	order.SetUnloadType(pickup ? OrderUnloadType::NoUnload : OrderUnloadType::UnloadIfPossible); return order;
}

/** Purely characterize actual source stock, complete existing routes, zero-cost orders and operating admission. */
static void QuerySurfaceFactoryGoods()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{54793}); REQUIRE(company->current_loan == Money{5000000});
	VerifySurfaceFarmFeederOrders(); VerifySurfaceFarmRebalance(true); VerifySurfaceIntertown();
	const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}); const auto *source = Station::Get(StationID{13}), *old = Station::Get(StationID{19}), *receiver = Station::Get(StationID{20});
	REQUIRE(receiver->goods[goods].status.Test(GoodsEntry::State::Acceptance));
	REQUIRE(IsStationRoadStopTile(TileIndex{2572})); REQUIRE(GetStationIndex(TileIndex{2572}) == StationID{13});
	REQUIRE(IsStationRoadStopTile(TileIndex{731})); REQUIRE(GetStationIndex(TileIndex{731}) == StationID{20});
	uint physical = 0; bool exact_source = true;
	// Only native existing Factory5offers at actual13are usable facts.
	// Unproduced cargo and virtual feeder credits cannot fund this proposal.
	for (const auto &[hop, packets] : *source->goods[goods].GetData().cargo.Packets()) for (const auto *packet : packets) {
		physical += packet->Count();
		if (packet->GetSource().type != SourceType::Industry || packet->GetSource().id != 5 || packet->GetFirstStation() != StationID{13}) exact_source = false;
	}
	const auto graph = ReadSurfaceShortcutGraph(); const auto distances = SurfaceShortcutDistances(graph, TileIndex{731});
	REQUIRE(distances.contains(TileIndex{2572})); REQUIRE(distances.contains(old->xy));
	const auto old_distance = distances.at(old->xy), distance = distances.at(TileIndex{2572}); bool orders = true;
	// Inspect the actual two paid bodies and quote each complete new order.
	// Existing loaded Goods remain owned and must finish at20before pickup;
	// no discarded cargo, replacement body or refit is part of admission.
	for (const auto id : {VehicleID{3}, VehicleID{25}}) {
		const auto *truck = RoadVehicle::Get(id); REQUIRE(truck->owner == _current_company); REQUIRE(truck->engine_type == EngineID{140});
		REQUIRE(truck->cargo_type == goods); REQUIRE(truck->cargo_cap == 18); REQUIRE(!truck->vehstatus.Test(VehState::Stopped));
		for (uint8_t index = 0; index < 2; ++index) {
			const auto quote = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, id, VehicleOrderID{index}, SurfaceFactoryGoodsOrder(index == 0));
			if (quote.Failed()) orders = false;
			if (quote.Succeeded()) REQUIRE(quote.GetCost() == Money{0});
			fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_ORDER vehicle={} index={} destination={} available={} invoice={} error={} non_stop=true paid=0\n",
				id.base(), index, index == 0 ? 13 : 20, quote.Succeeded(), quote.GetCost().base(), quote.GetErrorMessage().base());
		}
		fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_VEHICLE vehicle={} engine=140 capacity=18 held={} tile={} remaining={} finish_loaded_at=20 changed=0\n",
			id.base(), truck->cargo.TotalCount(), truck->tile.base(), truck->max_age.base() - truck->age.base());
	}
	const auto available = source->goods[goods].AvailableCount(); REQUIRE(available <= physical);
	const bool complete = exact_source && physical > 0 && available > 0 && orders;
	fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_SOURCE station=13 industry=5 tile=2572 receiver=20 receiver_tile=731 physical={} available={} old_station=19 old_available={} old_distance={} distance={} nodes={} exact_source={} complete={} purchases=0 ticks=0\n",
		physical, available, old->goods[goods].AvailableCount(), old_distance, distance, graph.size(), exact_source, complete);
	QuoteSurfaceFixedPriceIncome(2000); const auto work = QuoteSurfaceModuleCapitalWork(0, 2000); const auto operating = work.upkeep + work.interest + work.station;
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto target = reserve + operating;
		fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_ADMISSION ticks=2000 operating={} invoice=0 reserve={} target={} cash={} funded={} complete={} future_income_counted=0 refunds_counted=0 credit=0\n",
			operating.base(), reserve.base(), target.base(), company->money.base(), company->money >= target, complete);
	}
}
#endif /* TEST_SURFACE_ENTRY_FACTORY_GOODS_H */
