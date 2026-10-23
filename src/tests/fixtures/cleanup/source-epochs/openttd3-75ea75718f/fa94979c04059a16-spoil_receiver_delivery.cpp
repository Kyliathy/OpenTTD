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
 * @see docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-i-receiver-design.md, Bounded intake
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"

#include "../safeguards.h"

/** Default assigned origin unloading retries the same physical station visit; bounded refusal creates neither free room nor a loaded departure. */
TEST_CASE("An assigned hauler retries refused default origin unloading before departure", "[spoil-receiver-origin-retry]")
{
	LoadedAtOrigin world;
	world.Unload(OrderUnloadType::UnloadIfPossible);
	REQUIRE(ShouldTBMHaulerRetryOriginUnload(*world.rig.hauler));
	const auto remaining = world.Aboard(); const auto cash = SpoilReceiverRig::Cash();
	world.rig.hauler->HandleLoading(false);
	CHECK_FALSE(world.rig.hauler->vehicle_flags.Test(VehicleFlag::LoadingFinished));
	CHECK(std::ranges::count(world.rig.fixture.origin->loading_vehicles, world.rig.hauler) == 1);
	// With the old intake interval still full, the native station may stage
	// another visit but cannot credit a tonne or invoice a fee twice.
	for (uint tick = 0; tick < 1000 && !world.rig.hauler->vehicle_flags.Test(VehicleFlag::LoadingFinished); ++tick) LoadUnloadStation(world.rig.fixture.origin);
	CHECK(world.Aboard() == remaining); CHECK(SpoilReceiverRig::Cash() == cash);
	// The next real receiver interval creates room. The same actual loading
	// owner retries remaining packets without another train visit or transfer.
	SpoilReceiverRig::RunIntervals(*world.earth, 1); SpoilReceiverRig::RunIntervals(*world.rock, 1);
	world.rig.hauler->HandleLoading(false);
	for (uint tick = 0; tick < 1000 && !world.rig.hauler->vehicle_flags.Test(VehicleFlag::LoadingFinished); ++tick) LoadUnloadStation(world.rig.fixture.origin);
	CHECK(world.Aboard() == 0); CHECK_FALSE(ShouldTBMHaulerRetryOriginUnload(*world.rig.hauler));
	delete world.rig.hauler->cargo_payment;
	world.rig.fixture.origin->loading_vehicles.remove(world.rig.hauler);
}

/** The receivers take their room; the fee is the only money moved; the rest stays in the wagons for a later visit. */
TEST_CASE("Unloading at the origin delivers only the receivers' room and keeps the rest aboard", "[spoil-receiver-delivery]")
{
	LoadedAtOrigin world;
	const Money cash = SpoilReceiverRig::Cash();
	const CargoType earth_cargo = GetCargoTypeByLabel(CargoLabel{"ERTH"});
	const auto delivered_before = Company::Get(_current_company)->cur_economy.delivered_cargo[earth_cargo];
	REQUIRE(world.cut.earth_t >= 32); // The first visit fills one whole interval of each receiver.
	REQUIRE(world.cut.rock_t >= 64);
	world.Unload(OrderUnloadType::UnloadIfPossible);
	CHECK(SpoilReceiverRig::Stock(*world.earth) == 32); // One interval of Earth intake.
	CHECK(SpoilReceiverRig::Stock(*world.rock) == 64); // One interval of Rock intake.
	CHECK(world.Aboard() == world.cut.earth_t + world.cut.rock_t - 96);
	CHECK(SpoilReceiverRig::Cash() == cash - (32 * 2 + 64 * 3)); // Fees only: spoil pays no freight income.
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[earth_cargo] == delivered_before + 32);
	CHECK(world.rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer); // The set is not empty yet.

	// Next interval the receivers take more; a second visit settles only the newly admitted tonnes. Since R2TBMF task
	// 5B.1 halved the yields, one cut (36 t Earth, 72 t Rock) no longer fills a second interval, so each receiver takes
	// what is left, capped by its interval room.
	const uint64_t earth_second = std::min<uint64_t>(32, world.cut.earth_t - 32);
	const uint64_t rock_second = std::min<uint64_t>(64, world.cut.rock_t - 64);
	SpoilReceiverRig::RunIntervals(*world.earth, 1);
	SpoilReceiverRig::RunIntervals(*world.rock, 1);
	world.Unload(OrderUnloadType::UnloadIfPossible);
	const Money second_fees = static_cast<int64_t>(earth_second * 2 + rock_second * 3);
	CHECK(SpoilReceiverRig::Cash() == cash - (32 * 2 + 64 * 3) - second_fees);
	CHECK(world.Aboard() == world.cut.earth_t + world.cut.rock_t - 96 - earth_second - rock_second);
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
	// Receivers process whole 8 t batches: a remainder below one batch waits in the receiver's stock, conserved, for the
	// next delivery. With the halved yields (R2TBMF 5B.1) one cut's 36 t of Earth leaves 4 t waiting.
	CHECK(GetSpoilReceiver(world.earth->index)->disposed_t + world.earth->accepted.front().waiting == world.cut.earth_t);
	CHECK(GetSpoilReceiver(world.earth->index)->disposed_t % 8 == 0);
	CHECK(world.earth->accepted.front().waiting < 8);
	CHECK(GetSpoilReceiver(world.rock->index)->crushed_t + world.rock->accepted.front().waiting == world.cut.rock_t);
	const auto emptied = world.rig.StepHauler();
	CHECK(emptied.phase == TBMHaulerPhase::ReturnEmpties);
	CHECK(GetTBMWagonSet(emptied.set)->state == TBMWagonSetState::ReturningEmpty);
	world.rig.HaulerToSourceBay();
	CHECK(world.rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
}
