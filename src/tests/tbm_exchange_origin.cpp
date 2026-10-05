/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange_origin.cpp The exchange at the commissioning origin (Plan 5 Group 4): the source-side bay driven
 * through the cutter's real phases, pauses without free rock or lost spoil when empties are short or the bay is
 * obstructed, the origin/receiver admission contract, and then the recurring underground depot bay.
 * Real receivers giving this acceptance are tested in spoil_receiver_intake.cpp; the full first-cut-to-disposal loop
 * on driven trains waits for master Group 7.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 4
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"

#include "../safeguards.h"

/** The cutter's return ends in its source bay; the exchange there lets it resume, and the final return completes it. */
TEST_CASE("The source-side exchange completes the cutter's return through its phases", "[tbm-exchange-origin]")
{
	TBMExchangeRig rig;
	const auto estimate = rig.BoreOneCut();
	REQUIRE(rig.Project().phase == TBMPhase::ReturnForExchange); // The cut filled the wagons for this pass.
	CHECK(rig.Tick().phase == TBMPhase::WaitForEmpties); // The consist already stands wholly in its source bay.
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	// Without a hauler the cutter waits: no rock opens and no spoil is lost.
	for (int tick = 0; tick < 5; ++tick) CHECK(rig.Tick().phase == TBMPhase::WaitForEmpties);
	rig.Meet();
	CHECK(rig.Tick().phase == TBMPhase::WaitForEmpties); // Reserved but not exchanged yet.
	CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == estimate.earth_t);
	CHECK(rig.Spoil(TBMConsistRole::RockWagon) == estimate.rock_t);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);

	REQUIRE(Command<Commands::ExchangeTBMWagons>::Do(DoCommandFlag::Execute, rig.project.id, rig.hauler->index).Succeeded());
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == estimate.earth_t + estimate.rock_t);
	CHECK(rig.Tick().phase == TBMPhase::Resume);
	CHECK(rig.Tick().phase == TBMPhase::Completed); // The single cut was also the final return.
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
}

/** Short empties and an obstructed bay pause the exchange; nothing opens, nothing is lost, and the cutter keeps waiting. */
TEST_CASE("Short empties and an obstructed source bay pause without loss", "[tbm-exchange-origin]")
{
	SECTION("too few empty Earth hoppers before more cutting") {
		TBMExchangeRig rig(2, 1, 2);
		const auto estimate = rig.BoreOneCut();
		rig.Meet();
		const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
		const auto cutter = GetTBMChainIDs(rig.front);
		const auto hauler = GetTBMChainIDs(rig.hauler);
		CHECK(DescribeTBMWagonExchange(rig.Request()).error == TBMExchangeError::InsufficientEmpties);
		CHECK(Command<Commands::ExchangeTBMWagons>::Do(DoCommandFlag::Execute, rig.project.id, rig.hauler->index).Failed());
		for (int tick = 0; tick < 5; ++tick) rig.Tick();
		CHECK(rig.Job().cursor == 1); // The held cutter opens no further rock.
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == estimate.earth_t);
		CHECK(rig.Spoil(TBMConsistRole::RockWagon) == estimate.rock_t);
		CHECK(GetTBMChainIDs(rig.front) == cutter);
		CHECK(GetTBMChainIDs(rig.hauler) == hauler);
	}
	SECTION("the hauler stands partly outside the bay") {
		TBMExchangeRig rig;
		rig.BoreOneCut();
		REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
		rig.Meet();
		rig.hauler->Last()->track = Track::X; // The last hopper is still on the approach.
		CHECK(DescribeTBMWagonExchange(rig.Request()).error == TBMExchangeError::NotAtBay);
		CHECK(Command<Commands::ExchangeTBMWagons>::Do(DoCommandFlag::Execute, rig.project.id, rig.hauler->index).Failed());
		CHECK(rig.Tick().phase == TBMPhase::WaitForEmpties);
		CHECK(TBMExchangeRig::Aboard(rig.hauler) == 0);
		// Once the whole hauler is in, the same exchange goes ahead.
		rig.hauler->Last()->track = Track::Depot;
		CHECK(Command<Commands::ExchangeTBMWagons>::Do(DoCommandFlag::Execute, rig.project.id, rig.hauler->index).Succeeded());
		CHECK(rig.Tick().phase == TBMPhase::Resume);
	}
	SECTION("a moving cutter is not in its bay") {
		TBMExchangeRig rig;
		rig.BoreOneCut();
		rig.front->cur_speed = 5;
		CHECK(rig.Tick().phase == TBMPhase::ReturnForExchange); // ReturnForExchange ends only once the consist is admitted.
		rig.front->cur_speed = 0;
		CHECK(rig.Tick().phase == TBMPhase::WaitForEmpties);
	}
}

/** Loaded spoil may leave only towards the company's Surface station, and only once real receivers accept both types. */
TEST_CASE("Origin admission needs the Surface station and receivers for both spoil types", "[tbm-exchange-origin]")
{
	TBMExchangeRig rig;
	Station *origin = rig.fixture.origin;
	CHECK(CheckTBMOriginAdmission(rig.Project()) == TBMExchangeError::NoReceiver); // No receiver stands in this catchment.
	const CargoType earth = GetCargoTypeByLabel(CargoLabel{"ERTH"});
	const CargoType rock = GetCargoTypeByLabel(CargoLabel{"ROCK"});
	REQUIRE(IsValidCargoType(earth));
	REQUIRE(IsValidCargoType(rock));
	// Acceptance is what a receiver in the station's catchment gives; one spoil type alone is not enough.
	origin->goods[earth].status.Set(GoodsEntry::State::Acceptance);
	CHECK(CheckTBMOriginAdmission(rig.Project()) == TBMExchangeError::NoReceiver);
	origin->goods[rock].status.Set(GoodsEntry::State::Acceptance);
	CHECK(CheckTBMOriginAdmission(rig.Project()) == TBMExchangeError::None);

	SECTION("another company's station") {
		origin->owner = OWNER_NONE;
		CHECK(CheckTBMOriginAdmission(rig.Project()) == TBMExchangeError::MissingOrigin);
	}
	SECTION("a station below ground") {
		origin->xy = TileXYL(18, 20, MapLayer::Underground);
		CHECK(CheckTBMOriginAdmission(rig.Project()) == TBMExchangeError::MissingOrigin);
	}
	SECTION("a station without rail") {
		origin->facilities.Reset(StationFacility::Train);
		CHECK(CheckTBMOriginAdmission(rig.Project()) == TBMExchangeError::MissingOrigin);
	}
}

/** With the source-side exchange working, the same contract serves a recurring excavated depot bay underground. */
TEST_CASE("The same exchange serves a recurring underground depot bay", "[tbm-exchange-origin]")
{
	TBMExchangeRig rig;
	const auto estimate = rig.BoreOneCut();
	const TileIndex depot = rig.BuildDepotBay(3);
	REQUIRE(Command<Commands::SetTBMExchangeBay>::Do(DoCommandFlag::Execute, rig.project.id, depot).Succeeded());
	// The source bay no longer admits the consist, so the return goes on until it reaches the depot.
	CHECK(rig.Tick().phase == TBMPhase::ReturnForExchange);
	Train *hauler = rig.BuildHauler(depot, 2, 2); // Built in the depot, as a hauler arriving there would wait.
	rig.PlaceAt(rig.front, *hauler); // Stands in for the drive back to the depot (master Group 7).
	CHECK(rig.Tick().phase == TBMPhase::WaitForEmpties);
	REQUIRE(AssignTBMHauler(rig.project.id, hauler->index, true) == TBMExchangeError::None);
	CHECK(GetTBMExchangeAssignmentAtBay(depot) != nullptr);
	const auto proposal = ExecuteTBMWagonExchange({rig.project.id, hauler->index});
	REQUIRE(proposal.error == TBMExchangeError::None);
	CHECK(proposal.bay == depot);
	CHECK(TBMExchangeRig::Aboard(hauler) == estimate.earth_t + estimate.rock_t);
	CHECK(GetTBMChainIDs(rig.front) == proposal.proposed_cutter);
	CHECK(rig.Tick().phase == TBMPhase::Resume);
}
