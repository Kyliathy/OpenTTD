/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file solar_power_contract.cpp Draft M0 signatures/defaults and unsupported ETI identity fixture. @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../solar_power.h"

#include "../safeguards.h"

/** Compile draft types without invoking the undeveloped operational solar function. */
TEST_CASE("Draft solar contract uses shared utility identity and invalid targets", "[solar-power-contract]")
{
	SolarGenerationAccount source;
	MirrorAssignment assignment;
	SolarMirrorOutput output;
	CHECK(source.industry == IndustryID::Invalid());
	CHECK(source.utility_producer == UtilityProducerID{UINT32_MAX});
	CHECK(source.baseline_energy_j == 0);
	CHECK(source.mirror_remainder == 0);
	CHECK(assignment.mirror_id == SolarMirrorID{UINT32_MAX});
	CHECK(assignment.target == IndustryID::Invalid());
	CHECK_FALSE(assignment.operational);
	CHECK(output.effective_j == 0);
	STATIC_REQUIRE(std::is_invocable_r_v<std::optional<SolarMirrorOutput>, decltype(&ComputeMirrorSolarOutput), const SolarGenerationAccount &, const MirrorAssignment *, uint16_t>);
	GRFConfigList candidate;
	auto &eti = candidate.emplace_back(std::make_unique<GRFConfig>("energy_transition.grf"));
	eti->ident.grfid = GrfID{"RE01"};
	CHECK(PrepareMultiLayerProfileContent(MultiLayerGameplayProfileID::MultilayerIndustry, LandscapeType::Temperate, candidate) == MultiLayerProfileError::UnsupportedContent);
}
