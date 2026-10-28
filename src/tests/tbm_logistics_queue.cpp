/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native queue/custody ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23EC.
 * @file tbm_logistics_queue.cpp Two genuinely driven haulers preserve the source bay's FIFO turn through a real save.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13AC
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 7BB
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "underground_workflow_fixture.h"
#include "../tbm_cmd.h"
#include "../space_vehicle.h"
#include "../order_cmd.h"
#include "../saveload/saveload_func.h"
#include "../table/sprites.h"
#include <cstdlib>
#include <filesystem>

#include "../safeguards.h"

/** End only the fixture observation at its actual next queue transition; physical state remains untouched. */
struct ReachedQueueTransition {};

/** A short-empty first request retains its turn while a second real hauler waits; cancellation permits the same final atomic exchange. */
TEST_CASE("Two driven source haulers save an occupied FIFO turn and its next legal exchange", "[tbm-logistics-queue-save]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world(MapLayer::DeepUnderground, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	// A disk-ready Industry world includes the same default Space engine mapping as ordinary NewGRF finalization.
	// Register its actual definition before recording; no hull, order, inventory or purchase is created.
	REQUIRE(InitialiseSpaceEngineDefinition());
	std::vector<VehicleID> spare;
	uint earth = 0, rock = 0;
	// The first actual train brings a smaller empty set; unused initial units remain genuine owned depot wagons.
	for (const Train *unit = world.hauler->GetNextUnit(); unit != nullptr; unit = unit->GetNextUnit()) {
		const auto kind = GetSpoilReceiverKindForCargo(unit->cargo_type);
		// Capacity shortfall belongs to this declared scenario, while the cutter retains its complete accepted consist and route.
		if ((kind == SpoilReceiverKind::Earth && ++earth > 1) || (kind == SpoilReceiverKind::Rock && ++rock > 2)) spare.push_back(unit->index);
	}
	for (VehicleID id : spare) REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, id, VehicleID::Invalid(), false).Succeeded());
	const TileIndex depot = world.hauler->tile;
	Train *second = Train::From(world.BuyPreexisting(depot, 4));
	// Pay for the second complete ordinary hauler through the same native acquisition/kernel used by the initial scenario.
	for (const auto [model, count] : {std::pair<uint16_t, uint8_t>{2, 2}, {3, 4}}) {
		for (uint i = 0; i < count; ++i) {
			Vehicle *wagon = world.BuyPreexisting(depot, model);
			REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, wagon->index, second->Last()->index, false).Succeeded());
		}
	}
	Order order;
	order.MakeGoToStation(world.origin->index);
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, second->index, 0, order).Succeeded());
	const char *record_prefix = std::getenv("OTTD_TBM_LOGISTICS_RECORD");
	if (record_prefix != nullptr) {
		REQUIRE(std::filesystem::path(record_prefix).filename() == std::filesystem::path(record_prefix));
		REQUIRE_FALSE(std::string_view(record_prefix).starts_with("r2"));
	}
	bool assigned = false, occupied = false, cancelled = false, exchanged = false;
	TBMProjectID project_id{UINT32_MAX};
	uint32_t first_cycle = 0;
	uint32_t peer_cycle = 0; ///< The peer may already have served an earlier physical arrival; occupation must not steal its current turn.
	std::string journal, last; ///< Every change of both haulers' cycle and lead tile, so a missed checkpoint shows the real order of events.
	/** @param current Actual whole native frame; record the real second train's wait and the first train's preserved turn. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *first = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(first != nullptr);
		const auto *peer = GetTBMExchangeAssignmentForHauler(second->index);
		// Journal each change of either hauler's phase, wait, exchanges or lead tile, with the project's phase and cursor.
		const auto *work = GetTBMProject(first->project);
		const auto *job = work == nullptr ? nullptr : GetExcavationJob(work->job);
		std::string now = fmt::format("first {}/{}/{} at {} | second {} at {} | project {} cursor {}",
			to_underlying(first->phase), to_underlying(first->wait), first->exchanges, current.hauler->GetMovingFront()->tile.base(),
			peer == nullptr ? std::string{"unassigned"} : fmt::format("{}/{}/{}", to_underlying(peer->phase), to_underlying(peer->wait), peer->exchanges),
			second->GetMovingFront()->tile.base(), work == nullptr ? -1 : static_cast<int64_t>(to_underlying(work->phase)), job == nullptr ? -1 : static_cast<int64_t>(job->cursor));
		if (now != last) {
			journal += fmt::format("\n{}: {}", current.ticks, now);
			last = std::move(now);
		}
		// Both physical trains must be admitted at the source and the first must still own the turn; a staged queue index cannot establish occupation.
		if (peer != nullptr && !occupied && peer->wait == TBMHaulerWait::OccupiedBay) {
			REQUIRE(first->wait == TBMHaulerWait::NoEmpties);
			REQUIRE(first->phase == TBMHaulerPhase::AwaitExchange);
			REQUIRE(peer->phase == TBMHaulerPhase::AwaitExchange);
			REQUIRE(GetTBMExchangeTurn(first->bay)->hauler == current.hauler->index);
			CHECK(first->id < peer->id);
			project_id = first->project;
			first_cycle = first->exchanges;
			peer_cycle = peer->exchanges;
			occupied = true;
			if (record_prefix != nullptr) {
				// The explicit occupied-file check owns fresh disposable cache products; ordinary callers retain their existing recipe paths.
				const char *cache = std::getenv("OTTD_TBM_OCCUPIED_CACHE");
				const auto directory = cache == nullptr ? GetTestCaseArtifactPaths("tbm", "logistics").saves :
					RequireTestArtifactWithin(cache, FindTestRepositoryRoot() / "test-cache/merge-supply");
				REQUIRE(std::filesystem::is_directory(directory));
				// This ordinary-file regression must never smuggle the later phase20 format through its current reader.
				REQUIRE(to_underlying(work->phase) <= to_underlying(TBMPhase::FollowOrders));
				const auto path = directory / (std::string(record_prefix) + "-occupied-bay.sav");
				REQUIRE_FALSE(std::filesystem::exists(path));
				SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
				AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
			}
		}
		// The next legal queue event serves the existing first request after cancellation; the second cannot steal its loaded wagons.
		if (cancelled && first->exchanges == first_cycle + 1) {
			REQUIRE(peer != nullptr);
			CHECK(peer->exchanges == peer_cycle);
			exchanged = true;
			throw ReachedQueueTransition{};
		}
	};
	/** @param current Real player action boundary; assign/start the second train and later cancel remaining work through its canonical command. */
	const auto control = [&](const UndergroundWorkflowWorld &current) {
		const auto *first = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(first != nullptr);
		// The second hauler starts once the first is admitted at the bay. Both leave the same depot onto one unsignalled
		// block, so the native depot rule releases the second only after the first has left it; the declared FIFO order
		// is the first hauler's turn (master task 10R.1, after the r2df block boundary at the closed pair).
		if (!assigned && first->phase == TBMHaulerPhase::AwaitExchange) {
			REQUIRE(Command<Commands::AssignTBMHauler>::Do(DoCommandFlag::Execute, first->project, second->index).Succeeded());
			REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, second->index, true).Succeeded());
			assigned = true;
		}
		// Preserve the FIFO claim and all generated spoil while ending the unaffordable next pass; the owning exchange alone changes train chains.
		if (occupied && !cancelled) {
			REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project_id, TBMProjectAction::Cancel).Succeeded());
			cancelled = true;
		}
	};
	try {
		RunUndergroundExcavationWorkflow(world, Ticks::DAY_TICKS, observe, control);
		INFO("phase/wait/exchanges journal:" << journal);
		FAIL("the two-hauler scenario must reach its declared FIFO checkpoint");
	} catch (const ReachedQueueTransition &) {
		CHECK(assigned);
		CHECK(occupied);
		CHECK(cancelled);
		CHECK(exchanged);
	}
}
