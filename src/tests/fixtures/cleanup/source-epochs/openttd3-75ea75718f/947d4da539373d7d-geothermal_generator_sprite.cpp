/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_sprite.cpp Actual native paid work/ports/conversion select approved generator art without acquiring gameplay rights.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BB.1.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Explicit prepared floor and receiver-boundary material fixtures test isolated presentation; original paid-world raster acceptance remains84C.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_generator_operating_fixture.h"
#include "../geothermal_sprite.h"
#include "../multilayer_sprite.h"
#include "../industry_map.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** @return Actual native machinery/material/history/source/site/clock/company/RNG observation before a pure presentation burst, never another inventory or work owner. */
static std::string CaptureGeneratorSpriteOwners()
{
	std::string result = fmt::format("{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}", ComputeGeothermalGeneratorDigest(), ComputeGeothermalCollectorDigest(), ComputeGeothermalSiteDigest(),
		ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeUtilityPowerDigest(), Company::Get(_current_company)->money.base(), TimerGameTick::counter,
		_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]);
	// Native Industry remains the physical received stock/history owner, even
	// before the whole construction bill commits. Sprite selection cannot take
	// another receipt, finish work or modify a valid accepted cargo history.
	for (const Industry *industry : Industry::Iterate()) {
		result += fmt::format("/{}:{}:{}", industry->index.base(), industry->counter, industry->valid_history);
		for (const auto &slot : industry->accepted) {
			result += fmt::format(":{}:{}:{}:{}", slot.cargo, slot.waiting, slot.accumulated_waiting, slot.last_accepted.base());
			if (slot.history == nullptr) continue;
			for (uint month = 0; month < slot.history->size(); ++month) {
				if (month != THIS_MONTH && (industry->valid_history & (uint64_t{1} << month)) == 0) continue; // Only native saved valid/current history has receipt authority; stale unwritten slots cannot become test-owned state.
				result += fmt::format(":{}", (*slot.history)[month].accepted);
			}
		}
	}
	return result;
}

/** @param industry Actual funded native model. @param expected_state Approved source state independently required by this actual native scenario. Require every native cell/back-front/lamp frame to select its original model/access, preserving all gameplay owners. */
static void CheckGeneratorSpriteCells(IndustryID industry, uint8_t expected_state)
{
	const auto *native = GetGeothermalGeneratorState(industry); REQUIRE(native != nullptr); const auto &building = *Industry::Get(industry);
	const auto *claim = GetGeothermalSiteClaim(native->site); REQUIRE(claim != nullptr);
	const auto asset = native->model == GeothermalGeneratorModel::VentTap ? MultiLayerSpriteAsset::FacilityVentPowerhouse : MultiLayerSpriteAsset::FacilityMagmaPowerhouse;
	const auto before = CaptureGeneratorSpriteOwners();
	// All original16 cells and both authored parts must preserve the native
	// row-major footprint/access. Advancing only a visual lamp clock cannot
	// earn material intake, construction, Heat, conversion or a service bill.
	for (const auto visual : {uint64_t{0}, uint64_t{8}, uint64_t{16}, uint64_t{24}}) {
		for (uint y = 0; y < building.location.h; ++y) {
			for (uint x = 0; x < building.location.w; ++x) {
				const auto tile = TileAddWrap(building.location.tile, x, y); REQUIRE(Industry::GetByTile(tile)->index == industry);
				const uint8_t frame = expected_state == 0 ? GetIndustryConstructionStage(tile) : GetMultiLayerVisualFrame(asset, visual, false);
				for (const bool front : {false, true}) {
					const auto expected = GetMultiLayerSprite(asset, {expected_state, frame, static_cast<uint8_t>(claim->site.access), static_cast<uint16_t>(y * building.location.w + x), static_cast<uint8_t>(front)});
					REQUIRE(expected.has_value()); CHECK(GetGeothermalGeneratorSprite(tile, front, visual) == expected); CHECK(GetMultiLayerIndustrySprite(tile, front, visual) == expected);
				}
				const auto bounds = GetGeothermalIndustryFrontBounds(tile); REQUIRE(bounds.has_value()); CHECK(bounds->extent.x == TILE_SIZE); CHECK(bounds->extent.y == TILE_SIZE); CHECK(bounds->extent.z == 64);
			}
		}
	}
	CHECK(CaptureGeneratorSpriteOwners() == before);
}

/** Both actual funded models earn native construction stages at each admitted access side; ready artwork cannot finish an unfinished machine. */
TEST_CASE("Native generator art retains actual four-view construction and whole paid footprint", "[geothermal-generator-art-construction]")
{
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop); const auto access = GENERATE(DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW);
	GeothermalGeneratorRig rig{false, model, access}; REQUIRE(rig.Fund().Succeeded()); const auto industry = rig.Generator().index;
	const auto *terms = GetMultiLayerGeothermalConstruction(rig.request.kind); CheckGeneratorSpriteCells(industry, 0); rig.DeliverBill(); uint16_t completed = 0;
	// Native prepaid work, not a tile-bit assignment, earns the three partial
	// construction quarters and finally all sixteen completed physical cells.
	for (uint stage = 1; stage < 4; ++stage) {
		const auto target = static_cast<uint16_t>(terms->work_ticks * stage / 4); rig.Tick(target - completed); completed = target; CheckGeneratorSpriteCells(industry, 0);
	}
	rig.Tick(terms->work_ticks - completed); REQUIRE(IsGeothermalGeneratorConstructionComplete(industry)); CheckGeneratorSpriteCells(industry, 3);
	CHECK(GetGeothermalGeneratorState(industry)->heat_consumer.base() == UINT32_MAX); CHECK(GetGeothermalGeneratorState(industry)->electrical_producer.base() == UINT32_MAX);
	CHECK_FALSE(GetGeothermalGeneratorSprite(INVALID_TILE, false, 0).has_value());
	CHECK_FALSE(GetGeothermalGeneratorSprite(MultiLayerMap::TileInLayer(rig.request.tile, MapLayer::Surface), false, 0).has_value());
}

/** Real original source/path loss and paid restoration choose starved/working states without borrowing previous-interval output or manufacturing a grant. */
TEST_CASE("Native generator art reads actual ready conversion Heat loss and paid repair", "[geothermal-generator-art-operation]")
{
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop); GeothermalGeneratorOperatingRig rig{false, true, 1000000, model};
	CheckGeneratorSpriteCells(rig.generator, 1); rig.Operate(); CheckGeneratorSpriteCells(rig.generator, 2);
	const auto native = *GetGeothermalGeneratorState(rig.generator); const auto before = GetUtilityPowerProducer(native.electrical_producer)->generated_lifetime_j;
	const auto cell = rig.pipe[rig.pipe.size() / 2]; REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, cell, UtilityEnergyDomain::Heat).Succeeded());
	rig.Operate(); CheckGeneratorSpriteCells(rig.generator, 4); CHECK(GetUtilityPowerProducer(native.electrical_producer)->generated_lifetime_j == before);
	rig.Build(cell, UtilityEnergyDomain::Heat); rig.Operate(); CheckGeneratorSpriteCells(rig.generator, 2);
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, rig.plant_port, UtilityEnergyDomain::Electricity).Succeeded()); CheckGeneratorSpriteCells(rig.generator, 3);
	const auto collector_tile = Industry::Get(rig.collector)->location.tile;
	CHECK(GetMultiLayerIndustrySprite(collector_tile, false, 0) == GetMultiLayerCollectorSprite(collector_tile, false, 0)); CHECK_FALSE(GetGeothermalIndustryFrontBounds(collector_tile).has_value());
	CHECK_FALSE(GetGeothermalGeneratorSprite(collector_tile, false, 0).has_value());
}
