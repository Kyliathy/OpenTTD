/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_logistics_crash.cpp Actual ordinary collision, retained spoil packets and native delayed cleanup.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13AC
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 7BB
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "underground_workflow_fixture.h"
#include "logistics_crash_fixture.h"
#include "../tbm_cmd.h"
#include "../order_cmd.h"
#include "../saveload/saveload_func.h"
#include "../table/sprites.h"
#include <cstdlib>
#include <filesystem>

#include "../safeguards.h"

/** End only the full-area fixture observation after an actual irreversible collision. */
struct ReachedNativeCollision {};

/** Drive two ordinary same-company trains into a real collision; neither poses nor crash bits are manufactured. */
TEST_CASE("A loaded source hauler saves its actual collision and native cleanup", "[tbm-logistics-crash-save]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world(MapLayer::DeepUnderground, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	Train *opponent = Train::From(world.BuyPreexisting(world.hauler->tile, 4));
	const VehicleID opponent_id = opponent->index;
	const VehicleID hauler_id = world.hauler->index;
	Order order;
	order.MakeGoToStation(world.origin->index);
	order.SetNonStopType(OrderNonStopFlags{OrderNonStopFlag::GoVia, OrderNonStopFlag::NonStop});
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, opponent_id, 0, order).Succeeded());
	const char *record_prefix = std::getenv("OTTD_TBM_LOGISTICS_RECORD");
	if (record_prefix != nullptr) {
		REQUIRE(std::filesystem::path(record_prefix).filename() == std::filesystem::path(record_prefix));
		REQUIRE_FALSE(std::string_view(record_prefix).starts_with("r2"));
	}
	bool launched = false;
	TBMProjectID project_id{UINT32_MAX};
	std::unique_ptr<LogisticsCrashLedger> ledger;
	/** @param current Actual physical frame; retain the first real collision wait with all spoil still attached. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(hauler_id);
		REQUIRE(assignment != nullptr);
		if (assignment->wait != TBMHaulerWait::Crashed) return;
		REQUIRE(launched);
		REQUIRE(opponent->vehstatus.Test(VehState::Crashed));
		REQUIRE(GetTileLayer(current.hauler->tile) != MapLayer::DeepUnderground); // The collision occurs on ordinary haulage track outside the active cutter pocket.
		project_id = assignment->project;
		ledger = std::make_unique<LogisticsCrashLedger>(*assignment);
		if (record_prefix != nullptr) {
			const auto directory = std::filesystem::current_path().parent_path() / "src/tests/fixtures/haulage";
			const auto path = directory / (std::string(record_prefix) + "-crashed.sav");
			REQUIRE_FALSE(std::filesystem::exists(path));
			SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
			AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
		}
		throw ReachedNativeCollision{};
	};
	/** @param current Player command boundary; start the opposing owned locomotive only after the first real loaded departure. */
	const auto control = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(assignment != nullptr);
		// A real loaded delivery obligation precedes the deliberate unsafe single-track movement; an empty startup crash proves no spoil custody.
		if (!launched && assignment->phase == TBMHaulerPhase::VisitOrigin) {
			REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, opponent_id, true).Succeeded());
			// The ordinary reservation protects this single-track line. The player's explicit force-proceed control
			// creates the unsafe movement; the native collision owner still decides the actual crash and victims.
			REQUIRE(Command<Commands::ForceTrainProceed>::Do(DoCommandFlag::Execute, opponent_id).Succeeded());
			launched = true;
		}
	};
	try {
		RunUndergroundExcavationWorkflow(world, Ticks::DAY_TICKS, observe, control);
		INFO("opponent=" << DescribeTrain(opponent));
		INFO("hauler=" << DescribeTrain(world.hauler));
		FAIL("the declared opposing trains must actually collide");
	} catch (const ReachedNativeCollision &) {
		REQUIRE(ledger != nullptr);
	}
	// Stop further generation through its owning control; an irreversible wreck has no repair/start or fictitious next exchange.
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project_id, TBMProjectAction::Pause).Succeeded());
	uint cleanup_ticks = 0;
	// Native animation and delayed one-wagon deletion own cleanup. The observer only checks surviving custody and records actual destruction.
	while (!ledger->Observe() && cleanup_ticks < 6000) {
		world.Tick();
		++cleanup_ticks;
	}
	REQUIRE(cleanup_ticks < 6000);
	CHECK(Train::GetIfValid(opponent_id) == nullptr);
	CHECK(ledger->destroyed[0] + ledger->destroyed[1] > 0);
	const auto *project = GetTBMProject(project_id);
	REQUIRE(project != nullptr);
	const auto *job = GetExcavationJob(project->job);
	REQUIRE(job != nullptr);
	std::array<uint64_t, 2> retained{};
	// The surviving cutter's actual packets remain part of the same conservation equation; cleanup creates no replacement cargo.
	for (const Train *unit = Train::Get(project->cutter); unit != nullptr; unit = unit->Next()) {
		const auto kind = GetSpoilReceiverKindForCargo(unit->cargo_type);
		if (kind != SpoilReceiverKind::End) retained[kind == SpoilReceiverKind::Earth ? 0 : 1] += unit->cargo.TotalCount();
	}
	const auto *earth = GetSpoilReceiver(world.earth->index);
	const auto *stone = GetSpoilReceiver(world.stone->index);
	CHECK(retained[0] + ledger->destroyed[0] + earth->disposed_t + world.earth->accepted.front().waiting == job->completed_earth_t);
	CHECK(retained[1] + ledger->destroyed[1] + stone->disposed_t + stone->crushed_t + world.stone->accepted.front().waiting == job->completed_rock_t);
	fmt::print("LOGISTICS_CRASH_PRODUCER ticks={} destroyed_earth={} destroyed_rock={}\n", cleanup_ticks, ledger->destroyed[0], ledger->destroyed[1]);
}
