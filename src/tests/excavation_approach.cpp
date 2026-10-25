/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file excavation_approach.cpp The geology planner's approach producer (demo report R8). A floor beside the exit, behind
 * the housing or diagonally away needs working space beyond the floor, its one-cell ring and the housing: the approach
 * from the housing and the turning space a 3x3 cutter needs. PlanExcavationApproach finds that space inside a bounded
 * box, returns it as explicit extra cells, refuses with the blocking tile when nothing fits, and changes nothing.
 * Under the rule forbidding 90-degree turns, the support of every planned route avoids a 90-degree bend.
 * TBM quote/command consumer cases follow the separate approach-owner and support checks.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23CB
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "solid_starter_fixture.h"
#include "excavation_claim_fixture.h"
#include "../tbm_area_preview.h"
#include "../tbm.h"
#include "../road_cmd.h"
#include "../settings_type.h"

#include "../safeguards.h"

/** Underground cell of the fixture map. @param x Column. @param y Row. @return The tile. */
static TileIndex U(int x, int y)
{
	return TileXYL(x, y, MapLayer::Underground);
}

/** Real Sealed rail pair at (20,20) with its door facing SE (+y), as the TBM commission quote sees it. */
class ApproachRig {
public:
	SolidStarterFixture site; ///< Real pair, company and geology.
	LayerPortalBootstrapProof proof; ///< Portal-issued housing, exit heading and pocket.

	/** Issue the proof the commission quote starts from. */
	ApproachRig()
	{
		this->proof = IssueLayerPortalBootstrap(this->site.request);
		REQUIRE(this->proof.error == LayerPortalWorkError::None);
		REQUIRE(this->proof.exit_heading == Direction::SE);
	}

	/**
	 * Cutter in the housing, facing the door, with the whole consist still in the shaft.
	 * @param points Planning history points of the consist.
	 * @return The commission start pose.
	 */
	ExcavationPlanningPose Start(size_t points = 4) const
	{
		return SolidStarterFixture::StartPose(this->proof, points);
	}

	/**
	 * The rectangle a player drags, with the ring and housing the commission quote authorizes today: the same cells as
	 * MakeTBMWorkOutline, but without its refusal of a floor that does not touch the housing (Group 9 lifts that).
	 * @param x0 First column. @param y0 First row. @param x1 Last column. @param y1 Last row.
	 * @return Base outline: the requested floor and its possibly disconnected authorization.
	 */
	ExcavationOutline Area(int x0, int y0, int x1, int y1) const
	{
		std::vector<TileIndex> floor;
		std::vector<TileIndex> work;
		// Row-major floor without the housing, then each floor cell's and the housing's 3x3, like the TBM builder.
		for (int y = y0; y <= y1; ++y) {
			for (int x = x0; x <= x1; ++x) {
				if (U(x, y) != this->proof.destination) floor.push_back(U(x, y));
			}
		}
		std::vector<TileIndex> centres = floor;
		centres.push_back(this->proof.destination);
		for (TileIndex centre : centres) {
			for (int dy = -1; dy <= 1; ++dy) {
				for (int dx = -1; dx <= 1; ++dx) work.push_back(TileAddWrap(centre, dx, dy));
			}
		}
		std::ranges::sort(work);
		work.erase(std::unique(work.begin(), work.end()), work.end());
		auto outline = NormalizeExcavationOutline(MapLayer::Underground, floor);
		REQUIRE(outline.error == ExcavationError::None);
		outline.authorized_tiles = work;
		// Where the floor touches the housing, these are exactly the real builder's cells.
		const auto real = MakeTBMWorkOutline(this->proof.destination, U(x0, y0), U(x1, y1));
		if (real.error == ExcavationError::None) CHECK(real.authorized_tiles == outline.authorized_tiles);
		return outline;
	}

	/**
	 * Plan the approach of a fresh commission from this housing.
	 * @param outline Base outline.
	 * @param limits Search limits.
	 * @param exact Whether the cutter centre must reach the goal.
	 * @param points Planning history points.
	 * @return The proposal.
	 */
	ExcavationApproachProposal Plan(const ExcavationOutline &outline, const ExcavationPlanningLimits &limits = {}, bool exact = false, size_t points = 4) const
	{
		return PlanExcavationApproach(this->Start(points), outline, 0, limits, ExcavationJobID{UINT32_MAX}, true, exact);
	}
};

/**
 * Require an accepted approach to be a legal, explicit proposal: it leaves straight out of the door, every advance
 * turns at most 45 degrees, its extra cells are canonical and outside the base, and every swept cell is authorized by
 * the base plus those extra cells.
 * @param rig Housing.
 * @param outline Base outline.
 * @param approach Accepted proposal.
 */
static void CheckLegalApproach(const ApproachRig &rig, const ExcavationOutline &outline, const ExcavationApproachProposal &approach)
{
	REQUIRE(approach.error == ExcavationError::None);
	REQUIRE_FALSE(approach.route.empty());
	CHECK(approach.route.front().heading == rig.proof.exit_heading);
	CHECK(approach.end.position == approach.route.back().cutter_position);
	Direction heading = rig.proof.exit_heading;
	// Every cut advances one tile from the one before, turning by at most 45 degrees.
	TileIndex previous = rig.proof.destination;
	for (const auto &cut : approach.route) {
		const uint turn = (to_underlying(cut.heading) - to_underlying(heading) + 8) % 8;
		CHECK((turn == 0 || turn == 1 || turn == 7));
		CHECK(DistanceMax(cut.cutter_position, previous) == 1);
		heading = cut.heading;
		previous = cut.cutter_position;
	}
	CHECK(std::ranges::is_sorted(approach.extra_tiles));
	CHECK(std::adjacent_find(approach.extra_tiles.begin(), approach.extra_tiles.end()) == approach.extra_tiles.end());
	// Extra cells are exactly the swept cells the base does not authorize already.
	for (TileIndex tile : approach.extra_tiles) CHECK_FALSE(std::ranges::binary_search(outline.authorized_tiles, tile));
	for (const auto &cut : approach.route) {
		for (TileIndex tile : cut.swept_tiles) {
			CHECK((std::ranges::binary_search(outline.authorized_tiles, tile) || std::ranges::binary_search(approach.extra_tiles, tile)));
		}
	}
}

/**
 * The outline Group 9 will hand to the coverage planner: the base plus the approach's extra cells.
 * @param outline Base outline.
 * @param approach Accepted proposal.
 * @return Authorized outline with its quote.
 */
static ExcavationOutline WithApproach(const ExcavationOutline &outline, const ExcavationApproachProposal &approach)
{
	std::vector<TileIndex> extra = outline.authorized_tiles;
	extra.insert(extra.end(), approach.extra_tiles.begin(), approach.extra_tiles.end());
	return AuthorizeExcavationOutline(outline, extra, 0);
}

/**
 * The route as a job from this housing, for the TBM layer's support-rail turn rule.
 * @param rig Housing.
 * @param route Cuts.
 * @return Job geometry only.
 */
static ExcavationJob RouteJob(const ApproachRig &rig, const std::vector<ExcavationCutGeometry> &route)
{
	ExcavationJob job;
	job.company = _current_company;
	job.commissioning_anchor = rig.proof.destination;
	job.route = route;
	return job;
}

/** Screenshot 05: a strip beside the door. Today the planner refuses it; the approach supplies the turning space. */
TEST_CASE("A floor beside the exit gets its turning space as charged extra cells", "[excavation-approach]")
{
	ApproachRig rig;
	// The demo's (13,11)-(20,11) beside the door of (13,10): here the door of (20,20) faces SE, so the strip is row 21.
	const auto outline = rig.Area(20, 21, 27, 21);
	const auto today = PlanExcavationRoute(outline, rig.Start(), 0);
	CHECK(today.error == ExcavationError::OutsideAuthorization); // R8, reproduced: the turn needs cells outside the ring.

	const auto approach = rig.Plan(outline);
	CheckLegalApproach(rig, outline, approach);
	CHECK_FALSE(approach.extra_tiles.empty());
	// The solid start's first cut leaves straight through the door and sweeps the whole unopened pocket.
	CHECK(std::ranges::includes(approach.route.front().swept_tiles, rig.proof.pocket));
	UNSCOPED_INFO("cuts " << approach.route.size() << " extra " << approach.extra_tiles.size() << " expansions " << approach.expansions);
	CHECK(approach.expansions <= 2 * ExcavationPlanningLimits{}.expansions);

	// With those cells authorized, the unchanged coverage planner covers the whole strip from the housing.
	const auto authorized = WithApproach(outline, approach);
	REQUIRE(authorized.error == ExcavationError::None);
	const auto covered = PlanExcavationRoute(authorized, rig.Start(), 0);
	CHECK(covered.error == ExcavationError::None);
	CHECK_FALSE(covered.partial);
	CHECK(covered.remaining_targets.empty());
}

/** A floor behind the housing needs a full U-turn after the straight exit. */
TEST_CASE("A floor behind the housing is reached through a U-turn", "[excavation-approach]")
{
	ApproachRig rig;
	const auto outline = rig.Area(19, 14, 21, 15);
	CHECK(PlanExcavationRoute(outline, rig.Start(), 0).error != ExcavationError::None);
	const auto approach = rig.Plan(outline);
	CheckLegalApproach(rig, outline, approach);
	UNSCOPED_INFO("cuts " << approach.route.size() << " extra " << approach.extra_tiles.size() << " expansions " << approach.expansions);
	// The cutter turned back: its last heading points away from the door, towards the floor.
	CHECK(TilePlaneY(approach.end.position) < TilePlaneY(rig.proof.destination));
	const auto covered = PlanExcavationRoute(WithApproach(outline, approach), rig.Start(), 0);
	CHECK(covered.error == ExcavationError::None);
	CHECK(covered.remaining_targets.empty());
}

/** A floor diagonally away from the door is reached through 45-degree bends. */
TEST_CASE("A diagonally offset floor is reached through 45-degree bends", "[excavation-approach]")
{
	ApproachRig rig;
	const auto outline = rig.Area(25, 26, 27, 27);
	const auto approach = rig.Plan(outline);
	CheckLegalApproach(rig, outline, approach);
	UNSCOPED_INFO("cuts " << approach.route.size() << " extra " << approach.extra_tiles.size() << " expansions " << approach.expansions);
	// Some cut runs across the map's axes (an N, E, S or W heading; OpenTTD calls the axis headings "diagonal").
	CHECK(std::ranges::any_of(approach.route, [](const ExcavationCutGeometry &cut) { return !IsDiagonalDirection(cut.heading); }));
	const auto covered = PlanExcavationRoute(WithApproach(outline, approach), rig.Start(), 0);
	CHECK(covered.error == ExcavationError::None);
	CHECK(covered.remaining_targets.empty());

	// A floor that the ring and housing already let the cutter cover needs nothing.
	const auto ahead = rig.Area(19, 22, 21, 26);
	REQUIRE(PlanExcavationRoute(ahead, rig.Start(), 0).error == ExcavationError::None);
	const auto none = rig.Plan(ahead);
	CHECK(none.error == ExcavationError::None);
	CHECK(none.route.empty());
	CHECK(none.extra_tiles.empty());
	CHECK(none.end.position == rig.proof.destination);
}

/** A tunnel end off the exit axis: the cutter centre stands exactly on it. */
TEST_CASE("An exact goal puts the cutter centre on the requested cell", "[excavation-approach]")
{
	ApproachRig rig;
	const TileIndex goal = U(24, 26);
	const auto outline = NormalizeExcavationOutline(MapLayer::Underground, std::array{goal});
	REQUIRE(outline.error == ExcavationError::None);
	const auto approach = rig.Plan(outline, {}, true);
	CheckLegalApproach(rig, outline, approach);
	UNSCOPED_INFO("cuts " << approach.route.size() << " extra " << approach.extra_tiles.size() << " expansions " << approach.expansions);
	CHECK(approach.end.position == goal);
	// The area form only needs the body over the cell, so it may stop beside it.
	const auto area = rig.Plan(outline);
	REQUIRE(area.error == ExcavationError::None);
	CHECK(DistanceMax(area.end.position, goal) <= 1);
}

/** The 90-degree turn rule for one test, restored even when a check fails. */
struct NinetyDegreeRule {
	bool old = _settings_game.pf.forbid_90_deg; ///< Caller's setting.
	/** @param forbid Whether trains may not turn 90 degrees. */
	explicit NinetyDegreeRule(bool forbid) { _settings_game.pf.forbid_90_deg = forbid; }
	/** Restore the caller's setting. */
	~NinetyDegreeRule() { _settings_game.pf.forbid_90_deg = this->old; }
};

/**
 * R8's second finding: where 90-degree turns are forbidden, door-anchored floors were refused for sharp support bends.
 * The support path now passes the side cell that avoids such a bend (r2df Group 9), so every approach and coverage
 * route the planner proposes has drivable support.
 */
TEST_CASE("Approach and coverage routes have drivable support under the 90-degree rule", "[excavation-approach]")
{
	ApproachRig rig;
	NinetyDegreeRule forbid(true);
	ExcavationOutline outline;
	SECTION("Screenshot 05: the strip beside the door") { outline = rig.Area(20, 21, 27, 21); }
	SECTION("Behind the housing") { outline = rig.Area(19, 14, 21, 15); }
	SECTION("R8: a floor from the door widening to the right") { outline = rig.Area(20, 21, 23, 28); }
	const auto approach = rig.Plan(outline);
	REQUIRE(approach.error == ExcavationError::None);
	UNSCOPED_INFO("cuts " << approach.route.size() << " extra " << approach.extra_tiles.size() << " expansions " << approach.expansions);
	if (!approach.route.empty()) {
		CheckLegalApproach(rig, outline, approach);
		CHECK(CheckTBMSupportRailTurns(RouteJob(rig, approach.route)));
	}
	// The coverage route inside the floor, its ring, the housing and the approach has drivable support too.
	const auto covered = PlanExcavationRoute(WithApproach(outline, approach), rig.Start(), 0);
	CHECK(covered.error == ExcavationError::None);
	CHECK(covered.remaining_targets.empty());
	CHECK(CheckTBMSupportRailTurns(RouteJob(rig, covered.route)));
}

/** Something in the way: the refusal names the blocking tile and keeps no partial approach. */
TEST_CASE("An impossible approach names its blocking tile and proposes nothing", "[excavation-approach]")
{
	ApproachRig rig;
	const auto outline = rig.Area(20, 21, 27, 21);
	ExcavationError expected = ExcavationError::None;
	std::vector<TileIndex> blocking;
	SECTION("An owned building in front of the door") {
		// Every route starts with the straight exit through (20,22); a road there blocks them all.
		rig.site.world.Open(std::array{U(20, 22)});
		ResetRoadTypes();
		REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, U(20, 22), ROAD_Y, ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
		expected = ExcavationError::Conflict;
		blocking = {U(20, 22)};
	}
	SECTION("Another project's claimed work across the exit") {
		// A reserved job beside the exit claims (19..20, 23..25): every advance after the exit sweeps (19,23) or (20,23).
		ClaimSeparateExcavationWork(rig.site.world, U(17, 24));
		expected = ExcavationError::ActiveCut;
		for (const auto &[tile, job] : GetGeologyWorldState().job_claims) blocking.push_back(tile);
	}
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto approach = rig.Plan(outline);
	CHECK(approach.error == expected);
	CHECK(std::ranges::find(blocking, approach.failure_tile) != blocking.end());
	CHECK(approach.route.empty());
	CHECK(approach.extra_tiles.empty());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
}

/** The map edge: a cutter facing the edge one row away cannot advance at all. */
TEST_CASE("An approach that would leave the map is refused at the edge", "[excavation-approach]")
{
	ExcavationFixture world;
	// An open start at (30,1) facing NW (towards row 0) with its consist behind it, and a floor behind the consist.
	std::vector<TileIndex> open;
	for (int y = 0; y <= 4; ++y) {
		for (int x = 29; x <= 31; ++x) open.push_back(U(x, y));
	}
	world.Open(open);
	const ExcavationPlanningPose start{U(30, 1), Direction::NW, {U(30, 1), U(30, 2), U(30, 3)}};
	const auto floor = NormalizeExcavationOutline(MapLayer::Underground, std::array{U(30, 8)});
	const auto approach = PlanExcavationApproach(start, floor, 0, {}, ExcavationJobID{UINT32_MAX}, false, false);
	CHECK(approach.error == ExcavationError::InvalidTile);
	REQUIRE(approach.failure_tile != INVALID_TILE);
	CHECK(TilePlaneY(approach.failure_tile) == 0); // The first row a cut would leave the map through.
	CHECK(approach.route.empty());
}

/** Refusals that no search can change: wrong layer, locked Deep, unsafe limits, an exhausted budget. */
TEST_CASE("Approach planning refuses layers, locked Deep, unsafe limits and an exhausted budget", "[excavation-approach]")
{
	ApproachRig rig;
	const auto outline = rig.Area(20, 21, 27, 21);
	CHECK(rig.Plan(outline, {8193, 8192, 256, 1024}).error == ExcavationError::Limit);
	CHECK(rig.Plan(outline, {4096, 0, 256, 1024}).error == ExcavationError::Limit);

	// A cutter in a solid housing that does not face the door cannot leave it, however wide the box.
	auto sideways = rig.Start();
	sideways.heading = Direction::SW;
	CHECK(PlanExcavationApproach(sideways, outline, 0, {}, ExcavationJobID{UINT32_MAX}, true, false).error == ExcavationError::IllegalTurn);
	// The housing is still solid, so a caller that calls it open ground is refused.
	CHECK(PlanExcavationApproach(rig.Start(), outline, 0, {}, ExcavationJobID{UINT32_MAX}, false, false).error == ExcavationError::MissingSupport);

	// A Surface floor has no rock to plan in.
	ExcavationOutline surface;
	surface.layer = MapLayer::Surface;
	surface.requested_tiles = {TileXYL(24, 24, MapLayer::Surface)};
	CHECK(rig.Plan(surface).error == ExcavationError::MixedLayers);

	// Deep Underground before its opening date.
	ExcavationOutline deep;
	deep.layer = MapLayer::DeepUnderground;
	deep.requested_tiles = {TileXYL(24, 24, MapLayer::DeepUnderground)};
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	const ExcavationPlanningPose deep_start{TileXYL(20, 20, MapLayer::DeepUnderground), Direction::SE, std::vector<TileIndex>(3, TileXYL(20, 20, MapLayer::DeepUnderground))};
	CHECK(PlanExcavationApproach(deep_start, deep, 1, {}, ExcavationJobID{UINT32_MAX}, false, false).error == ExcavationError::LockedDeep);

	// A budget too small for the turn: a Limit refusal with no partial cells, within twice the budget
	// (the attempt inside the base outline, then the one in the wider box).
	const auto limited = rig.Plan(outline, {4096, 12, 256, 1024});
	CHECK(limited.error == ExcavationError::Limit);
	CHECK(limited.route.empty());
	CHECK(limited.extra_tiles.empty());
	CHECK(limited.expansions <= 24);
	UNSCOPED_INFO("limited expansions " << limited.expansions);
	CHECK(limited.failure_tile != INVALID_TILE);
}

/** Two identical requests give identical proposals and leave terrain, claims, portals, money and Random() unchanged. */
TEST_CASE("Approach planning is deterministic and pure", "[excavation-approach]")
{
	ApproachRig rig;
	const auto outline = rig.Area(20, 21, 27, 21);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto portals = ComputeLayerPortalWorkDigest();
	const auto money = Company::Get(_current_company)->money;
	const auto rng = _random;
	const auto first = rig.Plan(outline);
	const auto second = rig.Plan(outline);
	REQUIRE(first.error == ExcavationError::None);
	CHECK(second.error == first.error);
	CHECK(second.expansions == first.expansions);
	CHECK(second.extra_tiles == first.extra_tiles);
	CHECK(second.end.position == first.end.position);
	CHECK(second.end.heading == first.end.heading);
	CHECK(second.end.history == first.end.history);
	REQUIRE(second.route.size() == first.route.size());
	// Whole cut geometry, not only the end, is identical.
	for (size_t i = 0; i < first.route.size(); ++i) CHECK(second.route[i] == first.route[i]);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeLayerPortalWorkDigest() == portals);
	CHECK(GetGeologyWorldState().job_claims.empty());
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(_random.state[0] == rng.state[0]);
	CHECK(_random.state[1] == rng.state[1]);
}
