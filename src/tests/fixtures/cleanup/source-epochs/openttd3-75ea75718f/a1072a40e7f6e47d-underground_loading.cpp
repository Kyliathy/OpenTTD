/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_loading.cpp Native bought carriers, normal station orders and actual packet loading from finite mines. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Groups 6.3B and 7B */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_fleet_fixture.h"
#include "../order_cmd.h"
#include "../economy_func.h"
#include "../safeguards.h"

/** Actual mine packets enter a native purchased truck through the normal loading owner on either rock layer and station-join policy. */
TEST_CASE("Native mine trucks load finite output at joined and independent underground terminals", "[underground-loading]")
{
	size_t mine = 0;
	bool join = false;
	SECTION("Underground separate") {}
	SECTION("Underground joined") { join = true; }
	SECTION("Deep separate") { mine = 5; }
	SECTION("Deep joined") { mine = 5; join = true; }
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFleetRig rig(mine);
	AutoRestoreBackup distant_join(_settings_game.station.distant_join_stations, true);
	rig.OpenRailSite();
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, rig.RailSite(), RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	Station *rail = Station::GetByTile(rig.RailSite());
	rig.OpenRoadSite();
	REQUIRE(rig.BuildTerminal(DoCommandFlag::Execute, join ? rail->index : NEW_STATION).Succeeded());
	Station *terminal = Station::GetByTile(rig.RoadSite());
	const auto cargo = rig.industry->produced[0].cargo;
	const auto depot = rig.BuildDepot(GetTileLayer(rig.request.tile), true);
	Vehicle *truck = rig.Buy(depot, 7, cargo);
	Order order;
	order.MakeGoToStation(terminal->index);
	order.SetStopLocation(OrderStopLocation::FarEnd); // Native road orders have no train platform-position choice, matching the ordinary order GUI.
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, truck->index, 0, order).Succeeded());
	CHECK(truck->GetOrder(0)->GetDestination() == terminal->index);
	const auto stock = ProduceLayerMineCargo(*rig.industry, 74, true).amount;
	REQUIRE(stock > 0);
	const auto reserve = rig.Reserve();
	terminal->goods[cargo].rating = UINT8_MAX;
	terminal->goods[cargo].last_speed = 100;
	REQUIRE(DispatchLayerMineCargo(*rig.industry) == stock);
	CHECK(terminal->goods[cargo].TotalCount() == stock);
	// Position the actual purchased carrier at its actual terminal as the fixture arrival boundary; routing/elevator travel has separate acceptance.
	truck->tile = rig.RoadSite();
	truck->x_pos = TileX(truck->tile) * TILE_SIZE + TILE_SIZE / 2;
	truck->y_pos = TileY(truck->tile) * TILE_SIZE + TILE_SIZE / 2;
	truck->z_pos = 0;
	truck->UpdatePosition();
	truck->vehstatus.Reset({VehState::Stopped, VehState::Hidden});
	truck->last_station_visited = terminal->index;
	truck->current_order = *truck->GetOrder(0);
	truck->BeginLoading();
	// Tick ordinary loading with a bounded native interval; packets, capacity, station stock and loading time remain economy-owned.
	for (uint tick = 0; tick < 128; ++tick) LoadUnloadStation(terminal);
	REQUIRE(truck->cargo.TotalCount() > 0);
	CHECK(truck->cargo.TotalCount() <= truck->cargo_cap);
	CHECK(truck->cargo.TotalCount() + terminal->goods[cargo].TotalCount() == stock);
	CHECK(rig.industry->produced[0].waiting == 0);
	CHECK(rig.Reserve() == reserve);
	CHECK_FALSE(terminal->TileIsInCatchment(MultiLayerMap::ProjectToSurface(rig.request.tile)));
}
