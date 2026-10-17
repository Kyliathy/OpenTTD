/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file spoil_receiver_stall.cpp Published logistics waits and the pause of new cutting (Plan 5 Group 9.3): a cutter
 * without a hauler, a queued hauler, a lost origin, a lost route and a saturated origin are reported; a stall stops
 * new face work but never the return for exchange, the exchange or the haul of loaded spoil.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 9
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-i-receiver-design.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"

#include "../safeguards.h"

/** Each logistics wait is published for the project from its cutter's phase and its haulers' holds. */
TEST_CASE("Logistics waits are published from the cutter and its haulers", "[spoil-receiver-stall]")
{
	SECTION("a cutter that needs empties but has no hauler") {
		TBMExchangeRig rig;
		CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::None); // Boring needs no hauler yet.
		rig.BoreOneCut();
		CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::NoHauler); // On its way back.
		REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
		CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::NoHauler);
		rig.Meet();
		CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::None);
	}
	SECTION("a second hauler waits for the bay") {
		TBMExchangeRig rig;
		rig.BoreOneCut();
		REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
		rig.Meet();
		Train *second = rig.BuildHauler(rig.depot, 2, 2);
		rig.PlaceAt(second, *rig.front);
		REQUIRE(AssignTBMHauler(rig.project.id, second->index, true) == TBMExchangeError::None);
		REQUIRE(TBMExchangeRig::StepHauler(*second).phase == TBMHaulerPhase::AwaitExchange);
		REQUIRE(TBMExchangeRig::StepHauler(*second).wait == TBMHaulerWait::OccupiedBay);
		CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::OccupiedBay);
		CHECK_FALSE(IsTBMLogisticsStalled(rig.Project())); // Queueing is no reason to stop boring.
	}
	SECTION("a lost origin") {
		TBMExchangeRig rig;
		rig.fixture.origin->owner = OWNER_NONE;
		CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::LostOrigin);
		CHECK(IsTBMLogisticsStalled(rig.Project()));
	}
}

/** A hauler that cannot find its route stops new cutting; clearing the route lets the face work again. */
TEST_CASE("A blocked hauler pauses new cutting until its route is found", "[spoil-receiver-stall]")
{
	TBMExchangeRig rig;
	REQUIRE(rig.Project().phase == TBMPhase::Bore);
	Train *second = rig.BuildHauler(rig.depot, 2, 2);
	rig.PlaceOnTrack(second, TileXYL(25, 20, MapLayer::Surface));
	REQUIRE(AssignTBMHauler(rig.project.id, second->index, true) == TBMExchangeError::None);
	second->vehicle_flags.Set(VehicleFlag::PathfinderLost);
	const auto lost = TBMExchangeRig::StepHauler(*second);
	CHECK(lost.wait == TBMHaulerWait::BlockedLine);
	CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::BlockedLine);
	REQUIRE(IsTBMLogisticsStalled(rig.Project()));
	const auto partial = rig.Job().partial_work_ticks;
	CHECK(rig.Tick().blocked == TBMBlockedReason::LogisticsStall);
	CHECK(rig.Project().phase == TBMPhase::Bore);
	CHECK(rig.Job().partial_work_ticks == partial); // No work was banked while stalled.

	second->vehicle_flags.Reset(VehicleFlag::PathfinderLost);
	CHECK(TBMExchangeRig::StepHauler(*second).wait == TBMHaulerWait::None);
	CHECK(rig.Tick().blocked == TBMBlockedReason::None);
	CHECK((rig.Job().partial_work_ticks > partial || rig.Job().cursor == 1)); // Work resumed, or a one-tick cut completed.
}

/** A stall never strands loaded spoil: the cutter still returns, waits and exchanges with another hauler. */
TEST_CASE("A stalled project still returns for and completes its exchange", "[spoil-receiver-stall]")
{
	TBMExchangeRig rig;
	const auto estimate = rig.BoreOneCut(); // The exchange is now due.
	Train *second = rig.BuildHauler(rig.depot, 2, 2);
	rig.PlaceOnTrack(second, TileXYL(25, 20, MapLayer::Surface));
	REQUIRE(AssignTBMHauler(rig.project.id, second->index, true) == TBMExchangeError::None);
	second->vehicle_flags.Set(VehicleFlag::PathfinderLost);
	REQUIRE(TBMExchangeRig::StepHauler(*second).wait == TBMHaulerWait::BlockedLine);
	REQUIRE(IsTBMLogisticsStalled(rig.Project()));
	CHECK(rig.Tick().phase == TBMPhase::WaitForEmpties); // The return is not paused.
	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded); // The lost hauler is not admitted, so this one has the turn.
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == estimate.earth_t + estimate.rock_t);
	CHECK(rig.Tick().phase == TBMPhase::Resume);
}

/** At its origin, a loaded hauler with no receiver room is held SaturatedReceiver, which stalls the project. */
TEST_CASE("A saturated origin holds the loaded hauler until a receiver has room again", "[spoil-receiver-stall]")
{
	LoadedAtOrigin world;
	SpoilReceiverRig::Stock(*world.earth) = 1024;
	SpoilReceiverRig::Stock(*world.rock) = 1024;
	const auto held = world.rig.StepHauler();
	CHECK(held.wait == TBMHaulerWait::SaturatedReceiver);
	CHECK(held.phase == TBMHaulerPhase::DisposeOrTransfer);
	CHECK(world.Aboard() == world.cut.earth_t + world.cut.rock_t); // Nothing is lost while it waits.
	CHECK(GetTBMLogisticsWait(world.rig.Project()) == TBMLogisticsWait::SaturatedReceiver);
	CHECK(IsTBMLogisticsStalled(world.rig.Project()));
	// One Earth interval frees room for the Earth aboard, so the hauler is no longer saturated.
	SpoilReceiverRig::RunIntervals(*world.earth, 1);
	CHECK(world.rig.StepHauler().wait == TBMHaulerWait::None);
	CHECK_FALSE(IsTBMLogisticsStalled(world.rig.Project()));
}
