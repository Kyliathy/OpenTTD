/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file acceptance_ground_fixture.cpp Declared isolated connected three-plane fixture, saved through real native descriptors.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group2
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint11.1
 * @see docs/zz-reach2/architecture/archi-historical-debug.md immutable inputs and run-owned saves.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "test_artifact_paths.h"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "layer_save_fixture.h"
#include "../saveload/saveload_func.h"
#include "../gfx_func.h"
#include "../table/sprites.h"

#include "../safeguards.h"

/** A clearly seeded regression fixture uses actual owned construction/connection commands without claiming paid rock excavation or bootstrap acceptance. */
TEST_CASE("Acceptance fixture connects Surface Underground and Deep through actual owned portal commands", "[acceptance-ground-fixture]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world(MapLayer::DeepUnderground, true, true);
	std::vector<TileIndex> floor;
	// This is deliberate fixture-authoring, before acceptance play. The
	// metadata must disclose these open cells; they are never claimed as
	// normally excavated predecessors or credited to a boring project.
	for (TileIndex end : {world.portal, world.rock}) {
		const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(end));
		const TileIndex door = TileAddWrap(end, step.x, step.y);
		floor.push_back(end);
		floor.push_back(door);
	}
	world.Open(floor);
	const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(world.rock));
	const TileIndex door = TileAddWrap(world.rock, step.x, step.y);
	REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, door, RAILTYPE_RAIL,
		DiagDirToDiagTrack(GetLayerPortalShaftDirection(world.rock)), false).Succeeded());
	REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, world.portal, true).Succeeded());
	CHECK(GetLayerPortalWorkState(world.portal) == LayerPortalWorkState::Connected);
	const auto pairs = CaptureLayerPortalWorkRecords();
	REQUIRE(pairs.size() == 2);
	// Both actual pairs form one Surface/Underground/Deep access chain. A
	// coincident tile or a painted line is not accepted as connectivity.
	for (const auto &pair : pairs) CHECK(pair.state == LayerPortalWorkState::Connected);
	CHECK(CaptureTBMProjects().empty());
	// A newly authored connected world belongs to this invocation. The original
	// version384 source fixture is an immutable compatibility input and cannot be
	// replaced merely because the current writer generated another world successfully.
	const auto output = GetTestCaseArtifactPaths("acceptance", "ground-connected").Output("saves", "ground-connected.sav");
	LayerSaveContent content(world.ActivatedContent(), true);
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	AutoRestoreBackup no_ui(_game_mode, GameMode::Menu); // Save exact actual owners without creating progress UI in the isolated fixture.
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(output), false) == SaveLoadResult::Ok);
	fmt::print("ACCEPTANCE_CONNECTED isolated=true fixture_opened_floor=true actual_pairs=2 saved={}\n", output.string());
}
