/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_work_fixture.h Exact population cases and native component measurement output.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group21
 * Declared isolated fields and legacy hulls provide performance inputs, never paid mission procurement or a whole-game frame-rate claim.
 */
#ifndef TESTS_ASTEROID_WORK_FIXTURE_H
#define TESTS_ASTEROID_WORK_FIXTURE_H
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "asteroid_snapshot_fixture.h"
#include "../space_work_cost.h"
#include "../multilayer_asteroid_catalogue.h"
#include "test_artifact_paths.h"
#include "../core/format.hpp"
#include "../3rdparty/nlohmann/json.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdlib>
#include <fstream>

/** One explicit native population-table example; clustering changes only declared initial geometry after an actual first motion step. */
struct AsteroidWorkCase {
	std::string_view name; ///< Invocation's exact case identity.
	uint32_t width, height; ///< Canonical plane geometry, independent of the selected camera.
	AsteroidDensity density; ///< Actual native population column.
	uint32_t bodies; ///< Exact required table count, not a substitute body list.
	bool clustered = false; ///< Whether actual generated records receive explicit in-band clustered performance geometry.
};

inline constexpr std::array ASTEROID_WORK_CASES{
	AsteroidWorkCase{"small", 64, 64, AsteroidDensity::Normal, 10},
	AsteroidWorkCase{"normal", 256, 256, AsteroidDensity::Normal, 160},
	AsteroidWorkCase{"largest-normal", 4096, 4096, AsteroidDensity::Normal, 10240},
	AsteroidWorkCase{"largest-dense", 4096, 4096, AsteroidDensity::Dense, 14336},
	AsteroidWorkCase{"clustered", 4096, 4096, AsteroidDensity::Dense, 14336, true},
	AsteroidWorkCase{"narrow-tall", 64, 1024, AsteroidDensity::Normal, 160},
	AsteroidWorkCase{"narrow-wide", 1024, 64, AsteroidDensity::Normal, 160},
};
inline constexpr uint64_t ASTEROID_WORK_SEED = 12345; ///< Same explicit feature seed across independent component worlds/configurations.

/** @return Exactly one declared native case; reject malformed selection before generating a world. */
inline const AsteroidWorkCase &SelectedAsteroidWorkCase()
{
	const char *value = std::getenv("OTTD_ASTEROID_WORK_CASE"); REQUIRE(value);
	const auto found = std::ranges::find(ASTEROID_WORK_CASES, std::string_view{value}, &AsteroidWorkCase::name);
	REQUIRE(found != ASTEROID_WORK_CASES.end()); return *found;
}

/** Diagnostic counters and instrumented component time; neither controls admission or simulation progress. */
struct AsteroidWorkMeasurement {
	SpaceWorkSample work; ///< Actual operations emitted by original native producers.
	uint64_t nanoseconds = 0; ///< Diagnostic elapsed component time, including fixed observation overhead.
};

/** Measure actual native component calls without putting assertions or output inside their timed interval.
 * @tparam Action Original native operation sequence.
 * @param total Caller-owned cumulative operations/time, initially zero for a new phase.
 * @param action Original functions to execute once; this helper grants no work independently of those functions.
 */
template <typename Action>
static void MeasureAsteroidWork(AsteroidWorkMeasurement &total, Action action)
{
	const auto start = std::chrono::steady_clock::now();
	{ ScopedSpaceWork observe(total.work); action(); }
	total.nanoseconds += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count());
}

/** @param field Actual generated records. @param scenario Explicit performance input policy.
 * Cluster geometry preserves finite content and historical starter input; an actual first step precedes the declaration instead of installing an elapsed interval.
 */
static void PrepareAsteroidWorkGeometry(AsteroidField &field, const AsteroidWorkCase &scenario)
{
	if (!scenario.clustered) return;
	AdvanceContainedAsteroidSnapshot(field);
	// All actual IDs share one in-band bucket just before its boundary. The next real maximum-drift step must unlink each by the native constant-work swap/pop path.
	for (auto &body : field.bodies) {
		body.motion.position = {80 * SPACE_Q16 - 1, static_cast<int32_t>(scenario.height / 2) * SPACE_Q16};
		body.motion.velocity_x = 16384; body.motion.velocity_y = 0;
	}
	REQUIRE(ValidateAsteroidField(field));
}

/** @param field Actual native finite records. @return Per-class initial/remaining quantities without creating an inventory authority. */
static nlohmann::json AsteroidWorkResources(const AsteroidField &field)
{
	std::array<uint64_t, 3> initial{}, remaining{};
	// Report every actual resource separately; an untouched class cannot finance another body's measured extraction.
	for (const auto &body : field.bodies) { initial[to_underlying(body.resource_class)] += body.initial_reserve; remaining[to_underlying(body.resource_class)] += body.remaining_reserve; }
	return nlohmann::json{{"initial", initial}, {"remaining", remaining}};
}

/** @param measurement Actual native component totals. @return Named exact operations and diagnostic time, with stable labels independent of compiler timing. */
static nlohmann::json AsteroidWorkRecord(const AsteroidWorkMeasurement &measurement)
{
	static constexpr std::array<std::string_view, static_cast<size_t>(SpaceWorkKind::End)> names{
		"generation_cells", "generation_starter_candidates", "generation_shuffle_swaps", "generation_bodies", "validation_bodies", "validation_velocity_tests",
		"body_steps", "index_updates", "index_unlinks", "index_links", "query_calls", "query_buckets", "query_candidates", "query_order_tests", "body_lookups", "craft_ticks",
		"attachment_visits", "approach_craft_visits", "approach_occupant_checks", "approach_target_checks", "planner_requests", "planner_slots", "planner_slot_tests",
		"planner_owner_tests", "planner_order_tests", "staging_body_tests", "extraction_attempts", "extraction_units",
		"exit_forecasts", "intercept_forecasts", "mining_selections", "renewal_attempts", "renewal_entry_candidates", "retirement_participants",
	};
	nlohmann::json work = nlohmann::json::object();
	// Every original producer counter has one stable report label, including zeros that expose an unexpected whole-field query during a native craft tick.
	for (size_t index = 0; index < names.size(); ++index) work[std::string{names[index]}] = measurement.work.counts[index];
	return nlohmann::json{{"nanoseconds", measurement.nanoseconds}, {"work", std::move(work)}};
}

/** Freeze one native component result in this process's explicit test invocation.
 * @param scenario Exact native geometry/population input.
 * @param domain Field/index or actual native probe component scope.
 * @param detail Measured original operations, resource facts and diagnostic timings.
 */
static void WriteAsteroidWork(const AsteroidWorkCase &scenario, std::string_view domain, nlohmann::json detail)
{
	const auto paths = GetTestCaseArtifactPaths("space", "asteroid-work");
	detail["_documentation"] = "Owner: r2mlvs10-asteroids-and-mining-probes.md Group21; measured original native component work, declared isolated inputs, no paid procurement or whole-game FPS claim";
	detail["case"] = scenario.name; detail["domain"] = domain; detail["width"] = scenario.width; detail["height"] = scenario.height;
	detail["density"] = to_underlying(scenario.density); detail["bodies"] = scenario.bodies; detail["clustered"] = scenario.clustered; detail["seed"] = ASTEROID_WORK_SEED;
	std::ofstream output(paths.reports / fmt::format("asteroid-work-{}.json", domain)); REQUIRE(output.good()); output << detail.dump(2) << '\n'; output.flush(); REQUIRE(output.good());
	fmt::print("ASTEROID_WORK case={} domain={} bodies={} native=true paid=false\n", scenario.name, domain, scenario.bodies);
}

#endif /* TESTS_ASTEROID_WORK_FIXTURE_H */
