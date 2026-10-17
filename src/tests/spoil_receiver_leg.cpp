/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file spoil_receiver_leg.cpp The mandatory Surface origin leg (Plan 5 Groups 8.1 and 8.4): spoil is staged for
 * unloading only from the hauler carrying its loaded set, at the project's origin or after visiting it; never from
 * the cutter; and a replaced origin must again be a company Surface railway station, keeping earlier visits.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 8
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-i-receiver-design.md, Surface origin leg
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"
#include "../core/backup_type.hpp"

#include "../safeguards.h"

/**
 * Create a bare station record for origin checks.
 * @param tile Sign tile.
 * @param owner Owner.
 * @param rail Whether it has a railway facility.
 * @return The station.
 */
static Station *MakeStation(TileIndex tile, Owner owner, bool rail)
{
	REQUIRE(Station::CanAllocateItem());
	Station *station = Station::Create(tile);
	station->owner = owner;
	if (rail) station->facilities.Set(StationFacility::Train);
	return station;
}

/** The cutter keeps its spoil; a loaded set unloads only from its carrier, at the origin or after its visit. */
TEST_CASE("Spoil leaves its wagons only from its own hauler at or after the origin", "[spoil-receiver-leg]")
{
	SpoilReceiverRig rig;
	const StationID origin = rig.fixture.origin->index;
	const StationID elsewhere = MakeStation(TileXYL(30, 22, MapLayer::Surface), _current_company, true)->index;
	rig.BoreOneCut();
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
	const Train *cutter_wagon = rig.Wagons(TBMConsistRole::EarthWagon).front();
	REQUIRE(cutter_wagon->cargo.TotalCount() != 0);
	CHECK_FALSE(MayTBMSpoilLeaveVehicle(*rig.front, *cutter_wagon, origin)); // The cutter cannot haul its own spoil.

	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
	const Train *spoil = rig.hauler->Next();
	REQUIRE(spoil->cargo.TotalCount() != 0);
	CHECK_FALSE(MayTBMSpoilLeaveVehicle(*rig.hauler, *spoil, elsewhere)); // Not before the origin.
	CHECK(MayTBMSpoilLeaveVehicle(*rig.hauler, *spoil, origin)); // At the origin itself.
	CHECK(MayTBMSpoilLeaveVehicle(*rig.hauler, *rig.hauler, elsewhere)); // The locomotive carries no spoil.
	CHECK_FALSE(MayTBMSpoilLeaveVehicle(*rig.front, *rig.front->Last(), origin)); // The cutter's new empties stay bound to it.

	rig.PlaceOnTrack(rig.hauler, TileXYL(25, 20, MapLayer::Surface));
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin);
	rig.PlaceOnTrack(rig.hauler, rig.origin_tile);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer);
	CHECK(MayTBMSpoilLeaveVehicle(*rig.hauler, *spoil, elsewhere)); // After the visit it may go on to any receiver.

	SECTION("a released set has no carrier and stays aboard") {
		REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
		CHECK_FALSE(MayTBMSpoilLeaveVehicle(*rig.hauler, *spoil, origin));
	}
	SECTION("spoil outside every set was already transferred onward and moves freely") {
		Train *carrier = rig.BuildHauler(rig.depot, 1, 0);
		rig.Preload(carrier->Next(), 10);
		CHECK(MayTBMSpoilLeaveVehicle(*carrier, *carrier->Next(), elsewhere));
	}
}

/** A replacement origin is a company Surface railway station; a visit to the old origin still counts. */
TEST_CASE("A project's origin is replaced only by another company Surface railway station", "[spoil-receiver-leg]")
{
	SpoilReceiverRig rig;
	const StationID old_origin = rig.fixture.origin->index;
	const TBMProjectID id = rig.project.id;
	Station *next = MakeStation(TileXYL(30, 22, MapLayer::Surface), _current_company, true);
	CHECK(SetTBMProjectOrigin(id, MakeStation(TileXYL(30, 24, MapLayer::Underground), _current_company, true)->index, true) == TBMProjectError::MissingOrigin);
	CHECK(SetTBMProjectOrigin(id, MakeStation(TileXYL(30, 26, MapLayer::Surface), OWNER_NONE, true)->index, true) == TBMProjectError::MissingOrigin);
	CHECK(SetTBMProjectOrigin(id, MakeStation(TileXYL(30, 28, MapLayer::Surface), _current_company, false)->index, true) == TBMProjectError::MissingOrigin);
	CHECK(SetTBMProjectOrigin(id, old_origin, true) == TBMProjectError::Unavailable); // Already the origin.
	{
		Backup<CompanyID> other(_current_company, CompanyID{1});
		CHECK(SetTBMProjectOrigin(id, next->index, true) == TBMProjectError::InvalidOwner);
		other.Restore();
	}
	CHECK(rig.Project().origin == old_origin); // Every refusal left the project alone.

	rig.BoreOneCut();
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
	const Train *spoil = rig.hauler->Next();

	SECTION("an unvisited set must reach the new origin") {
		REQUIRE(Command<Commands::SetTBMProjectOrigin>::Do(DoCommandFlag::Execute, id, next->index).Succeeded());
		CHECK(rig.Project().origin == next->index);
		CHECK_FALSE(MayTBMSpoilLeaveVehicle(*rig.hauler, *spoil, old_origin));
		CHECK(MayTBMSpoilLeaveVehicle(*rig.hauler, *spoil, next->index));
		rig.PlaceOnTrack(rig.hauler, TileXYL(25, 20, MapLayer::Surface));
		REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin);
		rig.PlaceOnTrack(rig.hauler, rig.origin_tile);
		CHECK(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin); // The old origin no longer ends the leg.
	}
	SECTION("a set that visited the old origin keeps its visit") {
		rig.PlaceOnTrack(rig.hauler, TileXYL(25, 20, MapLayer::Surface));
		REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin);
		rig.PlaceOnTrack(rig.hauler, rig.origin_tile);
		REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer);
		REQUIRE(SetTBMProjectOrigin(id, next->index, true) == TBMProjectError::None);
		CHECK(GetTBMWagonSet(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->set)->visited_origin);
		CHECK(MayTBMSpoilLeaveVehicle(*rig.hauler, *spoil, old_origin));
		CHECK(rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer);
	}
}
