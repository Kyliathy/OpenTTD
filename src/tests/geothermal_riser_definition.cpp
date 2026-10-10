/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_riser_definition.cpp Shared native riser definitions preserve collector identities and actual four-input meaning.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../geothermal_riser.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../newgrf_commons.h"
#include "../multilayer_industry.h"

#include "../safeguards.h"

/** Native definition boundary with caller mappings restored after testing; no actual industry, stock or energy is created. */
class RiserDefinitionRig : public ExcavationFixture {
	decltype(_industry_mngr.mappings) industry_mappings = _industry_mngr.mappings; ///< Existing IIDS allocations, owned by the ordinary native manager.
	decltype(_industile_mngr.mappings) tile_mappings = _industile_mngr.mappings; ///< Existing TIDS allocations, independent of funding or cargo.
public:
	/** Install original specs before exercising complete native-family registration. */
	RiserDefinitionRig() { REQUIRE(Industry::GetNumItems() == 0); ResetIndustries(); }
	/** Restore original shared identity managers and release only this fixture's temporary specifications. */
	~RiserDefinitionRig()
	{
		ResetIndustries();
		_industry_mngr.mappings = std::move(this->industry_mappings);
		_industile_mngr.mappings = std::move(this->tile_mappings);
	}
};

/** Separate native families use the same canonical registration code without aliasing paid collector machinery or a vent source. */
TEST_CASE("Native riser definitions preserve collector IDs and all four construction cargos", "[geothermal-riser-definition]")
{
	RiserDefinitionRig rig;
	REQUIRE(InitialiseGeothermalCollectorDefinitions(rig.ActivatedContent()));
	const auto collector = GetGeothermalCollectorIndustryType();
	const auto collector_tile = GetIndustrySpec(collector)->layouts.front().front().gfx;
	REQUIRE(InitialiseGeothermalRiserDefinitions(rig.ActivatedContent()));
	const auto riser = GetGeothermalRiserIndustryType();
	REQUIRE(riser != IT_INVALID);
	CHECK(riser != collector);
	CHECK(GetGeothermalCollectorIndustryType() == collector);
	CHECK(GetIndustrySpec(collector)->layouts.front().front().gfx == collector_tile);
	CHECK(GetIndustrySpec(riser)->layouts.front().size() == 4);
	CHECK(GetIndustrySpec(riser)->layouts.front().front().gfx != collector_tile);
	CHECK(std::ranges::count_if(GetIndustrySpec(riser)->accepts_cargo, IsValidCargoType) == 4);
	CHECK_FALSE(std::ranges::any_of(GetIndustrySpec(riser)->produced_cargo, IsValidCargoType));
	const auto *capability = GetGeothermalRiserIndustryCapability(riser);
	REQUIRE(capability != nullptr);
	CHECK(capability->layers.Test(MapLayer::Underground));
	CHECK(capability->layers.Test(MapLayer::DeepUnderground));
	CHECK_FALSE(capability->layers.Test(MapLayer::Surface));
	REQUIRE(InitialiseGeothermalRiserDefinitions(rig.ActivatedContent()));
	CHECK(GetGeothermalRiserIndustryType() == riser);
	CHECK(Industry::GetNumItems() == 0);
}

/** Missing package identity or changed accepted meaning cannot repurpose a canonical saved facility into a working riser. */
TEST_CASE("Native riser definitions refuse foreign content and changed construction stock", "[geothermal-riser-definition]")
{
	RiserDefinitionRig rig;
	SECTION("missing content") {
		const auto industries = _industry_mngr.mappings;
		const auto tiles = _industile_mngr.mappings;
		CHECK_FALSE(InitialiseGeothermalRiserDefinitions({}));
		// Compare the actual native identity fields without depending on padding;
		// a refused package must leave every existing owner and free slot intact.
		for (size_t i = 0; i < industries.size(); ++i) {
			CHECK(_industry_mngr.mappings[i].grfid == industries[i].grfid);
			CHECK(_industry_mngr.mappings[i].entity_id == industries[i].entity_id);
		}
		for (size_t i = 0; i < tiles.size(); ++i) {
			CHECK(_industile_mngr.mappings[i].grfid == tiles[i].grfid);
			CHECK(_industile_mngr.mappings[i].entity_id == tiles[i].entity_id);
		}
	}
	SECTION("missing fourth construction input") {
		REQUIRE(InitialiseGeothermalRiserDefinitions(rig.ActivatedContent()));
		const auto type = GetGeothermalRiserIndustryType();
		const CargoType battery = GetCargoTypeByLabel(CargoLabel{"BATY"});
		auto &inputs = _industry_specs[type].accepts_cargo;
		const auto slot = std::ranges::find(inputs, battery);
		REQUIRE(slot != inputs.end());
		*slot = INVALID_CARGO;
		CHECK_FALSE(IsGeothermalRiserDefinition(type));
		CHECK_FALSE(InitialiseGeothermalRiserDefinitions(rig.ActivatedContent()));
		CHECK(*slot == INVALID_CARGO);
	}
}
