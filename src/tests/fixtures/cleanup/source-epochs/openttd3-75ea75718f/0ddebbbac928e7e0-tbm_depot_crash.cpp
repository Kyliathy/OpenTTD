/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_depot_crash.cpp Actual loaded depot-hauler collision and native destruction receipts through file transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13C
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "depot_file_fixture.h"
#include "logistics_crash_fixture.h"
#include "../order_cmd.h"

#include "../safeguards.h"

/** Native collision is irreversible; the next legal transition records actual destruction rather than inventing another exchange or repaired train. */
TEST_CASE("A loaded depot hauler saves its real crash and native cleanup", "[tbm-depot-crash-file]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const TBMProjectID project_id = workflow.StartFurther();
	/** @return Whether the actual first exchanged loaded hauler has departed the genuine depot. */
	const auto departed = [&]() { return GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->phase == TBMHaulerPhase::VisitOrigin; };
	workflow.DriveUntil(departed);
	Train *opponent = Train::From(workflow.world.BuyPreexisting(workflow.depot, 4));
	const VehicleID opponent_id = opponent->index;
	const VehicleID hauler_id = workflow.world.hauler->index;
	Order order;
	order.MakeGoToStation(workflow.world.origin->index);
	order.SetNonStopType(OrderNonStopFlags{OrderNonStopFlag::GoVia, OrderNonStopFlag::NonStop});
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, opponent_id, 0, order).Succeeded());
	workflow.Do<Commands::StartStopVehicle>(opponent_id, true);
	workflow.Do<Commands::ForceTrainProceed>(opponent_id);
	uint frames = 0;
	// The player's ordinary force-proceed control makes unsafe following possible. Native movement/collision alone
	// decides whether the real unladen engine catches the slower loaded set; no pose, crash flag or victim is staged.
	while (GetTBMExchangeAssignmentForHauler(hauler_id)->wait != TBMHaulerWait::Crashed && frames < 15000) {
		workflow.world.Tick();
		REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
		++frames;
	}
	INFO("opponent=" << DescribeTrain(opponent));
	INFO("loaded hauler=" << DescribeTrain(workflow.world.hauler));
	REQUIRE(GetTBMExchangeAssignmentForHauler(hauler_id)->wait == TBMHaulerWait::Crashed);
	REQUIRE(opponent->vehstatus.Test(VehState::Crashed));
	LogisticsCrashLedger ledger(*GetTBMExchangeAssignmentForHauler(hauler_id));
	DepotFileRecorder recorder;
	recorder.Save("crashed");
	workflow.Do<Commands::ControlTBMProject>(project_id, TBMProjectAction::Pause);
	uint cleanup = 0;
	// Delayed native wagon deletion owns irreversible cleanup. The observer receipts each disappeared original unit once.
	while (!ledger.Observe() && cleanup < 6000) {
		workflow.world.Tick();
		++cleanup;
	}
	REQUIRE(cleanup < 6000);
	CHECK(Train::GetIfValid(opponent_id) == nullptr);
	CHECK(ledger.destroyed[0] + ledger.destroyed[1] > 0);
	const auto *project = GetTBMProject(project_id);
	REQUIRE(project != nullptr);
	const auto *job = GetExcavationJob(project->job);
	REQUIRE(job != nullptr);
	std::array<uint64_t, 2> held{};
	// Remaining real vehicle inventories, native receiver stock/receipts and explicit destroyed tonnes account for both paid jobs.
	for (const Vehicle *vehicle : Vehicle::Iterate()) {
		const auto kind = GetSpoilReceiverKindForCargo(vehicle->cargo_type);
		if (kind != SpoilReceiverKind::End) held[kind == SpoilReceiverKind::Earth ? 0 : 1] += vehicle->cargo.TotalCount();
	}
	const auto *earth = GetSpoilReceiver(workflow.world.earth->index);
	const auto *rock = GetSpoilReceiver(workflow.world.stone->index);
	CHECK(held[0] + ledger.destroyed[0] + earth->disposed_t + workflow.world.earth->accepted.front().waiting == workflow.first_earth + job->completed_earth_t);
	CHECK(held[1] + ledger.destroyed[1] + rock->disposed_t + rock->crushed_t + workflow.world.stone->accepted.front().waiting == workflow.first_rock + job->completed_rock_t);
	fmt::print("DEPOT_CRASH_PRODUCER collision_frames={} cleanup_frames={} destroyed_earth={} destroyed_rock={}\n", frames, cleanup, ledger.destroyed[0], ledger.destroyed[1]);
}
