/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_farm_capacity.h Pure native farm-capacity admission within actual train/platform limits.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGE.
 * The original twelve wagons remain coupled and useful. A proposed extra body
 * exists only after a later native purchase; current quotes allocate nothing.
 */
#ifndef TEST_SURFACE_ENTRY_FARM_CAPACITY_H
#define TEST_SURFACE_ENTRY_FARM_CAPACITY_H
#include "../vehiclelist.h"

/** Price one real compatible wagon and the entire bounded return/service bill without mutation. */
static void QuerySurfaceFarmCapacity()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{85710});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	const auto *train = Train::Get(VehicleID{52}); REQUIRE(train->name == "Surface Income farm"); REQUIRE(train->IsFrontEngine());
	REQUIRE(train->owner == _current_company); REQUIRE(train->engine_type == EngineID{24});
	const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"}), grain = GetCargoTypeByLabel(CargoLabel{"GRAI"});
	uint parts = 0, livestock_capacity = 0, grain_capacity = 0, length = 0;
	// The quote protects the actual linked original bodies, not an aggregate
	// nominal capacity. An empty return cannot discard or substitute a wagon.
	for (const Train *part = train; part != nullptr; part = part->Next()) {
		REQUIRE(part->cargo.TotalCount() == 0); REQUIRE(part->GetEngine()->GetGRF() == nullptr);
		length += part->gcache.cached_veh_length; ++parts;
		if (part->cargo_type == livestock) livestock_capacity += part->cargo_cap;
		if (part->cargo_type == grain) grain_capacity += part->cargo_cap;
	}
	REQUIRE(parts == 13); REQUIRE(length == 104); REQUIRE(train->gcache.cached_total_length == length);
	REQUIRE(livestock_capacity == 150); REQUIRE(grain_capacity == 180);
	const auto *donor = train->Next(); REQUIRE(donor != nullptr); REQUIRE(donor->engine_type == EngineID{31});
	REQUIRE(donor->gcache.cached_veh_length == 8); REQUIRE(donor->cargo_cap == 25);
	const auto *engine = donor->GetEngine(); REQUIRE(engine->GetGRF() == nullptr);
	REQUIRE(engine->VehInfo<RailVehicleInfo>().running_cost_class == Price::Invalid); REQUIRE(engine->GetRunningCost() == Money{0});
	const auto proposed_length = length + donor->gcache.cached_veh_length;
	REQUIRE(_settings_game.vehicle.max_train_length == 7); REQUIRE(proposed_length == 112);
	REQUIRE(proposed_length <= _settings_game.vehicle.max_train_length * TILE_SIZE);
	for (const auto id : {StationID{15}, StationID{13}}) {
		const auto *station = Station::Get(id); REQUIRE(station->owner == _current_company);
		const auto platform = std::max(station->train_station.w, station->train_station.h);
		REQUIRE(platform == 7); REQUIRE(proposed_length <= platform * TILE_SIZE);
		fmt::print("SURFACE_RESUPPLY_FARM_PLATFORM station={} tiles={} proposed_length={} overhang=0\n", id.base(), platform, proposed_length);
	}
	REQUIRE(SurfaceResupplyRailConnected(TileIndex{1638}, train->tile, train->compatible_railtypes));
	const auto recall = QuoteSurfaceResupply<Commands::SendVehicleToDepot>(TileIndex{0}, train->index, DepotCommandFlags{}, VehicleListIdentifier{});
	REQUIRE(recall.Succeeded()); REQUIRE(recall.GetCost() == Money{0});
	const auto wagon = QuoteSurfaceResupply<Commands::BuildVehicle>(TileIndex{1638}, TileIndex{1638}, EngineID{31}, false, livestock, ClientID::Server);
	REQUIRE(std::get<0>(wagon).Succeeded()); REQUIRE(std::get<2>(wagon) == 25);
	const auto invoice = std::get<0>(wagon).GetCost(); REQUIRE(invoice > Money{0});
	const auto queued = Station::Get(StationID{15})->goods[livestock].AvailableCount(); REQUIRE(queued == 259);
	fmt::print("SURFACE_RESUPPLY_FARM_CAPACITY head=52 engine=24 wagon_engine=31 invoice={} native_capacity=25 queued_livestock={} original_livestock=150 proposed_livestock=175 grain=180 original_parts=13 proposed_parts=14 original_length=104 proposed_length=112 max_tiles=7 depot=1638 recall=0 connected=true callback_free=true extra_running=0 changed_orders=0 purchases=0\n", invoice.base(), queued);
	// The short guard verifies unchanged native rates and exhausted Space
	// queues. The longer forecast covers all still unpaid return/work time and
	// useful lives; an unpowered callback-free wagon adds no annual debit.
	QuoteSurfaceFixedPriceIncome(3000); const auto whole = QuoteSurfaceModuleCapitalWork(0, 6000);
	const auto operating = whole.upkeep + whole.interest + whole.station;
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto target = reserve + invoice + operating;
		fmt::print("SURFACE_RESUPPLY_FARM_ADMISSION ticks=6000 interval_limit=3000 days={} months={} year_boundary={} upkeep={} interest={} station_charge={} operating={} invoice={} reserve={} target={} cash={} funded={} projected_fares=0 refunds_counted=0 credit=0 working_shortfall=14290 module_base_shortfall=424290\n",
			whole.days, whole.months, whole.year_boundary, whole.upkeep.base(), whole.interest.base(), whole.station.base(), operating.base(), invoice.base(), reserve.base(), target.base(), company->money.base(), company->money >= target);
	}
}
#endif /* TEST_SURFACE_ENTRY_FARM_CAPACITY_H */
