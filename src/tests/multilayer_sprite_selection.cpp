/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_sprite_selection.cpp Native M1 pose boundaries and presentation-only animation checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group81B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.6
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_sprite.h"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include <set>

#include "../safeguards.h"

/** All selected families own distinct physical poses; invalid indices cannot cross a native family boundary. */
TEST_CASE("M1 native selectors refuse undeclared poses and preserve protected ranges", "[m1-sprite-selection]")
{
	std::set<SpriteID> selected;
	// Audit every delivered physical pose, including all receiver cells and portal
	// endpoints. A range collision would silently show unrelated gameplay state.
	for (size_t index = 0; index < _multilayer_m1_sprite_ranges.size(); ++index) {
		const auto asset = static_cast<MultiLayerSpriteAsset>(index);
		const auto &range = _multilayer_m1_sprite_ranges[index];
		for (uint8_t state = 0; state < range.states; ++state) {
			for (uint8_t frame = 0; frame < range.frames; ++frame) {
				for (uint8_t view = 0; view < range.directions; ++view) {
					for (uint16_t tile = 0; tile < range.tiles; ++tile) {
						for (uint8_t part = 0; part < range.parts; ++part) {
							const auto sprite = GetMultiLayerSprite(asset, {state, frame, view, tile, part});
							REQUIRE(sprite.has_value());
							CHECK(selected.insert(*sprite).second);
							CHECK(*sprite >= SPR_MAP_LAYER_BASE + 74);
							CHECK(*sprite < SPR_NEWGRFS_BASE);
						}
					}
				}
			}
		}
		CHECK_FALSE(GetMultiLayerSprite(asset, {range.states, 0, 0, 0, 0}).has_value());
		CHECK_FALSE(GetMultiLayerSprite(asset, {0, range.frames, 0, 0, 0}).has_value());
		CHECK_FALSE(GetMultiLayerSprite(asset, {0, 0, range.directions, 0, 0}).has_value());
		CHECK_FALSE(GetMultiLayerSprite(asset, {0, 0, 0, range.tiles, 0}).has_value());
		CHECK_FALSE(GetMultiLayerSprite(asset, {0, 0, 0, 0, range.parts}).has_value());
	}
	CHECK(selected.size() == 2311);
	CHECK_FALSE(GetMultiLayerSprite(MultiLayerSpriteAsset::End, {}).has_value());
	CHECK(GetMultiLayerSprite(MultiLayerSpriteAsset::VehicleModel00, {4, 3, 6, 0, 1}) == SPR_MAP_LAYER_BASE + 517);
	CHECK(GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityPortalSealed, {2, 0, 3, 1, 1}) == SPR_MAP_LAYER_BASE + 1265);
	CHECK(GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityRockReceiver, {3, 3, 0, 15, 1}) == SPR_MAP_LAYER_BASE + 2401);
}

/** Actual feature owners stay byte-equivalent while cameras/clocks choose source-authored phases. */
TEST_CASE("M1 visual clocks preserve native project receiver money and RNG owners", "[m1-sprite-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world;
	const auto projects = CaptureTBMProjects();
	const auto receivers = CaptureSpoilReceivers();
	const Money money = Company::Get(_current_company)->money;
	SavedRandomSeeds before;
	SaveRandomSeeds(&before);
	// Deliberately choose widely separated render clocks and both movement views.
	// Presentation must leave the live terrain/project/cargo/processing owners alone.
	for (uint64_t clock : {0ULL, 8ULL, 17ULL, 1023ULL, 1ULL << 40}) {
		for (auto asset : {MultiLayerSpriteAsset::VehicleModel02, MultiLayerSpriteAsset::VehicleModel03}) {
			CHECK(GetMultiLayerVisualFrame(asset, clock, false) == 0);
			CHECK(GetMultiLayerVisualFrame(asset, 8, true) == 1);
			CHECK(GetMultiLayerSprite(asset, {2, GetMultiLayerVisualFrame(asset, clock, false), 6, 0, 0}).has_value());
		}
		CHECK(GetMultiLayerVisualFrame(MultiLayerSpriteAsset::TerrainUnderground, clock, true) == 0);
		CHECK(GetMultiLayerVisualFrame(MultiLayerSpriteAsset::FacilityPortalSealed, clock, true) == 0);
		CHECK(GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityEarthReceiver, {2, GetMultiLayerVisualFrame(MultiLayerSpriteAsset::FacilityEarthReceiver, clock, false), 0, 5, 1}).has_value());
		CHECK(GetMultiLayerTerrainTexture(7, 11) == 1);
	}
	SavedRandomSeeds after;
	SaveRandomSeeds(&after);
	CHECK(CaptureTBMProjects() == projects);
	CHECK(CaptureSpoilReceivers() == receivers);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(std::ranges::equal(before.random.state, after.random.state));
	CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
}
