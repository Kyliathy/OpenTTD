/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_depot_queue.cpp Two actual depot haulers preserve FIFO custody through save and ordinary cancellation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13C
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "depot_file_fixture.h"
#include "../order_cmd.h"

#include "../safeguards.h"

/** Genuine separate trains must physically arrive before an occupied depot queue can be recorded or resumed. */
TEST_CASE("Two depot haulers preserve a short-empty FIFO turn through an actual file", "[tbm-depot-queue-file]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	WithdrawEmptyDepotHauler(workflow);
	std::vector<VehicleID> spare;
	uint earth = 0, rock = 0;
	// The first request brings one real hopper of each kind. Its spare units stay owned and empty in the ordinary depot.
	for (const Train *unit = workflow.world.hauler->GetNextUnit(); unit != nullptr; unit = unit->GetNextUnit()) {
		const auto kind = GetSpoilReceiverKindForCargo(unit->cargo_type);
		if ((kind == SpoilReceiverKind::Earth && ++earth > 1) || (kind == SpoilReceiverKind::Rock && ++rock > 1)) spare.push_back(unit->index); // A real capacity shortfall retains the first request's turn.
	}
	for (VehicleID unit : spare) workflow.Do<Commands::MoveRailVehicle>(unit, VehicleID::Invalid(), false); // No empty part or cargo identity is deleted to create the short set.
	const TileIndex depot = workflow.world.hauler->tile;
	Train *second = Train::From(workflow.world.BuyPreexisting(depot, 4));
	const auto consist = GetDefaultTBMConsist(MapLayer::Underground);
	// A complete separately paid real hauler provides the queued peer; it cannot borrow the first request's recorded units.
	for (const auto [model, count] : {std::pair<uint16_t, uint8_t>{2, consist.earth_wagons}, {3, consist.rock_wagons}}) {
		for (uint i = 0; i < count; ++i) {
			Vehicle *wagon = workflow.world.BuyPreexisting(depot, model);
			workflow.Do<Commands::MoveRailVehicle>(wagon->index, second->Last()->index, false);
		}
	}
	Order order;
	order.MakeGoToStation(workflow.world.origin->index);
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, second->index, 0, order).Succeeded());
	const TBMProjectID project = workflow.StartFurther();
	workflow.Do<Commands::AssignTBMHauler>(project, second->index);
	workflow.Do<Commands::StartStopVehicle>(second->index, true);
	DepotFileRecorder recorder;
	bool saved = false;
	uint32_t first_cycle = 0, peer_cycle = 0;
	/** Save only the public FIFO owner's real occupation hold, then end further work through the canonical cancellation command. */
	const auto observe = [&]() {
		const auto *first = GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index);
		const auto *peer = GetTBMExchangeAssignmentForHauler(second->index);
		REQUIRE(first != nullptr);
		REQUIRE(peer != nullptr);
		if (saved || peer->wait != TBMHaulerWait::OccupiedBay) return; // Retain only the first real blocked peer; later queue cycles cannot overwrite its custody.
		REQUIRE(first->wait == TBMHaulerWait::NoEmpties);
		REQUIRE(first->phase == TBMHaulerPhase::AwaitExchange);
		REQUIRE(peer->phase == TBMHaulerPhase::AwaitExchange);
		REQUIRE(GetTBMExchangeTurn(first->bay)->hauler == workflow.world.hauler->index);
		CHECK(first->id < peer->id);
		first_cycle = first->exchanges;
		peer_cycle = peer->exchanges;
		recorder.Save("occupied-bay");
		saved = true;
		workflow.Do<Commands::ControlTBMProject>(project, TBMProjectAction::Cancel);
	};
	/** @return Whether the exact saved first request, rather than its peer, received the next atomic depot exchange. */
	const auto resumed = [&]() { return saved && GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->exchanges == first_cycle + 1; };
	workflow.DriveUntil(resumed, 50000, observe);
	CHECK(GetTBMExchangeAssignmentForHauler(second->index)->exchanges == peer_cycle);
	CHECK(saved);
}
