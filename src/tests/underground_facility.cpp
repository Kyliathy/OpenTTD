/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_facility.cpp Actual ordinary station/terminal/depot commands on native mine layers. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 6 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_facility_fixture.h"
#include "../station_cmd.h"
#include "../rail_cmd.h"
#include "../road_cmd.h"
#include "../rail.h"
#include "../road.h"
#include "../newgrf_station.h"
#include "../newgrf_roadstop.h"
#include "../order_base.h"
#include "../depot_base.h"
#include "../roadstop_base.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Both rock layers use real StationIDs, complete approach checks, same-layer packets and ordinary order destinations. */
TEST_CASE("Mine freight facilities build and receive through joined or separate normal stations", "[underground-facility]")
{
	size_t mine = 0;
	bool join = false;
	SECTION("Underground separate") {}
	SECTION("Underground joined") { join = true; }
	SECTION("Deep separate") { mine = 5; }
	SECTION("Deep joined") { mine = 5; join = true; }
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFacilityRig rig(mine);
	AutoRestoreBackup<CompanyID> local(_local_company, _current_company);
	AutoRestoreBackup distant_join(_settings_game.station.distant_join_stations, true);
	const auto stock = ProduceLayerMineCargo(*rig.industry, 74, true).amount;
	REQUIRE(stock > 0);
	const auto reserve = rig.Reserve();
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(CmdBuildRailStation(DoCommandFlag::Execute, rig.RailSite(), RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Failed());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(Station::GetNumItems() == 0);
	rig.OpenRailSite();
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, rig.RailSite(), RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	Station *rail = Station::GetByTile(rig.RailSite());
	REQUIRE(rail != nullptr);
	CHECK(GetTileLayer(rail->xy) == GetTileLayer(rig.request.tile));
	CHECK(rail->owner == rig.request.company);
	CHECK(rail->facilities.Test(StationFacility::Train));
	CHECK_FALSE(rail->TileIsInCatchment(MultiLayerMap::ProjectToSurface(rig.request.tile)));
	CHECK(rig.industry->stations_near.contains(rail));
	CHECK(rig.BuildTerminal(DoCommandFlag::Execute, NEW_STATION).Failed());
	rig.OpenRoadSite();
	const auto before_join = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money;
	{
		AutoRestoreBackup spread(_settings_game.station.station_spread, decltype(_settings_game.station.station_spread){3});
		const auto refused = rig.BuildTerminal(DoCommandFlag::Execute, rail->index);
		CHECK(refused.Failed());
		CHECK(refused.GetErrorMessage() == STR_ERROR_STATION_TOO_SPREAD_OUT);
	}
	{
		REQUIRE(Company::CanAllocateItem());
		std::unique_ptr<Company> foreign{Company::Create()};
		foreign->avail_roadtypes.Set(ROADTYPE_ROAD);
		AutoRestoreBackup actor(_current_company, foreign->index);
		CHECK(rig.BuildTerminal(DoCommandFlag::Execute, rail->index).Failed());
	}
	CHECK(Station::GetNumItems() == 1);
	CHECK(RoadStop::GetNumItems() == 0);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before_join);
	CHECK(Company::Get(_current_company)->money == cash);
	REQUIRE(rig.BuildTerminal(DoCommandFlag::Execute, join ? rail->index : NEW_STATION).Succeeded());
	Station *road = Station::GetByTile(rig.RoadSite());
	REQUIRE(road != nullptr);
	CHECK(road->facilities.Test(StationFacility::TruckStop));
	CHECK((road->index == rail->index) == join);
	CHECK(rig.industry->stations_near.contains(road));
	Order order;
	order.MakeGoToStation(road->index);
	CHECK(order.GetDestination() == road->index);
	const CargoType cargo = rig.industry->produced[0].cargo;
	// Announced service/rating fields admit native station pickup; actual conventional vehicle loading is a later fleet integration.
	for (Station *station : {rail, road}) {
		station->goods[cargo].rating = UINT8_MAX;
		station->goods[cargo].last_speed = 100;
	}
	REQUIRE(DispatchLayerMineCargo(*rig.industry) == stock);
	const auto queued = rail->goods[cargo].TotalCount() + (join ? 0 : road->goods[cargo].TotalCount());
	CHECK(queued == stock);
	CHECK(rig.industry->produced[0].waiting == 0);
	CHECK(rig.Reserve() == reserve);
}

/** Compatible depots require their own full excavated entrance and preserve terrain/reserve on refused construction. */
TEST_CASE("Train and road service depots use actual rock-plane excavation guards", "[underground-depot]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFacilityRig rig(0);
	// Both native rock planes use the same actual depot commands and independent same-layer approach cells.
	for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		const TileIndex train_site = TileXYL(40, 40, layer);
		const TileIndex road_site = TileXYL(45, 40, layer);
		rig.Open(std::array{train_site, road_site});
		const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
		CHECK(CmdBuildTrainDepot(DoCommandFlag::Execute, train_site, RAILTYPE_RAIL, DiagDirection::SE).Failed());
		CHECK(CmdBuildRoadDepot(DoCommandFlag::Execute, road_site, ROADTYPE_ROAD, DiagDirection::SE).Failed());
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
		const auto approach = TileIndexDiffCByDiagDir(DiagDirection::SE);
		rig.Open(std::array{TileAddWrap(train_site, approach.x, approach.y), TileAddWrap(road_site, approach.x, approach.y)});
		REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, train_site, RAILTYPE_RAIL, DiagDirection::SE).Succeeded());
		REQUIRE(CmdBuildRoadDepot(DoCommandFlag::Execute, road_site, ROADTYPE_ROAD, DiagDirection::SE).Succeeded());
		CHECK(Depot::GetByTile(train_site)->xy == train_site);
		CHECK(Depot::GetByTile(road_site)->xy == road_site);
		CHECK(GetTileOwner(train_site) == rig.request.company);
		CHECK(GetTileOwner(road_site) == rig.request.company);
	}
}
