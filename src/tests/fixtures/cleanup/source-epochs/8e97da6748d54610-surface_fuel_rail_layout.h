/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_fuel_rail_layout.h Native quote-only railway planning for actual Surface earnings.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BI.
 * This temporary purchase description owns no map, routing, money or cargo.
 */
#ifndef TEST_SURFACE_FUEL_RAIL_LAYOUT_H
#define TEST_SURFACE_FUEL_RAIL_LAYOUT_H

#include "../rail_cmd.h"
#include "../track_func.h"
#include "../industry_map.h"
#include "surface_chemical_layout.h"
#include <queue>

/** One quoted terminus, including its complete platform and optional original depot. */
struct SurfaceFuelTerminus {
	TileIndex anchor; ///< Native station anchor, minimum platform tile.
	Axis axis; ///< Original platform track axis.
	uint8_t length; ///< Entire platform length, never a vehicle capacity override.
	DiagDirection outward; ///< End facing the connecting railway.
	TileIndex entrance; ///< First connecting ordinary rail tile.
	TileIndex depot = INVALID_TILE; ///< Optional service depot at the opposite end.
	std::vector<TileIndex> cells; ///< Platform and optional depot, protected during route search.
	Money invoice{0}; ///< Actual whole station/depot quotes only.
};

/** A complete physically separate one-train line, before any native purchase. */
struct SurfaceFuelRailLine {
	SurfaceFuelTerminus source, target; ///< Original source and actual accepting destination.
	std::vector<std::pair<TileIndex, Track>> tracks; ///< Canonical admitted shapes in travel order.
	Money invoice{0}; ///< Whole infrastructure, excluding rolling stock.
};

/** Bounded deterministic planner preserving all previously paid owners and both chemical footprints. */
class SurfaceFuelRailLayout {
	std::set<TileIndex> reserved; ///< Earlier proposed line, never installed ownership.
	std::map<std::pair<TileIndex, Track>, std::optional<Money>> quotes; ///< Ephemeral native shape quote cache.

	/** @param tile Actual candidate. @return Whether railway may use this terrain without destroying a prior owner. */
	bool Terrain(TileIndex tile) const
	{
		if (tile == INVALID_TILE) return false;
		if (SurfaceChemicalLayout::FactoryCell(tile)) return false;
		if (this->reserved.contains(tile)) return false;
		// Empty/tree ground and ordinary road crossings preserve every existing
		// station, railway, industry, house, waterway and portal owner.
		return IsTileType(tile, TileType::Clear) || IsTileType(tile, TileType::Trees) || IsNormalRoadTile(tile);
	}

	/** @param tile Real candidate. @param track Canonical native shape. @return Native price, or original refusal. */
	std::optional<Money> RailQuote(TileIndex tile, Track track)
	{
		const auto key = std::make_pair(tile, track);
		if (auto found = this->quotes.find(key); found != this->quotes.end()) return found->second;
		const auto quote = CmdBuildSingleRail(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRail>()), tile, RAILTYPE_RAIL, track, false);
		const std::optional<Money> result = quote.Succeeded() ? std::optional<Money>{quote.GetCost()} : std::nullopt;
		this->quotes.emplace(key, result); return result;
	}

	/** @param terminal Whole platform. @param industry Required actual provider/receiver. @return Whether its real machinery lies within native railway catchment. */
	static bool Covers(const SurfaceFuelTerminus &terminal, IndustryID industry)
	{
		const uint width = terminal.axis == Axis::X ? terminal.length : 1;
		const uint height = terminal.axis == Axis::Y ? terminal.length : 1;
		// An actual industry's owned machinery, rather than another nearby
		// provider with the same cargo, must lie in this proposed station's area.
		for (const auto tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
			if (!IsTileType(tile, TileType::Industry)) continue;
			if (GetIndustryIndex(tile) != industry) continue;
			const int x = int(TileX(tile)) - int(TileX(terminal.anchor));
			const int y = int(TilePlaneY(tile)) - int(TilePlaneY(terminal.anchor));
			if (x >= -int(CA_TRAIN) && x < int(width + CA_TRAIN) && y >= -int(CA_TRAIN) && y < int(height + CA_TRAIN)) return true;
		}
		return false;
	}

	/** @param industry Required natural provider/receiver, or invalid for town Goods. @param length Whole platform. @param depot Whether native service is needed. @return Diverse buildable catchment-admitted termini. */
	std::vector<SurfaceFuelTerminus> Termini(IndustryID industry, uint8_t length, bool depot)
	{
		const CargoType goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
		const TileIndex centre = Industry::IsValidID(industry) ? Industry::Get(industry)->location.tile : Town::Get(TownID{0})->xy;
		std::vector<TileIndex> candidates;
		for (const auto tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) candidates.push_back(tile);
		/** @param tile Real anchor. @return Stable proximity to this actual provider/receiver. */
		const auto rank = [centre](TileIndex tile) {
			return std::make_pair(std::abs(int(TileX(tile)) - int(TileX(centre))) + std::abs(int(TilePlaneY(tile)) - int(TilePlaneY(centre))), tile.base());
		};
		std::ranges::sort(candidates, {}, rank);
		std::array<uint, 4> counts{}; std::vector<SurfaceFuelTerminus> result;
		uint terrain_ok = 0, catchment_ok = 0, station_ok = 0, depot_ok = 0; uint station_error = 0, depot_error = 0;
		// Keep several sites per orientation: a nearest station alone can seal
		// its own approach or occupy the second commercial line's only corridor.
		for (const auto tile : candidates) {
			if (rank(tile).first > 12) break;
			for (const auto outward : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
				if (counts[to_underlying(outward)] >= 6) continue;
				SurfaceFuelTerminus terminal{tile, DiagDirToAxis(outward), length, outward, INVALID_TILE};
				const auto forward = AxisToDiagDir(terminal.axis); TileIndex cell = tile; bool valid = true;
				// Every platform cell must be real empty/tree terrain. Railway
				// stations cannot silently replace paid roads or prior owners.
				for (uint index = 0; index < length; ++index) {
					if (!this->Terrain(cell)) { valid = false; break; }
					if (IsNormalRoadTile(cell)) { valid = false; break; }
					terminal.cells.push_back(cell); cell = SurfaceChemicalLayout::Step(cell, forward);
				}
				if (!valid) continue;
				const bool at_first = outward == ReverseDiagDir(forward);
				terminal.entrance = SurfaceChemicalLayout::Step(at_first ? terminal.cells.front() : terminal.cells.back(), outward);
				if (!this->Terrain(terminal.entrance)) continue;
				++terrain_ok;
				const uint width = terminal.axis == Axis::X ? length : 1, height = terminal.axis == Axis::Y ? length : 1;
				if (Industry::IsValidID(industry)) {
					if (!Covers(terminal, industry)) continue;
				} else {
					const auto [acceptance, always] = GetAcceptanceAroundTiles(tile, width, height, CA_TRAIN);
					// Town Goods revenue requires actual accepting houses and the
					// original always-accepted role, not merely town population.
					if (acceptance[goods] < 8 || !always.Test(goods)) continue;
				}
				++catchment_ok;
				const auto station = CmdBuildRailStation(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRailStation>()), tile,
					RAILTYPE_RAIL, terminal.axis, 1, length, STAT_CLASS_DFLT, 0, NEW_STATION, true);
				if (station.Failed()) { station_error = station.GetErrorMessage().base(); continue; }
				++station_ok;
				terminal.invoice = station.GetCost();
				if (depot) {
					terminal.depot = SurfaceChemicalLayout::Step(at_first ? terminal.cells.back() : terminal.cells.front(), ReverseDiagDir(outward));
					if (!this->Terrain(terminal.depot)) continue;
					if (IsNormalRoadTile(terminal.depot)) continue;
					const auto service = CmdBuildTrainDepot(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRailDepot>()), terminal.depot, RAILTYPE_RAIL, outward);
					if (service.Failed()) { depot_error = service.GetErrorMessage().base(); continue; }
					++depot_ok;
					terminal.cells.push_back(terminal.depot); terminal.invoice += service.GetCost();
				}
				++counts[to_underlying(outward)]; result.push_back(std::move(terminal));
			}
		}
		fmt::print("SURFACE_FUEL_TERMINUS_SEARCH industry={} length={} terrain={} catchment={} station={} depot={} selected={} station_error={} depot_error={}\n",
			industry.base(), length, terrain_ok, catchment_ok, station_ok, depot_ok, result.size(), station_error, depot_error);
		return result;
	}

	/** @param source Actual source platform. @param target Actual destination platform. @return Whole original-command route, if disjoint and legally connected. */
	std::optional<SurfaceFuelRailLine> Connect(const SurfaceFuelTerminus &source, const SurfaceFuelTerminus &target)
	{
		std::set<TileIndex> protected_cells(source.cells.begin(), source.cells.end());
		for (const auto cell : target.cells) if (!protected_cells.insert(cell).second) return std::nullopt;
		// Neither platform may consume the other terminus's approach tile.
		if (protected_cells.contains(source.entrance) || protected_cells.contains(target.entrance)) return std::nullopt;
		const uint32_t states = Map::Size() * 16;
		std::vector<int32_t> parent(states, -1); std::vector<int64_t> cost(states, INT64_MAX);
		using Pending = std::pair<int64_t, uint32_t>; std::priority_queue<Pending, std::vector<Pending>, std::greater<Pending>> pending;
		const auto initial_dir = DiagDirToDiagTrackdir(source.outward);
		const auto target_dir = DiagDirToDiagTrackdir(ReverseDiagDir(target.outward));
		auto first = TrackdirReachesTrackdirs(initial_dir);
		if (_settings_game.pf.forbid_90_deg) first.Reset(TrackdirCrossesTrackdirs(initial_dir));
		for (const auto dir : first) {
			const auto quote = this->RailQuote(source.entrance, TrackdirToTrack(dir)); if (!quote) continue;
			const uint32_t state = source.entrance.base() * 16 + to_underlying(dir);
			parent[state] = state; cost[state] = quote->base(); pending.emplace(cost[state], state);
		}
		uint32_t found = states;
		// Directional states preserve native curve admission, including the
		// world's90-degree prohibition. Native whole shape prices choose the
		// cheapest admitted route without painting track or moving vehicles.
		while (!pending.empty()) {
			const auto [paid, state] = pending.top(); pending.pop(); if (paid != cost[state]) continue;
			const TileIndex tile{state / 16}; const auto dir = static_cast<Trackdir>(state % 16);
			auto reachable = TrackdirReachesTrackdirs(dir);
			if (_settings_game.pf.forbid_90_deg) reachable.Reset(TrackdirCrossesTrackdirs(dir));
			// The last track must actually enter the far platform in its own
			// axis, rather than merely touching its adjacent map coordinate.
			if (tile == target.entrance && TrackdirToExitdir(dir) == ReverseDiagDir(target.outward) && reachable.Test(target_dir)) { found = state; break; }
			const auto next = SurfaceChemicalLayout::Step(tile, TrackdirToExitdir(dir));
			if (!this->Terrain(next)) continue;
			if (protected_cells.contains(next)) continue;
			for (const auto next_dir : reachable) {
				const auto quote = this->RailQuote(next, TrackdirToTrack(next_dir)); if (!quote) continue;
				const uint32_t candidate = next.base() * 16 + to_underlying(next_dir);
				const int64_t next_cost = paid + quote->base(); if (next_cost >= cost[candidate]) continue;
				parent[candidate] = state; cost[candidate] = next_cost; pending.emplace(next_cost, candidate);
			}
		}
		if (found == states) return std::nullopt;
		SurfaceFuelRailLine line{source, target}; std::set<TileIndex> used;
		// Reject a trace revisiting a tile: two directions through one coordinate
		// would require a newly quoted junction, beyond this single-track plan.
		for (uint32_t state = found;; state = parent[state]) {
			const TileIndex tile{state / 16}; if (!used.insert(tile).second) return std::nullopt;
			line.tracks.emplace_back(tile, TrackdirToTrack(static_cast<Trackdir>(state % 16)));
			if (parent[state] == state) break;
		}
		std::ranges::reverse(line.tracks); line.invoice = source.invoice + target.invoice + Money{cost[found]}; return line;
	}

	/** @param line Complete earlier proposed line. Protect its entire footprint from the second line. */
	void Reserve(const SurfaceFuelRailLine &line)
	{
		for (const auto &terminal : {line.source, line.target}) for (const auto tile : terminal.cells) this->reserved.insert(tile);
		for (const auto &[tile, track] : line.tracks) this->reserved.insert(tile);
	}

	/** @param provider Real source industry. @param receiver Real receiver, or invalid for town Goods. @param length Full platform length. @return First bounded complete physical line. */
	SurfaceFuelRailLine Select(IndustryID provider, IndustryID receiver, uint8_t length)
	{
		const auto sources = this->Termini(provider, length, true), targets = this->Termini(receiver, length, false);
		REQUIRE_FALSE(sources.empty()); REQUIRE_FALSE(targets.empty());
		// Bounded diverse station pairs are attempted in stable proximity order.
		// A native refusal rejects the whole candidate; no terrain fallback exists.
		for (const auto &source : sources) for (const auto &target : targets) {
			if (auto line = this->Connect(source, target)) { this->Reserve(*line); return *line; }
		}
		FAIL("No complete native commercial railway layout fits without replacing an owner"); return {};
	}

public:
	std::vector<SurfaceFuelRailLine> lines; ///< Farm-to-factory, then separate factory-to-town.
	Money invoice{0}; ///< Entire quoted physical infrastructure only.

	/** Query and export a whole disjoint commercial railway before spending any money. */
	void Prepare()
	{
		this->lines.push_back(this->Select(IndustryID{7}, IndustryID{5}, 5));
		this->lines.push_back(this->Select(IndustryID{5}, IndustryID::Invalid(), 4));
		for (size_t role = 0; role < this->lines.size(); ++role) {
			const auto &line = this->lines[role]; this->invoice += line.invoice;
			for (uint end = 0; end < 2; ++end) {
				const auto &terminal = end == 0 ? line.source : line.target;
				fmt::print("SURFACE_FUEL_RAIL_TERMINUS role={} end={} anchor={} axis={} length={} outward={} entrance={} depot={} cost={}\n", role, end,
					terminal.anchor.base(), to_underlying(terminal.axis), terminal.length, to_underlying(terminal.outward), terminal.entrance.base(), terminal.depot.base(), terminal.invoice.base());
			}
			for (const auto &[tile, track] : line.tracks) fmt::print("SURFACE_FUEL_RAIL_TRACK role={} tile={} track={} cost={}\n", role, tile.base(), to_underlying(track), this->RailQuote(tile, track)->base());
			fmt::print("SURFACE_FUEL_RAIL_LINE role={} tracks={} cost={} distance={}\n", role, line.tracks.size(), line.invoice.base(),
				std::abs(int(TileX(line.source.anchor)) - int(TileX(line.target.anchor))) + std::abs(int(TilePlaneY(line.source.anchor)) - int(TilePlaneY(line.target.anchor))));
		}
		fmt::print("SURFACE_FUEL_RAIL_INFRASTRUCTURE lines={} cost={} forbid_90={}\n", this->lines.size(), this->invoice.base(), _settings_game.pf.forbid_90_deg);
	}
};

#endif /* TEST_SURFACE_FUEL_RAIL_LAYOUT_H */
