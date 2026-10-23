/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_control_selection.cpp Real UI calendar/cursor, cargo ownership and geological marker boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group83B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_sprite.h"
#include "../underground_gui.h"
#include "../underground_vehicle.h"
#include "../newgrf.h"
#include "../widgets/underground_widget.h"
#include "excavation_fixture.h"
#include "tbm_fixture.h"
#include "mock_environment.h"
#include "layer_window_fixture.h"

#include "../safeguards.h"

/** Real widget state, date and cursor ownership govern pictures while reserves/cash/RNG remain unchanged. */
TEST_CASE("Native control art follows real calendar and cursor owners", "[m1-control-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture world;
	LayerWindowTestUI ui(WindowClass::UndergroundConstruction, _current_company);
	const auto policy = GetCurrentMultiLayerProfileSelection();
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const Money money = Company::Get(_current_company)->money;
	SavedRandomSeeds random_before;
	SaveRandomSeeds(&random_before);
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::DeepUnderground, policy);
	REQUIRE(window != nullptr);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	window->OnPaint();
	CHECK(window->IsWidgetDisabled(WID_UC_SURVEY));
	CHECK(GetUndergroundConstructionWidgetSprite(*window, WID_UC_SURVEY) == GetMultiLayerControlSprite(MultiLayerSpriteAsset::ControlSurvey, true, false));
	CHECK(GetUndergroundConstructionWidgetSprite(*window, WID_UC_REQUIREMENTS) == GetMultiLayerControlSprite(MultiLayerSpriteAsset::ControlDeepLock, true, false));
	CHECK(GetUndergroundConstructionWidgetSprite(*window, WID_UC_MODEL_DATES) == GetMultiLayerControlSprite(MultiLayerSpriteAsset::ControlFutureLock, true, false));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	window->OnPaint();
	CHECK_FALSE(window->IsWidgetDisabled(WID_UC_SURVEY));
	CHECK(GetUndergroundConstructionWidgetSprite(*window, WID_UC_REQUIREMENTS) == GetMultiLayerControlSprite(MultiLayerSpriteAsset::ControlDeepLock, false, false));
	CHECK(GetUndergroundConstructionWidgetSprite(*window, WID_UC_MODEL_DATES) == GetMultiLayerControlSprite(MultiLayerSpriteAsset::ControlFutureLock, false, false));
	CHECK(GetUndergroundConstructionWidgetSprite(*window, WID_UC_AREA_FLOOR) == GetMultiLayerControlSprite(MultiLayerSpriteAsset::ControlTbmTurn45, true, false));
	const std::array buttons{WID_UC_PROJECT, WID_UC_AREA, WID_UC_EXCHANGE};
	const std::array assets{MultiLayerSpriteAsset::ControlExcavation, MultiLayerSpriteAsset::ControlTbmArea, MultiLayerSpriteAsset::ControlExchange};
	// Three independent actual buttons use their own real state rather than
	// reusing the survey's availability or a generic decorative lock picture.
	for (size_t i = 0; i < buttons.size(); ++i) CHECK(GetUndergroundConstructionWidgetSprite(*window, buttons[i]) == GetMultiLayerControlSprite(assets[i], false, false));
	window->OnClick({}, WID_UC_SURVEY, 1);
	window->OnPaint();
	CHECK(GetUndergroundConstructionWidgetSprite(*window, WID_UC_SURVEY) == GetMultiLayerControlSprite(MultiLayerSpriteAsset::ControlSurvey, false, true));
	// Select a real Deep seam and prove every viewport query follows the actual
	// active cursor, rather than a camera or a second survey information store.
	const auto records = GetGeologyWorldState().deposits.Records();
	const auto seam = std::ranges::find(records, MapLayer::DeepUnderground, &GeologyDeposit::layer);
	REQUIRE(seam != records.end());
	const TileIndex tile = TileXYL(seam->spans.front().first_x, seam->spans.front().y, seam->layer);
	CHECK(GetUndergroundSurveyOverlaySprite(tile) == GetMultiLayerSurveySprite(tile, true));
	CHECK_FALSE(GetUndergroundSurveyOverlaySprite(MultiLayerMap::TileInLayer(tile, MapLayer::Underground)).has_value());
	window->OnPlaceObject({}, tile);
	window->OnPlaceObjectAbort();
	CHECK_FALSE(GetUndergroundSurveyOverlaySprite(tile).has_value());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(Company::Get(_current_company)->money == money);
	SavedRandomSeeds random_after;
	SaveRandomSeeds(&random_after);
	CHECK(std::ranges::equal(random_before.random.state, random_after.random.state));
	CHECK(std::ranges::equal(random_before.interactive_random.state, random_after.interactive_random.state));
	CHECK_FALSE(GetMultiLayerControlSprite(MultiLayerSpriteAsset::VehicleModel00, false, false).has_value());
}

/** Ten actual resource meanings and finite reserves choose their exact public marker families without writing survey flags. */
TEST_CASE("Native survey markers read all seam identities and depletion", "[m1-control-selection]")
{
	ExcavationFixture world;
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	std::set<SpriteID> families;
	// Each guaranteed native seam is queried through its world position; this
	// also tests the lookup and avoids substituting a detached deposit record.
	for (const auto &deposit : GetGeologyWorldState().deposits.Records()) {
		const TileIndex tile = TileXYL(deposit.spans.front().first_x, deposit.spans.front().y, deposit.layer);
		const auto known = GetMultiLayerSurveySprite(tile, true);
		const auto unknown = GetMultiLayerSurveySprite(tile, false);
		REQUIRE(known.has_value());
		REQUIRE(unknown.has_value());
		CHECK(*known == *unknown + 1);
		families.insert(*unknown);
	}
	CHECK(families.size() == 10);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	// Explicit generation/load fixture staging changes only the true reserve
	// authority. Subsequent rendering must show depletion without extracting it.
	auto snapshot = CaptureGeologyState();
	for (auto &deposit : snapshot.deposits) deposit.remaining_reserve = 0;
	GeologyWorldState candidate;
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	StageLoadedGeologyState(std::move(candidate));
	AcceptLoadedGeologyState();
	for (const auto &deposit : GetGeologyWorldState().deposits.Records()) {
		const TileIndex tile = TileXYL(deposit.spans.front().first_x, deposit.spans.front().y, deposit.layer);
		CHECK(*GetMultiLayerSurveySprite(tile, true) == *GetMultiLayerSurveySprite(tile, false) + 2);
		CHECK(deposit.surveyed_companies == 0);
		CHECK(deposit.remaining_reserve == 0);
	}
	CHECK_FALSE(GetMultiLayerSurveySprite(INVALID_TILE, true).has_value());
}

/** Labels retain their actual package ownership; the native icon cannot replace foreign definitions. */
TEST_CASE("Native spoil icons preserve cargo definition ownership", "[m1-control-selection]")
{
	ExcavationFixture world;
	GRFFile native;
	native.grfid = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package.grfid;
	GRFFile foreign;
	foreign.grfid = GrfID{"FORE"};
	// Local definition copies exercise each actual icon resolver without changing
	// the accepted global cargo registry or the immutable distributed package.
	for (const auto label : {CargoLabel{"ERTH"}, CargoLabel{"ROCK"}}) {
		CargoSpec cargo = *CargoSpec::Get(GetCargoTypeByLabel(label));
		const auto asset = label == CargoLabel{"ERTH"} ? MultiLayerSpriteAsset::CargoERTH : MultiLayerSpriteAsset::CargoROCK;
		cargo.grffile = &native;
		CHECK(cargo.GetCargoIcon() == *GetMultiLayerSprite(asset, {}));
		cargo.grffile = &foreign;
		cargo.sprite = SPR_CARGO_GOODS;
		CHECK_FALSE(GetMultiLayerCargoSprite(cargo).has_value());
		CHECK(cargo.GetCargoIcon() == SPR_CARGO_GOODS);
	}
}

/** Persisted engine mappings choose purchase art; state selection confers no build/commissioning authority. */
TEST_CASE("Native purchase previews resolve actual engines and both availability states", "[m1-control-selection]")
{
	TBMEngineFixture fixture;
	REQUIRE(InitialiseUndergroundEngineDefinitions(fixture.package));
	// Both real cutter definitions, both hoppers and the conventional hauler
	// share the published five-family UI slice, with separate locked poses.
	for (uint16_t model = 0; model < 5; ++model) {
		const EngineID engine = model < 4 ? GetTBMEngine(model) : GetUndergroundEngine(model);
		REQUIRE(Engine::IsValidID(engine));
		CHECK(GetMultiLayerEnginePurchaseSprite(engine, true) == GetMultiLayerPurchaseSprite(model, true));
		CHECK(GetMultiLayerEnginePurchaseSprite(engine, false) == GetMultiLayerPurchaseSprite(model, false));
		CHECK(*GetMultiLayerEnginePurchaseSprite(engine, false) == *GetMultiLayerEnginePurchaseSprite(engine, true) + 1);
	}
	CHECK_FALSE(GetMultiLayerEnginePurchaseSprite(fixture.stock->index, true).has_value());
	CHECK_FALSE(GetMultiLayerPurchaseSprite(5, true).has_value());
}
