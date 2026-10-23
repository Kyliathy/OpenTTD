/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_controls.cpp Actual underground fold-out, synchronized service actions and native tube fleet views.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group12C.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md Group4B; actual mouse provider integration.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md Group8B; actual five-model/fleet dropdown input.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md Group8C; native purchase/refit/orders/service input.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * Explicit fixture-open portal/depot input is not normal-newgame or driven useful-route acceptance.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "underground_view_fixture.h"
#include "transport_tube_power_fixture.h"
#include "transport_tube_meter_fixture.h"
#include "native_input_fixture.h"
#include "tube_input_view_fixture.h"
#include "../underground_gui.h"
#include "../map_layer_gui.h"
#include "../transport_tube_model.h"
#include "../tilehighlight_func.h"
#include "../dropdown_func.h"
#include "../engine_func.h"
#include "../road.h"
#include "../depot_func.h"
#include "../vehicle_gui.h"
#include "../toolbar_gui.h"
#include "../widgets/underground_widget.h"
#include "../widgets/rail_widget.h"
#include "../widgets/dropdown_widget.h"
#include "../widgets/vehicle_widget.h"
#include "../widgets/depot_widget.h"
#include "../widgets/build_vehicle_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Native tube picker/view windows close before the fixture's actual vehicles, company and map disappear. */
class TubeControlsTestUI : public UndergroundToolbarTestUI {
public:
	/** @param company Real native player; inherited setup initializes actual viewport/widget owners. */
	explicit TubeControlsTestUI(CompanyID company) : UndergroundToolbarTestUI(company) {}
	/** Retire only this isolated test's native construction and vehicle windows before parent viewport teardown. */
	~TubeControlsTestUI()
	{
		// Native pickers and vehicle views borrow the fixture's actual definitions
		// and vehicles; closing them first prevents stale references during teardown.
		for (WindowClass cls : {WindowClass::BuildToolbar, WindowClass::BuildStation, WindowClass::BuildDepot, WindowClass::VehicleView}) CloseWindowByClass(cls);
		Window::DeleteClosedWindows();
	}
};

/** Native purchase/refit/order windows borrow the same actual vehicles and must close before their parent views and fixture owners disappear. */
class TubeNativeControlsTestUI : public TubeControlsTestUI {
public:
	/** @param company Actual local player whose native command/window permissions apply. */
	explicit TubeNativeControlsTestUI(CompanyID company) : TubeControlsTestUI(company) {}
	/** Retire this test's native child/purchase windows before inherited view and company teardown. */
	~TubeNativeControlsTestUI()
	{
		// These windows retain pointers into the real native fleet/definitions;
		// close the borrowers before base teardown destroys their original owners.
		for (WindowClass cls : {WindowClass::VehicleRefit, WindowClass::VehicleOrders, WindowClass::BuildVehicle, WindowClass::VehicleDepot}) CloseWindowByClass(cls);
		Window::DeleteClosedWindows();
	}
};

/**
 * Select one visible text row through real native dropdown hit testing and deferred dispatch.
 * @param window Existing original palette, already visible above unrelated windows.
 * @param widget Actual native model/fleet dropdown button.
 * @param row Zero-based visible row in the original bounded five-model/fleet list.
 * @param row_count Expected complete original list size, without filtering or scrolling.
 */
static void SelectTubeTextRowThroughMouse(Window &window, WidgetID widget, uint row, uint row_count)
{
	REQUIRE(row < row_count);
	FeedbackClick(window, widget); FeedbackPumpWindows(8);
	Window *menu = FindWindowById(WindowClass::DropdownMenu, 0); REQUIRE(menu != nullptr);
	const Rect list = menu->GetWidget<NWidgetBase>(WID_DM_ITEMS)->GetCurrentRect().Shrink(WidgetDimensions::scaled.dropdownlist)
		.Shrink(WidgetDimensions::scaled.dropdowntext, RectPadding::zero);
	// Native text item geometry also drives the existing TBM hangar input
	// observer. No direct dropdown callback or private selection field is used.
	const uint row_height = MakeDropDownListStringItem(std::string{"height"}, 0)->Height();
	REQUIRE(row_count * row_height <= static_cast<uint>(list.Height()));
	const Point point{menu->left + (list.left + list.right) / 2, menu->top + list.top + static_cast<int>(row * row_height + row_height / 2)};
	REQUIRE(FindWindowFromPt(point.x, point.y) == menu);
	FeedbackMouse(point, false); FeedbackMouse(point, true); FeedbackMouse(point, false); FeedbackPumpWindows(8);
	CHECK(FindWindowById(WindowClass::DropdownMenu, 0) == nullptr);
}

/** All five future/current model rows are inspectable through real deferred mouse selection without buying a vehicle or changing original owners. */
TEST_CASE("Tube model previews use actual native dropdown mouse input", "[tube-provider-model-input]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubePowerRig rig; TubeControlsTestUI ui(_current_company);
	auto *view = new UndergroundTestViewport(); SetViewportLayer(view, MapLayer::Underground); AllocateToolbar();
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr); window->OnPaint();
	FeedbackClick(*window, WID_UC_TUBES); window->OnPaint();
	const auto before = std::tuple{Vehicle::GetNumItems(), Engine::GetNumItems(), ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), Company::Get(_current_company)->money};
	const auto random = _random;
	// Each original row must retain its distinct native name, launch and cargo
	// role even in1970, when four models cannot yet be purchased.
	for (uint16_t model = 9; model <= 13; ++model) {
		SelectTubeTextRowThroughMouse(*window, WID_UC_TUBE_MODELS, model - 9, 5);
		const auto capability = GetTubeVehicleCapability(GetTubeEngine(model)); REQUIRE(capability.has_value());
		const auto text = window->GetWidgetString(WID_UC_TUBE_PREVIEW, STR_TUBE_HELP);
		CHECK(text.find(capability->catalogue->name) != std::string::npos);
		CHECK(text.find(fmt::format("{}", capability->catalogue->introduction_year.base())) != std::string::npos);
		// Actual canonical cargo labels define the preview role; a broad cargo
		// class must not make liquid/sealed/heavy models look interchangeable.
		for (CargoLabel label : capability->catalogue->cargo_labels) CHECK(text.find(GetString(CargoSpec::Get(GetCargoTypeByLabel(label))->name)) != std::string::npos);
	}
	CHECK((std::tuple{Vehicle::GetNumItems(), Engine::GetNumItems(), ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), Company::Get(_current_company)->money} == before));
	CHECK(std::ranges::equal(random.state, _random.state));
}

/** Actual native startup/purchases supply all five fronts; real owned-layer fleet row clicks open their original views without altering vehicles, funds, orders or power. */
TEST_CASE("Tube fleet views use actual native dropdown mouse input", "[tube-provider-fleet-input]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubePrototypeRig rig;
	const auto latest = Engine::Get(GetTubeEngine(13))->info.base_intro;
	AutoRestoreBackup calendar{TimerGameCalendar::date, latest};
	AutoRestoreBackup year{TimerGameCalendar::year, TimerGameCalendar::ConvertDateToYMD(latest).year};
	// Normal startup owns company availability. Do not force masks to bypass
	// the canonical introduction while constructing this real native fleet.
	ResetRoadTypes(); SetYearEngineAgingStops(); StartupEngines();
	std::vector<VehicleID> fronts;
	// Real native purchases create five distinct fronts in stable pool order;
	// the palette must borrow those identities instead of allocating a registry.
	for (uint16_t model = 9; model <= 13; ++model) {
		const auto engine = GetTubeEngine(model); REQUIRE(IsEngineBuildable(engine, VehicleType::Train, _current_company));
		const auto [cost, id, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute, rig.depot, engine, false, INVALID_CARGO, ClientID::Invalid);
		REQUIRE(cost.Succeeded()); fronts.push_back(id);
	}
	const auto ordinary = rig.BuildDepot(MapLayer::Underground, false); rig.Buy(ordinary, 2);
	CHECK(BuildUndergroundTubeFleetFilter(MapLayer::Underground, _current_company) == fronts);
	CHECK(BuildUndergroundTubeFleetFilter(MapLayer::DeepUnderground, _current_company).empty());
	TubeControlsTestUI ui(_current_company);
	auto *view = new UndergroundTestViewport(); SetViewportLayer(view, MapLayer::Underground); AllocateToolbar();
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr); window->OnPaint();
	FeedbackClick(*window, WID_UC_TUBES); window->OnPaint();
	const auto before = std::tuple{Vehicle::GetNumItems(), Engine::GetNumItems(), ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), Company::Get(_current_company)->money}; const auto random = _random;
	// Every real fleet row must open its own pool-owned native vehicle view.
	// Closing that presentation between selections leaves the palette exposed
	// without driving a command or changing any actual vehicle/order owner.
	for (uint row = 0; row < fronts.size(); ++row) {
		SelectTubeTextRowThroughMouse(*window, WID_UC_TUBE_FLEET, row, static_cast<uint>(fronts.size())); window->OnPaint();
		REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_VIEW)); FeedbackClick(*window, WID_UC_TUBE_VIEW); FeedbackPumpWindows();
		REQUIRE(FindWindowById(WindowClass::VehicleView, fronts[row]) != nullptr);
		CloseWindowByClass(WindowClass::VehicleView); Window::DeleteClosedWindows();
	}
	CHECK(BuildUndergroundTubeFleetFilter(MapLayer::Underground, _current_company) == fronts);
	CHECK((std::tuple{Vehicle::GetNumItems(), Engine::GetNumItems(), ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), Company::Get(_current_company)->money} == before));
	CHECK(std::ranges::equal(random.state, _random.state));
}

/** An ordinary click on the real tube depot opens its original purchase controls and one confirmed native build pays the exact quote for an empty capsule. */
TEST_CASE("Tube depot purchase uses actual native mouse input", "[tube-native-purchase-input]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubePrototypeRig rig;
	AutoRestoreBackup calendar{TimerGameCalendar::date, Engine::Get(GetTubeEngine(9))->info.base_intro};
	AutoRestoreBackup year{TimerGameCalendar::year, TimerGameCalendar::ConvertDateToYMD(TimerGameCalendar::date).year};
	ResetRoadTypes(); SetYearEngineAgingStops(); StartupEngines();
	REQUIRE(IsEngineBuildable(GetTubeEngine(9), VehicleType::Train, _current_company));
	TubeNativeControlsTestUI ui(_current_company);
	auto *view = new UndergroundTestViewport(); SetViewportLayer(view, MapLayer::Underground); AllocateToolbar();
	const auto cash = Company::Get(_current_company)->money; const auto count = Vehicle::GetNumItems();
	const auto [quote, predicted, capacity, mail, capacities] = CmdBuildVehicle({}, rig.depot, GetTubeEngine(9), false, INVALID_CARGO, ClientID::Invalid); REQUIRE(quote.Succeeded());
	const auto random = _random;
	MouseClickTubeWorldTile(*view, rig.depot); FeedbackPumpWindows();
	Window *depot = FindWindowById(WindowClass::VehicleDepot, rig.depot.base()); REQUIRE(depot != nullptr);
	FeedbackClick(*depot, WID_D_BUILD); FeedbackPumpWindows();
	Window *purchase = FindWindowById(WindowClass::BuildVehicle, rig.depot.base()); REQUIRE(purchase != nullptr); purchase->OnPaint();
	REQUIRE_FALSE(purchase->IsWidgetDisabled(WID_BV_BUILD));
	CHECK(Company::Get(_current_company)->money == cash); CHECK(Vehicle::GetNumItems() == count); CHECK(std::ranges::equal(random.state, _random.state));
	FeedbackClick(*purchase, WID_BV_BUILD); FeedbackPumpWindows();
	const auto fleet = BuildUndergroundTubeFleetFilter(MapLayer::Underground, _current_company); REQUIRE(fleet.size() == 1);
	const auto *capsule = Train::Get(fleet.front()); CHECK(capsule->engine_type == GetTubeEngine(9)); CHECK(capsule->IsStoppedInDepot()); CHECK(capsule->tile == rig.depot);
	CHECK(capsule->cargo.StoredCount() == 0); CHECK(capsule->cargo_cap == GetTubeVehicleDefinition(9)->capacity);
	CHECK(Vehicle::GetNumItems() == count + 1); CHECK(Company::Get(_current_company)->money == cash - quote.GetCost());
	CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(CaptureUtilityPowerState().consumers.empty());
}

/** Every original native capsule opens its own order/refit controls through real input, preserving single-cargo disabled policy and all original empty custody. */
TEST_CASE("Tube native refit and order controls use actual mouse input", "[tube-native-control-input]")
{
	const uint16_t model = GENERATE(9, 10, 11, 12, 13); const auto action = GENERATE(WID_VV_REFIT, WID_VV_SHOW_ORDERS);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubePrototypeRig rig;
	AutoRestoreBackup calendar{TimerGameCalendar::date, Engine::Get(GetTubeEngine(13))->info.base_intro};
	AutoRestoreBackup year{TimerGameCalendar::year, TimerGameCalendar::ConvertDateToYMD(TimerGameCalendar::date).year};
	ResetRoadTypes(); SetYearEngineAgingStops(); StartupEngines();
	const auto engine = GetTubeEngine(model); REQUIRE(IsEngineBuildable(engine, VehicleType::Train, _current_company));
	const auto [cost, id, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute, rig.depot, engine, false, INVALID_CARGO, ClientID::Invalid); REQUIRE(cost.Succeeded());
	const Train *capsule = Train::Get(id); TubeNativeControlsTestUI ui(_current_company);
	auto *view = new UndergroundTestViewport(); SetViewportLayer(view, MapLayer::Underground); AllocateToolbar();
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr); window->OnPaint();
	FeedbackClick(*window, WID_UC_TUBES); window->OnPaint();
	const auto help = window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP);
	CHECK(help.find("cannot use ordinary rail or layer portals") != std::string::npos);
	CHECK(help.find("transfer cargo to conventional freight") != std::string::npos);
	const auto *status_row = window->GetWidget<NWidgetBase>(WID_UC_TUBE_STATUS);
	CHECK(GetStringHeight(help, status_row->current_x) <= static_cast<int>(status_row->current_y));
	SelectTubeTextRowThroughMouse(*window, WID_UC_TUBE_FLEET, 0, 1); window->OnPaint();
	FeedbackClick(*window, WID_UC_TUBE_VIEW); FeedbackPumpWindows(); Window *vehicle = FindWindowById(WindowClass::VehicleView, id); REQUIRE(vehicle != nullptr);
	const auto before = std::tuple{Vehicle::GetNumItems(), capsule->tile, capsule->cargo_type, capsule->cargo_cap, capsule->GetNumOrders(), ComputeUtilityPowerDigest(), Company::Get(_current_company)->money}; const auto random = _random;
	const bool refittable = IsEngineRefittable(engine);
	if (action == WID_VV_REFIT) CHECK(vehicle->IsWidgetDisabled(action) == !refittable);
	FeedbackClick(*vehicle, action); FeedbackPumpWindows();
	if (action == WID_VV_SHOW_ORDERS) {
		REQUIRE(FindWindowById(WindowClass::VehicleOrders, id) != nullptr);
	} else if (refittable) {
		REQUIRE(FindWindowById(WindowClass::VehicleRefit, id) != nullptr);
	} else {
		// A canonical single-cargo capsule has no alternate refit. Opening its
		// view must not invent a cargo option merely to enable the shared button.
		CHECK(FindWindowById(WindowClass::VehicleRefit, id) == nullptr);
	}
	CHECK((std::tuple{Vehicle::GetNumItems(), capsule->tile, capsule->cargo_type, capsule->cargo_cap, capsule->GetNumOrders(), ComputeUtilityPowerDigest(), Company::Get(_current_company)->money} == before));
	CHECK(capsule->cargo.StoredCount() == 0); CHECK(std::ranges::equal(random.state, _random.state));
}

/** A physically departed paid capsule accepts the original Ctrl-service command from its real vehicle view without acquiring energy or changing its retained order list. */
TEST_CASE("Tube service order uses actual native mouse input", "[tube-native-service-input]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubeMeterRig rig; rig.Bind(); rig.Start();
	// The service control is the normal travelling-vehicle action. Wait for
	// actual native movement out of the depot; no tile/hidden-state relocation
	// may substitute for departure or expose the wrong stopped clone button.
	for (uint tick = 0; tick < 256; ++tick) {
		rig.Run();
		// Both physical departure and visible body ownership must agree before
		// the test can claim the real travelling service control was clicked.
		if (rig.capsule->tile != rig.depot && !rig.capsule->vehstatus.Test(VehState::Hidden)) break;
	}
	REQUIRE_FALSE(rig.capsule->IsInDepot()); REQUIRE_FALSE(rig.capsule->vehstatus.Test(VehState::Hidden));
	TubeNativeControlsTestUI ui(_current_company); auto *view = new UndergroundTestViewport(); SetViewportLayer(view, MapLayer::Underground); AllocateToolbar();
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr); window->OnPaint();
	FeedbackClick(*window, WID_UC_TUBES); window->OnPaint(); SelectTubeTextRowThroughMouse(*window, WID_UC_TUBE_FLEET, 0, 1); window->OnPaint();
	FeedbackClick(*window, WID_UC_TUBE_VIEW); FeedbackPumpWindows(); Window *vehicle = FindWindowById(WindowClass::VehicleView, rig.capsule->index); REQUIRE(vehicle != nullptr);
	const auto power = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	const auto *orders = rig.capsule->orders; const auto count = rig.capsule->GetNumOrders();
	const auto consumer = GetTubePowerVehicle(rig.capsule->index)->consumer;
	{
		AutoRestoreBackup control{_ctrl_pressed, true}; FeedbackClick(*vehicle, WID_VV_GOTO_DEPOT); FeedbackPumpWindows();
	}
	REQUIRE(rig.capsule->current_order.IsType(OT_GOTO_DEPOT));
	// Manual Ctrl-service clears the ordered Halt intent. The Service type bit
	// belongs to automatic maintenance (which may skip a healthy vehicle), so
	// the ordinary explicit player command correctly leaves that bit clear.
	CHECK_FALSE(rig.capsule->current_order.GetDepotOrderType().Test(OrderDepotTypeFlag::Service)); CHECK_FALSE(rig.capsule->current_order.GetDepotOrderType().Test(OrderDepotTypeFlag::PartOfOrders));
	CHECK_FALSE(rig.capsule->current_order.GetDepotActionType().Test(OrderDepotActionFlag::Halt));
	CHECK(GetTileLayer(rig.capsule->dest_tile) == MapLayer::Underground); CHECK(IsRailDepotTile(rig.capsule->dest_tile)); CHECK(GetRailType(rig.capsule->dest_tile) == rig.tube);
	CHECK(rig.capsule->orders == orders); CHECK(rig.capsule->GetNumOrders() == count); CHECK(GetTubePowerVehicle(rig.capsule->index)->consumer == consumer);
	CHECK(ComputeUtilityPowerDigest() == power); CHECK(Company::Get(_current_company)->money == cash);
}

/** Real native mouse press/release and window timers must retain each selected tube tool without changing source accounting. */
TEST_CASE("Tube provider tools survive real native mouse dispatch", "[tube-provider-native-input]")
{
	const auto action = GENERATE(WID_UC_TUBE_GUIDEWAY, WID_UC_TUBE_TERMINAL, WID_UC_TUBE_DEPOT, WID_UC_TUBE_ROUTE);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubePowerRig rig;
	TubeControlsTestUI ui(_current_company);
	auto *view = new UndergroundTestViewport(); SetViewportLayer(view, MapLayer::Underground);
	AllocateToolbar();
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr); window->OnPaint();
	const auto before = std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), Company::Get(_current_company)->money, TimerGameTick::counter};
	FeedbackClick(*window, WID_UC_TUBES); window->OnPaint();
	REQUIRE_FALSE(window->IsWidgetDisabled(action));
	FeedbackClick(*window, action); FeedbackPumpWindows();
	INFO("actual tube provider widget=" << action);
	if (action == WID_UC_TUBE_ROUTE) {
		REQUIRE(_thd.GetCallbackWnd() == window);
		// Closing the real menu cancels only its uncommitted route draft.
		// Native source/cash/work remains untouched through both input events.
		FeedbackClick(*window, WID_UC_TUBES); FeedbackPumpWindows();
		CHECK(_thd.GetCallbackWnd() != window);
	} else {
		Window *toolbar = FindWindowById(WindowClass::BuildToolbar, TransportType::Rail);
		REQUIRE(toolbar != nullptr);
		CHECK(toolbar->GetWidgetString(WID_RAT_CAPTION, STR_TUBE_CONSTRUCTION_CAPTION).find("Tube") != std::string::npos);
		// Terminal/depot shortcuts must leave the actual native rail picker in
		// charge after release; the parent palette cannot cancel its placement.
		if (action == WID_UC_TUBE_TERMINAL || action == WID_UC_TUBE_DEPOT) {
			CHECK(FindWindowById(action == WID_UC_TUBE_TERMINAL ? WindowClass::BuildStation : WindowClass::BuildDepot, TransportType::Rail) != nullptr);
			CHECK(_thd.GetCallbackWnd() == toolbar);
		}
	}
	CHECK((std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), Company::Get(_current_company)->money, TimerGameTick::counter} == before));
}

/** Real main/extra viewport clicks quote and buy the same original service, reject other-plane aliases and cancel only an unfinished draft on layer change. */
TEST_CASE("Tube power route uses actual main and extra viewport mouse input", "[tube-provider-viewport-input]")
{
	const bool use_extra = GENERATE(false, true);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubePowerRig rig;
	TubeControlsTestUI ui(_current_company);
	auto *main = new UndergroundTestViewport(); SetViewportLayer(main, use_extra ? MapLayer::Surface : MapLayer::Underground);
	Window *extra = CreateTubeInputExtraViewport(use_extra ? MapLayer::Underground : MapLayer::Surface);
	Window *active = use_extra ? extra : main;
	Window *other = use_extra ? main : extra;
	AllocateToolbar();
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr); window->OnPaint();
	const auto cash = Company::Get(_current_company)->money;
	const auto before = std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), ComputeGeologyStateDigest(GetGeologyWorldState()), cash, TimerGameTick::counter};
	FeedbackClick(*window, WID_UC_TUBES); window->OnPaint();
	FeedbackClick(*window, WID_UC_TUBE_ROUTE); FeedbackPumpWindows();
	REQUIRE(_thd.GetCallbackWnd() == window);
	MouseClickTubeWorldTile(*other, MultiLayerMap::TileInLayer(rig.portal, MapLayer::Surface));
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find(GetString(STR_UNDERGROUND_WRONG_LAYER)) != std::string::npos);
	MouseClickTubeWorldTile(*active, rig.portal);
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find(GetString(STR_TUBE_PICK_ROUTE)) != std::string::npos);
	MouseClickTubeWorldTile(*active, rig.path[0]); MouseClickTubeWorldTile(*active, rig.depot); window->OnPaint();
	CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_CONNECT));
	CHECK((std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), ComputeGeologyStateDigest(GetGeologyWorldState()), Company::Get(_current_company)->money, TimerGameTick::counter} == before));
	FeedbackClick(*window, WID_UC_TUBE_CONNECT); FeedbackPumpWindows();
	const auto sources = CaptureTubePowerConnections(); REQUIRE(sources.size() == 1);
	CHECK(Company::Get(_current_company)->money == cash - Money{GetMultiLayerTubePowerContract().lease_per_day});
	CHECK(sources.front().portal == rig.portal); CHECK(sources.front().interchange == rig.depot);
	const auto installed = std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), Company::Get(_current_company)->money};
	FeedbackClick(*window, WID_UC_TUBE_ROUTE); FeedbackPumpWindows(); REQUIRE(_thd.GetCallbackWnd() == window);
	SetViewportLayer(active, MapLayer::DeepUnderground);
	CHECK(_thd.GetCallbackWnd() != window);
	CHECK((std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), Company::Get(_current_company)->money} == installed));
}

/** Actual controls post one paid source and one explicit binding; repeated drawing never allocates power or changes work. */
TEST_CASE("Underground tube controls connect a paid source and bind a real capsule", "[transport-tube-controls]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	TubePowerRig rig;
	TubeControlsTestUI ui(_current_company);
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr);
	const auto cash = Company::Get(_current_company)->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest();
	const auto height = window->height;
	window->OnPaint();
	window->OnClick({}, WID_UC_TUBES, 1);
	window->OnPaint();
	CHECK(window->height > height);
	CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_ROUTE));
	CHECK(window->IsWidgetDisabled(WID_UC_TUBE_CONNECT));
	CHECK(ComputeUtilityPowerDigest() == ledger);
	CHECK(ComputeTubePowerDigest() == physical);
	window->OnClick({}, WID_UC_TUBE_ROUTE, 1);
	window->OnPlaceObject({}, MultiLayerMap::TileInLayer(rig.portal, MapLayer::Surface));
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find("another layer") != std::string::npos);
	window->OnPlaceObject({}, rig.portal);
	window->OnPlaceObject({}, rig.path[0]);
	window->OnPlaceObject({}, TileXYL(37, 40, MapLayer::Underground));
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find("adjacent") != std::string::npos);
	window->OnPlaceObject({}, rig.depot);
	window->OnPaint();
	CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_CONNECT));
	CHECK(CaptureTubePowerConnections().empty());
	CHECK(Company::Get(_current_company)->money == cash);
	window->OnClick({}, WID_UC_TUBE_CONNECT, 1);
	const auto sources = CaptureTubePowerConnections();
	REQUIRE(sources.size() == 1);
	CHECK(Company::Get(_current_company)->money == cash - Money{20});
	window->OnDropdownSelect(WID_UC_TUBE_SOURCE, sources.front().producer.base(), 0);
	window->OnDropdownSelect(WID_UC_TUBE_FLEET, rig.capsule->index.base(), 0);
	window->OnPaint();
	CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_BIND));
	window->OnClick({}, WID_UC_TUBE_BIND, 1);
	const auto *binding = GetTubePowerVehicle(rig.capsule->index);
	REQUIRE(binding != nullptr);
	const auto original_consumer = binding->consumer;
	CHECK(binding->source == sources.front().producer);
	CHECK(GetUtilityPowerConsumer(binding->consumer)->meter.granted_energy_j == 0);
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find("waiting for electricity") != std::string::npos);
	const auto after_binding = ComputeTubePowerVehicleDigest(), after_power = ComputeUtilityPowerDigest();
	window->OnPaint(); window->OnPaint();
	CHECK(ComputeTubePowerVehicleDigest() == after_binding);
	CHECK(ComputeUtilityPowerDigest() == after_power);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == cash - Money{20});
	window->OnClick({}, WID_UC_TUBE_CLOSE, 1);
	CHECK_FALSE(GetTubePowerConnection(sources.front().producer)->active);
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find("route disconnected") != std::string::npos);
	CHECK(Company::Get(_current_company)->money == cash - Money{20});
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
	rig.capsule->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep); // Explicit packet custody input, not a transported useful-route claim.
	const auto packet_id = packet->index;
	window->OnClick({}, WID_UC_TUBE_ROUTE, 1);
	window->OnPlaceObject({}, rig.portal);
	window->OnPlaceObject({}, rig.path[0]);
	window->OnPlaceObject({}, rig.depot);
	window->OnPaint();
	window->OnClick({}, WID_UC_TUBE_CONNECT, 1);
	const auto replacement_sources = CaptureTubePowerConnections();
	REQUIRE(replacement_sources.size() == 2);
	const auto replacement = replacement_sources.back().producer;
	CHECK(replacement != sources.front().producer);
	window->OnDropdownSelect(WID_UC_TUBE_SOURCE, replacement.base(), 0);
	window->OnPaint();
	CHECK_FALSE(window->IsWidgetDisabled(WID_UC_TUBE_BIND));
	CHECK(GetTubePowerVehicle(rig.capsule->index)->source == sources.front().producer);
	window->OnClick({}, WID_UC_TUBE_BIND, 1);
	const auto *rebound = GetTubePowerVehicle(rig.capsule->index);
	REQUIRE(rebound != nullptr);
	CHECK(rebound->source == replacement);
	CHECK(rebound->consumer != original_consumer);
	const auto bindings = CaptureTubePowerVehicles();
	const auto retired = std::ranges::find(bindings, original_consumer, &TubePowerVehicle::consumer);
	REQUIRE(retired != bindings.end());
	CHECK_FALSE(retired->active); // Physical binding retires; the shared ledger keeps its historical account identity.
	CHECK(GetUtilityPowerConsumer(original_consumer)->meter.granted_energy_j == 0);
	CHECK(GetUtilityPowerConsumer(rebound->consumer)->meter.granted_energy_j == 0);
	CHECK(rig.capsule->cargo.StoredCount() == 17);
	CHECK(rig.capsule->cargo.Packets()->front()->index == packet_id);
	CHECK(Company::Get(_current_company)->money == cash - Money{40});
}

/** Dedicated controls use the native construction toolbar/pickers and the same 1970/1980 gates as commands. */
TEST_CASE("Underground tube construction controls preserve native picker identity and dates", "[transport-tube-controls-construction]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	TubePowerRig rig;
	TubeControlsTestUI ui(_current_company);
	auto *view = new UndergroundTestViewport();
	SetViewportLayer(view, MapLayer::Underground);
	AllocateToolbar(); // Native construction toolbars align to the real main toolbar, as they do in the running game.
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1969}, 11, 31);
	TimerGameCalendar::year = TimerGameCalendar::Year{1969};
	window->OnClick({}, WID_UC_TUBES, 1);
	window->OnPaint();
	CHECK(window->IsWidgetDisabled(WID_UC_TUBE_GUIDEWAY));
	CHECK(window->IsWidgetDisabled(WID_UC_TUBE_ROUTE));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1970}, 0, 1);
	TimerGameCalendar::year = TimerGameCalendar::Year{1970};
	window->OnPaint();
	window->OnClick({}, WID_UC_RAIL, 1);
	Window *ordinary_toolbar = FindWindowById(WindowClass::BuildToolbar, TransportType::Rail);
	REQUIRE(ordinary_toolbar != nullptr);
	const auto ordinary_caption = ordinary_toolbar->GetWidgetString(WID_RAT_CAPTION, STR_TUBE_CONSTRUCTION_CAPTION);
	window->OnClick({}, WID_UC_TUBE_GUIDEWAY, 1);
	Window *toolbar = FindWindowById(WindowClass::BuildToolbar, TransportType::Rail);
	REQUIRE(toolbar != nullptr);
	CHECK(toolbar->GetWidgetString(WID_RAT_CAPTION, STR_TUBE_CONSTRUCTION_CAPTION).find("Tube") != std::string::npos);
	window->OnClick({}, WID_UC_TUBE_TERMINAL, 1);
	CHECK(FindWindowById(WindowClass::BuildStation, TransportType::Rail) != nullptr);
	window->OnClick({}, WID_UC_TUBE_DEPOT, 1);
	CHECK(FindWindowById(WindowClass::BuildDepot, TransportType::Rail) != nullptr);
	window->OnClick({}, WID_UC_RAIL, 1);
	CHECK(toolbar->GetWidgetString(WID_RAT_CAPTION, STR_TUBE_CONSTRUCTION_CAPTION) == ordinary_caption);
	Window *deep = ShowUndergroundConstructionToolbar(MapLayer::DeepUnderground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(deep != nullptr);
	deep->OnPaint();
	CHECK(deep->IsWidgetDisabled(WID_UC_TUBE_GUIDEWAY));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	TimerGameCalendar::year = TimerGameCalendar::Year{1980};
	deep->OnPaint();
	CHECK_FALSE(deep->IsWidgetDisabled(WID_UC_TUBE_GUIDEWAY));
}

/** The fleet contains actual capsule fronts only, and canonical model/refit previews never purchase a future design. */
TEST_CASE("Underground tube fleet and cargo previews borrow real vehicles and canonical labels", "[transport-tube-controls-fleet]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	TubePowerRig rig;
	TubeControlsTestUI ui(_current_company);
	new UndergroundTestViewport();
	const auto ordinary = rig.BuildDepot(MapLayer::Underground, false);
	rig.Buy(ordinary, 2);
	CHECK(BuildUndergroundTubeFleetFilter(MapLayer::Underground, _current_company) == std::vector{rig.capsule->index});
	CHECK(BuildUndergroundTubeFleetFilter(MapLayer::DeepUnderground, _current_company).empty());
	CHECK(BuildUndergroundTubeFleetFilter(MapLayer::Surface, _current_company).empty());
	CHECK(BuildUndergroundTubeFleetFilter(MapLayer::Underground, OWNER_NONE).empty());
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr);
	const auto vehicles = Vehicle::GetNumItems(), engines = Engine::GetNumItems();
	const auto cash = Company::Get(_current_company)->money;
	window->OnDropdownSelect(WID_UC_TUBE_FLEET, rig.capsule->index.base(), 0);
	const auto power_status = window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP);
	// Each distinct model exposes its real dates/refit labels even before its
	// introduction. Inspection must not create a model, vehicle or permission.
	for (uint16_t model = 9; model <= 13; ++model) {
		window->OnDropdownSelect(WID_UC_TUBE_MODELS, model, 0);
		const auto text = window->GetWidgetString(WID_UC_TUBE_PREVIEW, STR_TUBE_HELP);
		CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP) == power_status);
		const auto capability = GetTubeVehicleCapability(GetTubeEngine(model));
		REQUIRE(capability.has_value());
		CHECK(text.find(capability->catalogue->name) != std::string::npos);
		// All labels are checked against the native cargo names, not an alternate
		// packaged/fluid interpretation chosen by the UI.
		for (CargoLabel label : capability->catalogue->cargo_labels) {
			CHECK(text.find(GetString(CargoSpec::Get(GetCargoTypeByLabel(label))->name)) != std::string::npos);
		}
	}
	CHECK(Vehicle::GetNumItems() == vehicles);
	CHECK(Engine::GetNumItems() == engines);
	CHECK(Company::Get(_current_company)->money == cash);
	window->OnDropdownSelect(WID_UC_TUBE_FLEET, rig.capsule->index.base(), 0);
	window->OnPaint();
	window->OnClick({}, WID_UC_TUBE_VIEW, 1);
	CHECK(FindWindowById(WindowClass::VehicleView, rig.capsule->index.base()) != nullptr);
}

/** A draft cannot keep using a portal whose commissioned work owner vanished between actual clicks. */
TEST_CASE("Underground tube power draft refuses a stale commissioned endpoint", "[transport-tube-controls-stale]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	TubePowerRig rig;
	TubeControlsTestUI ui(_current_company);
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr);
	window->OnPaint();
	window->OnClick({}, WID_UC_TUBE_ROUTE, 1);
	window->OnPlaceObject({}, rig.portal);
	window->OnPlaceObject({}, rig.path[0]);
	StageLoadedLayerPortalWork({}); AcceptLoadedLayerPortalWork(); // Explicit lost-reference fault; this never commissions or excavates gameplay terrain.
	const auto ledger = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money;
	window->OnPlaceObject({}, rig.depot);
	window->OnPaint();
	CHECK(window->IsWidgetDisabled(WID_UC_TUBE_CONNECT));
	CHECK(window->GetWidgetString(WID_UC_TUBE_STATUS, STR_TUBE_HELP).find("commissioned") != std::string::npos);
	CHECK(_thd.GetCallbackWnd() != window);
	CHECK(CaptureTubePowerConnections().empty());
	CHECK(ComputeUtilityPowerDigest() == ledger);
	CHECK(Company::Get(_current_company)->money == cash);
}
