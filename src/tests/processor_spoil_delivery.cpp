/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file processor_spoil_delivery.cpp Mine-plan reuse of the delivered Earth/Rock intake owner on an almost-full interval. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 10.4; docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 8 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"

#include "../safeguards.h"

/** Almost-filled receiver intervals admit one unit each and leave every other excavated unit in its actual assigned wagon set. */
TEST_CASE("Mine-chain delivery reuses partial Earth and Rock receiver admission", "[processor-spoil-delivery]")
{
	LoadedAtOrigin world;
	const CargoType earth = GetCargoTypeByLabel(CargoLabel{"ERTH"});
	const CargoType rock = GetCargoTypeByLabel(CargoLabel{"ROCK"});
	// Seed an earlier admitted shipment through the real fee/interval owner, then its sole Industry stock write as delivery does.
	REQUIRE(AdmitSpoilToReceiver(*world.earth, earth, 31, _current_company) == 31);
	SpoilReceiverRig::Stock(*world.earth) += 31;
	REQUIRE(AdmitSpoilToReceiver(*world.rock, rock, 63, _current_company) == 63);
	SpoilReceiverRig::Stock(*world.rock) += 63;
	const uint64_t aboard = world.Aboard();
	const Money cash = SpoilReceiverRig::Cash();
	world.Unload(OrderUnloadType::UnloadIfPossible);
	CHECK(SpoilReceiverRig::Stock(*world.earth) == 32);
	CHECK(SpoilReceiverRig::Stock(*world.rock) == 64);
	CHECK(world.Aboard() == aboard - 2);
	CHECK(SpoilReceiverRig::Cash() == cash - 5);
	CHECK(world.rig.fixture.origin->goods[earth].TotalCount() == 0);
	CHECK(world.rig.fixture.origin->goods[rock].TotalCount() == 0);
}
