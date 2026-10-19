/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_passenger_layout.h Pure original-command planning for genuine launch financing.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEB.
 * These temporary quotes own no roads, passenger generation, money or routing.
 */
#ifndef TEST_SURFACE_PASSENGER_LAYOUT_H
#define TEST_SURFACE_PASSENGER_LAYOUT_H
#include "../industry.h"
#include "surface_chemical_layout.h"
#include "../station_func.h"
#include "../economy_func.h"
#include "../town.h"

/** One complete pure existing-road service proposal, before any native purchase. */
struct SurfacePassengerLayout {
	std::array<TileIndex, 2> stops{}; ///< Original drive-through stop anchors.
	std::array<DiagDirection, 2> directions{}; ///< Native stop orientation.
	std::array<uint, 2> production{}; ///< Actual native catchment production weights, not forecast passengers.
	TileIndex depot = INVALID_TILE; ///< Proposed real ordinary empty depot.
	TileIndex entrance = INVALID_TILE; ///< Existing public/paid road next to its door.
	DiagDirection door{}; ///< Depot's native opening.
	RoadBits connections{}; ///< Entire required road shape, quoted once.
	Money invoice{0}; ///< Complete infrastructure estimate, never company money.
	uint route_length = 0; ///< Existing connected road edges, without level crossings.
	uint distance = 0; ///< Actual endpoint Manhattan distance used by native revenue.

	/** @param first Actual Surface endpoint. @param last Actual Surface endpoint. @return Original Manhattan distance. */
	static uint Distance(TileIndex first, TileIndex last)
	{
		return std::abs(int(TileX(first)) - int(TileX(last))) + std::abs(int(TilePlaneY(first)) - int(TilePlaneY(last)));
	}

	/** @param start Existing road endpoint. @return Distances through real connected normal roads, preserving every railway. */
	static std::map<TileIndex, uint> Reach(TileIndex start)
	{
		std::map<TileIndex, uint> result{{start, 0}}; std::queue<TileIndex> pending; pending.push(start);
		// Passenger service stays within the real town road component. Excluding
		// crossings prevents new buses sharing the protected freight railway.
		while (!pending.empty()) {
			const auto tile = pending.front(); pending.pop(); const auto bits = GetRoadBits(tile, RoadTramType::Road);
			for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
				if ((bits & DiagDirToRoadBits(direction)) == RoadBits{}) continue;
				const auto next = SurfaceChemicalLayout::Step(tile, direction);
				if (next == INVALID_TILE) continue;
				// Only existing normal road can carry this bounded service. A
				// proposed station, crossing, house or bridge is not connectivity.
				if (!IsNormalRoadTile(next) || result.contains(next)) continue;
				if ((GetRoadBits(next, RoadTramType::Road) & DiagDirToRoadBits(ReverseDiagDir(direction))) == RoadBits{}) continue;
				result.emplace(next, result.at(tile) + 1); pending.push(next);
			}
		}
		return result;
	}

	/** @param tile Actual road. @param direction Original straight stop orientation. @return Whole original bus-stop quote. */
	static CommandCost StopQuote(TileIndex tile, DiagDirection direction)
	{
		return CmdBuildRoadStop(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoadStop>()), tile, 1, 1, RoadStopType::Bus,
			true, direction, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, true);
	}

	/** Select actual accepting/producing road stops and a genuinely buildable connected depot; never install them. */
	void Prepare()
	{
		const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"});
		std::vector<TileIndex> candidates; std::map<TileIndex, DiagDirection> orientation;
		std::map<TileIndex, uint> weights; std::map<TileIndex, Money> prices;
		uint normal = 0, produced = 0, accepted = 0, maximum = 0, error = 0;
		// Native catchments, complete stop admission and actual road geometry
		// jointly select useful service; town population alone is not demand.
		for (const auto tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
			if (!IsNormalRoadTile(tile)) continue;
			if (Distance(tile, Town::Get(TownID{0})->xy) > 24) continue;
			++normal;
			const auto production = GetProductionAroundTiles(tile, 1, 1, CA_BUS)[passenger];
			maximum = std::max(maximum, production);
			const auto acceptance = GetAcceptanceAroundTiles(tile, 1, 1, CA_BUS).first[passenger];
			// Each end must genuinely generate and accept passengers before any
			// vehicle is bought; these weights still promise no paid ridership.
			if (production < 4) continue;
			++produced;
			if (acceptance < 8) continue;
			++accepted;
			for (const auto direction : {DiagDirection::NE, DiagDirection::SE}) {
				const auto quote = StopQuote(tile, direction); if (quote.Failed()) { error = quote.GetErrorMessage().base(); continue; }
				candidates.push_back(tile); orientation.emplace(tile, direction); weights.emplace(tile, production); prices.emplace(tile, quote.GetCost()); break;
			}
		}
		uint64_t best = 0;
		fmt::print("SURFACE_PASSENGER_SEARCH normal={} produced={} accepted={} maximum={} error={} candidates={}\n", normal, produced, accepted, maximum, error, candidates.size());
		// Rank complete existing connections by useful direct distance and
		// weaker endpoint production. Long detours do not increase native fares.
		for (const auto first : candidates) {
			const auto connected = Reach(first);
			for (const auto last : candidates) {
				const auto direct = Distance(first, last);
				if (direct < 16) continue;
				if (!connected.contains(last)) continue;
				if (connected.at(last) > direct * 2) continue;
				const uint64_t score = uint64_t{direct} * std::min(weights.at(first), weights.at(last));
				if (score <= best) continue;
				best = score; this->stops = {first, last}; this->directions = {orientation.at(first), orientation.at(last)};
				this->production = {weights.at(first), weights.at(last)}; this->route_length = connected.at(last); this->distance = direct;
			}
		}
		INFO("Passenger candidates=" << candidates.size()); REQUIRE(best != 0);
		this->invoice = prices.at(this->stops[0]) + prices.at(this->stops[1]); const auto connected = Reach(this->stops[0]);
		Money service_price{INT64_MAX}; uint depot_distance = UINT_MAX;
		// Select a real clear/tree depot next to the same road component. The
		// original whole road-shape and depot commands must admit both purchases.
		for (const auto &[road, length] : connected) {
			for (const auto toward : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
				const auto tile = SurfaceChemicalLayout::Step(road, toward); if (tile == INVALID_TILE) continue;
				if (tile == this->stops[0] || tile == this->stops[1]) continue; // Keep both paid passenger footprints exclusive.
				if (!IsTileType(tile, TileType::Clear) && !IsTileType(tile, TileType::Trees)) continue; // Existing houses and assets retain ownership.
				const auto bits = GetRoadBits(road, RoadTramType::Road) | DiagDirToRoadBits(toward);
				const auto depot = CmdBuildRoadDepot(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoadDepot>()), tile, ROADTYPE_ROAD, ReverseDiagDir(toward));
				if (depot.Failed()) continue;
				CommandCost access;
				if (bits != GetRoadBits(road, RoadTramType::Road)) access = CmdBuildRoad(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoad>()), road, bits, ROADTYPE_ROAD, {}, TownID::Invalid());
				if (access.Failed()) continue;
				const auto price = depot.GetCost() + access.GetCost();
				// Keep the depot near the first pickup, then minimize its complete
				// purchase; an empty depot far away is not useful working capital.
				if (length > depot_distance || (length == depot_distance && price >= service_price)) continue;
				depot_distance = length; service_price = price; this->depot = tile; this->entrance = road;
				this->door = ReverseDiagDir(toward); this->connections = bits;
			}
		}
		REQUIRE(this->depot != INVALID_TILE); this->invoice += service_price;
		fmt::print("SURFACE_PASSENGER_LAYOUT first={} first_direction={} first_production={} last={} last_direction={} last_production={} depot={} entrance={} door={} bits={} infrastructure={} route_edges={} distance={} candidates={}\n",
			this->stops[0].base(), to_underlying(this->directions[0]), this->production[0], this->stops[1].base(), to_underlying(this->directions[1]), this->production[1],
			this->depot.base(), this->entrance.base(), to_underlying(this->door), this->connections.base(), this->invoice.base(), this->route_length, this->distance, candidates.size());
		fmt::print("SURFACE_PASSENGER_TARIFF quantity=40 distance={} periods=16 income={} forecast_only=true\n", this->distance, GetTransportedGoodsIncome(40, this->distance, 16, passenger).base());
	}
};
#endif /* TEST_SURFACE_PASSENGER_LAYOUT_H */
