/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_planning_seed.cpp Uncommissioned native portal scenarios for real planning input. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group3C; docs/zz-reach2/architecture/savegame/archi-savegame.md */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "layer_save_fixture.h"
#include "../company_cmd.h"
#include "../saveload/saveload_func.h"
#include "../table/sprites.h"
#include <cstdlib>
#include "../safeguards.h"

/** Write a funded native logistics world whose target rock and uncommissioned portal remain untouched by any TBM. */
TEST_CASE("Fresh TBM planning writes a real uncommissioned rock-layer scenario", "[.tbm-planning-seed]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const char *selected = std::getenv("OTTD_TBM_FEEDBACK_FRESH");
	REQUIRE(selected != nullptr);
	const std::string_view name{selected};
	REQUIRE((name == "underground" || name == "deep"));
	const MapLayer layer = name == "deep" ? MapLayer::DeepUnderground : MapLayer::Underground;
	UndergroundWorkflowWorld world(layer, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	// Area coverage in this declared scenario permits ordinary sharp rail
	// pieces; the cutter's own advancing bends remain limited to45 degrees.
	AutoRestoreBackup turns(_settings_game.pf.forbid_90_deg, false);
	AutoRestoreBackup player(_local_company, _current_company);
	// This unstarted scenario has no video driver for CmdPause's UI hook.
	// Declare its initial paused state before writing; no simulation ran here.
	AutoRestoreBackup initial_pause(_pause_mode, PauseModes{PauseMode::Normal});
	REQUIRE(Command<Commands::RenameCompany>::Do(DoCommandFlag::Execute, "Underground Mines").Succeeded());
	REQUIRE(CaptureTBMProjects().empty());
	REQUIRE(GetGeologyWorldState().terrain.Get(world.rock) == ExcavationState::Solid);
	const auto path = GetTestCaseArtifactPaths("tbm", "planning").saves / fmt::format("tbm-planning-{}-393.sav", name);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto money = Company::Get(_current_company)->money;
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE); // The real save writer temporarily displays its busy cursor, as in existing native save fixtures.
	AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == money);
	fmt::print("PLANNING_SEED layer={} portal={} rock={} origin={} hauler={} setup={} saved={}\n", name,
		world.portal.base(), world.rock.base(), world.origin->index.base(), world.hauler->index.base(), world.setup_cost.base(), path.string());
}
