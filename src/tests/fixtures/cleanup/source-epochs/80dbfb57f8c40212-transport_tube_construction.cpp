/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_construction.cpp Actual tube floor/depot/conversion/purchase/coupling command refusals.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Group2
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md task10.1
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "underground_view_fixture.h"
#include "transport_tube_fixture.h"
#include "../train_cmd.h"
#include "../tbm_cmd.h"
#include "../pbs.h"
#include "../rail_gui.h"
#include "../widgets/rail_widget.h"
#include "../toolbar_gui.h"

#include "../safeguards.h"

/** Direct synchronized track commands and pure quotes reject incompatible planes/solid ground without changing geology or tile type. */
TEST_CASE("Tube construction shares one rock layer and the complete excavation authority", "[transport-tube-construction]")
{
	TubePrototypeRig world;
	const auto layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground, MapLayer::Surface, MapLayer::Space);
	const auto tile = TileXYL(46, 44, layer);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto type = GetTileType(tile);
	CHECK(CheckTubeConstruction(std::array{tile}).Failed());
	CHECK(CmdBuildSingleRail({}, tile, world.tube, Track::X, false).Failed());
	CHECK(CmdBuildSingleRail(DoCommandFlag::Execute, tile, world.tube, Track::X, false).Failed());
	CHECK(GetTileType(tile) == type);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	// Only actual rock-layer fixture floors may be opened. Opening them is
	// explicit setup, never a normal tube command's excavation side effect.
	if (layer == MapLayer::Underground || layer == MapLayer::DeepUnderground) {
		world.Open(std::array{tile});
		CHECK(CheckTubeConstruction(std::array{tile}).Succeeded());
		CHECK(CmdBuildSingleRail(DoCommandFlag::Execute, tile, world.tube, Track::X, false).Succeeded());
	}
	CHECK(CheckTubeConstruction(std::array{world.depot, tile}).Succeeded() == (layer == MapLayer::Underground));
}

/** Stale company rail availability cannot bypass model introduction or the shared Deep calendar gate. */
TEST_CASE("Tube construction checks introduction and Deep dates despite forced availability", "[transport-tube-construction]")
{
	TubePrototypeRig world;
	const auto underground = TileXYL(46, 44, MapLayer::Underground);
	const auto deep = MultiLayerMap::TileInLayer(underground, MapLayer::DeepUnderground);
	world.Open(std::array{underground, deep});
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1969}, 11, 31);
	CHECK(CmdBuildSingleRail(DoCommandFlag::Execute, underground, world.tube, Track::X, false).Failed());
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1970}, 0, 1);
	CHECK(CmdBuildSingleRail(DoCommandFlag::Execute, underground, world.tube, Track::X, false).Succeeded());
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK(CmdBuildSingleRail(DoCommandFlag::Execute, deep, world.tube, Track::X, false).Failed());
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	CHECK(CmdBuildSingleRail(DoCommandFlag::Execute, deep, world.tube, Track::X, false).Succeeded());
}

/** A depot's one-tile floor grants no exemption for its still-solid approach or a wrapped/cross-layer footprint. */
TEST_CASE("Tube depot admission checks the actual complete door approach", "[transport-tube-construction]")
{
	TubePrototypeRig world;
	const auto tile = TileXYL(50, 44, MapLayer::Underground);
	const auto delta = TileIndexDiffCByDiagDir(DiagDirection::SE);
	const auto entrance = TileAddWrap(tile, delta.x, delta.y);
	world.Open(std::array{tile});
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(CheckTubeConstructionArea(tile, 1, 1, DiagDirection::SE).Failed());
	CHECK(CmdBuildTrainDepot(DoCommandFlag::Execute, tile, world.tube, DiagDirection::SE).Failed());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK_FALSE(IsRailDepotTile(tile));
	world.Open(std::array{entrance});
	CHECK(CmdBuildTrainDepot(DoCommandFlag::Execute, tile, world.tube, DiagDirection::SE).Succeeded());
	CHECK(CheckTubeConstructionArea(tile, 0, 1).Failed());
	CHECK(CheckTubeConstructionArea(tile, 1000, 1).Failed());
}

/** A late reserved cell refuses the complete ordinary conversion before its first otherwise-valid rail tile changes. */
TEST_CASE("Tube rail conversion preflights the entire selected footprint before mutation", "[transport-tube-conversion]")
{
	TubePrototypeRig world;
	const auto first = TileXYL(46, 44, MapLayer::Underground);
	const auto last = TileAddWrap(first, 1, 0);
	world.BuildTrack(first, RAILTYPE_RAIL);
	world.BuildTrack(last, RAILTYPE_RAIL);
	REQUIRE(TryReserveRailTrack(last, Track::X));
	CHECK(CmdConvertRail(DoCommandFlag::Execute, last, first, world.tube, false).Failed());
	CHECK(GetRailType(first) == RAILTYPE_RAIL);
	CHECK(GetRailType(last) == RAILTYPE_RAIL);
	CHECK(GetReservedTrackbits(last).Test(Track::X));
	UnreserveRailTrack(last, Track::X);
	CHECK(CmdConvertRail(DoCommandFlag::Execute, last, first, world.tube, false).Succeeded());
	CHECK(GetRailType(first) == world.tube);
	CHECK(GetRailType(last) == world.tube);
}

/** Neither endpoint of a real paired portal may acquire tube semantics, including a native rail source approached from Underground. */
TEST_CASE("Tube conversion refuses a complete portal pair without changing either endpoint", "[transport-tube-conversion]")
{
	TBMEngineFixture world;
	const auto tube = InitialiseTubeRailType(world.package);
	REQUIRE(tube != INVALID_RAILTYPE);
	Company::Get(_current_company)->avail_railtypes.Set(tube);
	const auto upper = world.request.portal;
	const auto lower = GetLayerPortalPairTile(upper);
	const auto state = ComputeLayerPortalWorkDigest();
	// Both construction directions target the same actual paired owner. A
	// refused conversion must not leave one end rewritten or reset its phase.
	for (const auto endpoint : {upper, lower}) {
		CHECK(CmdConvertRail(DoCommandFlag::Execute, endpoint, endpoint, tube, false).Failed());
		CHECK(GetRailType(upper) == RAILTYPE_RAIL);
		CHECK(GetRailType(lower) == RAILTYPE_RAIL);
		CHECK(ComputeLayerPortalWorkDigest() == state);
	}
	const auto candidate = TileXYL(30, 30, MapLayer::Surface);
	const auto geometry = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(CmdBuildLayerPortal(DoCommandFlag::Execute, candidate, TransportType::Rail, MapLayer::Underground,
		DiagDirection::SE, tube, ROADTYPE_ROAD).Failed());
	CHECK_FALSE(IsLayerPortalTile(candidate));
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geometry);
}

/** A real committed cut retains actual work-rail metadata until its separate support owner releases it. */
TEST_CASE("Tube conversion cannot bypass an unfinished or unreleased TBM support owner", "[transport-tube-conversion]")
{
	TBMWorkRig world;
	world.BoreOneCut();
	const auto tile = world.Job().route.front().cutter_position;
	REQUIRE(GetTBMSupportTile(tile) != nullptr);
	const auto tube = InitialiseTubeRailType(world.fixture.package);
	REQUIRE(tube != INVALID_RAILTYPE);
	Company::Get(_current_company)->avail_railtypes.Set(tube);
	const auto original = GetRailType(tile);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto material = CaptureTBMMaterial(world.project);
	CHECK(CmdConvertRail(DoCommandFlag::Execute, tile, tile, tube, false).Failed());
	CHECK(CmdConvertRail({DoCommandFlag::Execute, DoCommandFlag::Bankrupt}, tile, tile, tube, false).Failed());
	CHECK(GetRailType(tile) == original);
	CHECK(GetTBMSupportTile(tile) != nullptr);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	const auto after = CaptureTBMMaterial(world.project);
	CHECK(after.revision == material.revision);
	CHECK(after.earth_space_t == material.earth_space_t);
	CHECK(after.rock_space_t == material.rock_space_t);
	CHECK(after.support_units == material.support_units);
	CHECK(after.available_funds == material.available_funds);
}

/** Actual native station construction and the public utility-floor gate inspect every platform/approach/cable cell. */
TEST_CASE("Tube terminals and utility footprints require every actual floor and approach", "[transport-tube-construction]")
{
	LoadGeneratedWindowTestLanguage();
	TubePrototypeRig world;
	const auto station = TileXYL(50, 44, MapLayer::Underground);
	world.Open(std::array{station, TileAddWrap(station, 1, 0)});
	CHECK(CmdBuildRailStation(DoCommandFlag::Execute, station, world.tube, Axis::X, 1, 2,
		STAT_CLASS_DFLT, 0, NEW_STATION, false).Failed());
	CHECK(Station::GetNumItems() == 0);
	world.Open(std::array{TileAddWrap(station, -1, 0), TileAddWrap(station, 2, 0)});
	CHECK(CmdBuildRailStation(DoCommandFlag::Execute, station, world.tube, Axis::X, 1, 2,
		STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	CHECK(Station::GetNumItems() == 1);
	const auto first = TileXYL(50, 48, MapLayer::Underground);
	const auto second = TileAddWrap(first, 1, 0);
	world.Open(std::array{first});
	CHECK(CheckTubeConstruction(std::array{first, second}).Failed());
	world.Open(std::array{second});
	CHECK(CheckTubeConstruction(std::array{first, second}).Succeeded());
}

/** The actual shared rail toolbar forwards the clicked world tile to the same tube foundation guard as direct/script commands. */
TEST_CASE("Actual rail toolbar tube depot placement preserves the geological command boundary", "[transport-tube-construction-gui]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TubePrototypeRig world;
	UndergroundToolbarTestUI ui(_current_company);
	new UndergroundTestViewport();
	AllocateToolbar();
	AutoRestoreBackup pause_build(_settings_game.construction.command_pause_level, CommandPauseLevel::AllActions);
	Window *toolbar = ShowBuildRailToolbar(world.tube);
	REQUIRE(toolbar != nullptr);
	toolbar->OnPaint();
	const auto *widget = toolbar->GetWidget<NWidgetBase>(WID_RAT_BUILD_DEPOT);
	toolbar->OnClick({static_cast<int>(widget->pos_x + widget->current_x / 2),
		static_cast<int>(widget->pos_y + widget->current_y / 2)}, WID_RAT_BUILD_DEPOT, 1);
	const auto layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground, MapLayer::Surface, MapLayer::Space);
	const auto tile = TileXYL(50, 50, layer);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	toolbar->OnPlaceObject({}, tile);
	CHECK_FALSE(IsRailDepotTile(tile));
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	// Only supported rock layers receive explicit fixture-open floor. The same
	// GUI callback must reject Surface/Space independently of camera selection.
	if (layer == MapLayer::Underground || layer == MapLayer::DeepUnderground) {
		world.Open(std::array{tile, TileAddWrap(tile, -1, 0), TileAddWrap(tile, 1, 0),
			TileAddWrap(tile, 0, -1), TileAddWrap(tile, 0, 1)});
		if (layer == MapLayer::DeepUnderground) {
			TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
			toolbar->OnPlaceObject({}, tile);
			CHECK_FALSE(IsRailDepotTile(tile));
			TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
		}
		toolbar->OnPlaceObject({}, tile);
		CHECK(IsRailDepotTile(tile));
		CHECK(GetRailType(tile) == world.tube);
	}
	CloseWindowByClass(WindowClass::BuildDepot);
	CloseWindowByClass(WindowClass::ErrorMessage);
	Window::DeleteClosedWindows();
	ShowBuildRailToolbar(RAILTYPE_RAIL); // Restore the shared construction tool's ordinary type before this world ends.
	ResetObjectToPlace();
	CloseWindowByClass(WindowClass::BuildToolbar);
	Window::DeleteClosedWindows();
}

/** Native engine/cargo identity refuses conventional purchase and refit despite broad mutable mechanical/refit masks. */
TEST_CASE("Tube purchase and refit preserve the native movement and cargo domain", "[transport-tube-purchase]")
{
	TubePrototypeRig world;
	const auto ordinary = world.BuildDepot(MapLayer::Underground, false);
	const auto vehicles = Vehicle::GetNumItems();
	CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, ordinary, GetTubeEngine(9), false, INVALID_CARGO, ClientID::Invalid)).Failed());
	CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, world.depot, GetUndergroundEngine(4), false, INVALID_CARGO, ClientID::Invalid)).Failed());
	CHECK(Vehicle::GetNumItems() == vehicles);
	Train *capsule = world.BuyCapsule();
	Engine::Get(capsule->engine_type)->info.refit_mask.Set(GetCargoTypeByLabel(CargoLabel{"PASS"}));
	const auto original = capsule->cargo_type;
	CHECK(std::get<0>(CmdRefitVehicle(DoCommandFlag::Execute, capsule->index, GetCargoTypeByLabel(CargoLabel{"PASS"}), 0, false, false, 0)).Failed());
	CHECK(capsule->cargo_type == original);
	CHECK(Vehicle::GetNumItems() == vehicles + 1);
	CHECK(CmdConvertRail(DoCommandFlag::Execute, world.depot, world.depot, RAILTYPE_RAIL, false).Failed());
	CHECK(GetRailType(world.depot) == world.tube);
}

/** A deliberately stale depot placement cannot let actual native/ordinary vehicles couple; no unit or cargo is fabricated. */
TEST_CASE("Tube coupling refuses mixed native domains before changing either real chain", "[transport-tube-purchase]")
{
	TubePrototypeRig world;
	Train *capsule = world.BuyCapsule();
	Train *ordinary = Train::From(world.Buy(world.BuildDepot(MapLayer::Underground, false), 4));
	// This bounded invalid-state fixture parks an existing command-bought unit
	// at the other's depot. It tests refusal and cannot claim a legal journey.
	ordinary->tile = capsule->tile;
	CHECK(CmdMoveRailVehicle(DoCommandFlag::Execute, ordinary->index, capsule->index, true).Failed());
	CHECK(CmdMoveRailVehicle(DoCommandFlag::Execute, capsule->index, ordinary->index, true).Failed());
	CHECK(capsule->First() == capsule);
	CHECK(ordinary->First() == ordinary);
	CHECK(capsule->Next() == nullptr);
	CHECK(ordinary->Next() == nullptr);
}
