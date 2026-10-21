/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_farm_feeder.h Pure complete short Farm7-to-original-rail15 feeder admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGP.
 * Native commands retain geometry, authority, prices and cargo ownership.
 * A quote or virtual feeder credit supplies neither company cash nor cargo.
 */
#ifndef TEST_SURFACE_ENTRY_FARM_FEEDER_H
#define TEST_SURFACE_ENTRY_FARM_FEEDER_H
#include "../industry_map.h"
#include "../station_func.h"

/** Temporary whole native proposal, never a second map or station owner. */
struct SurfaceFarmFeederQuote {
	bool found = false; ///< Whether all physical and native estimates passed.
	TileIndex pickup = INVALID_TILE, transfer = INVALID_TILE; ///< New pickup and original15joined transfer bays.
	DiagDirection entrance = DiagDirection::NE; ///< Both bays' actual road-facing entrances.
	TileIndex first = INVALID_TILE, last = INVALID_TILE, anchor = INVALID_TILE; ///< Whole straight road and existing component join.
	Axis axis = Axis::X; ///< Native complete-drag axis.
	Money pickup_cost{0}, transfer_cost{0}, road_cost{0}; ///< All unpaid native invoices.
	uint clearing = 0; ///< Conservative whole proposal clearing-token requirement.
};

/** Check bare Surface scope without demolishing housing, industry, railway or water.
 * @param tile Actual proposed bay or road tile.
 * @return Whether native clear land or trees occupy this same-world tile.
 */
static bool SurfaceFarmFeederBare(TileIndex tile)
{
	if (tile == INVALID_TILE) return false;
	if (!MultiLayerMap::IsTileInLayer(tile, MapLayer::Surface)) return false;
	return IsTileType(tile, TileType::Clear) || IsTileType(tile, TileType::Trees);
}

/** Require actual native pickup catchment to contain only Farm7Livestock production.
 * @param tile Unchanged proposed bay tile.
 * @param livestock Original native cargo identifier.
 * @return Whether physical industry identity and native cargo production agree.
 */
static bool SurfaceFarmFeederCatchment(TileIndex tile, CargoType livestock)
{
	const auto radius = _settings_game.station.modified_catchment ? CA_TRUCK : CA_UNMODIFIED;
	if (GetProductionAroundTiles(tile, 1, 1, radius)[livestock] == 0) return false;
	std::set<IndustryID> producers;
	// Cargo labels cannot admit another farm into the original Source7cohort.
	// Inspect each actual industry touched by the real truck catchment.
	for (const auto nearby : TileArea(tile, 1, 1).Expand(radius)) {
		if (!IsTileType(nearby, TileType::Industry)) continue;
		const auto id = GetIndustryIndex(nearby);
		for (const auto &output : Industry::Get(id)->produced) if (output.cargo == livestock) producers.insert(id);
	}
	return producers == std::set<IndustryID>{IndustryID{7}};
}

/** Price a native bay with actual tile authority and explicit membership.
 * @param tile Bare unchanged Surface tile.
 * @param direction Real bay entrance direction.
 * @param station NEW_STATION for pickup, original15for transfer.
 * @return Full native price or unchanged authority/geometry refusal.
 */
static CommandCost QuoteSurfaceFarmFeederBay(TileIndex tile, DiagDirection direction, StationID station)
{
	return QuoteSurfaceResupply<Commands::BuildRoadStop>(tile, tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck,
		false, direction, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, station, true);
}

/** Price every complete road-bit union, including both actual bay entrance spurs.
 * @param corridor Entire scoped same-axis road including the existing component join.
 * @param axis Native straight-road axis.
 * @param mouth Pickup bay's real adjacent road tile.
 * @param other_mouth Transfer bay's real adjacent road tile.
 * @param entrance Direction from each bay to its mouth.
 * @return Sum of per-tile native complete unions, or a literal native refusal.
 */
static CommandCost QuoteSurfaceFarmFeederRoad(const std::vector<TileIndex> &corridor, Axis axis, TileIndex mouth, TileIndex other_mouth, DiagDirection entrance)
{
	CommandCost total(ExpensesType::Construction);
	// A straight road alone lacks the two bay-facing branches. Price each
	// final union once, so future construction cannot depend on ghost spurs
	// or double-count overlapping road/clearing/foundation invoices.
	for (const auto tile : corridor) {
		auto bits = AxisToRoadBits(axis);
		// Each bay must actually connect to its own mouth before the truck
		// can make the three-edge trip; both parallel branches are essential.
		if (tile == mouth || tile == other_mouth) bits |= DiagDirToRoadBits(ReverseDiagDir(entrance));
		if (IsNormalRoadTile(tile)) {
			if (GetDisallowedRoadDirections(tile).Any()) return CMD_ERROR;
			const auto current = GetRoadBits(tile, RoadTramType::Road);
			if ((current | bits) == current) continue;
		}
		const auto quote = QuoteSurfaceResupply<Commands::BuildRoad>(tile, tile, bits, ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid());
		if (quote.Failed()) return quote;
		total.AddCost(quote.GetCost());
	}
	return total;
}

/** Search adjacent parallel bays and one complete24edge maximum dry road.
 * @param graph Actual unchanged depot-connected reciprocal road graph.
 * @return Cheapest fully quoted proposal, or literal no candidate; no payments or ticks.
 */
static SurfaceFarmFeederQuote SelectSurfaceFarmFeeder(const SurfaceShortcutGraph &graph)
{
	SurfaceFarmFeederQuote selected; Money cheapest{INT64_MAX}; const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"});
	uint pickups = 0, pickup_refused = 0, transfers = 0, transfer_refused = 0, roads = 0, road_refused = 0, excluded = 0, quota_refused = 0, error = 0;
	const auto radius = _settings_game.station.modified_catchment ? CA_TRUCK : CA_UNMODIFIED;
	const auto quota = Company::Get(_current_company)->clear_limit >> 16;
	// The real farm footprint bounds the search. Adjacent parallel bays
	// create a three-edge trip; no speculative remote source expands scope.
	for (const auto pickup : TileArea(Industry::Get(IndustryID{7})->location).Expand(radius)) {
		if (!SurfaceFarmFeederBare(pickup)) continue;
		if (!SurfaceFarmFeederCatchment(pickup, livestock)) continue;
		for (const auto entrance : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			++pickups; const auto pickup_quote = QuoteSurfaceFarmFeederBay(pickup, entrance, NEW_STATION);
			if (pickup_quote.Failed()) { ++pickup_refused; error = pickup_quote.GetErrorMessage().base(); continue; }
			const auto mouth = SurfaceChemicalLayout::Step(pickup, entrance); if (mouth == INVALID_TILE) continue;
			// Perpendicular bay separation puts both actual mouths on the same
			// single priced straight road, without an unpriced connecting bend.
			for (const auto side : {ChangeDiagDir(entrance, DiagDirDiff::Right90), ChangeDiagDir(entrance, DiagDirDiff::Left90)}) {
				const auto transfer = SurfaceChemicalLayout::Step(pickup, side); if (!SurfaceFarmFeederBare(transfer)) continue;
				++transfers; const auto transfer_quote = QuoteSurfaceFarmFeederBay(transfer, entrance, StationID{15});
				if (transfer_quote.Failed()) { ++transfer_refused; error = transfer_quote.GetErrorMessage().base(); continue; }
				const auto other_mouth = SurfaceChemicalLayout::Step(transfer, entrance); if (other_mouth == INVALID_TILE) continue;
				for (const auto direction : {side, ReverseDiagDir(side)}) {
					auto anchor = mouth;
					// Stop at the first existing component connection within24edges.
					// Every intermediate tile must remain ordinary dry-road scope.
					for (uint length = 0; length <= 24; ++length) {
						if (anchor == INVALID_TILE) break;
						if (!IsNormalRoadTile(anchor) && !SurfaceFarmFeederBare(anchor)) { ++excluded; break; }
						if (graph.contains(anchor)) {
							const auto first = std::min({mouth, other_mouth, anchor}), last = std::max({mouth, other_mouth, anchor});
							const auto corridor = SurfaceShortcutCorridor(first, last); if (corridor.empty()) { ++excluded; break; }
							uint clearing = 2;
							// Reserve tokens for both bays and each non-road tile,
							// even when native automatic clearing may cost no token.
							for (const auto tile : corridor) if (!IsNormalRoadTile(tile)) ++clearing;
							if (clearing > quota) { ++quota_refused; break; }
							const auto axis = TilePlaneY(first) == TilePlaneY(last) ? Axis::X : Axis::Y;
							++roads; const auto road_quote = QuoteSurfaceResupply<Commands::BuildRoadLong>(first, last, first, ROADTYPE_ROAD, axis, DisallowedRoadDirections{}, false, true, true);
							if (road_quote.Failed()) { ++road_refused; error = road_quote.GetErrorMessage().base(); break; }
							const auto complete = QuoteSurfaceFarmFeederRoad(corridor, axis, mouth, other_mouth, entrance);
							if (complete.Failed()) { ++road_refused; error = complete.GetErrorMessage().base(); break; }
							const auto total = pickup_quote.GetCost() + transfer_quote.GetCost() + complete.GetCost();
							// Fixed trip length makes the cheapest whole native invoice
							// the best cash admission; equal prices retain tile order.
							if (total < cheapest) {
								cheapest = total; selected = {true, pickup, transfer, entrance, first, last, anchor, axis,
									pickup_quote.GetCost(), transfer_quote.GetCost(), complete.GetCost(), clearing};
							}
							break;
						}
						anchor = SurfaceChemicalLayout::Step(anchor, direction);
					}
				}
			}
		}
	}
	fmt::print("SURFACE_RESUPPLY_FARM_FEEDER_SEARCH nodes={} pickups={} pickup_refused={} transfers={} transfer_refused={} roads={} road_refused={} excluded={} quota_refused={} quota={} error={} found={} paid=0 ticks=0\n",
		graph.size(), pickups, pickup_refused, transfers, transfer_refused, roads, road_refused, excluded, quota_refused, quota, error, selected.found);
	return selected;
}

/** Query the complete actual investment, preserving original owners and physical time. */
static void QuerySurfaceFarmFeeder()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{60955}); REQUIRE(company->current_loan == Money{5000000});
	VerifySurfaceFarmRoadStops(); const auto *truck = VerifySurfaceFarmRoadIncome(); REQUIRE(truck->index == VehicleID{62}); REQUIRE(truck->cargo.TotalCount() == 0);
	VerifySurfaceFarmRebalance(true); VerifySurfaceIntertown(); QuoteSurfaceFixedPriceIncome(2000);
	const auto work = QuoteSurfaceModuleCapitalWork(0, 2000); const auto operating = work.upkeep + work.interest + work.station;
	const auto proposal = SelectSurfaceFarmFeeder(ReadSurfaceShortcutGraph());
	fmt::print("SURFACE_RESUPPLY_FARM_FEEDER_SOURCE vehicle=62 engine=137 capacity=18 held=0 industry=7 farm_tile={} farm_width={} farm_height={} transfer_station=15 rail=52 final_receiver=13 new_vehicle=0 paid=0 ticks=0 feeder_credit_is_cash=false\n",
		Industry::Get(IndustryID{7})->location.tile.base(), Industry::Get(IndustryID{7})->location.w, Industry::Get(IndustryID{7})->location.h);
	fmt::print("SURFACE_RESUPPLY_FARM_FEEDER_PROPOSAL found={} pickup={} transfer={} entrance={} first={} last={} anchor={} axis={} pickup_invoice={} transfer_invoice={} road_invoice={} clearing={} distance=3 strict_drag=true new_station=true paid=false\n",
		proposal.found, proposal.pickup.base(), proposal.transfer.base(), to_underlying(proposal.entrance), proposal.first.base(), proposal.last.base(), proposal.anchor.base(), to_underlying(proposal.axis),
		proposal.pickup_cost.base(), proposal.transfer_cost.base(), proposal.road_cost.base(), proposal.clearing);
	if (!proposal.found) return;
	// Neither virtual feeder accounting credits nor possible final rail fares
	// contribute to admission of the whole gross invoice and current company bill.
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto invoice = proposal.pickup_cost + proposal.transfer_cost + proposal.road_cost; const auto target = reserve + invoice + operating;
		fmt::print("SURFACE_RESUPPLY_FARM_FEEDER_ADMISSION ticks=2000 days={} months={} year_boundary={} operating={} invoice={} reserve={} target={} cash={} funded={} future_income_counted=0 refunds_counted=0 credit=0\n",
			work.days, work.months, work.year_boundary, operating.base(), invoice.base(), reserve.base(), target.base(), company->money.base(), company->money >= target);
	}
}
#endif /* TEST_SURFACE_ENTRY_FARM_FEEDER_H */
