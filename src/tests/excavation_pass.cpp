/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_pass.cpp Safe backing, frozen input and complete construction/viewport fixtures. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Groups 14/16 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../road_cmd.h"
#include "../road.h"
#include "../table/strings.h"
#include "../viewport_func.h"
#include "../viewport_type.h"
#include "../landscape.h"
#include "../rail.h"
#include "../rail_cmd.h"
#include "../tile_map.h"

#include "../safeguards.h"

/** Actual initialized world and explicitly supported two-cut forward proposal. */
class PassFixture {
public:
	ExcavationFixture world; ///< Actual terrain/company/profile owners.
	ExcavationJob job; ///< Canonical explicit work input, without invented vehicle/station references.
	/**
	 * Open the starter envelope and construct two legal advancing cutter sweeps.
	 * @param layer Actual rock plane for this fixture's job and supporting cells.
	 */
	explicit PassFixture(MapLayer layer = MapLayer::Underground)
	{
		job.company = _current_company;
		job.model_id = layer == MapLayer::DeepUnderground ? 1 : 0;
		job.commissioning_anchor = TileXYL(20, 20, layer);
		job.requested_tiles = {TileXYL(22, 20, layer)};
		std::vector<TileIndex> starter;
		// The real starting cutter envelope is already open; each future cut adds exactly one three-cell face.
		for (int y = 19; y <= 21; ++y) {
			for (int x = 19; x <= 21; ++x) starter.push_back(TileXYL(x, y, layer));
			for (int x = 19; x <= 23; ++x) job.authorized_tiles.push_back(TileXYL(x, y, layer));
		}
		world.Open(starter);
		// Each supported forward pose is explicit and retains its own complete authorized footprint.
		for (int x = 21; x <= 22; ++x) {
			ExcavationCutGeometry cut;
			cut.cutter_position = TileXYL(x, 20, layer);
			cut.heading = Direction::SW;
			cut.support_tiles = {job.commissioning_anchor};
			for (int y = 19; y <= 21; ++y) {
				for (int dx = -1; dx <= 1; ++dx) cut.swept_tiles.push_back(TileXYL(x + dx, y, layer));
			}
			job.route.push_back(std::move(cut));
		}
	}
};

/** Controlled withdrawal signal, distinct from the geometry owner's actual floor state. */
class PassWithdrawal final : public ExcavationWithdrawalParticipant {
public:
	bool withdrawn = false; ///< Actual fixture actor-owner signal.
	/**
	 * Report both actors' withdrawal without mutating support.
	 * @param job Exact accepted job being considered.
	 * @param removed_support Exact proposed support removal footprint.
	 * @return Explicit actor-owner permission.
	 */
	bool HasWithdrawn([[maybe_unused]] const ExcavationJob &job, [[maybe_unused]] std::span<const TileIndex> removed_support) const override { return withdrawn; }
};

/** Complete-prefix capacity splitting retains loaded backing and the original cutter headings. */
TEST_CASE("Excavation passes split only at supported complete-cut returns", "[excavation-pass]")
{
	PassFixture fixture;
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto rng = _random;
	ExcavationPassTerms terms;
	terms.earth_capacity_t = 18;
	terms.rock_capacity_t = 36;
	const auto prepared = PrepareExcavationPass(fixture.job, terms);
	REQUIRE(prepared.error == ExcavationError::None);
	CHECK(prepared.terms.exchange_after == std::vector<uint16_t>{1, 2});
	fixture.job.pass_terms = prepared.terms;
	CHECK(GetExcavationRetreatPath(fixture.job, 2) == std::vector<TileIndex>{fixture.job.route[1].cutter_position, fixture.job.route[0].cutter_position, fixture.job.commissioning_anchor});
	CHECK(fixture.job.route[1].heading == Direction::SW);
	terms.earth_capacity_t = 17;
	CHECK(PrepareExcavationPass(fixture.job, terms).error == ExcavationError::InsufficientCapacity);
	terms.earth_capacity_t = 18; terms.rock_capacity_t = 35;
	CHECK(PrepareExcavationPass(fixture.job, terms).error == ExcavationError::InsufficientCapacity);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(_random.state[0] == rng.state[0]);
	CHECK(_random.state[1] == rng.state[1]);
}

/** Actual reservation freezes limits, refuses bad input unchanged and round-trips it through the real snapshot owner. */
TEST_CASE("Excavation reservation retains validated pass input", "[excavation-pass]")
{
	PassFixture fixture;
	fixture.job.pass_terms = ExcavationPassTerms{};
	fixture.job.pass_terms->rock_capacity_t = 35;
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(ReserveExcavationJob(fixture.job, true).error == ExcavationError::InsufficientCapacity);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	fixture.job.pass_terms->rock_capacity_t = 36;
	fixture.job.pass_terms->earth_capacity_t = 18;
	const auto reserved = ReserveExcavationJob(fixture.job, true);
	REQUIRE(reserved.error == ExcavationError::None);
	const auto *job = GetExcavationJob(reserved.id);
	REQUIRE(job->pass_terms);
	CHECK(job->pass_terms->exchange_after == std::vector<uint16_t>{1, 2});
	const auto snapshot = CaptureGeologyState();
	GeologyWorldState restored;
	REQUIRE(PrepareGeologyRestoration(snapshot, restored, true) == GeologyError::None);
	CHECK(restored.jobs[0].pass_terms->fingerprint == job->pass_terms->fingerprint);
	auto changed = snapshot;
	++changed.jobs[0].pass_terms->rock_capacity_t;
	CHECK(PrepareGeologyRestoration(changed, restored, true) != GeologyError::None);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) != before);
}

/** Tube conversion requires actual withdrawal and an open retained alternative outside removed support. */
TEST_CASE("Excavation support replacement requires withdrawal and retained backing", "[excavation-pass]")
{
	PassFixture fixture;
	fixture.job.pass_terms = PrepareExcavationPass(fixture.job, ExcavationPassTerms{}).terms;
	PassWithdrawal actor;
	const std::array removed{TileXYL(21, 20, MapLayer::Underground)};
	const std::array alternative{fixture.job.commissioning_anchor};
	CHECK(CheckExcavationSupportReplacement(fixture.job, removed, alternative, actor).error == ExcavationError::Conflict);
	actor.withdrawn = true;
	CHECK(CheckExcavationSupportReplacement(fixture.job, removed, alternative, actor).error == ExcavationError::None);
	CHECK(CheckExcavationSupportReplacement(fixture.job, alternative, alternative, actor).error == ExcavationError::MissingSupport);
	fixture.job.cursor = 9999;
	CHECK(CheckExcavationSupportReplacement(fixture.job, removed, alternative, actor).error == ExcavationError::InvalidJob);
}

/** A real road built on an unclaimed exchange approach invalidates execution without touching the reserved excavation. */
TEST_CASE("Excavation execution revalidates changed exchange approach", "[excavation-pass]")
{
	PassFixture fixture;
	const auto exchange = TileXYL(18, 20, MapLayer::Underground);
	ResetRoadTypes();
	fixture.world.Open(std::array{exchange});
	fixture.job.pass_terms = ExcavationPassTerms{};
	fixture.job.pass_terms->exchange_path = {fixture.job.commissioning_anchor, TileXYL(19, 20, MapLayer::Underground), exchange};
	const auto reserved = ReserveExcavationJob(fixture.job, true);
	REQUIRE(reserved.error == ExcavationError::None);
	REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, exchange, ROAD_X, ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto rng = _random;
	const auto *job = GetExcavationJob(reserved.id);
	const auto quote = PrepareExcavationCut({reserved.id, job->revision, job->cursor}, {0, 64, 128, 64, Company::Get(_current_company)->money});
	CHECK(quote.error == ExcavationError::Conflict);
	CHECK(quote.failure_tile == exchange);
	CHECK(quote.earth_t == 0);
	CHECK(quote.rock_t == 0);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(_random.state[0] == rng.state[0]);
	CHECK(_random.state[1] == rng.state[1]);
}

/** Already occupied backing refuses before claims/identity allocation; normal placement cannot hide behind a pure capacity preview. */
TEST_CASE("Excavation reservation refuses an already obstructed exchange", "[excavation-pass]")
{
	PassFixture fixture;
	ResetRoadTypes();
	const auto exchange = TileXYL(18, 20, MapLayer::Underground);
	fixture.world.Open(std::array{exchange});
	REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, exchange, ROAD_X, ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
	fixture.job.pass_terms = ExcavationPassTerms{};
	fixture.job.pass_terms->exchange_path = {fixture.job.commissioning_anchor, TileXYL(19, 20, MapLayer::Underground), exchange};
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(ReserveExcavationJob(fixture.job, false).error == ExcavationError::Conflict);
	CHECK(ReserveExcavationJob(fixture.job, true).error == ExcavationError::Conflict);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(GetGeologyWorldState().jobs.empty());
	CHECK(GetGeologyWorldState().job_claims.empty());
}

/** Actual tile-owner metadata remains authoritative when support ownership changes after the route was accepted. */
TEST_CASE("Excavation execution refuses transferred backing ownership", "[excavation-pass]")
{
	PassFixture fixture;
	ResetRailTypes();
	const auto exchange = TileXYL(18, 20, MapLayer::Underground);
	fixture.world.Open(std::array{exchange});
	REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, exchange, RAILTYPE_RAIL, Track::X, false).Succeeded());
	fixture.job.pass_terms = ExcavationPassTerms{};
	fixture.job.pass_terms->exchange_path = {fixture.job.commissioning_anchor, TileXYL(19, 20, MapLayer::Underground), exchange};
	const auto reserved = ReserveExcavationJob(fixture.job, true);
	REQUIRE(reserved.error == ExcavationError::None);
	REQUIRE(Company::CanAllocateItem());
	std::unique_ptr<Company> rival{Company::Create()};
	// This focused fixture changes the actual tile owner, without claiming company-merger/accounting integration.
	SetTileOwner(exchange, rival->index);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto *job = GetExcavationJob(reserved.id);
	const auto quote = PrepareExcavationCut({reserved.id, job->revision, job->cursor}, {0, 64, 128, 64, Company::Get(_current_company)->money});
	CHECK(quote.error == ExcavationError::InvalidOwner);
	CHECK(quote.failure_tile == exchange);
	CHECK(quote.earth_t == 0);
	CHECK(quote.rock_t == 0);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
}

/** Every construction family preserves layer rules and applies the same live solid/active/cavity authority. */
TEST_CASE("Excavation construction matrix covers every family and complete footprints", "[excavation-construction-matrix]")
{
	PassFixture fixture;
	const auto open = TileXYL(18, 20, MapLayer::Underground);
	const auto solid = TileXYL(30, 20, MapLayer::Underground);
	const auto active = TileXYL(22, 20, MapLayer::Underground);
	fixture.world.Open(std::array{open});
	REQUIRE(ReserveExcavationJob(fixture.job, true).error == ExcavationError::None);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	// Enumerating End covers future families too; excavation permission cannot broaden a layer's existing construction policy.
	for (uint8_t value = 0; value < to_underlying(MapLayerConstruction::End); ++value) {
		const auto family = static_cast<MapLayerConstruction>(value);
		const bool allowed = MultiLayerMap::GetRules(MapLayer::Underground).allowed.Test(family);
		CHECK(CheckLayerAllowsConstruction(solid, family).GetErrorMessage() == (allowed ? STR_ERROR_EXCAVATION_SOLID_ROCK : STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER));
		CHECK(CheckLayerAllowsConstruction(active, family).GetErrorMessage() == (allowed ? STR_ERROR_EXCAVATION_ACTIVE_CUT : STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER));
		CHECK(CheckLayerAllowsConstruction(open, family).Succeeded() == allowed);
	}
	const auto first = TileXYL(30, 25, MapLayer::Underground);
	const auto blocked = TileXYL(31, 25, MapLayer::Underground);
	fixture.world.Open(std::array{first});
	CHECK(CheckExcavationConstructionArea(first, 2, 1).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	fixture.world.Open(std::array{blocked});
	CHECK(CheckExcavationConstructionArea(first, 2, 1).Succeeded());
	CHECK(CheckExcavationConstructionArea(open, 5, 1).GetErrorMessage() == STR_ERROR_EXCAVATION_ACTIVE_CUT);
	CHECK(GetGeologyWorldState().jobs.size() == 1);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) != before); // Only the two explicitly opened fixture cells changed; all matrix calls were readonly.
}

/** Actual viewport inverse projection feeds the same world-tile construction command used by an extra view. */
TEST_CASE("Excavation placement through an independent viewport keeps the clicked layer", "[excavation-construction-matrix]")
{
	ExcavationFixture fixture;
	ResetRoadTypes();
	const auto tile = TileXYL(30, 20, MapLayer::Underground);
	const auto point = RemapCoords(TileX(tile) * TILE_SIZE + 8, TileY(tile) * TILE_SIZE + 8, 0);
	Viewport extra{};
	extra.left = 100; extra.top = 100; extra.width = 64; extra.height = 64;
	extra.virtual_left = point.x - 32; extra.virtual_top = point.y - 32;
	extra.virtual_width = 64; extra.virtual_height = 64;
	extra.zoom = ZoomLevel::In4x;
	extra.layer = MapLayer::Underground;
	const auto picked = TranslateXYToTileCoord(extra, 132, 132);
	REQUIRE(picked.x != -1);
	const auto clicked = TileVirtXY(picked.x, picked.y);
	CHECK(clicked == tile);
	CHECK(GetTileLayer(clicked) == MapLayer::Underground);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(CmdBuildRoad(DoCommandFlag::Execute, clicked, ROAD_X, ROADTYPE_ROAD, {}, TownID::Invalid()).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	fixture.Open(std::array{tile});
	CHECK(CmdBuildRoad(DoCommandFlag::Execute, clicked, ROAD_X, ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
	CHECK(GetTileLayer(clicked) == MapLayer::Underground);
}

/** The shared calendar gate refuses a geometrically valid Deep pass before its introduction without reservation or RNG effects. */
TEST_CASE("Excavation pre-1980 Deep jobs refuse before terrain and identity writes", "[excavation-date-boundary]")
{
	PassFixture fixture(MapLayer::DeepUnderground);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto rng = _random;
	CHECK(ReserveExcavationJob(fixture.job, false).error == ExcavationError::LockedDeep);
	CHECK(ReserveExcavationJob(fixture.job, true).error == ExcavationError::LockedDeep);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(GetGeologyWorldState().jobs.empty());
	CHECK(_random.state[0] == rng.state[0]);
	CHECK(_random.state[1] == rng.state[1]);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	CHECK(ReserveExcavationJob(fixture.job, true).error == ExcavationError::None);
}
