/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_phase.cpp Saved phase edges (4.2A), resumable conditions (4.3) and reload with recoverable references (4.4).
 * Face/cut/support/exchange facts are supplied explicitly here; tbm_work.cpp and tbm_support.cpp cover the real work.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 4
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../tbm_cmd.h"
#include "../command_func.h"

#include "../safeguards.h"

/**
 * Step the rig's project once and require success.
 * @param rig Published project rig.
 * @param inputs Observed facts for this step.
 * @return The project after the step.
 */
static const TBMProject &StepOnce(const TBMProjectRig &rig, const TBMPhaseInputs &inputs)
{
	REQUIRE(AdvanceTBMProjectPhase(rig.project.id, inputs) == TBMProjectError::None);
	return *GetTBMProject(rig.project.id);
}

/** Every 4.2A edge needs its own observed fact; the one phase timer restarts on each phase change. */
TEST_CASE("Phase edges follow observed facts and restart the one phase timer", "[tbm-phase]")
{
	TBMProjectRig rig;
	TBMPhaseInputs inputs;
	CHECK(StepOnce(rig, inputs).phase == TBMPhase::TravelToFace); // Published and unblocked: leave the source.
	CHECK(GetTBMProject(rig.project.id)->phase_ticks == 0);
	CHECK(GetTBMProject(rig.project.id)->revision == 2);
	CHECK(StepOnce(rig, inputs).phase == TBMPhase::TravelToFace); // Not at the face yet.
	CHECK(StepOnce(rig, inputs).phase_ticks == 2);
	CHECK(GetTBMProject(rig.project.id)->revision == 2); // Timer steps alone do not invalidate prepared participants.
	inputs.at_face = true;
	CHECK(StepOnce(rig, inputs).phase == TBMPhase::Bore);
	CHECK(GetTBMProject(rig.project.id)->phase_ticks == 0);
	inputs = {};
	CHECK(StepOnce(rig, inputs).phase == TBMPhase::Bore); // No committed cut, no exchange due.
	inputs.cut_committed = true;
	CHECK(StepOnce(rig, inputs).phase == TBMPhase::AdvanceSupport);
	inputs = {};
	CHECK(StepOnce(rig, inputs).phase == TBMPhase::AdvanceSupport); // Support not reached yet.
	inputs.support_advanced = true;
	CHECK(StepOnce(rig, inputs).phase == TBMPhase::Bore);
	inputs = {};
	inputs.exchange_due = true;
	inputs.cut_committed = true;
	CHECK(StepOnce(rig, inputs).phase == TBMPhase::ReturnForExchange); // A due exchange comes before the next cut.
	inputs = {};
	CHECK(StepOnce(rig, inputs).phase == TBMPhase::ReturnForExchange); // Not admitted at the bay in these inputs (Group 4.2B).
	const auto &project = *GetTBMProject(rig.project.id);
	CHECK(project.active_ticks == 10);
	CHECK(project.idle_ticks == 0);
	CHECK(project.heading == rig.proof.exit_heading);
}

/** The pure step covers the remaining branches without touching the store. */
TEST_CASE("Phase step guards exchange entry and inconsistent completion", "[tbm-phase]")
{
	TBMProjectRig rig;
	const auto digest = ComputeTBMProjectDigest();
	auto project = rig.project;
	TBMPhaseInputs inputs;
	project.phase = TBMPhase::AdvanceSupport;
	inputs.support_advanced = true;
	inputs.exchange_due = true;
	CHECK(StepTBMProjectPhase(project, inputs).phase == TBMPhase::ReturnForExchange);
	inputs.exchange_due = false;
	inputs.route_complete = true;
	CHECK(StepTBMProjectPhase(project, inputs).error == TBMProjectError::InvalidProject); // Frozen terms always end with a return.
	project.phase = TBMPhase::Completed;
	CHECK(StepTBMProjectPhase(project, {}).error == TBMProjectError::Unavailable);
	project.phase = TBMPhase::WaitForEmpties;
	TBMPhaseInputs waiting;
	waiting.exchange_due = true; // Spoil is still aboard; the exchange edges themselves are in tbm_exchange_consumer.cpp.
	const auto held = StepTBMProjectPhase(project, waiting);
	CHECK(held.phase == TBMPhase::WaitForEmpties);
	CHECK_FALSE(held.phase_changed);
	CHECK(ComputeTBMProjectDigest() == digest);
}

/** Stopped, broken-down, servicing and paused projects keep their phase and resume exactly where they were. */
TEST_CASE("Resumable conditions hold the phase without losing it", "[tbm-phase]")
{
	TBMProjectRig rig;
	TBMPhaseInputs inputs;
	inputs.at_face = true;
	StepOnce(rig, {});
	REQUIRE(StepOnce(rig, inputs).phase == TBMPhase::Bore);
	// Each train condition holds Bore and accrues idle time; clearing it resumes Bore, not an earlier phase.
	for (int condition = 0; condition < 3; ++condition) {
		TBMPhaseInputs held;
		held.train_stopped = condition == 0;
		held.broken_down = condition == 1;
		held.servicing = condition == 2;
		held.cut_committed = true; // Even a committed cut cannot move a held project on.
		const auto expected = condition == 0 ? TBMBlockedReason::Stopped : condition == 1 ? TBMBlockedReason::Breakdown : TBMBlockedReason::Servicing;
		const auto idle = GetTBMProject(rig.project.id)->idle_ticks;
		const auto &project = StepOnce(rig, held);
		CHECK(project.phase == TBMPhase::Bore);
		CHECK(project.blocked == expected);
		CHECK(project.idle_ticks == idle + 1);
		CHECK(StepOnce(rig, {}).blocked == TBMBlockedReason::None);
		CHECK(GetTBMProject(rig.project.id)->phase == TBMPhase::Bore);
	}
	// A player pause survives every step until Resume, whatever the train does.
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Pause).Succeeded());
	inputs = {};
	inputs.cut_committed = true;
	CHECK(StepOnce(rig, inputs).blocked == TBMBlockedReason::Paused);
	CHECK(GetTBMProject(rig.project.id)->phase == TBMPhase::Bore);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Resume).Succeeded());
	CHECK(StepOnce(rig, inputs).phase == TBMPhase::AdvanceSupport);

	// Reversing the train is train state; the cutting heading is saved authority and never follows it.
	rig.front->direction = ReverseDir(rig.front->direction);
	inputs = {};
	CHECK(StepOnce(rig, inputs).heading == rig.proof.exit_heading);

	// The observer reads the train and its face approach; this cutter is still at the source bay, not the housing.
	auto observed = ObserveTBMPhaseInputs(*GetTBMProject(rig.project.id));
	CHECK(observed.train_stopped); // Acquired parts are built stopped.
	CHECK_FALSE(observed.at_face);
	rig.front->vehstatus.Reset(VehState::Stopped);
	rig.front->breakdown_ctr = 1;
	observed = ObserveTBMPhaseInputs(*GetTBMProject(rig.project.id));
	CHECK_FALSE(observed.train_stopped);
	CHECK(observed.broken_down);
	rig.front->breakdown_ctr = 0;
	rig.front->vehstatus.Set(VehState::Stopped);
}

/** A project reloads unchanged at TravelToFace and at Bore entry; a lost reference reloads as a recoverable block. */
TEST_CASE("Projects reload at TravelToFace and Bore entry with recoverable references", "[tbm-phase]")
{
	TBMProjectRig rig;
	/**
	 * Save and reload the project store through its staging boundary.
	 * @return Whether the reload was accepted.
	 */
	const auto reload = []() {
		const auto saved = CaptureTBMProjects();
		const auto next = GetNextTBMProjectID();
		ResetTBMProjectState();
		StageLoadedTBMProjects(saved, next);
		return AcceptLoadedTBMProjects();
	};
	StepOnce(rig, {});
	REQUIRE(GetTBMProject(rig.project.id)->phase == TBMPhase::TravelToFace);
	auto digest = ComputeTBMProjectDigest();
	REQUIRE(reload());
	CHECK(ComputeTBMProjectDigest() == digest);
	TBMPhaseInputs inputs;
	inputs.at_face = true;
	REQUIRE(StepOnce(rig, inputs).phase == TBMPhase::Bore);
	digest = ComputeTBMProjectDigest();
	const auto saved = *GetTBMProject(rig.project.id);
	REQUIRE(reload());
	CHECK(ComputeTBMProjectDigest() == digest);
	CHECK(*GetTBMProject(rig.project.id) == saved);

	SECTION("an origin that stopped qualifying loads as MissingOrigin and recovers") {
		rig.fixture.origin->facilities.Reset(StationFacility::Train);
		REQUIRE(reload());
		CHECK(GetTBMProject(rig.project.id)->blocked == TBMBlockedReason::MissingOrigin);
		CHECK(GetTBMProject(rig.project.id)->origin == rig.fixture.origin->index); // Still resolves, so it is kept.
		CHECK(GetTBMProject(rig.project.id)->phase == TBMPhase::Bore);
		rig.fixture.origin->facilities.Set(StationFacility::Train);
		CHECK(StepOnce(rig, {}).blocked == TBMBlockedReason::None);
		CHECK(GetTBMProject(rig.project.id)->phase == TBMPhase::Bore);
	}
	SECTION("a cancelled then retired job loads as MissingProject with its ID cleared") {
		REQUIRE(CancelExcavationJob(saved.job) == ExcavationError::None);
		REQUIRE(RetireExcavationJob(saved.job) == ExcavationError::None);
		REQUIRE(reload());
		CHECK(GetTBMProject(rig.project.id)->blocked == TBMBlockedReason::MissingProject);
		CHECK(GetTBMProject(rig.project.id)->job == ExcavationJobID{UINT32_MAX}); // A reused ID can never rebind it.
		CHECK(StepOnce(rig, inputs).blocked == TBMBlockedReason::MissingProject);
		CHECK(GetTBMProject(rig.project.id)->phase == TBMPhase::Bore);
	}
	SECTION("a dangling cutter clears with its machinery list") {
		auto lost = CaptureTBMProjects();
		const auto next = GetNextTBMProjectID();
		lost.back().cutter = VehicleID{0xFFFE}; // No such vehicle exists in the fixture pool.
		ResetTBMProjectState();
		StageLoadedTBMProjects(lost, next);
		REQUIRE(AcceptLoadedTBMProjects());
		const auto &project = *GetTBMProject(rig.project.id);
		CHECK(project.cutter == VehicleID::Invalid());
		CHECK(project.permanent_parts.empty());
		CHECK(project.blocked == TBMBlockedReason::MissingCutter);
	}
}
