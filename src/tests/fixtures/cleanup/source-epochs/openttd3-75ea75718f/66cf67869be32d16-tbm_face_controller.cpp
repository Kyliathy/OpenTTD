/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_controller.cpp The cutter waiting at its face and the hauler serving it: every phase edge of the face
 * wait and its fallback to the bay, the hold, logistics and wording of the wait, the hauler's own controller taking the
 * spoil at the face, and ordinary trains left alone. The isolated whole Standard loop is in
 * tbm_face_controller_loop.cpp; chamber loops of both layers are in tbm_face_controller_layers.cpp.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 6 (tasks 6.1-6.6)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-a-face-exchange-design.md (rules F2, F3, F6 and F7)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "tbm_source_loop_fixture.h"
#include "tbm_face_loop_watch.h"
#include "../tbm_gui.h"
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

/**
 * Units' capacity, every articulated part included.
 * @param units Unit heads.
 * @return Their capacity.
 */
static uint64_t CapacityOf(const std::vector<Train *> &units)
{
	uint64_t total = 0;
	for (Train *unit : units) {
		for (const Train *part = unit; part != nullptr; part = part->Next()) {
			total += part->cargo_cap;
			if (part == unit->GetLastEnginePart()) break;
		}
	}
	return total;
}

/** Tasks 6.2 and 6.6: every edge into, through and out of the face wait, and the fallback to the bay. */
TEST_CASE("Every edge of the face wait, including its fallback to the bay", "[demo-fix-face-controller]")
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

/** Tasks 6.1, 6.3 and 6.5: the waiting cutter holds still, asks for a hauler when it has none, and says what it does. */
TEST_CASE("The cutter holds still and needs a hauler while it waits at the face", "[demo-fix-face-controller]")
{
	TBMFaceTransferRig rig(TBMPhase::AwaitFaceTransfer, TBM_FACE_TRANSFER_TICKS, 40);
	CHECK(IsTBMHoldingTrain(*rig.front));
	CHECK(GetTBMPhaseString(TBMPhase::AwaitFaceTransfer) == STR_TBM_PHASE_AWAIT_FACE);
	CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::None); // Its hauler is assigned.
	// The face turn holder's wording says it serves the TBM itself; leaving with the load it reads as at the bay.
	TBMExchangeAssignment caption = *GetTBMExchangeAssignmentForHauler(rig.hauler->index);
	REQUIRE(IsTBMFaceTurnHolder(caption));
	caption.phase = TBMHaulerPhase::ReturnEmpties;
	CHECK(GetTBMHaulerPhaseString(caption) == STR_PORTAL_HAULER_FACE_RETURN);
	caption.phase = TBMHaulerPhase::AwaitExchange;
	CHECK(GetTBMHaulerPhaseString(caption) == STR_PORTAL_HAULER_FACE_TRANSFER);
	caption.phase = TBMHaulerPhase::CollectLoaded;
	CHECK(GetTBMHaulerPhaseString(caption) == STR_PORTAL_HAULER_COLLECT_LOADED);
	// Without a hauler the face wait asks for one, as the bay wait does.
	REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
	CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::NoHauler);
	CHECK(IsTBMHoldingTrain(*rig.front));
}

/** Task 6.4: the hauler's own controller takes the spoil at the face, holds with its reason, and leaves other trains alone. */
TEST_CASE("The hauler takes the spoil at the face through its own controller", "[demo-fix-face-controller]")
{
	SECTION("a transfer moves the cycle on and turns the hauler on the stand") {
		TBMFaceTransferRig rig(TBMPhase::AwaitFaceTransfer, TBM_FACE_TRANSFER_TICKS, 40);
		const uint64_t load = TBMExchangeRig::Aboard(rig.front);
		REQUIRE(load != 0);
		CHECK(GetTBMMeetingKind(InPhase(rig.Project(), TBMPhase::ReturnForExchange)) == TBMMeetingKind::Face);
		REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange); // On its stand, facing the cutter.
		const auto collected = rig.StepHauler();
		CHECK(collected.phase == TBMHaulerPhase::CollectLoaded);
		CHECK(collected.exchanges == 1);
		CHECK(TBMExchangeRig::Aboard(rig.hauler) == load);
		CHECK(TBMExchangeRig::Aboard(rig.front) == 0);
		CHECK(GetTBMMeetingKind(InPhase(rig.Project(), TBMPhase::ReturnForExchange)) == TBMMeetingKind::Bay); // A return with no actual spoil cannot invite another empty hauler and renew its own corridor hold.
		CHECK_FALSE(IsTBMHaulerHeld(*rig.hauler)); // Turned round on the stand, it no longer faces it.
		// The same physical geometry must remain available while the loaded
		// hauler leaves. A later cut in a folded chamber could block its return.
		CHECK(rig.Tick().blocked == TBMBlockedReason::Obstruction);
		rig.PlaceOnTrack(rig.hauler, GetLayerPortalDoorTile(rig.Key()));
		CHECK(rig.Tick().phase == TBMPhase::Bore);
	}
	SECTION("too little room in the hoppers holds the hauler with its reason") {
		TBMFaceTransferRig rig(TBMPhase::AwaitFaceTransfer, TBM_FACE_TRANSFER_TICKS, 40);
		REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
		const uint64_t room = CapacityOf(rig.SetWagons(TBMConsistRole::EarthWagon));
		REQUIRE(room < CargoPacket::MAX_COUNT);
		TBMFaceTransferRig::AddPacket(rig.Wagons(TBMConsistRole::EarthWagon).front(), static_cast<uint16_t>(room + 1));
		const auto held = rig.StepHauler();
		CHECK(held.phase == TBMHaulerPhase::AwaitExchange);
		CHECK(held.wait == TBMHaulerWait::NoEmpties);
		CHECK(held.exchanges == 0);
		CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::NoEmpties);
	}
	SECTION("an ordinary train on the stand is left alone") {
		TBMFaceTransferRig rig(TBMPhase::AwaitFaceTransfer, TBM_FACE_TRANSFER_TICKS, 40);
		Train *other = rig.BuildHauler(rig.depot, 2, 2);
		const auto stand = FindTBMFaceStand(rig.Project());
		REQUIRE(stand.has_value());
		// Put the ordinary train where the hauler stands, heading the same way, and the hauler back in its depot.
		rig.PlaceInDepot(rig.hauler, rig.depot);
		const auto path = rig.Path();
		const size_t at = static_cast<size_t>(std::ranges::find(path, stand->tile) - path.begin());
		PlaceTrainOnPath(other, path, at, at - std::min(at, CountTrainParts(*other) - 1));
		for (Train *part = other; part != nullptr; part = part->Next()) part->direction = DiagDirToDir(TrackdirToExitdir(stand->trackdir));
		CHECK_FALSE(IsTBMHaulerHeld(*other));
		CHECK_FALSE(GetTBMHaulerSteeringTarget(*other).has_value());
		CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*other, rig.Key()));
		CHECK_FALSE(IsTBMHaulerFaceStand(*other, stand->tile, stand->trackdir));
		CHECK(DescribeTBMFaceTransfer({rig.project.id, other->index}).error == TBMExchangeError::NotAssigned);
		CHECK_FALSE(GetTBMOrderProblem(*other).has_value()); // Ordinary trains keep the normal order review.
	}
}
