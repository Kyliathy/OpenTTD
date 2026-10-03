/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file map_layer.cpp Test stacked-plane addressing and MultiLayerMap. @see docs/zz-reach2/upgrades/2026-10/r2mlm2-layer-addressing-and-controller.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#include "../stdafx.h"

#include "../3rdparty/catch2/catch.hpp"

#include "../map_func.h"
#include "../map_layer.h"
#include "../tile_map.h"
#include "../clear_map.h"
#include "../tilearea_orthogonal.h"
#include "../tilearea_spiral.h"
#include "../layer_portal_map.h"
#include "../layer_portal_cmd.h"
#include "../rail_type.h"
#include "../road_type.h"
#include "../road.h"
#include "../tile_cmd.h"
#include "../track_func.h"

#include "../safeguards.h"

/**
 * Whether a one-tile step stays inside the tile's plane.
 * Debug TileAdd rejects a step that leaves the plane, which is the original
 * off-map step from the north or west border. Callers must not take that step.
 * @param tile Inner tile to step from.
 * @param diff Plane offset of one direction.
 * @return True when the destination column and row are still on the plane.
 */
static bool NeighbourStaysOnPlane(TileIndex tile, TileIndexDiffC diff)
{
	int nx = static_cast<int>(TileX(tile)) + diff.x;
	int ny = static_cast<int>(TilePlaneY(tile)) + diff.y;
	bool on_plane = nx >= 0 && ny >= 0 && nx < static_cast<int>(Map::SizeX()) && ny < static_cast<int>(Map::SizeY());
	if (!on_plane) {
		//Only a playable north or west border can step off the map, and that step already left the map before layers existed.
		CHECK_FALSE(_settings_game.construction.freeform_edges);
		CHECK((TileX(tile) == 0 || TilePlaneY(tile) == 0));
	}
	return on_plane;
}

//I-5: three layers store three planes, and the one-layer default stays one plane.
TEST_CASE("Map::Allocate layer count")
{
	Map::Allocate(64, 64, 3);
	CHECK(Map::LayerCount() == 3);
	CHECK(Map::Size() == 3 * Map::PlaneSize());

	Map::Allocate(64, 64);
	CHECK(Map::LayerCount() == 1);
	CHECK(Map::Size() == Map::PlaneSize());
}

//FD-11: a savegame layer count is storable only from one layer up to every layer this build knows.
TEST_CASE("Storable layer counts")
{
	CHECK_FALSE(MultiLayerMap::IsStorableLayerCount(0));
	CHECK(MultiLayerMap::IsStorableLayerCount(1));
	CHECK(MultiLayerMap::IsStorableLayerCount(MAX_MAP_LAYERS));
	CHECK_FALSE(MultiLayerMap::IsStorableLayerCount(MAX_MAP_LAYERS + 1));
	CHECK_FALSE(MultiLayerMap::IsStorableLayerCount(255));
}

//I-1: a tile round-trips through its column, plane row and layer, and TileY stays stacked.
TEST_CASE("TileXYL round trip")
{
	Map::Allocate(64, 64, 3);

	static const uint coords[] = {0, 32, 63};
	for (MapLayer layer : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space}) {
		//Corners and the centre of every layer.
		for (uint x : coords) {
			for (uint y : coords) {
				TileIndex tile = TileXYL(x, y, layer);
				CHECK(TileX(tile) == x);
				CHECK(TilePlaneY(tile) == y);
				CHECK(GetTileLayer(tile) == layer);
				CHECK(TileY(tile) == to_underlying(layer) * Map::SizeY() + y);
				CHECK(tile.base() == to_underlying(layer) * Map::PlaneSize() + y * Map::SizeX() + x);
			}
		}
	}
}

//I-5: each layer has its own void fence, with and without freeform edges.
TEST_CASE("Per-layer void fence")
{
	Map::Allocate(64, 64, 3);
	bool saved_freeform = _settings_game.construction.freeform_edges;

	for (bool freeform : {false, true}) {
		_settings_game.construction.freeform_edges = freeform;
		for (MapLayer layer : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space}) {
			for (uint x = 0; x < Map::SizeX(); ++x) {
				for (uint y = 0; y < Map::SizeY(); ++y) {
					TileIndex tile = TileXYL(x, y, layer);
					//South edges are always void. North edges are void only when freeform edges are on.
					bool on_fence = x == Map::MaxX() || y == Map::MaxY() || (freeform && (x == 0 || y == 0));
					CHECK(IsInnerTile(tile) == !on_fence);
				}
			}
		}
	}

	_settings_game.construction.freeform_edges = saved_freeform;
}

//I-2: a one-tile step from an inner tile stays in its layer.
TEST_CASE("Neighbour steps stay in their layer")
{
	Map::Allocate(64, 64, 3);
	bool saved_freeform = _settings_game.construction.freeform_edges;

	for (bool freeform : {false, true}) {
		_settings_game.construction.freeform_edges = freeform;
		for (MapLayer layer : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space}) {
			for (uint x = 0; x < Map::SizeX(); ++x) {
				for (uint y = 0; y < Map::SizeY(); ++y) {
					TileIndex tile = TileXYL(x, y, layer);
					if (!IsInnerTile(tile)) continue;

					for (DiagDirection dir : EnumRange(DiagDirection::End)) {
						TileIndexDiffC diff = TileIndexDiffCByDiagDir(dir);
						//Ask TileAdd only when the step stays on the plane, so the debug layer assert is not tripped by an off-map step.
						if (!NeighbourStaysOnPlane(tile, diff)) continue;
						TileIndex next = TileAddByDiagDir(tile, dir);
						CHECK(GetTileLayer(next) == layer);
						CHECK(next == TileXYL(static_cast<uint>(static_cast<int>(TileX(tile)) + diff.x), static_cast<uint>(static_cast<int>(TilePlaneY(tile)) + diff.y), layer));
					}
					for (Direction dir : EnumRange(Direction::End)) {
						TileIndexDiffC diff = TileIndexDiffCByDir(dir);
						if (!NeighbourStaysOnPlane(tile, diff)) continue;
						TileIndex next = TileAddByDir(tile, dir);
						CHECK(GetTileLayer(next) == layer);
						CHECK(next == TileXYL(static_cast<uint>(static_cast<int>(TileX(tile)) + diff.x), static_cast<uint>(static_cast<int>(TilePlaneY(tile)) + diff.y), layer));
					}
				}
			}
		}
	}

	_settings_game.construction.freeform_edges = saved_freeform;
}

//AD-5: distances use the plane, and a cross-layer pair has no diagonal direction.
TEST_CASE("Plane distances ignore the layer")
{
	Map::Allocate(64, 64, 3);

	TileIndex origin = TileXYL(1, 1, MapLayer::Surface);
	TileIndex same_spot_below = MultiLayerMap::TileInLayer(origin, MapLayer::Underground);
	CHECK(MultiLayerMap::ProjectToSurface(same_spot_below) == origin);
	CHECK(DistanceManhattan(origin, same_spot_below) == 0);
	CHECK(DistanceSquare(origin, same_spot_below) == 0);
	CHECK(DistanceMax(origin, same_spot_below) == 0);
	CHECK(DistanceMaxPlusManhattan(origin, same_spot_below) == 0);

	TileIndex other = TileXYL(4, 6, MapLayer::Underground);
	CHECK(DistanceManhattan(origin, other) == 8);
	CHECK(DistanceSquare(origin, other) == 34);
	CHECK(DistanceMax(origin, other) == 5);
	CHECK(DistanceMaxPlusManhattan(origin, other) == 13);
	CHECK(DiagdirBetweenTiles(origin, other) == DiagDirection::Invalid);

	TileIndex south_east = TileXYL(1, 6, MapLayer::Surface);
	CHECK(DiagdirBetweenTiles(origin, south_east) == DiagDirection::SE);
}

//I-5: a layer walk covers that plane only, and random tiles stay in the requested layer.
TEST_CASE("IterateLayer and RandomTileInLayer")
{
	Map::Allocate(64, 64, 3);

	for (MapLayer layer : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space}) {
		uint seen = 0;
		for (TileIndex tile : MultiLayerMap::IterateLayer(layer)) {
			CHECK(GetTileLayer(tile) == layer);
			++seen;
		}
		CHECK(seen == Map::PlaneSize());

		//High bits in the seed must be masked back into the requested layer.
		for (uint32_t n = 0; n < 10000; ++n) {
			uint32_t seed = n * 0x9E3779B9u;
			CHECK(GetTileLayer(MultiLayerMap::RandomTileInLayer(layer, seed)) == layer);
			CHECK(MultiLayerMap::IsTileInLayer(MultiLayerMap::RandomTileInLayer(layer, seed), layer));
		}
	}

	for (uint32_t n = 0; n < 10000; ++n) {
		CHECK(GetTileLayer(RandomTileSeed(n * 0x9E3779B9u)) == MapLayer::Surface);
	}
}

//C6: a spiral, an expanded area and a slope neighbour stay in the tile's layer.
TEST_CASE("Migrated spatial helpers stay in their layer")
{
	Map::Allocate(64, 64, 3);

	TileIndex centre = TileXYL(20, 20, MapLayer::Space);
	for (TileIndex tile : SpiralTileSequence(centre, 5)) {
		CHECK(GetTileLayer(tile) == MapLayer::Space);
	}

	OrthogonalTileArea area(TileXYL(10, 10, MapLayer::Underground), 2, 2);
	area.Expand(1);
	CHECK(GetTileLayer(area.tile) == MapLayer::Underground);
	CHECK(TilePlaneY(area.tile) == 9);
	CHECK(TileX(area.tile) == 9);

	//The south neighbour is underground. A surface tile at the same column must not affect the slope.
	TileIndex origin = TileXYL(5, 5, MapLayer::Underground);
	SetTileHeight(TileXYL(5, 6, MapLayer::Underground), 3);
	SetTileHeight(TileXYL(5, 6, MapLayer::Surface), 0);
	auto [slope, z] = GetTileSlopeZ(origin);
	CHECK(z == 0);
	CHECK((slope & SLOPE_E) != SLOPE_FLAT);

	CHECK(MultiLayerMap::ProjectToSurface(origin) == TileXYL(5, 5, MapLayer::Surface));
}

//Underground initialises as flat rock with its own void fence, and does not depend on freeform edges for the south fence.
TEST_CASE("InitialiseLayer underground ground and fence")
{
	Map::Allocate(64, 64, 3);
	bool saved_freeform = _settings_game.construction.freeform_edges;

	for (bool freeform : {false, true}) {
		_settings_game.construction.freeform_edges = freeform;
		MultiLayerMap::InitialiseLayer(MapLayer::Underground);
		uint base_height = MultiLayerMap::GetRules(MapLayer::Underground).base_height;

		for (uint x = 0; x < Map::SizeX(); ++x) {
			for (uint y = 0; y < Map::SizeY(); ++y) {
				TileIndex tile = TileXYL(x, y, MapLayer::Underground);
				bool south_fence = x == Map::MaxX() || y == Map::MaxY();
				bool north_fence = freeform && (x == 0 || y == 0);
				if (south_fence || north_fence) {
					CHECK(IsTileType(tile, TileType::Void));
				} else {
					CHECK(IsTileType(tile, TileType::Clear));
					CHECK(IsClearGround(tile, ClearGround::Rocks));
					CHECK(GetClearDensity(tile) == 3);
					CHECK(TileHeight(tile) == base_height);
				}
			}
		}
	}

	_settings_game.construction.freeform_edges = saved_freeform;
}

/**
 * A hand-built pair on a 64x64x3 map: shaft exits reach the other layer and every other exit stays a neighbour.
 */
TEST_CASE("Layer portal neighbour mapping and relocation")
{
	Map::Allocate(64, 64, MAX_MAP_LAYERS);
	TileIndex surface = TileXYL(10, 10, MapLayer::Surface);
	TileIndex underground = TileXYL(10, 10, MapLayer::Underground);

	//Every shaft direction: build the pair, then check its bits, its neighbour steps and the relocation across the shaft.
	for (DiagDirection shaft : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
		MakeRailLayerPortal(surface, OWNER_NONE, shaft, MapLayer::Underground, RAILTYPE_RAIL);
		MakeRailLayerPortal(underground, OWNER_NONE, ReverseDiagDir(shaft), MapLayer::Surface, RAILTYPE_RAIL);

		CHECK(GetLayerPortalTransportType(surface) == TransportType::Rail);
		CHECK(GetLayerPortalShaftDirection(surface) == shaft);
		CHECK(GetLayerPortalTargetLayer(surface) == MapLayer::Underground);
		//I-8: the other end's shaft is the reverse.
		CHECK(GetLayerPortalShaftDirection(GetLayerPortalPairTile(surface)) == ReverseDiagDir(shaft));
		CHECK_FALSE(HasLayerPortalReservation(surface));
		SetLayerPortalReservation(surface, true);
		CHECK(HasLayerPortalReservation(surface));
		SetLayerPortalReservation(surface, false);

		//Only the shaft exit leaves the layer. Every other exit is the plain neighbour on the same layer.
		for (DiagDirection exit : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			TileIndex expected = (exit == shaft) ? underground : TileAddByDiagDir(surface, exit);
			CHECK(GetLayerPortalNextTile(surface, exit) == expected);
			DiagDirection underground_shaft = ReverseDiagDir(shaft);
			TileIndex back = (exit == underground_shaft) ? surface : TileAddByDiagDir(underground, exit);
			CHECK(GetLayerPortalNextTile(underground, exit) == back);
		}

		TileIndexDiffC step = TileIndexDiffCByDiagDir(shaft);
		int x = (static_cast<int>(TileX(surface)) + step.x) * static_cast<int>(TILE_SIZE) + 4;
		int y = (static_cast<int>(TileY(surface)) + step.y) * static_cast<int>(TILE_SIZE) + 4;
		TranslateIntoLayerPortalPair(surface, x, y);
		CHECK(TileVirtXY(x, y) == underground);
	}

	//Road type masks are game state. The test sets the original types so SetRoadTypes accepts ROADTYPE_ROAD.
	ResetRoadTypes();
	MakeRoadLayerPortal(surface, OWNER_NONE, DiagDirection::SE, MapLayer::Underground, ROADTYPE_ROAD);
	CHECK(GetLayerPortalTransportType(surface) == TransportType::Road);
	CHECK(GetRoadTypeRoad(surface) == ROADTYPE_ROAD);
	CHECK(GetRoadTypeTram(surface) == INVALID_ROADTYPE);
	CHECK(MultiLayerMap::GetPortalTargetLayer(MapLayer::Surface) == MapLayer::Underground);
	CHECK(MultiLayerMap::GetPortalTargetLayer(MapLayer::Space) == MapLayer::Invalid);
}

/**
 * FD-3 and FD-4 on a hand-built pair: which part of an end hides a vehicle, and which sides answer a track query.
 */
TEST_CASE("Layer portal visibility and track status")
{
	Map::Allocate(64, 64, MAX_MAP_LAYERS);
	//Road type masks are game state. The original types let SetRoadTypes accept ROADTYPE_ROAD.
	ResetRoadTypes();
	TileIndex surface = TileXYL(10, 10, MapLayer::Surface);
	TileIndex underground = TileXYL(10, 10, MapLayer::Underground);
	const int base_x = static_cast<int>(TileX(surface) * TILE_SIZE);
	const int base_y = static_cast<int>(TileY(surface) * TILE_SIZE);
	const int tile_size = static_cast<int>(TILE_SIZE);

	//Every shaft direction: first the hidden band of the end, then the track status of a rail pair and a road pair.
	for (DiagDirection shaft : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
		//Walk from the door edge to the shaft edge: hidden starts once and never ends.
		TileIndexDiffC step = TileIndexDiffCByDiagDir(shaft);
		bool was_hidden = false;
		int hidden_count = 0;
		for (int depth = 0; depth < tile_size; depth++) {
			//SE and SW head to larger coordinates, so their door edge is the low end of the tile.
			int along = (step.x + step.y > 0) ? depth : tile_size - 1 - depth;
			int x = base_x + (step.x != 0 ? along : 5);
			int y = base_y + (step.y != 0 ? along : 5);
			bool hidden = IsHiddenInLayerPortal(shaft, x, y);
			CHECK((hidden || !was_hidden));
			//The position across the axis never matters.
			CHECK(IsHiddenInLayerPortal(shaft, x + (step.x == 0 ? 7 : 0), y + (step.y == 0 ? 7 : 0)) == hidden);
			was_hidden = hidden;
			if (hidden) hidden_count++;
		}
		//The tunnel frames hide the last 4 units of NE and NW ends and the last 8 of SE and SW ends.
		CHECK(hidden_count == ((shaft == DiagDirection::NE || shaft == DiagDirection::NW) ? 4 : 8));

		//Rail: the door side and a query without a side see the axis; every other side sees nothing.
		MakeRailLayerPortal(surface, OWNER_NONE, shaft, MapLayer::Underground, RAILTYPE_RAIL);
		MakeRailLayerPortal(underground, OWNER_NONE, ReverseDiagDir(shaft), MapLayer::Surface, RAILTYPE_RAIL);
		TrackdirBits axis = TrackBitsToTrackdirBits(DiagDirToDiagTrack(shaft));
		CHECK(GetTileTrackStatus(surface, TransportType::Rail, RoadTramType::Invalid).trackdirs == axis);
		for (DiagDirection side : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			TrackdirBits expected = (side == ReverseDiagDir(shaft)) ? axis : TrackdirBits{};
			CHECK(GetTileTrackStatus(surface, TransportType::Rail, RoadTramType::Invalid, side).trackdirs == expected);
		}
		CHECK(GetTileTrackStatus(surface, TransportType::Road, RoadTramType::Road).trackdirs.None());

		//Road: the same sides, and trams see nothing.
		MakeRoadLayerPortal(surface, OWNER_NONE, shaft, MapLayer::Underground, ROADTYPE_ROAD);
		MakeRoadLayerPortal(underground, OWNER_NONE, ReverseDiagDir(shaft), MapLayer::Surface, ROADTYPE_ROAD);
		CHECK(GetTileTrackStatus(surface, TransportType::Road, RoadTramType::Road, ReverseDiagDir(shaft)).trackdirs == axis);
		CHECK(GetTileTrackStatus(surface, TransportType::Road, RoadTramType::Road, shaft).trackdirs.None());
		CHECK(GetTileTrackStatus(surface, TransportType::Road, RoadTramType::Tram).trackdirs.None());
	}
}

//FD-10: two tiles share a layer only when both are on the map and on the same plane.
TEST_CASE("CheckTilesShareLayer")
{
	Map::Allocate(64, 64, MAX_MAP_LAYERS);
	TileIndex surface_a = TileXYL(3, 4, MapLayer::Surface);
	TileIndex surface_b = TileXYL(60, 61, MapLayer::Surface);
	TileIndex underground = TileXYL(3, 4, MapLayer::Underground);
	CHECK(CheckTilesShareLayer(surface_a, surface_b).Succeeded());
	CHECK(CheckTilesShareLayer(underground, underground).Succeeded());
	CHECK(CheckTilesShareLayer(surface_a, underground).Failed());
	CHECK(CheckTilesShareLayer(surface_a, TileIndex{Map::Size()}).Failed());
}
