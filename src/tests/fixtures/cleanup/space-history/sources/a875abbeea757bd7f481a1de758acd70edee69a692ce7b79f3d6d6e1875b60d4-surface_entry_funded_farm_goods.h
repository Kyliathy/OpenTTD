/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_funded_farm_goods.h Genuine Goods service after actual idle-capital retirement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEE.
 * Native packets, normal transport, full company costs and posted payment own
 * every result. Old mixed-service worker/verifier defaults remain unchanged.
 */
#ifndef TEST_SURFACE_ENTRY_FUNDED_FARM_GOODS_H
#define TEST_SURFACE_ENTRY_FUNDED_FARM_GOODS_H

/** Verify only the genuine post-retirement world and real mixed active service.
 * @return Actual original52head with its paid Goods39/60and retained original capacity.
 */
static const Train *VerifySurfaceFundedFarmGoods()
{
	const auto *train = VerifySurfaceFarmBackhaul(true); REQUIRE(ResolveSpaceCraft({VehicleID{38}, uint64_t{2}}) == nullptr);
	uint space = 0;
	// Actual native retirement leaves only the stored rocket. No substitute
	// builder or unpriced orbital participant can appear in the funded trial.
	for (const auto *body : Vehicle::Iterate()) {
		if (body->type != VehicleType::Space) continue;
		++space; REQUIRE(body->owner == _current_company); REQUIRE(body->index == VehicleID{54});
		const auto *rocket = SpaceVehicle::From(body); REQUIRE(rocket->generation == 1);
		REQUIRE(rocket->construction.placement == SpacePlacement::SurfaceStored); REQUIRE(rocket->GetRunningCost() == Money{0});
		REQUIRE(rocket->space_orders.cursor == rocket->space_orders.entries.size());
	}
	REQUIRE(space == 1); return train;
}

/** Query only a fresh fully funded interval from the actual current post-sale original. */
static void QuerySurfaceFundedFarmGoods()
{
	const auto *train = VerifySurfaceFundedFarmGoods(); const auto cash = Company::Get(_current_company)->money;
	const auto limit = QuoteSurfaceGoodsRailFeederWork(Money{0}, 2000);
	fmt::print("SURFACE_RESUPPLY_FUNDED_FARM_GOODS_PLAN head=52 initial={} cash={} selected={} funded={} reserve=40000 paid=0 elapsed=0 future_income_counted=0\n",
		SurfaceFarmBackhaulGoods(train), cash.base(), limit, limit > 0);
}

/** Earn one fresh complete transport or previously qualified pending-payment interval.
 * @param settlement Only true after actual earlier Goods acceptance/empty Goods/positive native52payment.
 * @param stop_on_payment Historical true stops at the first Goods payment; false only in Group15CSGZVEEF's separate actual-cash scope.
 */
static void WorkSurfaceFundedFarmGoods(bool settlement, bool stop_on_payment = true)
{
	const auto *train = VerifySurfaceFundedFarmGoods(); const auto *company = Company::Get(_current_company);
	const auto initial = SurfaceFarmBackhaulGoods(train); Money fare{0};
	if (settlement) {
		REQUIRE(initial == 0); REQUIRE(train->last_station_visited == StationID{15}); REQUIRE(train->cargo_payment != nullptr);
		REQUIRE(train->cargo_payment->route_profit > Money{0}); fare = train->cargo_payment->route_profit;
	}
	const auto ceiling = SurfaceChemicalWorkLimit(); REQUIRE(ceiling <= 2000);
	const auto limit = QuoteSurfaceGoodsRailFeederWork(Money{0}, ceiling);
	fmt::print("SURFACE_RESUPPLY_FUNDED_FARM_GOODS_SELECTION ceiling={} ticks={} found={} settlement={} initial={}\n", ceiling, limit, limit > 0, settlement, initial);
	if (limit == 0) return;
	uint previous = initial, boarded = 0, accepted = 0; bool posted = false;
	/** @return Whether genuine Goods settlement or actual working cash ends this finite interval. */
	const auto done = [&] {
		REQUIRE(company->money >= Money{40000}); const auto current = SurfaceFarmBackhaulGoods(train);
		if (current > previous) { REQUIRE(train->last_station_visited == StationID{13}); boarded += current-previous; }
		if (current < previous) {
			REQUIRE(train->last_station_visited == StationID{15});
			REQUIRE(Station::Get(StationID{15})->goods[GetCargoTypeByLabel(CargoLabel{"GOOD"})].status.Test(GoodsEntry::State::Acceptance));
			accepted += previous-current;
		}
		previous = current;
		// Only accepted Goods and this actual15receiver's positive whole
		// CargoPayment identify the unload. Earlier crops13or other trains'
		// fares cannot qualify the newly bought Goods capacity's first payment.
		if ((accepted > 0 || settlement) && train->cargo_payment != nullptr && train->last_station_visited == StationID{15}) fare = std::max(fare, train->cargo_payment->route_profit);
		posted = fare > Money{0} && current == 0 && train->cargo_payment == nullptr;
		// The first-payment consumer keeps its original stop. Only the separate
		// reserve consumer continues paid service until actual working cash is
		// restored; a posted fare alone cannot complete its cash requirement.
		return company->money >= Money{100000} || (stop_on_payment && posted);
	};
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, done); const auto final = SurfaceFarmBackhaulGoods(train);
	VerifySurfaceFundedFarmGoods(); REQUIRE(initial+boarded == accepted+final);
	fmt::print("SURFACE_RESUPPLY_FUNDED_FARM_GOODS_SERVICE ticks={} initial={} boarded={} accepted={} held={} observed_whole_unload_fare={} payment_pending={} payment_posted={} settlement={} cash={} restored={} reserve=40000 cohort_only_fare_claimed=false feeder_credit_is_cash=false\n",
		work.ticks, initial, boarded, accepted, final, fare.base(), train->cargo_payment != nullptr, posted, settlement, company->money.base(), company->money >= Money{100000});
	if (!stop_on_payment) fmt::print("SURFACE_RESUPPLY_WORKING_CASH_MODE stop_on_payment=false target=100000 cash={} restored={} reserve=40000\n", company->money.base(), company->money >= Money{100000});
}

/** Execute one selected actual post-retirement proposal or finite normal transport boundary. */
TEST_CASE("Actual funded farm Goods service earns only native first payment", "[.surface-funded-farm-goods]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_RESUPPLY_PHASE"); REQUIRE(selected != nullptr); const std::string phase{selected};
	const std::array<std::string_view, 3> allowed{"funded-farm-goods-query", "funded-farm-goods-work", "funded-farm-goods-payment"};
	REQUIRE(std::ranges::find(allowed, phase) != allowed.end());
	const auto paths = GetTestCaseArtifactPaths("space", "surface-funded-farm-goods"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Retain exact original progress or a partial native failure without repeating earned work. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves/("surface-resupply-before-"+phase+"-440.sav"));
			if (phase == "funded-farm-goods-query") QuerySurfaceFundedFarmGoods(); else WorkSurfaceFundedFarmGoods(phase == "funded-farm-goods-payment");
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
#endif /* TEST_SURFACE_ENTRY_FUNDED_FARM_GOODS_H */
