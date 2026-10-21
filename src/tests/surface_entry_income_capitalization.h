/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_income_capitalization.h Actual civic coverage and fixed-price income admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSD.
 * Native houses, current authority, command quotes and company books own every
 * decision. Temporary proposals create no passengers, station, cargo or money.
 */
#ifndef TEST_SURFACE_ENTRY_INCOME_CAPITALIZATION_H
#define TEST_SURFACE_ENTRY_INCOME_CAPITALIZATION_H

/** Quote native continuing costs only when actual unchanged owners guarantee fixed rates across a year.
 * @param limit Newly proposed native interval, at most3000ticks.
 * @return Existing authoritative cost-only forecast, preserving its actual year-boundary flag.
 */
static SurfaceModuleCapitalWork QuoteSurfaceFixedPriceIncome(uint limit)
{
	REQUIRE(limit > 0); REQUIRE(limit <= 3000); REQUIRE(!_settings_game.economy.inflation);
	REQUIRE(_economy.interest_rate == 2); uint priced = 0;
	// Inflation is disabled in this original, and the monthly owner therefore
	// does not recompute prices. Callback-free engines retain their native base
	// running factors; no forecast can silently assume an arbitrary NewGRF's
	// date-dependent cost. Existing historical year guards remain unchanged.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->owner != _current_company) continue;
		if (!vehicle->IsPrimaryVehicle()) continue;
		const bool ground = vehicle->type == VehicleType::Train || vehicle->type == VehicleType::Road;
		// Only the unchanged active ground cohort is priced. Empty stopped
		// reserves remain stopped and contribute neither cash nor future service.
		if (ground && vehicle->vehstatus.Test(VehState::Stopped)) continue;
		if (!ground && vehicle->type != VehicleType::Space) continue;
		REQUIRE(vehicle->GetEngine()->GetGRF() == nullptr);
		if (vehicle->type == VehicleType::Space) {
			const auto *craft = SpaceVehicle::From(vehicle);
			// The stored rocket and exhausted constructor queue cannot acquire
			// active mining/construction rates during an unchanged ground trial.
			REQUIRE(craft->space_orders.cursor >= craft->space_orders.entries.size());
			if (craft->construction.placement == SpacePlacement::SurfaceStored) {
				REQUIRE(craft->GetRunningCost() == Money{0});
			} else {
				REQUIRE(craft->construction.placement == SpacePlacement::Orbital);
				REQUIRE(GetSpaceVehicleModel(*craft) == 20);
				REQUIRE(craft->GetRunningCost() == Money{GetMultiLayerSpaceBudget().idle_per_day} * CalendarTime::DAYS_IN_YEAR * 256);
			}
		}
		++priced;
	}
	const auto result = QuoteSurfaceModuleCapitalWork(0, limit);
	fmt::print("SURFACE_RESUPPLY_INCOME_FIXED_PRICES ticks={} inflation=false callback_free=true priced_bodies={} year_boundary={} days={} months={} upkeep={} interest={} station_charge={}\n",
		limit, priced, result.year_boundary, result.days, result.months, result.upkeep.base(), result.interest.base(), result.station.base());
	return result;
}

/** Actual producing house coverage, never a passenger or fare forecast. */
struct SurfaceIncomeCoverage {
	uint town0 = 0; ///< Newly reached Town0producing house tiles outside all three old catchments.
	uint town1 = 0; ///< Newly reached Town1producing house tiles outside all three old catchments.
	uint all_town1 = 0; ///< All Town1producers in this candidate, protecting the extra bus's current Town0-only contract.
};

/** Count only actual unserved native house production around one real road tile.
 * @param tile Proposed connected drive-through stop.
 * @return Coverage counts excluding every currently served17/18/20house tile.
 */
static SurfaceIncomeCoverage SurfaceIncomeNewCoverage(TileIndex tile)
{
	SurfaceIncomeCoverage result; const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"});
	// A new member must add real producing houses rather than counting a town
	// population total or passengers already owned by an existing station.
	for (const auto house : TileArea(tile, 1, 1).Expand(CA_BUS)) {
		if (!IsTileType(house, TileType::House)) continue;
		if (GetProductionAroundTiles(house, 1, 1, 0)[passenger] == 0) continue;
		const auto town = GetTownIndex(house);
		if (town == TownID{1}) ++result.all_town1;
		bool covered = false;
		for (const auto id : {StationID{17}, StationID{18}, StationID{20}}) {
			if (Station::Get(id)->TileIsInCatchment(house)) covered = true;
		}
		if (covered) continue;
		if (town == TownID{0}) ++result.town0;
		if (town == TownID{1}) ++result.town1;
	}
	return result;
}

/** Query lawful new passenger coverage and the full next operating bill without spending or advancing time. */
static void QuerySurfaceIncomeCapitalization()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{95487});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"}); const auto connected = SurfaceRegionalLayout::DepotComponent();
	for (const auto id : {TownID{0}, TownID{1}}) {
		const auto *town = Town::Get(id);
		fmt::print("SURFACE_RESUPPLY_INCOME_AUTHORITY town={} rating={} center={}\n", id.base(), town->ratings[_current_company], town->xy.base());
	}
	TileIndex selected = INVALID_TILE; DiagDirection orientation = DiagDirection::NE; StationID join = NEW_STATION;
	SurfaceIncomeCoverage best; Money infrastructure{0}; uint attempts = 0, accepted = 0, refused = 0; uint error = 0;
	// Reuse only the actual reciprocal existing-road component. Joining20keeps
	// all original regional routes; a standalone Town0-only catchment can use
	// the already owned extra bus without changing any original useful role.
	for (const auto tile : connected) {
		if (!IsNormalRoadTile(tile)) continue;
		const auto bits = SurfaceRegionalLayout::Bits(tile);
		if (bits != ROAD_X && bits != ROAD_Y) continue; // Preserve current junctions and every existing road connection.
		const auto coverage = SurfaceIncomeNewCoverage(tile);
		if (coverage.town0 + coverage.town1 == 0) continue;
		if (GetAcceptanceAroundTiles(tile, 1, 1, CA_BUS).first[passenger] < 8) continue;
		const auto direction = bits == ROAD_X ? DiagDirection::NE : DiagDirection::SE;
		for (const auto candidate_join : {StationID{20}, NEW_STATION}) {
			if (candidate_join == NEW_STATION) {
				// A separate route may reuse only the existing extra Town0bus;
				// mixed-source cargo needs its own later explicit owner contract.
				if (coverage.town0 == 0) continue;
				if (coverage.all_town1 != 0) continue;
			}
			++attempts;
			const auto quote = QuoteSurfaceResupply<Commands::BuildRoadStop>(tile, tile, uint8_t{1}, uint8_t{1}, RoadStopType::Bus,
				true, direction, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, candidate_join, true);
			if (quote.Failed()) { ++refused; error = quote.GetErrorMessage().base(); continue; }
			++accepted; REQUIRE(quote.GetCost() >= 0);
			const auto score = coverage.town0 + coverage.town1, previous = best.town0 + best.town1;
			// Prefer the largest real new producing coverage, then its smaller
			// whole native invoice. Future passengers and fares remain uncounted.
			if (score < previous || (score == previous && quote.GetCost() >= infrastructure)) continue;
			selected = tile; orientation = direction; join = candidate_join; best = coverage; infrastructure = quote.GetCost();
		}
	}
	fmt::print("SURFACE_RESUPPLY_INCOME_COVERAGE tile={} direction={} join={} new_town0={} new_town1={} all_town1={} infrastructure={} attempts={} accepted={} refused={} last_error={} connected={} purchases=0 changed_orders=0\n",
		selected.base(), to_underlying(orientation), join.base(), best.town0, best.town1, best.all_town1, infrastructure.base(), attempts, accepted, refused, error, connected.size());
	const auto work = QuoteSurfaceFixedPriceIncome(3000); const auto operating = work.upkeep + work.interest + work.station;
	// Both reserve policies remain explicit. A refused construction proposal
	// leaves only a newly priced unchanged-service cash-stop interval; it does
	// not supply authority, an asset, guaranteed profit or restored working cash.
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto target = reserve + infrastructure + operating;
		fmt::print("SURFACE_RESUPPLY_INCOME_ADMISSION reserve={} infrastructure={} operating={} target={} cash={} funded={} coverage_admitted={} ticks=3000 projected_income=0 refunds_counted=0 credit=0 working_shortfall={} module_base_shortfall={}\n",
			reserve.base(), infrastructure.base(), operating.base(), target.base(), company->money.base(), company->money >= target, selected != INVALID_TILE,
			std::max(Money{0}, Money{100000} - company->money).base(), std::max(Money{0}, Money{510000} - company->money).base());
	}
}
#endif /* TEST_SURFACE_ENTRY_INCOME_CAPITALIZATION_H */
