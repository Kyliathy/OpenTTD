/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_farm_fleet.h Pure native further-farm capacity and complete known-cost refusals.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGG.
 * Available engines and public command quotes own every invoice. A passing
 * arrangement still needs its own whole native price before any new head.
 */
#ifndef TEST_SURFACE_ENTRY_FARM_FLEET_H
#define TEST_SURFACE_ENTRY_FARM_FLEET_H

/** Query current real farm offers, refit compatibility and affordable native additional-head lower bounds. */
static void QuerySurfaceFarmFleet()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{81662});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	const auto *train = VerifySurfaceFarmUpgrade(true); REQUIRE(train->gcache.cached_total_length == 112);
	const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"}), grain = GetCargoTypeByLabel(CargoLabel{"GRAI"});
	const auto *hopper = Engine::Get(EngineID{33}); REQUIRE(hopper->GetGRF() == nullptr);
	fmt::print("SURFACE_RESUPPLY_FARM_FLEET_SOURCE head=52 length=112 max_tiles=7 livestock_capacity=175 grain_capacity=180 waiting_livestock={} waiting_grain={} grain_to_livestock_refittable={} wagon=50 original_wagons=32 added_wagons=1 purchases=0 changed_orders=0\n",
		Station::Get(StationID{15})->goods[livestock].AvailableCount(), Station::Get(StationID{15})->goods[grain].AvailableCount(), hopper->info.refit_mask.Test(livestock));
	const auto wagon = QuoteSurfaceResupply<Commands::BuildVehicle>(TileIndex{1638}, TileIndex{1638}, EngineID{31}, false, livestock, ClientID::Server);
	REQUIRE(std::get<0>(wagon).Succeeded()); REQUIRE(std::get<2>(wagon) == 25);
	const auto unit = std::get<0>(wagon).GetCost(); REQUIRE(unit == Money{1125});
	std::array<SurfaceModuleCapitalWork, 2> windows{QuoteSurfaceFixedPriceIncome(2000), QuoteSurfaceFixedPriceIncome(3000)};
	uint attempts = 0, accepted = 0, refused = 0, freight = 0;
	// Native availability/depot compatibility rejects retired or wrong-rail
	// models. Do not assume that an inexpensive historic engine can be bought
	// now, or that a built-in passenger compartment supplies livestock capacity.
	for (const auto *engine : Engine::Iterate()) {
		if (engine->type != VehicleType::Train) continue;
		const auto &info = engine->VehInfo<RailVehicleInfo>();
		if (info.railveh_type == RailVehicleType::Wagon) continue;
		if (info.power < 1000) continue;
		if (info.max_speed < 112) continue;
		if (engine->GetGRF() != nullptr) continue;
		++attempts;
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(TileIndex{1638}, TileIndex{1638}, engine->index, false, INVALID_CARGO, ClientID::Server);
		if (std::get<0>(quote).Failed()) {
			++refused;
			fmt::print("SURFACE_RESUPPLY_FARM_FLEET_REFUSAL engine={} error={} power={} speed={}\n", engine->index.base(), std::get<0>(quote).GetErrorMessage().base(), info.power, info.max_speed);
			continue;
		}
		++accepted; const auto invoice = std::get<0>(quote).GetCost(); const auto annual = engine->GetRunningCost();
		REQUIRE(invoice > Money{0}); REQUIRE(annual >= Money{0});
		const uint units = info.railveh_type == RailVehicleType::Multihead ? 2 : 1;
		const auto capacity = std::get<2>(quote);
		fmt::print("SURFACE_RESUPPLY_FARM_FLEET_HEAD engine={} invoice={} units={} built_in_capacity={} power={} speed={} annual={} callback_free=true\n",
			engine->index.base(), invoice.base(), units, capacity, info.power, info.max_speed, annual.base());
		if (capacity != 0) continue;
		++freight;
		// Both powered halves belong to a multihead's actual whole invoice and
		// annual rate. Each proposed unpowered livestock wagon costs1125/25units;
		// none of the future cargo or possible fares contributes to current cash.
		for (const uint wagons : {3U, 6U}) for (uint index = 0; index < windows.size(); ++index) {
			const uint limit = index == 0 ? 2000 : 3000; const auto &work = windows[index];
			constexpr int64_t denominator = int64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS;
			const auto extra = Money{(annual.base() * limit + denominator - 1) / denominator + 2};
			const auto operating = work.upkeep + work.interest + work.station + extra;
			const auto assets = invoice + unit * wagons; const auto proposed_length = (units + wagons) * 8;
			REQUIRE(proposed_length <= 7 * TILE_SIZE);
			for (const Money reserve : {Money{100000}, Money{40000}}) {
				const auto known = reserve + assets + operating;
				fmt::print("SURFACE_RESUPPLY_FARM_FLEET_LOWER_BOUND engine={} wagons={} capacity={} length={} ticks={} days={} months={} year_boundary={} assets={} extra_upkeep={} operating={} reserve={} known_target={} cash={} known_funded={} infrastructure_priced=false route_admitted=false whole_funded=false projected_fares=0 refunds_counted=0 credit=0\n",
					engine->index.base(), wagons, wagons * 25, proposed_length, limit, work.days, work.months, work.year_boundary, assets.base(), extra.base(), operating.base(), reserve.base(), known.base(), company->money.base(), company->money >= known);
			}
		}
	}
	fmt::print("SURFACE_RESUPPLY_FARM_FLEET_SUMMARY attempts={} accepted={} refused={} freight_heads={} wagon_unit=1125 initial_cash=81662 purchases=0 changed_orders=0 working_shortfall=18338 module_base_shortfall=428338\n", attempts, accepted, refused, freight);
}
#endif /* TEST_SURFACE_ENTRY_FARM_FLEET_H */
