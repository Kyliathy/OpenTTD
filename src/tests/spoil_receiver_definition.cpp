/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file spoil_receiver_definition.cpp The two native receiver definitions (Plan 5 Group 8.2 and 8.4): accept-only
 * Temperate industries in the first-party package's scope that are never generated, every tile drawing an original
 * substitute and accepting its spoil, stable saved identities across a reload, and Surface-only placement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 8
 * @see docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-i-receiver-design.md, What a receiver is
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"
#include "../industry_cmd.h"
#include "../map_layer.h"

#include "../safeguards.h"

/** Each receiver is a native accept-only industry for exactly its own spoil, drawn with original art. */
TEST_CASE("Both receivers install as native accept-only Temperate industries", "[spoil-receiver-definition]")
{
	SpoilReceiverRig rig;
	for (SpoilReceiverKind kind : {SpoilReceiverKind::Earth, SpoilReceiverKind::Rock}) {
		const IndustryType type = GetSpoilReceiverIndustryType(kind);
		REQUIRE(type != IT_INVALID);
		CHECK(GetSpoilReceiverKind(type) == kind);
		const IndustrySpec *spec = GetIndustrySpec(type);
		CHECK(spec->enabled);
		CHECK_FALSE(spec->grf_prop.HasGrfFile()); // Native, not read from a file.
		CHECK_FALSE(spec->IsRawIndustry()); // A company may fund it, as any accepting industry.
		CHECK(spec->life_type == INDUSTRYLIFE_BLACK_HOLE);
		CHECK(spec->climate_availability == LandscapeTypes{LandscapeType::Temperate});
		// Never generated with the map or at random.
		CHECK(std::ranges::all_of(spec->appear_creation, [](uint8_t chance) { return chance == 0; }));
		CHECK(std::ranges::all_of(spec->appear_ingame, [](uint8_t chance) { return chance == 0; }));
		const CargoType cargo = GetCargoTypeByLabel(GetSpoilReceiverRecipe(kind).cargo);
		REQUIRE(IsValidCargoType(cargo));
		CHECK(spec->accepts_cargo[0] == cargo);
		CHECK(std::ranges::count_if(spec->accepts_cargo, IsValidCargoType) == 1);
		CHECK(std::ranges::none_of(spec->produced_cargo, IsValidCargoType));
		REQUIRE(spec->layouts.size() == 1);
		// Every tile is native, draws an original tile and accepts the spoil fully, without animation.
		for (const auto &cell : spec->layouts.front()) {
			REQUIRE(cell.gfx >= NEW_INDUSTRYTILEOFFSET);
			const IndustryTileSpec *tile = GetIndustryTileSpec(cell.gfx);
			CHECK(tile->enabled);
			CHECK(tile->grf_prop.subst_id < NEW_INDUSTRYTILEOFFSET);
			CHECK(tile->accepts_cargo[0] == cargo);
			CHECK(tile->acceptance[0] == 8);
			CHECK(tile->anim_production == INDUSTRYTILE_NOANIM);
		}
	}
	CHECK(GetSpoilReceiverIndustryType(SpoilReceiverKind::Earth) != GetSpoilReceiverIndustryType(SpoilReceiverKind::Rock));
	CHECK(GetSpoilReceiverKind(0) == SpoilReceiverKind::End); // The original coal mine is no receiver.
	CHECK(GetSpoilReceiverKind(IT_INVALID) == SpoilReceiverKind::End);
	CHECK(GetSpoilReceiverKindForCargo(GetCargoTypeByLabel(CargoLabel{"ERTH"})) == SpoilReceiverKind::Earth);
	CHECK(GetSpoilReceiverKindForCargo(GetCargoTypeByLabel(CargoLabel{"ROCK"})) == SpoilReceiverKind::Rock);
	CHECK(GetSpoilReceiverKindForCargo(GetCargoTypeByLabel(CT_PASSENGERS)) == SpoilReceiverKind::End);
	CHECK(GetSpoilReceiverKindForCargo(INVALID_CARGO) == SpoilReceiverKind::End);
}

/** A GRF reload reinstalls both definitions in their saved slots; without the exact package nothing is installed. */
TEST_CASE("Receiver definitions keep their identities and need the exact package", "[spoil-receiver-definition]")
{
	SpoilReceiverRig rig;
	const IndustryType earth = GetSpoilReceiverIndustryType(SpoilReceiverKind::Earth);
	const IndustryType rock = GetSpoilReceiverIndustryType(SpoilReceiverKind::Rock);
	const auto tiles = GetIndustrySpec(rock)->layouts.front();

	ResetIndustries(); // As every GRF reload does before the definitions are installed again.
	CHECK(GetSpoilReceiverIndustryType(SpoilReceiverKind::Earth) == IT_INVALID);
	REQUIRE(InitialiseSpoilReceiverDefinitions(rig.fixture.package));
	CHECK(GetSpoilReceiverIndustryType(SpoilReceiverKind::Earth) == earth);
	CHECK(GetSpoilReceiverIndustryType(SpoilReceiverKind::Rock) == rock);
	const auto &again = GetIndustrySpec(rock)->layouts.front();
	REQUIRE(again.size() == tiles.size());
	for (size_t i = 0; i < tiles.size(); ++i) CHECK(again[i].gfx == tiles[i].gfx);

	ResetIndustries();
	CHECK_FALSE(InitialiseSpoilReceiverDefinitions(GRFConfigList{}));
	CHECK(GetSpoilReceiverIndustryType(SpoilReceiverKind::Earth) == IT_INVALID);
	CHECK(GetSpoilReceiverIndustryType(SpoilReceiverKind::Rock) == IT_INVALID);
}

/** No receiver stands below ground: the loaded leg always ends on the Surface. */
TEST_CASE("Receivers are built on the Surface only", "[spoil-receiver-definition]")
{
	SpoilReceiverRig rig;
	const IndustryType earth = GetSpoilReceiverIndustryType(SpoilReceiverKind::Earth);
	for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		CHECK(CmdBuildIndustry(DoCommandFlag::Execute, TileXYL(30, 30, layer), earth, 0, true, 0).Failed());
	}
	CHECK(Industry::GetNumItems() == 0);
}
