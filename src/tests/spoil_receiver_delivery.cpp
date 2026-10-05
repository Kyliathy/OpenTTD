/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file spoil_receiver_delivery.cpp A loaded hauler unloading at its origin through the real station path (Plan 5
 * Group 8.3, with the accounting rules of Group 9.1): PrepareUnload stages its set, LoadUnloadStation delivers only
 * the receivers' room, the fee is charged once for exactly that, there is no freight income, and the refused
 * remainder stays aboard or, on an unload order, goes onto the platform for onward transfer.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Groups 8-9
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-i-receiver-design.md, Bounded intake
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"

#include "../safeguards.h"

/** The receivers take their room; the fee is the only money moved; the rest stays in the wagons for a later visit. */
TEST_CASE("Unloading at the origin delivers only the receivers' room and keeps the rest aboard", "[spoil-receiver-delivery]")
{
	LoadedAtOrigin world;
	const Money cash = SpoilReceiverRig::Cash();
	const CargoType earth_cargo = GetCargoTypeByLabel(CargoLabel{"ERTH"});
	const auto delivered_before = Company::Get(_current_company)->cur_economy.delivered_cargo[earth_cargo];
	world.Unload(OrderUnloadType::UnloadIfPossible);
	CHECK(SpoilReceiverRig::Stock(*world.earth) == 32); // One interval of Earth intake.
	CHECK(SpoilReceiverRig::Stock(*world.rock) == 64); // One interval of Rock intake.
	CHECK(world.Aboard() == world.cut.earth_t + world.cut.rock_t - 96);
	CHECK(SpoilReceiverRig::Cash() == cash - (32 * 2 + 64 * 3)); // Fees only: spoil pays no freight income.
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[earth_cargo] == delivered_before + 32);
	CHECK(world.rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer); // The set is not empty yet.

	// Next interval the receivers take more; a second visit settles only the newly admitted tonnes.
	SpoilReceiverRig::RunIntervals(*world.earth, 1);
	SpoilReceiverRig::RunIntervals(*world.rock, 1);
	world.Unload(OrderUnloadType::UnloadIfPossible);
	CHECK(SpoilReceiverRig::Cash() == cash - 2 * (32 * 2 + 64 * 3));
	CHECK(world.Aboard() == world.cut.earth_t + world.cut.rock_t - 192);
}

/** On an unload order the refused remainder goes onto the platform, keeping its packets, and the set is empty. */
TEST_CASE("An unload order transfers the refused spoil onto the origin's platform", "[spoil-receiver-delivery]")
{
	LoadedAtOrigin world;
	const uint64_t total = world.cut.earth_t + world.cut.rock_t;
	world.Unload(OrderUnloadType::Unload);
	CHECK(world.Aboard() == 0);
	const auto &goods = world.rig.fixture.origin->goods;
	const CargoType earth_cargo = GetCargoTypeByLabel(CargoLabel{"ERTH"});
	const CargoType rock_cargo = GetCargoTypeByLabel(CargoLabel{"ROCK"});
	REQUIRE(goods[earth_cargo].HasData());
	REQUIRE(goods[rock_cargo].HasData());
	const uint64_t platform = goods[earth_cargo].GetData().cargo.TotalCount() + goods[rock_cargo].GetData().cargo.TotalCount();
	CHECK(platform == total - 96); // Nothing vanished: received plus transferred is everything carried.
	CHECK(world.rig.StepHauler().phase == TBMHaulerPhase::ReturnEmpties);
}

/** A set without its carrier is not staged at all: nothing leaves its wagons and nothing is charged. */
TEST_CASE("A released loaded set does not unload at the origin", "[spoil-receiver-delivery]")
{
	LoadedAtOrigin world;
	const uint64_t total = world.Aboard();
	const Money cash = SpoilReceiverRig::Cash();
	REQUIRE(ReleaseTBMHauler(world.rig.hauler->index, true) == TBMExchangeError::None);
	world.Unload(OrderUnloadType::Unload);
	CHECK(world.Aboard() == total);
	CHECK(SpoilReceiverRig::Stock(*world.earth) == 0);
	CHECK(SpoilReceiverRig::Cash() == cash);
}

/** The whole cycle with real disposal (Plan 5 6.3): visits repeat until the receivers have taken every tonne. */
TEST_CASE("A hauler completes its cycle once real receivers have disposed of its whole load", "[spoil-receiver-delivery]")
{
	LoadedAtOrigin world;
	const uint64_t total = world.Aboard();
	uint64_t disposed = 0;
	// Each visit delivers one interval of intake per receiver; the receivers process it before the next visit.
	for (int visit = 0; visit < 6 && world.Aboard() != 0; ++visit) {
		world.Unload(OrderUnloadType::UnloadIfPossible);
		disposed = total - world.Aboard();
		SpoilReceiverRig::RunIntervals(*world.earth, 1);
		SpoilReceiverRig::RunIntervals(*world.rock, 1);
	}
	CHECK(world.Aboard() == 0);
	CHECK(disposed == total);
	CHECK(GetSpoilReceiver(world.earth->index)->disposed_t == world.cut.earth_t);
	CHECK(GetSpoilReceiver(world.rock->index)->crushed_t == world.cut.rock_t);
	const auto emptied = world.rig.StepHauler();
	CHECK(emptied.phase == TBMHaulerPhase::ReturnEmpties);
	CHECK(GetTBMWagonSet(emptied.set)->state == TBMWagonSetState::ReturningEmpty);
	world.rig.HaulerToSourceBay();
	CHECK(world.rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
}
