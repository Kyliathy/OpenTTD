/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_workflow.cpp Connected visible excavation and mine freight workflow using real controls and native movement. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Groups 9B-9D */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "underground_workflow_fixture.h"
#include "underground_view_fixture.h"
#include "../underground_gui.h"
#include "../vehicle_gui.h"
#include "../gui.h"
#include "../map_layer_gui.h"
#include "../widgets/underground_widget.h"
#include "../widgets/layer_portal_widget.h"
#include "../widgets/vehicle_widget.h"
#include "../widgets/order_widget.h"
#include "../widgets/industry_widget.h"
#include "../widgets/road_widget.h"
#include "../widgets/depot_widget.h"
#include "../widgets/build_vehicle_widget.h"
#include "../widgets/toolbar_widget.h"
#include "../toolbar_gui.h"
#include "../tbm_area_preview.h"
#include "../road_map.h"
#include "../roadveh.h"
#include "../industry_map.h"
#include "../safeguards.h"

/** Real workflow windows are closed while their vehicle, station, industry and map owners remain alive. */
class UndergroundWorkflowUI : public UndergroundToolbarTestUI {
public:
	/** @param company Actual funded scenario player. */
	explicit UndergroundWorkflowUI(CompanyID company) : UndergroundToolbarTestUI(company) {}
	/** Close all workflow child families and cancel unfinished cursor ownership before scenario teardown. */
	~UndergroundWorkflowUI()
	{
		ResetObjectToPlace();
		// Every opened workflow family may retain actual world references; close them before the native pools are retired.
		for (WindowClass family : {WindowClass::LayerPortal, WindowClass::VehicleView, WindowClass::VehicleOrders,
			WindowClass::VehicleRefit, WindowClass::BuildVehicle, WindowClass::BuildToolbar, WindowClass::ScenarioBuildToolbar, WindowClass::BuildDepot,
			WindowClass::BuildTruckStation, WindowClass::VehicleDepot, WindowClass::BuildIndustry, WindowClass::ErrorMessage, WindowClass::DropdownMenu}) CloseWindowByClass(family);
		Window::DeleteClosedWindows();
	}
};

/** @param window Actual visible control owner. @param widget Enabled widget to click. */
static void ClickWorkflowControl(Window *window, WidgetID widget)
{
	INFO("requested widget=" << widget);
	REQUIRE(window != nullptr);
	INFO("window=" << to_underlying(window->window_class) << " widget=" << widget);
	window->OnPaint();
	REQUIRE_FALSE(window->IsWidgetDisabled(widget));
	const auto *control = window->GetWidget<NWidgetBase>(widget);
	window->OnClick({static_cast<int>(control->pos_x + control->current_x / 2), static_cast<int>(control->pos_y + control->current_y / 2)}, widget, 1);
}

/** @param vehicle Actual bought vehicle whose normal orders window is opened. @param tile Visible station/depot destination. @param view Actual main/extra viewport used to select that destination. */
static void AddWorkflowOrder(const Vehicle *vehicle, TileIndex tile, Window *view)
{
	// Camera changes cancel active placement; select the destination's plane before arming the orders cursor.
	SetViewportLayer(view, GetTileLayer(tile));
	ShowOrdersWindow(vehicle);
	Window *orders = FindWindowById(WindowClass::VehicleOrders, vehicle->index);
	REQUIRE(orders != nullptr);
	// Keep the user's orders window beside the viewport so native cursor picking sees the world, not overlapping window chrome.
	orders->left = 750;
	// A new empty orders window can already arm Go To; clicking that active button would cancel the player's placement mode.
	if (!orders->IsWidgetLowered(WID_O_GOTO)) ClickWorkflowControl(orders, WID_O_GOTO);
	REQUIRE(_thd.GetCallbackWnd() == orders);
	ClickUndergroundWorldTile(view, tile);
	ResetObjectToPlace();
}

/**
 * Select a real enabled funding row through its visible matrix and place it through the active viewport.
 * @param view Main/extra world-picking owner.
 * @param type Actual native or original industry definition.
 * @param tile Actual visible funding anchor.
 * @return Newly command-created industry; no tiles or reserve are supplied by this helper.
 */
static Industry *FundWorkflowIndustry(Window *view, IndustryType type, TileIndex tile)
{
	const auto count = Industry::GetNumItems();
	ShowBuildIndustryWindow();
	Window *fund = FindWindowById(WindowClass::BuildIndustry, 0);
	REQUIRE(fund != nullptr);
	std::vector<IndustryType> offered;
	// Follow the public funding policy to locate its displayed row; the actual window still owns availability and command admission.
	for (IndustryType candidate : _sorted_industry_types) {
		const auto *spec = GetIndustrySpec(candidate);
		if (!spec->enabled) continue;
		// Native mines remain fundable independently of the ordinary raw-industry setting.
		if (_game_mode != GameMode::Editor && spec->IsRawIndustry() && _settings_game.construction.raw_industry_construction == 0 && GetLayerIndustryCapability(candidate) == nullptr) continue;
		offered.push_back(candidate);
	}
	const auto it = std::ranges::find(offered, type);
	REQUIRE(it != offered.end());
	const size_t row = it - offered.begin();
	Scrollbar *scroll = fund->GetScrollbar(WID_DPI_SCROLLBAR);
	scroll->ScrollTowards(static_cast<Scrollbar::size_type>(row));
	const auto *matrix = fund->GetWidget<NWidgetBase>(WID_DPI_MATRIX_WIDGET);
	fund->OnClick({static_cast<int>(matrix->pos_x + matrix->current_x / 2),
		static_cast<int>(matrix->pos_y + (row - scroll->GetPosition()) * matrix->resize_y + matrix->resize_y / 2)}, WID_DPI_MATRIX_WIDGET, 1);
	ClickWorkflowControl(fund, WID_DPI_FUND_WIDGET);
	SetViewportLayer(view, GetTileLayer(tile));
	ClickUndergroundWorldTile(view, tile);
	INFO("funding " << type << " at " << TileX(tile) << ',' << TilePlaneY(tile));
	// A read-only command quote makes a visible refusal diagnosable without replacing the player's funding action.
	const auto quote = IsLayerMineDefinition(type) ? CmdBuildLayerMine({}, tile, type, 0, GetGeologyWorldState().deposits.Find(tile), 1234) : CmdBuildIndustry({}, tile, type, 0, false, 1234);
	INFO("funding quote=" << quote.GetErrorMessage().base());
	REQUIRE(Industry::GetNumItems() == count + 1);
	Industry *industry = nullptr;
	// Original layouts can leave their anchor empty; resolve the genuine newly built location rather than treating every layout as a filled rectangle.
	for (Industry *candidate : Industry::Iterate()) {
		if (candidate->type == type && candidate->location.tile == tile) industry = candidate;
	}
	REQUIRE(industry != nullptr);
	return industry;
}

/** @param view Actual picking viewport. @param road Active ordinary road toolbar. @param tile Terminal anchor. @param door Actual orientation widget. @return Command-created owned truck terminal. */
static Station *BuildWorkflowTerminal(Window *view, Window *road, TileIndex tile, WidgetID door)
{
	SetViewportLayer(view, GetTileLayer(tile));
	ClickWorkflowControl(road, WID_ROT_TRUCK_STATION);
	ClickWorkflowControl(FindWindowById(WindowClass::BuildTruckStation, TransportType::Road), door);
	ClickUndergroundWorldTile(view, tile);
	road->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_BUILD_TRUCKSTOP, {}, tile, tile);
	REQUIRE(IsTileType(tile, TileType::Station));
	return Station::GetByTile(tile);
}

/** @param view Actual picking viewport. @param road Active road toolbar. @param start First road cell. @param end Last road cell, on the same world layer and X coordinate. */
static void BuildWorkflowRoad(Window *view, Window *road, TileIndex start, TileIndex end)
{
	SetViewportLayer(view, GetTileLayer(start));
	ClickWorkflowControl(road, WID_ROT_ROAD_Y);
	ClickUndergroundWorldTile(view, start, {4, 4});
	road->OnPlaceDrag(VPM_FIX_X, DDSP_PLACE_ROAD_Y_DIR, {static_cast<int>(TileX(end) * TILE_SIZE), static_cast<int>(TileY(end) * TILE_SIZE + 15)});
	road->OnPlaceMouseUp(VPM_FIX_X, DDSP_PLACE_ROAD_Y_DIR, {}, start, end);
	REQUIRE(IsTileType(end, TileType::Road));
	ResetObjectToPlace();
}

/** @param carrier Actual purchased depot vehicle. @param cargo Requested registry meaning selected through the visible, sorted refit list. */
static void RefitWorkflowCarrier(RoadVehicle *carrier, CargoType cargo)
{
	ShowVehicleViewWindow(carrier);
	ClickWorkflowControl(FindWindowById(WindowClass::VehicleView, carrier->index), WID_VV_REFIT);
	Window *refit = FindWindowById(WindowClass::VehicleRefit, carrier->index);
	REQUIRE(refit != nullptr);
	const auto *capability = GetUndergroundEngineCapability(carrier->engine_type);
	std::vector<CargoType> choices;
	// Public cargo presentation order determines visible rows; the actual refit window and command retain capability authority.
	for (CargoLabel label : capability->cargo_labels) choices.push_back(GetCargoTypeByLabel(label));
	std::ranges::sort(choices, CargoTypeComparator{});
	const auto choice = std::ranges::find(choices, cargo);
	REQUIRE(choice != choices.end());
	const size_t row = choice - choices.begin();
	Scrollbar *scroll = refit->GetScrollbar(WID_VR_SCROLLBAR);
	scroll->ScrollTowards(static_cast<Scrollbar::size_type>(row));
	const auto *matrix = refit->GetWidget<NWidgetBase>(WID_VR_MATRIX);
	refit->OnClick({static_cast<int>(matrix->pos_x + matrix->current_x / 2), static_cast<int>(matrix->pos_y + (row - scroll->GetPosition()) * matrix->resize_y + matrix->resize_y / 2)}, WID_VR_MATRIX, 1);
	ClickWorkflowControl(refit, WID_VR_REFIT);
	REQUIRE(carrier->cargo_type == cargo);
}

/**
 * Complete visible survey/commission/exchange and physically deliver its actual spoil in the caller's scenario.
 * @param world Accepted generated scenario; its target floor is still solid.
 * @param view Actual main or extra viewport used for world picking.
 * @param wide Whether this scenario has explicitly enabled sharp rail turns for a wider chamber.
 * @return Actual completed project identity; the same world continues into mine freight acceptance.
 */
static TBMProjectID RunVisibleExcavation(UndergroundWorkflowWorld &world, Window *view, bool wide = false)
{
	SetViewportLayer(view, world.layer);
	Window *tools = ShowUndergroundConstructionToolbar(world.layer, GetCurrentMultiLayerProfileSelection());
	tools->left = 750;
	const auto untouched = ComputeGeologyStateDigest(GetGeologyWorldState());
	ClickWorkflowControl(tools, WID_UC_SURVEY);
	ClickUndergroundWorldTile(view, world.site);
	CHECK(tools->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL).find("Solid rock") != std::string::npos);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == untouched);
	ClickWorkflowControl(tools, WID_UC_AREA);
	ClickUndergroundWorldTile(view, world.rock);
	const TileIndex end = TileAddWrap(world.site, wide ? 4 : 2, 8);
	const TileIndex start = ClickUndergroundWorldTile(view, world.site);
	tools->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, start, end);
	INFO("generated mine site=" << TileX(world.site) << ',' << TilePlaneY(world.site));
	INFO(tools->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL));
	const auto consist = GetDefaultTBMConsist(world.layer);
	const auto diagnostic = PreviewTBMCommissionArea(_current_company, world.portal, GetLayerPortalDefaultTBMModel(world.portal), consist.earth_wagons, consist.rock_wagons, world.origin->index, world.site, end, false);
	INFO("quote stage=" << static_cast<uint>(to_underlying(diagnostic.area.error)) << " at=" << diagnostic.area.failure_tile.base()
		<< " plan=" << static_cast<uint>(to_underlying(diagnostic.area.plan.error)) << " plan tile=" << diagnostic.area.plan.failure_tile.base());
	ClickWorkflowControl(tools, WID_UC_AREA_CONFIRM);
	REQUIRE(CaptureTBMProjects().size() == 1);
	const TBMProjectID project = CaptureTBMProjects().front().id;
	const VehicleID cutter = GetTBMProject(project)->cutter;
	ClickWorkflowControl(tools, WID_UC_PROJECT);
	ClickUndergroundWorldTile(view, world.rock);
	Window *portal = FindWindowById(WindowClass::LayerPortal, world.portal);
	REQUIRE(portal != nullptr);
	ClickWorkflowControl(portal, PW_ASSIGN);
	REQUIRE(portal->OnVehicleSelect(world.hauler));
	REQUIRE(GetTBMExchangeAssignmentForHauler(world.hauler->index) != nullptr);
	AddWorkflowOrder(world.hauler, world.origin->xy, view);
	REQUIRE(world.hauler->GetNumOrders() == 1);
	ShowVehicleViewWindow(world.hauler);
	ClickWorkflowControl(FindWindowById(WindowClass::VehicleView, world.hauler->index), WID_VV_START_STOP);
	ClickWorkflowControl(portal, PW_START_STOP);
	REQUIRE_FALSE(world.hauler->vehstatus.Test(VehState::Stopped));
	REQUIRE_FALSE(Train::Get(cutter)->vehstatus.Test(VehState::Stopped));
	uint attempts = 0;
	// Run only actual native owners until excavation and its final physical exchange/delivery finish; never force phases or empty wagons.
	while (attempts < 100000) {
		world.Tick();
		++attempts;
		const auto *record = GetTBMProject(project);
		const auto *assignment = GetTBMExchangeAssignmentForHauler(world.hauler->index);
		// Completion requires actual final disposal as well as digging; a waiting, still-loaded hauler cannot close this workflow.
		if (IsTBMProjectFinished(record->phase) && assignment->phase == TBMHaulerPhase::ReturnEmpties && assignment->exchanges != 0) break;
	}
	INFO("ticks=" << attempts << " phase=" << to_underlying(GetTBMProject(project)->phase) << " hold=" << to_underlying(GetTBMProject(project)->blocked));
	INFO("cutter=" << DescribeTrain(Train::Get(cutter)) << " hauler=" << DescribeTrain(world.hauler));
	const auto *job = GetExcavationJob(GetTBMProject(project)->job);
	REQUIRE(attempts < 100000);
	CHECK(GetGeologyWorldState().terrain.Get(world.site) == ExcavationState::Excavated);
	CHECK(GetGeologyWorldState().terrain.Get(end) == ExcavationState::Excavated);
	CHECK(GetTBMExchangeAssignmentForHauler(world.hauler->index)->exchanges > 0);
	CHECK_FALSE(Train::Get(cutter)->vehstatus.Test(VehState::Crashed));
	const auto *earth = GetSpoilReceiver(world.earth->index);
	const auto *stone = GetSpoilReceiver(world.stone->index);
	REQUIRE(earth != nullptr);
	REQUIRE(stone != nullptr);
	CHECK(earth->disposed_t + world.earth->accepted.front().waiting == job->completed_earth_t);
	CHECK(stone->disposed_t + stone->crushed_t + world.stone->accepted.front().waiting == job->completed_rock_t);
	// Final delivery must empty actual cutter/hauler packets, not merely advance the controller's phase.
	for (const Vehicle *vehicle : Vehicle::Iterate()) CHECK(vehicle->cargo.TotalCount() == 0);
	return project;
}

/** Actual UI commissioning, project assignment and native driving must excavate the selected seam and deliver every spoil tonne. */
TEST_CASE("Visible underground excavation and exchange produce the mine floor", "[underground-workflow-excavation]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world;
	bool wide = false;
	SECTION("Straight mine corridor with ordinary default rail rules") {}
	SECTION("Wide chamber with sharp rail turns enabled") { wide = true; }
	AutoRestoreBackup<bool> turns(_settings_game.pf.forbid_90_deg, !wide);
	UndergroundWorkflowUI ui(_current_company);
	RunVisibleExcavation(world, new UndergroundTestViewport(), wide);
}

/**
 * Continue the caller's actual excavation through visible facility/fleet controls and physical mineral delivery.
 * @param world Same accepted world whose machine has just completed digging and spoil disposal.
 * @param view Actual main/extra picking owner for every construction and order destination.
 */
static void RunVisibleMineFreight(UndergroundWorkflowWorld &world, Window *view)
{
	SetViewportLayer(view, world.layer);
	Window *portal = FindWindowById(WindowClass::LayerPortal, world.portal);
	ClickWorkflowControl(portal, PW_RELEASE);
	ClickWorkflowControl(portal, PW_START_STOP);
	ClickWorkflowControl(portal, PW_RETIRE);
	ClickWorkflowControl(portal, PW_SELL);
	ClickWorkflowControl(portal, PW_CONVERT);
	CHECK(CaptureTBMProjects().empty());
	Window *tools = ShowUndergroundConstructionToolbar(world.layer, GetCurrentMultiLayerProfileSelection());
	ClickWorkflowControl(tools, WID_UC_ROAD);
	Window *road = FindWindowById(WindowClass::BuildToolbar, TransportType::Road);
	ClickWorkflowControl(road, WID_ROT_DEMOLISH);
	ClickUndergroundWorldTile(view, world.site);
	road->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, world.site, TileAddWrap(world.site, 2, 2));
	ClickWorkflowControl(tools, WID_UC_MINE);
	Industry *mine = FundWorkflowIndustry(view, GetLayerMineIndustryType(world.resource), world.site);
	REQUIRE(GetLayerMineState(mine->index) != nullptr);
	Station *loading = BuildWorkflowTerminal(view, road, TileAddWrap(world.site, 2, 4), WID_BROS_STATION_SE);
	const TileIndex depot = TileAddWrap(world.site, 1, 6);
	ClickWorkflowControl(road, WID_ROT_DEMOLISH);
	ClickUndergroundWorldTile(view, depot);
	road->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, depot, depot);
	BuildWorkflowRoad(view, road, TileAddWrap(world.site, 2, 5), TileAddWrap(world.site, 2, 7));
	ClickWorkflowControl(tools, WID_UC_ROAD_SERVICE);
	ClickWorkflowControl(FindWindowById(WindowClass::BuildDepot, TransportType::Road), WID_BROD_DEPOT_SW);
	ClickUndergroundWorldTile(view, depot);
	REQUIRE(IsRoadDepotTile(depot));
	const TileIndex tunnel = TileAddWrap(world.site, 2, 8);
	ClickWorkflowControl(road, world.layer == MapLayer::Underground ? WID_ROT_LAYER_TUNNEL : WID_ROT_LAYER_ELEVATOR);
	Window *picker = FindWindowById(WindowClass::BuildDepot, TransportType::Road);
	ClickWorkflowControl(picker, WID_BREL_DOOR_NW);
	if (world.layer == MapLayer::DeepUnderground) ClickWorkflowControl(picker, WID_BREL_UPPER_SURFACE);
	ClickUndergroundWorldTile(view, tunnel);
	REQUIRE(IsLayerPortalTile(tunnel));
	const TileIndex surface = GetLayerPortalPairTile(tunnel);
	CHECK(GetTileLayer(surface) == MapLayer::Surface);
	const TileIndex unload = TileAddWrap(surface, 0, 4);
	Station *receiving = BuildWorkflowTerminal(view, road, unload, WID_BROS_STATION_NW);
	BuildWorkflowRoad(view, road, TileAddWrap(surface, 0, 1), TileAddWrap(surface, 0, 3));
	ShowLayerPortalWindow(tunnel);
	INFO("connection=" << static_cast<uint>(to_underlying(ChangeLayerPortalConnection(tunnel, _current_company, true, false)))
		<< " rock door=" << TileX(GetLayerPortalDoorTile(tunnel)) << ',' << TilePlaneY(GetLayerPortalDoorTile(tunnel))
		<< " surface door=" << TileX(GetLayerPortalDoorTile(surface)) << ',' << TilePlaneY(GetLayerPortalDoorTile(surface)));
	ClickWorkflowControl(FindWindowById(WindowClass::LayerPortal, *GetLayerPortalPairKey(tunnel)), PW_CONNECT);
	const auto receiver_type = world.layer == MapLayer::Underground ? GetLayerBaselineReceiverType(CT_COAL) : GetLayerProcessorIndustryType(MultiLayerRecipeID{"ELEC"});
	Industry *receiver = FundWorkflowIndustry(view, receiver_type, TileAddWrap(unload, 2, -1));
	REQUIRE_FALSE(receiver->accepted.empty());
	receiving->RecomputeCatchment();
	UpdateStationAcceptance(receiving, false);
	const CargoType cargo = GetCargoTypeByLabel(world.resource);
	REQUIRE(receiving->goods[cargo].status.Test(GoodsEntry::State::Acceptance));
	SetViewportLayer(view, world.layer);
	ResetObjectToPlace();
	ClickUndergroundWorldTile(view, depot);
	Window *depot_window = FindWindowById(WindowClass::VehicleDepot, depot);
	ClickWorkflowControl(depot_window, WID_D_BUILD);
	Window *buy = FindWindowById(WindowClass::BuildVehicle, depot);
	buy->OnDropdownSelect(WID_BV_CARGO_FILTER_DROPDOWN, cargo, 0);
	buy->OnPaint();
	ClickWorkflowControl(buy, WID_BV_BUILD);
	RoadVehicle *carrier = nullptr;
	// Locate the actual purchased road vehicle; existing rail logistics remain separate native pool entries.
	for (RoadVehicle *vehicle : RoadVehicle::Iterate()) { if (vehicle->IsPrimaryVehicle()) carrier = vehicle; }
	REQUIRE(carrier != nullptr);
	CHECK(carrier->engine_type == GetUndergroundEngine(world.layer == MapLayer::Underground ? 7 : 8));
	RefitWorkflowCarrier(carrier, GetCargoTypeByLabel(world.layer == MapLayer::Underground ? CargoLabel{"ERTH"} : CargoLabel{"URAN"}));
	RefitWorkflowCarrier(carrier, cargo);
	AddWorkflowOrder(carrier, loading->xy, view);
	AddWorkflowOrder(carrier, receiving->xy, view);
	REQUIRE(carrier->GetNumOrders() == 2);
	ClickWorkflowControl(FindWindowById(WindowClass::VehicleView, carrier->index), WID_VV_START_STOP);
	const auto deposit = GetLayerMineState(mine->index)->deposit;
	const uint64_t reserve = GetGeologyWorldState().deposits.Get(deposit)->remaining_reserve;
	world.simulate_industries = true;
	AutoRestoreBackup<PauseModes> unpaused(_pause_mode, PauseModes{});
	uint attempts = 0;
	const IndustryID receiver_id = receiver->index;
	const Company *company = Company::Get(_current_company);
	// Real industry construction/production and road movement must deliver actual packets; no prepared arrivals or cargo injection are permitted.
	while (attempts < 40000) {
		world.Tick();
		++attempts;
		// Receipt and empty actual packets must agree before closing the trip; a partly unloaded carrier cannot count as complete delivery.
		if (company->cur_economy.delivered_cargo[cargo] > 0 && carrier->cargo.TotalCount() == 0) break;
	}
	REQUIRE(Industry::GetIfValid(receiver_id) == receiver);
	INFO("freight ticks=" << attempts << " carrier=" << TileX(carrier->tile) << ',' << TilePlaneY(carrier->tile) << " cargo=" << carrier->cargo.TotalCount());
	INFO("truck speed=" << carrier->cur_speed << " stopped=" << carrier->vehstatus.Test(VehState::Stopped) << " depot=" << carrier->IsInDepot()
		<< " destination=" << carrier->dest_tile.base() << " order=" << static_cast<uint>(to_underlying(carrier->current_order.GetType())));
	INFO("mine status=" << to_underlying(GetLayerMineOperationalStatus(*mine)) << " extracted=" << GetLayerMineState(mine->index)->extracted);
	REQUIRE(attempts < 40000);
	const auto *state = GetLayerMineState(mine->index);
	const auto input = receiver->GetCargoAccepted(cargo);
	REQUIRE(input != receiver->accepted.end());
	const auto delivered = input->GetOrCreateHistory()[THIS_MONTH].accepted;
	CHECK(reserve - GetGeologyWorldState().deposits.Get(deposit)->remaining_reserve == state->extracted);
	CHECK(state->extracted == mine->produced.front().waiting + state->dispatched);
	// The ordinary power station consumes intake immediately; its actual accepted history accounts for delivery independently of waiting stock.
	CHECK(state->dispatched == loading->goods[cargo].TotalCount() + carrier->cargo.TotalCount() + delivered);
	CHECK(delivered > 0);
	CHECK(delivered == company->cur_economy.delivered_cargo[cargo]);
	CHECK(GetTileLayer(carrier->tile) == MapLayer::Surface);
}

/** The complete visible excavation and mine freight route must share one world and account for every extracted unit. */
TEST_CASE("Visible mine freight continues on the actual excavated floor", "[underground-workflow-freight]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world;
	UndergroundWorkflowUI ui(_current_company);
	Window *view = new UndergroundTestViewport();
	AllocateToolbar();
	RunVisibleExcavation(world, view);
	RunVisibleMineFreight(world, view);
}

/** Each functional context completes its own connected native excavation/transport route, including both immutable rock-plane identities. */
TEST_CASE("Connected underground workflow survives every viewport toolbar and keyboard context", "[underground-workflow-context]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const MapLayer layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground);
	const bool extra = GENERATE(false, true);
	const bool split = GENERATE(false, true);
	const bool keyboard = GENERATE(false, true);
	INFO("layer=" << static_cast<uint>(to_underlying(layer)) << " extra=" << extra << " split=" << split << " keyboard=" << keyboard);
	UndergroundWorkflowWorld world(layer);
	UndergroundWorkflowUI ui(_current_company);
	Window *main = new UndergroundTestViewport();
	SetViewportLayer(main, layer);
	// One-pixel mock icons require a fourteen-pixel toolbar to select the native split arrangement; world picking still uses a normal screen.
	if (split) { _screen.width = 14; ScreenSizeChanged(); }
	AllocateToolbar();
	Window *toolbar = FindWindowById(WindowClass::MainToolbar, 0);
	REQUIRE(toolbar != nullptr);
	if (split) {
		ResizeWindow(toolbar, 14 - toolbar->width, 0, false, false);
		if (!toolbar->IsWidgetLowered(WID_TN_SWITCH_BAR)) ClickWorkflowControl(toolbar, WID_TN_SWITCH_BAR);
		// The toolbar retains its selected row across lifetimes; use its visible feedback to reach the construction row through actual toggles.
		if (!toolbar->IsWidgetLowered(WID_TN_SWITCH_BAR)) ClickWorkflowControl(toolbar, WID_TN_SWITCH_BAR);
		CHECK(toolbar->IsWidgetLowered(WID_TN_SWITCH_BAR));
	}
	toolbar->OnPaint();
	CHECK(toolbar->GetWidget<NWidgetCore>(WID_TN_AIR)->GetToolTip() == STR_UNDERGROUND_MENU);
	if (keyboard) {
		CHECK(toolbar->OnHotkey(MTHK_BUILD_AIRPORT) == EventState::Handled);
	} else {
		ClickWorkflowControl(toolbar, WID_TN_AIR);
		toolbar->OnDropdownSelect(WID_TN_AIR, 100 + to_underlying(layer), -1);
		CloseWindowByClass(WindowClass::DropdownMenu);
	}
	REQUIRE(FindWindowById(WindowClass::UndergroundConstruction, layer) != nullptr);
	_screen.width = 1280;
	ScreenSizeChanged();
	Window *view = main;
	if (extra) {
		view = new UndergroundTestViewport(_underground_extra_view_desc);
		SetViewportLayer(main, MapLayer::Surface);
	}
	RunVisibleExcavation(world, view);
	RunVisibleMineFreight(world, view);
	if (extra) CHECK(main->viewport->layer == MapLayer::Surface);
}

/** Editor authoring keeps company fleet/claims disabled, builds an ordinary road on real open floor, and returns the same world to normal freight. */
TEST_CASE("Editor underground authoring preserves restrictions and normal freight continuation", "[underground-workflow-editor]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const MapLayer layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground);
	UndergroundWorkflowWorld world(layer);
	UndergroundWorkflowUI ui(_current_company);
	Window *view = new UndergroundTestViewport();
	SetViewportLayer(view, layer);
	AllocateToolbar();
	RunVisibleExcavation(world, view);
	CloseWindowByClass(WindowClass::MainToolbar);
	CloseWindowByClass(WindowClass::UndergroundConstruction);
	Window::DeleteClosedWindows();
	{
		AutoRestoreBackup<GameMode> mode(_game_mode, GameMode::Editor);
		AutoRestoreBackup<CompanyID> actor(_current_company, OWNER_NONE);
		AutoRestoreBackup<CompanyID> local(_local_company, COMPANY_SPECTATOR);
		world.SetEditorOverride(true);
		SetViewportLayer(view, layer);
		AllocateToolbar();
		Window *toolbar = FindWindowById(WindowClass::MainToolbar, 0);
		toolbar->OnPaint();
		CHECK(toolbar->GetWidget<NWidgetCore>(WID_TE_INDUSTRY)->GetToolTip() == STR_UNDERGROUND_MENU);
		ClickWorkflowControl(toolbar, WID_TE_INDUSTRY);
		Window *tools = FindWindowById(WindowClass::UndergroundConstruction, layer);
		REQUIRE(tools != nullptr);
		tools->OnPaint();
		CHECK(tools->IsWidgetDisabled(WID_UC_MINE));
		CHECK(tools->IsWidgetDisabled(WID_UC_AREA));
		CHECK(tools->IsWidgetDisabled(WID_UC_ROAD_TERMINAL));
		CHECK(tools->IsWidgetDisabled(WID_UC_ROAD_SERVICE));
		const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
		const auto industries = Industry::GetNumItems();
		const auto vehicles = Vehicle::GetNumItems();
		CHECK(CmdBuildLayerMine({}, world.site, GetLayerMineIndustryType(world.resource), 0, GetGeologyWorldState().deposits.Find(world.site), 1234).Failed());
		CHECK(CmdBuildLayerMine(DoCommandFlag::Execute, world.site, GetLayerMineIndustryType(world.resource), 0, GetGeologyWorldState().deposits.Find(world.site), 1234).Failed());
		ClickWorkflowControl(tools, WID_UC_SURVEY);
		ClickUndergroundWorldTile(view, world.site);
		CHECK(tools->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL).find("Excavated") != std::string::npos);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
		ClickWorkflowControl(tools, WID_UC_ROAD);
		Window *road = FindWindowById(WindowClass::ScenarioBuildToolbar, TransportType::Road);
		BuildWorkflowRoad(view, road, TileAddWrap(world.site, 0, 5), TileAddWrap(world.site, 0, 7));
		CHECK(GetRoadOwner(TileAddWrap(world.site, 0, 6), RoadTramType::Road) == OWNER_NONE);
		CHECK(Industry::GetNumItems() == industries);
		CHECK(Vehicle::GetNumItems() == vehicles);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
		CloseWindowByClass(WindowClass::MainToolbar);
		CloseWindowByClass(WindowClass::BuildToolbar);
		CloseWindowByClass(WindowClass::ScenarioBuildToolbar);
		CloseWindowByClass(WindowClass::UndergroundConstruction);
		Window::DeleteClosedWindows();
	}
	AllocateToolbar();
	CHECK(GetCurrentMultiLayerProfileSelection().editor_override);
	// The saved authoring flag cannot grant free excavation to normal company play; the later route still consumes only the actual dug floor.
	const TileIndex solid = TileAddWrap(world.site, 5, 5);
	CHECK(GetGeologyWorldState().terrain.Get(solid) == ExcavationState::Solid);
	CHECK(CheckExcavationConstruction(std::array{solid}).Failed());
	RunVisibleMineFreight(world, view);
}

/** An impossible physical bend is a visible pre-purchase refusal, without any partial machine, job, terrain or money mutation. */
TEST_CASE("Visible chamber quote refuses rail turns forbidden by the current game rule", "[underground-workflow-turn-refusal]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world;
	UndergroundWorkflowUI ui(_current_company);
	Window *tools = ShowUndergroundConstructionToolbar(world.layer, GetCurrentMultiLayerProfileSelection());
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money;
	const auto vehicles = Vehicle::GetNumItems();
	ClickWorkflowControl(tools, WID_UC_AREA);
	tools->OnPlaceObject({}, world.rock);
	tools->OnPlaceObject({}, world.site);
	const auto end = TileAddWrap(world.site, 4, 8);
	tools->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, world.site, end);
	tools->OnPaint();
	CHECK(tools->IsWidgetDisabled(WID_UC_AREA_CONFIRM));
	CHECK(tools->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL).find("90-degree") != std::string::npos);
	const auto model = GetLayerPortalDefaultTBMModel(world.portal);
	const auto estimate = CmdCommissionTBM({}, world.portal, model, 2, 2, world.origin->index, world.site, end, false);
	const auto execute = CmdCommissionTBM(DoCommandFlag::Execute, world.portal, model, 2, 2, world.origin->index, world.site, end, false);
	CHECK(estimate.GetErrorMessage() == STR_ERROR_TBM_COMMISSION_RAIL_TURNS);
	CHECK(execute.GetErrorMessage() == estimate.GetErrorMessage());
	CHECK(CaptureTBMProjects().empty());
	CHECK(Vehicle::GetNumItems() == vehicles);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
}
