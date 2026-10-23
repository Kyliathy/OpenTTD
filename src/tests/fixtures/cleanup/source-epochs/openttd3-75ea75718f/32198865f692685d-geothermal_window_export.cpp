/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_window_export.cpp Actual window selections and atomic native fresh/recovered geothermal town service.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15CC.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * Prepared floor, receiver-boundary stock and positioned native town are explicit fixture inputs; the paid first-plant proof remains separate.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "underground_view_fixture.h"
#include "geothermal_town_service_fixture.h"
#include "../underground_gui.h"
#include "../widgets/underground_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** @return Existing Deep window's actual town-service page; no utility identity or energy is created. */
static Window *OpenGeothermalExportWindow()
{
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::DeepUnderground, GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr);
	window->OnPaint(); window->OnClick({}, WID_UC_GEOTHERMAL, 1); window->OnDropdownSelect(WID_UC_GEO_PAGE, 2, 0); window->OnPaint(); return window;
}

/** @param window Actual parent window. @param request Explicit native participants, with no account IDs or joule value. Select the real original route and recipient through actual callbacks. */
static void SelectGeothermalService(Window &window, const GeothermalTownServiceRequest &request)
{
	window.OnDropdownSelect(WID_UC_GEO_GENERATOR, request.generator.base(), 0);
	window.OnDropdownSelect(WID_UC_GEO_FIRST_RISER, request.first_riser.base() + 1, 0);
	window.OnDropdownSelect(WID_UC_GEO_SECOND_RISER, request.second_riser == IndustryID::Invalid() ? 0 : request.second_riser.base() + 1, 0);
	window.OnDropdownSelect(WID_UC_GEO_TERMINAL, request.terminal.base(), 0);
	window.OnDropdownSelect(WID_UC_GEO_TOWN, request.town.base(), 0); window.OnPaint();
}

/** Both dated machines and real one/two-plane routes connect once through actual UI actions; painting/menu cancellation and repeated submission preserve native quantities. */
TEST_CASE("Geothermal window connects actual selected town service once", "[geothermal-window-export]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); const auto direct = GENERATE(false, true);
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop); GeothermalTownServiceRig rig{direct, model};
	AutoRestoreBackup pause{_settings_game.construction.command_pause_level, CommandPauseLevel::AllActions}; UndergroundToolbarTestUI ui{_current_company};
	const auto original = ConnectGeothermalTownService(rig.service, false); REQUIRE(original.error == UtilityPowerError::None);
	const auto before = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest(), work = ComputeGeothermalGeneratorDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	Window *window = OpenGeothermalExportWindow(); SelectGeothermalService(*window, rig.service); REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_CONNECT));
	window->OnDropdownSelect(WID_UC_GEO_GENERATOR, -1, 0); window->OnPaint(); window->OnPaint();
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(ComputeGeothermalGeneratorDigest() == work); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(std::ranges::equal(random.state, _random.state)); CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find("quote accepted") != std::string::npos);
	window->OnClick({}, WID_UC_GEO_CONNECT, 1); const auto after = CaptureUtilityPowerState();
	CHECK(after.operation_sequence == before.operation_sequence + 1); CHECK(after.next_consumer == before.next_consumer + (direct ? 2 : 3));
	REQUIRE(GetUtilityPowerConsumer(original.destination) != nullptr); CHECK(GetUtilityPowerConsumer(original.destination)->town == rig.service.town);
	CHECK(GetUtilityPowerConsumer(original.destination)->town_terminal == rig.service.terminal); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(ComputeGeothermalGeneratorDigest() == work); CHECK(std::ranges::equal(random.state, _random.state));
	const auto connected = ComputeUtilityPowerDigest(); window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_CONNECT)); window->OnClick({}, WID_UC_GEO_CONNECT, 1);
	CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find(GetString(STR_GEO_CONNECTED)) != std::string::npos);
	CHECK(ComputeUtilityPowerDigest() == connected); rig.Operate(); CHECK(GetUtilityTownPowerBonus(rig.service.town) == 25);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Rebuilding a native terminal retains the original town account, exact settled consumption and one later native service effect through the same actual action. */
TEST_CASE("Geothermal window repairs retained native town service", "[geothermal-window-export-repair]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTownServiceRig rig;
	AutoRestoreBackup pause{_settings_game.construction.command_pause_level, CommandPauseLevel::AllActions}; UndergroundToolbarTestUI ui{_current_company};
	const auto original = ConnectGeothermalTownService(rig.service, false); REQUIRE(original.error == UtilityPowerError::None);
	Window *window = OpenGeothermalExportWindow(); SelectGeothermalService(*window, rig.service); window->OnClick({}, WID_UC_GEO_CONNECT, 1); rig.Operate();
	const auto output = GetGeothermalGeneratorState(rig.generator)->electrical_producer; REQUIRE(GetUtilityPowerProducer(output)->consumed_lifetime_j == 135);
	REQUIRE(Command<Commands::RemoveFromRailStation>::Do(DoCommandFlag::Execute, rig.platform, rig.platform, false).Succeeded()); rig.Operate();
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_CONNECT)); const auto blocked = ComputeUtilityPowerDigest(); window->OnClick({}, WID_UC_GEO_CONNECT, 1); CHECK(ComputeUtilityPowerDigest() == blocked);
	rig.service.terminal = rig.BuildTerminal(); SelectGeothermalService(*window, rig.service); REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_CONNECT));
	const auto before = CaptureUtilityPowerState(); const auto cash = Company::Get(_current_company)->money;
	window->OnClick({}, WID_UC_GEO_CONNECT, 1); const auto repaired = CaptureUtilityPowerState();
	CHECK(repaired.next_consumer == before.next_consumer); CHECK(repaired.next_link == before.next_link); CHECK(repaired.operation_sequence == before.operation_sequence + 1);
	CHECK(GetUtilityPowerConsumer(original.destination)->town_terminal == rig.service.terminal); CHECK(GetUtilityPowerProducer(output)->consumed_lifetime_j == 135);
	CHECK(Company::Get(_current_company)->money == cash); rig.Operate(); CHECK(GetUtilityPowerProducer(output)->consumed_lifetime_j == 270);
	CHECK(GetUtilityTownPowerBonus(rig.service.town) == 25); CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** A stale/mismatched route, lost original port or spectator presentation cannot authorize a partial connection or change the actual shared owner. */
TEST_CASE("Geothermal window revalidates original export participants", "[geothermal-window-export-refusal]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTownServiceRig rig;
	AutoRestoreBackup pause{_settings_game.construction.command_pause_level, CommandPauseLevel::AllActions}; UndergroundToolbarTestUI ui{_current_company};
	Window *window = OpenGeothermalExportWindow(); SelectGeothermalService(*window, rig.service); REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_CONNECT));
	const auto digest = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	window->OnDropdownSelect(WID_UC_GEO_SECOND_RISER, 0, 0); window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_CONNECT)); window->OnClick({}, WID_UC_GEO_CONNECT, 1); CHECK(ComputeUtilityPowerDigest() == digest);
	SelectGeothermalService(*window, rig.service);
	{
		AutoRestoreBackup spectator{_local_company, COMPANY_SPECTATOR}; window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_CONNECT)); window->OnClick({}, WID_UC_GEO_CONNECT, 1); CHECK(ComputeUtilityPowerDigest() == digest);
	}
	REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, GetGeothermalRiserState(rig.service.first_riser)->site.portal, false).Succeeded());
	const auto lost = ComputeUtilityPowerDigest(); window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_CONNECT)); window->OnClick({}, WID_UC_GEO_CONNECT, 1);
	CHECK(ComputeUtilityPowerDigest() == lost); CHECK(Company::Get(_current_company)->money == cash);
}
