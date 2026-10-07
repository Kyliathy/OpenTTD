/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file portal_occupied_save.cpp Actual assigned Deep cutter and real carriages entering/spanning their restricted commissioning pair.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 16
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 7BD
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "depot_file_fixture.h"

#include "../safeguards.h"

/** Save native commissioning only when the assigned whole train physically occupies its real restricted pocket and both paired housings. */
TEST_CASE("A native Deep cutter saves actual commissioning pocket and spanning parts", "[portal-occupied-save-file]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world(MapLayer::DeepUnderground, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	DepotFileRecorder recorder;
	std::set<std::string> saved;
	/** @param current Actual fully stepped paid world; physical part presence supplies every selected state. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(assignment != nullptr);
		const auto *project = GetTBMProject(assignment->project);
		REQUIRE(project != nullptr);
		const Train *front = Train::Get(project->cutter);
		const auto *work = GetLayerPortalWorkRecord(project->portal);
		REQUIRE(work != nullptr);
		REQUIRE(work->state == LayerPortalWorkState::Commissioning);
		bool source = false, destination = false, wagon = false;
		// All observations refer to this exact assigned chain and its real pair. Unrelated trains on coincident planes cannot prove work-pocket admission.
		for (const Train *part = front; part != nullptr; part = part->Next()) {
			source |= part->tile == current.portal;
			destination |= part->tile == current.rock;
			if (part->tile == current.rock && !part->IsArticulatedPart() && part != front) wagon = true; // A real detachable wagon must enter with its own cargo/identity, not just the machinery head.
		}
		std::vector<std::string> states;
		if (front->vehstatus.Test(VehState::Stopped)) states.emplace_back("stopped");
		if (destination && wagon) states.emplace_back("work-pocket"); // Pocket proof includes a real trailing wagon in the restricted destination housing.
		if (source && destination) states.emplace_back("cutter-spanning"); // The same actual chain must physically occupy both reciprocal housings.
		// Preserve the first genuine occurrence of each condition; subsequent cutting/exchanges cannot overwrite its original state.
		for (const auto &state : states) {
			if (!saved.insert(state).second) continue;
			recorder.Save(state);
			fmt::print("PORTAL_OCCUPIED state={} pair={}/{} cutter={} frames={}\n", state, current.portal.base(), current.rock.base(), front->index.base(), current.ticks);
		}
	};
	RunUndergroundExcavationWorkflow(world, Ticks::DAY_TICKS, observe);
	CHECK(saved == std::set<std::string>{"stopped", "work-pocket", "cutter-spanning"});
}
