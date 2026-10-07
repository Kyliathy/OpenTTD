/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_presentation.cpp Live controller/geology/wagon presentation and refresh independence.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md task16.1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm_controls.h"
#include "../core/random_func.hpp"

#include "../safeguards.h"

/** Partial work and attached cargo are live quantities; repeated presentation queries cannot advance them or charge money. */
TEST_CASE("TBM presentation reads partial work and physical spoil without mutation", "[tbm-presentation]")
{
	TBMWorkRig rig(3);
	rig.Preload(rig.Wagons(TBMConsistRole::EarthWagon).front(), 7);
	rig.Preload(rig.Wagons(TBMConsistRole::RockWagon).front(), 11);
	rig.Tick();
	REQUIRE(rig.Job().partial_work_ticks > 0);
	REQUIRE(rig.Job().cursor == 0);
	const auto estimate = EstimateExcavationCut(rig.Project().job);
	const auto material = CaptureTBMMaterial(rig.Project());
	const auto before = rig.Digest();
	const auto projects = CaptureTBMProjects();
	const auto job = rig.Job();
	SavedRandomSeeds seeds;
	SaveRandomSeeds(&seeds);
	// Repeated paints must read the same owners while the simulation is held;
	// this also catches command estimates that accidentally perform work.
	for (uint paint = 0; paint < 20; ++paint) {
		const auto view = GetTBMPairControls(rig.Project().portal, rig.Project().company);
		REQUIRE(view.project.has_value());
		CHECK(view.work.face == job.route.front().cutter_position);
		CHECK(view.work.partial_work_ticks == job.partial_work_ticks);
		CHECK(view.face_work_ticks == estimate.work_ticks);
		CHECK(view.hardness == GetExcavationCellHardness(GetGeologyWorldState(), view.work.face));
		CHECK(view.earth_loaded_t == 7);
		CHECK(view.rock_loaded_t == 11);
		CHECK(view.earth_capacity_t - view.earth_loaded_t == material.earth_space_t);
		CHECK(view.rock_capacity_t - view.rock_loaded_t == material.rock_space_t);
		CHECK(view.pass_number == 1);
		CHECK(view.pass_total == job.pass_terms->exchange_after.size());
		CHECK(view.pass_cuts_done == 0);
		CHECK(view.pass_cuts_total == job.pass_terms->exchange_after.front());
		CHECK(view.active_cost_per_year == GetTBMCutterModel(rig.Project().model_id)->active_cost_per_year);
		CHECK(view.idle_cost_per_year > 0);
	}
	SavedRandomSeeds after;
	SaveRandomSeeds(&after);
	CHECK(std::ranges::equal(seeds.random.state, after.random.state));
	CHECK(std::ranges::equal(seeds.interactive_random.state, after.interactive_random.state));
	CHECK(rig.Digest() == before);
	CHECK(CaptureTBMProjects() == projects);
	CHECK(rig.Job() == job);

	REQUIRE(ControlTBMProject(rig.Project().id, TBMProjectAction::Pause, true) == TBMProjectError::None);
	const auto paused = GetTBMPairControls(rig.Project().portal, rig.Project().company);
	CHECK(paused.work.blocked == TBMBlockedReason::Paused);
	CHECK(paused.work.partial_work_ticks == job.partial_work_ticks);
	CHECK(paused.earth_loaded_t == 7);
	CHECK(paused.rock_loaded_t == 11);
}

/** Finished geological routes expose no fictitious next face while retaining the last pass and historical totals. */
TEST_CASE("TBM presentation clears the face after the final paid cut", "[tbm-presentation]")
{
	TBMWorkRig rig;
	const auto cut = rig.BoreOneCut();
	const auto before = rig.Digest();
	const auto view = GetTBMPairControls(rig.Project().portal, rig.Project().company);
	CHECK(view.work.face == INVALID_TILE);
	CHECK_FALSE(view.hardness.has_value());
	CHECK(view.face_work_ticks == 0);
	CHECK(view.cuts_done == view.cuts_total);
	CHECK(view.pass_number == view.pass_total);
	CHECK(view.pass_cuts_done == view.pass_cuts_total);
	CHECK(view.earth_loaded_t == cut.earth_t);
	CHECK(view.rock_loaded_t == cut.rock_t);
	CHECK(view.earth_t == cut.earth_t);
	CHECK(view.rock_t == cut.rock_t);
	CHECK(rig.Digest() == before);
}
