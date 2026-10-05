/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange_bay.cpp The controlled exchange location (Plan 5 Group 1): the source and depot bays, admission of
 * whole stationary trains, the hauler/bay reservation, and the unchanged ordinary depot guard.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 1
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"
#include "../core/backup_type.hpp"

#include "../table/strings.h"

#include "../safeguards.h"

/** The source bay is the project's own portal; only whole, stationary company trains on it are admitted. */
TEST_CASE("The source bay admits whole stationary trains and refuses the rest", "[tbm-exchange-bay]")
{
	TBMExchangeRig rig;
	const auto bay = GetTBMExchangeBay(rig.Project());
	REQUIRE(bay.error == TBMExchangeError::None);
	CHECK(bay.kind == TBMExchangeBayKind::Source);
	CHECK(bay.tile == rig.fixture.request.portal);
	CHECK(bay.approach.empty());
	CHECK(CheckTBMExchangeParticipant(*rig.front, bay, _current_company) == TBMExchangeError::None);
	CHECK(CheckTBMExchangeParticipant(*rig.hauler, bay, _current_company) == TBMExchangeError::NotAtBay); // Still in its depot.
	rig.HaulerToSourceBay();
	CHECK(CheckTBMExchangeParticipant(*rig.hauler, bay, _current_company) == TBMExchangeError::None);
	// The rock-side housing of the same pair is never a bay.
	CHECK(ResolveTBMExchangeBay(rig.Project(), rig.proof.destination).error == TBMExchangeError::InvalidBay);

	SECTION("a moving train") {
		rig.hauler->cur_speed = 1;
		CHECK(CheckTBMExchangeParticipant(*rig.hauler, bay, _current_company) == TBMExchangeError::NotAtBay);
	}
	SECTION("a crashed train") {
		rig.hauler->vehstatus.Set(VehState::Crashed);
		CHECK(CheckTBMExchangeParticipant(*rig.hauler, bay, _current_company) == TBMExchangeError::NotAtBay);
	}
	SECTION("a train with a part outside the bay") {
		rig.hauler->Last()->track = Track::X;
		CHECK(CheckTBMExchangeParticipant(*rig.hauler, bay, _current_company) == TBMExchangeError::NotAtBay);
	}
	SECTION("another company's train") {
		CHECK(CheckTBMExchangeParticipant(*rig.hauler, bay, OWNER_NONE) == TBMExchangeError::InvalidHauler);
	}
	SECTION("a wagon is not a train") {
		CHECK(CheckTBMExchangeParticipant(*rig.hauler->Next(), bay, _current_company) == TBMExchangeError::InvalidHauler);
	}
	SECTION("an invalid bay admits nobody") {
		CHECK(CheckTBMExchangeParticipant(*rig.front, TBMExchangeBay{}, _current_company) == TBMExchangeError::InvalidBay);
	}
}

/** A depot bay is the company's excavated rock-layer depot with a straight approach that holds the cutter consist. */
TEST_CASE("A depot bay needs an excavated company depot with a fitting straight approach", "[tbm-exchange-bay]")
{
	TBMExchangeRig rig;
	const uint length = CeilDiv(rig.front->gcache.cached_total_length, TILE_SIZE);
	REQUIRE(length == 3);
	const TileIndex depot = rig.BuildDepotBay(length);
	const auto bay = ResolveTBMExchangeBay(rig.Project(), depot);
	REQUIRE(bay.error == TBMExchangeError::None);
	CHECK(bay.kind == TBMExchangeBayKind::Depot);
	CHECK(bay.tile == depot);
	CHECK(bay.approach == std::vector<TileIndex>{TileXYL(41, 40, MapLayer::Underground), TileXYL(42, 40, MapLayer::Underground), TileXYL(43, 40, MapLayer::Underground)});

	SECTION("the bay command moves the exchange there and back") {
		REQUIRE(Command<Commands::SetTBMExchangeBay>::Do(DoCommandFlag::Execute, rig.project.id, depot).Succeeded());
		CHECK(rig.Project().exchange_bay == depot);
		CHECK(GetTBMExchangeBay(rig.Project()).kind == TBMExchangeBayKind::Depot);
		CHECK(ValidateTBMProject(rig.Project()) == TBMProjectError::None);
		CHECK(FindTBMProjectReferenceLoss(rig.Project()) == TBMBlockedReason::None);
		CHECK(Command<Commands::SetTBMExchangeBay>::Do(DoCommandFlag::Execute, rig.project.id, depot).Failed()); // Already there.
		REQUIRE(Command<Commands::SetTBMExchangeBay>::Do(DoCommandFlag::Execute, rig.project.id, rig.fixture.request.portal).Succeeded());
		CHECK(GetTBMExchangeBay(rig.Project()).kind == TBMExchangeBayKind::Source);
	}
	SECTION("a reserved bay cannot move") {
		rig.Assign();
		CHECK(Command<Commands::SetTBMExchangeBay>::Do(DoCommandFlag::Execute, rig.project.id, depot).Failed());
		CHECK(rig.Project().exchange_bay == rig.fixture.request.portal);
	}
	SECTION("an approach one tile short") {
		REQUIRE(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, TileXYL(43, 40, MapLayer::Underground), Track::X).Succeeded());
		CHECK(ResolveTBMExchangeBay(rig.Project(), depot).error == TBMExchangeError::InvalidBay);
		CHECK(Command<Commands::SetTBMExchangeBay>::Do(DoCommandFlag::Execute, rig.project.id, depot).Failed());
	}
	SECTION("an approach tile of another owner") {
		SetTileOwner(TileXYL(42, 40, MapLayer::Underground), OWNER_TOWN);
		CHECK(ResolveTBMExchangeBay(rig.Project(), depot).error == TBMExchangeError::InvalidBay);
	}
	SECTION("plain track is not a bay") {
		CHECK(ResolveTBMExchangeBay(rig.Project(), TileXYL(41, 40, MapLayer::Underground)).error == TBMExchangeError::InvalidBay);
	}
	SECTION("a Surface depot is not a rock-layer bay") {
		CHECK(ResolveTBMExchangeBay(rig.Project(), rig.depot).error == TBMExchangeError::InvalidBay);
	}
}

/** Each hauler has one assignment, which registers the empty set it brings; several haulers queue for one bay. */
TEST_CASE("Assignments reserve haulers and their empty sets and queue for the bay", "[tbm-exchange-bay]")
{
	TBMExchangeRig rig;
	CHECK(AssignTBMHauler(rig.project.id, rig.hauler->index, false) == TBMExchangeError::None);
	CHECK(CaptureTBMExchangeAssignments().empty()); // The check changes nothing.
	CHECK(CaptureTBMWagonSets().empty());
	REQUIRE(Command<Commands::AssignTBMHauler>::Do(DoCommandFlag::Execute, rig.project.id, rig.hauler->index).Succeeded());
	const auto *assignment = GetTBMExchangeAssignmentForHauler(rig.hauler->index);
	REQUIRE(assignment != nullptr);
	CHECK(assignment->project == rig.project.id);
	CHECK(assignment->bay == rig.fixture.request.portal);
	CHECK(assignment->phase == TBMHaulerPhase::ReturnEmpties); // A new hauler first brings its empties to the bay.
	CHECK(assignment->exchanges == 0);
	CHECK(GetTBMExchangeAssignment(rig.project.id) == assignment);
	CHECK(GetTBMExchangeAssignmentAtBay(rig.fixture.request.portal) == assignment);
	const TBMWagonSetID first_set = assignment->set;
	const auto *set = GetTBMWagonSet(first_set);
	REQUIRE(set != nullptr);
	const TBMChainIDs hauler = GetTBMChainIDs(rig.hauler);
	CHECK(set->wagons == TBMChainIDs(hauler.begin() + 1, hauler.end()));
	CHECK(set->state == TBMWagonSetState::ReturningEmpty);
	CHECK(set->project == rig.project.id);
	CHECK(GetTBMWagonSetForWagon(rig.hauler->Next()->index) == set);
	CHECK(AssignTBMHauler(rig.project.id, rig.hauler->index, true) == TBMExchangeError::Reserved); // One assignment per hauler.

	// A second hauler queues for the same bay behind the first, in accepted request order, with its own set.
	Train *second = rig.BuildHauler(rig.depot, 2, 2);
	REQUIRE(AssignTBMHauler(rig.project.id, second->index, true) == TBMExchangeError::None);
	CHECK(GetTBMExchangeAssignment(rig.project.id)->hauler == rig.hauler->index);
	CHECK(GetTBMExchangeAssignmentForHauler(second->index)->set != first_set);
	CHECK(CaptureTBMWagonSets().size() == 2);

	// The cutter, a wagon, an unknown project and another company's actor are all refused before anything is stored.
	CHECK(AssignTBMHauler(rig.project.id, rig.front->index, true) == TBMExchangeError::InvalidHauler);
	CHECK(AssignTBMHauler(rig.project.id, second->Next()->index, true) == TBMExchangeError::InvalidHauler);
	CHECK(AssignTBMHauler(TBMProjectID{99}, second->index, true) == TBMExchangeError::InvalidProject);
	{
		Backup<CompanyID> actor(_current_company, OWNER_NONE);
		CHECK(AssignTBMHauler(rig.project.id, second->index, true) == TBMExchangeError::InvalidProject);
		CHECK(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::NotAssigned); // Not this actor's project.
		actor.Restore();
	}
	CHECK(CaptureTBMExchangeAssignments().size() == 2);

	// Cancellation ends the assignment only; the empty set stays registered and a new assignment recovers it.
	REQUIRE(Command<Commands::ReleaseTBMHauler>::Do(DoCommandFlag::Execute, rig.hauler->index).Succeeded());
	CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index) == nullptr);
	CHECK(GetTBMWagonSet(first_set)->state == TBMWagonSetState::Available);
	CHECK(GetTBMChainIDs(rig.hauler) == hauler);
	CHECK(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::NotAssigned);
	REQUIRE(AssignTBMHauler(rig.project.id, rig.hauler->index, true) == TBMExchangeError::None);
	CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->set == first_set);
	CHECK(GetTBMWagonSet(first_set)->state == TBMWagonSetState::ReturningEmpty);
	CHECK(GetTBMExchangeAssignment(rig.project.id)->hauler == second->index); // The returning hauler now queues last.
}

/** Ordinary moves still need a genuine depot: a bay where the trains only stand in the depot-like state refuses them. */
TEST_CASE("Ordinary moves keep their genuine-depot guard at a bay", "[tbm-exchange-bay]")
{
	TBMExchangeRig rig;
	rig.HaulerToSourceBay();
	rig.front->vehstatus.Set(VehState::Stopped); // Both trains now pass the depot-like stopped check on the portal tile.
	REQUIRE(rig.front->IsStoppedInDepot());
	REQUIRE(rig.hauler->IsStoppedInDepot());
	const auto cutter = GetTBMChainIDs(rig.front);
	const auto hauler = GetTBMChainIDs(rig.hauler);
	const auto onto_cutter = Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, rig.hauler->Next()->index, rig.front->Last()->index, false);
	CHECK(onto_cutter.GetErrorMessage() == STR_ERROR_TRAINS_CAN_ONLY_BE_ALTERED_INSIDE_A_DEPOT);
	const auto off_cutter = Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, rig.front->Last()->index, VehicleID::Invalid(), false);
	CHECK(off_cutter.GetErrorMessage() == STR_ERROR_TRAINS_CAN_ONLY_BE_ALTERED_INSIDE_A_DEPOT);
	CHECK(GetTBMChainIDs(rig.front) == cutter);
	CHECK(GetTBMChainIDs(rig.hauler) == hauler);

	// In a genuine depot the same ordinary move still works.
	Train *other = rig.BuildHauler(rig.depot, 1, 1);
	const VehicleID last = other->Last()->index;
	REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, last, VehicleID::Invalid(), false).Succeeded());
	CHECK(Train::Get(last)->IsFreeWagon());
}
