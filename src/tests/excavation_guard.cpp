/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_guard.cpp Complete-floor, approach, policy and commissioning refusal fixtures. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 8 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../command_func.h"
#include "../rail_cmd.h"
#include "../road_cmd.h"
#include "../station_cmd.h"
#include "../layer_portal_cmd.h"
#include "../newgrf_station.h"
#include "../newgrf_roadstop.h"
#include "../signal_type.h"
#include "../table/strings.h"

#include "../safeguards.h"

/** Check pure full-footprint geometry, independent of tile type, camera, inventory and random streams. */
TEST_CASE("Excavation footprint checks every tile and canonical Deep date", "[excavation-footprint]")
{
	ExcavationFixture fixture;
	const auto open = TileXYL(20, 20, MapLayer::Underground);
	const auto solid = TileXYL(21, 20, MapLayer::Underground);
	fixture.Open(std::array{open});
	SavedRandomSeeds random_before;
	SaveRandomSeeds(&random_before);
	CHECK(CheckExcavatedFootprint(std::array{open}).error == ExcavationError::None);
	const auto refusal = CheckExcavatedFootprint(std::array{open, solid});
	CHECK(refusal.error == ExcavationError::SolidRock);
	CHECK(refusal.failure_tile == solid);
	CHECK(CheckExcavatedFootprint(std::array{open, TileXYL(20, 20, MapLayer::Surface)}).error == ExcavationError::MixedLayers);
	CHECK(CheckExcavatedFootprint(std::array{TileXYL(63, 20, MapLayer::Underground)}).error == ExcavationError::InvalidTile);
	CHECK(CheckExcavatedFootprint(std::span<const TileIndex>{}).error == ExcavationError::InvalidTile);
	const auto deep = TileXYL(20, 20, MapLayer::DeepUnderground);
	fixture.Open(std::array{deep});
	const auto pre1980 = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	auto policy = GetCurrentMultiLayerProfileSelection();
	CHECK(CheckExcavatedFootprint(GetGeologyWorldState(), policy, pre1980, std::array{deep}).error == ExcavationError::LockedDeep);
	policy.editor_override = true;
	CHECK(CheckExcavatedFootprint(GetGeologyWorldState(), policy, pre1980, std::array{deep}).error == ExcavationError::None);
	CHECK(CheckExcavatedFootprint(GetGeologyWorldState(), {}, pre1980, std::array{solid}).error == ExcavationError::None);
	CHECK(CheckExcavatedFootprint(GeologyWorldState{}, GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date, std::array{solid}).error == ExcavationError::None);
	policy.version = 2;
	CHECK(CheckExcavatedFootprint(GetGeologyWorldState(), policy, pre1980, std::array{deep}).error == ExcavationError::InvalidPolicy);
	auto prepared = PrepareGeologyGeneration(123);
	REQUIRE(prepared.error == GeologyError::None);
	REQUIRE(prepared.state.terrain.Set(solid, ExcavationState::Excavating));
	CHECK(CheckExcavatedFootprint(prepared.state, GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date, std::array{solid}).error == ExcavationError::ActiveCut);
	CHECK(GetGeologyWorldState().terrain.Get(open) == ExcavationState::Excavated);
	CHECK(GetGeologyWorldState().terrain.Get(solid) == ExcavationState::Solid);
	SavedRandomSeeds random_after;
	SaveRandomSeeds(&random_after);
	CHECK(random_before.random.state[0] == random_after.random.state[0]);
	CHECK(random_before.random.state[1] == random_after.random.state[1]);
	CHECK(random_before.interactive_random.state[0] == random_after.interactive_random.state[0]);
	CHECK(random_before.interactive_random.state[1] == random_after.interactive_random.state[1]);
}

/** Exercise live command entry points with mixed building/approach ground before any execution mutation. */
TEST_CASE("Construction families cannot use a cleared first cell to enter solid rock", "[excavation-construction]")
{
	ExcavationFixture fixture;
	const auto open = TileXYL(20, 20, MapLayer::Underground);
	const auto solid = TileXYL(21, 20, MapLayer::Underground);
	fixture.Open(std::array{open});
	const auto before = CaptureGeologyState();
	const auto tile_before = Tile{open}.m5();
	// Shared construction authority covers track/road/signal/depot/station families even through API calls.
	for (auto family : {MapLayerConstruction::Rail, MapLayerConstruction::Road, MapLayerConstruction::Signal, MapLayerConstruction::TrainDepot, MapLayerConstruction::RoadDepot, MapLayerConstruction::RailStation, MapLayerConstruction::RoadStop}) {
		CHECK(CheckLayerAllowsConstruction(solid, family).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
		CHECK(CheckLayerAllowsConstruction(open, family).Succeeded());
	}
	CHECK(CmdBuildSingleRail(DoCommandFlag::Execute, solid, RAILTYPE_RAIL, Track::X, false).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	CHECK(CmdBuildRoad(DoCommandFlag::Execute, solid, ROAD_X, ROADTYPE_ROAD, {}, TownID::Invalid()).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	CHECK(CmdBuildSingleSignal(DoCommandFlag::Execute, solid, Track::X, SignalType::Block, SignalVariant::Electric, false, false, false, SignalType::Block, SignalType::Block, 0, 0).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	CHECK(CmdBuildTrainDepot(DoCommandFlag::Execute, open, RAILTYPE_RAIL, DiagDirection::SW).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	CHECK(CmdBuildRoadDepot(DoCommandFlag::Execute, open, ROADTYPE_ROAD, DiagDirection::SW).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	StationClass::Reset();
	RoadStopClass::Reset();
	CHECK(CmdBuildRailStation(DoCommandFlag::Execute, open, RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	CHECK(CmdBuildRoadStop(DoCommandFlag::Execute, open, 2, 1, RoadStopType::Bus, false, DiagDirection::SW, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	CHECK(CmdBuildLayerPortal(DoCommandFlag::Execute, TileXYL(20, 20, MapLayer::Surface), TransportType::Rail, MapLayer::Underground, DiagDirection::NE, RAILTYPE_RAIL, ROADTYPE_ROAD).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	CHECK(CheckExcavationConstructionArea(open, 2, 1).Failed());
	CHECK(CheckExcavationConstructionArea(TileXYL(62, 20, MapLayer::Underground), 3, 1).Failed());
	CHECK(CaptureGeologyState().packed == before.packed);
	CHECK(Tile{open}.m5() == tile_before);
	CHECK(GetTileType(open) == TileType::Clear);
	CHECK(GetTileType(solid) == TileType::Clear);
}

/** Restricted portal occupancy cannot be supplied to ordinary construction or shared with another vehicle. */
TEST_CASE("Commissioning pocket is assigned occupancy and never cleared terrain", "[excavation-pocket]")
{
	ExcavationFixture fixture;
	const auto anchor = TileXYL(20, 20, MapLayer::Underground);
	ExcavationCommissioningPocket pocket{VehicleID{4}, anchor, {anchor, TileXYL(21, 20, MapLayer::Underground)}};
	CHECK(CheckExcavationCommissioningPocket(pocket, VehicleID{4}, anchor));
	CHECK_FALSE(CheckExcavationCommissioningPocket(pocket, VehicleID{5}, anchor));
	CHECK_FALSE(CheckExcavationCommissioningPocket(pocket, VehicleID::Invalid(), anchor));
	CHECK(CheckExcavatedFootprint(std::array{anchor}).error == ExcavationError::SolidRock);
	pocket.tiles.push_back(TileXYL(23, 20, MapLayer::Underground));
	CHECK_FALSE(CheckExcavationCommissioningPocket(pocket, VehicleID{4}, anchor));
}
