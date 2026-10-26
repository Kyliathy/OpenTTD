/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_spatial_membership.cpp Bounded derived membership under high-ID insertion, movement and retirement.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group13.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_work_cost.h"
#include "../safeguards.h"

/** Physical-plane validation occurs before bucket indexing; rejected edits retain previous query answers. */
TEST_CASE("Space spatial membership rejects invalid points and supports high live IDs", "[space-spatial-membership]")
{
	SpaceSpatialIndex index;
	index.Reset(64, 64, UINT32_MAX); // Capacity hints cannot allocate by lifetime issuance.
	const OrbitalBodyID high{UINT32_MAX - 1};
	REQUIRE(index.Insert(high, {0, 0}));
	CHECK(index.Size() == 1);
	CHECK_FALSE(index.Insert(high, {32 * SPACE_Q16, 32 * SPACE_Q16}));
	CHECK_FALSE(index.Insert(INVALID_ORBITAL_BODY, {0, 0}));
	CHECK_FALSE(index.Insert(OrbitalBodyID{70'000}, {-1, 0}));
	CHECK_FALSE(index.Update(high, {63 * SPACE_Q16 + 1, 0}));
	CHECK_FALSE(index.Update(high, {0, INT32_MAX}));
	CHECK(index.Query({0, 0}, {15 * SPACE_Q16, 15 * SPACE_Q16}) == std::vector{high});
	REQUIRE(index.Update(high, {63 * SPACE_Q16, 63 * SPACE_Q16}));
	CHECK(index.Query({0, 0}, {15 * SPACE_Q16, 15 * SPACE_Q16}).empty());
	CHECK(index.Query({48 * SPACE_Q16, 48 * SPACE_Q16}, {INT32_MAX, INT32_MAX}) == std::vector{high});
	CHECK(index.Query({64 * SPACE_Q16, 0}, {INT32_MAX, INT32_MAX}).empty());
	CHECK(index.Query({0, 64 * SPACE_Q16}, {INT32_MAX, INT32_MAX}).empty());
	CHECK(index.Query({-100, -100}, {-1, -1}).empty());
	CHECK(index.Query({1, 1}, {0, 0}).empty());
	REQUIRE(index.Remove(high));
	CHECK_FALSE(index.Remove(high));
	CHECK(index.Size() == 0);
	CHECK(index.Query({INT32_MIN, INT32_MIN}, {INT32_MAX, INT32_MAX}).empty());
	index.Reset(32, 64, 100);
	CHECK_FALSE(index.Insert(OrbitalBodyID{0}, {0, 0}));
	index.Reset(0, 0, 0);
	CHECK(index.Size() == 0);
}

/** Clustered swap/pop unlink never loses the moved member, and every query explicitly sorts original IDs. */
TEST_CASE("Space spatial membership unlinks a full cluster and moves surviving IDs", "[space-spatial-membership]")
{
	SpaceSpatialIndex index;
	index.Reset(64, 64, MAX_ORBITAL_BODIES);
	// Reverse publication deliberately disagrees with ID order; one bucket may
	// contain the entire population without requiring a linear unlink search.
	for (uint32_t n = MAX_ORBITAL_BODIES; n != 0; --n) REQUIRE(index.Insert(OrbitalBodyID{65'536 + n}, {10 * SPACE_Q16, 10 * SPACE_Q16}));
	CHECK_FALSE(index.Insert(OrbitalBodyID{1}, {0, 0}));
	const auto all = index.Query({0, 0}, {15 * SPACE_Q16, 15 * SPACE_Q16});
	REQUIRE(all.size() == MAX_ORBITAL_BODIES);
	CHECK(std::ranges::is_sorted(all));
	CHECK(all.front() == OrbitalBodyID{65'537});
	// Alternate removals and cross-bucket moves exercise moved-slot correction
	// repeatedly; the original query remains a harmless copy of candidate IDs.
	SpaceWorkSample work;
	{
		ScopedSpaceWork observation(work);
		for (uint32_t slot = 0; slot < all.size(); ++slot) {
			if (slot % 2 == 0) REQUIRE(index.Remove(all[slot]));
			else REQUIRE(index.Update(all[slot], {30 * SPACE_Q16, 30 * SPACE_Q16}));
		}
	}
	CHECK(work.Get(SpaceWorkKind::IndexUnlinks) == MAX_ORBITAL_BODIES);
	CHECK(work.Get(SpaceWorkKind::IndexLinks) == MAX_ORBITAL_BODIES / 2);
	CHECK(index.Query({0, 0}, {15 * SPACE_Q16, 15 * SPACE_Q16}).empty());
	const auto survivors = index.Query({16 * SPACE_Q16, 16 * SPACE_Q16}, {31 * SPACE_Q16, 31 * SPACE_Q16});
	REQUIRE(survivors.size() == MAX_ORBITAL_BODIES / 2);
	CHECK(std::ranges::is_sorted(survivors));
	for (const auto id : survivors) REQUIRE(index.Remove(id));
	CHECK(index.Size() == 0);
	CHECK(index.Query({0, 0}, {INT32_MAX, INT32_MAX}).empty());
}

/** Fresh IDs repeatedly occupy four live slots; neither index nor field needs retired-body tombstones. */
TEST_CASE("Space spatial membership survives long high-ID churn with bounded live owners", "[space-spatial-membership]")
{
	AsteroidField field;
	field.next_body_id = 2'000'000'000;
	field.renewal = MakeAsteroidRenewalState(17);
	REQUIRE(RebuildAsteroidBodyLookup(field));
	const auto ledger = field.renewal;
	SpaceSpatialIndex index;
	index.Reset(64, 64, 4);
	// Retain four copied records between insert/erase attempts while advancing
	// issuance ten thousand times. Stale IDs must fail after both owners unlink.
	for (uint32_t n = 0; n < 10'000; ++n) {
		AsteroidBody body;
		body.id = OrbitalBodyID{field.next_body_id};
		body.motion.position = {static_cast<int32_t>(n % 4) * 16 * SPACE_Q16, 0};
		REQUIRE(InsertAsteroidBody(field, body));
		REQUIRE(index.Insert(body.id, body.motion.position));
		if (field.bodies.size() > 4) {
			const auto old = GetAsteroidBodiesInIDOrder(field).front()->id;
			REQUIRE(index.Remove(old));
			REQUIRE(EraseAsteroidBody(field, old));
			CHECK(FindAsteroidBody(field, old) == nullptr);
		}
		CHECK(index.Size() == field.bodies.size());
	}
	CHECK(index.Size() == 4);
	CHECK(field.next_body_id == 2'000'010'000);
	CHECK(field.renewal == ledger);
	const auto remaining = index.Query({0, 0}, {INT32_MAX, INT32_MAX});
	for (const auto id : remaining) REQUIRE(FindAsteroidBody(field, id) != nullptr);
}

/** Restore actual native field/index after installation checks. */
struct SpatialFieldRestore {
	AsteroidField original = GetAsteroidField();
	/** Reinstall original authority and rebuild its disposable spatial membership. */
	~SpatialFieldRestore() { CHECK(InstallAsteroidField(std::move(this->original))); }
};

/** Installation derives a complete new index before publication; rejected fields keep original queries and resources. */
TEST_CASE("Space native field installation rebuilds membership and reset discards it", "[space-spatial-membership]")
{
	SpatialFieldRestore restore;
	const auto field = GenerateAsteroidField(64, 64, 8, AsteroidDensity::Normal);
	REQUIRE(field);
	REQUIRE(InstallAsteroidField(*field));
	const auto before = GetAsteroidField();
	const auto ids = QueryAsteroidBodies({0, 0}, {INT32_MAX, INT32_MAX});
	CHECK(ids.size() == field->bodies.size());
	CHECK(std::ranges::is_sorted(ids));
	for (const auto id : ids) REQUIRE(GetMutableAsteroidBody(id) != nullptr);
	auto invalid = *field;
	invalid.bodies.front().motion.position.x = -1;
	CHECK_FALSE(InstallAsteroidField(invalid));
	CHECK(QueryAsteroidBodies({0, 0}, {INT32_MAX, INT32_MAX}) == ids);
	CHECK(GetAsteroidField() == before);
	ResetAsteroidField();
	CHECK(QueryAsteroidBodies({0, 0}, {INT32_MAX, INT32_MAX}).empty());
	for (const auto id : ids) CHECK(GetMutableAsteroidBody(id) == nullptr);
}
