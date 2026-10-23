/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_controller_layers.cpp The whole native excavation workflow with actual room for a Standard and a
 * Deep project: every breakpoint met at the face is served there, with the hauler taking each load to the origin and
 * coming back empty for the next; a breakpoint reached before the face has room for the hauler still uses the bay.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 6 (task 6.6)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-a-face-exchange-design.md (rules F1, F2, F3, F6 and F7)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FA (whole-body approach clearance).
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "tbm_face_loop_watch.h"
#include "../safeguards.h"

/** Task 6.6: the chamber loop of each layer meets at its face once there is room, and never returns to the bay from there. */
TEST_CASE("Standard and Deep chamber loops serve the face whenever it has room", "[demo-fix-face-controller]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const MapLayer layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground);
	INFO("layer " << static_cast<int>(to_underlying(layer)));
	// The wide chamber's route is long enough to leave room behind the consist; it needs the sharp turns it plans.
	AutoRestoreBackup<bool> turns(_settings_game.pf.forbid_90_deg, false);
	// Deep's longer native consist needs a hard-rock load breakpoint while
	// its whole body still leaves a clear paid approach. Use the unchanged
	// regional vent geology rather than adding spoil or shortening the train.
	const bool deep = layer == MapLayer::DeepUnderground;
	std::optional<TileIndexDiffC> target;
	if (deep) target = TileIndexDiffC{0, -8};
	UndergroundWorkflowWorld world(layer, true, false, deep ? 1 : 0, 420, target);
	TBMFaceLoopWatch watch;
	/** @param current World after one native step. */
	const auto observe = [&watch](const UndergroundWorkflowWorld &current) {
		const auto projects = CaptureTBMProjects();
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		// The project exists from commissioning on, its hauler's assignment from the player's assignment on.
		if (projects.empty() || assignment == nullptr) return;
		watch.Observe(current.ticks, projects.front(), *assignment, *Train::Get(projects.front().cutter), *current.hauler);
	};
	// Earn actual room for the complete layer-specific machinery. The long
	// Deep pass leaves its longer body and a clear approach on the same leg;
	// a folded5x9 chamber may legitimately require the existing Bay instead.
	const TileIndex end = TileAddWrap(world.site, deep ? 0 : 4, deep ? 24 : 13);
	const TBMProjectID id = RunUndergroundExcavationWorkflow(world, Ticks::DAY_TICKS, observe, {}, true, end);
	INFO(watch.Describe());
	const TBMProject &project = *GetTBMProject(id);
	const ExcavationJob &job = *GetExcavationJob(project.job);
	CHECK(project.phase == TBMPhase::Completed);
	const auto &breakpoints = job.pass_terms->exchange_after;
	// The final breakpoint, after the last cut, is met at the face and followed by the empty final return.
	REQUIRE_FALSE(watch.face_transfers.empty());
	CHECK(watch.face_transfers.back() == job.route.size());
	// Once a breakpoint was served at the face, every later one was too: no bay exchange or bay return after it.
	const uint32_t first_face = watch.face_transfers.front();
	for (uint32_t cursor : watch.bay_exchanges) CHECK(cursor < first_face);
	for (uint32_t cursor : watch.bay_returns) CHECK(cursor < first_face);
	// Every frozen breakpoint had exactly one exchange, at the face or in the bay.
	CHECK(watch.face_transfers.size() + watch.bay_exchanges.size() == breakpoints.size());
	if (layer == MapLayer::DeepUnderground) CHECK(watch.face_transfers.size() >= 2); // The Deep chamber serves two breakpoints in a row at its face.
	CHECK_FALSE(watch.unserved_load);
	CHECK_FALSE(watch.moved_while_waiting);
	CHECK_FALSE(watch.kept_heading_on_return);
	CHECK(watch.obstructed_ticks > 0); // The final return waited for the loaded hauler to leave the corridor.
	CHECK_FALSE(watch.moved_while_obstructed);
	CHECK_FALSE(watch.order_problem);
}

/** Tight paid chambers must finish through the real bay when the complete cutter blocks a face approach. */
TEST_CASE("Tight Standard and Deep chambers finish through safe bay fallback", "[tbm-face-tight-chamber]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const MapLayer layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground);
	INFO("layer " << static_cast<int>(to_underlying(layer)));
	AutoRestoreBackup<bool> turns(_settings_game.pf.forbid_90_deg, false);
	UndergroundWorkflowWorld world(layer, true);
	TBMFaceLoopWatch watch;
	/** @param current Original paid world after one native frame; observe both controllers without moving either train. */
	const auto observe = [&watch](const UndergroundWorkflowWorld &current) {
		const auto projects = CaptureTBMProjects();
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		// Commissioning publishes the project before the player's actual hauler
		// assignment; neither absent owner can provide an exchange observation.
		if (projects.empty() || assignment == nullptr) return;
		watch.Observe(current.ticks, projects.front(), *assignment, *Train::Get(projects.front().cutter), *current.hauler);
	};
	const TBMProjectID id = RunUndergroundExcavationWorkflow(world, 0, observe, {}, true);
	INFO(watch.Describe());
	const TBMProject &project = *GetTBMProject(id);
	const ExcavationJob &job = *GetExcavationJob(project.job);
	CHECK(project.phase == TBMPhase::Completed);
	REQUIRE_FALSE(watch.bay_exchanges.empty());
	// Every frozen real load is settled exactly once even when its stand is
	// unreachable; fallback cannot lose a load or manufacture another cycle.
	CHECK(watch.face_transfers.size() + watch.bay_exchanges.size() == job.pass_terms->exchange_after.size());
	CHECK_FALSE(watch.unserved_load);
	CHECK_FALSE(watch.moved_while_waiting);
	CHECK_FALSE(watch.kept_heading_on_return);
	CHECK_FALSE(watch.moved_while_obstructed);
	CHECK_FALSE(watch.order_problem);
}
