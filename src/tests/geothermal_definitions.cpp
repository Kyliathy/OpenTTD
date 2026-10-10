/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geothermal_definitions.cpp Actual IIDS/TIDS collector identity, Deep/date separation and complete registration refusal.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1B
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../geothermal.h"
#include "../multilayer_industry.h"
#include "../layer_industry_processor.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../newgrf_commons.h"
#include "../industry_cmd.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

/** Actual profile/map/manager fixture; installing a definition creates no industry, delivered stock or operating collector. */
class GeothermalDefinitionRig : public ExcavationFixture {
	decltype(_industry_mngr.mappings) industry_mappings = _industry_mngr.mappings; ///< Caller IIDS identities restored after isolated definition checks.
	decltype(_industile_mngr.mappings) tile_mappings = _industile_mngr.mappings; ///< Caller TIDS identities, independent of native industry slots.
public:
	/** Reset native specs to actual original templates for one isolated catalogue preflight. */
	GeothermalDefinitionRig() { REQUIRE(Industry::GetNumItems() == 0); ResetIndustries(); }
	/** Restore canonical managers/specs before the inherited real profile/company/map teardown. */
	~GeothermalDefinitionRig()
	{
		ResetIndustries(); _industry_mngr.mappings = std::move(this->industry_mappings); _industile_mngr.mappings = std::move(this->tile_mappings);
	}
};

/** Collector registration retains exact native geometry/input-only capability and stays separate from Surface processing and unpaid generic funding. */
TEST_CASE("Geothermal native collector definition has real input-only Deep identity", "[geothermal-definition]")
{
	GeothermalDefinitionRig rig;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	REQUIRE(InitialiseGeothermalCollectorDefinitions(rig.ActivatedContent()));
	const auto type = GetGeothermalCollectorIndustryType(); REQUIRE(type != IT_INVALID);
	CHECK(IsGeothermalCollectorDefinition(type)); CHECK_FALSE(IsLayerMineDefinition(type)); CHECK_FALSE(IsLayerProcessorDefinition(type));
	const auto *capability = GetLayerIndustryCapability(type); REQUIRE(capability != nullptr);
	CHECK(capability->layers.Test(MapLayer::DeepUnderground)); CHECK_FALSE(capability->layers.Test(MapLayer::Surface));
	CHECK(capability->introduction_year == TimerGameCalendar::Year{1980});
	const auto *spec = GetIndustrySpec(type); CHECK(spec->layouts.front().size() == 9);
	CHECK(std::ranges::count_if(spec->accepts_cargo, IsValidCargoType) == 3); CHECK_FALSE(std::ranges::any_of(spec->produced_cargo, IsValidCargoType));
	const auto &vent = GetGeologyWorldState().vents.front(); const auto deep = TileXYL(vent.x, vent.y, vent.layer);
	CHECK(CheckLayerIndustryConstruction(deep, type).Succeeded());
	CHECK(CheckLayerIndustryConstruction(MultiLayerMap::ProjectToSurface(deep), type).Failed());
	CHECK(CmdBuildIndustry({}, deep, type, 0, true, 123).Failed());
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK(CheckLayerIndustryConstruction(deep, type).Failed());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(Industry::GetNumItems() == 0);
	CHECK(IsGeothermalCollectorDefinition(type)); CHECK(GetGeothermalCollectorIndustryType() == type);
	REQUIRE(InitialiseGeothermalCollectorDefinitions(rig.ActivatedContent())); CHECK(GetGeothermalCollectorIndustryType() == type);
}

/** Missing package/cargo and either full canonical mapping table refuse before registering any part of a collector. */
TEST_CASE("Geothermal whole definition preflight preserves managers on package cargo or capacity refusal", "[geothermal-definition]")
{
	GeothermalDefinitionRig rig;
	bool missing_package = false;
	SECTION("missing exact package") { missing_package = true; }
	SECTION("missing delivered input") { CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"ELEC"}))->label = CT_INVALID; BuildCargoLabelMap(); }
	SECTION("full IIDS") {
		// Fill only fixture-owned public mapping slots. The real canonical
		// allocator must refuse without publishing nine orphan TIDS definitions.
		for (size_t index = 0; index < _industry_mngr.mappings.size(); ++index) _industry_mngr.mappings[index] = {GrfID{"SALT"}, static_cast<uint16_t>(index + 1), 0};
	}
	SECTION("full TIDS") {
		// Exhausting tile identities cannot reserve a standalone collector
		// industry identity; whole family capacity precedes every registry write.
		for (size_t index = _industile_mngr.GetMaxOffset(); index < _industile_mngr.mappings.size(); ++index) _industile_mngr.mappings[index] = {GrfID{"SALT"}, static_cast<uint16_t>(index + 1), 0};
	}
	const auto industries = _industry_mngr.mappings, tiles = _industile_mngr.mappings;
	const bool installed = missing_package ? InitialiseGeothermalCollectorDefinitions(GRFConfigList{}) : InitialiseGeothermalCollectorDefinitions(rig.ActivatedContent());
	CHECK_FALSE(installed);
	// Field-wise comparison avoids padding and checks the actual two
	// authoritative allocators; no substitute private ID registry is inferred.
	for (size_t index = 0; index < industries.size(); ++index) {
		CHECK(_industry_mngr.mappings[index].grfid == industries[index].grfid); CHECK(_industry_mngr.mappings[index].entity_id == industries[index].entity_id);
	}
	for (size_t index = 0; index < tiles.size(); ++index) {
		CHECK(_industile_mngr.mappings[index].grfid == tiles[index].grfid); CHECK(_industile_mngr.mappings[index].entity_id == tiles[index].entity_id);
	}
}

/** An enabled malformed canonical occupant cannot be repaired into a collector by reinitialization, preserving shared native tile/industry ownership. */
TEST_CASE("Geothermal native definition refuses changed input or native tile ownership", "[geothermal-definition]")
{
	GeothermalDefinitionRig rig; REQUIRE(InitialiseGeothermalCollectorDefinitions(rig.ActivatedContent()));
	const auto type = GetGeothermalCollectorIndustryType(); REQUIRE(type != IT_INVALID);
	const auto gfx = GetIndustrySpec(type)->layouts.front().front().gfx;
	SECTION("changed inventory meaning") { _industry_specs[type].accepts_cargo[0] = INVALID_CARGO; }
	SECTION("changed tile ownership") { _industry_tile_specs[gfx].grf_prop.grfid = GrfID{"FORE"}; }
	SECTION("changed tile acceptance") { _industry_tile_specs[gfx].acceptance[0] = 7; }
	CHECK_FALSE(IsGeothermalCollectorDefinition(type)); CHECK(GetLayerIndustryCapability(type) == nullptr);
	CHECK_FALSE(InitialiseGeothermalCollectorDefinitions(rig.ActivatedContent()));
	CHECK(GetGeothermalCollectorIndustryType() == IT_INVALID);
}

/** Canonical orphan TIDS identity cannot authorize replacing an enabled foreign occupant before the industry definition exists. */
TEST_CASE("Geothermal complete tile preflight preserves an enabled orphan foreign tile", "[geothermal-definition]")
{
	GeothermalDefinitionRig rig; REQUIRE(InitialiseGeothermalCollectorDefinitions(rig.ActivatedContent()));
	const auto type = GetGeothermalCollectorIndustryType(); const auto gfx = GetIndustrySpec(type)->layouts.front().front().gfx;
	_industry_specs[type].enabled = false; _industry_tile_specs[gfx].grf_prop.grfid = GrfID{"FORE"};
	CHECK_FALSE(InitialiseGeothermalCollectorDefinitions(rig.ActivatedContent()));
	CHECK(_industry_tile_specs[gfx].grf_prop.grfid == GrfID{"FORE"}); CHECK_FALSE(_industry_specs[type].enabled);
}
