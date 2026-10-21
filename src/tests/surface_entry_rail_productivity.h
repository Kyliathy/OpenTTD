/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_rail_productivity.h Pure original consist/refit/connectivity admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZV.
 * Native trains, packets, command estimates and track followers retain all
 * authority; no existing cargo, coupling, route, clock or money is changed.
 */
#ifndef TEST_SURFACE_ENTRY_RAIL_PRODUCTIVITY_H
#define TEST_SURFACE_ENTRY_RAIL_PRODUCTIVITY_H

/** Inspect actual freight capacity and legal refit/depot prerequisites without a simulation tick. */
static void QuerySurfaceRailProductivity()
{
	VerifySurfaceGoodsRailDelivery(true);
	const auto *company = Company::Get(_current_company);
	REQUIRE(company->money == Money{45080}); REQUIRE(company->current_loan == Money{5000000});
	const auto bill = QuoteSurfaceFixedPriceIncome(500);
	const auto operating = bill.upkeep + bill.interest + bill.station;
	fmt::print("SURFACE_RESUPPLY_RAIL_PRODUCTIVITY_SOURCE cash={} debt={} operating={} reserve=40000 target={} funded={} paid=0 elapsed=0\n",
		company->money.base(), company->current_loan.base(), operating.base(), (Money{40000}+operating).base(), company->money >= Money{40000}+operating);
	// Every coupled part remains in its actual original train. Report complete
	// real capacity and freight, then inspect only native GUI refit estimates;
	// a positive GUI capacity cannot bypass later real depot/cargo admission.
	for (const VehicleID id : {VehicleID{42}, VehicleID{49}, VehicleID{52}}) {
		const auto *train = Train::Get(id); REQUIRE(train->IsFrontEngine()); REQUIRE(train->owner == _current_company);
		uint parts = 0, capacity = 0, held = 0;
		for (const auto *part = train; part != nullptr; part = part->Next()) {
			++parts; capacity += part->cargo_cap; held += part->cargo.TotalCount();
			fmt::print("SURFACE_RESUPPLY_RAIL_PRODUCTIVITY_PART head={} vehicle={} engine={} cargo={} capacity={} held={} tile={}\n",
				id.base(), part->index.base(), part->engine_type.base(), part->cargo_type, part->cargo_cap, part->cargo.TotalCount(), part->tile.base());
			if (part->cargo_cap == 0) continue;
			for (const auto label : {CargoLabel{"GOOD"}, CargoLabel{"STEL"}}) {
				const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo));
				const auto gui = CmdRefitVehicle(DoCommandFlag::QueryCost, part->index, cargo, uint8_t{0}, false, true, uint8_t{1});
				const auto ordinary = QuoteSurfaceResupply<Commands::RefitVehicle>(part->tile, part->index, cargo, uint8_t{0}, false, true, uint8_t{1});
				const bool supported = std::get<0>(gui).Succeeded() && std::get<1>(gui) > 0;
				fmt::print("SURFACE_RESUPPLY_RAIL_PRODUCTIVITY_REFIT head={} vehicle={} original_cargo={} requested_cargo={} held={} gui_available={} capacity={} supported={} invoice={} ordinary_available={} actual_depot={} work_permission=false\n",
					id.base(), part->index.base(), part->cargo_type, cargo, part->cargo.TotalCount(), std::get<0>(gui).Succeeded(), std::get<1>(gui), supported,
					std::get<0>(gui).GetCost().base(), std::get<0>(ordinary).Succeeded(), train->IsStoppedInDepot());
			}
		}
		fmt::print("SURFACE_RESUPPLY_RAIL_PRODUCTIVITY_TRAIN head={} parts={} capacity={} held={} remaining={} orders={} speed={}\n",
			id.base(), parts, capacity, held, train->max_age.base()-train->age.base(), train->GetNumOrders(), train->cur_speed);
		// Physical connectivity follows only actual owned native rail edges for
		// this exact locomotive. Parallel source platforms cannot count as joined
		// routes merely because both stations have uncollected useful cargo.
		for (const auto *depot : Depot::Iterate()) {
			if (!IsRailDepotTile(depot->xy)) continue;
			if (GetTileOwner(depot->xy) != _current_company) continue;
			for (const StationID station_id : {StationID{11}, StationID{12}, StationID{13}, StationID{15}}) {
				const auto *station = Station::Get(station_id);
				const bool connected = SurfaceResupplyRailConnected(depot->xy, station->train_station.tile, train->compatible_railtypes);
				fmt::print("SURFACE_RESUPPLY_RAIL_PRODUCTIVITY_EDGE head={} depot={} station={} tile={} connected={}\n",
					id.base(), depot->xy.base(), station_id.base(), station->train_station.tile.base(), connected);
			}
		}
	}
	// If an existing body cannot refit to Goods, actual new wagons are a
	// separate capital proposal. Price only the real current Goods model at
	// each owned rail depot; parked original wagons supply no refund or cash.
	const auto *goods_wagon = Train::Get(VehicleID{49})->Next(); REQUIRE(goods_wagon != nullptr);
	REQUIRE(goods_wagon->cargo_type == GetCargoTypeByLabel(CargoLabel{"GOOD"}));
	const auto *engine = goods_wagon->GetEngine(); REQUIRE(engine->GetGRF() == nullptr);
	const auto annual = engine->GetRunningCost().base(); REQUIRE(annual >= 0);
	constexpr int64_t denominator = CalendarTime::DAYS_IN_YEAR * Ticks::DAY_TICKS;
	const Money extra_upkeep{(annual * 500 + denominator - 1) / denominator + 2};
	for (const auto *depot : Depot::Iterate()) {
		if (!IsRailDepotTile(depot->xy)) continue;
		if (GetTileOwner(depot->xy) != _current_company) continue;
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(depot->xy, depot->xy, engine->index, false,
			GetCargoTypeByLabel(CargoLabel{"GOOD"}), ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		// Every lot includes its gross purchase and all proposed wagon upkeep
		// plus the unchanged company's entire first interval. Later real empty
		// arrival, native coupling and useful mixed capacity need their own gate.
		for (uint count = 1; count <= 6; ++count) {
			const auto invoice = std::get<0>(quote).GetCost() * count;
			const auto added = extra_upkeep * count;
			const auto target = Money{40000} + operating + invoice + added;
			fmt::print("SURFACE_RESUPPLY_RAIL_PRODUCTIVITY_WAGONS depot={} engine={} count={} capacity={} invoice={} added_upkeep={} existing_operating={} reserve=40000 target={} cash={} funded={} future_income_counted=0 refund_counted=0 work_permission=false\n",
				depot->xy.base(), engine->index.base(), count, std::get<2>(quote)*count, invoice.base(), added.base(), operating.base(), target.base(),
				company->money.base(), company->money >= target);
		}
	}
}

/** Preserve the exact original world while querying native rail productivity prerequisites. */
TEST_CASE("Current original freight trains expose only supported funded productivity prerequisites", "[.surface-rail-productivity]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-rail-productivity");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Inspect actual native cargo/track owners and preserve any query failure through normal shutdown. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves / "surface-resupply-before-rail-productivity-query-440.sav");
			QuerySurfaceRailProductivity();
			SaveSurfaceResupply(paths.saves / "surface-resupply-rail-productivity-query-440.sav");
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase=rail-productivity-query supplied_stock=false supplied_work=false\n"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect);
	IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
#endif /* TEST_SURFACE_ENTRY_RAIL_PRODUCTIVITY_H */
