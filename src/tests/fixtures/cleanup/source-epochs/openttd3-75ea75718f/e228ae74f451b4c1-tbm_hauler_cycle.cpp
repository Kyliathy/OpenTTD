/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_hauler_cycle.cpp One assigned hauler driven through its saved cycle (Plan 5 Group 6): each phase ends only
 * on an observed fact (location, wagon IDs, cargo), the exchange runs in the hauler's own step, and every intervention
 * is a recoverable hold with the cargo intact. The disposal itself is a stand-in until the receivers of Group 8.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 6
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"

#include "../safeguards.h"

/** A hauler goes round its whole cycle once, each phase ending on what the trains actually show. */
TEST_CASE("One hauler walks its whole saved cycle on observed facts", "[tbm-hauler-cycle]")
{
	TBMExchangeRig rig;
	const TileIndex origin = rig.MakeOriginStationTile();
	rig.Assign();
	const auto start = rig.StepHauler();
	CHECK(start.phase == TBMHaulerPhase::ReturnEmpties);
	CHECK(start.wait == TBMHaulerWait::Stopped); // Still stopped in its Surface depot, away from the bay.
	rig.HaulerToSourceBay();
	const auto arrived = rig.StepHauler();
	CHECK(arrived.phase == TBMHaulerPhase::AwaitExchange);
	CHECK(arrived.wait == TBMHaulerWait::None); // The same stop inside the bay is no hold.
	CHECK(IsTBMHaulerHeld(*rig.hauler));
	CHECK(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange); // The cutter is still boring: nothing is exchanged.
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == 0);

	const auto estimate = rig.BoreOneCut();
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
	const auto collected = rig.StepHauler(); // The exchange runs in the hauler's own step.
	CHECK(collected.phase == TBMHaulerPhase::CollectLoaded);
	CHECK(collected.exchanges == 1);
	const uint64_t spoil = estimate.earth_t + estimate.rock_t;
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == spoil);
	CHECK_FALSE(IsTBMHaulerHeld(*rig.hauler));
	CHECK(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded); // Still standing in the bay.

	rig.PlaceOnTrack(rig.hauler, TileXYL(25, 20, MapLayer::Surface));
	CHECK(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin);
	CHECK(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin);
	rig.PlaceOnTrack(rig.hauler, origin);
	const auto visited = rig.StepHauler();
	CHECK(visited.phase == TBMHaulerPhase::DisposeOrTransfer);
	CHECK(GetTBMWagonSet(visited.set)->visited_origin);
	CHECK(rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer); // Visiting the station consumed nothing.
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == spoil);

	for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->cargo.Truncate(); // Stand-in for Group 8 disposal.
	const auto emptied = rig.StepHauler();
	CHECK(emptied.phase == TBMHaulerPhase::ReturnEmpties);
	CHECK(GetTBMWagonSet(emptied.set)->state == TBMWagonSetState::ReturningEmpty);
	CHECK_FALSE(GetTBMWagonSet(emptied.set)->visited_origin); // A new loaded cycle must visit the origin again.
	rig.HaulerToSourceBay();
	CHECK(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
}

/** Every intervention holds the phase with its reason and the cargo intact, and clearing it resumes the same phase. */
TEST_CASE("Holds keep the phase and cargo until their cause is gone", "[tbm-hauler-cycle]")
{
	TBMExchangeRig rig;
	const TileIndex origin = rig.MakeOriginStationTile();
	const auto estimate = rig.BoreOneCut();
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
	rig.PlaceOnTrack(rig.hauler, TileXYL(25, 20, MapLayer::Surface));
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin);
	const uint64_t spoil = estimate.earth_t + estimate.rock_t;

	/**
	 * Step once and require a hold that keeps VisitOrigin and every tonne aboard.
	 * @param wait Expected hold.
	 */
	const auto held = [&](TBMHaulerWait wait) {
		const auto step = rig.StepHauler();
		CHECK(step.wait == wait);
		CHECK(step.phase == TBMHaulerPhase::VisitOrigin);
		CHECK(TBMExchangeRig::Aboard(rig.hauler) == spoil);
	};

	SECTION("the player stops the hauler on the way") {
		rig.hauler->vehstatus.Set(VehState::Stopped);
		held(TBMHaulerWait::Stopped);
		rig.hauler->vehstatus.Reset(VehState::Stopped);
		CHECK(rig.StepHauler().wait == TBMHaulerWait::None);
	}
	SECTION("the hauler crashes") {
		rig.hauler->vehstatus.Set(VehState::Crashed);
		held(TBMHaulerWait::Crashed);
	}
	SECTION("a wagon of the set leaves the hauler") {
		Train *last = rig.hauler->Last();
		last->Previous()->SetNext(nullptr);
		const auto step = rig.StepHauler();
		CHECK(step.wait == TBMHaulerWait::WrongWagons);
		CHECK(step.phase == TBMHaulerPhase::VisitOrigin);
		CHECK(TBMExchangeRig::Aboard(rig.hauler) + TBMExchangeRig::Aboard(last) == spoil); // The detached wagon keeps its load.
		rig.hauler->Last()->SetNext(last);
		CHECK(rig.StepHauler().wait == TBMHaulerWait::None);
	}
	SECTION("the origin is lost") {
		rig.fixture.origin->owner = OWNER_NONE;
		held(TBMHaulerWait::LostOrigin);
		const Train *spare = rig.BuildHauler(rig.depot, 1, 1);
		CHECK(AssignTBMHauler(rig.project.id, spare->index, true) == TBMExchangeError::MissingOrigin); // No new binding either.
		rig.fixture.origin->owner = _current_company;
		CHECK(rig.StepHauler().wait == TBMHaulerWait::None);
	}
	SECTION("a released loaded set is recovered where it was") {
		const TBMWagonSetID set = GetTBMExchangeAssignmentForHauler(rig.hauler->index)->set;
		REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
		CHECK(GetTBMWagonSet(set)->state == TBMWagonSetState::Loaded); // The spoil and its set stay where they are.
		CHECK(TBMExchangeRig::Aboard(rig.hauler) == spoil);
		rig.Assign();
		CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->set == set);
		CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->phase == TBMHaulerPhase::VisitOrigin);
		rig.PlaceOnTrack(rig.hauler, origin);
		REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer);
		// A visited set resumes at disposal; it does not have to visit the origin twice.
		REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
		rig.Assign();
		CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->phase == TBMHaulerPhase::DisposeOrTransfer);
	}
}

/** A hauler waiting stopped in the bay is not held, and the final return's spoil is still hauled after the project ends. */
TEST_CASE("The bay stop is no hold and a finished project's last load still travels", "[tbm-hauler-cycle]")
{
	TBMExchangeRig rig;
	rig.MakeOriginStationTile();
	rig.Meet(); // Arrives before the cut, still stopped as built.
	const auto waiting = rig.StepHauler();
	CHECK(waiting.phase == TBMHaulerPhase::AwaitExchange);
	CHECK(waiting.wait == TBMHaulerWait::None);
	rig.BoreOneCut(); // The project's own work ticks; the waiting hauler takes no step.
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
	REQUIRE(rig.Tick().phase == TBMPhase::Resume);
	REQUIRE(rig.Tick().phase == TBMPhase::Completed); // The single cut was the final return.
	rig.PlaceOnTrack(rig.hauler, TileXYL(25, 20, MapLayer::Surface));
	const auto travelling = rig.StepHauler();
	CHECK(travelling.wait == TBMHaulerWait::None);
	CHECK(travelling.phase == TBMHaulerPhase::VisitOrigin);
}
