/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_chemical_layout.h Pure original-command planning for authentic chemical freight.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.
 * These ephemeral route searches describe purchases; they never own saved
 * routing, terrain, cargo, financial balances or vehicle positions.
 */
#ifndef TEST_SURFACE_CHEMICAL_LAYOUT_H
#define TEST_SURFACE_CHEMICAL_LAYOUT_H

#include "../road_cmd.h"
#include "../road_func.h"
#include "../road_map.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include <queue>

/** One original bay and its actual physical road entrance, before purchase. */
struct SurfaceChemicalBay {
	TileIndex tile; ///< Exact ordinary command site.
	TileIndex entrance; ///< Actual adjacent normal-road connection.
	DiagDirection direction; ///< Native bay opening, never a camera orientation.
};

/** Complete bounded proposed layout whose every shape is queried through original commands. */
class SurfaceChemicalLayout {
public:
	std::vector<SurfaceChemicalBay> bays; ///< Oil pickup, chemical input/output, future fuel input/output.
	std::map<TileIndex, RoadBits> roads; ///< Requested union of actual physical road connections, not saved routing.
	Money invoice{0}; ///< Sum of pure current infrastructure quotes, never money.

	/** @param tile Real Surface origin. @param direction Native adjacent step. @return Same-plane neighbour, or invalid at an edge. */
	static TileIndex Step(TileIndex tile, DiagDirection direction)
	{
		const auto next = AddTileIndexDiffCWrap(tile, TileIndexDiffCByDiagDir(direction));
		// Physical routes may not cross map edges or escape the Surface through
		// a stacked-storage offset that merely looks adjacent in projected space.
		if (!MultiLayerMap::IsTileInLayer(next, MapLayer::Surface) ||
			std::abs(int(TileX(next)) - int(TileX(tile))) + std::abs(int(TilePlaneY(next)) - int(TilePlaneY(tile))) != 1) return INVALID_TILE;
		return next;
	}

	/** @param tile Actual terrain candidate. @return Whether it belongs to either complete reserved factory footprint. */
	static bool FactoryCell(TileIndex tile)
	{
		if (tile == INVALID_TILE) return false;
		for (const auto anchor : {TileIndex{1566}, TileIndex{1639}}) {
			// Both16cell factories must survive all road/station planning; an
			// initially clear footprint is not free land for its own access road.
			if (TileX(tile) >= TileX(anchor) && TileX(tile) < TileX(anchor) + 4 &&
				TilePlaneY(tile) >= TilePlaneY(anchor) && TilePlaneY(tile) < TilePlaneY(anchor) + 4) return true;
		}
		return false;
	}

	/** @param tile Native candidate. @return Whether an original road query may consider it without destroying another owner. */
	bool RoadCandidate(TileIndex tile) const
	{
		if (tile == INVALID_TILE) return false;
		if (FactoryCell(tile)) return false;
		for (const auto &bay : this->bays) if (bay.tile == tile) return false;
		// Only empty/tree terrain and ordinary roads participate. Houses,
		// machinery, stations, water and the paid railway keep their owners.
		return IsTileType(tile, TileType::Clear) || IsTileType(tile, TileType::Trees) || IsNormalRoadTile(tile);
	}

	/** @param tile Candidate road tile. @param bits Required connections. @return Whether the native whole shape is already present or can be bought. */
	bool CanRoad(TileIndex tile, RoadBits bits) const
	{
		if (!this->RoadCandidate(tile)) return false;
		if (IsNormalRoadTile(tile)) {
			const auto present = GetRoadBits(tile, RoadTramType::Road);
			if ((present | bits) == present) return true;
		}
		return CmdBuildRoad(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoad>()), tile, bits, ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded();
	}

	/** @param tile Selected real bay site. @param direction Bay opening. @return Original complete bay quote or refusal. */
	static CommandCost QuoteBay(TileIndex tile, DiagDirection direction)
	{
		return CmdBuildRoadStop(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoadStop>()), tile, 1, 1, RoadStopType::Truck,
			false, direction, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, true);
	}

	/** @param source Actual bay/depot endpoint. @param target Actual bay/depot endpoint. Add only original-command-admitted connecting shapes. */
	void Connect(const SurfaceChemicalBay &source, const SurfaceChemicalBay &target)
	{
		const uint32_t states = Map::PlaneSize() * 4;
		std::vector<int32_t> parent(states, -1); std::queue<uint32_t> pending;
		const uint32_t initial = source.entrance.base() * 4 + to_underlying(ReverseDiagDir(source.direction));
		parent[initial] = initial; pending.push(initial); uint32_t found = states;
		// Directional search states validate the real entry/exit shape at each
		// turn. A tile-only path could invent an impossible slope connection.
		while (!pending.empty()) {
			const uint32_t state = pending.front(); pending.pop();
			const TileIndex tile{state / 4}; const auto back = static_cast<DiagDirection>(state % 4);
			if (tile == target.entrance) {
				if (this->CanRoad(tile, DiagDirToRoadBits(back) | DiagDirToRoadBits(ReverseDiagDir(target.direction)))) { found = state; break; }
			}
			// Every next edge needs both a safe neighbour and the original native
			// shape quote. No fixture clearing or terrain foundation is supplied.
			for (const auto exit : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
				if (exit == back) continue;
				const auto next = Step(tile, exit);
				if (!this->RoadCandidate(next)) continue;
				const uint32_t candidate = next.base() * 4 + to_underlying(ReverseDiagDir(exit));
				if (parent[candidate] != -1) continue;
				if (!this->CanRoad(tile, DiagDirToRoadBits(back) | DiagDirToRoadBits(exit))) continue;
				parent[candidate] = state; pending.push(candidate);
			}
		}
		REQUIRE(found != states);
		std::vector<uint32_t> route;
		// Trace only the admitted predecessor chain; no saved route, vehicle
		// movement or station connectivity exists until ordinary purchases.
		for (uint32_t state = found;; state = parent[state]) { route.push_back(state); if (state == initial) break; }
		std::ranges::reverse(route);
		// Union intersecting routes before pricing: each actual tile is bought
		// once with every required connection, rather than counted as two roads.
		for (size_t index = 0; index < route.size(); ++index) {
			const TileIndex tile{route[index] / 4}; const auto back = static_cast<DiagDirection>(route[index] % 4);
			const auto exit = index + 1 == route.size() ? ReverseDiagDir(target.direction) : ReverseDiagDir(static_cast<DiagDirection>(route[index + 1] % 4));
			this->roads[tile] |= DiagDirToRoadBits(back) | DiagDirToRoadBits(exit);
		}
	}

	/** Resolve and quote the complete initial/future road layout without spending or replacing owners. */
	void Prepare()
	{
		this->bays = {{TileIndex{1824}, TileIndex{1888}, DiagDirection::SE}, {TileIndex{1825}, TileIndex{1889}, DiagDirection::SE},
			{TileIndex{1895}, TileIndex{1959}, DiagDirection::SE}, {TileIndex{1896}, TileIndex{1960}, DiagDirection::SE}};
		const auto *oil = Industry::Get(IndustryID{8});
		std::vector<TileIndex> candidates;
		for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) candidates.push_back(tile);
		/** @param tile Original Surface bay candidate. @return Stable distance from the actual natural Oil source, then native tile ID. */
		const auto rank = [oil](TileIndex tile) {
			return std::make_pair(std::abs(int(TileX(tile)) - int(TileX(oil->location.tile))) +
				std::abs(int(TilePlaneY(tile)) - int(TilePlaneY(oil->location.tile))), tile.base());
		};
		std::ranges::sort(candidates, {}, rank);
		bool found = false;
		// Select a native buildable bay next to the original natural Oil source.
		// Actual station catchment is rechecked after its normal construction.
		for (TileIndex tile : candidates) {
			if (rank(tile).first > 3) break;
			if (!IsTileType(tile, TileType::Clear)) continue;
			for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
				const auto entrance = Step(tile, direction);
				if (!this->CanRoad(entrance, ROAD_ALL)) continue;
				if (QuoteBay(tile, direction).Failed()) continue;
				this->bays.insert(this->bays.begin(), {tile, entrance, direction}); found = true; break;
			}
			if (found) break;
		}
		REQUIRE(found);
		const TileIndex depot{2443}; const auto door = GetRoadDepotDirection(depot);
		const SurfaceChemicalBay service{depot, Step(depot, door), door};
		// All five distinct bays reach the real preexisting paid depot. Separate
		// input/output bays prevent a full-load chemical pickup blocking Oil intake.
		for (const auto &bay : this->bays) this->Connect(service, bay);
		for (size_t index = 0; index < this->bays.size(); ++index) {
			const auto &bay = this->bays[index]; const auto quote = QuoteBay(bay.tile, bay.direction); REQUIRE(quote.Succeeded());
			this->invoice += quote.GetCost();
			fmt::print("SURFACE_CHEMICAL_BAY role={} tile={} entrance={} direction={} cost={}\n", index, bay.tile.base(), bay.entrance.base(),
				to_underlying(bay.direction), quote.GetCost().base());
		}
		// Price complete road unions against native existing bits and terrain.
		// Quote-only temporary path records cannot create network connectivity.
		for (const auto &[tile, bits] : this->roads) {
			REQUIRE(this->CanRoad(tile, bits));
			if (IsNormalRoadTile(tile)) {
				if ((GetRoadBits(tile, RoadTramType::Road) | bits) == GetRoadBits(tile, RoadTramType::Road)) continue;
			}
			const auto quote = CmdBuildRoad(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoad>()), tile, bits, ROADTYPE_ROAD, {}, TownID::Invalid());
			REQUIRE(quote.Succeeded()); this->invoice += quote.GetCost();
			fmt::print("SURFACE_CHEMICAL_ROAD tile={} bits={} cost={}\n", tile.base(), bits.base(), quote.GetCost().base());
		}
		fmt::print("SURFACE_CHEMICAL_INFRASTRUCTURE bays={} road_cells={} cost={} first=1566 second=1639\n", this->bays.size(), this->roads.size(), this->invoice.base());
	}
};

#endif /* TEST_SURFACE_CHEMICAL_LAYOUT_H */
