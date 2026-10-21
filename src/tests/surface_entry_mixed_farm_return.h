/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_mixed_farm_return.h Genuine loaded farm delivery and empty depot return.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CSGZVB/15CSGZVBC.
 * Public orders, original whole-company cost admission and native simulation
 * alone move cargo and money. No wagon is bought, detached, refitted or sold.
 */
#ifndef TEST_SURFACE_ENTRY_MIXED_FARM_RETURN_H
#define TEST_SURFACE_ENTRY_MIXED_FARM_RETURN_H

/** Verify unchanged actual farm capacity with either original or explicitly appended empty-return orders.
 * @param returning True requires the new no-pickup/depot-halt order contract; false retains the old strict producer.
 * @return Existing original head52 with all actual paid/original coupled bodies still present.
 */
static const Train *VerifySurfaceMixedFarmReturn(bool returning)
{
	if (!returning) return VerifySurfaceFarmRebalance(true);
	const auto *train = Train::Get(VehicleID{52}); REQUIRE(train->IsFrontEngine()); REQUIRE(train->owner == _current_company);
	const std::array<uint16_t, 14> ids{52, 12, 13, 14, 15, 16, 17, 21, 22, 23, 50, 57, 58, 59};
	const Train *part = train;
	// The return contract changes orders only. Every original/paid body keeps
	// its exact model, cargo and capacity until real delivery and depot arrival.
	for (const auto id : ids) {
		REQUIRE(part != nullptr); REQUIRE(part->index == VehicleID{id}); REQUIRE(part->owner == _current_company);
		if (id == 52) { REQUIRE(part->engine_type == EngineID{24}); REQUIRE(part->cargo_cap == 0); }
		else if (id >= 21 && id <= 23) {
			// Original Grain service remains physically attached while returning;
			// its loaded packets cannot be traded for prospective Goods capacity.
			REQUIRE(part->engine_type == EngineID{33}); REQUIRE(part->cargo_type == GetCargoTypeByLabel(CargoLabel{"GRAI"})); REQUIRE(part->cargo_cap == 30);
		} else {
			REQUIRE(part->engine_type == EngineID{31}); REQUIRE(part->cargo_type == GetCargoTypeByLabel(CargoLabel{"LVST"})); REQUIRE(part->cargo_cap == 25);
		}
		part = part->Next();
	}
	REQUIRE(part == nullptr); REQUIRE(train->gcache.cached_total_length == 112); REQUIRE(_settings_game.vehicle.max_train_length == 7);
	REQUIRE(train->GetNumOrders() == 3);
	// Both real station calls remain in sequence. NoLoad prevents another crop
	// pickup, while the current13receiver still performs normal final delivery.
	for (uint8_t index = 0; index < 2; ++index) {
		const auto *order = train->GetOrder(index); REQUIRE(order->IsType(OT_GOTO_STATION));
		REQUIRE(order->GetDestination() == DestinationID{static_cast<uint16_t>(index == 0 ? 15 : 13)});
		REQUIRE(order->GetLoadType() == OrderLoadType::NoLoad);
		REQUIRE(order->GetUnloadType() == (index == 0 ? OrderUnloadType::NoUnload : OrderUnloadType::UnloadIfPossible));
	}
	const auto *depot = Depot::GetByTile(TileIndex{1638}); const auto *halt = train->GetOrder(2);
	REQUIRE(halt->IsType(OT_GOTO_DEPOT)); REQUIRE(halt->GetDestination().base() == depot->index.base());
	REQUIRE(halt->GetDepotActionType().Test(OrderDepotActionFlag::Halt));
	return train;
}

/** Read the actual entire farm train's physical freight without counting packet shares as money.
 * @param train Exact original powered farm head.
 * @return Whole currently held Livestock and Grain units across every actual coupled part.
 */
static uint SurfaceMixedFarmHeld(const Train *train)
{
	uint held = 0;
	// Cargo remains solely in native packets on the whole real consist; an
	// empty locomotive cannot stand in for delivery of loaded trailing wagons.
	for (const auto *part = train; part != nullptr; part = part->Next()) held += part->cargo.TotalCount();
	return held;
}

/** @return Actual native no-pickup return has halted the entire empty train at its owned1638depot. */
static bool SurfaceMixedFarmAtDepot()
{
	const auto *train = Train::Get(VehicleID{52});
	return train->IsStoppedInDepot() && train->tile == TileIndex{1638} && SurfaceMixedFarmHeld(train) == 0 && train->cargo_payment == nullptr;
}

/** Quote only the actual zero-cost public return orders and fully protected prospective invoice.
 * @return Newly affordable complete native interval, or zero on an unchanged funding refusal.
 */
static uint QuerySurfaceMixedFarmReturn()
{
	const auto *train = VerifySurfaceMixedFarmReturn(false);
	REQUIRE(train->cur_real_order_index == 1); REQUIRE(train->last_station_visited == StationID{15});
	REQUIRE(SurfaceMixedFarmHeld(train) == 214); REQUIRE(train->cargo_payment == nullptr);
	const auto *depot = Depot::GetByTile(TileIndex{1638}); REQUIRE(depot != nullptr);
	REQUIRE(SurfaceResupplyRailConnected(depot->xy, Station::Get(StationID{13})->train_station.tile, train->compatible_railtypes));
	Order halt; halt.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
	const auto pickup = QuoteSurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, train->index, VehicleOrderID{0}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::NoLoad));
	const auto home = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, halt);
	REQUIRE(pickup.Succeeded()); REQUIRE(pickup.GetCost() == Money{0}); REQUIRE(home.Succeeded()); REQUIRE(home.GetCost() == Money{0});
	const auto limit = QuoteSurfaceGoodsRailFeederWork(Money{3339}, 500);
	fmt::print("SURFACE_RESUPPLY_FARM_RETURN_PLAN head=52 held=214 last=15 receiver=13 depot=1638 invoice=0 unpaid_wagons=3339 reserve=40000 ceiling=500 selected={} funded={} paid=0 elapsed=0 future_income_counted=0\n", limit, limit > 0);
	return limit;
}

/** Append only admitted ordinary orders while retaining every actual loaded crop and native position. */
static void OrderSurfaceMixedFarmReturn()
{
	REQUIRE(QuerySurfaceMixedFarmReturn() > 0);
	const auto *train = Train::Get(VehicleID{52}); const auto tile = train->tile; const auto held = SurfaceMixedFarmHeld(train);
	const auto cash = Company::Get(_current_company)->money;
	PaySurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, train->index, VehicleOrderID{0}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::NoLoad));
	const auto *depot = Depot::GetByTile(TileIndex{1638});
	Order halt; halt.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, halt);
	VerifySurfaceMixedFarmReturn(true); REQUIRE(train->tile == tile); REQUIRE(SurfaceMixedFarmHeld(train) == held);
	REQUIRE(train->cur_real_order_index == 1); REQUIRE(Company::Get(_current_company)->money == cash);
	fmt::print("SURFACE_RESUPPLY_FARM_RETURN_ORDERED head=52 held={} current=1 receiver=13 depot=1638 orders=3 invoice=0 elapsed=0\n", held);
}

/** Earn only a newly and fully funded actual delivery/depot interval, preserving its whole prospective invoice.
 * @param unpaid Historical3,339default;2,226onlyfor the separately selected smaller initial-batch producer.
 */
static void WorkSurfaceMixedFarmReturn(Money unpaid = Money{3339})
{
	REQUIRE((unpaid == Money{3339} || unpaid == Money{2226}));
	const auto *train = VerifySurfaceMixedFarmReturn(true); const auto initial = SurfaceMixedFarmHeld(train);
	REQUIRE(!SurfaceMixedFarmAtDepot());
	const auto ceiling = SurfaceChemicalWorkLimit(); REQUIRE(ceiling <= 500);
	const auto limit = QuoteSurfaceGoodsRailFeederWork(unpaid, ceiling);
	fmt::print("SURFACE_RESUPPLY_FARM_RETURN_SELECTION ceiling={} ticks={} found={} initial={}\n", ceiling, limit, limit > 0, initial);
	if (limit == 0) return;
	uint previous = initial, accepted = 0;
	/** @return Whether all original crops genuinely delivered and the real empty train halted at its owned depot. */
	const auto done = [&] {
		REQUIRE(Company::Get(_current_company)->money >= Money{40000}+unpaid);
		const auto current = SurfaceMixedFarmHeld(train); REQUIRE(current <= previous);
		if (current < previous) {
			// NoLoad at both calls means crop loss is legal only as actual final
			// accepting13unload; the prospective Goods purchase grants no stock.
			REQUIRE(train->last_station_visited == StationID{13}); accepted += previous-current;
		}
		previous = current; return SurfaceMixedFarmAtDepot();
	};
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, done);
	VerifySurfaceMixedFarmReturn(true); const auto final = SurfaceMixedFarmHeld(train);
	REQUIRE(initial == accepted+final);
	fmt::print("SURFACE_RESUPPLY_FARM_RETURN_SERVICE ticks={} initial={} accepted={} held={} depot_halt={} pending_payment={} unpaid_wagons={} reserve=40000 cash={}\n",
		work.ticks, initial, accepted, final, SurfaceMixedFarmAtDepot(), train->cargo_payment != nullptr, unpaid.base(), Company::Get(_current_company)->money.base());
}

/** Execute one explicitly selected actual farm-return query, order boundary or freshly funded work interval. */
TEST_CASE("Actual loaded farm freight returns empty only through funded original orders", "[.surface-farm-return]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_RESUPPLY_PHASE"); REQUIRE(selected != nullptr); const std::string phase{selected};
	const std::array<std::string_view, 5> allowed{"farm-return-query", "farm-return-order", "farm-return-work", "farm-two-return-query", "farm-two-return-work"};
	REQUIRE(std::ranges::find(allowed, phase) != allowed.end());
	const auto paths = GetTestCaseArtifactPaths("space", "surface-farm-return"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve the precise original boundary or any partial native failure without compensating resources or work. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves / ("surface-resupply-before-"+phase+"-440.sav"));
			if (phase == "farm-return-query") QuerySurfaceMixedFarmReturn();
			else if (phase == "farm-return-order") OrderSurfaceMixedFarmReturn();
			else if (phase == "farm-two-return-query") {
				// The previous three-wagon capital proposal still refuses under
				// its exact unchanged default. Only a smaller uncommitted batch
				// funds this new producer; no reserve or clock is changed here.
				const auto tick = TimerGameTick::counter;
				VerifySurfaceMixedFarmReturn(true); REQUIRE(!SurfaceMixedFarmAtDepot());
				const auto old_default = QuoteSurfaceGoodsRailFeederWork(Money{3339}, 500); REQUIRE(old_default == 0);
				const auto selected = QuoteSurfaceGoodsRailFeederWork(Money{2226}, 500); REQUIRE(selected > 0);
				REQUIRE(TimerGameTick::counter == tick);
				fmt::print("SURFACE_RESUPPLY_FARM_TWO_RETURN_PLAN old_default=3339 old_default_earned=0 unpaid_wagons=2226 reserve=40000 ceiling=500 selected={} funded=true paid=0 elapsed=0\n", selected);
			}
			else if (phase == "farm-two-return-work") WorkSurfaceMixedFarmReturn(Money{2226});
			else WorkSurfaceMixedFarmReturn();
			SaveSurfaceResupply(paths.saves / ("surface-resupply-"+phase+"-440.sav"));
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) {
			failure = std::current_exception();
			SaveSurfaceResupply(paths.saves / ("surface-resupply-failed-"+phase+"-440.sav"));
		}
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
#endif /* TEST_SURFACE_ENTRY_MIXED_FARM_RETURN_H */
