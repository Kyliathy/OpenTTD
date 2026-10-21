/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_module_capital.h Pure quotes for genuine remaining module financing.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CO.
 * Included after the native resupply command adapters. Temporary proposals own
 * no money, passengers, station rights, vehicle orders or simulation progress.
 */
#ifndef TEST_SURFACE_ENTRY_MODULE_CAPITAL_H
#define TEST_SURFACE_ENTRY_MODULE_CAPITAL_H
#include "../station_cmd.h"
#include "surface_regional_layout.h"

/** Complete cost-only calendar for two separately bounded future work intervals. */
struct SurfaceModuleCapitalWork {
	Money upkeep{0}; ///< All current active costs, pending ticks and proposed new buses.
	Money interest{0}; ///< Conservative charge at every crossed native month.
	Money station{0}; ///< Fixed native station charge, including when infrastructure maintenance is disabled.
	uint days = 0; ///< Upper bound on native daily accounting boundaries.
	uint months = 0; ///< Actual possible month boundaries, never assumed to be just two.
	bool year_boundary = false; ///< Unpriced annual repricing forbids startup admission.
};

/** Quote continuing native services without counting fares, refunds or credit.
 * @param added Number of genuinely proposed new40-seat model119buses.
 * @param limit Combined remaining native cap, at most6000ticks across separately bounded3000tick intervals.
 * @return Complete6000tick cost ceiling, including the three original regional buses and Steel service before renewal.
 */
static SurfaceModuleCapitalWork QuoteSurfaceModuleCapitalWork(uint added, uint limit = 6000)
{
	REQUIRE(limit > 0); REQUIRE(limit <= 6000);
	const auto *company = Company::Get(_current_company);
	REQUIRE(!_settings_game.economy.infrastructure_maintenance); REQUIRE(company->engine_renew_list == nullptr);
	SurfaceModuleCapitalWork result;
	result.days = (limit + TimerGameEconomy::date_fract + Ticks::DAY_TICKS - 1) / Ticks::DAY_TICKS;
	const int lead = company->settings.engine_renew ? std::max(0, -int{company->settings.engine_renew_months} * 30) : 0;
	constexpr uint64_t denominator = uint64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS * 256;
	// All active conventional services and orbital hulls retain their costs.
	// Stopped ground reserves stay stopped; their sale value cannot pay startup.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->owner != _current_company) continue;
		if (!vehicle->IsPrimaryVehicle()) continue;
		const bool ground = vehicle->type == VehicleType::Train || vehicle->type == VehicleType::Road;
		// Only active ground services can accrue running ticks in this proposal;
		// orbital upkeep remains payable even for an idle or stopped constructor.
		if (ground && vehicle->vehstatus.Test(VehState::Stopped)) continue;
		if (!ground && vehicle->type != VehicleType::Space) continue;
		if (ground) REQUIRE(vehicle->max_age.base() - vehicle->age.base() > static_cast<int>(result.days) + lead);
		const auto annual = vehicle->GetRunningCost().base(); REQUIRE(annual >= 0);
		const uint ticks = limit + (ground ? vehicle->running_ticks : 0);
		result.upkeep += Money{static_cast<int64_t>((static_cast<uint64_t>(annual) * ticks + denominator - 1) / denominator) + 1};
		fmt::print("SURFACE_RESUPPLY_CAPITAL_SERVICE vehicle={} engine={} annual256={} pending={} remaining={} profit_last={} profit_this={} orders={}\n",
			vehicle->index.base(), vehicle->engine_type.base(), annual, ground ? vehicle->running_ticks : 0,
			vehicle->max_age.base() - vehicle->age.base(), vehicle->profit_last_year.base(), vehicle->profit_this_year.base(), vehicle->GetNumOrders());
		// Retain actual public ordered destinations so service characterization
		// distinguishes native routes from merely counting purchased bodies.
		for (const auto &order : vehicle->Orders()) {
			fmt::print("SURFACE_RESUPPLY_CAPITAL_ROUTE vehicle={} type={} destination={} load={} unload={}\n", vehicle->index.base(),
				to_underlying(order.GetType()), order.GetDestination().base(), to_underlying(order.GetLoadType()), to_underlying(order.GetUnloadType()));
		}
	}
	// Engine purchase information is plain annual money; existing vehicle costs
	// above are fixed-point256. Round each proposed bus upward independently.
	const auto annual = Engine::Get(EngineID{119})->GetRunningCost().base(); REQUIRE(annual >= 0);
	result.upkeep += Money{(annual * limit + CalendarTime::DAYS_IN_YEAR * Ticks::DAY_TICKS - 1) /
		(CalendarTime::DAYS_IN_YEAR * Ticks::DAY_TICKS) + 2} * added;
	auto previous = TimerGameEconomy::month;
	// Every crossed month charges interest and the fixed station fee. A year
	// crossing requires a separately priced proposal before any native purchase.
	for (uint offset = 1; offset <= result.days; ++offset) {
		const auto future = TimerGameEconomy::ConvertDateToYMD(TimerGameEconomy::Date{TimerGameEconomy::date.base() + static_cast<int>(offset)});
		if (future.month != previous) { ++result.months; previous = future.month; }
		if (future.year != TimerGameEconomy::year) result.year_boundary = true;
	}
	const auto yearly = company->current_loan * _economy.interest_rate / 100;
	result.interest = Money{(yearly.base() + 11) / 12} * result.months;
	result.station = (_price[Price::StationValue] >> 2) * result.months;
	return result;
}

/** Price one paid joined stop and additional buses while fully protecting near-due original services.
 * Native command queries alone determine civic permission and prices. Catchment
 * production ranks real coverage; it never creates passengers or predicts fares.
 */
static void QuerySurfaceModuleCapital()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{124567});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"});
	const auto connected = SurfaceRegionalLayout::DepotComponent();
	REQUIRE(connected.contains(TileIndex{2546})); REQUIRE(connected.contains(TileIndex{2789}));
	REQUIRE(Station::Get(StationID{17})->GetCatchmentRadius() == CA_BUS);
	REQUIRE(Station::Get(StationID{18})->GetCatchmentRadius() == CA_BUS);
	TileIndex selected = INVALID_TILE; DiagDirection orientation = DiagDirection::NE; Money infrastructure{0};
	uint houses = 0, candidates = 0, refusals = 0; uint last_error = 0;
	// Search only the existing reciprocal depot component. A joined stop must
	// add actual Town0house coverage without clearing buildings or buying roads.
	for (const auto tile : connected) {
		if (!IsNormalRoadTile(tile)) continue;
		const auto supply = SurfaceRegionalLayout::NewSupply(tile, TownID{0});
		if (supply == 0) continue;
		const auto bits = SurfaceRegionalLayout::Bits(tile);
		if (bits != ROAD_X && bits != ROAD_Y) continue; // A straight road preserves all current junctions and their traffic.
		const auto direction = bits == ROAD_X ? DiagDirection::NE : DiagDirection::SE;
		++candidates;
		const auto quote = QuoteSurfaceResupply<Commands::BuildRoadStop>(tile, tile, uint8_t{1}, uint8_t{1}, RoadStopType::Bus,
			true, direction, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, StationID{17}, true);
		if (quote.Failed()) { ++refusals; last_error = quote.GetErrorMessage().base(); continue; }
		REQUIRE(quote.GetCost() >= 0);
		// Prefer more genuinely unserved producing houses, then the smaller real
		// invoice. Neither a refused station nor existing coverage supplies income.
		if (supply < houses || (supply == houses && quote.GetCost() >= infrastructure)) continue;
		selected = tile; orientation = direction; infrastructure = quote.GetCost(); houses = supply;
	}
	fmt::print("SURFACE_RESUPPLY_CAPITAL_COVERAGE tile={} direction={} new_houses={} infrastructure={} candidates={} refusals={} error={} connected={} joined_station=17 new_roads=0 granted_passengers=0\n",
		selected.base(), to_underlying(orientation), houses, infrastructure.base(), candidates, refusals, last_error, connected.size());
	const auto bus = QuoteSurfaceResupply<Commands::BuildVehicle>(SurfaceRegionalLayout::depot, SurfaceRegionalLayout::depot,
		EngineID{119}, false, passenger, ClientID::Server);
	REQUIRE(std::get<0>(bus).Succeeded()); REQUIRE(std::get<2>(bus) == 40);
	const auto unit = std::get<0>(bus).GetCost(); Money renewal{0}; uint due = 0;
	// Buy-price every active road service due within one year, with no resale
	// proceeds. All four original roles must receive genuine paid replacements.
	for (const auto *vehicle : RoadVehicle::Iterate()) {
		if (vehicle->owner != _current_company) continue;
		if (!vehicle->IsPrimaryVehicle()) continue;
		if (vehicle->vehstatus.Test(VehState::Stopped)) continue;
		const auto remaining = vehicle->max_age.base() - vehicle->age.base();
		if (remaining > CalendarTime::DAYS_IN_YEAR) continue;
		const auto depot = vehicle->cargo_type == passenger ? SurfaceRegionalLayout::depot : surface_resupply_road_depot;
		const auto replacement = QuoteSurfaceResupply<Commands::BuildVehicle>(depot, depot, vehicle->engine_type, false, vehicle->cargo_type, ClientID::Server);
		REQUIRE(std::get<0>(replacement).Succeeded()); REQUIRE(std::get<2>(replacement) == vehicle->cargo_cap);
		renewal += std::get<0>(replacement).GetCost(); ++due;
		fmt::print("SURFACE_RESUPPLY_CAPITAL_RENEWAL vehicle={} engine={} depot={} capacity={} remaining={} gross={} refund_counted=0\n",
			vehicle->index.base(), vehicle->engine_type.base(), depot.base(), vehicle->cargo_cap, remaining, std::get<0>(replacement).GetCost().base());
	}
	REQUIRE(due == 4);
	const auto first_waiting = Station::Get(StationID{17})->goods[passenger].AvailableCount();
	const auto last_waiting = Station::Get(StationID{18})->goods[passenger].AvailableCount();
	REQUIRE(first_waiting == 29); REQUIRE(last_waiting == 55);
	fmt::print("SURFACE_RESUPPLY_CAPITAL_LOCAL first=17 last=18 depot=2984 first_waiting={} last_waiting={} existing_local_buses=3 model=119 capacity=40 infrastructure=0 granted_passengers=0\n", first_waiting, last_waiting);
	// Compare both explicit policy cases and each concrete fleet size. This is
	// read-only: any later startup needs its own chosen, freshly funded producer.
	for (uint added = 1; added <= 6; ++added) {
		const auto work = QuoteSurfaceModuleCapitalWork(added);
		const auto purchases = infrastructure + unit * added + renewal;
		const auto operating = work.upkeep + work.interest + work.station;
		for (const Money reserve : {Money{100000}, Money{40000}}) {
			const auto target = reserve + purchases + operating;
			// Both actual civic admission and the entire cost-only cash bill are
			// required. Future fares, asset refunds and additional debt contribute0.
			const bool funded = selected != INVALID_TILE && !work.year_boundary && company->money >= target;
			fmt::print("SURFACE_RESUPPLY_CAPITAL_ADMISSION buses={} capacity=40 unit={} infrastructure={} renewal={} purchases={} ticks=6000 intervals=2 days={} months={} upkeep={} interest={} station_charge={} operating={} reserve={} target={} cash={} shortfall={} funded={} year_boundary={} projected_fares=0 refund_counted=0 credit_counted=0 module_reserve=150000\n",
				added, unit.base(), infrastructure.base(), renewal.base(), purchases.base(), work.days, work.months, work.upkeep.base(), work.interest.base(),
				work.station.base(), operating.base(), reserve.base(), target.base(), company->money.base(), std::max(Money{0}, target - company->money).base(), funded, work.year_boundary);
			const auto local_target = target - infrastructure;
			// The existing paid route can admit a single added bus against its
			// actual84waiting passengers. More fleet expansion needs measured
			// demand and profit after this first actual investment, not forecasts.
			const bool local_funded = added == 1 && !work.year_boundary && company->money >= local_target;
			fmt::print("SURFACE_RESUPPLY_CAPITAL_LOCAL_ADMISSION buses={} unit={} renewal={} purchases={} operating={} reserve={} target={} cash={} shortfall={} funded={} first=17 last=18 depot=2984 capacity=40 ticks=6000 intervals=2 days={} months={} infrastructure=0 year_boundary={} projected_fares=0 refund_counted=0 credit_counted=0 module_reserve=150000\n",
				added, unit.base(), renewal.base(), (unit * added + renewal).base(), operating.base(), reserve.base(), local_target.base(), company->money.base(),
				std::max(Money{0}, local_target - company->money).base(), local_funded, work.days, work.months, work.year_boundary);
		}
	}
}
#endif /* TEST_SURFACE_ENTRY_MODULE_CAPITAL_H */
