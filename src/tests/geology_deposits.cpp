/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geology_deposits.cpp Canonical shape, overlap, finite claims and rebuilt-index fixtures. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 2 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../excavation.h"

#include "../safeguards.h"

/**
 * Create a finite compact seam with deliberately reversed span order.
 * @param id Stable next identity supplied by the store.
 * @return Candidate whose canonicalization cannot change its twelve physical cells.
 */
static GeologyDeposit DepositFixture(GeologyDepositID id)
{
	GeologyDeposit deposit;
	deposit.id = id;
	deposit.layer = MapLayer::Underground;
	deposit.resource = CargoLabel{"COAL"};
	deposit.initial_reserve = 120;
	deposit.remaining_reserve = 120;
	deposit.spans = {{5, 4, 7}, {3, 4, 7}, {4, 4, 7}};
	return deposit;
}

/** Verify intersections/malformed shapes cannot depend on ID iteration, cargo slots or caller span ordering. */
TEST_CASE("Geological seams reject intersections and invalid authority", "[geology-deposits]")
{
	Map::Allocate(64, 64, 4);
	GeologyDepositStore store;
	auto deposit = DepositFixture(store.NextID());
	CHECK(store.Insert(deposit) == GeologyError::None);
	const auto tile = TileXYL(5, 4, MapLayer::Underground);
	CHECK(store.Find(tile) == deposit.id);
	REQUIRE(store.Get(deposit.id) != nullptr);
	CHECK(store.Get(deposit.id)->spans.front().y == 3);
	CHECK(store.Find(MultiLayerMap::ProjectToSurface(tile)) == GeologyDepositID{UINT32_MAX});
	CHECK(store.Find(MultiLayerMap::TileInLayer(tile, MapLayer::DeepUnderground)) == GeologyDepositID{UINT32_MAX});
	CHECK(store.Find(INVALID_TILE) == GeologyDepositID{UINT32_MAX});
	CHECK(store.Get(GeologyDepositID{UINT32_MAX}) == nullptr);
	auto intersecting = deposit;
	intersecting.id = store.NextID();
	intersecting.resource = CargoLabel{"CORE"};
	CHECK(store.Insert(intersecting) == GeologyError::Overlap);
	CHECK(store.Records().size() == 1);
	CHECK(store.NextID() == GeologyDepositID{1});
	intersecting.id = GeologyDepositID{99};
	CHECK(store.Insert(intersecting) == GeologyError::InvalidID);
	intersecting.id = store.NextID();
	intersecting.remaining_reserve = 121;
	CHECK(store.Insert(intersecting) == GeologyError::InvalidReserve);
	intersecting.initial_reserve = 119;
	intersecting.remaining_reserve = 119;
	CHECK(store.Insert(intersecting) == GeologyError::InvalidReserve);
	intersecting.initial_reserve = 1;
	intersecting.remaining_reserve = 1;
	CHECK(store.Insert(intersecting) == GeologyError::InvalidReserve);
	intersecting.initial_reserve = 120;
	intersecting.remaining_reserve = 120;
	intersecting.spans = {{3, 8, 4}};
	CHECK(store.Insert(intersecting) == GeologyError::InvalidShape);
	intersecting.spans = {{3, 9, 12}, {3, 11, 13}};
	CHECK(store.Insert(intersecting) == GeologyError::Overlap);
	intersecting.spans = {{64, 9, 12}};
	CHECK(store.Insert(intersecting) == GeologyError::InvalidShape);
	intersecting.spans = {{3, 9, 12}};
	intersecting.layer = MapLayer::DeepUnderground;
	CHECK(store.Insert(intersecting) == GeologyError::InvalidResource);
	CHECK(store.Get(deposit.id)->remaining_reserve == 120);
}

/** Verify claim release/deletion never recreates consumed ore or aliases a stable identity. */
TEST_CASE("Geological claims conserve finite reserve through release and deletion", "[geology-claims]")
{
	Map::Allocate(64, 64, 4);
	GeologyDepositStore store;
	const auto id = store.NextID();
	REQUIRE(store.Insert(DepositFixture(id)) == GeologyError::None);
	CHECK(store.ConsumeReserve(id, CompanyID{0}, 1) == GeologyError::ClaimConflict);
	CHECK(store.Claim(id, CompanyID::Invalid()) == GeologyError::ClaimConflict);
	CHECK(store.Claim(id, CompanyID{0}) == GeologyError::None);
	CHECK(store.Claim(id, CompanyID{1}) == GeologyError::ClaimConflict);
	CHECK(store.ConsumeReserve(id, CompanyID{0}, 121) == GeologyError::Depleted);
	CHECK(store.Get(id)->remaining_reserve == 120);
	CHECK(store.ConsumeReserve(id, CompanyID{0}, 119) == GeologyError::None);
	CHECK(store.ReleaseClaim(id, CompanyID{1}) == GeologyError::ClaimConflict);
	CHECK(store.ReleaseClaim(id, CompanyID{0}) == GeologyError::None);
	CHECK(store.Get(id)->remaining_reserve == 1);
	CHECK(store.Claim(id, CompanyID{1}) == GeologyError::None);
	CHECK(store.ConsumeReserve(id, CompanyID{1}, 1) == GeologyError::None);
	CHECK(store.ReleaseClaim(id, CompanyID{1}) == GeologyError::None);
	CHECK(store.Claim(id, CompanyID{0}) == GeologyError::Depleted);
	CHECK(store.Get(id)->remaining_reserve == 0);
	CHECK_FALSE(store.Erase(GeologyDepositID{99}));
	CHECK(store.Erase(id));
	CHECK(store.Get(id) == nullptr);
	CHECK(store.Find(TileXYL(5, 4, MapLayer::Underground)) == GeologyDepositID{UINT32_MAX});
	CHECK(store.Insert(DepositFixture(id)) == GeologyError::InvalidID);
	CHECK(store.Insert(DepositFixture(store.NextID())) == GeologyError::None);
	CHECK(store.Get(GeologyDepositID{1})->remaining_reserve == 120);
}

/** Verify canonical authoritative records can rebuild bounded cache results without serializing addresses. */
TEST_CASE("Geology cache rebuild retains reserve and caps query candidates", "[geology-index]")
{
	Map::Allocate(64, 64, 4);
	GeologyDepositStore store;
	// Thirty-two disjoint one-cell records occupy one bucket; its explicit limit bounds every future lookup.
	for (uint i = 0; i < MAX_GEOLOGY_BUCKET_ENTRIES; ++i) {
		auto deposit = DepositFixture(store.NextID());
		deposit.remaining_reserve = 17;
		deposit.spans = {{static_cast<uint16_t>(2 + i / 8), static_cast<uint16_t>(2 + i % 8), static_cast<uint16_t>(2 + i % 8)}};
		REQUIRE(store.Insert(deposit) == GeologyError::None);
	}
	auto extra = DepositFixture(store.NextID());
	extra.spans = {{8, 2, 2}};
	CHECK(store.Insert(extra) == GeologyError::Limit);
	CHECK(store.Records().size() == MAX_GEOLOGY_BUCKET_ENTRIES);
	const auto authority = std::vector<GeologyDeposit>(store.Records().begin(), store.Records().end());
	store.RebuildIndex();
	GeologyDepositStore restored;
	// Only record/shape/reserve authority is replayed here; Group 4 later tests the actual save transport.
	for (const auto &deposit : authority) {
		REQUIRE(restored.Insert(deposit) == GeologyError::None);
		const auto &span = deposit.spans.front();
		const auto tile = TileXYL(span.first_x, span.y, deposit.layer);
		CHECK(store.Find(tile) == deposit.id);
		CHECK(restored.Find(tile) == deposit.id);
		CHECK(restored.Get(deposit.id)->remaining_reserve == 17);
	}
	CHECK(restored.IndexPayloadBytes() == store.IndexPayloadBytes());
}
