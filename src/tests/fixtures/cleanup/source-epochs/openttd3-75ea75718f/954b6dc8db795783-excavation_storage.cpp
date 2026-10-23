/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_storage.cpp Rock indexing, conversion survival and measured storage budget. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 1 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../excavation.h"
#include "../clear_map.h"
#include "../void_map.h"
#include <chrono>
#include <iostream>

#include "../safeguards.h"

/** Verify plane isolation, reserved-state refusal and persistence through tile reconstruction. */
TEST_CASE("Packed excavation survives infrastructure metadata and rejects unrelated planes", "[excavation-storage]")
{
	Map::Allocate(64, 128, 4);
	PackedExcavationState state;
	REQUIRE(state.Reset(Map::PlaneSize()));
	CHECK(state.StorageBytes() == 4096);
	const auto underground = TileXYL(3, 7, MapLayer::Underground);
	const auto deep = MultiLayerMap::TileInLayer(underground, MapLayer::DeepUnderground);
	CHECK(state.Get(underground) == ExcavationState::Solid);
	CHECK(state.Set(underground, ExcavationState::Excavated));
	CHECK(state.Get(deep) == ExcavationState::Solid);
	CHECK(state.Set(deep, ExcavationState::Excavating));
	CHECK(state.Get(underground) == ExcavationState::Excavated);
	// Track, industry and station conversions reuse all generic fields; only independent storage can retain the cavity.
	for (const auto type : {TileType::Railway, TileType::Industry, TileType::Station}) {
		SetTileType(underground, type);
		CHECK(state.Get(underground) == ExcavationState::Excavated);
		MakeClear(underground, ClearGround::Rocks, 3);
		CHECK(state.Get(underground) == ExcavationState::Excavated);
	}
	CHECK_FALSE(state.Set(underground, ExcavationState::Reserved));
	CHECK_FALSE(state.Set(underground, static_cast<ExcavationState>(255)));
	CHECK_FALSE(state.Get(MultiLayerMap::ProjectToSurface(underground)));
	CHECK_FALSE(state.Get(MultiLayerMap::TileInLayer(underground, MapLayer::Space)));
	CHECK_FALSE(state.Get(INVALID_TILE));
	const auto fence = TileXYL(Map::MaxX(), 7, MapLayer::DeepUnderground);
	MakeVoid(fence);
	CHECK_FALSE(state.Get(fence));
	CHECK_FALSE(state.Set(fence, ExcavationState::Excavated));
	CHECK_FALSE(state.Reset(MAX_MAP_SIZE * MAX_MAP_SIZE + 4));
	CHECK(state.Get(underground) == ExcavationState::Excavated);
	Map::Allocate(64, 64, 4);
	CHECK_FALSE(state.Get(TileXYL(3, 7, MapLayer::Underground)));
	CHECK(state.Reset(0));
	CHECK(state.StorageBytes() == 0);
}

/** Measure maximum-map packed/byte payloads separately from sparse records and rebuilt lookup entries. */
TEST_CASE("Excavation maximum-map allocation budget", "[excavation-storage-budget]")
{
	using Clock = std::chrono::steady_clock;
	const uint32_t plane = MAX_MAP_SIZE * MAX_MAP_SIZE;
	PackedExcavationState packed;
	const auto start = Clock::now();
	REQUIRE(packed.Reset(plane));
	const auto allocated = Clock::now();
	REQUIRE(packed.Reset(plane));
	const auto reset = Clock::now();
	std::vector<uint8_t> byte_state(2 * plane, 0);
	const auto byte_allocated = Clock::now();
	std::vector<GeologyDeposit> deposits(1024);
	// Sparse seam storage scales with explicit bounded shapes, rather than an object for every maximum-map cell.
	for (auto &deposit : deposits) deposit.spans.resize(8);
	std::vector<uint32_t> lookup_entries(1024 * 8);
	const size_t sparse_bytes = deposits.capacity() * sizeof(GeologyDeposit) + 1024 * 8 * sizeof(GeologySpan);
	CHECK(packed.StorageBytes() == 8 * 1024 * 1024);
	CHECK(byte_state.size() == 32 * 1024 * 1024);
	CHECK(lookup_entries.size() * sizeof(uint32_t) == 32768);
	std::cout << "Excavation budget: packed=" << packed.StorageBytes() << " byte=" << byte_state.size()
		<< " sparse_1024x8=" << sparse_bytes << " lookup_entries=" << lookup_entries.size() * sizeof(uint32_t)
		<< " packed_object=" << sizeof(packed) << " deposit_record=" << sizeof(GeologyDeposit)
		<< " allocation_us=" << std::chrono::duration_cast<std::chrono::microseconds>(allocated - start).count()
		<< " reset_us=" << std::chrono::duration_cast<std::chrono::microseconds>(reset - allocated).count()
		<< " byte_allocation_us=" << std::chrono::duration_cast<std::chrono::microseconds>(byte_allocated - reset).count() << '\n';
}
