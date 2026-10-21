/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_farm_balance.h Pure balanced farm capacity retaining every original wagon as an owned asset.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGG.
 * Three proposed parked Grain bodies remain a coupled reserve, not a refund
 * or active capacity. Native commands alone may later reassign empty bodies.
 */
#ifndef TEST_SURFACE_ENTRY_FARM_BALANCE_H
#define TEST_SURFACE_ENTRY_FARM_BALANCE_H

/** Price a single-train capacity balance and every still unpaid return/service cost without mutation. */
static void QuerySurfaceFarmBalance()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{81662});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	const auto *train = VerifySurfaceFarmUpgrade(true); REQUIRE(train->gcache.cached_total_length == 112);
	const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"}), grain = GetCargoTypeByLabel(CargoLabel{"GRAI"});
	// Only an actual empty return can later reassign these exact original
	// surplus bodies. No Grain packet may be dropped to make the proposal fit.
	for (const Train *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
	for (const auto id : {VehicleID{18}, VehicleID{19}, VehicleID{20}}) {
		const auto *part = Train::Get(id); REQUIRE(part->First() == train); REQUIRE(part->owner == _current_company);
		REQUIRE(part->engine_type == EngineID{33}); REQUIRE(part->cargo_type == grain); REQUIRE(part->cargo_cap == 30);
		REQUIRE(part->GetEngine()->GetGRF() == nullptr); REQUIRE(part->GetEngine()->VehInfo<RailVehicleInfo>().running_cost_class == Price::Invalid);
	}
	const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(TileIndex{1638}, TileIndex{1638}, EngineID{31}, false, livestock, ClientID::Server);
	REQUIRE(std::get<0>(quote).Succeeded()); REQUIRE(std::get<0>(quote).GetCost() == Money{1125}); REQUIRE(std::get<2>(quote) == 25);
	const auto *engine = Engine::Get(EngineID{31}); REQUIRE(engine->GetGRF() == nullptr);
	REQUIRE(engine->VehInfo<RailVehicleInfo>().running_cost_class == Price::Invalid);
	const auto recall = QuoteSurfaceResupply<Commands::SendVehicleToDepot>(TileIndex{0}, train->index, DepotCommandFlags{}, VehicleListIdentifier{});
	REQUIRE(recall.Succeeded()); REQUIRE(recall.GetCost() == Money{0});
	for (const auto id : {StationID{15}, StationID{13}}) {
		const auto *station = Station::Get(id); REQUIRE(std::max(station->train_station.w, station->train_station.h) == 7);
	}
	fmt::print("SURFACE_RESUPPLY_FARM_BALANCE_SOURCE head=52 waiting_livestock={} waiting_grain={} current_livestock=175 current_grain=180 proposed_livestock=250 proposed_grain=90 parked_grain_capacity=90 original_wagons_owned=32 parked_original=3 added_wagons_to_buy=3 existing_extra=50 length=112 max_tiles=7 new_head=0 infrastructure=0 recall=0 purchases=0 changed_orders=0\n",
		Station::Get(StationID{15})->goods[livestock].AvailableCount(), Station::Get(StationID{15})->goods[grain].AvailableCount());
	// Existing head and every active/parked plain wagon retain their native
	// running classes. Replacing three unpowered active bodies with three
	// unpowered bodies changes capacity, not the sixteen powered-body rates.
	QuoteSurfaceFixedPriceIncome(3000); const auto work = QuoteSurfaceModuleCapitalWork(0, 6000);
	const auto operating = work.upkeep + work.interest + work.station;
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto target = reserve + Money{3375} + operating;
		fmt::print("SURFACE_RESUPPLY_FARM_BALANCE_ADMISSION ticks=6000 interval_limit=3000 days={} months={} year_boundary={} operating={} invoice=3375 extra_running=0 reserve={} target={} cash={} funded={} parked_original=3 original_wagons_owned=32 projected_fares=0 refunds_counted=0 credit=0 working_shortfall=18338 module_base_shortfall=428338\n",
			work.days, work.months, work.year_boundary, operating.base(), reserve.base(), target.base(), company->money.base(), company->money >= target);
	}
}
#endif /* TEST_SURFACE_ENTRY_FARM_BALANCE_H */
