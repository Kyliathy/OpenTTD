/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file transport_tube_registry.cpp Canonical tube allocation, collision/capacity refusal and capability reset. @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Group1 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../transport_tube.h"

#include "../safeguards.h"

/** Actual native content/world fixture; derived rail definitions reset with this test's world rather than surviving into other fixtures. */
class TubeRailRegistryFixture {
public:
	TBMEngineFixture world; ///< Exact package, actual company/map and existing native engine identities.
	/** Prepare the shared fixture's actual canonical rail registry. */
	TubeRailRegistryFixture() = default;
	/** Retire only this isolated world's derived registry rows before the next fixture starts. */
	~TubeRailRegistryFixture() { ResetRailTypes(); }
};

/** A foreign occupied slot changes the runtime tube index while native identity remains its own exact admitted canonical row. */
TEST_CASE("Native tube rail allocation preserves existing slots and resets semantic authority", "[transport-tube-registry]")
{
	TubeRailRegistryFixture fixture;
	const RailType foreign = AllocateRailType(RailTypeLabel{"OTHR"});
	REQUIRE(foreign != INVALID_RAILTYPE);
	const uint original_cost = GetRailTypeInfo(RAILTYPE_RAIL)->cost_multiplier;
	const RailType tube = InitialiseTubeRailType(fixture.world.package);
	REQUIRE(tube != INVALID_RAILTYPE);
	CHECK(tube != foreign);
	CHECK(GetTubeRailType() == tube);
	CHECK(IsTubeRailType(tube));
	CHECK_FALSE(IsTubeRailType(foreign));
	CHECK_FALSE(IsTubeRailType(INVALID_RAILTYPE));
	CHECK(GetRailTypeInfo(foreign)->label == RailTypeLabel{"OTHR"});
	CHECK(GetRailTypeInfo(RAILTYPE_RAIL)->cost_multiplier == original_cost);
	CHECK(GetRailTypeInfo(tube)->powered_railtypes == RailTypes{tube});
	CHECK(GetRailTypeInfo(tube)->compatible_railtypes == RailTypes{tube});
	CHECK(GetRailTypeInfo(tube)->flags.Test(RailTypeFlag::Hidden));
	CHECK(InitialiseTubeRailType(fixture.world.package) == tube);
	ResetRailTypes();
	CHECK(GetTubeRailType() == INVALID_RAILTYPE);
	CHECK_FALSE(IsTubeRailType(tube));
}

/** External main/alternate claims and missing content cannot allocate or seize native capability. */
TEST_CASE("Tube label collisions and missing package refuse before registry mutation", "[transport-tube-registry]")
{
	TubeRailRegistryFixture fixture;
	SECTION("exact foreign label") {
		const RailType foreign = AllocateRailType(RAILTYPE_LABEL_TUBE);
		REQUIRE(foreign != INVALID_RAILTYPE);
		CHECK(InitialiseTubeRailType(fixture.world.package) == INVALID_RAILTYPE);
		CHECK_FALSE(IsTubeRailType(foreign));
		CHECK(GetRailTypeByLabel(RAILTYPE_LABEL_TUBE, false) == foreign);
	}
	SECTION("foreign alternate label") {
		const RailType foreign = AllocateRailType(RailTypeLabel{"OTHR"});
		REQUIRE(foreign != INVALID_RAILTYPE);
		const_cast<RailTypeInfo *>(GetRailTypeInfo(foreign))->alternate_labels.insert(RAILTYPE_LABEL_TUBE);
		CHECK(InitialiseTubeRailType(fixture.world.package) == INVALID_RAILTYPE);
		CHECK(GetRailTypeByLabel(RAILTYPE_LABEL_TUBE, false) == INVALID_RAILTYPE);
		CHECK(GetRailTypeInfo(foreign)->alternate_labels.contains(RAILTYPE_LABEL_TUBE));
	}
	SECTION("missing required package") {
		CHECK(InitialiseTubeRailType({}) == INVALID_RAILTYPE);
		CHECK(GetRailTypeByLabel(RAILTYPE_LABEL_TUBE, false) == INVALID_RAILTYPE);
	}
	CHECK(GetTubeRailType() == INVALID_RAILTYPE);
}

/** Exhausting actual canonical slots preserves every prior definition instead of overwriting an arbitrary railtype. */
TEST_CASE("A full canonical rail registry refuses native tube allocation", "[transport-tube-registry]")
{
	TubeRailRegistryFixture fixture;
	// Reserve all available real slots with distinct external labels. Capacity
	// refusal must leave these owners intact, including the last legal index.
	for (uint slot = 4; slot < RAILTYPE_END; ++slot) {
		const uint8_t label[4] = {'T', 'S', 'T', static_cast<uint8_t>(slot)};
		REQUIRE(AllocateRailType(RailTypeLabel{label}) != INVALID_RAILTYPE);
	}
	const auto last_label = GetRailTypeInfo(static_cast<RailType>(RAILTYPE_END - 1))->label;
	CHECK(InitialiseTubeRailType(fixture.world.package) == INVALID_RAILTYPE);
	CHECK(GetRailTypeInfo(static_cast<RailType>(RAILTYPE_END - 1))->label == last_label);
	CHECK(GetTubeRailType() == INVALID_RAILTYPE);
}
