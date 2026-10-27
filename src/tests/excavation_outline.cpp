/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file excavation_outline.cpp Canonical irregular/branch/diagonal floors and explicit extra-space cost/refusal fixtures.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 12
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 11 (yields follow the halved cut terms)
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"

#include "../safeguards.h"

/** Different UI insertion orders/duplicates describe the same final floor, including diagonal and branched reachable cells. */
TEST_CASE("Excavation outlines normalize arbitrary connected floor without final-use restrictions", "[excavation-outline]")
{
	ExcavationFixture fixture;
	const auto a = TileXYL(20, 20, MapLayer::Underground);
	const auto b = TileXYL(21, 20, MapLayer::Underground);
	const auto c = TileXYL(20, 21, MapLayer::Underground);
	const auto d = TileXYL(21, 22, MapLayer::Underground);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto first = NormalizeExcavationOutline(MapLayer::Underground, std::array{d, a, c, b, a});
	const auto second = NormalizeExcavationOutline(MapLayer::Underground, std::array{a, b, c, d});
	REQUIRE(first.error == ExcavationError::None);
	CHECK(first.requested_tiles == second.requested_tiles);
	CHECK(first.requested_tiles == first.authorized_tiles);
	CHECK(first.extra_tiles.empty());
	CHECK(first.requested_tiles.size() == 4);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	const auto disconnected = NormalizeExcavationOutline(MapLayer::Underground, std::array{a, TileXYL(30, 30, MapLayer::Underground)});
	CHECK(disconnected.error == ExcavationError::InvalidRoute);
	CHECK(disconnected.failure_tile == TileXYL(30, 30, MapLayer::Underground));
	CHECK(disconnected.authorized_tiles.empty());
	CHECK(NormalizeExcavationOutline(MapLayer::Underground, {}).error == ExcavationError::InvalidRoute);
	CHECK(NormalizeExcavationOutline(MapLayer::Underground, std::array{a, TileXYL(20, 20, MapLayer::Surface)}).error == ExcavationError::MixedLayers);
	CHECK(NormalizeExcavationOutline(MapLayer::Underground, std::array{TileXYL(63, 20, MapLayer::Underground)}).error == ExcavationError::InvalidTile);
	CHECK(NormalizeExcavationOutline(MapLayer::Underground, std::array{INVALID_TILE}).error == ExcavationError::InvalidTile);
}

/** Only explicitly accepted extra cells enter the working domain, and their cost comes from actual shared geology terms. */
TEST_CASE("Excavation outline extra work is explicit charged and read-only", "[excavation-outline-authorization]")
{
	ExcavationFixture fixture;
	const auto requested = TileXYL(20, 20, MapLayer::Underground);
	const auto extra = TileXYL(21, 20, MapLayer::Underground);
	const auto base = NormalizeExcavationOutline(MapLayer::Underground, std::array{requested});
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto accepted = AuthorizeExcavationOutline(base, std::array{extra, extra}, 0);
	REQUIRE(accepted.error == ExcavationError::None);
	CHECK(accepted.requested_tiles == base.requested_tiles);
	CHECK(accepted.authorized_tiles == std::vector<TileIndex>{requested, extra});
	CHECK(accepted.extra_tiles == std::vector<TileIndex>{extra});
	CHECK(accepted.extra_estimate.new_cells == 1);
	// One Soft (hardness 0) cell yields 3 t earth and 6 t rock since the 2026-10-09 halving (r2tbmf-tbm-fixes.md line 37).
	CHECK(accepted.extra_estimate.earth_t == 3);
	CHECK(accepted.extra_estimate.rock_t == 6);
	CHECK(accepted.extra_estimate.cost == 30);
	CHECK(accepted.extra_estimate.work_ticks == 74);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(AuthorizeExcavationOutline(base, {}, 0).authorized_tiles == base.requested_tiles);
	CHECK(AuthorizeExcavationOutline(base, std::array{TileXYL(30, 30, MapLayer::Underground)}, 0).error == ExcavationError::InvalidRoute);
	CHECK(AuthorizeExcavationOutline(base, std::array{extra}, 2).error == ExcavationError::InvalidPolicy);
	CHECK(QuoteExcavationCells(std::array{INVALID_TILE}, 0).error == ExcavationError::InvalidTile);
	fixture.Open(std::array{extra});
	CHECK(AuthorizeExcavationOutline(base, std::array{extra}, 0).extra_estimate.cost == 0);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK(NormalizeExcavationOutline(MapLayer::DeepUnderground, std::array{TileXYL(20, 20, MapLayer::DeepUnderground)}).error == ExcavationError::LockedDeep);
}
