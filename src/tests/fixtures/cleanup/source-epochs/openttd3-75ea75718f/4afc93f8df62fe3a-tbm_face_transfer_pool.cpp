/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_transfer_pool.cpp Isolated packet-pool exhaustion check of the face transfer: a full pool refuses it
 * before any packet moves, and room for its splits admits it again. It fills the real pool, so run it optimized and
 * apart from the ordinary face transfer checks, as layer_mine_dispatch_pool.cpp is.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 5 (task 5.5)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-a-face-exchange-design.md (rule F3)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "../safeguards.h"

/** Empty real packet allocations that hold the pool full, without adding any cargo. */
class ExhaustedTBMPacketPool {
	std::vector<CargoPacket *> allocated; ///< Pool-owned empty packets, in allocation order.

public:
	/** Allocate until the real pool refuses another packet. */
	ExhaustedTBMPacketPool()
	{
		this->allocated.reserve(CargoPacketPool::MAX_SIZE - CargoPacket::GetNumItems());
		// Empty packets reserve the allocator's slots while carrying no tonnes.
		while (CargoPacket::CanAllocateItem()) this->allocated.push_back(CargoPacket::Create());
	}

	/** Release the remaining allocations of this pool filler only. */
	~ExhaustedTBMPacketPool()
	{
		for (CargoPacket *packet : this->allocated) delete packet;
	}

	/**
	 * Free some real slots.
	 * @param count Slots to free.
	 */
	void Release(size_t count)
	{
		REQUIRE(count <= this->allocated.size());
		// Each released slot is one more packet the transfer may split off.
		for (size_t i = 0; i < count; ++i) {
			delete this->allocated.back();
			this->allocated.pop_back();
		}
	}
};

/**
 * @param rig Rig whose trains are counted.
 * @return Parts of the cutter's spoil units and of the hauler's set wagons: the most packets one transfer may split.
 */
static size_t CountTransferParts(const TBMFaceTransferRig &rig)
{
	size_t parts = 0;
	std::vector<Train *> units;
	for (const auto role : {TBMConsistRole::EarthWagon, TBMConsistRole::RockWagon}) {
		for (Train *unit : rig.Wagons(role)) units.push_back(unit);
		for (Train *unit : rig.SetWagons(role)) units.push_back(unit);
	}
	// Every unit counts with its articulated parts, as the description counts them.
	for (Train *unit : units) {
		for (const Train *part = unit; part != nullptr; part = part->Next()) {
			++parts;
			if (part == unit->GetLastEnginePart()) break;
		}
	}
	return parts;
}

/** Task 5.5: a full packet pool refuses the transfer with Limit and nothing moves; freed room admits it again. */
TEST_CASE("A full packet pool refuses the face transfer and leaves every packet in place", "[demo-fix-face-transfer-pool]")
{
	TBMFaceTransferRig rig;
	const auto totals = CountTBMPackets({rig.front, rig.hauler});
	const uint64_t aboard = TBMExchangeRig::Aboard(rig.front);
	REQUIRE(aboard != 0);
	ExhaustedTBMPacketPool pool;
	REQUIRE_FALSE(CargoPacket::CanAllocateItem());
	const auto before = TBMFaceSnapshot::Capture(rig);
	CHECK(DescribeTBMFaceTransfer(rig.Request()).error == TBMExchangeError::Limit);
	CHECK(ExecuteTBMFaceTransfer(rig.Request()).error == TBMExchangeError::Limit);
	CHECK(TBMFaceSnapshot::Capture(rig) == before);

	// One slot short of a split per part still refuses; room for all of them admits the transfer.
	const size_t parts = CountTransferParts(rig);
	pool.Release(parts - 1);
	CHECK(ExecuteTBMFaceTransfer(rig.Request()).error == TBMExchangeError::Limit);
	CHECK(TBMFaceSnapshot::Capture(rig) == before);
	pool.Release(1);
	REQUIRE(ExecuteTBMFaceTransfer(rig.Request()).error == TBMExchangeError::None);
	CHECK(TBMExchangeRig::Aboard(rig.front) == 0);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == aboard);
	CHECK(CountTBMPackets({rig.front, rig.hauler}) == totals);
}
