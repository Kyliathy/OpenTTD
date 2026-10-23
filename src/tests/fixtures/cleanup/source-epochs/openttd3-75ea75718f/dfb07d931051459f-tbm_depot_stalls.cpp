/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_depot_stalls.cpp Real recurring-depot infrastructure/capacity waits, files and ordinary recovery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13C
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "depot_file_fixture.h"
#include "../station_cmd.h"
#include "../newgrf_station.h"
#include "../train_cmd.h"

#include "../safeguards.h"

/** Actual infrastructure loss and bounded intake retain the loaded depot wagons until supported recovery permits another exchange. */
TEST_CASE("Recurring depot haulage saves real origin loss receiver saturation and blocked track", "[tbm-depot-infrastructure-files]")
{
	const auto state = GENERATE(std::string{"lost-origin"}, std::string{"saturated-receiver"}, std::string{"blocked-line"});
	INFO("depot state=" << state);
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const TBMProjectID project = workflow.StartFurther();
	DepotFileRecorder recorder;
	const TileArea platform = workflow.world.origin->train_station;
	const TileIndex gap = TileXYL(TileX(workflow.world.rock) - 6, TilePlaneY(workflow.world.rock) + 2, MapLayer::Surface);
	REQUIRE(IsPlainRailTile(gap));
	const RailType railtype = GetRailType(gap);
	bool disrupted = false, saved = false, recovered = false;
	uint32_t cycle = 0;
	/** Native post-frame observer; commands apply only after actual loaded departure and after the first saved wait. */
	const auto observe = [&]() {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index);
		REQUIRE(assignment != nullptr);
		// Only a real loaded delivery may lose its origin or route; initial empty travel cannot stand in for cargo custody under failure.
		if (!disrupted && assignment->phase == TBMHaulerPhase::VisitOrigin) {
			if (state == "lost-origin") workflow.Do<Commands::RemoveFromRailStation>(platform.tile, TileAddWrap(platform.tile, platform.w - 1, platform.h - 1), true);
			if (state == "blocked-line") workflow.Do<Commands::RemoveRail>(gap, Track::Y);
			disrupted = true;
		}
		const TBMHaulerWait expected = state == "lost-origin" ? TBMHaulerWait::LostOrigin : state == "blocked-line" ? TBMHaulerWait::BlockedLine : TBMHaulerWait::SaturatedReceiver;
		// Capture precisely the native owner's wait with a real still-loaded set, before any repair or additional intake.
		if (!saved && assignment->wait == expected) {
			const auto *set = GetTBMWagonSet(assignment->set);
			REQUIRE(set != nullptr);
			REQUIRE(set->state == TBMWagonSetState::Loaded);
			uint tonnes = 0;
			for (VehicleID wagon : set->wagons) tonnes += Train::Get(wagon)->cargo.TotalCount(); // Every carried original wagon contributes once to the held spoil ledger.
			REQUIRE(tonnes > 0);
			cycle = assignment->exchanges;
			recorder.Save(state);
			if (state == "blocked-line") fmt::print("DEPOT_LINE_REPAIR tile={} track={}\n", gap.base(), to_underlying(Track::Y));
			saved = true;
			if (state == "lost-origin") {
				workflow.Do<Commands::BuildRailStation>(platform.tile, RAILTYPE_RAIL, Axis::Y, uint8_t{1}, static_cast<uint8_t>(platform.h), STAT_CLASS_DFLT, uint16_t{0}, StationID::Invalid(), false);
				CHECK(Station::GetByTile(platform.tile)->index == GetTBMProject(project)->origin);
			}
			if (state == "blocked-line") {
				workflow.Do<Commands::BuildRail>(gap, railtype, Track::Y, false);
				workflow.Do<Commands::ReverseTrainDirection>(workflow.world.hauler->index, false);
			}
		}
		// Native pathfinding/intake must clear the saved wait for the same cycle; a construction invoice alone does not prove recovery.
		if (saved && assignment->exchanges == cycle && assignment->wait == TBMHaulerWait::None) recovered = true;
	};
	/** @return Whether this exact saved delivery recovered and returned physically for the next depot exchange. */
	const auto resumed = [&]() { return saved && recovered && GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->exchanges > cycle; };
	workflow.DriveUntil(resumed, 50000, observe);
	CHECK(saved);
	CHECK(recovered);
}

/** Capacity changes happen only to real empty wagons in an ordinary depot; the registered set and its native refusal remain authoritative. */
TEST_CASE("Recurring depot haulage saves detached wagons and genuinely insufficient empties", "[tbm-depot-capacity-files]")
{
	const bool short_set = GENERATE(false, true);
	INFO("short depot set=" << short_set);
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	if (short_set) {
		WithdrawEmptyDepotHauler(workflow);
		std::vector<VehicleID> spare;
		uint earth = 0, rock = 0;
		// Declare one real hopper of each kind before assignment. Spare original empty units remain owned free wagons in the ordinary depot.
		for (const Train *unit = workflow.world.hauler->GetNextUnit(); unit != nullptr; unit = unit->GetNextUnit()) {
			const auto kind = GetSpoilReceiverKindForCargo(unit->cargo_type);
			if ((kind == SpoilReceiverKind::Earth && ++earth > 1) || (kind == SpoilReceiverKind::Rock && ++rock > 1)) spare.push_back(unit->index); // The declared short capacity cannot be repaired by changing the cutter's accepted consist.
		}
		for (VehicleID unit : spare) workflow.Do<Commands::MoveRailVehicle>(unit, VehicleID::Invalid(), false); // Retain actual detached parts without creating or destroying cargo.
	}
	const TBMProjectID project = workflow.StartFurther();
	DepotFileRecorder recorder;
	VehicleID detached = VehicleID::Invalid();
	bool saved = false;
	uint32_t cycle = 0;
	/** Observe the true mismatch/capacity hold, save it, then recover by ordinary recoupling or owning cancellation. */
	const auto observe = [&]() {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index);
		REQUIRE(assignment != nullptr);
		// Ordinary rearrangement requires every part inside the genuine depot. Stop the empty arrival before its first real exchange.
		if (!short_set && detached == VehicleID::Invalid() && assignment->phase == TBMHaulerPhase::AwaitExchange && workflow.world.hauler->track == Track::Depot) {
			workflow.Do<Commands::StartStopVehicle>(workflow.world.hauler->index, true);
			REQUIRE(workflow.world.hauler->IsStoppedInDepot());
			detached = workflow.world.hauler->Last()->index;
			workflow.Do<Commands::MoveRailVehicle>(detached, VehicleID::Invalid(), false);
		}
		const TBMHaulerWait expected = short_set ? TBMHaulerWait::NoEmpties : TBMHaulerWait::WrongWagons;
		if (saved || assignment->wait != expected) return; // One actual wait supplies the immutable input; later cycles cannot overwrite its original custody.
		cycle = assignment->exchanges;
		recorder.Save(short_set ? "no-empties" : "wrong-wagons");
		saved = true;
		if (short_set) {
			workflow.Do<Commands::ControlTBMProject>(project, TBMProjectAction::Cancel);
		} else {
			workflow.Do<Commands::MoveRailVehicle>(detached, workflow.world.hauler->Last()->index, false);
			workflow.Do<Commands::StartStopVehicle>(workflow.world.hauler->index, true);
		}
	};
	/** @return Whether the saved original set has undergone its next actual legal atomic depot exchange. */
	const auto resumed = [&]() { return saved && GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->exchanges > cycle; };
	workflow.DriveUntil(resumed, 50000, observe);
	CHECK(saved);
}
