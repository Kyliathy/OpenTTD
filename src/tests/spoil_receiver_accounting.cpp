/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file spoil_receiver_accounting.cpp Spoil accounting across onward transfer (Plan 5 Groups 9.1-9.2): visiting the
 * origin and transferring there pay and charge nothing; an ordinary carrier outside every set takes the spoil on;
 * and the company's own receivers charge their fee once, only for the tonnes they finally admit.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 9
 * @see docs/zz-rhr/upgrades/2026-10/r2mlvs1-d-recipes-and-bootstrap.md, Earth/Rock disposal settlement row
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"

#include "../safeguards.h"

/** A transfer leaves the money alone; a later carrier's final admission is charged once, for exactly what fits. */
TEST_CASE("Spoil transferred onward is charged once, where a receiver finally admits it", "[spoil-receiver-accounting]")
{
	LoadedAtOrigin world;
	const uint64_t total = world.cut.earth_t + world.cut.rock_t;
	const Money cash = SpoilReceiverRig::Cash();
	Station *origin = world.rig.fixture.origin;
	world.Unload(OrderUnloadType::Transfer);
	CHECK(world.Aboard() == 0);
	CHECK(SpoilReceiverRig::Cash() == cash); // A visit and a transfer pay nothing and charge nothing.
	CHECK(SpoilReceiverRig::Stock(*world.earth) == 0);
	CHECK(world.rig.StepHauler().phase == TBMHaulerPhase::ReturnEmpties); // The set is empty once its spoil is on the platform.

	// An ordinary train, outside every wagon set, takes the spoil on from the platform.
	Train *carrier = world.rig.BuildHauler(world.rig.depot, 2, 2);
	world.rig.PlaceOnTrack(carrier, world.rig.origin_tile);
	RunStationStop(carrier, origin, OrderUnloadType::NoUnload, OrderLoadType::LoadIfPossible);
	CHECK(TBMExchangeRig::Aboard(carrier) == total);
	CHECK(SpoilReceiverRig::Cash() == cash);

	// Delivered to the company's own receivers, only the admitted tonnes are charged, once.
	RunStationStop(carrier, origin, OrderUnloadType::UnloadIfPossible, OrderLoadType::NoLoad);
	CHECK(SpoilReceiverRig::Stock(*world.earth) == 32);
	CHECK(SpoilReceiverRig::Stock(*world.rock) == 64);
	CHECK(TBMExchangeRig::Aboard(carrier) == total - 96);
	CHECK(SpoilReceiverRig::Cash() == cash - (32 * 2 + 64 * 3));
	CHECK(GetSpoilReceiver(world.earth->index)->fees + GetSpoilReceiver(world.rock->index)->fees == 32 * 2 + 64 * 3);
}
