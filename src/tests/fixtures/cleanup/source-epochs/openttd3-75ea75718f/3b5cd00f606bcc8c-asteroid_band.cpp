/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_band.cpp Shared static-footprint, moving-envelope and translated-attachment separation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group6
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Shared Space geometry and motion defaults
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_structure.h"
#include "../space_vehicle.h"
#include "../multilayer_asteroid_catalogue.h"
#include "../safeguards.h"

/** Verify every supported rectangular plane and complete static recipe at both body-strip boundaries. */
TEST_CASE("Static module footprints and approaches reserve the shared outer regions", "[asteroid-band]")
{
	// Every supported rectangle must keep its whole site/approach outside the declared middle-half strip, irrespective of camera or density.
	for (uint32_t width = 64; width <= 4096; width *= 2) for (uint32_t height = 64; height <= 4096; height *= 2) {
		const auto band = GetSpaceTravelBand(width, height);
		CHECK(band.min.y - 2 * SPACE_Q16 == static_cast<int32_t>(height / 4) * SPACE_Q16);
		CHECK(band.max.y + 2 * SPACE_Q16 == static_cast<int32_t>(3 * height / 4 - 1) * SPACE_Q16);
		// Definition widths/heights describe the actual producer, not a copied list of module footprints.
		for (const auto &definition : GetSpaceConstructionDefinitions()) {
			SpaceStructureState site; site.kind = definition.kind; site.x = 4;
			// Carried/mobile recipes cannot reserve a static body exclusion footprint.
			if (definition.width == 0 || definition.height == 0) { CHECK_FALSE(IsSpaceStructureSiteValid(site, width, height)); continue; }
			site.y = static_cast<uint16_t>(height / 4 - definition.height - 1);
			CHECK(IsSpaceStructureSiteValid(site, width, height));
			const auto north_approach = GetSpaceStructureApproach(site);
			CHECK(north_approach.y + SPACE_Q16 <= band.min.y - 2 * SPACE_Q16);
			++site.y; CHECK_FALSE(IsSpaceStructureSiteValid(site, width, height));
			site.y = static_cast<uint16_t>(3 * height / 4 - 1); CHECK_FALSE(IsSpaceStructureSiteValid(site, width, height));
			++site.y; CHECK(IsSpaceStructureSiteValid(site, width, height));
			CHECK(int32_t{site.y} * SPACE_Q16 > band.max.y + 2 * SPACE_Q16);
		}
	}
}

/** Verify every declared velocity at every corner while retaining the actual mission offset and full reserved envelope. */
TEST_CASE("Reflected body envelopes and attached points cannot enter static regions", "[asteroid-band]")
{
	const auto offset = SpaceMission{}.attachment_offset;
	const auto headings = GetMultiLayerAsteroidHeadingDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto speeds = GetMultiLayerAsteroidSpeedDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto *profile = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	// The current independent size catalogue fits the shared reserved margin without shrinking the population or its travel band.
	for (const auto &size : profile->asteroid_sizes) {
		CHECK(size.mining_slots == 1); CHECK(size.diameter_q16 / 2 <= 2 * SPACE_Q16);
	}
	// Exhaust map shape, direction/speed and corner signs, including maximum carried fractional travel before reflection.
	for (uint32_t width = 64; width <= 4096; width *= 2) for (uint32_t height = 64; height <= 4096; height *= 2) {
		const auto band = GetSpaceTravelBand(width, height);
		for (const auto &heading : headings) for (const auto &speed : speeds) for (bool right : {false, true}) for (bool bottom : {false, true}) {
			SpaceMotion motion{{right ? band.max.x : band.min.x, bottom ? band.max.y : band.min.y},
				static_cast<int32_t>(int64_t{speed.distance_q16_per_day} * heading.x_q16 / SPACE_Q16),
				static_cast<int32_t>(int64_t{speed.distance_q16_per_day} * heading.y_q16 / SPACE_Q16), 73, 73};
			AdvanceSpaceMotion(motion, band);
			CHECK(band.Contains(motion.position));
			CHECK(motion.position.y - 2 * SPACE_Q16 >= static_cast<int32_t>(height / 4) * SPACE_Q16);
			CHECK(motion.position.y + 2 * SPACE_Q16 < static_cast<int32_t>(3 * height / 4) * SPACE_Q16);
			const SpacePoint attached{motion.position.x + offset.x, motion.position.y + offset.y};
			CHECK(attached.x >= 0); CHECK(attached.x < static_cast<int32_t>(width) * SPACE_Q16);
			CHECK(attached.y >= static_cast<int32_t>(height / 4) * SPACE_Q16); CHECK(attached.y < static_cast<int32_t>(3 * height / 4) * SPACE_Q16);
		}
	}
}
