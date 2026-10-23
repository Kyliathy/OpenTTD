/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_terrain_portal.cpp Actual geological cell and reciprocal portal bindings to final M1 native poses.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group82
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_sprite.h"
#include "../excavation_state.h"
#include "../layer_portal_map.h"
#include "excavation_fixture.h"
#include "tbm_fixture.h"

#include "../safeguards.h"

/**
 * Stage one exact cell condition through the existing generation/load transport.
 * @param tile Real same-layer terrain cell for this isolated query fixture.
 * @param state Exact staged geological state, independent of infrastructure metadata.
 * @return None; publish only the prepared fixture candidate, never from the renderer.
 */
static void StageTerrainCell(TileIndex tile, ExcavationState state)
{
	auto snapshot = CaptureGeologyState();
	GeologyWorldState candidate;
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	REQUIRE(candidate.terrain.Set(tile, state));
	snapshot.packed.assign(candidate.terrain.Data().begin(), candidate.terrain.Data().end());
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	StageLoadedGeologyState(std::move(candidate));
	AcceptLoadedGeologyState();
}

/** Real independent rock layers select material phases and screen-ordered neighbor walls without advancing work. */
TEST_CASE("M1 terrain uses authoritative states and bounded screen edge masks", "[m1-terrain-portal]")
{
	ExcavationFixture world;
	const TileIndex tile = TileXYL(20, 20, MapLayer::Underground);
	const TileIndex deep = MultiLayerMap::TileInLayer(tile, MapLayer::DeepUnderground);
	CHECK(GetMultiLayerTerrainSprite(tile) == GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardness, {0, 0, 0, 0, 0}));
	CHECK(GetMultiLayerTerrainSprite(deep) == GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardness, {8, 0, 0, 0, 0}));
	CHECK_FALSE(GetMultiLayerTerrainSprite(MultiLayerMap::TileInLayer(tile, MapLayer::Surface)).has_value());
	CHECK_FALSE(GetMultiLayerTerrainSprite(INVALID_TILE).has_value());
	// Only this fixture's generation/load owner stages exact geological cell
	// conditions. The production sprite query receives no terrain-write access.
	StageTerrainCell(tile, ExcavationState::Excavated);
	CHECK(GetMultiLayerTerrainSprite(tile) == GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardness, {3, 0, 0, 0, 0}));
	static constexpr std::array<TileIndexDiffC, 4> neighbors{{{0, -1}, {-1, 0}, {0, 1}, {1, 0}}};
	// Opening each true same-layer neighbor removes exactly its authored screen
	// rim; aligned Deep tiles must remain solid throughout this surface-plane scene.
	for (size_t bit = 0; bit < neighbors.size(); ++bit) {
		const auto delta = neighbors[bit];
		const TileIndex adjacent = TileAddWrap(tile, delta.x, delta.y);
		StageTerrainCell(adjacent, ExcavationState::Excavated);
		const auto walls = GetMultiLayerTerrainEdgeSprites(tile);
		// Opening a neighbor removes only that exposed face; all remaining
		// walls retain the uncut neighbor's canonical hardness and orientation.
		for (size_t side = 0; side < walls.size(); ++side) {
			if (side <= bit) CHECK_FALSE(walls[side].has_value());
			else CHECK(walls[side] == GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardnessEdges, {static_cast<uint8_t>(side), 0, 0, 0, 0}));
		}
		CHECK(GetMultiLayerTerrainSprite(deep) == GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardness, {8, 0, 0, 0, 0}));
	}
	CHECK(GetMultiLayerTerrainSprite(tile) == GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardness, {3, 0, 0, 0, 0}));
	StageTerrainCell(tile, ExcavationState::Solid);
	CHECK(GetMultiLayerTerrainSprite(tile) == GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardness, {1, 0, 0, 0, 0}));
	CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Solid);
	CHECK(GetGeologyWorldState().terrain.Get(deep) == ExcavationState::Solid);
}

/** A real published project reserves its swept cells; rendering reads that claim without advancing partial work. */
TEST_CASE("M1 work ground and commissioning portal read the actual project owners", "[m1-terrain-portal]")
{
	TBMProjectRig rig;
	const auto project = *GetTBMProject(rig.project.id);
	const auto job = *GetExcavationJob(project.job);
	const TileIndex cell = job.route.front().cutter_position;
	REQUIRE(GetGeologyWorldState().terrain.Get(cell) == ExcavationState::Excavating);
	CHECK(GetMultiLayerTerrainSprite(cell) == GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardness, {2, 0, 0, 0, 0}));
	const TileIndex upper = rig.fixture.request.portal;
	const uint8_t door = static_cast<uint8_t>(GetLayerPortalDoorDirection(upper));
	REQUIRE(GetLayerPortalWorkState(upper) == LayerPortalWorkState::Commissioning);
	CHECK(GetMultiLayerPortalSprite(upper, false) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityPortalSealed, {1, 0, door, 0, 0}));
	CHECK(GetMultiLayerPortalSprite(upper, true) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityPortalSealed, {1, 0, door, 0, 1}));
	CHECK(*GetTBMProject(project.id) == project);
	CHECK(*GetExcavationJob(project.job) == job);
}

/** Existing physical portal owners supply both independently layered endpoints; the renderer cannot commission either. */
TEST_CASE("M1 rail portal uses reciprocal endpoint and saved Sealed state", "[m1-terrain-portal]")
{
	TBMEngineFixture world;
	const TileIndex upper = world.request.portal;
	const TileIndex lower = GetLayerPortalPairTile(upper);
	const auto key = GetLayerPortalPairKey(upper);
	REQUIRE(key.has_value());
	const uint8_t upper_door = static_cast<uint8_t>(GetLayerPortalDoorDirection(upper));
	const uint8_t lower_door = static_cast<uint8_t>(GetLayerPortalDoorDirection(lower));
	CHECK(GetLayerPortalWorkState(upper) == LayerPortalWorkState::Sealed);
	CHECK(GetMultiLayerPortalSprite(upper, false) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityPortalSealed, {0, 0, upper_door, 0, 0}));
	CHECK(GetMultiLayerPortalSprite(upper, true) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityPortalSealed, {0, 0, upper_door, 0, 1}));
	CHECK(GetMultiLayerPortalSprite(lower, false) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityPortalSealed, {0, 0, lower_door, 1, 0}));
	CHECK(GetMultiLayerPortalSprite(lower, true) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityPortalSealed, {0, 0, lower_door, 1, 1}));
	CHECK(GetLayerPortalPairKey(lower) == key);
	CHECK(GetLayerPortalWorkState(lower) == LayerPortalWorkState::Sealed);
	CHECK_FALSE(GetMultiLayerPortalSprite(TileXYL(10, 10, MapLayer::Surface), false).has_value());
}
