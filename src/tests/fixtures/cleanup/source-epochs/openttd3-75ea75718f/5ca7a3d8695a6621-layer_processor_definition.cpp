/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file layer_processor_definition.cpp Actual funded Surface processor geometry for dependent source art.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Native command-body fixtures prove quoted costs and actual construction, not a driven transport loop.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "processor_fixture.h"
#include "../layer_processor_presentation.h"

#include "../safeguards.h"

/** Native command fixture that restores caller TIDS mappings after each independent family. */
class ProcessorGeometryRig : public MineAdmissionRig {
	decltype(_industile_mngr.mappings) tile_mappings = _industile_mngr.mappings; ///< Caller identities, never source-created tile IDs.

public:
	/** Construct the existing native content/company/map owner before installing processor definitions. */
	ProcessorGeometryRig() : MineAdmissionRig() {}
	/** Restore native tile identities; the parent removes actual fixture industries and state. */
	~ProcessorGeometryRig() { _industile_mngr.mappings = std::move(this->tile_mappings); }
};

/**
 * Emit exact ordered cells from the installed native industry and tile specifications.
 * @param primary Existing primary recipe identifying this actual native Surface family.
 * @param asset_id Capability-owned public art family, never inferred from an industry name.
 * @param spec Actual installed IIDS definition whose only layout funded the industry.
 */
static void PrintProcessorArtGeometry(MultiLayerRecipeID primary, std::string_view asset_id, const IndustrySpec &spec)
{
	fmt::print("PROCESSOR_ART_GEOMETRY {{\"asset_id\":\"{}\",\"primary_recipe\":\"{}\",\"industry_local_id\":{},\"layout_count\":{},\"machinery_count\":{},\"cells\":[",
		asset_id, primary.AsString(), spec.grf_prop.local_id, spec.layouts.size(), spec.layouts.front().size());
	bool first = true;
	// Export actual native order and TIDS ancestry. A plausible painted4x4
	// rectangle cannot substitute for the exact installed/funded machinery.
	for (const auto &cell : spec.layouts.front()) {
		const auto *tile = GetIndustryTileSpec(cell.gfx);
		fmt::print("{}{{\"cell\":[{},{}],\"tile_local_id\":{},\"substitute_graphics\":{}}}", first ? "" : ",",
			cell.ti.x, cell.ti.y, tile->grf_prop.local_id, tile->grf_prop.subst_id);
		first = false;
	}
	fmt::print("]}}\n");
}

/** Four actual selected Surface definitions preserve estimates and fund all sixteen exact machinery cells. */
TEST_CASE("Selected Surface processor definitions fund one exact sixteen-cell layout", "[layer-processor-definition]")
{
	const size_t index = GENERATE(0, 1, 2, 3);
	constexpr std::array primaries{MultiLayerRecipeID{"CFMT"}, MultiLayerRecipeID{"CHEM"}, MultiLayerRecipeID{"BATT"}, MultiLayerRecipeID{"ELEC"}};
	constexpr std::array<std::string_view, 4> assets{"facility.metal-refinery", "facility.chemical-works", "facility.battery-works", "facility.electronics-works"};
	ProcessorGeometryRig rig;
	REQUIRE(InitialiseLayerProcessorDefinitions(rig.ActivatedContent()));
	rig.PrepareFunding();
	_game_mode = GameMode::Normal;
	const IndustryType type = GetLayerProcessorIndustryType(primaries[index]);
	REQUIRE(type != IT_INVALID);
	const auto *capability = GetLayerIndustryCapability(type);
	REQUIRE(capability != nullptr);
	CHECK(capability->asset_id == assets[index]);
	CHECK(capability->recipe == primaries[index]);
	CHECK(capability->resource == CT_INVALID);
	CHECK(capability->layers.Test(MapLayer::Surface));
	const auto *spec = GetIndustrySpec(type);
	CHECK(spec->grf_prop.local_id == LAYER_PROCESSOR_LOCAL_BASE + index);
	REQUIRE(spec->layouts.size() == 1);
	REQUIRE(spec->layouts.front().size() == 16);
	const TileIndex origin = TileXYL(30, 30, MapLayer::Surface);
	const auto cash = Company::Get(_current_company)->money;
	const auto random = _random, interactive = _interactive_random;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto quote = CmdBuildIndustry({}, origin, type, 0, true, 123);
	REQUIRE(quote.Succeeded());
	CHECK(quote.GetCost() > 0);
	CHECK(Industry::GetNumItems() == 0);
	CHECK(CaptureLayerProcessors().empty());
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == random.state[0]);
	CHECK(_random.state[1] == random.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]);
	CHECK(_interactive_random.state[1] == interactive.state[1]);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	const auto funded = CmdBuildIndustry(DoCommandFlag::Execute, origin, type, 0, true, 123);
	REQUIRE(funded.Succeeded());
	CHECK(funded.GetCost() == quote.GetCost());
	REQUIRE(Industry::GetNumItems() == 1);
	const Industry *industry = *Industry::Iterate().begin();
	CHECK(industry->type == type);
	CHECK(industry->location.w == 4);
	CHECK(industry->location.h == 4);
	REQUIRE(GetLayerProcessorRecipe(*industry) != nullptr);
	CHECK(GetLayerProcessorOperationalStatus(*industry) == LayerProcessorOperationalStatus::Construction);
	size_t variant = 0;
	// Each actual cell must retain its exact row-major machinery/TIDS identity;
	// there is no saved art rotation or invented loading extension to supply it.
	for (const auto &cell : spec->layouts.front()) {
		CHECK(cell.ti.x == variant % 4);
		CHECK(cell.ti.y == variant / 4);
		const auto *tile = GetIndustryTileSpec(cell.gfx);
		CHECK(tile->enabled);
		CHECK(tile->grf_prop.local_id == LAYER_PROCESSOR_LOCAL_BASE + index * 16 + variant);
		CHECK(tile->grf_prop.subst_id < NEW_INDUSTRYTILEOFFSET);
		const TileIndex world = TileAddWrap(origin, cell.ti.x, cell.ti.y);
		REQUIRE(industry->TileBelongsToIndustry(world));
		CHECK(GetIndustryGfx(world) == cell.gfx);
		CHECK(GetIndustryConstructionStage(world) == 0);
		++variant;
	}
	PrintProcessorArtGeometry(primaries[index], assets[index], *spec);
}
