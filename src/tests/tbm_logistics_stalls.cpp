/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_logistics_stalls.cpp Actual player-command logistics loss and recovery with native file transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13AB
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 7BB
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "underground_workflow_fixture.h"
#include "../saveload/saveload_func.h"
#include "../table/sprites.h"
#include "../tbm_cmd.h"
#include <cstdlib>
#include <filesystem>

#include "../safeguards.h"

/** Stop only the fixture's observation loop at a reached native transition; no gameplay state is changed. */
struct ReachedLogisticsCheckpoint {};

/** Remove a genuinely empty Surface origin while real loaded wagons travel there, then save the observed wait and rebuild by command. */
TEST_CASE("Loaded source haulage saves and recovers a removed Surface origin", "[tbm-logistics-origin-save]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world(MapLayer::DeepUnderground, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	const TileArea platform = world.origin->train_station;
	const StationID origin = world.origin->index;
	const char *record_prefix = std::getenv("OTTD_TBM_LOGISTICS_RECORD");
	if (record_prefix != nullptr) {
		REQUIRE(std::filesystem::path(record_prefix).filename() == std::filesystem::path(record_prefix));
		REQUIRE_FALSE(std::string_view(record_prefix).starts_with("r2"));
	}
	bool removed = false, observed = false, recovered = false;
	/** @param current Actual stepped world; snapshot the first real loss with its actual loaded packets. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(assignment != nullptr);
		// The wait must follow our real station removal, and recording happens once before any recovery command.
		if (!removed || observed || assignment->wait != TBMHaulerWait::LostOrigin) return;
		const auto *set = GetTBMWagonSet(assignment->set);
		REQUIRE(set != nullptr);
		REQUIRE(set->state == TBMWagonSetState::Loaded);
		uint tonnes = 0;
		// Every actual carried unit remains attached and keeps its spoil during infrastructure loss; no substitute inventory is counted.
		for (VehicleID id : set->wagons) {
			const Train *wagon = Train::Get(id);
			REQUIRE(wagon->First() == current.hauler);
			tonnes += wagon->cargo.TotalCount();
		}
		REQUIRE(tonnes > 0);
		observed = true;
		if (record_prefix == nullptr) return;
		const auto directory = std::filesystem::current_path().parent_path() / "src/tests/fixtures/haulage";
		const auto path = directory / (std::string(record_prefix) + "-lost-origin.sav");
		REQUIRE_FALSE(std::filesystem::exists(path));
		SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
		AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	};
	/** @param current Actual player action boundary; use only paid station commands and retain the complete loaded wagon set. */
	const auto control = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(assignment != nullptr);
		// Loaded departure leaves the Surface platform unoccupied, allowing ordinary demolition without touching a train or its cargo.
		if (!removed && assignment->phase == TBMHaulerPhase::VisitOrigin) {
			const TileIndex end = TileAddWrap(platform.tile, platform.w - 1, platform.h - 1);
			REQUIRE(Command<Commands::RemoveFromRailStation>::Do(DoCommandFlag::Execute, platform.tile, end, true).Succeeded());
			removed = true;
		}
		// Recover only after the native hauler owner reports the actual missing origin; rebuilding an unobserved loss would conceal a stalled-state defect.
		if (observed && !recovered) {
			// Ordinary station construction can reuse its nearby deleted sign. An explicit distant-join ID would
			// require the optional distant-join setting, even though this restoration is on the original footprint.
			const auto result = Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, platform.tile, RAILTYPE_RAIL, Axis::Y,
				uint8_t{1}, static_cast<uint8_t>(platform.h), STAT_CLASS_DFLT, uint16_t{0}, StationID::Invalid(), false);
			INFO("native origin rebuild error=" << result.GetErrorMessage().base());
			REQUIRE(result.Succeeded());
			CHECK(Station::GetByTile(platform.tile)->index == origin);
			CHECK(CheckTBMOriginAdmission(*GetTBMProject(assignment->project)) == TBMExchangeError::None);
			recovered = true;
		}
	};
	RunUndergroundExcavationWorkflow(world, Ticks::DAY_TICKS, observe, control);
	CHECK(removed);
	CHECK(observed);
	CHECK(recovered);
}

/** Reach the real per-interval intake limit with loaded spoil, save its wait, then recover solely through normal receiver ticks. */
TEST_CASE("Loaded source haulage saves bounded receiver saturation and physically resumes", "[tbm-logistics-receiver-save]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world(MapLayer::DeepUnderground, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	const char *record_prefix = std::getenv("OTTD_TBM_LOGISTICS_RECORD");
	if (record_prefix != nullptr) {
		REQUIRE(std::filesystem::path(record_prefix).filename() == std::filesystem::path(record_prefix));
		REQUIRE_FALSE(std::string_view(record_prefix).starts_with("r2"));
	}
	bool saturated = false, reopened = false, returned_empty = false;
	uint32_t stalled_cycle = 0;
	/** @param current Actual stepped scenario; real intake/processing alone determines saturation and recovery. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(assignment != nullptr);
		// Capture only the first actual lack of receiver room while spoil remains aboard; a phase name or a manufactured full stock is insufficient.
		if (!saturated && assignment->wait == TBMHaulerWait::SaturatedReceiver) {
			REQUIRE(assignment->phase == TBMHaulerPhase::DisposeOrTransfer);
			REQUIRE(assignment->exchanges > 0);
			const auto *set = GetTBMWagonSet(assignment->set);
			REQUIRE(set != nullptr);
			uint tonnes = 0;
			// Test the actual still-loaded cargo types against their real catchment receivers; spare room for an already empty type cannot recover this train.
			for (VehicleID id : set->wagons) {
				const Train *wagon = Train::Get(id);
				const uint carried = wagon->cargo.TotalCount();
				if (carried != 0) CHECK(GetSpoilReceiverRoomAt(*current.origin, wagon->cargo_type) == 0);
				tonnes += carried;
			}
			REQUIRE(tonnes > 0);
			saturated = true;
			stalled_cycle = assignment->exchanges;
			if (record_prefix != nullptr) {
				const auto directory = std::filesystem::current_path().parent_path() / "src/tests/fixtures/haulage";
				const auto path = directory / (std::string(record_prefix) + "-saturated-receiver.sav");
				REQUIRE_FALSE(std::filesystem::exists(path));
				SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
				AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
			}
		}
		// The same real load must regain intake without a command-side stock/timer edit, then complete actual disposal before empty return.
		if (saturated && assignment->exchanges == stalled_cycle && assignment->wait == TBMHaulerWait::None) reopened = true;
		// An empty-return phase after a different cycle would conceal a failed saturated delivery, so retain the original cycle identity here.
		if (reopened && assignment->exchanges == stalled_cycle && assignment->phase == TBMHaulerPhase::ReturnEmpties) returned_empty = true;
	};
	RunUndergroundExcavationWorkflow(world, Ticks::DAY_TICKS, observe);
	CHECK(saturated);
	CHECK(reopened);
	CHECK(returned_empty);
	CHECK(GetTBMExchangeAssignmentForHauler(world.hauler->index)->exchanges > stalled_cycle);
}

/** Ordinary empty-wagon rearrangement may temporarily separate an assigned set; the native WrongWagons hold preserves its exact identities. */
TEST_CASE("Assigned source haulage saves and recovers a detached empty wagon", "[tbm-logistics-wagons-save]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world(MapLayer::DeepUnderground, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	const VehicleID wagon = world.hauler->Last()->index;
	const char *record_prefix = std::getenv("OTTD_TBM_LOGISTICS_RECORD");
	if (record_prefix != nullptr) {
		REQUIRE(std::filesystem::path(record_prefix).filename() == std::filesystem::path(record_prefix));
		REQUIRE_FALSE(std::string_view(record_prefix).starts_with("r2"));
	}
	bool detached = false, observed = false, recovered = false;
	/** @param current Actual native step; save only the owner's observed mismatched set, not an invented wait enum. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(assignment != nullptr);
		// Ordinary depot movement must have separated the real wagon before the hauler owner reports its missing participant.
		if (!detached || observed || assignment->wait != TBMHaulerWait::WrongWagons) return;
		const auto *set = GetTBMWagonSet(assignment->set);
		REQUIRE(set != nullptr);
		REQUIRE(std::ranges::find(set->wagons, wagon) != set->wagons.end());
		REQUIRE(Train::Get(wagon)->First() != current.hauler);
		CHECK(Train::Get(wagon)->cargo.TotalCount() == 0);
		observed = true;
		if (record_prefix == nullptr) return;
		const auto directory = std::filesystem::current_path().parent_path() / "src/tests/fixtures/haulage";
		const auto path = directory / (std::string(record_prefix) + "-wrong-wagons.sav");
		REQUIRE_FALSE(std::filesystem::exists(path));
		SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
		AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	};
	/** @param current Player action boundary; detach and recouple the same empty unit through ordinary paid depot commands. */
	const auto control = [&](const UndergroundWorkflowWorld &current) {
		if (!detached) {
			REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, wagon, VehicleID::Invalid(), false).Succeeded());
			detached = true;
		}
		// Recoupling follows the saved native hold, preserving the recorded set identity and original wagon rather than buying replacement capacity.
		if (observed && !recovered) {
			REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, wagon, current.hauler->Last()->index, false).Succeeded());
			CHECK(Train::Get(wagon)->First() == current.hauler);
			recovered = true;
		}
	};
	RunUndergroundExcavationWorkflow(world, Ticks::DAY_TICKS, observe, control);
	CHECK(detached);
	CHECK(observed);
	CHECK(recovered);
}

/** A genuinely undersized empty set pauses the next pass; ordinary cancellation permits the final real spoil exchange and disposal. */
TEST_CASE("Source haulage saves short empties and cancels safely through its final exchange", "[tbm-logistics-empties-save]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world(MapLayer::DeepUnderground, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	std::vector<VehicleID> spare;
	uint earth = 0, rock = 0;
	// Declare genuinely smaller initial haulage through normal depot rearrangement, before assignment.
	// Extra empty units remain real owned free wagons; cutter capacity and its canonical pass quote remain unchanged.
	for (const Train *unit = world.hauler->GetNextUnit(); unit != nullptr; unit = unit->GetNextUnit()) {
		const auto kind = GetSpoilReceiverKindForCargo(unit->cargo_type);
		// This scenario brings one Earth and two Rock hoppers; every other actual initial unit is retained in its depot.
		if ((kind == SpoilReceiverKind::Earth && ++earth > 1) || (kind == SpoilReceiverKind::Rock && ++rock > 2)) spare.push_back(unit->index);
	}
	for (VehicleID id : spare) REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, id, VehicleID::Invalid(), false).Succeeded());
	const char *record_prefix = std::getenv("OTTD_TBM_LOGISTICS_RECORD");
	if (record_prefix != nullptr) {
		REQUIRE(std::filesystem::path(record_prefix).filename() == std::filesystem::path(record_prefix));
		REQUIRE_FALSE(std::string_view(record_prefix).starts_with("r2"));
	}
	bool observed = false, cancelled = false, exchanged = false;
	uint32_t stalled_cycle = 0;
	TBMProjectID project_id{UINT32_MAX};
	/** @param current Actual native step; record the genuine insufficient-capacity wait and stop after its real final exchange. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(assignment != nullptr);
		// The native exchange must refuse actual short empties while a real unfinished route and loaded cutter still need another pass.
		if (!observed && assignment->wait == TBMHaulerWait::NoEmpties) {
			const auto *project = GetTBMProject(assignment->project);
			const auto *job = GetExcavationJob(project->job);
			REQUIRE(job->cursor > 0);
			REQUIRE(job->cursor < job->route.size());
			REQUIRE(assignment->phase == TBMHaulerPhase::AwaitExchange);
			project_id = project->id;
			stalled_cycle = assignment->exchanges;
			observed = true;
			if (record_prefix != nullptr) {
				const auto directory = std::filesystem::current_path().parent_path() / "src/tests/fixtures/haulage";
				const auto path = directory / (std::string(record_prefix) + "-no-empties.sav");
				REQUIRE_FALSE(std::filesystem::exists(path));
				SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
				AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
			}
		}
		// Cancellation changes the future work obligation through its owner. Stop this partial-work fixture only
		// after the actual atomic final exchange, rather than applying full-area completion assertions to a cancelled project.
		if (cancelled && assignment->exchanges == stalled_cycle + 1) {
			exchanged = true;
			throw ReachedLogisticsCheckpoint{};
		}
	};
	/** @param current Real player command boundary; cancel the remaining job while preserving paid cuts, support and every generated packet. */
	const auto control = [&](const UndergroundWorkflowWorld &current) {
		if (observed && !cancelled) {
			REQUIRE(GetTBMExchangeAssignmentForHauler(current.hauler->index)->project == project_id);
			REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project_id, TBMProjectAction::Cancel).Succeeded());
			cancelled = true;
		}
	};
	try {
		RunUndergroundExcavationWorkflow(world, Ticks::DAY_TICKS, observe, control);
		FAIL("the short-empties scenario must reach its declared cancellation checkpoint");
	} catch (const ReachedLogisticsCheckpoint &) {
		CHECK(observed);
		CHECK(cancelled);
		CHECK(exchanged);
	}
	const auto *project = GetTBMProject(project_id);
	REQUIRE(project != nullptr);
	const auto *job = GetExcavationJob(project->job);
	REQUIRE(job != nullptr);
	CHECK(job->state == ExcavationJobState::Cancelled);
	uint remaining = 20000;
	// Run ordinary physical delivery after the final exchange; a cancelled floor never discards its already paid spoil.
	while (GetTBMExchangeAssignmentForHauler(world.hauler->index)->phase != TBMHaulerPhase::ReturnEmpties && remaining != 0) {
		world.Tick();
		--remaining;
	}
	REQUIRE(remaining != 0);
	CHECK(GetSpoilReceiver(world.earth->index)->disposed_t + world.earth->accepted.front().waiting == job->completed_earth_t);
	const auto *stone = GetSpoilReceiver(world.stone->index);
	CHECK(stone->disposed_t + stone->crushed_t + world.stone->accepted.front().waiting == job->completed_rock_t);
}

/** Legally interrupt the ordinary Surface haulage line, record its native pathfinding wait and repair it without touching train state. */
TEST_CASE("Loaded source haulage saves and recovers a blocked physical line", "[tbm-logistics-line-save]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	PathfinderDefaults pathfinder; // A real junction needs the normal native search budget; zero-initialized unit settings cannot establish a reachable route.
	AutoRestoreBackup turns(_settings_game.pf.forbid_90_deg, false); // Declared conventional freight fork; the cutter still obeys its separate 45-degree advancing contract.
	UndergroundWorkflowWorld world(MapLayer::DeepUnderground, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	// Native single-track following deliberately skips full route searches. A genuine paid fork on the declared
	// pre-existing Underground approach makes this scenario exercise ordinary pathfinding and its own lost reporting.
	// The dead-end branch opens no rock and belongs to the hauler's conventional network, outside the cutter pocket.
	const TileIndex junction = TileAddByDiagDir(TileAddByDiagDir(world.portal, DiagDirection::NW), DiagDirection::NW);
	const TileIndex branch = TileAddByDiagDir(junction, DiagDirection::SW);
	REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, junction, RAILTYPE_RAIL, Track::Lower, false).Succeeded());
	REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, branch, RAILTYPE_RAIL, Track::X, false).Succeeded());
	// This declared straight source line has two ordinary approach cells between its complete platform and the access portal.
	// Interrupt the second existing cell, away from both the protected cutter corridor and the loaded train's current parts.
	const TileIndex gap = TileAddWrap(world.origin->train_station.tile, 0, world.origin->train_station.h + 1);
	REQUIRE(IsPlainRailTile(gap));
	const RailType railtype = GetRailType(gap);
	const char *record_prefix = std::getenv("OTTD_TBM_LOGISTICS_RECORD");
	if (record_prefix != nullptr) {
		REQUIRE(std::filesystem::path(record_prefix).filename() == std::filesystem::path(record_prefix));
		REQUIRE_FALSE(std::string_view(record_prefix).starts_with("r2"));
	}
	bool interrupted = false, observed = false, repaired = false, recovered = false;
	uint32_t stalled_cycle = 0;
	/** @param current Actual native step; read the pathfinder's own lost flag and keep real loaded packets attached. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(assignment != nullptr);
		// Record only a wait produced by the genuine route interruption and ordinary pathfinding, once before rail repair.
		if (interrupted && !observed && assignment->wait == TBMHaulerWait::BlockedLine) {
			REQUIRE(current.hauler->vehicle_flags.Test(VehicleFlag::PathfinderLost));
			const auto *set = GetTBMWagonSet(assignment->set);
			REQUIRE(set != nullptr);
			REQUIRE(set->state == TBMWagonSetState::Loaded);
			uint tonnes = 0;
			// Account for each still-attached real unit; the route failure never moves cargo into a replacement buffer.
			for (VehicleID id : set->wagons) {
				const Train *wagon = Train::Get(id);
				REQUIRE(wagon->First() == current.hauler);
				tonnes += wagon->cargo.TotalCount();
			}
			REQUIRE(tonnes > 0);
			observed = true;
			stalled_cycle = assignment->exchanges;
			if (record_prefix != nullptr) {
				const auto directory = std::filesystem::current_path().parent_path() / "src/tests/fixtures/haulage";
				const auto path = directory / (std::string(record_prefix) + "-blocked-line.sav");
				REQUIRE_FALSE(std::filesystem::exists(path));
				SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
				AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
			}
		}
		// A construction invoice alone does not prove recovery: the native pathfinder and hauler owner must independently clear their actual waits.
		if (repaired && !current.hauler->vehicle_flags.Test(VehicleFlag::PathfinderLost) && assignment->wait == TBMHaulerWait::None) recovered = true;
	};
	/** @param current Actual player action boundary; remove and restore only an already built ordinary rail cell through its paid commands. */
	const auto control = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(assignment != nullptr);
		// Loaded departure supplies a genuine delivery obligation before the route is interrupted; an initially unreachable empty train cannot substitute.
		if (!interrupted && assignment->phase == TBMHaulerPhase::VisitOrigin) {
			REQUIRE_FALSE(current.hauler->vehicle_flags.Test(VehicleFlag::PathfinderLost));
			REQUIRE(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, gap, Track::Y).Succeeded());
			interrupted = true;
		}
		// Repair follows the saved native wait. Movement, heading, lost flags and project phases remain exclusively with their normal owners.
		if (observed && !repaired) {
			REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, gap, railtype, Track::Y, false).Succeeded());
			// Ordinary following of a single track keeps the previous lost advice until a new route search occurs.
			// The player's reverse command brings this actual train back through the paid fork for a fresh native decision;
			// the test never clears its lost flag or changes its position/order/assignment directly.
			REQUIRE(Command<Commands::ReverseTrainDirection>::Do(DoCommandFlag::Execute, current.hauler->index, false).Succeeded());
			repaired = true;
		}
	};
	RunUndergroundExcavationWorkflow(world, Ticks::DAY_TICKS, observe, control);
	CHECK(interrupted);
	CHECK(observed);
	CHECK(repaired);
	CHECK(recovered);
	CHECK(GetTBMExchangeAssignmentForHauler(world.hauler->index)->exchanges > stalled_cycle);
}
