/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */
/**
 * @file utility_controls_window.cpp Original native window callbacks use canonical paid owners and pure live admission.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md.
 * Fixtures disclose floor, material and consumed-frame boundaries. These cases
 * verify controls, command custody and estimates; they provide no raster approval.
 */
#include "../stdafx.h"
#include <any>
#include <filesystem> // Generated-language APIs precede fixture allocation/file safeguards.
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_view_fixture.h"
#include "geothermal_riser_fixture.h"
#include "utility_scheduler_mixed_fixture.h"
#include "../underground_gui.h"
#include "../transport_tube_gui.h"
#include "../geothermal_site_cmd.h"
#include "../ground_sprite.h"
#include "../widgets/underground_widget.h"
#include "../tilehighlight_func.h"
#include "../core/random_func.hpp"
#include "../toolbar_gui.h"
#include "../map_layer_gui.h"
#include "../widgets/rail_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Actual original construction window, including its placement and modal lifetimes, closed before the native fixture disappears. */
class NativeUtilityWindow {
	UndergroundToolbarTestUI ui{_current_company}; ///< Original language/screen/player/viewport lifetime, never simulation permission.
public:
	Window *window = nullptr; ///< Actual native window; no synthetic widget or private controller installer.
	/** @param layer Explicit construction plane, independent of subsequent camera state. */
	explicit NativeUtilityWindow(MapLayer layer)
	{
		this->window = ShowUndergroundConstructionToolbar(layer, GetCurrentMultiLayerProfileSelection()); REQUIRE(this->window != nullptr);
		this->window->OnPaint();
	}
	/** Cancel only the actual transient cursor/menus before the owning window and native world disappear. */
	~NativeUtilityWindow()
	{
		ResetObjectToPlace(); CloseWindowByClass(WindowClass::DropdownMenu); CloseWindowByClass(WindowClass::ErrorMessage);
		// Native construction pickers borrow this world's definitions and map;
		// close them before the inherited main toolbar/viewport lifetime ends.
		for (const auto cls : {WindowClass::BuildToolbar, WindowClass::BuildStation, WindowClass::BuildDepot}) CloseWindowByClass(cls);
		Window::DeleteClosedWindows();
	}
	/** @param widget Actual owned control. Click through the original native window callback, then refresh its live admission. */
	void Click(WidgetID widget) { this->window->OnClick({}, widget, 1); this->window->OnPaint(); }
	/** @param widget Original dropdown owner. @param id Actual native source/vehicle/site choice, never a copied pointer. */
	void Select(WidgetID widget, int id) { this->window->OnDropdownSelect(widget, id, 0); this->window->OnPaint(); }
	/** @param tile Explicit actual world tile delivered to the parent placement owner. */
	void Place(TileIndex tile) { this->window->OnPlaceObject({}, tile); this->window->OnPaint(); }
};

/** Real control states and the armed natural-vent cursor borrow original art without granting work or retaining a reveal. */
TEST_CASE("Native Tube geothermal controls and vent overlay keep their own physical anchors", "[merged-native-control-art]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalGeneratorRig rig;
	const auto &vent = GetGeologyWorldState().vents.front();
	const TileIndex tile = TileXYL(vent.x, vent.y, vent.layer);
	rig.Open(std::array{tile}); // Disclosed floor input permits the excavated source pose; the survey itself opens no terrain.
	NativeUtilityWindow ui(MapLayer::DeepUnderground);
	const auto before = std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeGeothermalGeneratorDigest(),
		ComputeGeothermalSiteDigest(), ComputeUtilityPowerDigest(), Company::Get(_current_company)->money, TimerGameTick::counter,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
	// Every admitted control state keeps its original source row. A body family
	// cannot masquerade as a clickable construction or provider action.
	for (const auto family : {GroundSpriteFamily::ControlTube, GroundSpriteFamily::ControlGeothermal}) {
		CHECK(GetGroundControlSprite(family, false, false) == GetGroundSprite(family, {0}));
		CHECK(GetGroundControlSprite(family, true, false) == GetGroundSprite(family, {1}));
		CHECK(GetGroundControlSprite(family, false, true) == GetGroundSprite(family, {2}));
		CHECK(GetGroundControlSprite(family, true, true) == GetGroundSprite(family, {1}));
	}
	CHECK_FALSE(GetGroundControlSprite(GroundSpriteFamily::VehicleModel09, false, false));
	CHECK_FALSE(GetUndergroundGeothermalSurveyOverlaySprite(tile));
	CHECK(GetUndergroundConstructionWidgetSprite(*ui.window, WID_UC_TUBES) == GetGroundSprite(GroundSpriteFamily::ControlTube, {0}));
	ui.Click(WID_UC_GEOTHERMAL); ui.Click(WID_UC_GEO_SURVEY);
	CHECK(GetUndergroundGeothermalSurveyOverlaySprite(tile) == GetGroundSprite(GroundSpriteFamily::GeothermalVent,
		{2, GetGroundVisualFrame(GroundSpriteFamily::GeothermalVent, TimerGameTick::counter)}));
	CHECK_FALSE(GetUndergroundGeothermalSurveyOverlaySprite(MultiLayerMap::ProjectToSurface(tile)));
	ResetObjectToPlace();
	CHECK_FALSE(GetUndergroundGeothermalSurveyOverlaySprite(tile));
	CHECK(before == std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeGeothermalGeneratorDigest(),
		ComputeGeothermalSiteDigest(), ComputeUtilityPowerDigest(), Company::Get(_current_company)->money, TimerGameTick::counter,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}});
}

/** Dedicated Tube construction opens the original native pickers with an explicit domain, retaining ordinary rail identity and the actual Deep calendar gate. */
TEST_CASE("Actual tube construction controls retain native picker domain and date", "[utility-control-native-pickers]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); UtilitySchedulerMixedRig rig{GeothermalGeneratorModel::MagmaLoop};
	NativeUtilityWindow ui(MapLayer::DeepUnderground); auto *view = new UndergroundTestViewport(); SetViewportLayer(view, MapLayer::DeepUnderground); AllocateToolbar();
	ui.Click(WID_UC_TUBES); ui.Click(WID_UC_RAIL);
	auto *ordinary = FindWindowById(WindowClass::BuildToolbar, TransportType::Rail); REQUIRE(ordinary != nullptr);
	const auto caption = ordinary->GetWidgetString(WID_RAT_CAPTION, STR_NULL);
	ui.Click(WID_UC_TUBE_GUIDEWAY); auto *tube = FindWindowById(WindowClass::BuildToolbar, TransportType::Rail); REQUIRE(tube != nullptr);
	CHECK(tube->GetWidgetString(WID_RAT_CAPTION, STR_NULL).find("Tube") != std::string::npos);
	ui.Click(WID_UC_TUBE_TERMINAL); CHECK(FindWindowById(WindowClass::BuildStation, TransportType::Rail) != nullptr);
	ui.Click(WID_UC_TUBE_DEPOT); CHECK(FindWindowById(WindowClass::BuildDepot, TransportType::Rail) != nullptr);
	ui.Click(WID_UC_RAIL); auto *restored = FindWindowById(WindowClass::BuildToolbar, TransportType::Rail); REQUIRE(restored != nullptr);
	CHECK(restored->GetWidgetString(WID_RAT_CAPTION, STR_NULL) == caption);
	AutoRestoreBackup date(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31));
	ui.window->OnPaint(); CHECK(ui.window->IsWidgetDisabled(WID_UC_TUBE_GUIDEWAY)); CHECK(ui.window->IsWidgetDisabled(WID_UC_TUBE_GEOTHERMAL));
}

/** A real three-click paid riser request creates only its original native machine; pure previews cannot acquire stock, work or electricity. */
TEST_CASE("Actual geothermal riser controls fund original commissioned machinery", "[utility-control-native-riser]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeRiserRig rig{true}; AutoRestoreBackup pause(_pause_mode, PauseModes{});
	NativeUtilityWindow ui(MapLayer::DeepUnderground); ui.Click(WID_UC_GEOTHERMAL); ui.Select(WID_UC_GEO_PAGE, 1);
	ui.Select(WID_UC_GEO_ACCESS, to_underlying(rig.request.access));
	const auto native = ComputeGeothermalRiserDigest(), power = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	ui.Click(WID_UC_GEO_RISER); ui.Place(rig.request.portal); ui.Place(rig.request.tile); CHECK(ui.window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM));
	ui.Place(rig.request.port); REQUIRE_FALSE(ui.window->IsWidgetDisabled(WID_UC_GEO_RISER_CONFIRM));
	CHECK(ComputeGeothermalRiserDigest() == native); CHECK(ComputeUtilityPowerDigest() == power); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(std::ranges::equal(_random.state, random.state)); ui.Click(WID_UC_GEO_RISER_CONFIRM);
	const auto &industry = rig.Riser(); const auto *state = GetGeothermalRiserState(industry.index); REQUIRE(state != nullptr);
	CHECK(state->site == rig.request); CHECK(state->work_ticks == 0); CHECK_FALSE(state->materials_committed);
	CHECK(Company::Get(_current_company)->money == cash - Money{120000}); CHECK(ComputeUtilityPowerDigest() == power);
}

/** Native provider menus inspect current issued identities; binding one real stopped capsule uses the original complete recipient transaction. */
TEST_CASE("Actual underground provider controls bind original geothermal identities", "[utility-control-native-provider]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	UtilitySchedulerMixedRig rig{GeothermalGeneratorModel::MagmaLoop}; auto *capsule = rig.Buy(true); RetireTubePowerVehicle(capsule->index);
	NativeUtilityWindow ui(MapLayer::DeepUnderground); ui.Click(WID_UC_TUBES);
	const auto before = ComputeUtilityPowerDigest(), providers = ComputeTubeGeothermalPowerDigest(), physical = ComputeTubePowerVehicleDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	ui.Select(WID_UC_TUBE_FLEET, capsule->index.base()); ui.Select(WID_UC_TUBE_SOURCE, TUBE_GEOTHERMAL_SOURCE_MENU_BASE + rig.source.base());
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(ComputeTubeGeothermalPowerDigest() == providers);
	CHECK(ComputeTubePowerVehicleDigest() == physical); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(std::ranges::equal(_random.state, random.state)); REQUIRE_FALSE(ui.window->IsWidgetDisabled(WID_UC_TUBE_BIND));
	ui.Click(WID_UC_TUBE_BIND); const auto *bound = GetTubePowerVehicle(capsule->index); REQUIRE(bound != nullptr);
	CHECK(bound->geothermal_source == rig.source); CHECK(bound->remainder_mj == 0); CHECK(bound->consumed_tick == UINT64_MAX);
	CHECK(capsule->vehstatus.Test(VehState::Stopped)); CHECK(capsule->reliability == 100); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(ComputeTubeGeothermalPowerDigest() == providers); CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
}

/** A plant selected before source loss cannot post an obsolete original-output request merely because its coordinate remains in the draft. */
TEST_CASE("Actual native provider draft recaptures lost machinery before confirmation", "[utility-control-stale-provider]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); UtilitySchedulerMixedRig rig{GeothermalGeneratorModel::MagmaLoop};
	REQUIRE(CloseTubeGeothermalPower(rig.source) == TubePowerError::None);
	NativeUtilityWindow ui(MapLayer::DeepUnderground); ui.Click(WID_UC_TUBES); ui.Click(WID_UC_TUBE_GEOTHERMAL);
	ui.Place(rig.plant_port); ui.Place(rig.inlets[0]); REQUIRE_FALSE(ui.window->IsWidgetDisabled(WID_UC_TUBE_CONNECT));
	REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, rig.plant_port,
		std::make_tuple(rig.plant_port)).Succeeded());
	const auto original = ComputeUtilityPowerDigest(), references = ComputeTubeGeothermalPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	ui.window->OnPaint(); CHECK(ui.window->IsWidgetDisabled(WID_UC_TUBE_CONNECT)); ui.Click(WID_UC_TUBE_CONNECT);
	CHECK(ComputeUtilityPowerDigest() == original); CHECK(ComputeTubeGeothermalPowerDigest() == references); CHECK(Company::Get(_current_company)->money == cash);
}

/** Actual planning previews and inspection issue no rights; explicit confirmation/funding acquires only the original synchronized site/machinery. */
TEST_CASE("Actual geothermal controls quote and fund the canonical issued site", "[utility-control-native-site]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalGeneratorRig rig;
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); // Exercise ordinary player command admission; the cold fixture otherwise pauses native construction, and these callbacks advance no simulation ticks.
	REQUIRE(CmdCancelGeothermalSite(DoCommandFlag::Execute, rig.site).Succeeded());
	NativeUtilityWindow ui(MapLayer::DeepUnderground); ui.Click(WID_UC_GEOTHERMAL);
	ui.Select(WID_UC_GEO_FAMILY, to_underlying(rig.request.kind)); ui.Select(WID_UC_GEO_VENT, rig.request.vent.base());
	const auto before = ComputeGeothermalSiteDigest(), utility = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	const auto random = _random; ui.Click(WID_UC_GEO_PLAN); ui.Place(rig.request.tile); ui.Place(rig.request.endpoint);
	REQUIRE_FALSE(ui.window->IsWidgetDisabled(WID_UC_GEO_CONFIRM));
	CHECK(ComputeGeothermalSiteDigest() == before); CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(_random.state, random.state));
	ui.Click(WID_UC_GEO_CONFIRM); const auto *claim = GetGeothermalSiteClaimForTile(rig.request.tile); REQUIRE(claim != nullptr); REQUIRE(claim->active);
	CHECK(claim->id != rig.site); const auto issued = claim->id; ui.Select(WID_UC_GEO_SITE, issued.base());
	REQUIRE_FALSE(ui.window->IsWidgetDisabled(WID_UC_GEO_FUND)); ui.Click(WID_UC_GEO_FUND);
	CHECK(Company::Get(_current_company)->money == cash - Money{GetMultiLayerGeothermalConstruction(rig.request.kind)->capital_cost});
	CHECK_FALSE(GetGeothermalSiteClaim(issued)->active); REQUIRE(Industry::GetNumItems() == 1);
	const auto *native = GetGeothermalGeneratorState(rig.Generator().index); REQUIRE(native != nullptr);
	CHECK(native->site == issued); CHECK(native->work_ticks == 0); CHECK_FALSE(native->materials_committed);
	CHECK(native->electrical_producer.base() == UINT32_MAX); CHECK(ComputeUtilityPowerDigest() == utility);
}

/** Physical tools preserve existing guideway and domain ownership while each actual cell command pays/removes its own original installation. */
TEST_CASE("Actual geothermal utility tools retain native guideway and cell payment", "[utility-control-native-cell]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); UtilitySchedulerMixedRig rig{GeothermalGeneratorModel::MagmaLoop};
	const auto tile = TileXYL(32, 25, MapLayer::DeepUnderground); const auto original = GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity); REQUIRE(original.has_value());
	NativeUtilityWindow ui(MapLayer::DeepUnderground); ui.Click(WID_UC_GEOTHERMAL); ui.Select(WID_UC_GEO_PAGE, 1);
	ui.Click(WID_UC_GEO_REMOVE_ELECTRIC); ui.Place(tile); CHECK_FALSE(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity).has_value());
	CHECK(IsPlainRailTile(tile)); CHECK(GetTrackBits(tile) == TrackBits{Track::Y});
	const auto cash = Company::Get(_current_company)->money; ui.Click(WID_UC_GEO_ELECTRIC); ui.Place(tile);
	const auto fresh = GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity); REQUIRE(fresh.has_value()); CHECK(fresh->id != original->id);
	CHECK(Company::Get(_current_company)->money < cash); CHECK(IsPlainRailTile(tile)); CHECK(GetTrackBits(tile) == TrackBits{Track::Y});
}

/** Actual town dropdowns pass only selected native participants to the existing atomic connection owner; repainting never reconnects or settles electricity. */
TEST_CASE("Actual geothermal town controls connect selected paid machinery once", "[utility-control-native-town]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTownServiceRig rig{true};
	NativeUtilityWindow ui(MapLayer::DeepUnderground); ui.Click(WID_UC_GEOTHERMAL); ui.Select(WID_UC_GEO_PAGE, 2);
	const auto original = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	ui.Select(WID_UC_GEO_GENERATOR, rig.service.generator.base()); ui.Select(WID_UC_GEO_FIRST_RISER, rig.service.first_riser.base() + 1);
	ui.Select(WID_UC_GEO_SECOND_RISER, 0); ui.Select(WID_UC_GEO_TERMINAL, rig.service.terminal.base()); ui.Select(WID_UC_GEO_TOWN, rig.service.town.base());
	CHECK(ComputeUtilityPowerDigest() == original); REQUIRE_FALSE(ui.window->IsWidgetDisabled(WID_UC_GEO_CONNECT));
	ui.Click(WID_UC_GEO_CONNECT); const auto connected = ComputeUtilityPowerDigest(); CHECK(connected != original);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ui.window->IsWidgetDisabled(WID_UC_GEO_CONNECT));
	ui.window->OnPaint(); ui.Click(WID_UC_GEO_CONNECT); CHECK(ComputeUtilityPowerDigest() == connected);
	CHECK(Company::Get(_current_company)->money == cash);
}
