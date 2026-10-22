/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_mixed_farm_backhaul.h Actual paid two-way farm/Goods service.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVD.
 * Public commands own every purchase, move, order and restart. Normal native
 * ticks alone own transport, final acceptance and posted CargoPayment.
 */
#ifndef TEST_SURFACE_ENTRY_MIXED_FARM_BACKHAUL_H
#define TEST_SURFACE_ENTRY_MIXED_FARM_BACKHAUL_H

/** Verify the real mixed consist and its separately retained empty reserves.
 * @return Original powered52head with actual two paid Goods tails.
 */
static const Train *VerifySurfaceFarmBackhaul()
{
	const auto *train = Train::Get(VehicleID{52}); REQUIRE(train->IsFrontEngine());
	const std::array<uint16_t, 12> prefix{52, 12, 13, 14, 15, 16, 17, 21, 22, 23, 50, 57};
	const Train *part = train;
	// Preserve all original active roles and the first paid Livestock tail;
	// two additional paid Livestock bodies remain separately owned in reserve.
	for (const auto id : prefix) {
		REQUIRE(part != nullptr); REQUIRE(part->index == VehicleID{id}); REQUIRE(part->owner == _current_company);
		REQUIRE(part->gcache.cached_veh_length == 8);
		if (id == 52) { REQUIRE(part->engine_type == EngineID{24}); REQUIRE(part->cargo_cap == 0); }
		else if (id >= 21 && id <= 23) {
			// Original Grain capacity stays active for real Farm7delivery to13.
			REQUIRE(part->engine_type == EngineID{33}); REQUIRE(part->cargo_cap == 30); REQUIRE(part->cargo_type == GetCargoTypeByLabel(CargoLabel{"GRAI"}));
		} else {
			REQUIRE(part->engine_type == EngineID{31}); REQUIRE(part->cargo_cap == 25); REQUIRE(part->cargo_type == GetCargoTypeByLabel(CargoLabel{"LVST"}));
		}
		part = part->Next();
	}
	// Actual returned BuildVehicle IDs define these two bodies. Native links
	// and cargo model checks alone grant no payment provenance to a future ID.
	for (uint index = 0; index < 2; ++index) {
		REQUIRE(part != nullptr); REQUIRE(part->owner == _current_company); REQUIRE(part->engine_type == EngineID{32});
		REQUIRE(part->cargo_type == GetCargoTypeByLabel(CargoLabel{"GOOD"})); REQUIRE(part->cargo_cap == 25);
		REQUIRE(part->gcache.cached_veh_length == 8); part = part->Next();
	}
	REQUIRE(part == nullptr); REQUIRE(train->gcache.cached_total_length == 112); REQUIRE(_settings_game.vehicle.max_train_length == 7);
	// Both separately owned depot chains remain physically empty and exact;
	// ownership does not make their parked capacity active or spendable.
	for (const uint16_t head : {18, 58}) {
		part = Train::Get(VehicleID{head}); REQUIRE(part->IsFreeWagon()); REQUIRE(part->IsStoppedInDepot());
		const uint count = head == 18 ? 3 : 2;
		for (uint index = 0; index < count; ++index) {
			REQUIRE(part != nullptr); REQUIRE(part->index.base() == head+index); REQUIRE(part->owner == _current_company);
			REQUIRE(part->tile == TileIndex{1638}); REQUIRE(part->cargo.TotalCount() == 0);
			REQUIRE(part->engine_type == EngineID{static_cast<uint16_t>(head == 18 ? 33 : 31)}); part = part->Next();
		}
		REQUIRE(part == nullptr);
	}
	REQUIRE(train->GetNumOrders() == 2);
	// Normal loading and unloading at both existing calls enables the Goods
	// reverse leg while retaining native crop pickup and factory delivery.
	for (uint8_t index = 0; index < 2; ++index) {
		const auto *order = train->GetOrder(index); REQUIRE(order->IsType(OT_GOTO_STATION));
		REQUIRE(order->GetDestination() == DestinationID{static_cast<uint16_t>(index == 0 ? 15 : 13)});
		REQUIRE(order->GetLoadType() == OrderLoadType::LoadIfPossible); REQUIRE(order->GetUnloadType() == OrderUnloadType::UnloadIfPossible);
	}
	REQUIRE(!train->vehstatus.Test(VehState::Stopped)); return train;
}

/** Price the complete purchase and reactivation before any real command.
 * @return Longest fully funded candidate interval; zero leaves the world unchanged.
 */
static uint QuerySurfaceFarmBackhaul()
{
	const auto *train = VerifySurfaceMixedFarmReturn(true); REQUIRE(SurfaceMixedFarmAtDepot());
	const auto *company = Company::Get(_current_company); REQUIRE(company->current_loan == Money{5000000});
	const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	const auto *known = Train::Get(VehicleID{26}); REQUIRE(known->engine_type == EngineID{32}); REQUIRE(known->gcache.cached_veh_length == 8);
	// Callback-free, zero-rate plain wagons cannot change the locomotive's
	// annual charge when exchanged. New bodies still receive upward margins.
	for (const EngineID id : {EngineID{31}, EngineID{32}}) {
		const auto *engine = Engine::Get(id); REQUIRE(engine->GetGRF() == nullptr); REQUIRE(engine->GetRunningCost() == Money{0});
	}
	REQUIRE(train->GetEngine()->GetGRF() == nullptr); REQUIRE(Train::CanAllocateItem(2));
	const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(TileIndex{1638}, TileIndex{1638}, EngineID{32}, false, goods, ClientID::Server);
	REQUIRE(std::get<0>(quote).Succeeded()); REQUIRE(std::get<0>(quote).GetCost() == Money{1113}); REQUIRE(std::get<2>(quote) == 25);
	const auto park = QuoteSurfaceResupply<Commands::MoveRailVehicle>(TileIndex{1638}, VehicleID{58}, VehicleID::Invalid(), true);
	REQUIRE(park.Succeeded()); REQUIRE(park.GetCost() == Money{0});
	const auto remove = QuoteSurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, train->index, VehicleOrderID{2});
	const auto restart = QuoteSurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, train->index, true);
	REQUIRE(remove.Succeeded()); REQUIRE(remove.GetCost() == Money{0}); REQUIRE(restart.Succeeded()); REQUIRE(restart.GetCost() == Money{0});
	// Query every ordinary policy change on the actual existing orders. No
	// temporary body or invented pool ID is needed to estimate a future move.
	for (uint8_t index = 0; index < 2; ++index) {
		for (const auto flag : {MOF_LOAD, MOF_UNLOAD}) {
			const auto data = static_cast<uint16_t>(flag == MOF_LOAD ? to_underlying(OrderLoadType::LoadIfPossible) : to_underlying(OrderUnloadType::UnloadIfPossible));
			const auto *order = train->GetOrder(index);
			const auto current = flag == MOF_LOAD ? to_underlying(order->GetLoadType()) : to_underlying(order->GetUnloadType());
			// Native ModifyOrder deliberately refuses an unchanged flag. Existing
			// normal factory unloading needs no command or permission to change.
			if (current == data) continue;
			const auto change = QuoteSurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, train->index, VehicleOrderID{index}, flag, data);
			REQUIRE(change.Succeeded()); REQUIRE(change.GetCost() == Money{0});
		}
		const auto *station = Station::Get(StationID{static_cast<uint16_t>(index == 0 ? 15 : 13)});
		REQUIRE(SurfaceResupplyRailConnected(TileIndex{1638}, station->train_station.tile, train->compatible_railtypes));
	}
	REQUIRE(Station::Get(StationID{15})->goods[goods].status.Test(GoodsEntry::State::Acceptance));
	const auto waiting = Station::Get(StationID{13})->goods[goods].AvailableCount(); REQUIRE(waiting > 0);
	uint selected = 0;
	// The ordinary full-cost owner omits stopped ground heads. Add the actual
	// proposed52reactivation, including its already pending running ticks, to
	// every candidate. Future Goods fares and reserve assets fund nothing here.
	for (const uint limit : {1000, 500, 250, 64}) {
		const auto bill = QuoteSurfaceFixedPriceIncome(limit);
		const int lead = company->settings.engine_renew ? std::max(0, -int{company->settings.engine_renew_months} * 30) : 0;
		REQUIRE(train->max_age.base()-train->age.base() > static_cast<int>(bill.days)+lead);
		constexpr uint64_t denominator = uint64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS * 256;
		const auto annual = train->GetRunningCost().base(); REQUIRE(annual >= 0);
		const Money reactivation{static_cast<int64_t>((static_cast<uint64_t>(annual)*(limit+train->running_ticks)+denominator-1)/denominator)+1};
		const auto operating = bill.upkeep+bill.interest+bill.station+reactivation+Money{4};
		const auto target = operating+Money{2226}+Money{40000};
		const bool funded = company->money >= target;
		fmt::print("SURFACE_RESUPPLY_FARM_BACKHAUL_ADMISSION ticks={} existing={} reactivation={} pending={} added=4 operating={} invoice=2226 reserve=40000 target={} cash={} funded={} future_income_counted=0 refund_counted=0\n",
			limit, (bill.upkeep+bill.interest+bill.station).base(), reactivation.base(), train->running_ticks, operating.base(), target.base(), company->money.base(), funded);
		if (funded) { selected = limit; break; }
	}
	fmt::print("SURFACE_RESUPPLY_FARM_BACKHAUL_PLAN head=52 depot=1638 waiting={} receiver=15 goods_capacity=50 livestock_capacity=200 grain_capacity=90 length=112 parked_livestock=50 invoice=2226 selected={} funded={} paid=0 elapsed=0\n", waiting, selected, selected > 0);
	return selected;
}

/** Pay only the freshly admitted actual two-wagon invoice and native changes. */
static void PaySurfaceFarmBackhaul()
{
	REQUIRE(QuerySurfaceFarmBackhaul() > 0); const auto cash = Company::Get(_current_company)->money;
	const auto tick = TimerGameTick::counter;
	PaySurfaceResupply<Commands::MoveRailVehicle>(TileIndex{1638}, VehicleID{58}, VehicleID::Invalid(), true);
	VehicleID tail{57}; const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	// Each real paid return ID is coupled immediately after the actual retained
	// tail. A fresh native move estimate is required after purchase, when that
	// body truly exists; no synthetic query world or rollback supplies capacity.
	for (uint index = 0; index < 2; ++index) {
		const auto bought = PaySurfaceResupply<Commands::BuildVehicle>(TileIndex{1638}, TileIndex{1638}, EngineID{32}, false, goods, ClientID::Server);
		REQUIRE(std::get<0>(bought).GetCost() == Money{1113}); const auto *wagon = Train::Get(std::get<1>(bought));
		REQUIRE(wagon->cargo_cap == 25); REQUIRE(wagon->cargo.TotalCount() == 0);
		if (wagon->First()->index != VehicleID{52}) {
			const auto move = QuoteSurfaceResupply<Commands::MoveRailVehicle>(TileIndex{1638}, wagon->index, tail, false);
			REQUIRE(move.Succeeded()); REQUIRE(move.GetCost() == Money{0});
			PaySurfaceResupply<Commands::MoveRailVehicle>(TileIndex{1638}, wagon->index, tail, false);
		}
		REQUIRE(Train::Get(tail)->Next() == wagon); tail = wagon->index;
		fmt::print("SURFACE_RESUPPLY_FARM_BACKHAUL_WAGON position={} wagon={} invoice=1113 capacity=25\n", index, tail.base());
	}
	// Restore only normal public two-way station policy; removing the temporary
	// depot halt lets native order progression resume the existing farm route.
	for (uint8_t index = 0; index < 2; ++index) {
		PaySurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, VehicleID{52}, VehicleOrderID{index}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::LoadIfPossible));
		// Factory13already unloads normally; only15needs its old NoUnload flag removed.
		if (index == 0) PaySurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, VehicleID{52}, VehicleOrderID{index}, MOF_UNLOAD, static_cast<uint16_t>(OrderUnloadType::UnloadIfPossible));
	}
	PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, VehicleID{52}, VehicleOrderID{2});
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, VehicleID{52}, true);
	VerifySurfaceFarmBackhaul(); REQUIRE(SurfaceMixedFarmHeld(Train::Get(VehicleID{52})) == 0);
	REQUIRE(TimerGameTick::counter == tick); REQUIRE(Company::Get(_current_company)->money == cash-Money{2226});
	fmt::print("SURFACE_RESUPPLY_FARM_BACKHAUL_PAID invoice=2226 elapsed=0 cash={} original_wagons_owned=32 goods_capacity=50 livestock_capacity=200 grain_capacity=90 length=112 reserve=40000\n", Company::Get(_current_company)->money.base());
}

/** Count only actual Goods on the farm train's real two new tails.
 * @param train Verified actual mixed powered head.
 * @return Actual native first13Goods quantity, never station waiting stock.
 */
static uint SurfaceFarmBackhaulGoods(const Train *train)
{
	uint quantity = 0;
	// Other active farm freight remains native and cannot become Goods income.
	// First-station/source checks conserve this genuine reverse-leg shipment.
	for (const auto *part = train; part != nullptr; part = part->Next()) {
		if (part->cargo_type != GetCargoTypeByLabel(CargoLabel{"GOOD"})) continue;
		for (const auto *packet : *part->cargo.Packets()) {
			REQUIRE(packet->GetFirstStation() == StationID{13}); REQUIRE(packet->GetSource().type == SourceType::Industry);
			REQUIRE((packet->GetSource().id == 5 || packet->GetSource().id == 11)); quantity += packet->Count();
		}
	}
	return quantity;
}

/** Earn only a freshly funded real transport or previously qualified settlement interval.
 * @param settlement Only true after an actual earlier accepted-Goods receipt and positive pending payment.
 */
static void WorkSurfaceFarmBackhaul(bool settlement)
{
	const auto *train = VerifySurfaceFarmBackhaul(); const auto *company = Company::Get(_current_company);
	const auto initial = SurfaceFarmBackhaulGoods(train); Money fare{0};
	if (settlement) {
		REQUIRE(initial == 0); REQUIRE(train->last_station_visited == StationID{15}); REQUIRE(train->cargo_payment != nullptr);
		REQUIRE(train->cargo_payment->route_profit > Money{0}); fare = train->cargo_payment->route_profit;
	}
	const auto ceiling = SurfaceChemicalWorkLimit(); REQUIRE(ceiling <= 1000);
	const auto limit = QuoteSurfaceGoodsRailFeederWork(Money{0}, ceiling);
	fmt::print("SURFACE_RESUPPLY_FARM_BACKHAUL_SELECTION ceiling={} ticks={} found={} settlement={} initial={}\n", ceiling, limit, limit > 0, settlement, initial);
	if (limit == 0) return;
	uint previous = initial, boarded = 0, accepted = 0; bool posted = false;
	/** @return Whether an actual first Goods payment posted or real cash reached the working floor. */
	const auto done = [&] {
		REQUIRE(company->money >= Money{40000}); const auto current = SurfaceFarmBackhaulGoods(train);
		if (current > previous) { REQUIRE(train->last_station_visited == StationID{13}); boarded += current-previous; }
		if (current < previous) {
			REQUIRE(train->last_station_visited == StationID{15});
			REQUIRE(Station::Get(StationID{15})->goods[GetCargoTypeByLabel(CargoLabel{"GOOD"})].status.Test(GoodsEntry::State::Acceptance));
			accepted += previous-current;
		}
		previous = current;
		// Actual accepted Goods plus this receiver's positive CargoPayment
		// identify the unload. Other crop fares never qualify a Goods payment.
		if ((accepted > 0 || settlement) && train->cargo_payment != nullptr && train->last_station_visited == StationID{15}) fare = std::max(fare, train->cargo_payment->route_profit);
		posted = fare > Money{0} && current == 0 && train->cargo_payment == nullptr;
		return posted || company->money >= Money{100000};
	};
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, done); const auto final = SurfaceFarmBackhaulGoods(train);
	VerifySurfaceFarmBackhaul(); REQUIRE(initial+boarded == accepted+final);
	fmt::print("SURFACE_RESUPPLY_FARM_BACKHAUL_SERVICE ticks={} initial={} boarded={} accepted={} held={} observed_whole_unload_fare={} payment_pending={} payment_posted={} settlement={} cash={} restored={} reserve=40000\n",
		work.ticks, initial, boarded, accepted, final, fare.base(), train->cargo_payment != nullptr, posted, settlement, company->money.base(), company->money >= Money{100000});
}

/** Execute only the selected actual mixed-service boundary and retain any partial failure. */
TEST_CASE("Actual paid mixed farm service earns its genuine Goods backhaul", "[.surface-farm-backhaul]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_RESUPPLY_PHASE"); REQUIRE(selected != nullptr); const std::string phase{selected};
	const std::array<std::string_view, 4> allowed{"farm-backhaul-query", "farm-backhaul-pay", "farm-backhaul-work", "farm-backhaul-payment"};
	REQUIRE(std::ranges::find(allowed, phase) != allowed.end());
	const auto paths = GetTestCaseArtifactPaths("space", "surface-farm-backhaul"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve the precise original boundary or partial native failure without replacing resources. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves/("surface-resupply-before-"+phase+"-440.sav"));
			if (phase == "farm-backhaul-query") QuerySurfaceFarmBackhaul();
			else if (phase == "farm-backhaul-pay") PaySurfaceFarmBackhaul();
			else WorkSurfaceFarmBackhaul(phase == "farm-backhaul-payment");
			SaveSurfaceResupply(paths.saves/("surface-resupply-"+phase+"-440.sav"));
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) {
			failure = std::current_exception(); SaveSurfaceResupply(paths.saves/("surface-resupply-failed-"+phase+"-440.sav"));
		}
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
#endif /* TEST_SURFACE_ENTRY_MIXED_FARM_BACKHAUL_H */
