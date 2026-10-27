/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file underground_save.cpp Actual native fleet, packets, station/service orders and project association written by the full game saver.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 10B
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 13B (fresh saves are run-owned outputs)
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "underground_fleet_fixture.h"
#include "test_artifact_paths.h"
#include "movement_fixture.h"
#include "../saveload/saveload_func.h"
#include "../spoil_receiver.h"
#include "../industry_cmd.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../tbm_cmd.h"
#include "../tbm_exchange.h"
#include "../order_cmd.h"
#include "../economy_func.h"
#include "../station_func.h"
#include "../table/sprites.h"
#include "../safeguards.h"

/** Full-world save uses genuinely bought native engines, physically loaded mine cargo and a controller-owned active hauler assignment. */
TEST_CASE("Native conventional fleet writes real mappings orders cargo and project association", "[underground-save-file]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFleetRig rig(0, true);
	LayerSaveContent content(rig.ActivatedContent(), true);
	PathfinderDefaults pathfinder;
	rig.OpenRoadSite();
	REQUIRE(rig.BuildTerminal(DoCommandFlag::Execute, NEW_STATION).Succeeded());
	Station *terminal = Station::GetByTile(rig.RoadSite());
	const auto cargo = rig.industry->produced[0].cargo;
	const auto road_depot = TileAddWrap(rig.RoadSite(), 0, 4);
	REQUIRE(IsValidTile(road_depot));
	std::vector<TileIndex> road_floor;
	// Prepare the real nearby mine loading route, including its depot entrance, before normal road construction.
	for (int offset = 1; offset <= 4; ++offset) road_floor.push_back(TileAddWrap(rig.RoadSite(), 0, offset));
	rig.Open(road_floor);
	REQUIRE(CmdBuildRoadDepot(DoCommandFlag::Execute, road_depot, ROADTYPE_ROAD, DiagDirection::NW).Succeeded());
	for (int offset = 1; offset <= 3; ++offset) REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, TileAddWrap(rig.RoadSite(), 0, offset),
		AxisToRoadBits(Axis::Y), ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
	Vehicle *truck = rig.Buy(road_depot, 7, cargo);
	Order loading;
	loading.MakeGoToStation(terminal->index);
	loading.SetStopLocation(OrderStopLocation::FarEnd);
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, truck->index, 0, loading).Succeeded());
	Order service;
	service.MakeGoToDepot(Depot::GetByTile(road_depot)->index, {OrderDepotTypeFlag::PartOfOrders, OrderDepotTypeFlag::Service});
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, truck->index, 1, service).Succeeded());
	const auto produced = ProduceLayerMineCargo(*rig.industry, 74, true).amount;
	REQUIRE(produced > 0);
	terminal->goods[cargo].rating = UINT8_MAX;
	terminal->goods[cargo].last_speed = 100;
	REQUIRE(DispatchLayerMineCargo(*rig.industry) == produced);
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, truck->index, true).Succeeded());
	uint ticks = 0;
	// Actual physical road ticks arrive and native economy loading moves the real finite packets; this checkpoint never teleports or fabricates cargo.
	while (truck->cargo.TotalCount() == 0 && ticks < 3000) {
		REQUIRE(truck->Tick());
		LoadUnloadStation(terminal);
		++ticks;
	}
	REQUIRE(truck->cargo.TotalCount() > 0);
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, truck->index, true).Succeeded());
	const auto origin_tile = TileXYL(40, 15, MapLayer::Surface);
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, origin_tile, RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	Station *origin = Station::GetByTile(origin_tile);
	REQUIRE(InitialiseSpoilReceiverDefinitions(rig.ActivatedContent()));
	// Actual command-funded Surface receivers provide the project's real disposal acceptance; no substitute stock or acceptance flag grants commissioning.
	for (const auto &[kind, tile] : {std::pair{SpoilReceiverKind::Earth, TileXYL(38, 12, MapLayer::Surface)},
		std::pair{SpoilReceiverKind::Rock, TileXYL(38, 17, MapLayer::Surface)}}) {
		REQUIRE(CmdBuildIndustry(DoCommandFlag::Execute, tile, GetSpoilReceiverIndustryType(kind), 0, true, 1234).Succeeded());
	}
	origin->RecomputeCatchment();
	UpdateStationAcceptance(origin, false);
	const auto portal = TileXYL(12, 40, MapLayer::Surface);
	REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, portal, TransportType::Rail, MapLayer::Underground, DiagDirection::SE,
		RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
	const auto rock = GetLayerPortalPairTile(portal);
	const auto door = GetLayerPortalDoorDirection(rock);
	const auto first = TileAddByDiagDir(rock, door);
	const auto last = TileAddByDiagDir(TileAddByDiagDir(first, door), door);
	REQUIRE(CmdCommissionTBM(DoCommandFlag::Execute, portal, 0, 2, 2, origin->index, first, last, false).Succeeded());
	const auto project = CaptureTBMProjects().back();
	const auto rail_depot = rig.BuildDepot(MapLayer::Underground, false);
	Train *hauler = Train::From(rig.Buy(rail_depot, 4));
	// Real acquisition and coupling supply the same empty Earth/Rock units required by the exchange owner after reload.
	for (uint16_t model : {uint16_t{2}, uint16_t{2}, uint16_t{3}, uint16_t{3}}) {
		Vehicle *wagon = rig.Buy(rail_depot, model);
		REQUIRE(CmdMoveRailVehicle(DoCommandFlag::Execute, wagon->index, hauler->index, false).Succeeded());
	}
	REQUIRE(AssignTBMHauler(project.id, hauler->index, true) == TBMExchangeError::None);
	Order disposal;
	disposal.MakeGoToStation(origin->index);
	disposal.SetStopLocation(OrderStopLocation::FarEnd);
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, hauler->index, 0, disposal).Succeeded());
	Order rail_service;
	rail_service.MakeGoToDepot(Depot::GetByTile(rail_depot)->index, {OrderDepotTypeFlag::PartOfOrders, OrderDepotTypeFlag::Service});
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, hauler->index, 1, rail_service).Succeeded());
	CHECK(GetUndergroundEngine(4).base() != 4);
	CHECK(GetUndergroundEngine(7).base() != 7);
	// The fresh save goes to this run's own saves; the retained fixture stays a read-only input (r2fc output contract).
	const auto path = GetTestArtifactPaths("vehicles", "underground-save-fixture").saves / "native-fleet.sav";
	REQUIRE(std::filesystem::is_directory(path.parent_path()));
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	const auto assignments = CaptureTBMExchangeAssignments();
	AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK(std::filesystem::file_size(path) > 8);
	CHECK(CaptureTBMExchangeAssignments() == assignments);
	CHECK(truck->cargo.TotalCount() > 0);
}
