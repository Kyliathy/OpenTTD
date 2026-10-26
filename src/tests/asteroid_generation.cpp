/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_generation.cpp Independent complete-grid, actual terrain and saved-input field acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
 */
#include "../stdafx.h"
#include "asteroid_snapshot_fixture.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../multilayer_asteroid_catalogue.h"
#include "../launch_site.h"
#include "../command_type.h"
#include "../map_layer.h"
#include "../clear_map.h"
#include "../water_map.h"
#include "../void_map.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../settings_type.h"
#include "../safeguards.h"

/**
 * Reproduce the established full-grid leading swaps using independent literal spacing and probe-offset defaults.
 * @param width Supported plane width.
 * @param height Supported plane height.
 * @param preference Explicit projected body-centre preference.
 * @return Complete-grid reference triple and actual approach bound, without using the optimized selector.
 */
static AsteroidStarterTriple CompleteGridReference(uint32_t width, uint32_t height, SpacePoint preference)
{
	std::vector<SpacePoint> grid;
	// Enumerate every admissible literal four-tile cell so edge/narrow-map tests can expose local-window omissions.
	for (int32_t y = (height / 4 + 2) * 65536; y <= static_cast<int32_t>(3 * height / 4 - 3) * 65536; y += 4 * 65536)
		for (int32_t x = 2 * 65536; x <= static_cast<int32_t>(width - 3) * 65536; x += 4 * 65536) grid.push_back({x, y});
	AsteroidStarterTriple result;
	// Match the established full-vector tie break; it is intentionally independent of the optimized local candidate list.
	for (size_t starter = 0; starter < 3; ++starter) {
		auto best = grid.begin() + starter;
		for (auto point = best; point != grid.end(); ++point)
			if (SpaceDistanceSquared(*point, preference) < SpaceDistanceSquared(*best, preference)) best = point;
		std::iter_swap(grid.begin() + starter, best);
		result.positions[starter] = grid[starter];
		const SpacePoint approach{grid[starter].x + 65536, grid[starter].y - 32768};
		result.approach_distance_squared = std::max(result.approach_distance_squared, SpaceDistanceSquared(approach, preference));
	}
	return result;
}

/** Check all small-plane tile/half-tile preferences and distant narrow/max-map edges against a full-grid oracle. */
TEST_CASE("Bounded asteroid starter selection matches the complete grid including distance ties", "[asteroid-generation]")
{
	// Exhaust small-map preferences inside and outside the band, including fractional ties and every map edge.
	for (int32_t y = 0; y < 64; ++y) for (int32_t x = 0; x < 64; ++x) for (int32_t fraction : {0, 32768}) {
		const SpacePoint preference{x * 65536 + fraction, y * 65536 + fraction};
		uint32_t examined = 0;
		const auto actual = GetAsteroidStarterTriple(64, 64, preference, &examined);
		REQUIRE(actual);
		const auto expected = CompleteGridReference(64, 64, preference);
		CHECK(actual->positions == expected.positions);
		CHECK(actual->approach_distance_squared == expected.approach_distance_squared);
		CHECK(examined <= 25);
	}
	// Far preferences need three cells along one boundary, rather than a too-small immediate-neighbor window.
	for (const auto dimensions : {std::pair{64U, 4096U}, std::pair{4096U, 64U}, std::pair{4096U, 4096U}}) {
		for (SpacePoint preference : {SpacePoint{0, 0}, SpacePoint{static_cast<int32_t>(dimensions.first - 1) * 65536, static_cast<int32_t>(dimensions.second - 1) * 65536}}) {
			const auto actual = GetAsteroidStarterTriple(dimensions.first, dimensions.second, preference);
			REQUIRE(actual);
			const auto expected = CompleteGridReference(dimensions.first, dimensions.second, preference);
			CHECK(actual->positions == expected.positions);
			CHECK(actual->approach_distance_squared == expected.approach_distance_squared);
		}
	}
	CHECK_FALSE(GetAsteroidStarterTriple(32, 64, {0, 0}));
	CHECK_FALSE(GetAsteroidStarterTriple(64, 63, {0, 0}));
	CHECK_FALSE(GetAsteroidStarterTriple(64, 64, {-1, 0}));
	CHECK_FALSE(GetAsteroidStarterTriple(64, 64, {64 * 65536, 0}));
}

/** Verify canonical profile-owned population/share/placement records against independent design literals. */
TEST_CASE("Asteroid population and placement records belong to the selected Industry profile", "[asteroid-generation]")
{
	const auto rows = GetMultiLayerAsteroidPopulationDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(rows.size() == 13);
	CHECK(rows.front().area_log2 == 12); CHECK(rows.back().area_log2 == 24);
	CHECK(rows.front().counts == std::array<uint32_t, 3>{6, 10, 14});
	CHECK(rows.back().counts == std::array<uint32_t, 3>{6144, 10240, 14336});
	const auto shares = GetMultiLayerAsteroidClassShares(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(shares.size() == 3);
	CHECK(shares[0].class_index == 0); CHECK(shares[0].percent == 60);
	CHECK(shares[1].class_index == 1); CHECK(shares[1].percent == 30);
	CHECK(shares[2].class_index == 2); CHECK(shares[2].percent == 10);
	const auto *placement = GetMultiLayerAsteroidPlacementDefinition(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(placement); CHECK(placement->generation_version == 1);
	CHECK(placement->grid_spacing_tiles == 4); CHECK(placement->starter_radius_tiles == 32);
	// Classic and unknown identities cannot acquire Industry generation defaults through a fallback lookup.
	for (auto profile : {MultiLayerGameplayProfileID::Classic, MultiLayerGameplayProfileID::Invalid}) {
		CHECK(GetMultiLayerAsteroidPopulationDefinitions(profile).empty());
		CHECK(GetMultiLayerAsteroidClassShares(profile).empty());
		CHECK(GetMultiLayerAsteroidPlacementDefinition(profile) == nullptr);
	}
}

/** Verify geometric no-site/far-site/preferred-site policy on isolated actual Surface terrain without company creation. */
TEST_CASE("World asteroid starter search uses real launch geometry and preserves feature RNG ownership", "[asteroid-generation]")
{
	AutoRestoreBackup edges(_settings_game.construction.freeform_edges, false);
	Map::Allocate(64, 256, MAX_MAP_LAYERS);
	// This isolated terrain fixture has no initial water, exposing the explicit no-site policy before any eligible shore is added.
	for (const auto tile : Map::Iterate()) {
		if (IsInnerTile(tile)) MakeClear(tile, ClearGround::Grass, 0);
		else MakeVoid(tile);
	}
	const auto random = _random, interactive = _interactive_random;
	AsteroidStarterSearchWork work;
	const auto absent = FindWorldAsteroidStarterInput(64, 256, &work);
	REQUIRE(absent); CHECK(absent->kind == AsteroidStarterKind::NoSite);
	CHECK(absent->site_x == UINT32_MAX); CHECK(absent->site_y == UINT32_MAX);
	CHECK(work.anchors == 64 * 256); CHECK(work.geometry_queries == 2 * 64 * 256);
	CHECK_FALSE(FindWorldAsteroidStarterInput(128, 256));
	MakeRiver(TileXYL(10, 9, MapLayer::Surface), 0);
	const auto distant = FindWorldAsteroidStarterInput(64, 256, &work);
	REQUIRE(distant); REQUIRE(distant->kind == AsteroidStarterKind::Site);
	CHECK(distant->initial_approach_distance_squared > uint64_t{32 * 65536} * (32 * 65536));
	CHECK(GetLaunchCenterSiteGeometryBlock(TileXYL(distant->site_x, distant->site_y, MapLayer::Surface), distant->rotation).Succeeded());
	MakeRiver(TileXYL(32, 99, MapLayer::Surface), 0);
	const auto preferred = FindWorldAsteroidStarterInput(64, 256, &work);
	REQUIRE(preferred); REQUIRE(preferred->kind == AsteroidStarterKind::Site);
	CHECK(preferred->initial_approach_distance_squared <= uint64_t{32 * 65536} * (32 * 65536));
	CHECK(preferred == FindWorldAsteroidStarterInput(64, 256));
	CHECK(work.anchors <= 64 * 256); CHECK(work.geometry_queries <= 2 * work.anchors);
	CHECK(work.grid_candidates <= 25 * work.geometry_queries);
	// Every exact population column keeps finite reserves, three distinct classes and seeded continuation with the same chosen input.
	for (AsteroidDensity density : {AsteroidDensity::Sparse, AsteroidDensity::Normal, AsteroidDensity::Dense}) {
		const auto field = GenerateAsteroidField(64, 256, 12345, density, preferred);
		REQUIRE(field); REQUIRE(ValidateAsteroidField(*field));
		CHECK(field == GenerateAsteroidField(64, 256, 12345, density, preferred));
		CHECK(field != GenerateAsteroidField(64, 256, 54321, density, preferred));
		const auto expected = CompleteGridReference(64, 256, preferred->preference);
		// Startup positions and class identities must agree with the independent grid selection, rather than just a mirrored validator.
		for (uint32_t starter = 0; starter < 3; ++starter) {
			CHECK(field->bodies[starter].motion.position == expected.positions[starter]);
			CHECK(to_underlying(field->bodies[starter].resource_class) == starter);
		}
		CHECK(field->starter_input.initial_approach_distance_squared == expected.approach_distance_squared);
		auto moved = *field; AdvanceContainedAsteroidSnapshot(moved);
		CHECK(moved.starter_input == field->starter_input); CHECK(ValidateAsteroidField(moved));
	}
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]); CHECK(_interactive_random.state[1] == interactive.state[1]);
}

/** Reject malformed historical input and publish conservative Q16 distances without inventing legacy history. */
TEST_CASE("Asteroid initial access records validate frozen bounds and explicit legacy absence", "[asteroid-generation]")
{
	const auto generated = GenerateAsteroidField(64, 64, 12345, AsteroidDensity::Normal);
	REQUIRE(generated);
	const auto distance = GetAsteroidInitialApproachDistance(generated->starter_input);
	REQUIRE(distance);
	CHECK(uint64_t{*distance} * *distance >= generated->starter_input.initial_approach_distance_squared);
	CHECK(uint64_t{*distance - 1} * (*distance - 1) < generated->starter_input.initial_approach_distance_squared);
	auto legacy = *generated; legacy.starter_input = {};
	CHECK(ValidateAsteroidField(legacy)); CHECK_FALSE(GetAsteroidInitialApproachDistance(legacy.starter_input));
	auto corrupt = *generated; ++corrupt.starter_input.initial_approach_distance_squared;
	CHECK_FALSE(ValidateAsteroidField(corrupt));
	corrupt = *generated; corrupt.starter_input.site_x = 1;
	CHECK_FALSE(ValidateAsteroidField(corrupt));
	corrupt = *generated; corrupt.starter_input.kind = static_cast<AsteroidStarterKind>(4);
	CHECK_FALSE(ValidateAsteroidField(corrupt));
	corrupt = *generated; corrupt.starter_input.preference.x = -1;
	CHECK_FALSE(ValidateAsteroidField(corrupt));
}
