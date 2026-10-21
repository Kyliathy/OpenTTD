/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_intertown.h Genuine longer delivery using only the existing paid extra bus.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CSGK/15CSGY.
 * Historical pickup remains SourceTown0 only; explicit later return mode
 * admits actual Town1pickup at20and final delivery17. Native owners alone act;
 * no body, road, station, civic permission, passenger or fare is supplied.
 */
#ifndef TEST_SURFACE_ENTRY_INTERTOWN_H
#define TEST_SURFACE_ENTRY_INTERTOWN_H

/** @return Ordinary final-delivery receiver order; NoLoad retains strict Town0 provenance. */
static Order SurfaceIntertownReceiver()
{
	Order order; order.MakeGoToStation(StationID{20}); order.SetLoadType(OrderLoadType::NoLoad);
	order.SetUnloadType(OrderUnloadType::UnloadIfPossible); order.SetStopLocation(OrderStopLocation::FarEnd); return order;
}

/** Verify the single extra bus's actual intertown pickup/receiver orders.
 * @param capital_return Explicit later two-way passenger policy; false preserves every historical one-way phase.
 * @return Existing paid model119extra bus, separate from the original eleven roles.
 */
static const RoadVehicle *VerifySurfaceIntertown(bool capital_return = false)
{
	const auto *bus = FindSurfaceCapitalIncome(); REQUIRE(bus != nullptr); REQUIRE(bus->index == VehicleID{9}); REQUIRE(bus->GetNumOrders() == 2);
	// This existing extra body alone gains the admitted empty return leg.
	// Both native destinations stay fixed; other passenger services are untouched.
	for (uint8_t index = 0; index < 2; ++index) {
		const auto *order = bus->GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_STATION));
		REQUIRE(order->GetDestination() == DestinationID{static_cast<uint16_t>(index == 0 ? 17 : 20)});
		REQUIRE(order->GetLoadType() == (capital_return || index == 0 ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad));
		REQUIRE(order->GetUnloadType() == (!capital_return && index == 0 ? OrderUnloadType::NoUnload : OrderUnloadType::UnloadIfPossible));
	}
	return bus;
}

/** Read only actual Town1return passengers in the paid extra bus.
 * @param bus Exact previously verified extra9/119/cap40 passenger body.
 * @return Real return-cohort quantity across native packet splits and merges, never forecast income.
 */
static uint SurfaceReturnPassengerStock(const RoadVehicle *bus)
{
	REQUIRE(bus->index == VehicleID{9}); REQUIRE(bus->owner == _current_company);
	REQUIRE(bus->engine_type == EngineID{119}); REQUIRE(bus->cargo_cap == 40);
	REQUIRE(bus->cargo_type == GetCargoTypeByLabel(CargoLabel{"PASS"})); uint quantity = 0;
	// Source identity and first station survive native split/merge. Recycled
	// packet IDs cannot identify the cohort, and waiting passengers own no fare.
	for (const auto *packet : *bus->cargo.Packets()) {
		REQUIRE(packet->GetSource().type == SourceType::Town); REQUIRE(packet->GetSource().id <= 1);
		if (packet->GetSource().id != 1) continue;
		REQUIRE(packet->GetFirstStation() == StationID{20}); quantity += packet->Count();
	}
	return quantity;
}

/** Purely quote actual existing-stop delivery and the entire first2000tick company bill. */
static void QuerySurfaceIntertown()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{62726}); REQUIRE(company->current_loan == Money{5000000});
	const auto *bus = FindSurfaceCapitalIncome(); REQUIRE(bus != nullptr); REQUIRE(bus->index == VehicleID{9}); VerifySurfaceCapitalIncomeOrders(*bus);
	REQUIRE(!bus->vehstatus.Test(VehState::Stopped)); const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"});
	const auto *source = Station::Get(StationID{17}), *receiver = Station::Get(StationID{20});
	REQUIRE(source->town->index == TownID{0}); REQUIRE(receiver->town->index == TownID{1});
	REQUIRE(receiver->goods[passenger].status.Test(GoodsEntry::State::Acceptance));
	REQUIRE(IsStationRoadStopTile(TileIndex{1055})); REQUIRE(GetStationIndex(TileIndex{1055}) == StationID{20});
	REQUIRE(GetRoadStopType(TileIndex{1055}) == RoadStopType::Bus);
	const auto component = SurfaceGoodsRoadComponent(TileIndex{2443}); REQUIRE(component.contains(source->xy)); REQUIRE(component.contains(TileIndex{1055}));
	const auto remove = QuoteSurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, bus->index, VehicleOrderID{1});
	const auto insert = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, bus->index, VehicleOrderID{1}, SurfaceIntertownReceiver());
	const auto pickup = QuoteSurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, bus->index, VehicleOrderID{0}, MOF_UNLOAD, static_cast<uint16_t>(OrderUnloadType::NoUnload));
	REQUIRE(remove.Succeeded()); REQUIRE(remove.GetCost() == Money{0}); REQUIRE(insert.Succeeded()); REQUIRE(insert.GetCost() == Money{0});
	REQUIRE(pickup.Succeeded()); REQUIRE(pickup.GetCost() == Money{0});
	fmt::print("SURFACE_RESUPPLY_INTERTOWN_SOURCE vehicle=9 engine=119 capacity=40 held={} source=17 source_town=0 waiting={} receiver=20 receiver_town=1 bus_stop=1055 connected=true component={} remaining_days={} order_cost=0 purchases=0 projected_fares=0\n",
		bus->cargo.TotalCount(), source->goods[passenger].AvailableCount(), component.size(), bus->max_age.base() - bus->age.base());
	QuoteSurfaceFixedPriceIncome(2000); const auto work = QuoteSurfaceModuleCapitalWork(0, 2000); const auto operating = work.upkeep + work.interest + work.station;
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto target = reserve + operating;
		fmt::print("SURFACE_RESUPPLY_INTERTOWN_ADMISSION ticks=2000 days={} months={} year_boundary={} operating={} reserve={} target={} cash={} funded={} order_cost=0 future_income_counted=0 refunds_counted=0 credit=0\n",
			work.days, work.months, work.year_boundary, operating.base(), reserve.base(), target.base(), company->money.base(), company->money >= target);
	}
}

/** Reassign only the extra bus while retaining every already loaded Town0passenger. */
static void ChangeSurfaceIntertown()
{
	QuerySurfaceIntertown(); QuoteSurfaceFarmUpgrade("intertown-change", 2000, 0, Money{0}, false);
	const auto *bus = FindSurfaceCapitalIncome(); const auto held = bus->cargo.TotalCount(); const auto cash = Company::Get(_current_company)->money;
	PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, bus->index, VehicleOrderID{1});
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, bus->index, VehicleOrderID{1}, SurfaceIntertownReceiver());
	PaySurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, bus->index, VehicleOrderID{0}, MOF_UNLOAD, static_cast<uint16_t>(OrderUnloadType::NoUnload));
	// Existing passengers should reach their actual new accepting receiver;
	// the source pickup cannot discharge them as an artificial local payment.
	if (held > 0) PaySurfaceResupply<Commands::SkipToOrder>(TileIndex{0}, bus->index, VehicleOrderID{1});
	VerifySurfaceIntertown(); REQUIRE(bus->cargo.TotalCount() == held); REQUIRE(Company::Get(_current_company)->money == cash);
	fmt::print("SURFACE_RESUPPLY_INTERTOWN_CHANGED vehicle=9 held={} source=17 receiver=20 order_cost=0 elapsed=0 original_services_changed=0\n", held);
}

/** Measure actual intertown payment and whole-company cash without any annual projection. */
static void WorkSurfaceIntertown()
{
	const auto *bus = VerifySurfaceIntertown(); REQUIRE(!bus->vehstatus.Test(VehState::Stopped));
	QuoteSurfaceFarmUpgrade("intertown-service", 2000, 0, Money{0}, false); const auto *company = Company::Get(_current_company);
	const auto profit = bus->profit_this_year; const auto year = TimerGameEconomy::year; bool delivered = false;
	/** @return Whether actual company cash has restored the working reserve. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", 2000, [&] {
		REQUIRE(company->money >= Money{40000});
		// NoLoad at Town1 means receiver-side emptying follows real final
		// delivery; it cannot be replaced by newly picked-up Town1passengers.
		if (bus->last_station_visited == StationID{20} && bus->cargo.TotalCount() == 0) delivered = true;
		return company->money >= Money{100000};
	});
	auto delta = bus->profit_this_year - profit;
	// Native yearly rollover retains the preceding year separately. Include
	// both actual sides so a reset cannot fabricate a route profit or loss.
	if (TimerGameEconomy::year != year) { delta = bus->profit_last_year - profit + bus->profit_this_year; }
	VerifySurfaceIntertown();
	fmt::print("SURFACE_RESUPPLY_INTERTOWN_SERVICE ticks={} vehicle=9 delivered={} held={} profit_delta_256={} cash={} restored={} partial={} annual_projection=false module_capital_complete=false\n",
		receipt.ticks, delivered, bus->cargo.TotalCount(), delta.base(), company->money.base(), company->money >= Money{100000}, !receipt.reached);
}
#endif /* TEST_SURFACE_ENTRY_INTERTOWN_H */
