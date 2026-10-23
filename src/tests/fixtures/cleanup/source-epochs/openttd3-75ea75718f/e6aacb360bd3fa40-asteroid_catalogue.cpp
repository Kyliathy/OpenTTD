/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid_catalogue.cpp Independent finite-resource, size-sampling and mining-budget acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group1
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../core/random_func.hpp"
#include "../multilayer_space_content.h"
#include "../safeguards.h"

/** Verify selected-profile ownership and every percentile boundary against independent design literals. */
TEST_CASE("Industry publishes canonical asteroid class and size definitions", "[asteroid-catalogue]")
{
	const auto *industry = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto *classic = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::Classic);
	REQUIRE(industry); REQUIRE(classic);
	REQUIRE(industry->asteroid_resources.size() == 3);
	REQUIRE(industry->asteroid_sizes.size() == 3);
	CHECK(classic->asteroid_resources.empty()); CHECK(classic->asteroid_sizes.empty());
	CHECK(industry->asteroid_resources.data() == GetMultiLayerAsteroidDefinitions().data());
	CHECK(industry->asteroid_sizes.data() == GetMultiLayerAsteroidSizeDefinitions().data());
	constexpr CargoLabel cargo[] = {CargoLabel{"NORE"}, CargoLabel{"WICE"}, CargoLabel{"RARE"}};
	constexpr uint32_t minimum[] = {400, 800, 100}, maximum[] = {800, 1600, 200};
	constexpr uint8_t chance[] = {50, 35, 15}, multiplier[] = {1, 2, 4};
	constexpr uint32_t diameter[] = {32768, 65536, 131072};
	// Each saved class/size index must keep its canonical meaning rather than inheriting sprite metadata.
	for (size_t index = 0; index < 3; ++index) {
		const auto &resource = industry->asteroid_resources[index];
		const auto &size = industry->asteroid_sizes[index];
		CHECK(resource.cargo == cargo[index]);
		CHECK(resource.small_min == minimum[index]); CHECK(resource.small_max == maximum[index]);
		CHECK(size.chance_percent == chance[index]); CHECK(size.reserve_multiplier == multiplier[index]);
		CHECK(size.diameter_q16 == diameter[index]); CHECK(size.mining_slots == 1);
	}
	std::array<uint32_t, 3> selected{};
	// Exhaust the whole percentile domain to catch both boundary shifts and incorrect total probability.
	for (uint32_t percentile = 0; percentile < 100; ++percentile) {
		const uint8_t expected = percentile < 50 ? 0 : percentile < 85 ? 1 : 2;
		const auto actual = SelectMultiLayerAsteroidSize(percentile);
		REQUIRE(actual == expected); ++selected[actual];
	}
	CHECK(selected == std::array<uint32_t, 3>{50, 35, 15});
}

/** Exercise real feature-local RNG at both inclusive endpoints and preserve its one-draw contract. */
TEST_CASE("Asteroid reserve sampling includes both endpoints before size scaling", "[asteroid-catalogue]")
{
	const auto simulation = _random, interactive = _interactive_random;
	constexpr uint32_t minimum[] = {400, 800, 100}, maximum[] = {800, 1600, 200};
	constexpr uint8_t multiplier[] = {1, 2, 4};
	// All nine class/size combinations must include both finite reserve endpoints without extra RNG draws.
	for (size_t resource = 0; resource < 3; ++resource) for (size_t size = 0; size < 3; ++size) {
		for (uint32_t seed : {8U, 0U}) {
			Randomizer random; random.SetSeed(seed); auto expected = random;
			// Native Randomizer's first result is exactly zero for seed8 and UINT32_MAX for seed0.
			CHECK(expected.Next() == (seed == 8 ? 0U : UINT32_MAX));
			const auto sampled = SampleMultiLayerAsteroidReserve(GetMultiLayerAsteroidDefinitions()[resource],
				GetMultiLayerAsteroidSizeDefinitions()[size], random);
			CHECK(sampled == (seed == 8 ? minimum[resource] : maximum[resource]) * multiplier[size]);
			CHECK(random.state[0] == expected.state[0]); CHECK(random.state[1] == expected.state[1]);
		}
	}
	CHECK(_random.state[0] == simulation.state[0]); CHECK(_random.state[1] == simulation.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]); CHECK(_interactive_random.state[1] == interactive.state[1]);
}

/** Verify finite full-load budgets and extraction duration using the runtime's shared mission budget. */
TEST_CASE("Asteroid reserves support canonical full loads and bounded mining duration", "[asteroid-catalogue]")
{
	const auto &budget = GetMultiLayerSpaceBudget();
	REQUIRE(budget.probe_hold_sixteenths == 320);
	REQUIRE(budget.mining_units == 2); REQUIRE(budget.mining_interval == 74);
	CHECK(budget.returns_per_service == 2);
	const uint32_t hold = budget.probe_hold_sixteenths / 16;
	CHECK((hold + budget.mining_units - 1) / budget.mining_units * budget.mining_interval == 740);
	uint32_t weighted_multiplier = 0;
	// Size selection is independent of composition, so every class uses the same weighted multiplier.
	for (const auto &size : GetMultiLayerAsteroidSizeDefinitions()) weighted_multiplier += size.chance_percent * size.reserve_multiplier;
	CHECK(weighted_multiplier == 180);
	constexpr uint32_t least_loads[] = {20, 40, 5}, most_loads[] = {160, 320, 40}, mean[] = {1080, 2160, 270};
	// Resource classes retain different reserves while sharing the same bounded hold and extraction rate.
	for (size_t resource = 0; resource < 3; ++resource) {
		const auto &definition = GetMultiLayerAsteroidDefinitions()[resource];
		CHECK(definition.small_min / hold == least_loads[resource]);
		CHECK(definition.small_max * GetMultiLayerAsteroidSizeDefinitions().back().reserve_multiplier / hold == most_loads[resource]);
		CHECK((definition.small_min + definition.small_max) * weighted_multiplier / 200 == mean[resource]);
		CHECK(definition.small_min >= hold * budget.returns_per_service);
	}
	const auto field = GenerateAsteroidField(256, 256, 0x20261012, AsteroidDensity::Normal);
	REQUIRE(field); REQUIRE(field->bodies.size() == 160); REQUIRE(ValidateAsteroidField(*field));
	std::array<uint32_t, 3> reserves{};
	// Freeze the pre-catalogue producer's per-cargo ledger; moving constants must preserve feature-seed output.
	for (const auto &body : field->bodies) reserves[to_underlying(body.resource_class)] += body.initial_reserve;
	CHECK(reserves == std::array<uint32_t, 3>{95050, 99715, 4018});
}
