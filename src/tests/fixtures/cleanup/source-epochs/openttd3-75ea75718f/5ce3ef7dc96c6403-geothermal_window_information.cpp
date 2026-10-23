/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_window_information.cpp Live original stock/work/meters and active natural survey ownership without raster acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15CDB.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * Native fixtures explicitly prepare floor/portals and receiver-boundary stock; later15CDC owns actual loaded-world raster acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "underground_view_fixture.h"
#include "geothermal_riser_fixture.h"
#include "geothermal_town_service_fixture.h"
#include "../underground_gui.h"
#include "../geothermal_controls.h"
#include "../geothermal_utility_cmd.h"
#include "../cargotype.h"
#include "../multilayer_sprite.h"
#include "../widgets/underground_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** @param tile Actual native owner to inspect. @param information Explicit construction/energy/town view. @return Real bounded Information page; no gameplay command or quantity is created. */
static Window *OpenGeothermalInformation(TileIndex tile, int information)
{
	Window *window = ShowUndergroundConstructionToolbar(GetTileLayer(tile), GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr);
	window->OnPaint(); window->OnClick({}, WID_UC_GEOTHERMAL, 1); window->OnDropdownSelect(WID_UC_GEO_PAGE, 3, 0);
	window->OnDropdownSelect(WID_UC_GEO_INFORMATION, information, 0); window->OnPaint();
	window->OnClick({}, WID_UC_GEO_SURVEY, 1); window->OnPlaceObject({}, tile); window->OnPaint(); CHECK(window->height <= 720); return window;
}

/** Actual stock changes and whole-bill consumption appear immediately; repeated presentation cannot deliver cargo, pay labour or recreate consumed material. */
TEST_CASE("Geothermal information follows native construction stock and work", "[geothermal-window-information-stock]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); UndergroundToolbarTestUI ui{_current_company};
	Window *window = OpenGeothermalInformation(rig.request.tile, 0);
	const auto &input = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->inputs.front();
	const auto cargo = GetCargoTypeByLabel(input.label); const auto name = GetString(CargoSpec::Get(cargo)->name);
	CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find(GetString(STR_GEO_STOCK, name, 0, input.quantity)) != std::string::npos);
	REQUIRE(rig.Deliver(input.label, 1) == 1);
	CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find(GetString(STR_GEO_STOCK, name, 1, input.quantity)) != std::string::npos);
	const auto work = ComputeGeothermalRiserDigest(), utility = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	const auto random = _random; window->OnPaint(); window->OnPaint();
	CHECK(ComputeGeothermalRiserDigest() == work); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(std::ranges::equal(random.state, _random.state));
	// Fill only each actual remaining receiver-boundary bill. The earlier
	// delivered unit stays original; no repeated whole bill is fabricated.
	for (const auto &bill : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->inputs) {
		const auto room = GetGeothermalRiserInputRoom(rig.Riser(), GetCargoTypeByLabel(bill.label)); REQUIRE(rig.Deliver(bill.label, room) == room);
	}
	const auto ready = window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP);
	CHECK(ready.find(GetString(STR_GEO_BILL_READY)) != std::string::npos);
	CHECK(ready.find(GetString(STR_GEO_LABOUR_REQUIRED)) != std::string::npos);
	rig.Tick(296); REQUIRE(GetGeothermalRiserState(rig.Riser().index)->materials_committed);
	const auto text = window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP);
	CHECK(text.find(GetString(STR_GEO_CONSTRUCTION, 100)) != std::string::npos); CHECK(text.find(GetString(STR_GEO_BILL_CONSUMED)) != std::string::npos);
	CHECK(text.find(GetString(STR_GEO_STOCK, name, 0, input.quantity)) != std::string::npos);
}

/** Both dated generators expose original current requests/remaining rights separately from lifetime generation/use; a fully consumed Heat grant never appears available again. */
TEST_CASE("Geothermal information preserves original current and lifetime meters", "[geothermal-window-information-energy]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	GeothermalGeneratorOperatingRig rig{false, true, 1000000, model}; rig.Operate(); UndergroundToolbarTestUI ui{_current_company};
	Window *window = OpenGeothermalInformation(rig.plant_port, 1); const auto actual = GetGeothermalControlInspection(rig.plant_port);
	REQUIRE(actual.heat_input != nullptr); REQUIRE(actual.electricity != nullptr); REQUIRE(actual.heat != nullptr);
	const auto remaining = GetUtilityHeatAllocation(actual.heat_input->meter.consumer_id); REQUIRE(remaining.has_value()); CHECK(remaining->granted_energy_j == 0);
	const auto text = window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP);
	CHECK(text.find(GetString(STR_GEO_GRANT_METER, actual.heat_input->meter.demand_energy_j, actual.heat_input->meter.granted_energy_j, 0)) != std::string::npos);
	CHECK(text.find(GetString(STR_GEO_ELECTRIC_METER, actual.electricity->generated_lifetime_j, actual.electricity->consumed_lifetime_j)) != std::string::npos);
	CHECK(text.find(GetString(STR_GEO_SOURCE_METER, actual.heat->meter.generation_energy_j, actual.heat->reserved_j)) != std::string::npos);
	CHECK(text.find(GetString(STR_GEO_CONVERSION, GetMultiLayerGeothermalGeneratorTerms(model)->efficiency_percent)) != std::string::npos);
	const auto power = ComputeUtilityPowerDigest(), native = ComputeGeothermalGeneratorDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	window->OnPaint(); window->OnPaint(); CHECK(ComputeUtilityPowerDigest() == power); CHECK(ComputeGeothermalGeneratorDigest() == native);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
	CHECK(HasGeothermalGeneratorServiceFunds(rig.generator, 1)); CHECK_FALSE(HasGeothermalGeneratorServiceFunds(rig.generator, 0));
	CHECK_FALSE(HasGeothermalGeneratorServiceFunds(rig.generator, MAX_UTILITY_INTERVAL_TICKS + 1));
	{
		AutoRestoreBackup funds{Company::Get(_current_company)->money, Money{0}};
		CHECK_FALSE(HasGeothermalGeneratorServiceFunds(rig.generator, 1));
		CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find(GetString(STR_GEO_FUNDS_REQUIRED)) != std::string::npos);
		CHECK(ComputeUtilityPowerDigest() == power); CHECK(ComputeGeothermalGeneratorDigest() == native);
	}
	REQUIRE(rig.pipe.size() > 1);
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, rig.pipe[1], UtilityEnergyDomain::Heat).Succeeded());
	const auto broken = ComputeUtilityPowerDigest();
	CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find(GetString(STR_GEO_HEAT_PATH_REQUIRED)) != std::string::npos);
	CHECK(ComputeUtilityPowerDigest() == broken); CHECK(ComputeGeothermalGeneratorDigest() == native);
}

/** The retained original town account supplies exact current final-consumer debits and actual bonus, including terminal loss without renewed rights. */
TEST_CASE("Geothermal information follows retained town service and blocked terminal", "[geothermal-window-information-town]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTownServiceRig rig;
	const auto proposed = ConnectGeothermalTownService(rig.service, false); REQUIRE(proposed.error == UtilityPowerError::None);
	REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded()); rig.Operate(); UndergroundToolbarTestUI ui{_current_company};
	Window *window = OpenGeothermalInformation(rig.plant_port, 2); window->OnDropdownSelect(WID_UC_GEO_TOWN, rig.service.town.base(), 0);
	const auto *town = GetUtilityPowerConsumer(proposed.destination); REQUIRE(town != nullptr); uint64_t used = 0;
	for (const auto &share : town->shares) used += share.consumed_j; // Compare the exact final-town current-epoch debits; upstream forwarding is not a second useful use.
	CHECK(used > 0); CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find(GetString(STR_GEO_SERVICE_METER, used, 25)) != std::string::npos);
	CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find(GetString(STR_GEO_CURRENT_ELECTRIC, GetUtilityPowerLink(proposed.links[1])->flow_j)) != std::string::npos);
	const auto before = ComputeUtilityPowerDigest(); window->OnPaint(); window->OnPaint(); CHECK(ComputeUtilityPowerDigest() == before);
	REQUIRE(Command<Commands::RemoveFromRailStation>::Do(DoCommandFlag::Execute, rig.platform, rig.platform, false).Succeeded()); rig.Operate();
	const auto blocked = ComputeUtilityPowerDigest(); const auto text = window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP);
	CHECK(text.find(GetString(STR_GEO_TERMINAL_LOST)) != std::string::npos); CHECK(ComputeUtilityPowerDigest() == blocked);
}

/** Exact delivered surveyed/excavated poses remain with the existing active same-plane/calendar survey cursor; none is a discovery flag, source or utility operation. */
TEST_CASE("Geothermal survey overlay borrows original natural field and cursor", "[geothermal-window-survey-overlay]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalGeneratorRig rig; UndergroundToolbarTestUI ui{_current_company};
	const auto vent = GetGeologyWorldState().vents.front(); const auto tile = TileXYL(vent.x, vent.y, vent.layer);
	CHECK_FALSE(GetUndergroundGeothermalSurveyOverlaySprite(tile).has_value());
	Window *window = OpenGeothermalInformation(tile, 1);
	const auto frame = GetMultiLayerVisualFrame(MultiLayerSpriteAsset::GeothermalVent, TimerGameTick::counter, false);
	CHECK(GetUndergroundGeothermalSurveyOverlaySprite(tile) == GetMultiLayerSprite(MultiLayerSpriteAsset::GeothermalVent, {1, frame}));
	const auto solid = ComputeGeologyStateDigest(GetGeologyWorldState()); window->OnPaint(); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == solid);
	rig.Open(std::array{tile}); // Explicit native boundary floor transition, not an excavation performed by survey or evidence of the later paid-world raster.
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), utility = ComputeUtilityPowerDigest(); const auto random = _random;
	CHECK(GetUndergroundGeothermalSurveyOverlaySprite(tile) == GetMultiLayerSprite(MultiLayerSpriteAsset::GeothermalVent, {2, frame}));
	CHECK_FALSE(GetUndergroundGeothermalSurveyOverlaySprite(MultiLayerMap::TileInLayer(tile, MapLayer::Underground)).has_value());
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); CHECK_FALSE(GetUndergroundGeothermalSurveyOverlaySprite(tile).has_value());
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1); CHECK(GetUndergroundGeothermalSurveyOverlaySprite(tile).has_value());
	window->OnClick({}, WID_UC_SURVEY, 1); CHECK_FALSE(GetUndergroundGeothermalSurveyOverlaySprite(tile).has_value());
	window->OnClick({}, WID_UC_GEO_SURVEY, 1); CHECK(GetUndergroundGeothermalSurveyOverlaySprite(tile).has_value());
	window->OnClick({}, WID_UC_GEOTHERMAL, 1); CHECK_FALSE(GetUndergroundGeothermalSurveyOverlaySprite(tile).has_value());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(std::ranges::equal(random.state, _random.state));
}
