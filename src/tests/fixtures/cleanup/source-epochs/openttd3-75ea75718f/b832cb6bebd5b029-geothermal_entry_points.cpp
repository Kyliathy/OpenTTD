/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_entry_points.cpp Generic native funding cannot replace explicit dated geothermal participants.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group13B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Boundary fixtures explicitly prepare their floor; the separate8F receipt owns the complete paid bootstrap.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_generator_fixture.h"
#include "geothermal_riser_fixture.h"
#include "../tile_map.h"
#include "../safeguards.h"

/**
 * Verify the ordinary public funding entry point cannot invent a missing explicit geothermal participant.
 * @param tile Actual admissible dated native floor, whose dedicated command already quoted successfully.
 * @param type Exact admitted native generator or riser definition; translated names are not used.
 */
static void CheckGenericGeothermalFundingRefusal(TileIndex tile, IndustryType type)
{
	const auto cash = Company::Get(_current_company)->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), sites = ComputeGeothermalSiteDigest();
	const auto utility = ComputeUtilityPowerDigest(), portals = ComputeLayerPortalWorkDigest();
	const auto random = std::array{_random.state[0], _random.state[1]};
	const auto tile_type = GetTileType(tile); const auto industries = Industry::GetNumItems();
	// Quote and real execution enter the ordinary industry command separately.
	// A real valid company/floor/date must still supply its original explicit
	// cold site or commissioned portal request; generic funding creates none.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::BuildIndustry>::Do(flags, tile, type, uint32_t{0}, true, uint32_t{123}).Failed());
		CHECK(Company::Get(_current_company)->money == cash); CHECK(Industry::GetNumItems() == industries);
		CHECK(GetTileType(tile) == tile_type); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		CHECK(ComputeGeothermalSiteDigest() == sites); CHECK(ComputeUtilityPowerDigest() == utility);
		CHECK(ComputeLayerPortalWorkDigest() == portals); CHECK((std::array{_random.state[0], _random.state[1]} == random));
	}
}

/** Both actual dated models require their original cold site even when the ordinary native funding command executes. */
TEST_CASE("Generic native funding cannot impersonate either geothermal generator", "[geothermal-generator-generic]")
{
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	GeothermalGeneratorRig rig{false, model}; REQUIRE(rig.Fund(true).Succeeded());
	CheckGenericGeothermalFundingRefusal(rig.request.tile, GetGeothermalGeneratorIndustryType(model));
	CHECK(GetGeothermalSiteClaim(rig.site)->active); CHECK(CaptureGeothermalGenerators().empty());
}

/** An actual connected paid portal and admissible floor do not authorize generic funding without the explicit complete riser request. */
TEST_CASE("Generic native funding cannot omit the physical riser participants", "[geothermal-riser-generic]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund(true).Succeeded());
	CheckGenericGeothermalFundingRefusal(rig.request.tile, GetGeothermalRiserIndustryType());
	CHECK(CaptureGeothermalRisers().empty());
}
