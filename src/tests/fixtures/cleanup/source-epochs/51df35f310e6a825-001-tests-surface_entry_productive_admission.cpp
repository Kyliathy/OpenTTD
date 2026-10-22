/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_productive_admission.cpp Pure original-world productive recovery admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZU.
 * Original command, cargo, company and clock owners remain authoritative.
 * The historical resupply translation unit supplies its exact command/save/
 * complete-cost adapters; its hidden test is registered but never selected by
 * this separate target's productive-admission invocation. No old file changes.
 */
#include "surface_entry_resupply.cpp"

/** Read current real demand and whole native productive-service invoices without buying or earning work. */
static void QuerySurfaceProductiveAdmission()
{
	VerifySurfaceGoodsRailDelivery(true);
	const auto *company = Company::Get(_current_company);
	REQUIRE(company->money == Money{45080}); REQUIRE(company->current_loan == Money{5000000});
	const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"});
	const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	const TileIndex depot{2984}; REQUIRE(Depot::GetByTile(depot) != nullptr);
	const auto bill = QuoteSurfaceFixedPriceIncome(500);
	const auto operating = bill.upkeep + bill.interest + bill.station;
	fmt::print("SURFACE_RESUPPLY_PRODUCTIVE_SOURCE cash={} debt={} reserve=40000 ticks=500 operating={} days={} months={} future_income_counted=0 paid=0 elapsed=0\n",
		company->money.base(), company->current_loan.base(), operating.base(), bill.days, bill.months);
	// Actual waiting packets and native acceptance identify unmet service demand;
	// neither a station's catchment nor its inventory represents posted money.
	for (const auto *station : Station::Iterate()) {
		if (station->owner != _current_company) continue;
		fmt::print("SURFACE_RESUPPLY_PRODUCTIVE_DEMAND station={} tile={} passengers={} passenger_accepts={} goods={} goods_accepts={} bus_stop={} truck_stop={} rail_width={} rail_height={}\n",
			station->index.base(), station->xy.base(), station->goods[passenger].AvailableCount(), station->goods[passenger].status.Test(GoodsEntry::State::Acceptance),
			station->goods[goods].AvailableCount(), station->goods[goods].status.Test(GoodsEntry::State::Acceptance), station->bus_stops != nullptr,
			station->truck_stops != nullptr, station->train_station.w, station->train_station.h);
	}
	// Only current buildable callback-free passenger bodies are compared. Native
	// BuildVehicle quotes include actual refit/capacity, while the complete company
	// operating bill and existing reserve remain payable before any purchase.
	for (const auto *engine : Engine::Iterate()) {
		if (engine->type != VehicleType::Road) continue;
		if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
		if (engine->GetGRF() != nullptr) continue;
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(depot, depot, engine->index, false, passenger, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		const auto invoice = std::get<0>(quote).GetCost();
		// Engine information is plain annual money. Price the proposed body's
		// entire first interval, rounded upward with the shared owner's safety
		// margin, before comparing any whole purchase with actual current cash.
		const auto annual = engine->GetRunningCost().base(); REQUIRE(annual >= 0);
		constexpr int64_t denominator = CalendarTime::DAYS_IN_YEAR * Ticks::DAY_TICKS;
		const Money added{(annual * 500 + denominator - 1) / denominator + 2};
		const auto target = Money{40000} + invoice + operating + added;
		fmt::print("SURFACE_RESUPPLY_PRODUCTIVE_MODEL engine={} depot=2984 capacity={} speed_half_kmh={} invoice={} existing_operating={} added_upkeep={} annual_upkeep={} reserve=40000 target={} cash={} funded={} new_body_running_cost_priced=true work_permission=false\n",
			engine->index.base(), std::get<2>(quote), engine->VehInfo<RoadVehicleInfo>().max_speed, invoice.base(), operating.base(), added.base(), annual, target.base(),
			company->money.base(), company->money >= target);
	}
	// Preserve every actual loaded vehicle and current order. Empty existing
	// bodies can expose a refit estimate, but a remote estimate cannot bypass
	// the native depot requirement or supply travel, passengers or revenue.
	for (const auto *vehicle : RoadVehicle::Iterate()) {
		if (vehicle->owner != _current_company) continue;
		fmt::print("SURFACE_RESUPPLY_PRODUCTIVE_VEHICLE vehicle={} engine={} tile={} cargo={} capacity={} held={} stopped={} depot={} orders={} speed={} profit_256={}\n",
			vehicle->index.base(), vehicle->engine_type.base(), vehicle->tile.base(), vehicle->cargo_type, vehicle->cargo_cap, vehicle->cargo.TotalCount(),
			vehicle->vehstatus.Test(VehState::Stopped), vehicle->IsStoppedInDepot(), vehicle->GetNumOrders(), vehicle->cur_speed, vehicle->profit_this_year.base());
		if (vehicle->cargo.TotalCount() != 0) continue;
		for (const auto cargo : {passenger, goods}) {
			const auto quote = QuoteSurfaceResupply<Commands::RefitVehicle>(vehicle->tile, vehicle->index, cargo, uint8_t{0}, false, false, uint8_t{1});
			fmt::print("SURFACE_RESUPPLY_PRODUCTIVE_REFIT vehicle={} cargo={} available={} capacity={} invoice={} depot={} work_permission=false\n",
				vehicle->index.base(), cargo, std::get<0>(quote).Succeeded(), std::get<1>(quote), std::get<0>(quote).GetCost().base(), vehicle->IsStoppedInDepot());
		}
	}
}

/** Load the exact original world and retain a pure productive-service admission checkpoint. */
TEST_CASE("Current original productive recovery has only native fully priced admission", "[.surface-productive-admission]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-productive-admission");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Inspect real owners under the existing paused console boundary, preserving any native failure. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves / "surface-resupply-before-productive-query-440.sav");
			QuerySurfaceProductiveAdmission();
			SaveSurfaceResupply(paths.saves / "surface-resupply-productive-query-440.sav");
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase=productive-query supplied_stock=false supplied_work=false\n"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect);
	IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

// Separately selected current rail admission reuses the exact same real
// command/save owners; the previous isolated source/application is archived.
#include "surface_entry_rail_productivity.h"

// Real empty return is a distinct selected producer with its own complete
// prospective wagon invoice and unchanged forty-thousand reserve admission.
#include "surface_entry_mixed_farm_return.h"
#include "surface_entry_mixed_farm_backhaul.h"
