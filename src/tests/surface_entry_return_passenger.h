/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_return_passenger.h Pure actual Town1 return-pickup admission on the paid extra bus.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CSGX/15CSGY.
 * Existing passenger offers, native orders/roads and company books decide
 * admission. No passenger, permission, route, vehicle or payment is supplied.
 */
#ifndef TEST_SURFACE_ENTRY_RETURN_PASSENGER_H
#define TEST_SURFACE_ENTRY_RETURN_PASSENGER_H

/** Quote only real Town1pickup and Town0return delivery on the existing extra9bus. */
static void QuerySurfaceReturnPassenger()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{54619}); REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	const auto *bus = VerifySurfaceIntertown(); REQUIRE(bus->engine_type == EngineID{119}); REQUIRE(bus->cargo_cap == 40); REQUIRE(bus->cargo.TotalCount() == 0);
	VerifySurfaceFactoryGoodsOrders(); VerifySurfaceFarmFeederOrders(); VerifySurfaceFarmRebalance(true);
	const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"}); const auto *outbound = Station::Get(StationID{17}), *return_source = Station::Get(StationID{20});
	REQUIRE(outbound->owner == _current_company); REQUIRE(return_source->owner == _current_company);
	REQUIRE(outbound->town->index == TownID{0}); REQUIRE(return_source->town->index == TownID{1});
	REQUIRE(outbound->goods[passenger].status.Test(GoodsEntry::State::Acceptance)); REQUIRE(return_source->goods[passenger].status.Test(GoodsEntry::State::Acceptance));
	REQUIRE(IsStationRoadStopTile(TileIndex{1055})); REQUIRE(GetStationIndex(TileIndex{1055}) == StationID{20}); REQUIRE(GetRoadStopType(TileIndex{1055}) == RoadStopType::Bus);
	uint physical = 0; bool exact_source = true;
	// Real Town1passengers already waiting at20are a physical opportunity,
	// not a forecast fare. No copied town population or fabricated production
	// quantity may replace their actual sole station-owned packet references.
	for (const auto &[hop, packets] : *return_source->goods[passenger].GetData().cargo.Packets()) for (const auto *packet : packets) {
		physical += packet->Count();
		if (packet->GetSource().type != SourceType::Town || packet->GetSource().id != 1 || packet->GetFirstStation() != StationID{20}) exact_source = false;
	}
	const auto available = return_source->goods[passenger].AvailableCount(); REQUIRE(available <= physical);
	const auto graph = ReadSurfaceShortcutGraph(); const auto forward = SurfaceShortcutDistances(graph, outbound->xy), reverse = SurfaceShortcutDistances(graph, TileIndex{1055});
	REQUIRE(forward.contains(TileIndex{1055})); REQUIRE(reverse.contains(outbound->xy)); REQUIRE(forward.at(TileIndex{1055}) == reverse.at(outbound->xy));
	const auto pickup = QuoteSurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, bus->index, VehicleOrderID{1}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::LoadIfPossible));
	const auto unload = QuoteSurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, bus->index, VehicleOrderID{0}, MOF_UNLOAD, static_cast<uint16_t>(OrderUnloadType::UnloadIfPossible));
	// Only these two cargo policies change in the proposal. Other local and
	// regional buses retain their actual profitable service and original roles.
	for (const auto &[kind, quote] : std::array<std::pair<std::string_view, CommandCost>, 2>{{{"pickup20", pickup}, {"unload17", unload}}}) {
		if (quote.Succeeded()) REQUIRE(quote.GetCost() == Money{0});
		fmt::print("SURFACE_RESUPPLY_RETURN_PASSENGER_ORDER kind={} available={} invoice={} error={} paid=0\n", kind, quote.Succeeded(), quote.GetCost().base(), quote.GetErrorMessage().base());
	}
	const bool complete = physical > 0 && available > 0 && exact_source && pickup.Succeeded() && unload.Succeeded();
	fmt::print("SURFACE_RESUPPLY_RETURN_PASSENGER_SOURCE vehicle=9 engine=119 capacity=40 held=0 outbound=17 outbound_town=0 return_source=20 return_town=1 physical={} available={} exact_source={} forward={} reverse={} nodes={} last_profit_256={} current_profit_256={} complete={} purchases=0 ticks=0\n",
		physical, available, exact_source, forward.at(TileIndex{1055}), reverse.at(outbound->xy), graph.size(), bus->profit_last_year.base(), bus->profit_this_year.base(), complete);
	const auto bill = QuoteSurfaceFixedPriceIncome(2000); const auto operating = bill.upkeep + bill.interest + bill.station;
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto target = reserve + operating;
		fmt::print("SURFACE_RESUPPLY_RETURN_PASSENGER_ADMISSION ticks=2000 operating={} invoice=0 reserve={} target={} cash={} funded={} complete={} future_income_counted=0 refunds_counted=0 credit=0\n",
			operating.base(), reserve.base(), target.base(), company->money.base(), company->money >= target, complete);
	}
}
/** Apply only the two already admitted zero-price policies to the actual empty extra bus. */
static void OrderSurfaceReturnPassenger()
{
	QuerySurfaceReturnPassenger(); const auto *company = Company::Get(_current_company);
	const auto bill = QuoteSurfaceFixedPriceIncome(2000); const auto operating = bill.upkeep + bill.interest + bill.station;
	REQUIRE(company->money >= Money{40000} + operating); const auto cash = company->money;
	const auto *bus = VerifySurfaceIntertown(); REQUIRE(bus->cargo.TotalCount() == 0); const auto tile = bus->tile;
	PaySurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, bus->index, VehicleOrderID{1}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::LoadIfPossible));
	PaySurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, bus->index, VehicleOrderID{0}, MOF_UNLOAD, static_cast<uint16_t>(OrderUnloadType::UnloadIfPossible));
	VerifySurfaceIntertown(true); REQUIRE(bus->tile == tile); REQUIRE(bus->cargo.TotalCount() == 0); REQUIRE(company->money == cash);
	fmt::print("SURFACE_RESUPPLY_RETURN_PASSENGER_ORDERED vehicle=9 pickup=20 receiver=17 invoice=0 elapsed=0 cash={} operating={} target={} original_services_changed=0\n",
		cash.base(), operating.base(), (Money{40000} + operating).base());
}
#endif /* TEST_SURFACE_ENTRY_RETURN_PASSENGER_H */
