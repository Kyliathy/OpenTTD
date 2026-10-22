/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_logistics_queue.cpp Two genuinely driven haulers preserve the source bay's FIFO turn through a real save.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13AC
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 7BB
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "underground_workflow_fixture.h"
#include "../tbm_cmd.h"
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
	/** @param current Actual whole native frame; record the real second train's wait and the first train's preserved turn. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *first = GetTBMExchangeAssignmentForHauler(current.hauler->index);
		REQUIRE(first != nullptr);
		const auto *peer = GetTBMExchangeAssignmentForHauler(second->index);
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
				const auto directory = std::filesystem::current_path().parent_path() / "src/tests/fixtures/haulage";
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
		if (!assigned) {
			const auto *first = GetTBMExchangeAssignmentForHauler(current.hauler->index);
			REQUIRE(first != nullptr);
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
		FAIL("the two-hauler scenario must reach its declared FIFO checkpoint");
	} catch (const ReachedQueueTransition &) {
		CHECK(assigned);
		CHECK(occupied);
		CHECK(cancelled);
		CHECK(exchanged);
	}
}
