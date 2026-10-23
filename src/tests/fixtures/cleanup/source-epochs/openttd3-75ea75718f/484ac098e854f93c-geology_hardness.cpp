/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geology_hardness.cpp Shared regional cut, material rendering and saved-authority acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group10
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../multilayer_sprite.h"

#include "../safeguards.h"

/** Actual generated rock agrees between quotes and material art, with a frozen deposit override and immutable rendering. */
TEST_CASE("Regional rock quote art and restored authority agree", "[geology-regional]")
{
	ExcavationFixture fixture(false, GEOLOGY_HARDNESS_VERSION);
	const auto original = CaptureGeologyState();
	const auto &geology = GetGeologyWorldState();
	const auto before = ComputeGeologyStateDigest(geology);
	GeologyWorldState restored;
	REQUIRE(PrepareGeologyRestoration(original, restored) == GeologyError::None);
	CHECK(ComputeGeologyStateDigest(restored) == before);
	const std::array<uint, 4> earth{3, 2, 1, 1}, rock{6, 12, 20, 24}, days{1, 1, 2, 4};
	std::array<bool, 4> seen{};
	uint mismatches = 0;
	// Inspect both planes once, aggregating unchanged state while checking one
	// real quote for each encountered material. No fixture assigns hardness.
	for (const MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		for (uint y = 1; y < 63; ++y) {
			for (uint x = 1; x < 63; ++x) {
				const TileIndex tile = TileXYL(x, y, layer);
				const auto hardness = GetExcavationCellHardness(geology, tile);
				const auto index = to_underlying(hardness);
				mismatches += hardness != GetExcavationCellHardness(restored, tile);
				mismatches += GetMultiLayerTerrainSprite(tile) != GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardness, {static_cast<uint8_t>(index * 4), 0, 0, 0, 0});
				if (seen[index]) continue;
				seen[index] = true;
				const std::array cells{tile};
				const auto quote = QuoteExcavationCells(cells, 1);
				REQUIRE(quote.error == ExcavationError::None);
				CHECK(quote.earth_t == earth[index]);
				CHECK(quote.rock_t == rock[index]);
				CHECK(quote.work_ticks == days[index] * 74);
				if (layer == MapLayer::Underground && hardness == GeologyHardness::Basalt) CHECK(QuoteExcavationCells(cells, 0).error == ExcavationError::InvalidPolicy);
			}
		}
	}
	CHECK(mismatches == 0);
	CHECK(std::ranges::count(seen, true) >= 3);
	CHECK(ComputeGeologyStateDigest(geology) == before);
	for (const auto &deposit : geology.deposits.Records()) {
		const auto &span = deposit.spans.front();
		CHECK(GetExcavationCellHardness(geology, TileXYL(span.first_x, span.y, deposit.layer)) == deposit.hardness);
	}
	// Refuse unknown or hidden regional fields before replacing a valid loaded
	// candidate, including when the legacy generator itself remains supported.
	for (uint mode = 0; mode < 3; ++mode) {
		auto bad = original;
		if (mode == 0) bad.hardness_version = GEOLOGY_HARDNESS_VERSION + 1;
		if (mode == 1) { bad.hardness_version = 0; bad.hardness_seed = 1; }
		if (mode == 2) bad.generator_version = 0;
		CHECK(PrepareGeologyRestoration(bad, restored) == GeologyError::InvalidShape);
		CHECK(ComputeGeologyStateDigest(restored) == before);
	}
}

/** Exposed walls inherit the neighboring material even across a region boundary; floor color cannot replace rock authority. */
TEST_CASE("Regional exposed wall reads its adjacent solid material", "[geology-regional]")
{
	ExcavationFixture fixture(false, GEOLOGY_HARDNESS_VERSION);
	TileIndex open = INVALID_TILE, neighbor = INVALID_TILE;
	const auto &geology = GetGeologyWorldState();
	for (uint y = 2; y < 62 && open == INVALID_TILE; ++y) {
		for (uint x = 2; x < 62; ++x) {
			const auto tile = TileXYL(x, y, MapLayer::DeepUnderground);
			const auto adjacent = TileAddWrap(tile, 0, -1);
			if (GetExcavationCellHardness(geology, tile) == GetExcavationCellHardness(geology, adjacent)) continue;
			open = tile; neighbor = adjacent; break;
		}
	}
	REQUIRE(open != INVALID_TILE);
	const auto expected = GetExcavationCellHardness(geology, neighbor);
	auto snapshot = CaptureGeologyState();
	GeologyWorldState candidate;
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	REQUIRE(candidate.terrain.Set(open, ExcavationState::Excavated));
	// Exact cavity state is staged solely through the fixture's load owner;
	// selectors never receive write access to the actual geological authority.
	StageLoadedGeologyState(std::move(candidate));
	AcceptLoadedGeologyState();
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto edges = GetMultiLayerTerrainEdgeSprites(open);
	CHECK(edges[0] == GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardnessEdges, {static_cast<uint8_t>(to_underlying(expected) * 4), 0, 0, 0, 0}));
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK_FALSE(GetMultiLayerTerrainEdgeSprites(INVALID_TILE)[0]);
}
