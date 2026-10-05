/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_route.cpp Real advancing bends, full diagonal/consist sweeps and bounded deterministic coverage fixtures. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 13 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../road.h"
#include "../road_cmd.h"

#include "../safeguards.h"

/** Actual supported fixture geometry; no actor, movement or hidden wagon capacity is invented by this planner. */
class RouteFixture {
public:
	ExcavationFixture world; ///< Actual initialized profile/company/geological owners.
	ExcavationPlanningPose start{TileXYL(20, 20, MapLayer::Underground), Direction::SW, {TileXYL(20, 20, MapLayer::Underground), TileXYL(19, 20, MapLayer::Underground), TileXYL(18, 20, MapLayer::Underground)}}; ///< Explicit head and two trailing occupied/support cells.
	/** Open only this fixture's actual starter/support floor before readonly planning begins. */
	RouteFixture()
	{
		std::vector<TileIndex> floor;
		// Conservative cutter occupancy and the trailing consist cell are real starter support, not newly opened planning output.
		for (int y = 19; y <= 21; ++y) {
			for (int x = 19; x <= 21; ++x) floor.push_back(TileXYL(x, y, MapLayer::Underground));
		}
		floor.push_back(TileXYL(18, 20, MapLayer::Underground));
		this->world.Open(floor);
	}
	/**
	 * Explicitly accept a bounded room/turn/approach domain around the test's final floor.
	 * @param requested User final-floor geometry, independent of its future infrastructure use.
	 * @return Actual normalized/charged planning proposal.
	 */
	ExcavationOutline Outline(std::span<const TileIndex> requested) const
	{
		const auto base = NormalizeExcavationOutline(MapLayer::Underground, requested);
		std::vector<TileIndex> extra;
		// This working-space rectangle is explicitly submitted; the route solver is never allowed to invent its margins.
		for (int y = 14; y <= 26; ++y) {
			for (int x = 17; x <= 31; ++x) extra.push_back(TileXYL(x, y, MapLayer::Underground));
		}
		return AuthorizeExcavationOutline(base, extra, 0);
	}
};

/** A 90-degree change needs two actual advancing 45-degree bends; diagonal corners and the complete tail remain in the sweep. */
TEST_CASE("Excavation planning bends advance and retain complete diagonal consist clearance", "[excavation-route-geometry]")
{
	RouteFixture fixture;
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto straight = AdvanceExcavationPlanningPose(fixture.start, Direction::SW);
	REQUIRE(straight.error == ExcavationError::None);
	CHECK(straight.next.position == TileXYL(21, 20, MapLayer::Underground));
	CHECK(std::binary_search(straight.cut.swept_tiles.begin(), straight.cut.swept_tiles.end(), TileXYL(18, 20, MapLayer::Underground)));
	CHECK(AdvanceExcavationPlanningPose(fixture.start, Direction::NW).error == ExcavationError::IllegalTurn);
	const auto first = AdvanceExcavationPlanningPose(fixture.start, Direction::W);
	REQUIRE(first.error == ExcavationError::None);
	CHECK(first.next.position == TileXYL(21, 19, MapLayer::Underground));
	CHECK(std::binary_search(first.cut.swept_tiles.begin(), first.cut.swept_tiles.end(), TileXYL(22, 21, MapLayer::Underground)));
	CHECK(std::binary_search(first.cut.swept_tiles.begin(), first.cut.swept_tiles.end(), TileXYL(19, 18, MapLayer::Underground)));
	const auto second = AdvanceExcavationPlanningPose(first.next, Direction::NW);
	REQUIRE(second.error == ExcavationError::None);
	CHECK(second.next.position == TileXYL(21, 18, MapLayer::Underground));
	CHECK(second.next.heading == Direction::NW);
	CHECK(second.next.history[1] == first.next.position);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
}

/** Stable target/neighbour/tie ordering produces the same bounded legal route and aggregate quote without live excavation. */
TEST_CASE("Excavation route search deterministically covers straight and irregular final floor", "[excavation-route-search]")
{
	RouteFixture fixture;
	std::vector<TileIndex> requested;
	SECTION("Straight target pass") { requested = {TileXYL(27, 20, MapLayer::Underground)}; }
	SECTION("Irregular chamber with a branch") {
		requested = {TileXYL(24, 19, MapLayer::Underground), TileXYL(25, 19, MapLayer::Underground), TileXYL(24, 20, MapLayer::Underground), TileXYL(25, 20, MapLayer::Underground), TileXYL(26, 21, MapLayer::Underground), TileXYL(27, 21, MapLayer::Underground)};
	}
	const auto outline = fixture.Outline(requested);
	REQUIRE(outline.error == ExcavationError::None);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto planned = PlanExcavationRoute(outline, fixture.start, 0);
	REQUIRE(planned.error == ExcavationError::None);
	CHECK_FALSE(planned.partial);
	CHECK(planned.remaining_targets.empty());
	CHECK(planned.covered_targets == outline.requested_tiles);
	CHECK(planned.route.size() <= MAX_EXCAVATION_ROUTE);
	CHECK(planned.expansions <= 8192);
	const auto repeated = PlanExcavationRoute(outline, fixture.start, 0);
	CHECK(repeated.expansions == planned.expansions);
	CHECK(repeated.end.position == planned.end.position);
	CHECK(repeated.end.heading == planned.end.heading);
	CHECK(repeated.estimate == planned.estimate);
	REQUIRE(repeated.route.size() == planned.route.size());
	// Full per-edge geometry equality proves stable tie/consist handling rather than merely equal endpoint centres.
	for (size_t i = 0; i < planned.route.size(); ++i) {
		CHECK(planned.route[i].cutter_position == repeated.route[i].cutter_position);
		CHECK(planned.route[i].heading == repeated.route[i].heading);
		CHECK(planned.route[i].swept_tiles == repeated.route[i].swept_tiles);
		CHECK(planned.route[i].support_tiles == repeated.route[i].support_tiles);
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
}

/** A point-only corridor cannot authorize the cutter envelope; exact deterministic limits refuse before any world or claim change. */
TEST_CASE("Excavation route rejects tight corners and visibly bounds impossible turn space", "[excavation-route-limits]")
{
	RouteFixture fixture;
	const auto target = TileXYL(27, 20, MapLayer::Underground);
	const auto outline = fixture.Outline(std::array{target});
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto limited = PlanExcavationRoute(outline, fixture.start, 0, {1, 1, 1, 1});
	CHECK(limited.error == ExcavationError::Limit);
	CHECK(limited.expansions <= 1);
	CHECK(limited.route.size() <= 1);
	CHECK(PlanExcavationRoute(outline, fixture.start, 0, {8193, 8192, 256, 1024}).error == ExcavationError::Limit);
	std::vector<TileIndex> tight;
	// Only a one-cell turn/corridor is accepted here; a legal head cannot silently widen its swept work beyond that domain.
	for (int x = 18; x <= 27; ++x) tight.push_back(TileXYL(x, 20, MapLayer::Underground));
	const auto point_domain = AuthorizeExcavationOutline(NormalizeExcavationOutline(MapLayer::Underground, std::array{target}), tight, 0);
	const auto refused = PlanExcavationRoute(point_domain, fixture.start, 0);
	CHECK(refused.error == ExcavationError::OutsideAuthorization);
	CHECK(refused.route.empty());
	CHECK_FALSE(refused.partial);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(GetGeologyWorldState().job_claims.empty());
}

/** A real occupied starter blocks every full cutter sweep; the search cannot delete existing infrastructure to escape it. */
TEST_CASE("Excavation route refuses occupied advancing clearance unchanged", "[excavation-route-boundary]")
{
	RouteFixture fixture;
	ResetRoadTypes();
	REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, fixture.start.position, ROAD_X, ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
	const auto outline = fixture.Outline(std::array{TileXYL(27, 20, MapLayer::Underground)});
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto rng = _random;
	const auto result = PlanExcavationRoute(outline, fixture.start, 0);
	CHECK(result.error == ExcavationError::Conflict);
	CHECK(result.route.empty());
	CHECK(GetGeologyWorldState().job_claims.empty());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(_random.state[0] == rng.state[0]);
	CHECK(_random.state[1] == rng.state[1]);
}
