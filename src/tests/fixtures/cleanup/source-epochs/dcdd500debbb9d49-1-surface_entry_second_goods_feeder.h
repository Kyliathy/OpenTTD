/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_second_goods_feeder.h Pure second paid Goods carrier admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZK.
 * The real loaded25hull and existing paid12bay retain all physical owners.
 * Order estimates and waiting Goods grant no movement, fare or capital.
 */
#ifndef TEST_SURFACE_ENTRY_SECOND_GOODS_FEEDER_H
#define TEST_SURFACE_ENTRY_SECOND_GOODS_FEEDER_H

/** Quote only the existing loaded25receiver change and complete present operating bill. */
static void QuerySurfaceSecondGoodsFeeder()
{
	const auto *train = VerifySurfaceGoodsRailDelivery(); const auto *truck = RoadVehicle::Get(VehicleID{25});
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{49250});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	REQUIRE(truck->cargo.TotalCount() == 18); REQUIRE(truck->cargo_payment == nullptr);
	REQUIRE(truck->cur_implicit_order_index == VehicleOrderID{1}); REQUIRE(truck->current_order.GetDestination() == DestinationID{20});
	REQUIRE(CanVehicleUseStation(truck, Station::Get(StationID{12})));
	// This loaded commercial cohort may be redirected through the same
	// physical railway. Original launch Goods and invented first-station or
	// production identities cannot become feeder supply.
	for (const auto *packet : *truck->cargo.Packets()) {
		REQUIRE(packet->GetFirstStation() == StationID{13}); REQUIRE(packet->GetSource().type == SourceType::Industry);
		REQUIRE((packet->GetSource().id == 5 || packet->GetSource().id == 11));
	}
	auto order = SurfaceFactoryGoodsOrder(false); order.SetDestination(DestinationID{12}); order.SetUnloadType(OrderUnloadType::Transfer);
	const auto remove = QuoteSurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{1});
	const auto insert = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, order);
	const auto pickup = QuoteSurfaceResupply<Commands::SkipToOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
	const auto receiver = QuoteSurfaceResupply<Commands::SkipToOrder>(TileIndex{0}, truck->index, VehicleOrderID{1});
	REQUIRE(remove.Succeeded()); REQUIRE(insert.Succeeded()); REQUIRE(pickup.Succeeded()); REQUIRE(receiver.Failed());
	REQUIRE(remove.GetCost() == Money{0}); REQUIRE(insert.GetCost() == Money{0}); REQUIRE(pickup.GetCost() == Money{0});
	const auto stock = SurfaceGoodsRailDeliveryStock(train);
	fmt::print("SURFACE_RESUPPLY_SECOND_GOODS_FEEDER_SOURCE vehicle=25 engine=140 capacity=18 held=18 pickup=13 old_receiver=20 proposed_receiver=12 transfer=true actual_facility=true delete_available=true insert_available=true invoice=0 current_index=1 skip_pickup_available=true skip_receiver_available=false skip_reason=already_current rail_capacity=300 rail_first13={} rail_share={} cash={} paid=0 elapsed=0 future_income_counted=0\n",
		stock.first, stock.second.base(), company->money.base());
	// The first whole funded candidate alone admits a later bounded work
	// proposal. Loaded and waiting Goods, prospective train fares, virtual
	// transfer shares and constructor resale value never pay this bill.
	const auto cap = QuoteSurfaceGoodsRailFeederWork(Money{0}, 2000);
	fmt::print("SURFACE_RESUPPLY_SECOND_GOODS_FEEDER_SELECTION ceiling=2000 ticks={} funded={} route_quoted=true order_change_complete=false work_permission=false\n", cap, cap > 0);
}
#endif /* TEST_SURFACE_ENTRY_SECOND_GOODS_FEEDER_H */
