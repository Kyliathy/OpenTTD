/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_farm_road_work.h Actual paid native farm-road stops, carrier and measured service.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGN.
 * Separate saved command boundaries retain successful stop purchases before
 * the later actual truck ID. No civic permission, cargo or future fare is given.
 */
#ifndef TEST_SURFACE_ENTRY_FARM_ROAD_WORK_H
#define TEST_SURFACE_ENTRY_FARM_ROAD_WORK_H

/** @return Sole actual named137Source7Livestock body, or nullptr before its native purchase. */
static const RoadVehicle *FindSurfaceFarmRoadIncome()
{
	const RoadVehicle *result = nullptr;
	// Follow the actual service name rather than guessing a recyclable pool ID.
	// Historical defaults still reject this role until explicitly admitted.
	for (const auto *truck : RoadVehicle::Iterate()) {
		if (truck->name != "Surface Farm livestock income") continue;
		REQUIRE(result == nullptr); REQUIRE(truck->owner == _current_company); REQUIRE(truck->engine_type == EngineID{137});
		REQUIRE(truck->cargo_type == GetCargoTypeByLabel(CargoLabel{"LVST"})); REQUIRE(truck->cargo_cap == 18); result = truck;
	}
	return result;
}

/** Protect every unpaid native invoice and the entire next2000tick company bill.
 * @param stage Actual stop, vehicle or service admission diagnostic.
 * @param unpaid Gross still-unpaid native invoices, never future refunds.
 * @param adding Whether the not-yet-built model137running rate must also be reserved.
 */
static void QuoteSurfaceFarmRoadWork(std::string_view stage, Money unpaid, bool adding)
{
	QuoteSurfaceFixedPriceIncome(2000); const auto work = QuoteSurfaceModuleCapitalWork(0, 2000); Money extra{0};
	if (adding) {
		const auto *engine = Engine::Get(EngineID{137}); REQUIRE(engine->GetGRF() == nullptr);
		const auto annual = engine->GetRunningCost().base(); REQUIRE(annual >= 0);
		const auto *company = Company::Get(_current_company);
		const int lead = company->settings.engine_renew ? std::max(0, -int{company->settings.engine_renew_months} * 30) : 0;
		REQUIRE(engine->GetLifeLengthInDays().base() > static_cast<int>(work.days) + lead);
		// A proposed body is absent from current active-vehicle costs. Reserve
		// its whole native annual rate separately, with independent rounding.
		constexpr uint64_t denominator = uint64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS;
		extra = Money{static_cast<int64_t>((static_cast<uint64_t>(annual) * 2000 + denominator - 1) / denominator) + 2};
	}
	const auto *company = Company::Get(_current_company); const auto operating = work.upkeep + work.interest + work.station;
	const auto target = Money{40000} + unpaid + operating + extra;
	fmt::print("SURFACE_RESUPPLY_FARM_ROAD_WORK_ADMISSION stage={} ticks=2000 unpaid={} operating={} extra_running={} reserve=40000 target={} cash={} funded={} future_income_counted=0 refunds_counted=0 credit=0\n",
		stage, unpaid.base(), operating.base(), extra.base(), target.base(), company->money.base(), company->money >= target);
	REQUIRE(company->money >= target); REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
}

/** Verify both real paid native joined stops without changing road bits or authority. */
static void VerifySurfaceFarmRoadStops()
{
	// Each exact paid tile retains its existing road connection while joining
	// the actual original source/receiver inventory owner through native rules.
	for (const auto &[tile, station] : std::array<std::pair<TileIndex, StationID>, 2>{{{TileIndex{1372}, StationID{15}}, {TileIndex{2572}, StationID{13}}}}) {
		REQUIRE(IsStationRoadStopTile(tile)); REQUIRE(GetStationIndex(tile) == station); REQUIRE(GetRoadStopType(tile) == RoadStopType::Truck);
	}
	const auto component = SurfaceGoodsRoadComponent(TileIndex{2443}); REQUIRE(component.contains(TileIndex{1372})); REQUIRE(component.contains(TileIndex{2572}));
}

/** Freshly pay only the two fully admitted native joined truck stops. */
static void PaySurfaceFarmRoadStops()
{
	REQUIRE(FindSurfaceFarmRoadIncome() == nullptr); QuerySurfaceFarmRoad(); QuoteSurfaceFarmRoadWork("stops", Money{6042}, true);
	const auto *company = Company::Get(_current_company); const auto before = company->money;
	for (const auto &[tile, station] : std::array<std::pair<TileIndex, StationID>, 2>{{{TileIndex{1372}, StationID{15}}, {TileIndex{2572}, StationID{13}}}}) {
		const auto quote = QuoteSurfaceResupply<Commands::BuildRoadStop>(tile, tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck,
			true, DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, station, true);
		REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == Money{150});
		PaySurfaceResupply<Commands::BuildRoadStop>(tile, tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck,
			true, DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, station, true);
	}
	VerifySurfaceFarmRoadStops(); REQUIRE(company->money == before - Money{300});
	fmt::print("SURFACE_RESUPPLY_FARM_ROAD_STOPS_PAID source_tile=1372 source=15 receiver_tile=2572 receiver=13 invoice=300 cash={} elapsed=0 new_vehicle=0 changed_rating=0\n", company->money.base());
}

/** Verify the sole new carrier's ordinary Source7pickup and final receiver policy.
 * @return Actual paid new road carrier with native two-stop orders.
 */
static const RoadVehicle *VerifySurfaceFarmRoadIncome()
{
	const auto *truck = FindSurfaceFarmRoadIncome(); REQUIRE(truck != nullptr); REQUIRE(truck->GetNumOrders() == 2);
	for (uint8_t index = 0; index < 2; ++index) {
		const auto *order = truck->GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_STATION));
		REQUIRE(order->GetDestination() == DestinationID{static_cast<uint16_t>(index == 0 ? 15 : 13)});
		REQUIRE(order->GetLoadType() == (index == 0 ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad));
		REQUIRE(order->GetUnloadType() == (index == 0 ? OrderUnloadType::NoUnload : OrderUnloadType::UnloadIfPossible));
	}
	return truck;
}

/** Buy the one actual native truck, then name/order/start only its returned identity. */
static void BuySurfaceFarmRoadIncome()
{
	REQUIRE(FindSurfaceFarmRoadIncome() == nullptr); VerifySurfaceFarmRoadStops(); REQUIRE(RoadVehicle::CanAllocateItem(1));
	QuoteSurfaceFarmRoadWork("vehicle", Money{5742}, true); const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"});
	const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, EngineID{137}, false, livestock, ClientID::Server);
	REQUIRE(std::get<0>(quote).Succeeded()); REQUIRE(std::get<0>(quote).GetCost() == Money{5742}); REQUIRE(std::get<2>(quote) == 18);
	const auto *company = Company::Get(_current_company); const auto before = company->money;
	const auto bought = PaySurfaceResupply<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, EngineID{137}, false, livestock, ClientID::Server);
	auto *truck = RoadVehicle::Get(std::get<1>(bought)); REQUIRE(truck->cargo_type == livestock); REQUIRE(truck->cargo_cap == 18); REQUIRE(truck->cargo.TotalCount() == 0);
	PaySurfaceResupply<Commands::RenameVehicle>(TileIndex{0}, truck->index, std::string{"Surface Farm livestock income"});
	// Source and receiver share existing real station inventories. NoLoad at
	// the factory prevents another industry's cargo from entering this cohort.
	for (uint8_t index = 0; index < 2; ++index) {
		Order order; order.MakeGoToStation(StationID{static_cast<uint16_t>(index == 0 ? 15 : 13)});
		order.SetLoadType(index == 0 ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad);
		order.SetUnloadType(index == 0 ? OrderUnloadType::NoUnload : OrderUnloadType::UnloadIfPossible); order.SetStopLocation(OrderStopLocation::FarEnd);
		PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{index}, order);
	}
	VerifySurfaceFarmRoadIncome(); PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
	REQUIRE(!truck->vehstatus.Test(VehState::Stopped)); REQUIRE(company->money == before - Money{5742});
	fmt::print("SURFACE_RESUPPLY_FARM_ROAD_TRUCK_PAID vehicle={} engine=137 capacity=18 invoice=5742 cash={} elapsed=0 original_services_changed=0\n", truck->index.base(), company->money.base());
}

/** Measure real native pickup/delivery/profit and company cash within the freshly paid2000cap. */
static void WorkSurfaceFarmRoadIncome()
{
	VerifySurfaceFarmRoadStops(); const auto *truck = VerifySurfaceFarmRoadIncome(); REQUIRE(!truck->vehstatus.Test(VehState::Stopped));
	QuoteSurfaceFarmRoadWork("service", Money{0}, false); const auto *company = Company::Get(_current_company);
	const auto profit = truck->profit_this_year; const auto year = TimerGameEconomy::year; bool loaded = false, delivered = false;
	/** @return Whether actual paid whole-company cash has restored the working reserve. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", 2000, [&] {
		REQUIRE(company->money >= Money{40000}); if (truck->cargo.TotalCount() > 0) loaded = true;
		// Only native Source7pickup followed by actual factory-side emptying
		// proves this new body's delivery; empty unused capacity proves none.
		if (loaded && truck->last_station_visited == StationID{13} && truck->cargo.TotalCount() == 0) delivered = true;
		return company->money >= Money{100000};
	});
	auto delta = truck->profit_this_year - profit;
	// Preserve both actual sides of native yearly rollover instead of treating
	// reset profit_this_year as lifetime route income or a fabricated loss.
	if (TimerGameEconomy::year != year) delta = truck->profit_last_year - profit + truck->profit_this_year;
	VerifySurfaceFarmRoadIncome();
	fmt::print("SURFACE_RESUPPLY_FARM_ROAD_SERVICE ticks={} vehicle={} picked_up={} delivered={} held={} profit_delta_256={} cash={} restored={} partial={} annual_projection=false module_capital_complete=false\n",
		receipt.ticks, truck->index.base(), loaded, delivered, truck->cargo.TotalCount(), delta.base(), company->money.base(), receipt.reached, !receipt.reached);
}
#endif /* TEST_SURFACE_ENTRY_FARM_ROAD_WORK_H */
