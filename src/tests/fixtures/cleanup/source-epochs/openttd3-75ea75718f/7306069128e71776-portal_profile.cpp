/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file portal_profile.cpp Profile-specific Deep rail matrix and preserved portal geometry. @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 1 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../layer_portal_map.h"
#include "../road.h"
#include "../tile_map.h"
#include "../layer_portal_cmd.h"
#include "../rail.h"
#include "../economy_type.h"
#include "../economy_func.h"
#include "../core/backup_type.hpp"

#include "../safeguards.h"

/** Native rail extends only the intended Deep pair while every existing connection remains available. */
TEST_CASE("Portal profile matrix adds Underground Deep rail and preserves existing pairs", "[portal-profile]")
{
	ExcavationFixture fixture;
	const auto &native = MultiLayerMap::GetRules(MapLayer::DeepUnderground);
	CHECK(native.ground == MapLayerGround::DeepBedrock);
	// Each supported rail family follows the new profile; unrelated Surface-only infrastructure is still excluded.
	for (auto family : {MapLayerConstruction::Rail, MapLayerConstruction::RailStation, MapLayerConstruction::TrainDepot, MapLayerConstruction::Signal}) CHECK(native.allowed.Test(family));
	CHECK_FALSE(native.allowed.Test(MapLayerConstruction::Tram));
	CHECK_FALSE(native.allowed.Test(MapLayerConstruction::Terraform));
	// Both endpoint orders consume the same pair authority, covering every allocated layer combination and transport kind.
	for (auto a : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space, MapLayer::DeepUnderground}) {
		for (auto b : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space, MapLayer::DeepUnderground}) {
			const auto low = std::min(a, b), high = std::max(a, b);
			const bool tunnel = low == MapLayer::Surface && high == MapLayer::Underground;
			const bool deep_rail = low == MapLayer::Underground && high == MapLayer::DeepUnderground;
			const bool elevator = high == MapLayer::DeepUnderground && (low == MapLayer::Surface || low == MapLayer::Underground);
			CHECK(MultiLayerMap::IsPortalPairAllowed(LayerPortalKind::RailTunnel, a, b) == (tunnel || deep_rail));
			CHECK(MultiLayerMap::IsPortalPairAllowed(LayerPortalKind::RoadTunnel, a, b) == tunnel);
			CHECK(MultiLayerMap::IsPortalPairAllowed(LayerPortalKind::RoadElevator, a, b) == elevator);
		}
	}
	CHECK(MultiLayerMap::GetPortalTargetLayer(MapLayer::Underground, LayerPortalKind::RailTunnel, MapLayer::Invalid) == MapLayer::Surface);
	CHECK(MultiLayerMap::GetPortalTargetLayer(MapLayer::Underground, LayerPortalKind::RailTunnel, MapLayer::DeepUnderground) == MapLayer::DeepUnderground);
	CHECK(MultiLayerMap::GetPortalTargetLayer(MapLayer::DeepUnderground, LayerPortalKind::RailTunnel, MapLayer::Invalid) == MapLayer::Underground);
	CHECK(MultiLayerMap::GetPortalTargetLayer(MapLayer::Underground, LayerPortalKind::RailTunnel, MapLayer::Space) == MapLayer::Invalid);
	ResetMultiLayerProfileSelection();
	CHECK_FALSE(MultiLayerMap::GetRules(MapLayer::DeepUnderground).allowed.Test(MapLayerConstruction::Rail));
	CHECK_FALSE(MultiLayerMap::IsPortalPairAllowed(LayerPortalKind::RailTunnel, MapLayer::Underground, MapLayer::DeepUnderground));
	CHECK(MultiLayerMap::GetPortalTargetLayer(MapLayer::DeepUnderground, LayerPortalKind::RailTunnel, MapLayer::Invalid) == MapLayer::Invalid);
}

/** Direct Surface/Deep elevator endpoints retain their physical pair and never mutate the intervening Underground tile. */
TEST_CASE("Portal profile preserves layer IDs and direct elevator intervening geometry", "[portal-profile]")
{
	ExcavationFixture fixture;
	ResetRoadTypes();
	STATIC_REQUIRE(to_underlying(MapLayer::Surface) == 0);
	STATIC_REQUIRE(to_underlying(MapLayer::Underground) == 1);
	STATIC_REQUIRE(to_underlying(MapLayer::Space) == 2);
	STATIC_REQUIRE(to_underlying(MapLayer::DeepUnderground) == 3);
	const auto surface = TileXYL(20, 20, MapLayer::Surface);
	const auto intervening = MultiLayerMap::TileInLayer(surface, MapLayer::Underground);
	const auto deep = MultiLayerMap::TileInLayer(surface, MapLayer::DeepUnderground);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto type = GetTileType(intervening);
	const auto owner = GetTileOwner(intervening);
	const auto m5 = Tile{intervening}.m5();
	// The existing public tile constructor represents only its two housings; this fixture does not claim the later sealed command boundary.
	MakeRoadLayerPortal(surface, _current_company, DiagDirection::SE, MapLayer::DeepUnderground, ROADTYPE_ROAD);
	MakeRoadLayerPortal(deep, _current_company, DiagDirection::NW, MapLayer::Surface, ROADTYPE_ROAD);
	CHECK(GetLayerPortalPairTile(surface) == deep);
	CHECK(GetLayerPortalNextTile(surface, DiagDirection::SE) == deep);
	CHECK(GetLayerPortalKind(surface) == LayerPortalKind::RoadElevator);
	CHECK(GetTileType(intervening) == type);
	CHECK(GetTileOwner(intervening) == owner);
	CHECK(Tile{intervening}.m5() == m5);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
}

/** Destination selection feeds real command quotes; estimates preserve both endpoints, geology, funds and RNG. */
TEST_CASE("Portal destination command quotes are unchanged estimates", "[portal-quote]")
{
	ExcavationFixture fixture;
	ResetRailTypes();
	const auto source = TileXYL(20, 20, MapLayer::Underground);
	const auto target = MultiLayerMap::GetPortalTargetLayer(MapLayer::Underground, LayerPortalKind::RailTunnel, MapLayer::DeepUnderground);
	REQUIRE(target == MapLayer::DeepUnderground);
	const auto destination = MultiLayerMap::TileInLayer(source, target);
	AutoRestoreBackup price(_price[Price::BuildTunnel], Money{17});
	const auto funds = Company::Get(_current_company)->money;
	const auto rng = _random;
	const auto solid = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(CmdBuildLayerPortal({}, source, TransportType::Rail, target, DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE).Failed());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == solid);
	const std::array floor{source, TileAddByDiagDir(source, DiagDirection::NW), destination, TileAddByDiagDir(destination, DiagDirection::SE)};
	fixture.Open(floor);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto quote = CmdBuildLayerPortal({}, source, TransportType::Rail, target, DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE);
	CHECK(quote.Succeeded());
	CHECK(quote.GetCost() >= 68);
	CHECK(GetTileType(source) == TileType::Clear);
	CHECK(GetTileType(destination) == TileType::Clear);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(Company::Get(_current_company)->money == funds);
	CHECK(_random.state[0] == rng.state[0]);
	CHECK(_random.state[1] == rng.state[1]);
}

/** Every ordered layer pair and shaft heading is exercised through the actual native command, beyond structural matrix declarations. */
TEST_CASE("Native portal commands cover every layer pair and shaft heading", "[portal-acceptance]")
{
	ExcavationFixture fixture;
	ResetLayerPortalWorkState(); ResetRailTypes(); ResetRoadTypes();
	// Prepare only explicit fixture floor at both rock origins so topology, rather than inaccessible source ground, determines each estimate.
	for (MapLayer rock : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		std::vector<TileIndex> floor;
		for (int y = 19; y <= 21; ++y) for (int x = 19; x <= 21; ++x) floor.push_back(TileXYL(x, y, rock));
		fixture.Open(floor);
	}
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto funds = Company::Get(_current_company)->money;
	const auto rng = _random;
	// Actual commands must preserve all old tunnel/elevator pairs and add only Underground/Deep rail in either order, for every door heading.
	for (auto a : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space, MapLayer::DeepUnderground}) {
		for (auto b : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space, MapLayer::DeepUnderground}) {
			const auto low = std::min(a, b), high = std::max(a, b);
			for (auto transport : {TransportType::Rail, TransportType::Road}) {
				const bool tunnel = low == MapLayer::Surface && high == MapLayer::Underground;
				const bool deep = high == MapLayer::DeepUnderground && (low == MapLayer::Underground || (low == MapLayer::Surface && transport == TransportType::Road));
				for (auto shaft : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
					const auto result = CmdBuildLayerPortal({}, TileXYL(20, 20, a), transport, b, shaft, RAILTYPE_RAIL, ROADTYPE_ROAD);
					CHECK(result.Succeeded() == (tunnel || deep));
				}
			}
		}
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo); CHECK(GetLayerPortalWorkPairCount() == 0);
	CHECK(Company::Get(_current_company)->money == funds);
	CHECK(_random.state[0] == rng.state[0]); CHECK(_random.state[1] == rng.state[1]);
}
