/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_facility_selection.cpp Native receiver world-cell, stock and road-service presentation boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group83B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_sprite.h"
#include "spoil_receiver_fixture.h"
#include "../road_cmd.h"
#include "../road_map.h"

#include "../safeguards.h"

/** Each real funded-layout cell maps to one authored tile, while all packet/processing balances remain with the industry. */
TEST_CASE("Native receiver art uses fixed cells and actual bounded stock", "[m1-facility-selection]")
{
	SpoilReceiverRig rig;
	for (const auto kind : {SpoilReceiverKind::Earth, SpoilReceiverKind::Rock}) {
		Industry &industry = *rig.BuildReceiver(kind, kind == SpoilReceiverKind::Earth ? 15 : 25, 22);
		const auto asset = kind == SpoilReceiverKind::Earth ? MultiLayerSpriteAsset::FacilityEarthReceiver : MultiLayerSpriteAsset::FacilityRockReceiver;
		// The native layouts are fixed, not four fictional build rotations. Every
		// real cell selects its own aligned back/front and never changes footprint.
		for (uint y = 0; y < industry.location.h; ++y) {
			for (uint x = 0; x < industry.location.w; ++x) {
				const TileIndex cell = TileAddXY(industry.location.tile, x, y);
				const uint16_t variant = static_cast<uint16_t>(y * industry.location.w + x);
				// Explicit industry-owner fixture staging completes this constructed
				// cell; the graphics query itself can never finish construction.
				CHECK(GetMultiLayerReceiverSprite(cell, false, 8) == GetMultiLayerSprite(asset, {0, 1, 0, variant, 0}));
				SetIndustryCompleted(cell);
				CHECK(GetMultiLayerReceiverSprite(cell, false, 8) == GetMultiLayerSprite(asset, {1, 1, 0, variant, 0}));
				CHECK(GetMultiLayerReceiverSprite(cell, true, 8) == GetMultiLayerSprite(asset, {1, 1, 0, variant, 1}));
			}
		}
		const auto &recipe = GetSpoilReceiverRecipe(kind);
		const CargoType cargo = GetCargoTypeByLabel(recipe.cargo);
		const uint admitted = AdmitSpoilToReceiver(industry, cargo, recipe.batch_t, _current_company);
		REQUIRE(admitted == recipe.batch_t);
		SpoilReceiverRig::Stock(industry) += admitted; // Existing delivery writes actual accepted stock after its admission receipt.
		const auto before = *GetSpoilReceiver(industry.index);
		CHECK(GetMultiLayerReceiverSprite(industry.location.tile, false, 16) == GetMultiLayerSprite(asset, {2, 2, 0, 0, 0}));
		CHECK(*GetSpoilReceiver(industry.index) == before);
		CHECK(SpoilReceiverRig::Stock(industry) == admitted);
		// Producer fixture staging fills the real industry's stock. A render call
		// must show the owner's saturation without processing or discarding it.
		SpoilReceiverRig::Stock(industry) = recipe.stock_cap_t;
		CHECK(GetMultiLayerReceiverSprite(industry.location.tile, true, 0) == GetMultiLayerSprite(asset, {3, 0, 0, 0, 1}));
		CHECK(SpoilReceiverRig::Stock(industry) == recipe.stock_cap_t);
	}
}

/** Actual road depot commands establish the service building; its appearance grants no new vehicle or layer capability. */
TEST_CASE("Native road-service art applies only to actual rock-layer depots", "[m1-facility-selection]")
{
	TBMEngineFixture world;
	ResetRoadTypes(); // This fixture normally owns rail definitions; initialize the actual conventional road registry too.
	const TileIndex tile = TileXYL(42, 42, MapLayer::Underground);
	const std::array floor{tile, TileAddByDiagDir(tile, DiagDirection::SE)};
	world.world.Open(floor);
	REQUIRE(Command<Commands::BuildRoadDepot>::Do(DoCommandFlag::Execute, tile, ROADTYPE_ROAD, DiagDirection::SE).Succeeded());
	CHECK(GetMultiLayerRoadDepotSprite(tile, false, 8) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityRoadService, {1, 1, 1, 0, 0}));
	CHECK(GetMultiLayerRoadDepotSprite(tile, true, 8) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityRoadService, {1, 1, 1, 0, 1}));
	CHECK_FALSE(GetMultiLayerRoadDepotSprite(world.request.portal, false, 0).has_value());
	CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
}
