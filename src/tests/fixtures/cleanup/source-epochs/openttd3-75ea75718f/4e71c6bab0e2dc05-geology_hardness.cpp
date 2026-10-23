/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geology_hardness.cpp Immutable regional rock difficulty shared by cutting, survey and rendering.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group10
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "stdafx.h"
#include "excavation.h"
#include "tile_map.h"

#include "safeguards.h"

/** @param value Explicit unsigned input. @return Frozen SplitMix64 finalizer; no global or mutable random stream is consulted. */
static uint64_t MixRockRegion(uint64_t value)
{
	value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
	value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
	return value ^ (value >> 31);
}

/** @copydoc DeriveGeologyHardnessSeed */
uint64_t DeriveGeologyHardnessSeed(uint64_t geology_seed)
{
	return MixRockRegion(geology_seed ^ 0x524f434b5f563031ULL); // ROCK_V01, independent of the frozen ore/vent streams.
}

/** @param seed Saved rock root. @param layer Actual rock plane. @param x Signed regional column. @param y Signed regional row. @return Stable candidate identity across platforms and reloads. */
static uint64_t RockRegionIdentity(uint64_t seed, MapLayer layer, int32_t x, int32_t y)
{
	const uint64_t coordinates = (uint64_t{static_cast<uint32_t>(x)} << 32) | static_cast<uint32_t>(y);
	return MixRockRegion(seed ^ MixRockRegion(coordinates) ^ (uint64_t{to_underlying(layer)} * 0x9e3779b97f4a7c15ULL));
}

/** @copydoc GetExcavationCellHardness */
GeologyHardness GetExcavationCellHardness(const GeologyWorldState &geology, TileIndex tile)
{
	const auto *deposit = geology.deposits.Get(geology.deposits.Find(tile));
	if (deposit != nullptr) return deposit->hardness; // Saved mineral seams retain their independent, frozen geological difficulty.
	const MapLayer layer = GetTileLayer(tile);
	const bool deep = layer == MapLayer::DeepUnderground;
	if (geology.hardness_version == 0 || (layer != MapLayer::Underground && !deep)) return deep ? GeologyHardness::Hard : GeologyHardness::Soft;
	assert(geology.hardness_version == GEOLOGY_HARDNESS_VERSION);
	constexpr int32_t REGION_SIZE = 24;
	const int32_t x = static_cast<int32_t>(TileX(tile)), y = static_cast<int32_t>(TilePlaneY(tile));
	uint64_t selected = 0;
	int32_t nearest = INT32_MAX;
	// Jittered centres at offsets4..19 keep regions coherent without square
	// boundaries. The home centre is at most27 tiles away; any centre outside
	// these nine regions is at least28 away. Query work is therefore fixed.
	for (int32_t row = y / REGION_SIZE - 1; row <= y / REGION_SIZE + 1; ++row) {
		for (int32_t column = x / REGION_SIZE - 1; column <= x / REGION_SIZE + 1; ++column) {
			const uint64_t identity = RockRegionIdentity(geology.hardness_seed, layer, column, row);
			const int32_t dx = x - (column * REGION_SIZE + 4 + static_cast<int32_t>(identity & 15));
			const int32_t dy = y - (row * REGION_SIZE + 4 + static_cast<int32_t>((identity >> 8) & 15));
			const int32_t distance = dx * dx + dy * dy;
			if (distance < nearest) { nearest = distance; selected = identity; } // Row/column traversal is the deterministic equal-distance tie break.
		}
	}
	const uint32_t material = static_cast<uint32_t>(MixRockRegion(selected ^ 0x4d4154455249414cULL) % 100);
	// Expected regional shares: Underground60/25/14/1 and Deep5/15/60/20.
	// Rare upper Basalt is a visible Standard-cutter boundary; the existing
	// canonical preview/capability checks refuse it before accepting new cuts.
	if (material < (deep ? 5U : 60U)) return GeologyHardness::Soft;
	if (material < (deep ? 20U : 85U)) return GeologyHardness::Mixed;
	if (material < (deep ? 80U : 99U)) return GeologyHardness::Hard;
	return GeologyHardness::Basalt;
}
