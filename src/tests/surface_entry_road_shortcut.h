/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_road_shortcut.h Pure native road-detour and complete shortcut admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGL.
 * Temporary graphs rank ordinary dry-road proposals. Public native commands
 * alone own construction rights/prices; this query supplies no map or income.
 */
#ifndef TEST_SURFACE_ENTRY_ROAD_SHORTCUT_H
#define TEST_SURFACE_ENTRY_ROAD_SHORTCUT_H
#include "../road_cmd.h"
#include "../tunnelbridge_map.h"

/** Native reciprocal static road graph; no proposal owns or modifies its tiles. */
using SurfaceShortcutGraph = std::map<TileIndex, std::vector<std::pair<TileIndex, uint>>>;

/** Read every actual reciprocal adjacency, retaining native bay/depot/crossing bits.
 * @return Current bidirectional graph including weighted canonical existing bridge/tunnel connections.
 */
static SurfaceShortcutGraph ReadSurfaceShortcutGraph()
{
	SurfaceShortcutGraph graph; std::queue<TileIndex> pending; std::set<TileIndex> seen{TileIndex{2443}}; pending.push(TileIndex{2443});
	// Native bridge heads connect through their canonical opposite endpoint,
	// not to ground tiles underneath the span. Weighted physical lengths keep
	// an existing bridge from becoming a false detour or an imaginary shortcut.
	while (!pending.empty()) {
		const auto tile = pending.front(); pending.pop();
		if (IsNormalRoadTile(tile)) REQUIRE(GetDisallowedRoadDirections(tile).None());
		const bool structure = IsTileType(tile, TileType::TunnelBridge);
		if (structure) REQUIRE(GetTunnelBridgeTransportType(tile) == TransportType::Road);
		auto &neighbors = graph[tile]; const auto bits = GetAnyRoadBits(tile, RoadTramType::Road);
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			if ((bits & DiagDirToRoadBits(direction)) == RoadBits{}) continue;
			const bool crossing = structure && GetTunnelBridgeDirection(tile) == direction;
			const auto next = crossing ? GetOtherTunnelBridgeEnd(tile) : SurfaceChemicalLayout::Step(tile, direction);
			if (next == INVALID_TILE) continue;
			REQUIRE(MultiLayerMap::IsTileInLayer(next, MapLayer::Surface));
			// A ground road cannot enter the inside of an abutment. That side
			// belongs only to the canonical bridge/tunnel jump above.
			if (!crossing && IsTileType(next, TileType::TunnelBridge) && GetTunnelBridgeDirection(next) != direction) continue;
			if ((GetAnyRoadBits(next, RoadTramType::Road) & DiagDirToRoadBits(ReverseDiagDir(direction))) == RoadBits{}) continue;
			neighbors.emplace_back(next, DistanceManhattan(tile, next));
			if (seen.insert(next).second) pending.push(next);
		}
	}
	return graph;
}

/** Compute weighted static road distances, separate from traffic, elapsed time or fares.
 * @param graph Same-world actual reciprocal graph.
 * @param source Actual source/receiver endpoint.
 * @return Shortest static edge count to each connected native tile.
 */
static std::map<TileIndex, uint> SurfaceShortcutDistances(const SurfaceShortcutGraph &graph, TileIndex source)
{
	REQUIRE(graph.contains(source)); std::map<TileIndex, uint> distances{{source, 0}};
	std::priority_queue<std::pair<uint, TileIndex>, std::vector<std::pair<uint, TileIndex>>, std::greater<>> pending; pending.emplace(0, source);
	// Dijkstra retains actual bridge/tunnel span lengths while ordinary road
	// edges cost one. Coordinate proximity cannot supply an unbuilt connection.
	while (!pending.empty()) {
		const auto [distance, tile] = pending.top(); pending.pop(); if (distance != distances.at(tile)) continue;
		for (const auto &[next, length] : graph.at(tile)) {
			const auto proposed = distance + length;
			// A stale or longer route cannot replace a shorter physical path.
			if (distances.contains(next) && distances.at(next) <= proposed) continue;
			distances[next] = proposed; pending.emplace(proposed, next);
		}
	}
	REQUIRE(distances.size() == graph.size()); return distances;
}

/** Complete priced proposal; a record is neither a road nor spendable capitalization. */
struct SurfaceRoadShortcut {
	bool found = false; ///< Whether an ordinary native command accepted a useful candidate.
	TileIndex first = INVALID_TILE; ///< Existing normal-road start junction.
	TileIndex last = INVALID_TILE; ///< Existing normal-road end junction.
	Axis axis = Axis::X; ///< Native same-axis public road construction direction.
	Money invoice{0}; ///< Entire native route price including both endpoint joins.
	uint length = 0; ///< Actual straight adjacent-edge count, at most48.
	uint old_bus = 0, bus = 0; ///< Existing and conservative proposed bus edge counts.
	uint old_goods = 0, goods = 0; ///< Existing and conservative proposed Goods edge counts.
	uint saving = 0; ///< Bus plus two Goods services' conservative static saved edges.
};

/** Rank accepted useful proposals without counting any projected payment.
 * @param candidate Current complete native quote and static route benefit.
 * @param previous Earlier accepted proposal, or an empty record.
 * @return More saved edges, then cheaper native invoice, then deterministic tile order.
 */
static bool PreferSurfaceRoadShortcut(const SurfaceRoadShortcut &candidate, const SurfaceRoadShortcut &previous)
{
	if (!previous.found) return true;
	if (candidate.saving != previous.saving) return candidate.saving > previous.saving;
	if (candidate.invoice != previous.invoice) return candidate.invoice < previous.invoice;
	return std::pair{candidate.first, candidate.last} < std::pair{previous.first, previous.last};
}

/** Enumerate one straight native adjacent corridor without crossing any excluded tile type.
 * @param first Existing normal-road start, in canonical tile order.
 * @param last Existing same-axis normal-road end.
 * @return Actual corridor including both ends, or empty when scope/geometry forbids it.
 */
static std::vector<TileIndex> SurfaceShortcutCorridor(TileIndex first, TileIndex last)
{
	const auto length = DistanceManhattan(first, last); DiagDirection selected = DiagDirection::NE; bool found = false;
	// The layer-aware native step chooses the physical axis. No ad hoc plane
	// offset or packed index arithmetic can authorize road geometry.
	for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
		const auto next = SurfaceChemicalLayout::Step(first, direction); if (next == INVALID_TILE) continue;
		if (DistanceManhattan(next, last) != length - 1) continue;
		selected = direction; found = true; break;
	}
	REQUIRE(found); std::vector<TileIndex> tiles; auto tile = first;
	// Only clear land, trees and ordinary road joins belong to this proposal.
	// Houses, stations, depots, rail, water, bridges and portals remain excluded.
	for (uint index = 0; index <= length; ++index) {
		if (tile == INVALID_TILE) return {};
		if (!IsNormalRoadTile(tile) && !IsTileType(tile, TileType::Clear) && !IsTileType(tile, TileType::Trees)) return {};
		tiles.push_back(tile); if (index != length) tile = SurfaceChemicalLayout::Step(tile, selected);
	}
	REQUIRE(tiles.back() == last); return tiles;
}

/** Quote every scoped useful corridor, preferring an actually funded proposal.
 * @param graph Actual unchanged reciprocal road component.
 * @param max_invoice Present cash after reserving40000and the full2000operating bill.
 * @return Best funded accepted route, otherwise the best actual unfunded quote/refusal.
 */
static SurfaceRoadShortcut SelectSurfaceRoadShortcut(const SurfaceShortcutGraph &graph, Money max_invoice)
{
	const auto bs = SurfaceShortcutDistances(graph, Station::Get(StationID{17})->xy), bd = SurfaceShortcutDistances(graph, TileIndex{1055});
	const auto gs = SurfaceShortcutDistances(graph, Station::Get(StationID{19})->xy), gd = SurfaceShortcutDistances(graph, TileIndex{731});
	const uint old_bus = bs.at(TileIndex{1055}), old_goods = gs.at(TileIndex{731}); SurfaceRoadShortcut accepted, funded;
	uint candidates = 0, refused = 0, excluded = 0, last_error = 0;
	// Same-axis pairs propose a single bounded shortcut. Rank only actual
	// static savings for the paid bus and two existing Goods trucks; this is
	// conservative endpoint-only geometry, never promised travel time or fare.
	for (const auto &[first, first_neighbors] : graph) {
		if (!IsNormalRoadTile(first)) continue;
		for (const auto &[last, last_neighbors] : graph) {
			if (last <= first) continue;
			if (!IsNormalRoadTile(last)) continue;
			// A single public straight-road command cannot authorize a diagonal
			// bend or arbitrary connection across a second native storage plane.
			if (TileX(first) != TileX(last) && TilePlaneY(first) != TilePlaneY(last)) continue;
			const uint length = DistanceManhattan(first, last);
			// Exclude adjacent/no-op joins and horizons beyond the admitted48edges.
			if (length < 2 || length > 48) continue;
			const uint bus = std::min({old_bus, bs.at(first) + length + bd.at(last), bs.at(last) + length + bd.at(first)});
			const uint goods = std::min({old_goods, gs.at(first) + length + gd.at(last), gs.at(last) + length + gd.at(first)});
			const uint saving = old_bus - bus + 2 * (old_goods - goods); if (saving == 0) continue;
			if (SurfaceShortcutCorridor(first, last).empty()) { ++excluded; continue; }
			++candidates; const auto axis = TilePlaneY(first) == TilePlaneY(last) ? Axis::X : Axis::Y;
			// The public strict drag mode rejects the whole proposal on a blocked
			// tile. Normal interactive dragging may instead quote a partial road,
			// which cannot establish this complete shortcut's cost or connectivity.
			const auto quote = QuoteSurfaceResupply<Commands::BuildRoadLong>(first, last, first, ROADTYPE_ROAD, axis, DisallowedRoadDirections{}, false, false, true);
			if (quote.Failed()) { ++refused; last_error = quote.GetErrorMessage().base(); continue; }
			REQUIRE(quote.GetCost() >= Money{0}); SurfaceRoadShortcut candidate{true, first, last, axis, quote.GetCost(), length, old_bus, bus, old_goods, goods, saving};
			if (PreferSurfaceRoadShortcut(candidate, accepted)) accepted = candidate;
			// The full operating bill/floor is already withheld from max_invoice.
			// A better unaffordable route cannot hide a smaller genuinely funded one.
			if (candidate.invoice <= max_invoice && PreferSurfaceRoadShortcut(candidate, funded)) funded = candidate;
		}
	}
	fmt::print("SURFACE_RESUPPLY_SHORTCUT_SEARCH nodes={} old_bus={} old_goods={} candidates={} refused={} excluded={} error={} funded_candidate={} max_invoice={}\n",
		graph.size(), old_bus, old_goods, candidates, refused, excluded, last_error, funded.found, max_invoice.base());
	return funded.found ? funded : accepted;
}

/** Pure native road-detour query and complete actual shortcut/operating capitalization. */
static void QuerySurfaceRoadShortcut()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{73667}); REQUIRE(company->current_loan == Money{5000000});
	const auto *bus = VerifySurfaceIntertown(); const auto graph = ReadSurfaceShortcutGraph();
	QuoteSurfaceFixedPriceIncome(2000); const auto work = QuoteSurfaceModuleCapitalWork(0, 2000); const auto operating = work.upkeep + work.interest + work.station;
	const auto proposal = SelectSurfaceRoadShortcut(graph, company->money - Money{40000} - operating);
	uint bridges = 0;
	for (const auto &[tile, neighbors] : graph) for (const auto &[next, length] : neighbors) {
		// Count each existing structural connection once; no new bridge or
		// tunnel is proposed, priced or supplied by this ordinary-road query.
		if (tile < next && length > 1) ++bridges;
	}
	fmt::print("SURFACE_RESUPPLY_SHORTCUT_SOURCE vehicle=9 held={} source=17 bus_receiver=1055 goods_source=19 goods_receiver=731 nodes={} one_way=0 bridge_jumps={} purchases=0 work=0\n", bus->cargo.TotalCount(), graph.size(), bridges);
	// Emit actual unique adjacencies for independent mathematical path checks;
	// these diagnostic records create no second map or cargo authority.
	for (const auto &[tile, neighbors] : graph) for (const auto &[next, length] : neighbors) {
		if (tile < next) fmt::print("SURFACE_RESUPPLY_SHORTCUT_EDGE first={} last={} length={}\n", tile.base(), next.base(), length);
	}
	fmt::print("SURFACE_RESUPPLY_SHORTCUT_PROPOSAL found={} first={} last={} axis={} length={} invoice={} old_bus={} bus={} old_goods={} goods={} saving={} strict_drag=true paid=false changed_map=false\n",
		proposal.found, proposal.first.base(), proposal.last.base(), to_underlying(proposal.axis), proposal.length, proposal.invoice.base(), proposal.old_bus, proposal.bus, proposal.old_goods, proposal.goods, proposal.saving);
	if (!proposal.found) return;
	uint index = 0;
	for (const auto tile : SurfaceShortcutCorridor(proposal.first, proposal.last)) fmt::print("SURFACE_RESUPPLY_SHORTCUT_TILE index={} tile={}\n", index++, tile.base());
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto target = reserve + proposal.invoice + operating;
		fmt::print("SURFACE_RESUPPLY_SHORTCUT_ADMISSION ticks=2000 days={} months={} year_boundary={} operating={} invoice={} reserve={} target={} cash={} funded={} future_income_counted=0 refunds_counted=0 credit=0\n",
			work.days, work.months, work.year_boundary, operating.base(), proposal.invoice.base(), reserve.base(), target.base(), company->money.base(), company->money >= target);
	}
}
#endif /* TEST_SURFACE_ENTRY_ROAD_SHORTCUT_H */
