/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_logistics_save.cpp Actual driven source logistics snapshots for the real game loader.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13A
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 7BB
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "underground_workflow_fixture.h"
#include "../saveload/saveload_func.h"
#include "../table/sprites.h"
#include "../core/backup_type.hpp"
#include <cstdlib>

#include "../safeguards.h"

/** Write real native waits, cross-layer hauling or partial bends only after actual commands and physical movement reach them. */
TEST_CASE("Driven source logistics writes native waits and geometry snapshots", "[tbm-logistics-save-file]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const bool wide = std::getenv("OTTD_TBM_LOGISTICS_WIDE") != nullptr;
	UndergroundWorkflowWorld world(wide ? MapLayer::Underground : MapLayer::DeepUnderground, false, true);
	AutoRestoreBackup<bool> turns(_settings_game.pf.forbid_90_deg, !wide);
	LayerSaveContent content(world.ActivatedContent(), true);
	const char *record_prefix = std::getenv("OTTD_TBM_LOGISTICS_RECORD");
	// Recording is opt-in and uses a fresh basename; normal verification never overwrites retained real-game inputs.
	if (record_prefix != nullptr) {
		REQUIRE(std::filesystem::path(record_prefix).filename() == std::filesystem::path(record_prefix));
		REQUIRE_FALSE(std::string_view(record_prefix).starts_with("r2"));
	}
	const auto directory = std::filesystem::current_path().parent_path() / "src/tests/fixtures/haulage";
	REQUIRE(std::filesystem::is_directory(directory.parent_path()));
	std::filesystem::create_directories(directory);
	std::set<std::string> written;
	Direction first_heading = Direction::Invalid;
	/** @param current Actual stepped world; the observer writes evidence and never changes a vehicle, packet or phase. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(assignment != nullptr);
		const auto *project = GetTBMProject(assignment->project);
		REQUIRE(project != nullptr);
		const auto *job = GetExcavationJob(project->job);
		REQUIRE(job != nullptr);
		if (first_heading == Direction::Invalid) first_heading = project->heading;
		std::vector<std::string> states;
		// This extra snapshot requires actual advanced bend geometry and
		// unfinished work while the real cutter stands there. A planned turn
		// or staging-only controller copy cannot establish serialized bend proof.
		if (wide && project->phase == TBMPhase::Bore && project->heading != first_heading && job->partial_work_ticks != 0) states.emplace_back("bend-partial");
		if (assignment->wait == TBMHaulerWait::Stopped) states.emplace_back("stopped");
		if (assignment->phase == TBMHaulerPhase::AwaitExchange) states.emplace_back("await-exchange");
		if (assignment->phase == TBMHaulerPhase::DisposeOrTransfer) states.emplace_back("dispose-origin");
		// Empty return evidence follows a real completed delivery; the identically named initial empty-start phase cannot establish it.
		if (assignment->phase == TBMHaulerPhase::ReturnEmpties && assignment->exchanges != 0) states.emplace_back("return-empties");
		bool surface = false, underground = false;
		// A real whole chain crossing the ordinary access pair supplies the saved spanning case; no part is repositioned.
		for (const Train *part = current.hauler; part != nullptr; part = part->Next()) {
			surface |= GetTileLayer(part->tile) == MapLayer::Surface;
			underground |= GetTileLayer(part->tile) == MapLayer::Underground;
		}
		// The same hauler must physically occupy both layers, rather than an unrelated train satisfying each half.
		if (surface && underground) states.emplace_back("spanning");
		// Persist the first actual occurrence of each selected state so later steps cannot overwrite its immutable input.
		for (const auto &state : states) {
			if (written.contains(state)) continue;
			written.insert(state);
			if (record_prefix == nullptr) continue;
			const auto path = directory / (std::string(record_prefix) + "-" + state + ".sav");
			REQUIRE_FALSE(std::filesystem::exists(path));
			SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
			const auto projects = ComputeTBMProjectDigest();
			const auto assignments = CaptureTBMExchangeAssignments();
			const auto packets = CargoPacket::GetNumItems();
			const auto money = Company::Get(_current_company)->money;
			AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
			REQUIRE(std::filesystem::file_size(path) > 8);
			CHECK(ComputeTBMProjectDigest() == projects);
			CHECK(CaptureTBMExchangeAssignments() == assignments);
			CHECK(CargoPacket::GetNumItems() == packets);
			CHECK(Company::Get(_current_company)->money == money);
		}
	};
	RunUndergroundExcavationWorkflow(world, Ticks::DAY_TICKS, observe, {}, wide);
	std::set<std::string> expected{"stopped", "await-exchange", "dispose-origin", "return-empties"};
	// The Deep source bay requires the hauler's real Surface/Underground
	// crossing. The Underground bend scenario exchanges at its Surface source
	// magazine, so its hauler cannot establish a fictitious spanning snapshot.
	if (wide) expected.insert("bend-partial");
	else expected.insert("spanning");
	CHECK(written == expected);
}
