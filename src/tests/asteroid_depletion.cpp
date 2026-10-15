/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_depletion.cpp Finite-zero persistence and monotonic body reference boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group5
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../safeguards.h"

/** Keep exhausted natural identities, generated composition and movement without respawning their reserve. */
TEST_CASE("Depleted asteroid records retain identity resources and motion", "[asteroid-depletion]")
{
	const auto generated = GenerateAsteroidField(64, 64, 12345, AsteroidDensity::Normal); REQUIRE(generated);
	auto field = *generated;
	field.bodies[0].remaining_reserve = 0;
	REQUIRE(ValidateAsteroidField(field));
	const auto id = field.bodies[0].id;
	const auto initial = field.bodies[0].initial_reserve;
	const auto position = field.bodies[0].motion.position;
	// Exhaustion changes extraction eligibility only: repeated motion must never refill, replace or drop the natural target.
	for (uint32_t tick = 0; tick < 148; ++tick) {
		AdvanceAsteroidField(field);
		CHECK(field.bodies.size() == generated->bodies.size()); CHECK(field.bodies[0].id == id);
		CHECK(field.bodies[0].remaining_reserve == 0); CHECK(field.bodies[0].initial_reserve == initial);
		CHECK(field.bodies[0].resource_class == generated->bodies[0].resource_class);
		CHECK(field.bodies[0].size == generated->bodies[0].size); CHECK(ValidateAsteroidField(field));
	}
	CHECK(field.bodies[0].motion.position != position);
	// These are structural reference records only; a native lease additionally requires a real matching hull during full acceptance.
	field.bodies[0].mining_slot = VehicleID{17}; CHECK(ValidateAsteroidField(field));
	field.bodies[0].mining_slot = VehicleID::End(); CHECK_FALSE(ValidateAsteroidField(field));
	field.bodies[0].mining_slot = VehicleID::Invalid(); REQUIRE(ValidateAsteroidField(field));
}

/** Reject erased/renumbered bodies and keep invalid references distinct from the surviving exhausted body. */
TEST_CASE("Asteroid removal is unsupported and invalid identities never alias depleted targets", "[asteroid-depletion]")
{
	const auto generated = GenerateAsteroidField(64, 64, 12345, AsteroidDensity::Normal); REQUIRE(generated);
	auto removed = *generated; removed.bodies.erase(removed.bodies.begin()); CHECK_FALSE(ValidateAsteroidField(removed));
	auto reused = *generated; reused.bodies[0].id = OrbitalBodyID{1}; CHECK_FALSE(ValidateAsteroidField(reused));
	const auto previous = GetAsteroidField();
	auto depleted = *generated; depleted.bodies[0].remaining_reserve = 0; REQUIRE(InstallAsteroidField(depleted));
	CHECK(GetMutableAsteroidBody(OrbitalBodyID{0}) != nullptr);
	CHECK(GetMutableAsteroidBody(OrbitalBodyID{0})->remaining_reserve == 0);
	CHECK(GetMutableAsteroidBody(OrbitalBodyID{10}) == nullptr);
	CHECK(GetMutableAsteroidBody(INVALID_ORBITAL_BODY) == nullptr);
	REQUIRE(InstallAsteroidField(previous));
}
