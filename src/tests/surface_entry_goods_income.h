/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_goods_income.h Pure actual Goods source, road receiver and whole operating admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSG.
 * A native refusal remains a refusal. Actual production and loading own future
 * cargo; neither an unserved factory nor a stationary truck promises income.
 */
#ifndef TEST_SURFACE_ENTRY_GOODS_INCOME_H
#define TEST_SURFACE_ENTRY_GOODS_INCOME_H

/** Discover reciprocal existing Surface roads including actual depots, crossings and bay stops.
 * @param start Existing owned road-depot tile, never a newly supplied connection.
 * @return Physically reciprocal component; no road, portal or bridge is constructed.
 */
static std::set<TileIndex> SurfaceGoodsRoadComponent(TileIndex start)
{
	std::set<TileIndex> result{start}; std::queue<TileIndex> pending; pending.push(start);
	// Coordinate proximity cannot supply connectivity. Include the real native
	// bay/depot mouth and level-crossing bits missing from the old bus-only scan.
	while (!pending.empty()) {
		const auto tile = pending.front(); pending.pop(); const auto bits = GetAnyRoadBits(tile, RoadTramType::Road);
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			if ((bits & DiagDirToRoadBits(direction)) == RoadBits{}) continue;
			const auto next = SurfaceChemicalLayout::Step(tile, direction); if (next == INVALID_TILE) continue;
			if (result.contains(next)) continue;
			if ((GetAnyRoadBits(next, RoadTramType::Road) & DiagDirToRoadBits(ReverseDiagDir(direction))) == RoadBits{}) continue;
			result.insert(next); pending.push(next);
		}
	}
	return result;
}

/** Quote actual idle Goods bodies, native lawful receivers and complete first operating costs without mutation. */
static void QuerySurfaceGoodsIncome()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{96397});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}); const auto *source = Station::Get(StationID{19});
	const auto component = SurfaceGoodsRoadComponent(TileIndex{2443});
	const auto *factory = Industry::Get(IndustryID{11});
	fmt::print("SURFACE_RESUPPLY_GOODS_SOURCE station=19 tile={} factory=11 factory_tile={} waiting={} source_connected={} passenger_depot_connected={} roads={} original_goods_head=49 original_source=12 original_receiver=11 purchases=0\n",
		source->xy.base(), factory->location.tile.base(), source->goods[goods].AvailableCount(), component.contains(source->xy),
		component.contains(SurfaceRegionalLayout::depot), component.size());
	// Public current buffers describe the producer only. Historical production,
	// future Steel deliveries and dispatch opportunities cannot fund this quote.
	for (const auto &item : factory->produced) {
		fmt::print("SURFACE_RESUPPLY_GOODS_PRODUCTION cargo={} waiting={} rate={}\n", item.cargo, item.waiting, item.rate);
	}
	for (const auto &item : factory->accepted) {
		fmt::print("SURFACE_RESUPPLY_GOODS_INPUT cargo={} waiting={}\n", item.cargo, item.waiting);
	}
	Money extra{0}; uint capacity = 0;
	constexpr uint64_t denominator = uint64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS * 256;
	// Reusing only these two real useful empty reserves needs no asset purchase.
	// Their complete future running cost is added despite their stopped state.
	for (const auto id : {VehicleID{3}, VehicleID{25}}) {
		const auto *truck = RoadVehicle::Get(id); REQUIRE(truck->owner == _current_company);
		REQUIRE(truck->engine_type == EngineID{140}); REQUIRE(truck->cargo_type == goods); REQUIRE(truck->cargo_cap == 18);
		REQUIRE(truck->cargo.TotalCount() == 0); REQUIRE(truck->vehstatus.Test(VehState::Stopped)); REQUIRE(truck->tile == TileIndex{2443});
		REQUIRE(truck->GetEngine()->GetGRF() == nullptr); REQUIRE(truck->max_age.base() - truck->age.base() > 300);
		const auto annual = truck->GetRunningCost().base(); REQUIRE(annual >= 0);
		const auto bill = Money{static_cast<int64_t>((static_cast<uint64_t>(annual) * (3000 + truck->running_ticks) + denominator - 1) / denominator) + 1};
		extra += bill; capacity += truck->cargo_cap;
		fmt::print("SURFACE_RESUPPLY_GOODS_RESERVE vehicle={} engine=140 capacity=18 remaining={} annual256={} future_upkeep={} callback_free=true asset_invoice=0\n",
			id.base(), truck->max_age.base() - truck->age.base(), annual, bill.base());
	}
	TileIndex selected = INVALID_TILE; Money infrastructure{0}; uint attempts = 0, accepted = 0, refused = 0, error = 0, distance = 0;
	// A Goods receiver must be genuinely reachable by the original idle trucks
	// and legally accept native cargo. Every existing building and road survives.
	for (const auto tile : component) {
		if (!component.contains(source->xy)) break;
		if (!IsNormalRoadTile(tile)) continue;
		const auto bits = GetRoadBits(tile, RoadTramType::Road);
		if (bits != ROAD_X && bits != ROAD_Y) continue; // A new straight stop must preserve every existing road junction.
		if (GetAcceptanceAroundTiles(tile, 1, 1, CA_TRUCK).first[goods] < 8) continue;
		++attempts;
		const auto quote = QuoteSurfaceResupply<Commands::BuildRoadStop>(tile, tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck,
			true, bits == ROAD_X ? DiagDirection::NE : DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, StationID{20}, true);
		if (quote.Failed()) { ++refused; error = quote.GetErrorMessage().base(); continue; }
		++accepted; const auto span = DistanceManhattan(source->xy, tile);
		// Prefer a longer genuinely admitted delivery distance, then its lower
		// invoice. Distance never becomes a fare or a profitability guarantee.
		if (span < distance || (span == distance && quote.GetCost() >= infrastructure)) continue;
		selected = tile; distance = span; infrastructure = quote.GetCost();
	}
	fmt::print("SURFACE_RESUPPLY_GOODS_RECEIVER tile={} station=20 distance={} infrastructure={} attempts={} accepted={} refused={} last_error={} added_capacity={} changed_orders=0\n",
		selected.base(), distance, infrastructure.base(), attempts, accepted, refused, error, capacity);
	const auto work = QuoteSurfaceFixedPriceIncome(3000); const auto operating = work.upkeep + work.interest + work.station + extra;
	// Both cash policies price the entire first interval. Connectivity/civic
	// refusals stay explicit even when the hypothetical cost-only sum fits cash.
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto target = reserve + infrastructure + operating;
		const bool admitted = selected != INVALID_TILE && company->money >= target;
		fmt::print("SURFACE_RESUPPLY_GOODS_ADMISSION reserve={} infrastructure={} asset_invoice=0 extra_upkeep={} operating={} target={} cash={} cash_funded={} receiver_admitted={} funded={} ticks=3000 projected_fares=0 refunds_counted=0 credit=0 working_shortfall=3603 module_base_shortfall=413603\n",
			reserve.base(), infrastructure.base(), extra.base(), operating.base(), target.base(), company->money.base(), company->money >= target, selected != INVALID_TILE, admitted);
	}
}
#endif /* TEST_SURFACE_ENTRY_GOODS_INCOME_H */
