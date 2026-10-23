/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_mine_dispatch_pool.cpp Isolated actual packet-pool exhaustion/retry check; run optimized and separately from ordinary dispatch checks. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 8 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mine_production_fixture.h"

#include "../safeguards.h"

/** Own empty real packet allocations solely to exhaust actual pool resources, without fabricating cargo or altering allocator counters. */
class ExhaustedMinePacketPool {
	std::vector<CargoPacket *> allocated; ///< Real pool-owned empty objects, not another gameplay cargo inventory.

public:
	/** Allocate to the real hard limit; the focused test runs in isolation with no caller packets. */
	ExhaustedMinePacketPool()
	{
		REQUIRE(CargoPacket::GetNumItems() == 0);
		this->allocated.reserve(CargoPacketPool::MAX_SIZE);
		// Empty loading-form objects reserve actual allocator capacity while introducing no physical cargo quantity.
		while (CargoPacket::CanAllocateItem()) this->allocated.push_back(CargoPacket::Create());
	}

	/** Release only this fixture's real allocations; the station's later successful supply remains under its own owner. */
	~ExhaustedMinePacketPool()
	{
		for (CargoPacket *packet : this->allocated) delete packet;
	}

	/** Free exactly one real slot so the previously refused finite source can retry normally. */
	void ReleaseOne()
	{
		REQUIRE_FALSE(this->allocated.empty());
		delete this->allocated.back();
		this->allocated.pop_back();
	}
};

/** An actually full packet pool preserves output; freeing one actual allocation admits exactly one source-to-station receipt. */
TEST_CASE("Finite mine dispatch retains output through actual packet-pool exhaustion", "[layer-mine-dispatch-pool]")
{
	MineProductionRig rig;
	Station *station = rig.StationAt(TileAddWrap(rig.request.tile, 3, 0));
	const CargoType cargo = rig.industry->produced[0].cargo;
	station->goods[cargo].rating = UINT8_MAX;
	station->goods[cargo].last_speed = 100;
	REQUIRE(rig.industry->stations_near.contains(station));
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	const auto reserve = rig.Reserve();
	ExhaustedMinePacketPool pool;
	REQUIRE_FALSE(CargoPacket::CanAllocateItem());
	CHECK(DispatchLayerMineCargo(*rig.industry) == 0);
	CHECK(rig.industry->produced[0].waiting == 4);
	CHECK(station->goods[cargo].TotalCount() == 0);
	CHECK(GetLayerMineState(rig.industry->index)->dispatched == 0);
	pool.ReleaseOne();
	CHECK(DispatchLayerMineCargo(*rig.industry) == 4);
	CHECK(rig.industry->produced[0].waiting == 0);
	CHECK(station->goods[cargo].TotalCount() == 4);
	CHECK(rig.Reserve() == reserve);
	CHECK(GetLayerMineState(rig.industry->index)->dispatched == 4);
}
