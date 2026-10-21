/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_capital_income.h Genuine additional local service and bounded measured income.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CP.
 * Native purchase, orders, movement, passengers and books retain every owner.
 * Include after the pure capital proposal and native resupply command adapters.
 */
#ifndef TEST_SURFACE_ENTRY_CAPITAL_INCOME_H
#define TEST_SURFACE_ENTRY_CAPITAL_INCOME_H

/** @return The sole explicitly paid extra bus, or nullptr before its actual native purchase. */
static const RoadVehicle *FindSurfaceCapitalIncome()
{
	const RoadVehicle *result = nullptr;
	// Follow the explicit service name and actual model/cargo rather than a pool
	// slot which can be reused after an unrelated legitimate vehicle renewal.
	for (const auto *bus : RoadVehicle::Iterate()) {
		if (bus->name != "Surface Capital passenger 1") continue;
		REQUIRE(result == nullptr); REQUIRE(bus->owner == _current_company);
		REQUIRE(bus->engine_type == EngineID{119}); REQUIRE(bus->cargo_cap == 40);
		REQUIRE(bus->cargo_type == GetCargoTypeByLabel(CargoLabel{"PASS"})); result = bus;
	}
	return result;
}

/** Freshly quote all four still-unpaid original near-due road replacements.
 * @return Gross native invoice without any future refund or fare credit.
 */
static Money QuoteSurfaceCapitalRenewals()
{
	const std::map<std::string, std::pair<EngineID, TileIndex>> roles{
		{"Surface Regional passenger 1", {EngineID{119}, TileIndex{2984}}},
		{"Surface Regional passenger 2", {EngineID{119}, TileIndex{2984}}},
		{"Surface Regional passenger 3", {EngineID{119}, TileIndex{2984}}},
		{"Surface Steel delivery", {EngineID{152}, TileIndex{2443}}}};
	Money total{0}; std::set<std::string> found;
	// Every original role remains present and useful. Counting a sold or loaded
	// replacement candidate as a completed renewal would release its reserve.
	for (const auto *vehicle : RoadVehicle::Iterate()) {
		const auto role = roles.find(vehicle->name); if (role == roles.end()) continue;
		REQUIRE(found.insert(vehicle->name).second); REQUIRE(vehicle->owner == _current_company);
		REQUIRE(vehicle->engine_type == role->second.first); REQUIRE(vehicle->age < vehicle->max_age);
		const auto depot = role->second.second;
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(depot, depot, vehicle->engine_type, false, vehicle->cargo_type, ClientID::Server);
		REQUIRE(std::get<0>(quote).Succeeded()); REQUIRE(std::get<2>(quote) == vehicle->cargo_cap);
		total += std::get<0>(quote).GetCost();
	}
	REQUIRE(found.size() == roles.size()); REQUIRE(total == Money{25633}); return total;
}

/** Verify the genuine additional bus's canonical direct local route.
 * @param bus Actual purchased extra PASSbody, never one of the original eleven services.
 */
static void VerifySurfaceCapitalIncomeOrders(const RoadVehicle &bus)
{
	REQUIRE(bus.GetNumOrders() == 2);
	for (uint8_t index = 0; index < 2; ++index) {
		const auto *order = bus.GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_STATION));
		REQUIRE(order->GetDestination() == DestinationID{static_cast<uint16_t>(index == 0 ? 17 : 18)});
		REQUIRE(order->GetLoadType() == OrderLoadType::LoadIfPossible);
		REQUIRE(order->GetUnloadType() == OrderUnloadType::UnloadIfPossible);
		REQUIRE(order->GetStopLocation() == OrderStopLocation::FarEnd);
	}
}

/** Pay one fresh whole model119invoice and install only ordinary direct local service. */
static void BuySurfaceCapitalIncome()
{
	REQUIRE(FindSurfaceCapitalIncome() == nullptr); QuerySurfaceModuleCapital();
	const auto *company = Company::Get(_current_company); const auto before = company->money;
	const auto *depot = Depot::GetByTile(SurfaceRegionalLayout::depot); REQUIRE(depot != nullptr);
	const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"}); REQUIRE(RoadVehicle::CanAllocateItem(1));
	const auto bought = PaySurfaceResupply<Commands::BuildVehicle>(SurfaceRegionalLayout::depot, SurfaceRegionalLayout::depot,
		EngineID{119}, false, passenger, ClientID::Server);
	auto *bus = RoadVehicle::Get(std::get<1>(bought)); REQUIRE(bus->cargo.TotalCount() == 0);
	PaySurfaceResupply<Commands::RenameVehicle>(TileIndex{0}, bus->index, std::string{"Surface Capital passenger 1"});
	// Both existing paid stations accept normal delivery/loading. No transfer,
	// fabricated passenger source or new civic permission funds this experiment.
	for (uint8_t index = 0; index < 2; ++index) {
		Order order; order.MakeGoToStation(StationID{static_cast<uint16_t>(index == 0 ? 17 : 18)});
		order.SetLoadType(OrderLoadType::LoadIfPossible); order.SetUnloadType(OrderUnloadType::UnloadIfPossible);
		order.SetStopLocation(OrderStopLocation::FarEnd);
		PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, bus->index, VehicleOrderID{index}, order);
	}
	VerifySurfaceCapitalIncomeOrders(*bus);
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, bus->index, true);
	REQUIRE(FindSurfaceCapitalIncome() == bus); REQUIRE(company->money == before - Money{6562});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(!bus->vehstatus.Test(VehState::Stopped));
	fmt::print("SURFACE_RESUPPLY_CAPITAL_PAID vehicle={} engine=119 capacity=40 gross=6562 cash_before={} cash={} debt=5000000 first=17 last=18 depot=2984 buses=1 credit=0\n",
		bus->index.base(), before.base(), company->money.base());
}

/** Run only the first separately admitted native3000tick income observation.
 * The unpaid four renewals and second bounded interval remain protected. Native
 * passenger payments and costs determine the measured result without forecasts.
 */
static void WorkSurfaceCapitalIncome()
{
	const auto *bus = FindSurfaceCapitalIncome(); REQUIRE(bus != nullptr); VerifySurfaceCapitalIncomeOrders(*bus);
	REQUIRE(!bus->vehstatus.Test(VehState::Stopped)); REQUIRE(SurfaceChemicalWorkLimit() == 3000);
	const auto *company = Company::Get(_current_company); const auto renewals = QuoteSurfaceCapitalRenewals();
	const auto full = QuoteSurfaceModuleCapitalWork(0); REQUIRE(!full.year_boundary);
	const auto target = Money{40000} + renewals + full.upkeep + full.interest + full.station;
	REQUIRE(company->money >= target); const auto initial = bus->profit_this_year; const auto year = TimerGameEconomy::year;
	fmt::print("SURFACE_RESUPPLY_CAPITAL_PROTECTION ticks=6000 first_interval=3000 remaining_interval=3000 reserve=40000 renewal={} upkeep={} interest={} station_charge={} target={} cash={} funded=true projected_fares=0 refund_counted=0 credit_counted=0\n",
		renewals.base(), full.upkeep.base(), full.interest.base(), full.station.base(), target.base(), company->money.base());
	/** @return False until the actual interval ends; unpaid original renewals remain cash-backed on every native tick. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", 3000, [&] {
		REQUIRE(company->money >= Money{40000} + renewals); return false;
	});
	REQUIRE(receipt.ticks == 3000); REQUIRE(TimerGameEconomy::year == year);
	const auto remaining = QuoteSurfaceModuleCapitalWork(0, 3000); REQUIRE(!remaining.year_boundary);
	const auto protected_cash = Money{40000} + renewals + remaining.upkeep + remaining.interest + remaining.station;
	REQUIRE(company->money >= protected_cash); REQUIRE(company->current_loan == Money{5000000});
	fmt::print("SURFACE_RESUPPLY_CAPITAL_MEASURED vehicle={} ticks=3000 profit_delta256={} profit256={} cash={} renewal={} remaining_operating={} protected_cash={} reserve=40000 remaining_interval=3000 module_capital_complete=false\n",
		bus->index.base(), (bus->profit_this_year - initial).base(), bus->profit_this_year.base(), company->money.base(), renewals.base(),
		(remaining.upkeep + remaining.interest + remaining.station).base(), protected_cash.base());
}
#endif /* TEST_SURFACE_ENTRY_CAPITAL_INCOME_H */
