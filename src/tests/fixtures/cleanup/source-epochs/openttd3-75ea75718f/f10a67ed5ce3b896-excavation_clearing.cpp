/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_clearing.cpp Direct/API/NoTest clearing and explicit editor override boundaries. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 9 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../landscape_cmd.h"
#include "../terraform_cmd.h"
#include "../command_func.h"
#include "../tile_cmd.h"
#include "../clear_map.h"
#include "../multilayer_state.h"
#include "../table/strings.h"
#include "../rail_cmd.h"
#include "../road_cmd.h"
#include "../map_layer_gui.h"

#include "../safeguards.h"

/** Bulldozing and auto-clearing cannot consume money, limits or RNG to turn solid rock into usable ground. */
TEST_CASE("Solid terrain refuses direct and automatic clearing unchanged", "[excavation-clearing]")
{
	ExcavationFixture fixture;
	const auto tile = TileXYL(20, 20, MapLayer::Underground);
	const auto before = CaptureGeologyState();
	const auto limit = Company::Get(_current_company)->clear_limit;
	const auto money = Company::Get(_current_company)->money;
	const auto metadata = Tile{tile}.m5();
	SavedRandomSeeds random_before;
	SaveRandomSeeds(&random_before);
	// Exercise estimate, direct execute and script/nested command boundaries without depending on a toolbar.
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::Auto}}) {
		CHECK(CmdLandscapeClear(flags, tile).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
		CHECK(_tile_type_procs[TileType::Clear]->clear_tile_proc(tile, flags).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
		CHECK(Command<Commands::LandscapeClear>::Do(flags, tile).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	}
	CHECK(std::get<0>(CmdTerraformLand(DoCommandFlag::Execute, tile, SLOPE_N, true)).Failed());
	CHECK(std::get<0>(CmdLevelLand(DoCommandFlag::Execute, tile, TileXYL(21, 20, MapLayer::Underground), false, LevelMode::Raise)).Failed());
	CHECK(CmdConvertRail(DoCommandFlag::Execute, tile, tile, RAILTYPE_RAIL, false).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	CHECK(CmdConvertRoad(DoCommandFlag::Execute, tile, tile, ROADTYPE_ROAD, false).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	// Extra-view world Y resolves the same explicit layer; the construction guard never consults a selected viewport.
	CHECK(GetLayerOfWorldY(TileY(tile) * TILE_SIZE + TILE_SIZE / 2) == MapLayer::Underground);
	CHECK(CaptureGeologyState().packed == before.packed);
	CHECK(Tile{tile}.m5() == metadata);
	CHECK(Company::Get(_current_company)->clear_limit == limit);
	CHECK(Company::Get(_current_company)->money == money);
	SavedRandomSeeds random_after;
	SaveRandomSeeds(&random_after);
	CHECK(random_before.random.state[0] == random_after.random.state[0]);
	CHECK(random_before.random.state[1] == random_after.random.state[1]);
}

/** Area clear keeps historical partial success while refusing the solid member in both estimate and execution. */
TEST_CASE("NoTest area clearing preserves cavities and refuses its solid members", "[excavation-area-clearing]")
{
	ExcavationFixture fixture;
	const auto open = TileXYL(20, 20, MapLayer::Underground);
	const auto solid = TileXYL(21, 20, MapLayer::Underground);
	fixture.Open(std::array{open});
	const auto before = CaptureGeologyState();
	STATIC_REQUIRE(CommandTraits<Commands::ClearArea>::flags.Test(CommandFlag::NoTest));
	CHECK(std::get<0>(CmdClearArea({}, solid, open, false)).Succeeded());
	CHECK(CaptureGeologyState().packed == before.packed);
	CHECK(std::get<0>(CmdClearArea({DoCommandFlag::Execute, DoCommandFlag::Auto}, solid, open, false)).Succeeded());
	CHECK(CaptureGeologyState().packed == before.packed);
	CHECK(GetGeologyWorldState().terrain.Get(open) == ExcavationState::Excavated);
	CHECK(GetGeologyWorldState().terrain.Get(solid) == ExcavationState::Solid);
	CHECK(IsClearGround(open, ClearGround::Rocks));
	CHECK(std::get<0>(CmdClearArea(DoCommandFlag::Execute, solid, solid, false)).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
}

/** Saved editor permission is necessary but insufficient outside actual scenario editing. */
TEST_CASE("Explicit editor painting never becomes normal-game free excavation", "[excavation-editor]")
{
	ExcavationFixture fixture;
	const auto first = TileXYL(20, 20, MapLayer::DeepUnderground);
	const auto last = TileXYL(21, 20, MapLayer::DeepUnderground);
	const auto before = CaptureGeologyState();
	CHECK(CmdPaintExcavationTerrain(DoCommandFlag::Execute, first, last, true).Failed());
	_game_mode = GameMode::Editor;
	CHECK(CmdPaintExcavationTerrain(DoCommandFlag::Execute, first, last, true).Failed());
	fixture.SetEditorOverride(true);
	CHECK(CaptureMultiLayerStateSchema().profile.editor_override);
	CHECK(CmdLandscapeClear(DoCommandFlag::Execute, first).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	CHECK(CmdPaintExcavationTerrain({}, first, last, true).Succeeded());
	CHECK(CaptureGeologyState().packed == before.packed);
	CHECK(CmdPaintExcavationTerrain(DoCommandFlag::Execute, first, last, true).Succeeded());
	CHECK(GetGeologyWorldState().terrain.Get(first) == ExcavationState::Excavated);
	CHECK(GetGeologyWorldState().terrain.Get(last) == ExcavationState::Excavated);
	_game_mode = GameMode::Normal;
	CHECK(CmdPaintExcavationTerrain(DoCommandFlag::Execute, first, last, false).Failed());
	CHECK(GetGeologyWorldState().terrain.Get(first) == ExcavationState::Excavated);
	_game_mode = GameMode::Editor;
	CHECK(CmdPaintExcavationTerrain(DoCommandFlag::Execute, first, TileXYL(21, 20, MapLayer::Surface), false).Failed());
	CHECK(CmdPaintExcavationTerrain(DoCommandFlag::Execute, first, last, false).Succeeded());
	CHECK(CaptureGeologyState().packed == before.packed);
	CHECK(CaptureGeologyState().deposits.front().remaining_reserve == before.deposits.front().remaining_reserve);
}
