/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_generator_geometry.cpp Actual native physical descriptor/site boundary, without operating generator activation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group8B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups55-56.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../geothermal.h"
#include "../geothermal_generator_geometry.h"
#include "../industry.h"
#include "../industry_type.h"
#include "../newgrf_commons.h"
#include "../utility_power.h"
#include "../timer/timer_game_tick.h"

#include "../safeguards.h"

/** Actual native source/company world with explicit unit-input floor; this is not paid/transported bootstrap acceptance. */
class GeneratorGeometryRig : public ExcavationFixture {
public:
	TileIndex floor_origin = TileXYL(20, 20, MapLayer::DeepUnderground); ///< Fixed actual fixture floor; malformed request layers cannot change its ownership.
	GeothermalSiteRequest request; ///< Real source/company and existing first-Powerhouse kind; no new construction ID is invented.
	/**
	 * Prepare an actual source-backed first-Powerhouse request before any floor is supplied.
	 * @return No value; native geometry remains solid until explicit fixture input is opened.
	 */
	GeneratorGeometryRig()
	{
		ResetGeothermalSiteClaims();
		this->request.kind = MultiLayerGeothermalConstructionKind::Powerhouse;
		this->request.vent = GetGeologyWorldState().vents.front().id;
		this->request.owner = _current_company;
		this->request.tile = this->request.endpoint = this->floor_origin;
		this->request.access = DiagDirection::SE;
	}
	/**
	 * Retire this isolated fixture's cold registry before its source/company owners disappear.
	 * @return No value; no production registry is supplied by this test fixture.
	 */
	~GeneratorGeometryRig() { ResetGeothermalSiteClaims(); }
	/**
	 * Supply declared floor/access through the geology owner's existing fixture boundary.
	 * @param complete True supplies the full4x4 floor and access; false supplies only a3x3 pocket.
	 * @return No value; production site preparation must not open the missing physical cells.
	 */
	void OpenFloor(bool complete)
	{
		std::vector<TileIndex> floor;
		const int end = complete ? 4 : 2;
		// Explicit unit input opens the selected footprint/access only. The3x3
		// variant leaves the actual4x4 far edge/corner solid for refusal coverage.
		for (int y = -1; y <= end; ++y) {
			for (int x = -1; x <= end; ++x) floor.push_back(TileAddWrap(this->floor_origin, x, y));
		}
		this->Open(floor);
	}
};

/**
 * Verify both immutable physical designs without changing existing native IDs or quantity owners.
 * @return No value; the actual native fixture restores caller profile/company/geology state.
 */
TEST_CASE("Geothermal dated generator geometry is passive and preserves native ownership", "[geothermal-generator-geometry]")
{
	GeneratorGeometryRig rig;
	const auto industries = _industry_mngr.mappings, tiles = _industile_mngr.mappings;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), claims = ComputeGeothermalSiteDigest();
	const auto power = ComputeUtilityPowerDigest();
	const auto tick = TimerGameTick::counter;
	const auto random = std::array{_random.state[0], _random.state[1]};
	const auto date = TimerGameCalendar::date;
	const auto cash = Company::Get(_current_company)->money;
	const auto industry_count = Industry::GetNumItems();
	const auto *first = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Powerhouse);
	const auto *future_construction = GetMultiLayerGeothermalConstruction(static_cast<MultiLayerGeothermalConstructionKind>(3));
	static constexpr std::array models{GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop};
	// Future geometry is readable for art at1980 without installing an advanced
	// type or changing any actual command/date/cargo/Heat admission.
	for (const auto model : models) {
		const auto geometry = GetGeothermalGeneratorGeometry(model); REQUIRE(geometry.has_value());
		CHECK(geometry->model == model); CHECK(geometry->width == 4); CHECK(geometry->height == 4);
		CHECK(geometry->construction_stages == INDUSTRY_COMPLETED + 1); REQUIRE(geometry->cells.size() == 16);
		CHECK(geometry->introduction_year == TimerGameCalendar::Year{model == GeothermalGeneratorModel::VentTap ? 1980 : 2000});
		CHECK(geometry->sprite_asset == (model == GeothermalGeneratorModel::VentTap ? "facility.vent-powerhouse" : "facility.magma-powerhouse"));
		CHECK(geometry->first_build_contract == (model == GeothermalGeneratorModel::VentTap ? first : nullptr));
		// Logical row-major source cells are the actual native world offsets;
		// no giant single-tile building or camera-dependent layout can pass.
		for (size_t index = 0; index < geometry->cells.size(); ++index) {
			CHECK(static_cast<size_t>(geometry->cells[index].x) == index % 4);
			CHECK(static_cast<size_t>(geometry->cells[index].y) == index / 4);
		}
	}
	CHECK_FALSE(GetGeothermalGeneratorGeometry(static_cast<GeothermalGeneratorModel>(UINT8_MAX)).has_value());
	CHECK_FALSE(GetGeothermalGeneratorGeometry(static_cast<GeothermalGeneratorModel>(2)).has_value());
	CHECK(static_cast<uint8_t>(MultiLayerGeothermalConstructionKind::Riser) == 2);
	CHECK(GetMultiLayerGeothermalConstruction(static_cast<MultiLayerGeothermalConstructionKind>(3)) == future_construction);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeGeothermalSiteDigest() == claims);
	CHECK(ComputeUtilityPowerDigest() == power); CHECK(TimerGameTick::counter == tick);
	CHECK(std::array{_random.state[0], _random.state[1]} == random);
	CHECK(TimerGameCalendar::date == date); CHECK(Company::Get(_current_company)->money == cash); CHECK(Industry::GetNumItems() == industry_count);
	// Compare the actual two canonical managers field-wise, avoiding padding or
	// a substitute local ID registry. Descriptor reads cannot publish any mapping.
	for (size_t index = 0; index < industries.size(); ++index) {
		CHECK(_industry_mngr.mappings[index].grfid == industries[index].grfid);
		CHECK(_industry_mngr.mappings[index].entity_id == industries[index].entity_id);
		CHECK(_industry_mngr.mappings[index].substitute_id == industries[index].substitute_id);
	}
	for (size_t index = 0; index < tiles.size(); ++index) {
		CHECK(_industile_mngr.mappings[index].grfid == tiles[index].grfid);
		CHECK(_industile_mngr.mappings[index].entity_id == tiles[index].entity_id);
		CHECK(_industile_mngr.mappings[index].substitute_id == tiles[index].substitute_id);
	}
}

/**
 * Verify the existing paid first-Powerhouse quote against the complete physical source layout.
 * @return No value; actual cold preparation changes neither supplied floor nor native quantities.
 */
TEST_CASE("Geothermal first generator source geometry matches actual whole native site quotes", "[geothermal-generator-geometry]")
{
	GeneratorGeometryRig rig; rig.OpenFloor(true);
	const auto geometry = GetGeothermalGeneratorGeometry(GeothermalGeneratorModel::VentTap); REQUIRE(geometry.has_value());
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), power = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money;
	// All four physical access sides use the same complete world layout and
	// native boundary endpoint. Neither source rotation nor an anchor clips cells.
	for (uint8_t side = 0; side < 4; ++side) {
		rig.request.access = static_cast<DiagDirection>(side);
		const auto quote = PrepareGeothermalSite(rig.request); REQUIRE(quote.error == GeothermalError::None);
		CHECK(quote.construction == geometry->first_build_contract); REQUIRE(quote.footprint.size() == geometry->cells.size());
		for (size_t index = 0; index < geometry->cells.size(); ++index) {
			const auto cell = geometry->cells[index];
			CHECK(quote.footprint[index] == TileAddWrap(rig.request.tile, cell.x, cell.y));
		}
		CHECK(quote.construction->work_ticks == 592); CHECK(quote.construction->inputs.size() == 4);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == power);
		CHECK(Company::Get(_current_company)->money == cash);
	}
}

/**
 * Verify native admission still refuses missing floor, invalid kinds and wrong layer/date.
 * @return No value; source descriptors repair no world state and grant no construction capability.
 */
TEST_CASE("Geothermal generator geometry cannot bypass actual site admission", "[geothermal-generator-geometry]")
{
	GeneratorGeometryRig rig;
	bool complete = true;
	SECTION("missing actual4x4 far edge") { complete = false; }
	SECTION("pre1980 first plant") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("wrong physical layer") { rig.request.tile = rig.request.endpoint = MultiLayerMap::ProjectToSurface(rig.request.tile); }
	SECTION("unknown construction family") { rig.request.kind = static_cast<MultiLayerGeothermalConstructionKind>(UINT8_MAX); }
	SECTION("riser retains its separate portal admission") { rig.request.kind = MultiLayerGeothermalConstructionKind::Riser; }
	SECTION("foreign command actor") { rig.request.owner = CompanyID::Invalid(); }
	rig.OpenFloor(complete);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), power = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money;
	REQUIRE(GetGeothermalGeneratorGeometry(GeothermalGeneratorModel::VentTap).has_value());
	REQUIRE(GetGeothermalGeneratorGeometry(GeothermalGeneratorModel::MagmaLoop).has_value());
	CHECK(PrepareGeothermalSite(rig.request).error != GeothermalError::None);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == power);
	CHECK(Company::Get(_current_company)->money == cash);
}
