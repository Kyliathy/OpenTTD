/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file selected_control_icons.cpp Real tube/geothermal widget states borrow native dates, player policy and fold-out owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BD.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * Native content and the prepared depot are explicit isolated boundary inputs; ordinary paid-world raster proof remains separate.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "underground_view_fixture.h"
#include "transport_tube_fixture.h"
#include "../underground_gui.h"
#include "../multilayer_sprite.h"
#include "../geothermal_collector.h"
#include "../geothermal_generator.h"
#include "../geothermal_riser.h"
#include "../widgets/underground_widget.h"
#include "../safeguards.h"

/** @return Exact native simulation owners, all company cash, clock and both RNG streams surrounding a pure control repaint. */
static std::string CaptureSelectedControlOwners()
{
	std::string result = fmt::format("{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}", ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeUtilityPowerDigest(),
		ComputeTubePowerDigest(), ComputeGeothermalSiteDigest(), ComputeGeothermalCollectorDigest(), ComputeGeothermalGeneratorDigest(), ComputeGeothermalRiserDigest(),
		TimerGameTick::counter, _random.state[0], _random.state[1], _interactive_random.state[0]);
	result += fmt::format(":{}", _interactive_random.state[1]);
	for (const auto *company : Company::Iterate()) result += fmt::format("/{}:{}", company->index.base(), company->money.base()); // Every wallet remains original even when the viewing player temporarily becomes a spectator.
	return result;
}

/** Exact delivered controls retain disabled priority and refuse unrelated/future/Classic presentation without granting command permission. */
TEST_CASE("Selected native control families preserve explicit status and profile boundaries", "[selected-control-icons-selection]")
{
	ExcavationFixture world; const auto before = CaptureSelectedControlOwners();
	for (const auto asset : {MultiLayerSpriteAsset::ControlTube, MultiLayerSpriteAsset::ControlGeothermal}) {
		for (const bool disabled : {false, true}) {
			for (const bool pressed : {false, true}) {
				// Existing native widget availability has priority over a retained
				// lowered flag; presentation cannot make a locked action look ready.
				const uint8_t state = disabled ? 1 : pressed ? 2 : 0;
				CHECK(GetMultiLayerControlSprite(asset, disabled, pressed) == GetMultiLayerSprite(asset, {state}));
			}
		}
	}
	for (const auto asset : {MultiLayerSpriteAsset::VehicleModel00, MultiLayerSpriteAsset::FacilityUtilityRiser, MultiLayerSpriteAsset::GeothermalConduit, MultiLayerSpriteAsset::End}) CHECK_FALSE(GetMultiLayerControlSprite(asset, false, false).has_value());
	CHECK(CaptureSelectedControlOwners() == before);
	ResetMultiLayerProfileSelection();
	for (const auto asset : {MultiLayerSpriteAsset::ControlSurvey, MultiLayerSpriteAsset::ControlTube, MultiLayerSpriteAsset::ControlGeothermal}) CHECK_FALSE(GetMultiLayerControlSprite(asset, false, true).has_value()); // Classic keeps the existing fallback rather than acquiring a selected-family binding.
}

/** Actual menus remain inspectable while native calendar/company policy disables physical actions; expansion and repaint consume no simulation resource. */
TEST_CASE("Selected native control icons follow real foldouts dates and spectators", "[selected-control-icons-window]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubePrototypeRig rig; UndergroundToolbarTestUI ui{_current_company};
	const auto before = CaptureSelectedControlOwners();
	const std::array<WidgetID, 2> menus{WID_UC_TUBES, WID_UC_GEOTHERMAL}, actions{WID_UC_TUBE_GUIDEWAY, WID_UC_GEO_PLAN};
	const std::array assets{MultiLayerSpriteAsset::ControlTube, MultiLayerSpriteAsset::ControlGeothermal};
	// Both actual rock planes use the same source family, but their native
	// Deep availability and geothermal admission remain independent owners.
	for (const auto layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		Window *window = ShowUndergroundConstructionToolbar(layer, GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr);
		AutoRestoreBackup date{TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2000}, 0, 1)};
		window->OnPaint(); const int closed_height = window->height;
		for (size_t index = 0; index < menus.size(); ++index) {
			CHECK_FALSE(window->IsWidgetDisabled(menus[index])); CHECK_FALSE(window->IsWidgetLowered(menus[index]));
			CHECK(GetUndergroundConstructionWidgetSprite(*window, menus[index]) == GetMultiLayerSprite(assets[index], {0}));
			CHECK(window->GetWidget<NWidgetBase>(menus[index])->current_y >= static_cast<uint>(ScaleGUITrad(20)));
			window->OnClick({}, menus[index], 1); window->OnPaint(); CHECK(window->IsWidgetLowered(menus[index]));
			CHECK(GetUndergroundConstructionWidgetSprite(*window, menus[index]) == GetMultiLayerSprite(assets[index], {2}));
		}
		CHECK(window->height > closed_height); CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_GUIDEWAY));
		CHECK(window->IsWidgetDisabled(WID_UC_GEO_PLAN) == (layer != MapLayer::DeepUnderground));
		{
			AutoRestoreBackup spectator{_local_company, COMPANY_SPECTATOR}; window->OnPaint();
			for (size_t index = 0; index < actions.size(); ++index) {
				CHECK(window->IsWidgetDisabled(actions[index])); CHECK(GetUndergroundConstructionWidgetSprite(*window, actions[index]) == GetMultiLayerSprite(assets[index], {1}));
				CHECK_FALSE(window->IsWidgetDisabled(menus[index])); // Information remains inspectable; a disabled construction icon does not close a real fold-out or grant ownership.
			}
		}
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1969}, 11, 31); window->OnPaint();
		for (size_t index = 0; index < actions.size(); ++index) {
			CHECK(window->IsWidgetDisabled(actions[index])); CHECK(GetUndergroundConstructionWidgetSprite(*window, actions[index]) == GetMultiLayerSprite(assets[index], {1}));
		}
		window->Close(); Window::DeleteClosedWindows();
	}
	CHECK(CaptureSelectedControlOwners() == before);
}
