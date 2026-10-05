/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_reuse.cpp Reusing cleared ground after support (Group 15, shared removal/replacement and existing road and
 * rail cases): support is never replaced in place, removed support leaves open ground for ordinary roads or rails,
 * and converted support is ordinary track. Tube and plant construction on cleared ground waits for master Group 10.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 15
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../tbm_cmd.h"
#include "../rail_cmd.h"
#include "../road.h"
#include "../road_cmd.h"
#include "../road_map.h"
#include "../command_func.h"

#include "../table/strings.h"

#include "../safeguards.h"

/** Support stays rail until it is removed or converted; afterwards the open floor takes ordinary roads and rails. */
TEST_CASE("Cleared support ground becomes ordinary road or rail after release", "[tbm-reuse]")
{
	TBMWorkRig rig;
	rig.BoreOneCut();
	ResetRoadTypes();
	const TileIndex cell = rig.Job().route[0].cutter_position;
	const Track laid = *GetTrackBits(cell).begin();
	const RoadBits across = laid == Track::X ? ROAD_Y : ROAD_X; // A road across the support would make a level crossing.

	/**
	 * Try a road across the cell.
	 * @return The command result.
	 */
	const auto build_road = [&]() { return CmdBuildRoad(DoCommandFlag::Execute, cell, across, ROADTYPE_ROAD, {}, TownID::Invalid()); };
	// In use, the support is never turned into a crossing in place.
	CHECK(build_road().GetErrorMessage() == STR_ERROR_TBM_SUPPORT_CONVERT_FIRST);
	CHECK(IsPlainRailTile(cell));

	SECTION("released but unconverted support still refuses replacement in place") {
		rig.Reload(TBMPhase::Completed);
		CHECK(build_road().GetErrorMessage() == STR_ERROR_TBM_SUPPORT_CONVERT_FIRST);
		CHECK(GetTBMSupportTile(cell) != nullptr);
	}
	SECTION("removed support leaves open ground that takes an ordinary road") {
		rig.Reload(TBMPhase::Completed);
		REQUIRE(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, cell, laid).Succeeded());
		CHECK(IsTileType(cell, TileType::Clear));
		CHECK(GetGeologyWorldState().terrain.Get(cell) == ExcavationState::Excavated);
		CHECK(CmdBuildRoad(DoCommandFlag::Execute, cell, ROAD_X, ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
		CHECK(IsTileType(cell, TileType::Road));
	}
	SECTION("removed support leaves open ground that takes ordinary rail") {
		rig.Reload(TBMPhase::Completed);
		REQUIRE(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, cell, laid).Succeeded());
		CHECK(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, cell, GetRailType(rig.proof.destination), laid, false).Succeeded());
		CHECK(GetTBMSupportTile(cell) == nullptr); // Ordinary track carries no support restriction.
	}
	SECTION("converted support is ordinary track that may become a crossing") {
		rig.Reload(TBMPhase::Completed);
		REQUIRE(Command<Commands::ConvertTBMSupport>::Do(DoCommandFlag::Execute, cell).Succeeded());
		CHECK(build_road().Succeeded());
		CHECK(IsLevelCrossingTile(cell));
	}
}
