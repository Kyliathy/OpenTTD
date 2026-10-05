/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_contract.cpp Public type/signature fixture without pretending draft jobs/cuts already execute. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 7 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../excavation.h"

#include "../safeguards.h"

/** Compile exact producer interfaces and safe invalid defaults without calling undeveloped mutation functions. */
TEST_CASE("Excavation handoff keeps typed invalid references and one external material owner", "[excavation-contract]")
{
	ExcavationJob job;
	ExcavationCutRequest request;
	ExcavationCutResult result;
	ExcavationMaterialSnapshot material;
	CHECK(job.id == ExcavationJobID{UINT32_MAX});
	CHECK(job.company == CompanyID::Invalid());
	CHECK(job.cutter == VehicleID::Invalid());
	CHECK(job.origin == StationID::Invalid());
	CHECK(job.model_id == UINT16_MAX);
	CHECK(job.commissioning_anchor == INVALID_TILE);
	CHECK(job.state == ExcavationJobState::Proposed);
	CHECK(job.route.empty());
	CHECK(job.cursor == 0);
	CHECK(job.partial_work_ticks == 0);
	CHECK(request.job == ExcavationJobID{UINT32_MAX});
	CHECK(result.earth_t == 0);
	CHECK(result.rock_t == 0);
	CHECK(material.earth_space_t == 0);
	CHECK(material.rock_space_t == 0);
	STATIC_REQUIRE(std::is_abstract_v<ExcavationMaterialParticipant>);
	using CommitSignature = ExcavationCutResult (*)(const ExcavationCutRequest &, ExcavationMaterialParticipant &);
	STATIC_REQUIRE(std::is_same_v<decltype(&CommitExcavationCut), CommitSignature>);
	using FootprintSignature = ExcavationFootprintResult (*)(std::span<const TileIndex>);
	STATIC_REQUIRE(std::is_same_v<decltype(static_cast<FootprintSignature>(&CheckExcavatedFootprint)), FootprintSignature>);
	STATIC_REQUIRE(to_underlying(MapLayer::Space) == 2);
	STATIC_REQUIRE(to_underlying(MapLayer::DeepUnderground) == 3);
}
