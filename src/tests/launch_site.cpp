/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_site.cpp Shared launch geometry before companies and its native obstruction boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group3
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_site.h"
#include "../launch_center_cmd.h"
#include "../map_layer.h"
#include "../clear_map.h"
#include "../water_map.h"
#include "../void_map.h"
#include "../bridge_map.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../settings_type.h"
#include "../timer/timer_game_calendar.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Initialize only isolated test terrain; native placement remains responsible for all paid game-world changes. */
static void PrepareGeometryMap()
{
	Map::Allocate(64, 64, MAX_MAP_LAYERS);
	// Keep every layer's interior flat while retaining native void edges, allowing explicit cross-layer refusal checks.
	for (const auto tile : Map::Iterate()) {
		if (IsInnerTile(tile)) MakeClear(tile, ClearGround::Grass, 0);
		else MakeVoid(tile);
	}
}

/** Verify generation-time geometry does not depend on player existence or technology availability. */
TEST_CASE("Launch site geometry is queryable before company and technology initialization", "[launch-site]")
{
	AutoRestoreBackup edges(_settings_game.construction.freeform_edges, false);
	AutoRestoreBackup actor(_current_company, OWNER_NONE);
	AutoRestoreBackup year(TimerGameCalendar::year, TimerGameCalendar::Year{1989});
	PrepareGeometryMap();
	const auto random = _random, interactive = _interactive_random;
	const auto companies = Company::GetNumItems();
	const auto anchor = TileXYL(10, 10, MapLayer::Surface);
	MakeRiver(TileXYL(10, 9, MapLayer::Surface), 0);
	// Every native rotation has the same company-independent geometry policy, while paid placement retains its own gates.
	for (uint8_t rotation = 0; rotation < 4; ++rotation) {
		const auto geometry = GetLaunchCenterSiteGeometryBlock(anchor, rotation);
		CHECK(geometry.Succeeded()); CHECK(geometry.GetCost() == 0);
		const auto placement = CmdBuildLaunchCenter({}, anchor, rotation, NEW_STATION, true);
		CHECK(placement.Failed()); CHECK(placement.GetErrorMessage() == STR_ERROR_LAUNCH_CENTER_UNAVAILABLE);
	}
	CHECK(Company::GetNumItems() == companies);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]); CHECK(_interactive_random.state[1] == interactive.state[1]);
}

/** Check the complete guarded footprint and each terrain obstruction using independent native error literals. */
TEST_CASE("Launch site geometry rejects cross-layer bounds water slopes and overhead bridges", "[launch-site]")
{
	AutoRestoreBackup edges(_settings_game.construction.freeform_edges, false);
	PrepareGeometryMap();
	const auto anchor = TileXYL(10, 10, MapLayer::Surface);
	CHECK(GetLaunchCenterSiteGeometryBlock(anchor, 0).GetErrorMessage() == STR_ERROR_LAUNCH_CENTER_WATER);
	MakeRiver(TileXYL(10, 9, MapLayer::Surface), 0);
	REQUIRE(GetLaunchCenterSiteGeometryBlock(anchor, 0).Succeeded());
	// Projected overlap grants no Surface build permission, even when the other plane's terrain is identical.
	for (MapLayer layer : {MapLayer::Underground, MapLayer::Space, MapLayer::DeepUnderground}) {
		const auto tile = TileXYL(10, 10, layer);
		CHECK(GetLaunchCenterSiteGeometryBlock(tile, 0).GetErrorMessage() == STR_ERROR_LAUNCH_CENTER_SURFACE);
		CHECK(GetLaunchCenterTerrainTileBlock(tile, 0).GetErrorMessage() == STR_ERROR_LAUNCH_CENTER_SURFACE);
	}
	CHECK(GetLaunchCenterSiteGeometryBlock(INVALID_TILE, 0).Failed());
	CHECK(GetLaunchCenterSiteGeometryBlock(anchor, 4).Failed());
	CHECK(GetLaunchCenterSiteGeometryBlock(TileXYL(62, 62, MapLayer::Surface), 0).Failed());
	const auto corner = TileXYL(13, 12, MapLayer::Surface);
	SetTileHeight(corner, 1);
	CHECK(GetLaunchCenterSiteGeometryBlock(anchor, 0).GetErrorMessage() == STR_ERROR_FLAT_LAND_REQUIRED);
	SetTileHeight(corner, 0);
	REQUIRE(GetLaunchCenterSiteGeometryBlock(anchor, 0).Succeeded());
	MakeSea(corner);
	CHECK(GetLaunchCenterSiteGeometryBlock(anchor, 0).GetErrorMessage() == STR_ERROR_FLAT_LAND_REQUIRED);
	MakeClear(corner, ClearGround::Grass, 0);
	SetBridgeMiddle(corner, Axis::X);
	CHECK(GetLaunchCenterSiteGeometryBlock(anchor, 0).GetErrorMessage() == STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
	ClearBridgeMiddle(corner);
	CHECK(GetLaunchCenterSiteGeometryBlock(anchor, 0).Succeeded());
	CHECK(GetLaunchCenterTerrainTileBlock(anchor, 1).GetErrorMessage() == STR_ERROR_FLAT_LAND_REQUIRED);
}
