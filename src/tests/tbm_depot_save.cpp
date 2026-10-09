/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_depot_save.cpp Actual paid recurring-depot snapshots for production loader continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13C
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 7BB
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_depot_fixture.h"
#include "../saveload/saveload_func.h"
#include "../table/sprites.h"
#include <cstdlib>

#include "../safeguards.h"

/** Persist only naturally reached recurring-depot waits, including actual parts spanning the separate Connected pair. */
TEST_CASE("Recurring depot logistics writes real stopped exchange disposal return and spanning files", "[tbm-depot-save-file]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const TBMProjectID project = workflow.StartFurther();
	const char *prefix = std::getenv("OTTD_TBM_LOGISTICS_RECORD");
	if (prefix != nullptr) {
		REQUIRE(std::filesystem::path(prefix).filename() == std::filesystem::path(prefix));
		REQUIRE_FALSE(std::string_view(prefix).starts_with("r2"));
	}
	const auto directory = GetTestCaseArtifactPaths("tbm", "logistics").saves;
	REQUIRE(std::filesystem::is_directory(directory));
	std::set<std::string> written;
	/** Observe only actual command/tick outcomes; the writer never changes a physical train, packet, project or assignment. */
	const auto observe = [&]() {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index);
		REQUIRE(assignment != nullptr);
		CHECK(assignment->bay == workflow.depot);
		CHECK(GetTBMExchangeBay(*GetTBMProject(project)).kind == TBMExchangeBayKind::Depot);
		std::vector<std::string> states;
		if (assignment->wait == TBMHaulerWait::Stopped) states.emplace_back("stopped");
		if (assignment->phase == TBMHaulerPhase::AwaitExchange) states.emplace_back("await-exchange");
		if (assignment->phase == TBMHaulerPhase::DisposeOrTransfer) states.emplace_back("dispose-origin");
		// Only a return following a genuine completed depot delivery establishes the saved empty-return obligation.
		if (assignment->phase == TBMHaulerPhase::ReturnEmpties && assignment->exchanges != 0) states.emplace_back("return-empties");
		bool surface = false, underground = false;
		// Both halves must belong to this actual whole hauler crossing the independent native Connected pair.
		for (const Train *part = workflow.world.hauler; part != nullptr; part = part->Next()) {
			surface |= GetTileLayer(part->tile) == MapLayer::Surface;
			underground |= GetTileLayer(part->tile) == MapLayer::Underground;
		}
		if (surface && underground) states.emplace_back("spanning"); // Layer coincidence of unrelated trains cannot establish a spanning consist.
		// First occurrences are immutable inputs. Later physical cycles cannot overwrite their original packets or cost buckets.
		for (const auto &state : states) {
			if (!written.insert(state).second) continue;
			if (prefix == nullptr) continue;
			const auto path = directory / (std::string(prefix) + "-" + state + ".sav");
			REQUIRE_FALSE(std::filesystem::exists(path));
			const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			const auto projects = ComputeTBMProjectDigest();
			const auto exchanges = ComputeTBMExchangeDigest();
			const Money cash = Company::Get(_current_company)->money;
			SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
			AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			CHECK(ComputeTBMProjectDigest() == projects);
			CHECK(ComputeTBMExchangeDigest() == exchanges);
			CHECK(Company::Get(_current_company)->money == cash);
		}
	};
	workflow.Do<Commands::StartStopVehicle>(workflow.world.hauler->index, true);
	workflow.world.Tick();
	observe();
	workflow.Do<Commands::StartStopVehicle>(workflow.world.hauler->index, true);
	/** @return Whether the native second depot exchange followed real disposal and physical empty return. */
	const auto repeated = [&]() { return GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->exchanges >= 2; };
	workflow.DriveUntil(repeated, 50000, observe);
	CHECK(written == std::set<std::string>{"stopped", "await-exchange", "dispose-origin", "return-empties", "spanning"});
}
