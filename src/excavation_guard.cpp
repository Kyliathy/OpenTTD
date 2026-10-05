/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_guard.cpp Shared read-only construction and restricted commissioning predicates. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 8 */

#include "stdafx.h"
#include "excavation.h"
#include "command_func.h"
#include "timer/timer_game_calendar.h"
#include "table/strings.h"

#include "safeguards.h"

/**
 * Validate complete explicit construction geometry against one accepted policy.
 * @param geology Owner's read-only terrain, independent of tile types.
 * @param policy Saved profile and explicit exceptions.
 * @param calendar_date Synchronized calendar date.
 * @param footprint Ordered complete world footprint; the first refused tile is stable for this order.
 * @return Unchanged refusal or full permission.
 */
ExcavationFootprintResult CheckExcavatedFootprint(const GeologyWorldState &geology, const MultiLayerProfileSelection &policy, TimerGameCalendar::Date calendar_date, std::span<const TileIndex> footprint)
{
	if (footprint.empty()) return {ExcavationError::InvalidTile, INVALID_TILE};
	const auto *profile = GetMultiLayerContentProfile(policy.id);
	// A malformed saved policy cannot silently acquire Classic's construction permissions.
	if (profile == nullptr || policy.version != profile->version || (policy.id != MultiLayerGameplayProfileID::Classic && policy.legacy)) {
		return {ExcavationError::InvalidPolicy, footprint.front()};
	}
	MapLayer layer = MapLayer::Invalid;
	// Every footprint/approach cell must pass before any constructor clears an earlier legal cell.
	for (TileIndex tile : footprint) {
		if (!IsValidTile(tile)) return {ExcavationError::InvalidTile, tile};
		const auto current = GetTileLayer(tile);
		if (!MultiLayerMap::IsValidLayer(current)) return {ExcavationError::InvalidTile, tile};
		if (layer == MapLayer::Invalid) layer = current;
		if (layer != current) return {ExcavationError::MixedLayers, tile};
		if (current == MapLayer::DeepUnderground) {
			if (!CheckDeepLayerAvailability(policy, calendar_date)) return {ExcavationError::LockedDeep, tile};
		}
		if (policy.id == MultiLayerGameplayProfileID::Classic) continue;
		// Surface and Space retain their independent terrain rules; only the two rock layers own these packed cells.
		if (current != MapLayer::Underground && current != MapLayer::DeepUnderground) continue;
		// Older Native saves without geology retain their existing open terrain; loading must not invent solid rock.
		if (geology.generator_version == 0) continue;
		if (geology.generator_version != 1) return {ExcavationError::InvalidPolicy, tile};
		if (geology.job_claims.contains(tile)) return {ExcavationError::ActiveCut, tile};
		const auto terrain = geology.terrain.Get(tile);
		if (!terrain) return {ExcavationError::InvalidTile, tile};
		if (*terrain == ExcavationState::Excavating) return {ExcavationError::ActiveCut, tile};
		if (*terrain != ExcavationState::Excavated) return {ExcavationError::SolidRock, tile};
	}
	return {};
}

/**
 * Use the sole live geological and profile owners for ordinary command validation.
 * @param footprint Explicit complete footprint and approaches.
 * @return Read-only stable construction decision.
 */
ExcavationFootprintResult CheckExcavatedFootprint(std::span<const TileIndex> footprint)
{
	return CheckExcavatedFootprint(GetGeologyWorldState(), GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date, footprint);
}

/**
 * Validate bounded restricted occupancy; this never changes the packed cavity state.
 * @param pocket Actual commissioning owner's issued capability.
 * @param actor Exact assigned TBM vehicle.
 * @param tile Requested pocket cell.
 * @return True only for well-formed assigned occupancy.
 */
bool CheckExcavationCommissioningPocket(const ExcavationCommissioningPocket &pocket, VehicleID actor, TileIndex tile)
{
	// A missing actor, anchor, membership or oversized capability cannot open rock to unrelated vehicles.
	if (actor == VehicleID::Invalid() || actor != pocket.assigned_cutter || !IsValidTile(pocket.anchor) || pocket.tiles.empty() || pocket.tiles.size() > 9) return false;
	const auto layer = GetTileLayer(pocket.anchor);
	// Commissioning pockets belong only to rock-plane TBM approaches, never Surface/Space infrastructure.
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return false;
	if (layer == MapLayer::DeepUnderground) {
		if (!CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return false;
	}
	bool contains = false;
	// Validate the entire producer-issued envelope so one good member cannot conceal an out-of-pocket capability.
	for (TileIndex member : pocket.tiles) {
		if (!IsValidTile(member)) return false;
		// Same-layer 3x3 bounds prevent a capability from becoming an arbitrary excavation bypass.
		if (GetTileLayer(member) != layer || std::abs(static_cast<int>(TileX(member)) - static_cast<int>(TileX(pocket.anchor))) > 1 || std::abs(static_cast<int>(TilePlaneY(member)) - static_cast<int>(TilePlaneY(pocket.anchor))) > 1) return false;
		if (member == tile) contains = true;
	}
	return contains;
}

/**
 * Convert a geological refusal without changing map, cargo, cash or RNG.
 * @param footprint Complete command footprint, including required approaches.
 * @return Stable command error or empty cost.
 */
CommandCost CheckExcavationConstruction(std::span<const TileIndex> footprint)
{
	const auto result = CheckExcavatedFootprint(footprint);
	switch (result.error) {
		case ExcavationError::None: return {};
		case ExcavationError::SolidRock: return CommandCost(STR_ERROR_EXCAVATION_SOLID_ROCK);
		case ExcavationError::ActiveCut: return CommandCost(STR_ERROR_EXCAVATION_ACTIVE_CUT);
		case ExcavationError::LockedDeep: return CommandCost(STR_ERROR_EXCAVATION_DEEP_LOCKED);
		default: return CMD_ERROR;
	}
}

/**
 * Check bounded rectangular geometry before iterating any tile addresses.
 * @param tile Explicit origin within one layer.
 * @param width Positive footprint width.
 * @param height Positive footprint height.
 * @param entrance Outward approach direction, or Invalid for none.
 * @param through Require the opposite approaches as well.
 * @return Permission or unchanged refusal.
 */
CommandCost CheckExcavationConstructionArea(TileIndex tile, uint width, uint height, DiagDirection entrance, bool through)
{
	// Reject wraparound/zero dimensions before allocating or walking an atomic construction footprint.
	if (!IsValidTile(tile) || width == 0 || height == 0 || width > Map::SizeX() - TileX(tile) || height > Map::SizeY() - TilePlaneY(tile)) return CMD_ERROR;
	std::vector<TileIndex> tiles;
	tiles.reserve(static_cast<size_t>(width) * height);
	// Row-major layer-local traversal gives every multi-tile constructor the same stable refusal order.
	for (uint y = 0; y < height; ++y) {
		for (uint x = 0; x < width; ++x) tiles.push_back(TileAddWrap(tile, x, y));
	}
	// Geological profiles require actual approaches; Classic keeps its historical edge/building policy.
	if (IsValidDiagDirection(entrance) && GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::Classic) {
		const auto offset = TileIndexDiffCByDiagDir(entrance);
		const size_t cells = tiles.size();
		// Inspect only the original floor; appended approaches must never recursively enlarge the envelope.
		for (size_t i = 0; i < cells; ++i) {
			for (int sign : {1, -1}) {
				if (sign == -1 && !through) continue; // A one-ended stop needs only its actual entrance.
				const int x = static_cast<int>(TileX(tiles[i])) - static_cast<int>(TileX(tile)) + sign * offset.x;
				const int y = static_cast<int>(TilePlaneY(tiles[i])) - static_cast<int>(TilePlaneY(tile)) + sign * offset.y;
				// Internal neighbours already belong to the checked floor; only boundary approaches add ground.
				if (x >= 0 && y >= 0 && x < static_cast<int>(width) && y < static_cast<int>(height)) continue;
				tiles.push_back(TileAddWrap(tile, x, y));
			}
		}
	}
	return CheckExcavationConstruction(tiles);
}

/**
 * Check both an end/building and its required entry cell.
 * @param tile Explicit building tile.
 * @param entrance Direction from that tile to the approach.
 * @return Read-only complete permission.
 */
CommandCost CheckExcavationConstructionEntrance(TileIndex tile, DiagDirection entrance)
{
	// Classic keeps its historical edge/approach behavior; geological profiles require a complete excavated entrance.
	if (GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::Classic) return CheckExcavationConstruction(std::array{tile});
	if (!IsValidDiagDirection(entrance)) return CMD_ERROR;
	const auto offset = TileIndexDiffCByDiagDir(entrance);
	return CheckExcavationConstruction(std::array{tile, TileAddWrap(tile, offset.x, offset.y)});
}
