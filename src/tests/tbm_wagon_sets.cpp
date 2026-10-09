/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_wagon_sets.cpp Wagon sets queued independently of train heads (Plan 5 Group 7): a set keeps its identity
 * and wagon order when a genuine depot moves it to another locomotive, belongs to at most one hauler, must be wholly
 * aboard to be recovered, and several haulers take one bay in accepted request order. Queue reload waits for the
 * assignment saves of Group 10.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 7
 * @see docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-h-exchange-design.md, Wagon sets and queue
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"

#include "../safeguards.h"

/**
 * Move one wagon, or it and every part behind it, to the end of another train through the ordinary depot move.
 * @param wagon Unit to move.
 * @param onto Train whose end it joins.
 * @param chain Whether the parts behind the wagon go with it.
 */
static void MoveWagons(const Train *wagon, const Train *onto, bool chain)
{
	REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, wagon->index, onto->Last()->index, chain).Succeeded());
}

/** After a genuine depot moves a loaded set between locomotives, the set's identity follows its wagons, not a head. */
TEST_CASE("A loaded set keeps its identity across a change of locomotive", "[tbm-wagon-sets]")
{
	TBMExchangeRig rig;
	const TileIndex origin = rig.MakeOriginStationTile();
	const auto estimate = rig.BoreOneCut();
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
	const TBMWagonSetID set = GetTBMExchangeAssignmentForHauler(rig.hauler->index)->set;
	const auto wagons = GetTBMWagonSet(set)->wagons;
	const uint64_t spoil = estimate.earth_t + estimate.rock_t;

	// The loaded hauler is taken into a genuine depot, where a bare locomotive waits to take its wagons.
	rig.PlaceInDepot(rig.hauler, rig.depot);
	Train *other = rig.BuildHauler(rig.depot, 0, 0);

	SECTION("the whole set moves to the other locomotive") {
		MoveWagons(rig.hauler->Next(), other, true);
		CHECK(TBMExchangeRig::Aboard(other) == spoil);
		const auto stranded = rig.StepHauler(); // The old head still holds the assignment but no longer carries its set.
		CHECK(stranded.wait == TBMHaulerWait::WrongWagons);
		CHECK(stranded.phase == TBMHaulerPhase::CollectLoaded);
		// The set belongs to its one carrier until that assignment ends.
		CHECK(AssignTBMHauler(rig.project.id, other->index, true) == TBMExchangeError::Reserved);
		REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
		REQUIRE(AssignTBMHauler(rig.project.id, other->index, true) == TBMExchangeError::None);
		const auto *adopted = GetTBMExchangeAssignmentForHauler(other->index);
		CHECK(adopted->set == set);
		CHECK(adopted->phase == TBMHaulerPhase::VisitOrigin); // Still loaded and not yet at the origin.
		CHECK(GetTBMWagonSet(set)->wagons == wagons); // Same wagon IDs in the same order.
		CHECK(GetTBMWagonSet(set)->state == TBMWagonSetState::Loaded);
		CHECK(GetTBMWagonSetForWagon(other->Next()->index)->id == set);
		CHECK(CaptureTBMWagonSets().size() == 2); // No set was made or lost by the head change.
		// The new head continues the same cycle.
		rig.PlaceOnTrack(other, origin);
		CHECK(TBMExchangeRig::StepHauler(*other).phase == TBMHaulerPhase::DisposeOrTransfer);
		CHECK(GetTBMWagonSet(set)->visited_origin);
	}
	SECTION("only part of the set moves") {
		Train *split = rig.hauler->Last();
		MoveWagons(split, other, false);
		CHECK(rig.StepHauler().wait == TBMHaulerWait::WrongWagons);
		REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
		// A split set has no single carrier, so neither locomotive can recover it.
		CHECK(AssignTBMHauler(rig.project.id, other->index, true) == TBMExchangeError::WrongWagons);
		CHECK(AssignTBMHauler(rig.project.id, rig.hauler->index, true) == TBMExchangeError::WrongWagons);
		CHECK(GetTBMWagonSet(set)->state == TBMWagonSetState::Loaded);
		CHECK(CaptureTBMExchangeAssignments().empty());
		// Rejoined, the set is whole again and its old head recovers it where it was.
		MoveWagons(split, rig.hauler, false);
		TBMChainIDs whole{rig.hauler->index};
		whole.insert(whole.end(), wagons.begin(), wagons.end()); // Each hopper is one part, so the set is the chain's tail.
		CHECK(GetTBMChainIDs(rig.hauler) == whole);
		CHECK(GetTBMWagonSet(set)->wagons == wagons);
		REQUIRE(AssignTBMHauler(rig.project.id, rig.hauler->index, true) == TBMExchangeError::None);
		CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->phase == TBMHaulerPhase::VisitOrigin);
	}
}

/** Two haulers at one bay: one active exchange at a time, in accepted request order, each with its own set. */
TEST_CASE("Haulers queued at one bay exchange in request order", "[tbm-wagon-sets]")
{
	TBMExchangeRig rig;
	const auto estimate = rig.BoreOneCut();
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
	rig.Meet(); // The first request.
	Train *second = rig.BuildHauler(rig.depot, 2, 2);
	rig.PlaceAt(second, *rig.front);
	REQUIRE(AssignTBMHauler(rig.project.id, second->index, true) == TBMExchangeError::None);
	const TileIndex bay = rig.fixture.request.portal;

	// The later request steps first, but the bay's turn follows request order.
	REQUIRE(TBMExchangeRig::StepHauler(*second).phase == TBMHaulerPhase::AwaitExchange);
	CHECK(GetTBMExchangeTurn(bay)->hauler == rig.hauler->index);
	const auto queued = TBMExchangeRig::StepHauler(*second);
	CHECK(queued.phase == TBMHaulerPhase::AwaitExchange);
	CHECK(queued.wait == TBMHaulerWait::OccupiedBay);
	CHECK(queued.exchanges == 0);
	CHECK(TBMExchangeRig::Aboard(second) == 0);

	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == estimate.earth_t + estimate.rock_t);
	CHECK(GetTBMExchangeTurn(bay)->hauler == second->index); // A hauler carrying spoil has had its turn.

	// Stand-in for the cutter's next cut into the empties it was just given (the drive back to the face is master Group 7).
	rig.Preload(rig.Wagons(TBMConsistRole::EarthWagon).front(), 10);
	rig.Preload(rig.Wagons(TBMConsistRole::RockWagon).back(), 30);
	const auto served = TBMExchangeRig::StepHauler(*second);
	CHECK(served.phase == TBMHaulerPhase::CollectLoaded);
	CHECK(served.wait == TBMHaulerWait::None);
	CHECK(served.exchanges == 1);
	CHECK(TBMExchangeRig::Aboard(second) == 40);
	CHECK(GetTBMExchangeTurn(bay) == nullptr); // Both queued haulers now carry spoil.

	// Three sets: one on each hauler and the second hauler's former empties on the cutter, no wagon in two of them.
	const auto sets = CaptureTBMWagonSets();
	CHECK(sets.size() == 3);
	std::vector<VehicleID> seen;
	for (const auto &set : sets) {
		for (VehicleID wagon : set.wagons) {
			CHECK(std::ranges::find(seen, wagon) == seen.end());
			seen.push_back(wagon);
		}
	}
	CHECK(GetTBMWagonSet(served.set)->state == TBMWagonSetState::Loaded);
	CHECK(GetTBMWagonSetForWagon(rig.front->Last()->index)->state == TBMWagonSetState::Assigned);
}

/** A returning set must really be empty before its hauler may wait for the exchange; its cargo is never discarded. */
TEST_CASE("A returning set with cargo aboard cannot await the exchange", "[tbm-wagon-sets]")
{
	TBMExchangeRig rig;
	rig.BoreOneCut();
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
	rig.Meet();
	rig.Preload(rig.hauler->Next(), 5); // Left over from an incomplete disposal.
	const auto blocked = rig.StepHauler();
	CHECK(blocked.phase == TBMHaulerPhase::ReturnEmpties);
	CHECK(blocked.wait == TBMHaulerWait::None);
	CHECK(DescribeTBMWagonExchange(rig.Request()).error == TBMExchangeError::WrongWagons);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == 5);
	rig.hauler->Next()->cargo.Truncate(); // Stand-in for finishing the disposal.
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	CHECK(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
}
