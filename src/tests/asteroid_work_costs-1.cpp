/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_work_costs-1.cpp Actual canonical field generation, motion and derived bucket work at exact requested populations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group21
 * Measures native detached field/index components; Surface generation and whole-game frame rates are separate producers.
 */
#include "../stdafx.h"
#include "asteroid_snapshot_fixture.h"
#include "asteroid_work_fixture.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/** Exact native generation, local queries and actual clustered bucket departure must remain bounded without body-pair motion scans. */
TEST_CASE("Asteroid field work follows exact populations and output sensitive native buckets", "[.asteroid-field-work]")
{
	const auto &scenario = SelectedAsteroidWorkCase(); const auto random = _random, interactive = _interactive_random;
	std::optional<AsteroidField> generated; AsteroidWorkMeasurement generation;
	/** Generate only through the canonical native field/content owner. */
	MeasureAsteroidWork(generation, [&] { generated = GenerateAsteroidField(scenario.width, scenario.height, ASTEROID_WORK_SEED, scenario.density); });
	REQUIRE(generated); auto &field = *generated; REQUIRE(field.bodies.size() == scenario.bodies); REQUIRE(ValidateAsteroidField(field));
	const auto &placement = *GetMultiLayerAsteroidPlacementDefinition(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto columns = (field.band.max.x - field.band.min.x) / (placement.grid_spacing_tiles * SPACE_Q16) + 1;
	const auto rows = (field.band.max.y - field.band.min.y) / (placement.grid_spacing_tiles * SPACE_Q16) + 1;
	const uint64_t cells = static_cast<uint64_t>(columns) * rows;
	CHECK(generation.work.Get(SpaceWorkKind::GenerationCells) == cells);
	CHECK(generation.work.Get(SpaceWorkKind::GenerationStarterCandidates) == 3 * cells - 3);
	CHECK(generation.work.Get(SpaceWorkKind::GenerationShuffleSwaps) == 2 * (scenario.bodies - 3));
	CHECK(generation.work.Get(SpaceWorkKind::GenerationBodies) == scenario.bodies);
	CHECK(generation.work.Get(SpaceWorkKind::ValidationBodies) == scenario.bodies);
	CHECK(generation.work.Get(SpaceWorkKind::ValidationVelocityTests) == uint64_t{24} * scenario.bodies);
	const auto resources = AsteroidWorkResources(field); PrepareAsteroidWorkGeometry(field, scenario);
	SpaceSpatialIndex index; AsteroidWorkMeasurement initialization;
	/** The original derived index consumes actual native IDs/positions and confers no independent resource or target permission. */
	MeasureAsteroidWork(initialization, [&] { index.Reset(field.width, field.height, scenario.bodies); for (const auto &body : field.bodies) index.Update(body.id, body.motion.position); });
	CHECK(initialization.work.Get(SpaceWorkKind::IndexUpdates) == scenario.bodies); CHECK(initialization.work.Get(SpaceWorkKind::IndexLinks) == scenario.bodies);
	constexpr uint32_t steps = 32; AsteroidWorkMeasurement motion;
	/** Each actual field step publishes one motion and one disposable bucket update per original body, including a full clustered bucket departure. */
	MeasureAsteroidWork(motion, [&] { for (uint32_t tick = 0; tick < steps; ++tick) { AdvanceContainedAsteroidSnapshot(field); for (const auto &body : field.bodies) index.Update(body.id, body.motion.position); } });
	CHECK(motion.work.Get(SpaceWorkKind::BodySteps) == uint64_t{steps} * scenario.bodies);
	CHECK(motion.work.Get(SpaceWorkKind::IndexUpdates) == uint64_t{steps} * scenario.bodies);
	CHECK(motion.work.Get(SpaceWorkKind::BodyLookups) == 0); CHECK(motion.work.Get(SpaceWorkKind::QueryCalls) == 0);
	CHECK(motion.work.Get(SpaceWorkKind::IndexUnlinks) <= uint64_t{steps} * scenario.bodies);
	if (scenario.clustered) { CHECK(motion.work.Get(SpaceWorkKind::IndexUnlinks) == scenario.bodies); CHECK(motion.work.Get(SpaceWorkKind::IndexLinks) == scenario.bodies); }
	REQUIRE(ValidateAsteroidField(field)); CHECK(AsteroidWorkResources(field) == resources);
	const auto point = field.bodies.front().motion.position; AsteroidWorkMeasurement query; std::vector<OrbitalBodyID> found;
	/** Query actual local native buckets; dense output is allowed to cost its real candidate count rather than being mislabeled constant work. */
	MeasureAsteroidWork(query, [&] { found = index.Query({point.x - 2 * SPACE_Q16, point.y - 2 * SPACE_Q16}, {point.x + 2 * SPACE_Q16, point.y + 2 * SPACE_Q16}); });
	CHECK(query.work.Get(SpaceWorkKind::QueryCalls) == 1); CHECK(query.work.Get(SpaceWorkKind::QueryBuckets) <= 4);
	CHECK(query.work.Get(SpaceWorkKind::QueryCandidates) == found.size()); REQUIRE(!found.empty()); CHECK(std::ranges::is_sorted(found));
	CHECK(std::ranges::find(found, OrbitalBodyID{0}) != found.end());
	const uint64_t sort_bound = uint64_t{64} * found.size() * std::bit_width(static_cast<uint32_t>(found.size()));
	CHECK(query.work.Get(SpaceWorkKind::QueryOrderTests) <= sort_bound);
	if (scenario.clustered) CHECK(found.size() == scenario.bodies);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]); CHECK(_interactive_random.state[0] == interactive.state[0]); CHECK(_interactive_random.state[1] == interactive.state[1]);
	WriteAsteroidWork(scenario, "field", {{"generation", AsteroidWorkRecord(generation)}, {"initialization", AsteroidWorkRecord(initialization)}, {"motion", AsteroidWorkRecord(motion)},
		{"query", AsteroidWorkRecord(query)}, {"motion_steps", steps}, {"grid_cells", cells}, {"query_results", found.size()}, {"sort_work_bound", sort_bound},
		{"resources", resources}, {"final_digest", ComputeAsteroidFieldDigest(field)}, {"surface_site_search_measured", false}});
}
