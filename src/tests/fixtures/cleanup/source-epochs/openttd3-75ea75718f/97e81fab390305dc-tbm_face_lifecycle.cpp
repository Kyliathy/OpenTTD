/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_lifecycle.cpp Parking, release, hangar and far-exit flows around the face wait: each request made
 * while the cutter waits at its face, with its hauler on the stand (the spoil goes over at the face first) and without
 * one (the loaded cutter settles at the bay as before); settlement and the far exit waiting for a hauler in the
 * corridor; a change of work during the wait; and a hauler crashed, stopped or offered for sale in the corridor.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 7 (tasks 7.1-7.6)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-a-face-exchange-design.md (rules F1, F7 and F8)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "tbm_source_loop_fixture.h"
#include "tbm_face_loop_watch.h"
#include "../tbm_replanning.h"
#include "../tbm_cmd.h"
#include "../vehicle_cmd.h"
#include "../safeguards.h"

/**
 * Run one game tick of a source loop and let the watch read it.
 * @param loop Loop to run.
 * @param watch What the face checks saw.
 */
static void Step(SourceLoop &loop, TBMFaceLoopWatch &watch)
{
	AdvanceSpoilReceiver(*loop.earth);
	AdvanceSpoilReceiver(*loop.rock);
	CallVehicleTicks();
	++loop.ticks;
	REQUIRE_FALSE(loop.cutter->vehstatus.Test(VehState::Crashed));
	REQUIRE_FALSE(loop.hauler->vehstatus.Test(VehState::Crashed));
	watch.Observe(loop.ticks, loop.Project(), loop.Cycle(), *loop.cutter, *loop.hauler);
}

/**
 * Run a loop until a condition holds.
 * @param loop Loop to run.
 * @param watch What the face checks saw.
 * @param done Condition to reach.
 * @param limit Most ticks to run; it only ends a stalled run.
 * @return Whether the condition holds.
 */
static bool RunUntil(SourceLoop &loop, TBMFaceLoopWatch &watch, const std::function<bool()> &done, uint limit = 60000)
{
	const uint end = loop.ticks + limit;
	while (!done() && loop.ticks < end) Step(loop, watch);
	return done();
}

/**
 * Drive a long Standard loop to its first face wait, before any spoil has gone over.
 * @param loop Loop to drive.
 * @param watch What the face checks saw.
 * @param with_hauler Whether the hauler then stands on the stand; otherwise the player keeps it stopped in its depot.
 */
static void ReachFaceWait(SourceLoop &loop, TBMFaceLoopWatch &watch, bool with_hauler)
{
	if (!with_hauler) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, loop.hauler->index, false).Succeeded());
	REQUIRE(RunUntil(loop, watch, [&loop, with_hauler] {
		// The wait has begun, and the hauler, when wanted, stands held on its stand facing the cutter.
		return IsTBMAwaitingFaceTransfer(loop.Project().phase) && (!with_hauler || (loop.Cycle().phase == TBMHaulerPhase::AwaitExchange && IsTBMHaulerHeld(*loop.hauler)));
	}));
	REQUIRE(loop.Cycle().exchanges == 0);
	REQUIRE(IsTBMExchangeDue(loop.Project()));
}

/**
 * Start a hauler the player stopped, so it brings its empties to the bay.
 * @param loop Loop whose hauler starts.
 */
static void StartHauler(SourceLoop &loop)
{
	REQUIRE(loop.hauler->vehstatus.Test(VehState::Stopped));
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, loop.hauler->index, false).Succeeded());
}

/**
 * Check the face column of a table row: one transfer at the face, no bay exchange, and the cutter still while the
 * hauler was in its corridor.
 * @param watch What the face checks saw.
 * @param cursor Completed cuts at the wait.
 */
static void CheckServedAtFace(const TBMFaceLoopWatch &watch, uint32_t cursor)
{
	INFO(watch.Describe());
	CHECK(watch.face_transfers == std::vector<uint32_t>{cursor});
	CHECK(watch.bay_exchanges.empty());
	CHECK(watch.obstructed_ticks > 0);
	CHECK_FALSE(watch.moved_while_obstructed);
	CHECK_FALSE(watch.kept_heading_on_return); // The request backed the cutter at once (task 7.1).
}

/** Tasks 7.1, 7.2 and 7.6: a park request at the face wait, with and without the hauler on the stand. */
TEST_CASE("Return and park from the face wait", "[demo-fix-face-lifecycle]")
{
	SourceLoop loop(LoopStart::Long);
	TBMFaceLoopWatch watch;
	SECTION("the hauler on the stand takes the spoil, leaves, and the empty cutter parks") {
		ReachFaceWait(loop, watch, true);
		const uint32_t cursor = loop.Job().cursor;
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, loop.project, TBMProjectAction::ReturnAndPark).Succeeded());
		CHECK(loop.Project().phase == TBMPhase::ReturnToPark);
		CHECK(GetTBMMeetingKind(loop.Project()) == TBMMeetingKind::Face); // Held for the hauler already in the corridor (rule F1, Group 7).
		REQUIRE(RunUntil(loop, watch, [&loop] { return loop.Project().phase == TBMPhase::Parked; }));
		CheckServedAtFace(watch, cursor);
		CHECK(SourceLoop::Aboard(loop.cutter) == SpoilTonnes{});
	}
	SECTION("with no hauler at the face the loaded cutter parks after a bay exchange, as before") {
		ReachFaceWait(loop, watch, false);
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, loop.project, TBMProjectAction::ReturnAndPark).Succeeded());
		CHECK(GetTBMMeetingKind(loop.Project()) == TBMMeetingKind::Bay);
		REQUIRE(RunUntil(loop, watch, [&loop] { return loop.Project().phase == TBMPhase::WaitToPark; }));
		CHECK(IsTBMExchangeDue(loop.Project())); // It came back loaded.
		StartHauler(loop);
		REQUIRE(RunUntil(loop, watch, [&loop] { return loop.Project().phase == TBMPhase::Parked; }));
		INFO(watch.Describe());
		CHECK(watch.face_transfers.empty());
		CHECK(watch.bay_exchanges.size() == 1);
		CHECK_FALSE(watch.kept_heading_on_return);
	}
	CHECK(loop.IsConserved());
}

/** Tasks 7.1-7.3 and 7.6: a release request at the face wait; settlement waits for, and names, a hauler in the corridor. */
TEST_CASE("Finish and release from the face wait", "[demo-fix-face-lifecycle]")
{
	SourceLoop loop(LoopStart::Long);
	TBMFaceLoopWatch watch;
	SECTION("the hauler on the stand takes the spoil and leaves before the project settles") {
		ReachFaceWait(loop, watch, true);
		const uint32_t cursor = loop.Job().cursor;
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, loop.project, TBMProjectAction::Cancel).Succeeded());
		CHECK(loop.Project().phase == TBMPhase::ReturnToRelease);
		// While the hauler is in the corridor nothing settles, and the settlement names it (task 7.3).
		CHECK(GetTBMProjectSettlementBlocker(loop.project) == std::optional<VehicleID>{loop.hauler->index});
		CHECK_FALSE(ReleaseSettledTBMProjectHaulers(loop.project, false));
		REQUIRE(RunUntil(loop, watch, [&loop] { return loop.Project().phase == TBMPhase::ReleasePending || IsTBMProjectFinished(loop.Project().phase); }));
		CheckServedAtFace(watch, cursor);
		CHECK(SourceLoop::Aboard(loop.cutter) == SpoilTonnes{});
		// The cutter waits withdrawn while the hauler takes the last load away; release names that haulage.
		if (loop.Project().phase == TBMPhase::ReleasePending) {
			CHECK(GetTBMProjectReleaseProblem(loop.project) == STR_ERROR_TBM_RELEASE_HAULAGE);
			CHECK(GetTBMProjectSettlementBlocker(loop.project) == std::optional<VehicleID>{loop.hauler->index});
		}
		// Once the hauler has delivered and left, the project settles by itself.
		REQUIRE(RunUntil(loop, watch, [&loop] { return IsTBMProjectFinished(loop.Project().phase); }));
		CHECK(loop.Project().phase == TBMPhase::Cancelled);
	}
	SECTION("with no hauler at the face the loaded cutter releases after a bay exchange, as before") {
		ReachFaceWait(loop, watch, false);
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, loop.project, TBMProjectAction::Cancel).Succeeded());
		REQUIRE(RunUntil(loop, watch, [&loop] { return loop.Project().phase == TBMPhase::WaitToRelease; }));
		CHECK(IsTBMExchangeDue(loop.Project()));
		StartHauler(loop);
		REQUIRE(RunUntil(loop, watch, [&loop] { return IsTBMProjectFinished(loop.Project().phase); }));
		INFO(watch.Describe());
		CHECK(watch.face_transfers.empty());
		CHECK(watch.bay_exchanges.size() == 1);
		CHECK(loop.Project().phase == TBMPhase::Cancelled);
	}
	CHECK(loop.IsConserved());
}

/** Tasks 7.1, 7.2 and 7.6: a hangar order at the face wait goes on to the hangar once the spoil is gone. */
TEST_CASE("A hangar order from the face wait", "[demo-fix-face-lifecycle]")
{
	SourceLoop loop(LoopStart::Long);
	TBMFaceLoopWatch watch;
	/** @return Whether the cutter has set off for, or reached, its hangar. */
	const auto travelling = [&loop] { return loop.Project().phase == TBMPhase::TravelToHangar || loop.Project().phase == TBMPhase::ParkedHangar; };
	SECTION("the hauler on the stand takes the spoil first") {
		ReachFaceWait(loop, watch, true);
		const uint32_t cursor = loop.Job().cursor;
		REQUIRE(Command<Commands::SendTBMToHangar>::Do(DoCommandFlag::Execute, loop.cutter->index, loop.source, false).Succeeded());
		CHECK(loop.Project().phase == TBMPhase::ReturnForHangar);
		REQUIRE(RunUntil(loop, watch, travelling));
		CheckServedAtFace(watch, cursor);
		CHECK(SourceLoop::Aboard(loop.cutter) == SpoilTonnes{});
	}
	SECTION("with no hauler at the face the loaded cutter is emptied at the bay, as before") {
		ReachFaceWait(loop, watch, false);
		REQUIRE(Command<Commands::SendTBMToHangar>::Do(DoCommandFlag::Execute, loop.cutter->index, loop.source, false).Succeeded());
		CHECK(loop.Project().phase == TBMPhase::ReturnForHangar);
		REQUIRE(RunUntil(loop, watch, [&loop] { return loop.Project().phase == TBMPhase::WaitForHangar; }));
		StartHauler(loop);
		REQUIRE(RunUntil(loop, watch, travelling));
		INFO(watch.Describe());
		CHECK(watch.face_transfers.empty());
		CHECK(watch.bay_exchanges.size() == 1);
	}
	CHECK(loop.IsConserved());
}

/** Task 7.4: a change of work during the face wait keeps the wait, the cursor and the stand until the transfer. */
TEST_CASE("Changing the work area keeps the face wait", "[demo-fix-face-lifecycle]")
{
	SourceLoop loop(LoopStart::Long);
	TBMFaceLoopWatch watch;
	ReachFaceWait(loop, watch, true);
	const uint32_t cursor = loop.Job().cursor;
	REQUIRE(cursor + 1 < loop.Job().route.size());
	const auto stand = FindTBMFaceStand(loop.Project());
	REQUIRE(stand.has_value());
	// A shorter straight tunnel from the face the cutter works from, ending one cut earlier than planned.
	const TileIndex from = loop.Job().route[cursor - 1].cutter_position;
	const TileIndex to = loop.Job().route[loop.Job().route.size() - 2].cutter_position;
	const auto quote = PreviewTBMWorkChange(_current_company, loop.project, from, to, ExcavationPlanIntent::StraightTunnel);
	INFO("refusal string " << quote.result.GetErrorMessage().base() << ", plan error " << static_cast<int>(to_underlying(quote.plan.error)) << " at " << quote.plan.failure_tile.base());
	REQUIRE(quote.result.Succeeded());
	REQUIRE(CmdChangeTBMWork(DoCommandFlag::Execute, loop.project, from, to, quote.cursor, quote.fingerprint, ExcavationPlanIntent::StraightTunnel).Succeeded());
	CHECK(loop.Project().phase == TBMPhase::AwaitFaceTransfer);
	CHECK(loop.Job().cursor == cursor);
	CHECK(IsTBMExchangeDue(loop.Project())); // The breakpoint at the cursor stays: the wagons leave empty before the new cuts.
	CHECK(FindTBMFaceStand(loop.Project()) == stand);
	// The wait holds until the transfer, which then goes ahead at the face as before; the cutter then cuts on.
	bool left_early = false;
	REQUIRE(RunUntil(loop, watch, [&loop, &left_early] {
		left_early |= !IsTBMAwaitingFaceTransfer(loop.Project().phase) && loop.Cycle().exchanges == 0;
		return loop.Cycle().exchanges == 1;
	}));
	CHECK_FALSE(left_early);
	CHECK(watch.face_transfers == std::vector<uint32_t>{cursor});
	REQUIRE(RunUntil(loop, watch, [&loop] { return loop.Project().phase == TBMPhase::Bore; }));
	CHECK(loop.Job().cursor == cursor);
}

/** Tasks 7.3 and 7.4: a hauler in the corridor keeps release unsettled and the far exit unbuilt; gone, it no longer does. */
TEST_CASE("Settlement and the far exit wait for a hauler in the corridor", "[demo-fix-face-lifecycle]")
{
	SECTION("settlement") {
		TBMFaceTransferRig rig;
		CHECK(GetTBMProjectSettlementBlocker(rig.project.id) == std::optional<VehicleID>{rig.hauler->index});
		rig.PlaceInDepot(rig.hauler, rig.depot);
		CHECK(GetTBMProjectSettlementBlocker(rig.project.id) != std::optional<VehicleID>{rig.hauler->index});
	}
	SECTION("far exit") {
		TBMFaceRig rig(24);
		rig.Bore(static_cast<uint>(rig.Job().route.size()) - 1);
		rig.BoreOneCut();
		REQUIRE(rig.Job().state == ExcavationJobState::Completed);
		rig.Assign();
		const auto path = rig.Path();
		const TileIndex exit = path.back();
		const DiagDirection shaft = DiagdirBetweenTiles(path[path.size() - 2], exit);
		PlaceTrainOnPath(rig.hauler, path, path.size() / 2, path.size() / 2 - std::min(path.size() / 2, CountTrainParts(*rig.hauler) - 1));
		REQUIRE(FindTBMHaulerInCorridor(rig.Project()) == rig.hauler->index);
		CHECK(CheckTBMExitHandover(exit, shaft, RAILTYPE_RAIL).GetErrorMessage() == STR_ERROR_TBM_EXIT_HAULER);
		rig.PlaceInDepot(rig.hauler, rig.depot);
		CHECK(CheckTBMExitHandover(exit, shaft, RAILTYPE_RAIL).GetErrorMessage() != STR_ERROR_TBM_EXIT_HAULER);
	}
}

/** Task 7.5: a hauler crashed, stopped or offered for sale in the corridor goes through the existing fleet hooks. */
TEST_CASE("A hauler crashed, stopped or offered for sale in the corridor", "[demo-fix-face-lifecycle]")
{
	TBMFaceTransferRig rig; // The cutter, sent back to park, is held for the hauler on its stand.
	REQUIRE(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::Obstruction);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::Obstruction); // The controller records the hold, which the status line names.
	SECTION("stopped: the cutter's hold names it") {
		rig.hauler->vehstatus.Set(VehState::Stopped);
		CHECK(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::Obstruction);
		CHECK(FindTBMObstruction(rig.Project()).vehicle == rig.hauler->index);
	}
	SECTION("offered for sale: refused away from a depot, the cycle unchanged") {
		const auto before = TBMFaceSnapshot::Capture(rig);
		CHECK(Command<Commands::SellVehicle>::Do(DoCommandFlag::Execute, rig.hauler->index, true, false, ClientID::Invalid).Failed());
		CHECK(TBMFaceSnapshot::Capture(rig) == before);
	}
	SECTION("crashed: named while its wreck stands there; deleted, the cutter returns loaded to the bay") {
		for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->vehstatus.Set(VehState::Crashed);
		CHECK(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::Obstruction);
		CHECK(FindTBMObstruction(rig.Project()).vehicle == rig.hauler->index);
		CHECK_FALSE(IsTBMFaceTurnHolder(*GetTBMExchangeAssignmentForHauler(rig.hauler->index)));
		delete rig.hauler; // As the wreck's removal does.
		rig.hauler = nullptr;
		CHECK(CaptureTBMExchangeAssignments().empty());
		CHECK(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::None); // Nothing holds the return any more.
		CHECK(rig.Tick().blocked == TBMBlockedReason::None);
		CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::NoHauler); // The loaded cutter asks for a new hauler.
		CHECK(IsTBMExchangeDue(rig.Project())); // The spoil stays aboard until a hauler takes it.
	}
}
