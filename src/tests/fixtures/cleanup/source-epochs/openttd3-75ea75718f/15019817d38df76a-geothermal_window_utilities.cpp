/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_window_utilities.cpp Actual paid single-cell and original-portal riser window actions.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15CC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md Group4C; actual mouse utilities/riser confirmation.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * Prepared excavated floor and commissioned portal are explicit native boundary inputs, separate from paid-world boring/supply acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "underground_view_fixture.h"
#include "tube_input_view_fixture.h"
#include "geothermal_riser_fixture.h"
#include "../geothermal_utility_cmd.h"
#include "../geothermal_utility.h"
#include "../underground_gui.h"
#include "../toolbar_gui.h"
#include "../tilehighlight_func.h"
#include "../widgets/underground_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** @param layer Actual rock plane. @return Existing window with its physical geothermal page visible; no native quantity is created. */
static Window *OpenGeothermalPhysicalWindow(MapLayer layer)
{
	Window *window = ShowUndergroundConstructionToolbar(layer, GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr);
	window->OnPaint(); window->OnClick({}, WID_UC_GEOTHERMAL, 1); window->OnDropdownSelect(WID_UC_GEO_PAGE, 1, 0); window->OnPaint(); return window;
}

/** Actual main/extra viewport clicks buy/remove one domain cell, confirm original-portal riser funding and cancel only transient geometry on a layer switch. */
TEST_CASE("Geothermal physical controls use actual main and extra viewport mouse input", "[geothermal-provider-utility-input]")
{
	const auto [layer, domain] = GENERATE(std::tuple{MapLayer::DeepUnderground, UtilityEnergyDomain::Heat},
		std::tuple{MapLayer::DeepUnderground, UtilityEnergyDomain::Electricity}, std::tuple{MapLayer::Underground, UtilityEnergyDomain::Electricity});
	const bool use_extra = GENERATE(false, true);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeRiserRig rig;
	AutoRestoreBackup pause{_settings_game.construction.command_pause_level, CommandPauseLevel::AllActions}; UndergroundToolbarTestUI ui{_current_company};
	auto *main = new UndergroundTestViewport(); SetViewportLayer(main, use_extra ? MapLayer::Surface : layer);
	Window *extra = CreateTubeInputExtraViewport(use_extra ? layer : MapLayer::Surface);
	Window *active = use_extra ? extra : main; Window *other = use_extra ? main : extra; AllocateToolbar();
	Window *window = ShowUndergroundConstructionToolbar(layer, GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr); window->OnPaint();
	FeedbackClick(*window, WID_UC_GEOTHERMAL); window->OnDropdownSelect(WID_UC_GEO_PAGE, 1, 0); window->OnPaint();
	const auto tile = TileXYL(23, 23, layer); const auto cash = Company::Get(_current_company)->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()); const auto random = _random;
	const auto quote = CmdBuildGeothermalUtilityLink({}, tile, domain); REQUIRE(quote.Succeeded());
	FeedbackClick(*window, domain == UtilityEnergyDomain::Heat ? WID_UC_GEO_HEAT : WID_UC_GEO_ELECTRIC); FeedbackPumpWindows(); REQUIRE(_thd.GetCallbackWnd() == window);
	MouseClickTubeWorldTile(*other, MultiLayerMap::ProjectToSurface(tile)); CHECK_FALSE(GetGeothermalUtilityLink(tile, domain).has_value()); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find(GetString(STR_UNDERGROUND_WRONG_LAYER)) != std::string::npos);
	MouseClickTubeWorldTile(*active, tile); REQUIRE(GetGeothermalUtilityLink(tile, domain).has_value()); CHECK(Company::Get(_current_company)->money == cash - quote.GetCost());
	const auto once = Company::Get(_current_company)->money; MouseClickTubeWorldTile(*active, tile); CHECK(Company::Get(_current_company)->money == once);
	const auto removal = CmdRemoveGeothermalUtilityLink({}, tile, domain); REQUIRE(removal.Succeeded());
	FeedbackClick(*window, domain == UtilityEnergyDomain::Heat ? WID_UC_GEO_REMOVE_HEAT : WID_UC_GEO_REMOVE_ELECTRIC); FeedbackPumpWindows(); MouseClickTubeWorldTile(*active, tile);
	CHECK_FALSE(GetGeothermalUtilityLink(tile, domain).has_value()); CHECK(Company::Get(_current_company)->money == once - removal.GetCost());
	CHECK(std::ranges::equal(random.state, _random.state));
	// The selected Deep electrical route also exposes its original sending
	// riser. Both cameras must collect all three real inputs before one paid
	// native confirmation; local Heat and Underground relay cells cannot do so.
	if (layer == MapLayer::DeepUnderground && domain == UtilityEnergyDomain::Electricity) {
		const auto capital = Company::Get(_current_company)->money;
		FeedbackClick(*window, WID_UC_GEO_RISER); FeedbackPumpWindows(); REQUIRE(_thd.GetCallbackWnd() == window);
		MouseClickTubeWorldTile(*active, rig.request.portal); MouseClickTubeWorldTile(*active, rig.request.tile);
		window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM));
		MouseClickTubeWorldTile(*active, rig.request.port); window->OnPaint(); REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM));
		CHECK(Company::Get(_current_company)->money == capital); CHECK(Industry::GetNumItems() == 0);
		FeedbackClick(*window, WID_UC_GEO_RISER_CONFIRM); FeedbackPumpWindows();
		const auto id = FindGeothermalRiserForPortal(rig.request.portal); REQUIRE(Industry::IsValidID(id));
		const auto *state = GetGeothermalRiserState(id); REQUIRE(state != nullptr); CHECK(state->site == rig.request); CHECK(state->work_ticks == 0); CHECK_FALSE(state->materials_committed);
		CHECK(Company::Get(_current_company)->money == capital - Money{GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->capital_cost});
		// Native funding supplies neither cargo nor work; transported material
		// delivery and completed thermal/electrical service retain their owners.
		for (const auto &slot : Industry::Get(id)->accepted) CHECK(slot.waiting == 0);
	}
	CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(CaptureUtilityPowerState().consumers.empty());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	// Native riser funding initializes a real Industry colour/counter using
	// simulation RNG; freeze its resulting state before testing pure UI cancel.
	const auto funded_random = _random;
	const auto installed = std::tuple{ComputeGeothermalRiserDigest(), ComputeUtilityPowerDigest(), Company::Get(_current_company)->money};
	FeedbackClick(*window, WID_UC_GEO_ELECTRIC); FeedbackPumpWindows(); SetViewportLayer(active, layer == MapLayer::Underground ? MapLayer::DeepUnderground : MapLayer::Underground);
	CHECK(_thd.GetCallbackWnd() != window); CHECK((std::tuple{ComputeGeothermalRiserDigest(), ComputeUtilityPowerDigest(), Company::Get(_current_company)->money} == installed));
	CHECK(std::ranges::equal(funded_random.state, _random.state));
}

/** Each explicitly selected domain cell pays its native invoice once and removal preserves the independent network; quotes/painting cannot create energy or geology. */
TEST_CASE("Geothermal window installs and removes actual paid domain cells", "[geothermal-window-utilities]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeRiserRig rig;
	AutoRestoreBackup pause{_settings_game.construction.command_pause_level, CommandPauseLevel::AllActions};
	UndergroundToolbarTestUI ui{_current_company}; const auto layer = GENERATE(MapLayer::DeepUnderground, MapLayer::Underground);
	Window *window = OpenGeothermalPhysicalWindow(layer); const auto tile = TileXYL(23, 23, layer);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), power = ComputeUtilityPowerDigest();
	const auto random = _random; const auto initial_cash = Company::Get(_current_company)->money;
	const auto electric = CmdBuildGeothermalUtilityLink({}, tile, UtilityEnergyDomain::Electricity); REQUIRE(electric.Succeeded());
	window->OnPaint(); window->OnClick({}, WID_UC_GEO_ELECTRIC, 1); window->OnPaint();
	CHECK(Company::Get(_current_company)->money == initial_cash); CHECK(ComputeUtilityPowerDigest() == power);
	window->OnPlaceObject({}, MultiLayerMap::TileInLayer(tile, layer == MapLayer::Underground ? MapLayer::DeepUnderground : MapLayer::Underground));
	CHECK_FALSE(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity).has_value());
	window->OnPlaceObject({}, tile); REQUIRE(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity).has_value());
	CHECK(Company::Get(_current_company)->money == initial_cash - electric.GetCost());
	const auto once = Company::Get(_current_company)->money; window->OnPlaceObject({}, tile); CHECK(Company::Get(_current_company)->money == once);
	if (layer == MapLayer::DeepUnderground) {
		const auto heat = CmdBuildGeothermalUtilityLink({}, tile, UtilityEnergyDomain::Heat); REQUIRE(heat.Succeeded());
		window->OnClick({}, WID_UC_GEO_HEAT, 1); window->OnPlaceObject({}, tile);
		REQUIRE(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Heat).has_value()); CHECK(Company::Get(_current_company)->money == once - heat.GetCost());
		const auto removal = CmdRemoveGeothermalUtilityLink({}, tile, UtilityEnergyDomain::Heat); REQUIRE(removal.Succeeded());
		const auto before = Company::Get(_current_company)->money;
		window->OnClick({}, WID_UC_GEO_REMOVE_HEAT, 1); window->OnPlaceObject({}, tile);
		CHECK_FALSE(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Heat).has_value()); CHECK(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity).has_value());
		CHECK(Company::Get(_current_company)->money == before - removal.GetCost());
	} else { CHECK(window->IsWidgetDisabled(WID_UC_GEO_HEAT)); CHECK(window->IsWidgetDisabled(WID_UC_GEO_REMOVE_HEAT)); }
	const auto removal = CmdRemoveGeothermalUtilityLink({}, tile, UtilityEnergyDomain::Electricity); REQUIRE(removal.Succeeded());
	const auto before = Company::Get(_current_company)->money; window->OnClick({}, WID_UC_GEO_REMOVE_ELECTRIC, 1); window->OnPlaceObject({}, tile);
	CHECK_FALSE(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity).has_value()); CHECK(Company::Get(_current_company)->money == before - removal.GetCost());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(CaptureUtilityPowerState().consumers.empty()); CHECK(std::ranges::equal(random.state, _random.state));
}

/** The real window requires all three original sending/anchor/boundary inputs and only native funding creates empty stock/zero work. */
TEST_CASE("Geothermal window funds original commissioned portal riser machinery", "[geothermal-window-riser]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); const auto direct = GENERATE(false, true); NativeRiserRig rig{direct};
	AutoRestoreBackup pause{_settings_game.construction.command_pause_level, CommandPauseLevel::AllActions}; UndergroundToolbarTestUI ui{_current_company};
	Window *window = OpenGeothermalPhysicalWindow(MapLayer::DeepUnderground);
	const auto cash = Company::Get(_current_company)->money; const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto power = ComputeUtilityPowerDigest(), native = ComputeGeothermalRiserDigest(); const auto random = _random;
	window->OnDropdownSelect(WID_UC_GEO_ACCESS, to_underlying(rig.request.access), 0);
	window->OnClick({}, WID_UC_GEO_RISER, 1); window->OnPlaceObject({}, rig.request.portal); window->OnPlaceObject({}, rig.request.tile);
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM)); window->OnPlaceObject({}, rig.request.port); window->OnPaint();
	REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM)); window->OnPaint();
	CHECK(ComputeGeothermalRiserDigest() == native); CHECK(ComputeUtilityPowerDigest() == power); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(std::ranges::equal(random.state, _random.state));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM)); window->OnClick({}, WID_UC_GEO_RISER_CONFIRM, 1); CHECK(Industry::GetNumItems() == 0);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	window->OnPaint(); REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM)); window->OnClick({}, WID_UC_GEO_RISER_CONFIRM, 1);
	const auto id = FindGeothermalRiserForPortal(rig.request.portal); REQUIRE(Industry::IsValidID(id)); const auto *state = GetGeothermalRiserState(id); REQUIRE(state != nullptr);
	CHECK(state->site == rig.request); CHECK(state->destination == rig.destination); CHECK(state->work_ticks == 0); CHECK_FALSE(state->materials_committed);
	for (const auto &slot : Industry::Get(id)->accepted) CHECK(slot.waiting == 0); // Native funding provides no material bill; subsequent transported construction remains mandatory.
	CHECK(Company::Get(_current_company)->money == cash - Money{120000}); CHECK(ComputeUtilityPowerDigest() == power); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM));
}

/** Page changes and original excavation cursor takeover discard geometry only; a broken original housing refuses before funding or work. */
TEST_CASE("Geothermal physical drafts cancel and refuse without native side effects", "[geothermal-window-physical-refusal]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeRiserRig rig; UndergroundToolbarTestUI ui{_current_company};
	Window *window = OpenGeothermalPhysicalWindow(MapLayer::DeepUnderground); const auto cash = Company::Get(_current_company)->money;
	window->OnClick({}, WID_UC_GEO_RISER, 1); window->OnPlaceObject({}, rig.request.portal); window->OnPlaceObject({}, rig.request.tile); window->OnPlaceObject({}, rig.request.port);
	window->OnDropdownSelect(WID_UC_GEO_PAGE, 0, 0); window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM));
	window->OnDropdownSelect(WID_UC_GEO_PAGE, 1, 0); window->OnClick({}, WID_UC_GEO_RISER, 1);
	window->OnPlaceObject({}, rig.request.portal); window->OnPlaceObject({}, rig.request.tile); window->OnPlaceObject({}, rig.request.port);
	window->OnClick({}, WID_UC_SURVEY, 1); window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM));
	window->OnClick({}, WID_UC_GEO_RISER, 1); window->OnPlaceObject({}, rig.request.portal); window->OnPlaceObject({}, rig.request.tile); window->OnPlaceObject({}, rig.request.port);
	REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, rig.request.portal, false).Succeeded());
	const auto native = ComputeGeothermalRiserDigest(), power = ComputeUtilityPowerDigest(); window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM));
	window->OnClick({}, WID_UC_GEO_RISER_CONFIRM, 1); CHECK(Industry::GetNumItems() == 0); CHECK(ComputeGeothermalRiserDigest() == native); CHECK(ComputeUtilityPowerDigest() == power);
	CHECK(Company::Get(_current_company)->money == cash);
}
