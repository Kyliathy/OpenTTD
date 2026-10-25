/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file tbm_face_regression.cpp Actual wait replanning and explicit corridor source-loss/far-exit faults.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical support, cargo and exit ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23E.
 * The replan is driven; fault cases declare their component poses/crash state and do not certify a driven arrival.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "tbm_face_lifecycle_fixture.h"
#include "../tbm_replanning.h"
#include "../tbm_cmd.h"
#include "../vehicle_cmd.h"
#include "../safeguards.h"

/** Task 7.4: a change of work during the face wait keeps the wait, the cursor and the stand until the transfer. */
TEST_CASE("Changing the work area keeps the face wait", "[tbm-face-regression]")
{
	SourceLoop loop(LoopStart::Long);
	TBMFaceLoopWatch watch;
	ReachTBMFaceWait(loop, watch, true);
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
	REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, /** @return Whether the real transfer occurred, recording any premature wait departure. */
	[&loop, &left_early] {
		left_early |= !IsTBMAwaitingFaceTransfer(loop.Project().phase) && loop.Cycle().exchanges == 0;
		return loop.Cycle().exchanges == 1;
	}));
	CHECK_FALSE(left_early);
	CHECK(watch.face_transfers == std::vector<uint32_t>{cursor});
	REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, /** @return Whether the same cutter physically rejoined its next cut. */
	[&loop] { return loop.Project().phase == TBMPhase::Bore; }));
	CHECK(loop.Job().cursor == cursor);
}

/** Tasks 7.3 and 7.4: a hauler in the corridor keeps release unsettled and the far exit unbuilt; gone, it no longer does. */
TEST_CASE("Settlement and the far exit wait for a hauler in the corridor", "[tbm-face-regression]")
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
TEST_CASE("A hauler crashed, stopped or offered for sale in the corridor", "[tbm-face-regression]")
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
		// Mark every real part as the declared crash fault; the native wreck-removal hooks own custody cleanup.
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
