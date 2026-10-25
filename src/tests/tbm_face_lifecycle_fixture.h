/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file tbm_face_lifecycle_fixture.h Bounded real vehicle/receiver driving for face lifecycle regressions.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md original movement and custody owners.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Groups23D/23E.
 */
#ifndef TESTS_TBM_FACE_LIFECYCLE_FIXTURE_H
#define TESTS_TBM_FACE_LIFECYCLE_FIXTURE_H
#include "tbm_source_loop_fixture.h"
#include "tbm_face_loop_watch.h"
#include "../tbm_cmd.h"
#include "../vehicle_cmd.h"

/**
 * Run one game tick of a source loop and let the watch read it.
 * @param loop Loop to run.
 * @param watch What the face checks saw.
 */
inline void StepTBMFaceLifecycle(SourceLoop &loop, TBMFaceLoopWatch &watch)
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
inline bool RunTBMFaceLifecycleUntil(SourceLoop &loop, TBMFaceLoopWatch &watch, const std::function<bool()> &done, uint limit = 60000)
{
	const uint end = loop.ticks + limit;
	// Drive only native vehicle/receiver steps until actual settlement or a bounded stall; no pose or cargo is supplied.
	while (!done() && loop.ticks < end) StepTBMFaceLifecycle(loop, watch);
	INFO(watch.Describe());
	return done();
}

/**
 * Drive a long Standard loop to its first face wait, before any spoil has gone over.
 * @param loop Loop to drive.
 * @param watch What the face checks saw.
 * @param with_hauler Whether the hauler then stands on the stand; otherwise the player keeps it stopped in its depot.
 */
inline void ReachTBMFaceWait(SourceLoop &loop, TBMFaceLoopWatch &watch, bool with_hauler)
{
	if (!with_hauler) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, loop.hauler->index, false).Succeeded());
	/** @return Whether the real due face and, when required, the held directed hauler have both arrived. */
	REQUIRE(RunTBMFaceLifecycleUntil(loop, watch, [&loop, with_hauler] {
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
inline void StartTBMFaceHauler(SourceLoop &loop)
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
inline void CheckTBMServedAtFace(const TBMFaceLoopWatch &watch, uint32_t cursor)
{
	INFO(watch.Describe());
	CHECK(watch.face_transfers == std::vector<uint32_t>{cursor});
	CHECK(watch.bay_exchanges.empty());
	CHECK(watch.obstructed_ticks > 0);
	CHECK_FALSE(watch.moved_while_obstructed);
	CHECK_FALSE(watch.kept_heading_on_return); // The request backed the cutter at once (task 7.1).
}

#endif /* TESTS_TBM_FACE_LIFECYCLE_FIXTURE_H */
