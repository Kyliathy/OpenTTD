/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_controller.cpp Native face-wait phase decisions and actual paid-cut activation.
 * Component train poses isolate controller behavior; the full driven origin/disposal loop is a separate case.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical work and cargo owners.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Groups23-23B.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "../safeguards.h"

/**
 * @param project Project to copy.
 * @param phase Phase to give the copy.
 * @return The copy, read by the pure queries as if the project were in that phase.
 */
static TBMProject InPhase(const TBMProject &project, TBMPhase phase)
{
	TBMProject copy = project;
	copy.phase = phase;
	copy.blocked = TBMBlockedReason::None;
	return copy;
}

/** A paid cut activates the native wait, earns the dwell, transfers the actual load and resumes via new support. */
TEST_CASE("Paid due cut installs face wait and whole transfer resumes through support", "[tbm-face-controller]")
{
	TBMFaceTransferRig rig(TBMPhase::AwaitFaceTransfer, TBM_FACE_TRANSFER_TICKS, 40);
	REQUIRE(rig.Project().phase == TBMPhase::AwaitFaceTransfer);
	CHECK(ValidateTBMProject(rig.Project()) == TBMProjectError::None);
	CHECK(IsTBMHoldingTrain(*rig.front));
	CHECK_FALSE(ObserveTBMPhaseInputs(rig.Project()).at_face);
	const auto cursor = rig.Job().cursor;
	const auto before = CountTBMPackets({rig.front, rig.hauler});
	const auto load = TBMExchangeRig::Aboard(rig.front);
	REQUIRE(load != 0);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
	CHECK(TBMExchangeRig::Aboard(rig.front) == 0);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == load);
	CHECK(CountTBMPackets({rig.front, rig.hauler}) == before);
	CHECK(rig.Tick().blocked == TBMBlockedReason::Obstruction);
	CHECK(rig.Job().cursor == cursor);
	// This is a declared clearance component: moving the hauler beyond the Source isolates the release
	// edge. Group23B must prove the same clearance through actual native departure and empty return.
	rig.PlaceOnTrack(rig.hauler, GetLayerPortalDoorTile(rig.Key()));
	CHECK(rig.Tick().phase == TBMPhase::AdvanceSupport);
	CHECK(rig.Job().cursor == cursor);
	CHECK_FALSE(IsTBMExchangeDue(rig.Project()));
}

/**
 * @param rig Rig whose live project is stepped.
 * @param phase Phase the project is stepped from.
 * @param inputs Observed facts.
 * @return The pure step of the project in that phase.
 */
static TBMPhaseStep StepAs(const TBMFaceRig &rig, TBMPhase phase, const TBMPhaseInputs &inputs)
{
	return StepTBMProjectPhase(InPhase(rig.Project(), phase), inputs);
}

/** @return The facts of a cutter on its new support at the face, with an exchange due. */
static TBMPhaseInputs DueAtFace()
{
	TBMPhaseInputs inputs;
	inputs.exchange_due = true;
	inputs.at_face = true;
	inputs.support_advanced = true;
	return inputs;
}

/** Tasks 6.2 and 6.6: every edge into, through and out of the face wait, and the fallback to the bay. */
TEST_CASE("Every edge of the face wait, including its fallback to the bay", "[tbm-face-controller]")
{
	TBMFaceRig rig;
	rig.BoreClearOfHousing();
	rig.StandAtFace();
	rig.Assign(); // After the boring, whose reloads start the exchange store afresh.
	REQUIRE(GetTBMMeetingKind(InPhase(rig.Project(), TBMPhase::Bore)) == TBMMeetingKind::Face);

	SECTION("a breakpoint met at the face waits there") {
		CHECK(StepAs(rig, TBMPhase::Bore, DueAtFace()).phase == TBMPhase::AwaitFaceTransfer);
		CHECK(StepAs(rig, TBMPhase::AdvanceSupport, DueAtFace()).phase == TBMPhase::AwaitFaceTransfer);
		TBMPhaseInputs driving = DueAtFace();
		driving.support_advanced = false;
		CHECK(StepAs(rig, TBMPhase::AdvanceSupport, driving).phase == TBMPhase::AdvanceSupport); // Still driving onto the new support.
		// The wait goes on, unblocked, while the spoil is aboard and the face is the meeting point.
		const auto waiting = StepAs(rig, TBMPhase::AwaitFaceTransfer, DueAtFace());
		CHECK(waiting.phase == TBMPhase::AwaitFaceTransfer);
		CHECK_FALSE(waiting.phase_changed);
		CHECK(waiting.blocked == TBMBlockedReason::None);
	}
	SECTION("the wait ends once the spoil has left") {
		TBMPhaseInputs done;
		done.at_face = true;
		CHECK(StepAs(rig, TBMPhase::AwaitFaceTransfer, done).phase == TBMPhase::Bore);
		done.at_face = false;
		CHECK(StepAs(rig, TBMPhase::AwaitFaceTransfer, done).phase == TBMPhase::AdvanceSupport); // One cut behind: onto the new support first.
		done.route_complete = true;
		CHECK(StepAs(rig, TBMPhase::AwaitFaceTransfer, done).phase == TBMPhase::ReturnForExchange); // The empty final return.
		TBMPhaseInputs cut;
		cut.at_face = true;
		cut.cut_committed = true;
		CHECK(StepAs(rig, TBMPhase::Bore, cut).phase == TBMPhase::AdvanceSupport); // A cut with no exchange due moves on as before.
	}
	SECTION("a lost stand settles at the bay") {
		// Two cuts short of the face the consist has left it, and the meeting point is the bay in every phase.
		const auto path = rig.Path();
		PlaceTrainOnPath(rig.front, path, path.size() - 3, path.size() - 2 - rig.parts);
		REQUIRE(GetTBMMeetingKind(InPhase(rig.Project(), TBMPhase::AwaitFaceTransfer)) == TBMMeetingKind::Bay);
		CHECK(StepAs(rig, TBMPhase::Bore, DueAtFace()).phase == TBMPhase::ReturnForExchange);
		CHECK(StepAs(rig, TBMPhase::AdvanceSupport, DueAtFace()).phase == TBMPhase::ReturnForExchange);
		CHECK(StepAs(rig, TBMPhase::AwaitFaceTransfer, DueAtFace()).phase == TBMPhase::ReturnForExchange);
	}
	SECTION("a cancelled job, a service visit and the holds") {
		TBMPhaseInputs cancelled = DueAtFace();
		cancelled.cancelled = true;
		cancelled.route_complete = true;
		CHECK(StepAs(rig, TBMPhase::AwaitFaceTransfer, cancelled).phase == TBMPhase::ReturnForExchange);
		TBMPhaseInputs service = DueAtFace();
		service.servicing = true;
		const auto serviced = StepAs(rig, TBMPhase::AwaitFaceTransfer, service);
		CHECK(serviced.phase == TBMPhase::TravelToFace); // It rejoins at the face after the visit.
		CHECK(serviced.blocked == TBMBlockedReason::Servicing);
		TBMPhaseInputs stopped = DueAtFace();
		stopped.train_stopped = true;
		const auto held = StepAs(rig, TBMPhase::AwaitFaceTransfer, stopped);
		CHECK(held.phase == TBMPhase::AwaitFaceTransfer);
		CHECK(held.blocked == TBMBlockedReason::Stopped);
		TBMPhaseInputs stalled = DueAtFace();
		stalled.work_wait = TBMBlockedReason::LogisticsStall;
		CHECK(StepAs(rig, TBMPhase::AwaitFaceTransfer, stalled).blocked == TBMBlockedReason::LogisticsStall);
	}
}

