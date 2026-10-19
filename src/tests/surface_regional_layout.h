/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_regional_layout.h Pure native catchment and road admission for useful regional service.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQA.
 * Ephemeral searches own no roads, passengers, station rights, cash or vehicle pose.
 */
#ifndef TEST_SURFACE_REGIONAL_LAYOUT_H
#define TEST_SURFACE_REGIONAL_LAYOUT_H
#include "surface_passenger_layout.h"
#include "../town_map.h"

/** One complete native inter-town infrastructure proposal, never saved routing authority. */
struct SurfaceRegionalLayout {
	std::array<TileIndex, 2> stops{}; ///< Actual road tiles in the two native town catchments.
	std::array<DiagDirection, 2> directions{}; ///< Freshly admitted native drive-through orientations.
	std::array<uint, 2> production{}; ///< Current catchment production weights; these are not fares.
	std::array<uint, 2> unserved{}; ///< Producing house tiles outside the old local stop catchments.
	std::map<TileIndex, RoadBits> roads; ///< Whole native requested road shapes, excluding existing assets.
	Money invoice{0}; ///< Whole fresh infrastructure quote, excluding the three buses.
	uint edges = 0; ///< Selected physically connected route length.
	static constexpr TileIndex depot{2984}; ///< Existing paid passenger depot, reused without a second purchase.
	static constexpr TileIndex entrance{2920}; ///< Its actual public-road entrance.

	/** @param tile Actual candidate. @return Real road geometry, including existing drive-through passenger stops. */
	static RoadBits Bits(TileIndex tile)
	{
		if (IsNormalRoadTile(tile)) return GetRoadBits(tile, RoadTramType::Road);
		if (IsDriveThroughStopTile(tile)) return AxisToRoadBits(GetDriveThroughStopAxis(tile));
		return {};
	}

	/** @param tile Actual map tile. @return Whether road construction preserves all existing houses, railways and industry. */
	static bool Candidate(TileIndex tile)
	{
		if (tile == INVALID_TILE) return false;
		// Only original roads or genuinely clearable ground may join the towns;
		// existing freight, stations, water and buildings cannot be erased for income.
		return IsNormalRoadTile(tile) || IsTileType(tile, TileType::Clear) || IsTileType(tile, TileType::Trees);
	}

	/** @param tile Actual road or clear ground. @param bits Complete required shape.
	 * @return Native whole construction cost/refusal, without constructing any part.
	 */
	static CommandCost Quote(TileIndex tile, RoadBits bits)
	{
		if (bits == Bits(tile)) return CommandCost{};
		return CmdBuildRoad(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoad>()), tile, bits, ROADTYPE_ROAD, {}, TownID::Invalid());
	}

	/** @param tile Actual producing house. @param stop Existing local stop.
	 * @return Whether that old one-tile bus catchment already covers the house.
	 */
	static bool Covered(TileIndex tile, TileIndex stop)
	{
		// Both coordinate bounds define the native square bus catchment; this
		// prevents calling already served local houses an additional income source.
		return std::abs(int(TileX(tile)) - int(TileX(stop))) <= CA_BUS &&
			std::abs(int(TilePlaneY(tile)) - int(TilePlaneY(stop))) <= CA_BUS;
	}

	/** @param tile Proposed stop. @param town Actual native source town.
	 * @return Producing house tiles genuinely outside both existing local catchments.
	 */
	static uint NewSupply(TileIndex tile, TownID town)
	{
		const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"}); uint result = 0;
		// Inspect actual native house production, rather than deriving a fare or
		// generation promise from town population or station waiting counters.
		for (const auto house : TileArea(tile, 1, 1).Expand(CA_BUS)) {
			if (!IsTileType(house, TileType::House)) continue;
			if (GetTownIndex(house) != town) continue;
			if (Covered(house, TileIndex{2546}) || Covered(house, TileIndex{2789})) continue; // Either local catchment already serves this house.
			if (GetProductionAroundTiles(house, 1, 1, 0)[passenger] != 0) ++result;
		}
		return result;
	}

	/** @return Existing physical road component of the reused depot, preserving the two current stops. */
	static std::set<TileIndex> DepotComponent()
	{
		std::set<TileIndex> result{entrance}; std::queue<TileIndex> pending; pending.push(entrance);
		// Reuse requires real reciprocal road connections; coordinate proximity
		// to a paid depot is not permission to spawn a bus on another component.
		while (!pending.empty()) {
			const auto tile = pending.front(); pending.pop(); const auto bits = Bits(tile);
			for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
				if ((bits & DiagDirToRoadBits(direction)) == RoadBits{}) continue;
				const auto next = SurfaceChemicalLayout::Step(tile, direction); if (next == INVALID_TILE) continue;
				if (result.contains(next)) continue;
				if ((Bits(next) & DiagDirToRoadBits(ReverseDiagDir(direction))) == RoadBits{}) continue;
				result.insert(next); pending.push(next);
			}
		}
		return result;
	}

	/** Select actual useful house catchments and quote every complete route shape without spending money. */
	void Prepare()
	{
		const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"}); const auto connected = DepotComponent();
		REQUIRE(IsRoadDepotTile(depot)); REQUIRE(GetTileOwner(depot) == _current_company);
		REQUIRE(GetRoadDepotDirection(depot) == DiagDirection::NW); REQUIRE(connected.contains(entrance));
		std::array<Money, 2> prices{};
		// The current authority refuses a new Town0stop and there is insufficient
		// native plantable land to recover its rating. Reuse the actual owned
		// terminal instead of paying civic work that cannot admit this service.
		this->stops[0] = TileIndex{2546}; REQUIRE(IsDriveThroughStopTile(this->stops[0]));
		this->directions[0] = GetDriveThroughStopAxis(this->stops[0]) == Axis::X ? DiagDirection::NE : DiagDirection::SE;
		REQUIRE(GetTileOwner(this->stops[0]) == _current_company);
		REQUIRE(GetStationIndex(this->stops[0]) == StationID{17}); REQUIRE(connected.contains(this->stops[0]));
		this->production[0] = GetProductionAroundTiles(this->stops[0], 1, 1, CA_BUS)[passenger]; this->unserved[0] = 0;
		fmt::print("SURFACE_REGIONAL_STOP town=0 tile={} direction={} production={} new_houses=0 cost=0 reused=true station=17\n",
			this->stops[0].base(), to_underlying(this->directions[0]), this->production[0]);
		// A new stop must natively accept passengers and reach producing houses
		// of its actual town. Town0 also needs a real paid route from the old depot.
		for (uint town = 1; town < 2; ++town) {
			uint best = 0, normal = 0, access = 0, maximum = 0, new_maximum = 0, error = 0; this->stops[town] = INVALID_TILE;
			for (const TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
				if (!IsNormalRoadTile(tile)) continue;
				if (SurfacePassengerLayout::Distance(tile, Town::Get(TownID{static_cast<uint16_t>(town)})->xy) > 12) continue;
				++normal; maximum = std::max(maximum, GetProductionAroundTiles(tile, 1, 1, CA_BUS)[passenger]);
				new_maximum = std::max(new_maximum, NewSupply(tile, TownID{static_cast<uint16_t>(town)}));
				if (town == 0 && !connected.contains(tile)) continue; // Existing depot access must be real before route spending.
				++access;
				const auto supply = NewSupply(tile, TownID{static_cast<uint16_t>(town)});
				if (supply <= best) continue;
				if (GetAcceptanceAroundTiles(tile, 1, 1, CA_BUS).first[passenger] < 8) continue;
				for (const auto direction : {DiagDirection::NE, DiagDirection::SE}) {
					const auto quote = SurfacePassengerLayout::StopQuote(tile, direction); if (quote.Failed()) { error = quote.GetErrorMessage().base(); continue; }
					best = supply; this->stops[town] = tile; this->directions[town] = direction;
					this->production[town] = GetProductionAroundTiles(tile, 1, 1, CA_BUS)[passenger];
					this->unserved[town] = supply; prices[town] = quote.GetCost(); break;
				}
			}
			fmt::print("SURFACE_REGIONAL_SEARCH town={} normal={} connected={} maximum={} new_maximum={} error={} depot_component={}\n", town, normal, access, maximum, new_maximum, error, connected.size());
			fmt::print("SURFACE_REGIONAL_STOP town={} tile={} direction={} production={} new_houses={} cost={}\n", town,
				this->stops[town].base(), to_underlying(this->directions[town]), this->production[town], this->unserved[town], prices[town].base());
			REQUIRE(this->stops[town] != INVALID_TILE);
		}
		using State = std::pair<TileIndex, uint>; // Incoming road bit, or4for the original starting road.
		using QueueEntry = std::tuple<uint64_t, TileIndex, uint>;
		std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> pending;
		const State start{this->stops[0], 4}; std::map<State, uint64_t> costs{{start, 0}}; std::map<State, State> previous;
		pending.emplace(0, start.first, start.second); State final{INVALID_TILE, 4};
		// Search actual buildable shapes. Existing paid roads cost no new money;
		// a small distance penalty rejects gratuitous detours without forecasting fares.
		while (!pending.empty()) {
			const auto [cost, tile, incoming] = pending.top(); pending.pop(); const State state{tile, incoming};
			if (cost != costs.at(state)) continue;
			if (tile == this->stops[1]) { final = state; break; }
			for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
				const auto next = SurfaceChemicalLayout::Step(tile, direction);
				if (next != this->stops[0] && !Candidate(next)) continue; // Only the already owned departure terminal may be an existing station tile.
				const auto outgoing = DiagDirToRoadBits(direction), arrival = DiagDirToRoadBits(ReverseDiagDir(direction));
				if (tile == this->stops[0] && (Bits(tile) & outgoing) == RoadBits{}) continue; // Stop geometry stays as originally quoted.
				if (next == this->stops[1] && (Bits(next) & arrival) == RoadBits{}) continue; // Native receiving stop needs its real entrance.
				auto bits = Bits(tile) | outgoing; if (incoming < 4) bits |= DiagDirToRoadBits(static_cast<DiagDirection>(incoming));
				const auto quote = Quote(tile, bits); if (quote.Failed()) continue; REQUIRE(quote.GetCost() >= 0);
				const auto next_cost = cost + static_cast<uint64_t>(quote.GetCost().base()) + 5;
				const State after{next, to_underlying(ReverseDiagDir(direction))};
				if (costs.contains(after) && costs.at(after) <= next_cost) continue; // Keep only the cheapest actual complete road admission.
				costs[after] = next_cost; previous[after] = state; pending.emplace(next_cost, next, after.second);
			}
		}
		REQUIRE(final.first != INVALID_TILE); std::vector<State> reverse{final};
		// Reconstruct only the selected physical route. Full union shapes are
		// queried once per tile so sequential purchases cannot double-count bits.
		while (reverse.back() != start) reverse.push_back(previous.at(reverse.back()));
		std::reverse(reverse.begin(), reverse.end()); this->edges = static_cast<uint>(reverse.size() - 1);
		for (size_t index = 0; index < reverse.size(); ++index) {
			const auto tile = reverse[index].first; auto bits = Bits(tile);
			if (index > 0) bits |= DiagDirToRoadBits(static_cast<DiagDirection>(reverse[index].second));
			if (index + 1 < reverse.size()) bits |= DiagDirToRoadBits(ReverseDiagDir(static_cast<DiagDirection>(reverse[index + 1].second)));
			if (bits == Bits(tile)) continue;
			this->roads[tile] |= bits;
		}
		this->invoice = prices[0] + prices[1];
		for (const auto &[tile, bits] : this->roads) {
			const auto quote = Quote(tile, bits); REQUIRE(quote.Succeeded()); this->invoice += quote.GetCost();
			fmt::print("SURFACE_REGIONAL_ROAD tile={} bits={} cost={}\n", tile.base(), bits.base(), quote.GetCost().base());
		}
		fmt::print("SURFACE_REGIONAL_LAYOUT first={} last={} depot={} entrance={} infrastructure={} roads={} edges={} distance={} crossings=0\n",
			this->stops[0].base(), this->stops[1].base(), depot.base(), entrance.base(), this->invoice.base(), this->roads.size(), this->edges,
			SurfacePassengerLayout::Distance(this->stops[0], this->stops[1]));
	}
};
#endif /* TEST_SURFACE_REGIONAL_LAYOUT_H */
