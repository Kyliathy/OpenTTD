/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file cargo_partial_transfer.cpp A vehicle's cargo list reassigns part of its Deliver cargo to Transfer (demo report
 * R11). A spoil receiver with too little room accepts only part of a hauler's load. On an "Unload all" or "Transfer"
 * order the refused part is reassigned to Transfer; that amount is usually smaller than the whole Deliver amount, so a
 * packet must be split. Packet counts must never wrap, their sum must equal the cached total, the designation counts
 * must add up, and exactly the requested amount must change designation. Reassigning the whole amount keeps its old
 * packets unchanged.
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 6B (task 6B.1)
 * @see docs/zz-reach2/upgrades/2026-10/r2df-a-reproduction-notes.md R11
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../cargopacket.h"
#include "../station_base.h"
#include "../order_type.h"

#include "../safeguards.h"

/** The designations a vehicle cargo list keeps, for readable checks. */
using Action = VehicleCargoList::MoveToAction;

/**
 * A vehicle cargo list holding the given packets, all designated for delivery, after the given Transfer packets.
 * @param transfer Counts of packets already designated for transfer, in list order.
 * @param deliver Counts of packets designated for delivery, in list order.
 * @return The list; its destructor frees the packets.
 */
static std::unique_ptr<VehicleCargoList> MakeList(std::initializer_list<uint16_t> transfer, std::initializer_list<uint16_t> deliver)
{
	auto list = std::make_unique<VehicleCargoList>();
	uint total = 0;
	uint16_t origin = 10;
	/**
	 * Append the next packet as kept cargo, as a station stop loads it. Each packet comes from its own station, as the
	 * demo's packets came from different loads, so Append does not merge it into the one before.
	 * @param count Size of the packet.
	 */
	const auto append = [&list, &total, &origin](uint16_t count) {
		REQUIRE(CargoPacket::CanAllocateItem());
		list->Append(CargoPacket::Create(StationID{origin++}, count, Source{IndustryID::Invalid(), SourceType::Industry}), Action::Keep);
		total += count;
	};
	// In list order: the packets that will be transferred first.
	for (uint16_t count : transfer) append(count);
	const uint transfer_total = total;
	for (uint16_t count : deliver) append(count);
	// Staging at a station that accepts the cargo, on an "Unload all" order, designates all of it for delivery: none of
	// it was loaded there. The leading whole packets are then transferred; that reassignment ends on a packet boundary,
	// so it splits nothing.
	const GoodsEntry ge{};
	REQUIRE(list->Stage(true, StationID{1}, {}, OrderUnloadType::Unload, &ge, CargoType{0}, nullptr, INVALID_TILE));
	REQUIRE(list->ActionCount(Action::Deliver) == total);
	if (transfer_total != 0) REQUIRE(list->Reassign<Action::Deliver, Action::Transfer>(transfer_total) == transfer_total);
	return list;
}

/**
 * @param list Vehicle cargo list.
 * @return The count of every packet, in list order.
 */
static std::vector<uint> PacketCounts(const VehicleCargoList &list)
{
	std::vector<uint> counts;
	for (const CargoPacket *cp : *list.Packets()) counts.push_back(cp->Count());
	return counts;
}

/**
 * Require the list to be consistent: no wrapped or empty packet, packet counts adding up to the cached total, and the
 * designation counts adding up to it as well (what AssertCountConsistency checks at the next station stop).
 * @param list Vehicle cargo list.
 * @param capacity Largest count any packet may hold here: the largest packet the list started with.
 */
static void CheckConsistent(const VehicleCargoList &list, uint capacity)
{
	uint sum = 0;
	// A wrapped packet shows up as a count far above anything the list was given.
	for (uint count : PacketCounts(list)) {
		CHECK(count > 0);
		CHECK(count <= capacity);
		sum += count;
	}
	CHECK(sum == list.TotalCount());
	CHECK(list.ActionCount(Action::Transfer) + list.ActionCount(Action::Deliver) + list.ActionCount(Action::Keep) + list.ActionCount(Action::Load) == list.TotalCount());
}

/** The R11 case: part of a hopper's Deliver cargo is refused and transferred, which splits a packet. */
TEST_CASE("A partial Deliver-to-Transfer reassignment splits one packet exactly", "[demo-fix-cargo-transfer]")
{
	// The demo's Earth hopper: 62 t in packets of 8 and 9 t; the receiver had room for 30 t, so 32 t were refused.
	auto list = MakeList({}, {8, 9, 9, 9, 9, 9, 9});
	REQUIRE(list->ActionCount(Action::Deliver) == 62);
	REQUIRE(list->Reassign<Action::Deliver, Action::Transfer>(32) == 32);
	CHECK(list->ActionCount(Action::Transfer) == 32);
	CHECK(list->ActionCount(Action::Deliver) == 30);
	CheckConsistent(*list, 9);
	// The fourth packet is split: its first 6 t are transferred with the three packets before it, its other 3 t stay.
	CHECK(PacketCounts(*list) == std::vector<uint>{8, 9, 9, 6, 3, 9, 9, 9});
}

/** Cargo already designated for transfer stays in front; only the requested amount joins it. */
TEST_CASE("A partial reassignment after earlier Transfer cargo moves only the requested amount", "[demo-fix-cargo-transfer]")
{
	// The demo's Rock hopper after an earlier transfer: 18 t already transferred, then 106 t to deliver.
	auto list = MakeList({18}, {16, 18, 18, 18, 18, 18});
	REQUIRE(list->ActionCount(Action::Transfer) == 18);
	REQUIRE(list->Reassign<Action::Deliver, Action::Transfer>(40) == 40);
	CHECK(list->ActionCount(Action::Transfer) == 58);
	CHECK(list->ActionCount(Action::Deliver) == 66);
	CheckConsistent(*list, 18);
	// 18 t were there before; 16 + 18 + 6 t join them, and the third Deliver packet keeps its other 12 t.
	CHECK(PacketCounts(*list) == std::vector<uint>{18, 16, 18, 6, 12, 18, 18, 18});
}

/** A request that ends exactly on a packet boundary splits nothing. */
TEST_CASE("A partial reassignment on a packet boundary splits no packet", "[demo-fix-cargo-transfer]")
{
	auto list = MakeList({}, {9, 9, 9, 9});
	REQUIRE(list->Reassign<Action::Deliver, Action::Transfer>(18) == 18);
	CHECK(list->ActionCount(Action::Transfer) == 18);
	CHECK(list->ActionCount(Action::Deliver) == 18);
	CheckConsistent(*list, 9);
	CHECK(PacketCounts(*list) == std::vector<uint>{9, 9, 9, 9});
}

/** Upstream's caller passes the whole Deliver amount: every packet changes designation and none is split. */
TEST_CASE("A full Deliver-to-Transfer reassignment keeps its packets unchanged", "[demo-fix-cargo-transfer]")
{
	auto list = MakeList({5}, {8, 9, 9});
	// More than the Deliver amount is asked for, as a station that stopped accepting does; it is capped at 26 t.
	REQUIRE(list->Reassign<Action::Deliver, Action::Transfer>(100) == 26);
	CHECK(list->ActionCount(Action::Transfer) == 31);
	CHECK(list->ActionCount(Action::Deliver) == 0);
	CheckConsistent(*list, 9);
	CHECK(PacketCounts(*list) == std::vector<uint>{5, 8, 9, 9});
}
