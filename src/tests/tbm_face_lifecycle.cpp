/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_lifecycle.cpp Parking, release, hangar and far-exit flows around the face wait: each request made
 * while the cutter waits at its face, with its hauler on the stand (the spoil goes over at the face first) and without
 * one (the loaded cutter settles at the bay as before); settlement and the far exit waiting for a hauler in the
 * corridor. Replan and source-loss regressions continue in Group23E.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23D native park/cancel/hangar settlement.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "tbm_face_lifecycle_fixture.h"
#include "../tbm_replanning.h"
#include "../tbm_cmd.h"
#include "../vehicle_cmd.h"
#include "../safeguards.h"

/** Tasks 7.1, 7.2 and 7.6: a park request at the face wait, with and without the hauler on the stand. */
TEST_CASE("Return and park from the face wait", "[tbm-face-lifecycle]")
{
	SourceLoop loop(LoopStart::Long);
	TBMFaceLoopWatch watch;
	SECTION("the hauler on the stand takes the spoil, leaves, and the empty cutter parks") {
		ReachTBMFaceWait(loop, watch, true);
		const uint32_t cursor = loop.Job().cursor;
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, loop.project, TBMProjectAction::ReturnAndPark).Succeeded());
		CHECK(loop.Project().phase == TBMPhase::ReturnToPark);
		CHECK(GetTBMMeetingKind(loop.Project()) == TBMMeetingKind::Face); // Held for the hauler already in the corridor (rule F1, Group 7).
		REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, /** @return Whether the actual empty whole cutter is parked. */
		[&loop] { return loop.Project().phase == TBMPhase::Parked; }));
		CheckTBMServedAtFace(watch, cursor);
		CHECK(SourceLoop::Aboard(loop.cutter) == SpoilTonnes{});
	}
	SECTION("with no hauler at the face the loaded cutter parks after a bay exchange, as before") {
		ReachTBMFaceWait(loop, watch, false);
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, loop.project, TBMProjectAction::ReturnAndPark).Succeeded());
		CHECK(GetTBMMeetingKind(loop.Project()) == TBMMeetingKind::Bay);
		REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, /** @return Whether the actual cutter reached its bay still awaiting spoil exchange. */
		[&loop] { return loop.Project().phase == TBMPhase::WaitToPark; }));
		CHECK(IsTBMExchangeDue(loop.Project())); // It came back loaded.
		StartTBMFaceHauler(loop);
		REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, /** @return Whether the actual empty whole cutter is parked. */
		[&loop] { return loop.Project().phase == TBMPhase::Parked; }));
		INFO(watch.Describe());
		CHECK(watch.face_transfers.empty());
		CHECK(watch.bay_exchanges.size() == 1);
		CHECK_FALSE(watch.kept_heading_on_return);
	}
	CHECK(loop.IsConserved());
}

/** Tasks 7.1-7.3 and 7.6: a release request at the face wait; settlement waits for, and names, a hauler in the corridor. */
TEST_CASE("Finish and release from the face wait", "[tbm-face-lifecycle]")
{
	SourceLoop loop(LoopStart::Long);
	TBMFaceLoopWatch watch;
	SECTION("the hauler on the stand takes the spoil and leaves before the project settles") {
		ReachTBMFaceWait(loop, watch, true);
		const uint32_t cursor = loop.Job().cursor;
		const auto claims = GetGeologyWorldState().job_claims;
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, loop.project, TBMProjectAction::Cancel).Succeeded());
		CHECK(GetGeologyWorldState().job_claims == claims);
		CHECK(loop.Job().state != ExcavationJobState::Cancelled);
		CHECK(loop.Project().phase == TBMPhase::ReturnToRelease);
		// While the hauler is in the corridor nothing settles, and the settlement names it (task 7.3).
		CHECK(GetTBMProjectSettlementBlocker(loop.project) == std::optional<VehicleID>{loop.hauler->index});
		CHECK_FALSE(ReleaseSettledTBMProjectHaulers(loop.project, false));
		REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, /** @return Whether the complete cutter withdrew, without assuming haulage is settled. */
		[&loop] { return loop.Project().phase == TBMPhase::ReleasePending || IsTBMProjectFinished(loop.Project().phase); }));
		CheckTBMServedAtFace(watch, cursor);
		CHECK(SourceLoop::Aboard(loop.cutter) == SpoilTonnes{});
		// The cutter waits withdrawn while the hauler takes the last load away; release names that haulage.
		if (loop.Project().phase == TBMPhase::ReleasePending) {
			CHECK(GetTBMProjectReleaseProblem(loop.project) == STR_ERROR_TBM_RELEASE_HAULAGE);
			CHECK(loop.Job().state != ExcavationJobState::Cancelled);
			// Unfinished claims remain live until the real final packets reach disposal and the complete hauler clears.
			CHECK(std::ranges::any_of(GetGeologyWorldState().job_claims, /** @param entry Live tile/job claim. @return Whether this unfinished job still owns it. */
			[&loop](const auto &entry) { return entry.second == loop.Job().id; }));
			CHECK(GetTBMProjectSettlementBlocker(loop.project) == std::optional<VehicleID>{loop.hauler->index});
		}
		// Once the hauler has delivered and left, the project settles by itself.
		REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, /** @return Whether actual final haulage and withdrawal ended the controller. */
		[&loop] { return IsTBMProjectFinished(loop.Project().phase); }));
		CHECK(loop.Project().phase == TBMPhase::Cancelled);
	}
	SECTION("with no hauler at the face the loaded cutter releases after a bay exchange, as before") {
		ReachTBMFaceWait(loop, watch, false);
		const auto claims = GetGeologyWorldState().job_claims;
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, loop.project, TBMProjectAction::Cancel).Succeeded());
		CHECK(GetGeologyWorldState().job_claims == claims);
		CHECK(loop.Job().state != ExcavationJobState::Cancelled);
		REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, /** @return Whether the loaded cutter reached its release exchange bay. */
		[&loop] { return loop.Project().phase == TBMPhase::WaitToRelease; }));
		CHECK(IsTBMExchangeDue(loop.Project()));
		StartTBMFaceHauler(loop);
		REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, /** @return Whether actual final haulage and withdrawal ended the controller. */
		[&loop] { return IsTBMProjectFinished(loop.Project().phase); }));
		INFO(watch.Describe());
		CHECK(watch.face_transfers.empty());
		CHECK(watch.bay_exchanges.size() == 1);
		CHECK(loop.Project().phase == TBMPhase::Cancelled);
	}
	// Final settlement releases this job's unfinished claims only after native withdrawal and disposal.
	CHECK(std::ranges::none_of(GetGeologyWorldState().job_claims,
		/** @param entry Remaining world claim. @return Whether it still belongs to the settled job. */
		[&loop](const auto &entry) { return entry.second == loop.Job().id; }));
	CHECK(loop.IsConserved());
}

/** Tasks 7.1, 7.2 and 7.6: a hangar order at the face wait goes on to the hangar once the spoil is gone. */
TEST_CASE("A hangar order from the face wait", "[tbm-face-lifecycle]")
{
	SourceLoop loop(LoopStart::Long);
	TBMFaceLoopWatch watch;
	/** @return Whether the cutter has set off for, or reached, its hangar. */
	const auto travelling = [&loop] { return loop.Project().phase == TBMPhase::TravelToHangar || loop.Project().phase == TBMPhase::ParkedHangar; };
	SECTION("the hauler on the stand takes the spoil first") {
		ReachTBMFaceWait(loop, watch, true);
		const uint32_t cursor = loop.Job().cursor;
		REQUIRE(Command<Commands::SendTBMToHangar>::Do(DoCommandFlag::Execute, loop.cutter->index, loop.source, false).Succeeded());
		CHECK(loop.Project().phase == TBMPhase::ReturnForHangar);
		REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, travelling));
		CheckTBMServedAtFace(watch, cursor);
		CHECK(SourceLoop::Aboard(loop.cutter) == SpoilTonnes{});
	}
	SECTION("with no hauler at the face the loaded cutter is emptied at the bay, as before") {
		ReachTBMFaceWait(loop, watch, false);
		REQUIRE(Command<Commands::SendTBMToHangar>::Do(DoCommandFlag::Execute, loop.cutter->index, loop.source, false).Succeeded());
		CHECK(loop.Project().phase == TBMPhase::ReturnForHangar);
		REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, /** @return Whether the loaded cutter reached its hangar exchange bay. */
		[&loop] { return loop.Project().phase == TBMPhase::WaitForHangar; }));
		StartTBMFaceHauler(loop);
		REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, travelling));
		INFO(watch.Describe());
		CHECK(watch.face_transfers.empty());
		CHECK(watch.bay_exchanges.size() == 1);
	}
	CHECK(loop.IsConserved());
}

