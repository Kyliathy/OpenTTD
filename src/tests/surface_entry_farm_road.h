/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_farm_road.h Pure actual farm-road authority and whole investment admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGM.
 * Native tile authority and existing station inventory decide this proposal.
 * No civic permission, road, cargo, carrier or anticipated fare is supplied.
 */
#ifndef TEST_SURFACE_ENTRY_FARM_ROAD_H
#define TEST_SURFACE_ENTRY_FARM_ROAD_H
#include "../town.h"

/** Complete native joined-stop quote, without building or acquiring rights. */
struct SurfaceFarmRoadStop {
	TileIndex tile = INVALID_TILE; ///< Actual existing connected normal-road candidate.
	DiagDirection direction = DiagDirection::NE; ///< Actual straight-road drive-through axis.
	Money invoice{0}; ///< Full native stop price, never a guessed150constant.
	int authority = -1; ///< Actual local authority ID, or no town within the configured radius.
};

/** Quote legal joins using actual current authority at each individual road tile.
 * @param station Existing owned Farm pickup15or Factory receiver13.
 * @param component Current reciprocal native depot-connected road tiles.
 * @return Cheapest accepted actual join or a literal missing candidate.
 */
static SurfaceFarmRoadStop QuoteSurfaceFarmRoadStop(StationID station, const std::set<TileIndex> &component)
{
	SurfaceFarmRoadStop selected; uint candidates = 0, accepted = 0, refused = 0, last_error = 0;
	// Distant station membership shares the real rail station's cargo owner.
	// Existing roads/actual native authority must admit the join before any
	// road vehicle can use that inventory; town association alone grants none.
	for (const auto tile : component) {
		if (!IsNormalRoadTile(tile)) continue;
		const auto bits = GetAnyRoadBits(tile, RoadTramType::Road);
		// A drive-through stop retains a straight existing road. Do not demolish
		// junctions, stations, depots, rail or bridges to make the proposal fit.
		if (bits != ROAD_X && bits != ROAD_Y) continue;
		++candidates; const auto direction = bits == ROAD_X ? DiagDirection::NE : DiagDirection::SE;
		const auto quote = QuoteSurfaceResupply<Commands::BuildRoadStop>(tile, tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck,
			true, direction, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, station, true);
		if (quote.Failed()) { ++refused; last_error = quote.GetErrorMessage().base(); continue; }
		++accepted; REQUIRE(quote.GetCost() >= Money{0});
		// Native price then canonical tile order chooses a reproducible whole
		// stop. A refused civic proposal cannot become an imaginary permission.
		if (selected.tile != INVALID_TILE && quote.GetCost() >= selected.invoice) continue;
		const auto *authority = ClosestTownFromTile(tile, _settings_game.economy.dist_local_authority);
		selected = {tile, direction, quote.GetCost(), authority == nullptr ? -1 : static_cast<int>(authority->index.base())};
	}
	fmt::print("SURFACE_RESUPPLY_FARM_ROAD_STOP station={} tile={} direction={} invoice={} authority={} candidates={} accepted={} refused={} error={} found={} road_built=0 permission_granted=0\n",
		station.base(), selected.tile.base(), to_underlying(selected.direction), selected.invoice.base(), selected.authority, candidates, accepted, refused, last_error, selected.tile != INVALID_TILE);
	return selected;
}

/** Pure actual stop/model selection and full new-truck/current-company cost admission. */
static void QuerySurfaceFarmRoad()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{73667}); REQUIRE(company->current_loan == Money{5000000});
	const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"}); VerifySurfaceIntertown(); VerifySurfaceFarmRebalance(true);
	const auto *source = Station::Get(StationID{15}), *receiver = Station::Get(StationID{13});
	REQUIRE(source->owner == _current_company); REQUIRE(receiver->owner == _current_company);
	REQUIRE(receiver->goods[livestock].status.Test(GoodsEntry::State::Acceptance));
	const auto component = SurfaceGoodsRoadComponent(TileIndex{2443});
	const auto pickup = QuoteSurfaceFarmRoadStop(StationID{15}, component), destination = QuoteSurfaceFarmRoadStop(StationID{13}, component);
	QuoteSurfaceFixedPriceIncome(2000); const auto work = QuoteSurfaceModuleCapitalWork(0, 2000);
	const auto operating = work.upkeep + work.interest + work.station;
	EngineID selected = EngineID::Invalid(); Money invoice{0}, extra{0}; uint capacity = 0;
	const int lead = company->settings.engine_renew ? std::max(0, -int{company->settings.engine_renew_months} * 30) : 0;
	// Only native default-Livestock road models are candidates. No free refit,
	// expired model, NewGRF callback or invented future useful life is admitted.
	for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
		if (engine->GetDefaultCargoType() != livestock) continue;
		if (engine->GetGRF() != nullptr) continue;
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, engine->index, false, livestock, ClientID::Server);
		const auto &cost = std::get<0>(quote); const auto life = engine->GetLifeLengthInDays().base();
		const auto annual = engine->GetRunningCost().base(); REQUIRE(annual >= 0);
		fmt::print("SURFACE_RESUPPLY_FARM_ROAD_MODEL engine={} available={} error={} invoice={} capacity={} annual={} life_days={} speed={}\n",
			engine->index.base(), cost.Succeeded(), cost.GetErrorMessage().base(), cost.GetCost().base(), std::get<2>(quote), annual, life, engine->GetDisplayMaxSpeed());
		if (cost.Failed()) continue;
		if (std::get<2>(quote) == 0) continue;
		if (life <= static_cast<int>(work.days) + lead) continue;
		if (selected != EngineID::Invalid() && cost.GetCost() >= invoice) continue;
		selected = engine->index; invoice = cost.GetCost(); capacity = std::get<2>(quote);
		// Engine running costs are plain annual money, unlike existing vehicles'
		// fixed256 books. Round the entire new2000tick rate upward independently.
		constexpr uint64_t denominator = uint64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS;
		extra = Money{static_cast<int64_t>((static_cast<uint64_t>(annual) * 2000 + denominator - 1) / denominator) + 2};
	}
	const bool complete = pickup.tile != INVALID_TILE && destination.tile != INVALID_TILE && selected != EngineID::Invalid();
	fmt::print("SURFACE_RESUPPLY_FARM_ROAD_SOURCE source=15 receiver=13 industry=7 waiting={} accepts=true component={} town0_rating={} authority_radius={} engine={} capacity={} complete={} purchases=0 changed_roads=0 changed_rating=0\n",
		source->goods[livestock].AvailableCount(), component.size(), Town::Get(TownID{0})->ratings[_current_company], _settings_game.economy.dist_local_authority,
		selected.base(), capacity, complete);
	if (!complete) return;
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto gross = pickup.invoice + destination.invoice + invoice; const auto target = reserve + gross + operating + extra;
		fmt::print("SURFACE_RESUPPLY_FARM_ROAD_ADMISSION ticks=2000 days={} months={} year_boundary={} operating={} extra_running={} stops={} vehicle_invoice={} gross={} reserve={} target={} cash={} funded={} future_income_counted=0 refunds_counted=0 credit=0\n",
			work.days, work.months, work.year_boundary, operating.base(), extra.base(), (pickup.invoice + destination.invoice).base(), invoice.base(), gross.base(), reserve.base(), target.base(), company->money.base(), company->money >= target);
	}
}
#endif /* TEST_SURFACE_ENTRY_FARM_ROAD_H */
