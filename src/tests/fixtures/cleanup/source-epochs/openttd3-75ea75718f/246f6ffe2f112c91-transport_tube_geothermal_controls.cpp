/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal_controls.cpp Actual native window input, original source choices and pure live supply guidance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BDC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md Group4B; actual main/extra viewport provider mouse input.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * Paid machinery/cable/capsules use real commands; prepared floor/stock/portal geometry remain explicit boundary inputs.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "underground_view_fixture.h"
#include "geothermal_tube_switch_fixture.h"
#include "tube_input_view_fixture.h"
#include "../transport_tube_gui.h"
#include "../industry.h"
#include "../toolbar_gui.h"
#include "../widgets/underground_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** @return Actual Deep construction window with its tube fold-out open; this helper issues no source or vehicle command. */
static Window *OpenGeothermalTubeControls()
{
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::DeepUnderground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr); window->OnClick({}, WID_UC_TUBES, 1); window->OnPaint();
	return window;
}

/** @param window Actual existing palette. @param rig Real completed original plant and cable/facility owners. @param inlet Explicit native recipient index. Draft only the native connection through ordinary placement callbacks. */
static void DraftGeothermalTubeInlet(Window &window, const GeothermalTubePowerRig &rig, size_t inlet = 0)
{
	window.OnClick({}, WID_UC_TUBE_GEOTHERMAL, 1);
	window.OnPlaceObject({}, Industry::Get(rig.generator)->location.tile);
	window.OnPlaceObject({}, rig.inlets[inlet]); window.OnPaint();
}

/** @param window Actual palette. @param source Independently issued original inlet ID. Select its disjoint presentation row, never a shared producer cast. */
static void SelectGeothermalTubeInlet(Window &window, TubeGeothermalSourceID source)
{
	window.OnDropdownSelect(WID_UC_TUBE_SOURCE, TUBE_GEOTHERMAL_SOURCE_MENU_BASE + source.base(), 0); window.OnPaint();
}

/** @param window Actual laid-out palette. Check native wrapped request and provider text against the rectangles used by the real renderer. */
static void CheckGeothermalTubeTextFit(const Window &window)
{
	// Both live request/custody and bound identity must fit their own visible
	// rows; a passing textual query cannot hide clipped gameplay guidance.
	for (WidgetID widget : {WID_UC_TUBE_STATUS, WID_UC_TUBE_BOUND}) {
		const auto *row = window.GetWidget<NWidgetBase>(widget);
		const auto text = window.GetWidgetString(widget, STR_TUBE_HELP);
		INFO("widget=" << widget << " rectangle=" << row->current_x << 'x' << row->current_y << " text=" << text);
		CHECK(GetStringHeight(text, row->current_x) <= static_cast<int>(row->current_y));
	}
}

/** Actual mouse placement chooses the original completed plant and native inlet without creating energy; a later view change cancels only its new draft. */
TEST_CASE("Tube geothermal source uses actual main and extra viewport mouse input", "[tube-provider-geothermal-input]")
{
	const bool use_extra = GENERATE(false, true);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTubeWorkRig rig;
	UndergroundToolbarTestUI ui(_current_company);
	auto *main = new UndergroundTestViewport(); SetViewportLayer(main, use_extra ? MapLayer::Surface : MapLayer::DeepUnderground);
	Window *extra = CreateTubeInputExtraViewport(use_extra ? MapLayer::DeepUnderground : MapLayer::Surface);
	Window *active = use_extra ? extra : main;
	Window *other = use_extra ? main : extra;
	AllocateToolbar();
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::DeepUnderground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr); window->OnPaint();
	const auto before = std::tuple{ComputeUtilityPowerDigest(), ComputeTubeGeothermalPowerDigest(), ComputeGeothermalGeneratorDigest(), Company::Get(_current_company)->money};
	FeedbackClick(*window, WID_UC_TUBES); window->OnPaint();
	REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_GEOTHERMAL));
	FeedbackClick(*window, WID_UC_TUBE_GEOTHERMAL); FeedbackPumpWindows(); REQUIRE(_thd.GetCallbackWnd() == window);
	const auto plant = Industry::Get(rig.generator)->location.tile;
	MouseClickTubeWorldTile(*other, MultiLayerMap::TileInLayer(plant, MapLayer::Surface));
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find(GetString(STR_UNDERGROUND_WRONG_LAYER)) != std::string::npos);
	MouseClickTubeWorldTile(*active, plant);
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find(GetString(STR_TUBE_GEOTHERMAL_PICK_INLET)) != std::string::npos);
	MouseClickTubeWorldTile(*active, rig.inlets[0]); window->OnPaint(); CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_CONNECT));
	CHECK((std::tuple{ComputeUtilityPowerDigest(), ComputeTubeGeothermalPowerDigest(), ComputeGeothermalGeneratorDigest(), Company::Get(_current_company)->money} == before));
	FeedbackClick(*window, WID_UC_TUBE_CONNECT); FeedbackPumpWindows();
	const auto state = CaptureTubeGeothermalPowerState(); REQUIRE(state.sources.size() == 1);
	const auto source = state.sources.front(); CHECK(source.generator == rig.generator); CHECK(source.producer == rig.output); CHECK(source.inlet == rig.inlets[0]);
	CHECK_FALSE(GetUtilityAllocation(source.supply).has_value());
	const auto installed = std::tuple{ComputeUtilityPowerDigest(), ComputeTubeGeothermalPowerDigest(), ComputeGeothermalGeneratorDigest(), Company::Get(_current_company)->money};
	FeedbackClick(*window, WID_UC_TUBE_GEOTHERMAL); FeedbackPumpWindows(); REQUIRE(_thd.GetCallbackWnd() == window);
	SetViewportLayer(active, MapLayer::Underground); CHECK(_thd.GetCallbackWnd() != window);
	CHECK((std::tuple{ComputeUtilityPowerDigest(), ComputeTubeGeothermalPowerDigest(), ComputeGeothermalGeneratorDigest(), Company::Get(_current_company)->money} == installed));
}

/** Real source creation/binding/closure comes only from confirmed synchronized commands; repeated native drawing/inspection creates no energy or invoice. */
TEST_CASE("Tube controls connect and close the actual original geothermal inlet", "[tube-geothermal-controls-source]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTubeWorkRig rig;
	auto *capsule = rig.Buy(); const auto packet = rig.Load(*capsule);
	UndergroundToolbarTestUI ui(_current_company); auto *window = OpenGeothermalTubeControls();
	const auto cash = Company::Get(_current_company)->money;
	const auto utility = ComputeUtilityPowerDigest(), metadata = ComputeTubeGeothermalPowerDigest(), native = ComputeGeothermalGeneratorDigest();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_GEOTHERMAL));
	window->OnClick({}, WID_UC_TUBE_GEOTHERMAL, 1);
	window->OnPlaceObject({}, MultiLayerMap::TileInLayer(rig.inlets[0], MapLayer::Underground));
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find("another layer") != std::string::npos);
	window->OnPlaceObject({}, rig.inlets[0]); window->OnPaint();
	CHECK(window->IsWidgetDisabled(WID_UC_TUBE_CONNECT)); // A native depot cannot masquerade as the selected plant.
	DraftGeothermalTubeInlet(*window, rig); CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_CONNECT));
	CheckGeothermalTubeTextFit(*window);
	CHECK(window->GetWidgetString(WID_UC_TUBE_CONNECT, STR_TUBE_CONNECT).find("geothermal") != std::string::npos);
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubeGeothermalPowerDigest() == metadata);
	CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(Company::Get(_current_company)->money == cash);
	window->OnClick({}, WID_UC_TUBE_CONNECT, 1);
	const auto sources = CaptureTubeGeothermalPowerState(); REQUIRE(sources.sources.size() == 1); const auto source = sources.sources.front();
	CHECK(source.generator == rig.generator); CHECK(source.producer == rig.output); CHECK(source.inlet == rig.inlets[0]);
	CHECK_FALSE(GetUtilityAllocation(source.supply).has_value()); CHECK(ComputeGeothermalGeneratorDigest() == native);
	SelectGeothermalTubeInlet(*window, source.id);
	CHECK(window->GetWidgetString(WID_UC_TUBE_SOURCE, STR_TUBE_SOURCE).find("Geothermal inlet 0") != std::string::npos);
	window->OnDropdownSelect(WID_UC_TUBE_FLEET, capsule->index.base(), 0); window->OnPaint();
	CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_BIND)); window->OnClick({}, WID_UC_TUBE_BIND, 1);
	REQUIRE(GetTubePowerVehicle(capsule->index) != nullptr);
	CHECK(GetTubePowerVehicle(capsule->index)->geothermal_source == source.id);
	CHECK(window->GetWidgetString(WID_UC_TUBE_BOUND, STR_TUBE_BOUND).find("Geothermal inlet 0") != std::string::npos);
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find("waiting for electricity") != std::string::npos);
	CheckGeothermalTubeTextFit(*window);
	const auto after = ComputeUtilityPowerDigest(), bindings = ComputeTubePowerVehicleDigest();
	window->OnClick({}, WID_UC_TUBE_SOURCE, 1); window->OnPaint(); window->OnPaint();
	const auto ready = GetTubeNetworkReadiness(rig.inlets[0], _current_company); REQUIRE(ready.has_value());
	REQUIRE(ready->geothermal_inlets.size() == 1); CHECK(ready->inlets.empty()); CHECK(ready->geothermal_inlets[0].service_payable);
	CHECK(ready->geothermal_inlets[0].source == source.id); CHECK(ready->geothermal_inlets[0].generator == rig.generator);
	CHECK(ComputeUtilityPowerDigest() == after); CHECK(ComputeTubePowerVehicleDigest() == bindings);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(capsule->vehstatus.Test(VehState::Stopped)); CHECK(capsule->cargo.Packets()->front()->index == packet);
	window->OnClick({}, WID_UC_TUBE_CLOSE, 1); CHECK_FALSE(GetTubeGeothermalSource(source.id)->active);
	CHECK(GetUtilityPowerProducer(rig.output)->active); CHECK(IsGeothermalGeneratorConstructionComplete(rig.generator));
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find("route disconnected") != std::string::npos);
	CHECK(window->GetWidgetString(WID_UC_TUBE_BOUND, STR_TUBE_BOUND).find("closed") != std::string::npos);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(capsule->cargo.StoredCount() == 17);
}

/** Both existing providers and multiple original inlets remain explicit choices; a proposed source cannot silently change the actual binding or player stop. */
TEST_CASE("Tube controls explicitly switch both provider families and original inlets", "[tube-geothermal-controls-switch]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTubeSwitchRig rig;
	auto *capsule = rig.Buy(); const auto packet = rig.Load(*capsule);
	UndergroundToolbarTestUI ui(_current_company); auto *window = OpenGeothermalTubeControls();
	DraftGeothermalTubeInlet(*window, rig, 1); CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_CONNECT));
	window->OnClick({}, WID_UC_TUBE_CONNECT, 1); const auto sources = CaptureTubeGeothermalPowerState(); REQUIRE(sources.sources.size() == 2);
	const auto second = sources.sources.back(); CHECK(second.producer == rig.output); CHECK(second.id != rig.geothermal.source);
	window->OnDropdownSelect(WID_UC_TUBE_FLEET, capsule->index.base(), 0);
	window->OnDropdownSelect(WID_UC_TUBE_SOURCE, rig.external.base(), 0); window->OnPaint(); window->OnClick({}, WID_UC_TUBE_BIND, 1);
	REQUIRE(GetTubePowerVehicle(capsule->index) != nullptr); CHECK(GetTubePowerVehicle(capsule->index)->source == rig.external);
	const auto original = GetTubePowerVehicle(capsule->index)->consumer;
	SelectGeothermalTubeInlet(*window, rig.geothermal.source);
	CHECK(window->GetWidgetString(WID_UC_TUBE_SOURCE, STR_TUBE_SOURCE).find("Geothermal") != std::string::npos);
	CHECK(window->GetWidgetString(WID_UC_TUBE_BOUND, STR_TUBE_BOUND).find("External") != std::string::npos);
	CHECK(GetTubePowerVehicle(capsule->index)->consumer == original);
	window->OnClick({}, WID_UC_TUBE_BIND, 1); CHECK(GetTubePowerVehicle(capsule->index)->geothermal_source == rig.geothermal.source);
	SelectGeothermalTubeInlet(*window, second.id); CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_BIND));
	window->OnClick({}, WID_UC_TUBE_BIND, 1); CHECK(GetTubePowerVehicle(capsule->index)->geothermal_source == second.id);
	const auto ready = GetTubeNetworkReadiness(rig.inlets[0], _current_company); REQUIRE(ready.has_value());
	CHECK(ready->inlets.size() == 1); REQUIRE(ready->geothermal_inlets.size() == 2);
	CHECK(ready->geothermal_inlets[0].source == rig.geothermal.source); CHECK(ready->geothermal_inlets[1].source == second.id);
	window->OnDropdownSelect(WID_UC_TUBE_SOURCE, rig.external.base(), 0); window->OnPaint();
	CHECK(window->GetWidgetString(WID_UC_TUBE_BOUND, STR_TUBE_BOUND).find("Geothermal inlet 1") != std::string::npos);
	window->OnClick({}, WID_UC_TUBE_BIND, 1); CHECK(GetTubePowerVehicle(capsule->index)->source == rig.external);
	CHECK(GetTubePowerVehicle(capsule->index)->geothermal_source.base() == UINT32_MAX);
	CHECK(capsule->vehstatus.Test(VehState::Stopped)); CHECK(capsule->cargo.Packets()->front()->index == packet); CHECK(capsule->cargo.StoredCount() == 17);
}

/** Current cable, service funds and actual capsule waits are read without publishing power, spending cash or changing stopped intent. */
TEST_CASE("Tube geothermal guidance reflects current physical and service loss purely", "[tube-geothermal-controls-guidance]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTubeWorkRig rig;
	const auto source = rig.Connect(); auto *capsule = rig.Buy();
	REQUIRE(BindTubeGeothermalPowerVehicle(capsule->index, source.source).has_value());
	UndergroundToolbarTestUI ui(_current_company); auto *window = OpenGeothermalTubeControls();
	SelectGeothermalTubeInlet(*window, source.source); window->OnDropdownSelect(WID_UC_TUBE_FLEET, capsule->index.base(), 0);
	std::string expected;
	SECTION("no unallocated promise") { expected = "waiting for electricity"; }
	SECTION("plant service funds") { Company::Get(_current_company)->money = Money{0}; expected = "plant cannot afford operating service"; }
	SECTION("lost purchased cable") {
		const auto wire = TileXYL(34, 24, MapLayer::DeepUnderground);
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, wire, UtilityEnergyDomain::Electricity).Succeeded());
		expected = "route disconnected";
	}
	SECTION("original model calendar") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 0, 1); expected = "technology unavailable"; }
	SECTION("actual native repair") {
		capsule->reliability = 100; rig.Run(std::array{capsule});
		REQUIRE(capsule->reliability == capsule->GetEngine()->reliability); expected = "no power hold";
	}
	SECTION("actual Heat path loss") {
		const auto wire = rig.pipe[rig.pipe.size() / 2];
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, wire, UtilityEnergyDomain::Heat).Succeeded());
		capsule->reliability = 100; rig.Run(std::array{capsule});
		CHECK(capsule->reliability == 100); expected = "waiting for electricity";
	}
	const auto utility = ComputeUtilityPowerDigest(), metadata = ComputeTubeGeothermalPowerDigest(), native = ComputeGeothermalGeneratorDigest(), bindings = ComputeTubePowerVehicleDigest();
	const auto cash = Company::Get(_current_company)->money;
	window->OnPaint(); window->OnPaint();
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find(expected) != std::string::npos);
	CheckGeothermalTubeTextFit(*window);
	const auto ready = GetTubeNetworkReadiness(rig.inlets[0], _current_company); REQUIRE(ready.has_value());
	if (expected == "route disconnected") CHECK(ready->geothermal_inlets.empty());
	if (expected == "plant cannot afford operating service") { REQUIRE(ready->geothermal_inlets.size() == 1); CHECK_FALSE(ready->geothermal_inlets[0].service_payable); }
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubeGeothermalPowerDigest() == metadata);
	CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(ComputeTubePowerVehicleDigest() == bindings);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(capsule->vehstatus.Test(VehState::Stopped));
}

/** Pending source/plant references cannot adopt missing or replacement identities, and actual stale/foreign callback choices acquire nothing. */
TEST_CASE("Tube geothermal controls refuse stale source and original output selections", "[tube-geothermal-controls-stale]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTubeWorkRig rig;
	const auto source = rig.Connect(); auto *capsule = rig.Buy();
	UndergroundToolbarTestUI ui(_current_company); auto *window = OpenGeothermalTubeControls();
	DraftGeothermalTubeInlet(*window, rig, 1); CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_CONNECT));
	// Real demolition between the placement and confirmation invalidates the
	// cable admission without replacing or repairing a hidden source record.
	const auto wire = TileXYL(34, 24, MapLayer::DeepUnderground);
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, wire, UtilityEnergyDomain::Electricity).Succeeded());
	const auto utility = ComputeUtilityPowerDigest(), metadata = ComputeTubeGeothermalPowerDigest();
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_TUBE_CONNECT)); window->OnClick({}, WID_UC_TUBE_CONNECT, 1);
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubeGeothermalPowerDigest() == metadata);
	rig.Build(wire, UtilityEnergyDomain::Electricity);
	const auto restored = ComputeUtilityPowerDigest();
	// Both command phases pin the original output independently of the native
	// recyclable machinery ID, including requests queued by other clients.
	for (const DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::ConnectTubeGeothermalPower>::Do(flags, rig.generator, rig.inlets[1], UtilityProducerID{UINT32_MAX}).Failed());
		CHECK(Command<Commands::ConnectTubeGeothermalPower>::Do(flags, rig.generator, rig.inlets[1], UtilityProducerID{rig.output.base() + 1}).Failed());
	}
	CHECK(ComputeUtilityPowerDigest() == restored);
	{
		AutoRestoreBackup<CompanyID> spectator{_local_company, CompanyID{OWNER_NONE}};
		SelectGeothermalTubeInlet(*window, source.source);
		CHECK(window->IsWidgetDisabled(WID_UC_TUBE_BIND)); CHECK(window->IsWidgetDisabled(WID_UC_TUBE_CLOSE));
		CHECK_FALSE(GetTubeNetworkReadiness(rig.inlets[0], _local_company).has_value());
		window->OnClick({}, WID_UC_TUBE_CONNECT, 1); window->OnClick({}, WID_UC_TUBE_BIND, 1);
		CHECK(ComputeUtilityPowerDigest() == restored); CHECK(ComputeTubeGeothermalPowerDigest() == metadata);
	}
	window->OnPaint();
	REQUIRE(CloseTubeGeothermalPower(source.source) == TubePowerError::None);
	const auto closed = ComputeTubeGeothermalPowerDigest(), after_close = ComputeUtilityPowerDigest();
	SelectGeothermalTubeInlet(*window, source.source); window->OnDropdownSelect(WID_UC_TUBE_FLEET, capsule->index.base(), 0); window->OnPaint();
	CHECK(window->IsWidgetDisabled(WID_UC_TUBE_BIND)); CHECK(window->IsWidgetDisabled(WID_UC_TUBE_CLOSE));
	window->OnDropdownSelect(WID_UC_TUBE_SOURCE, INT_MAX, 0); window->OnPaint();
	window->OnClick({}, WID_UC_TUBE_BIND, 1); window->OnClick({}, WID_UC_TUBE_CLOSE, 1);
	CHECK(GetTubePowerVehicle(capsule->index) == nullptr); CHECK(ComputeTubeGeothermalPowerDigest() == closed); CHECK(ComputeUtilityPowerDigest() == after_close);
}
