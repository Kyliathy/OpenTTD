/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file layer_construction_controls.cpp Native minimap navigation and refused construction overlays.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md view-independent simulation.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group24.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "underground_view_fixture.h"
#include "underground_workflow_fixture.h"
#include "../smallmap_gui.h"
#include "../map_layer_gui.h"
#include "../underground_gui.h"
#include "../construction_preview.h"
#include "../tbm_area_preview.h"
#include "../core/random_func.hpp"
#include "../widgets/smallmap_widget.h"
#include "../widgets/underground_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Close the actual small map before its main viewport, company and world disappear. */
class LayerMapControlsUI final : public UndergroundToolbarTestUI {
public:
	/** @param company Real local actor; all map actions remain read-only for simulation. */
	explicit LayerMapControlsUI(CompanyID company) : UndergroundToolbarTestUI(company) {}
	/** Release map overlays while their original viewport/world owners still exist. */
	~LayerMapControlsUI()
	{
		CloseWindowByClass(WindowClass::SmallMap);
		Window::DeleteClosedWindows();
	}
};

/**
 * Pick the actual map panel's centre through its native click handler.
 * @param map Real small map whose displayed layer must determine the navigation target.
 */
static void PickSmallMapCentre(Window &map)
{
	const auto *panel = map.GetWidget<NWidgetBase>(WID_SM_MAP);
	map.OnClick({panel->pos_x + static_cast<int>(panel->current_x / 2), panel->pos_y + static_cast<int>(panel->current_y / 2)}, WID_SM_MAP, 1);
}

/** Main changes follow every physical layer; independent map/extra-view choices never advance simulation. */
TEST_CASE("Small map follows main layers and retains independent Space navigation", "[layer-construction-controls]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture world;
	LayerMapControlsUI ui(_current_company);
	Window *main = new UndergroundTestViewport();
	Window *extra = new UndergroundTestViewport(_underground_extra_view_desc);
	BuildIndustriesLegend();
	BuildLinkStatsLegend();
	ShowSmallMap();
	Window *map = FindWindowById(WindowClass::SmallMap, 0);
	REQUIRE(map != nullptr);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto projects = ComputeTBMProjectDigest();
	const auto money = Company::Get(_current_company)->money;
	const std::array random{_random.state[0], _random.state[1]};
	// Exercise native main-view notifications and map picking, rather than checking a duplicate UI layer variable.
	for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground, MapLayer::Space, MapLayer::Surface}) {
		SetViewportLayer(main, layer);
		map->ProcessScheduledInvalidations();
		CHECK(map->GetWidgetString(WID_SM_CAPTION, STR_SMALLMAP_CAPTION_WITH_LAYER).find(GetString(GetMapLayerName(layer))) != std::string::npos);
		PickSmallMapCentre(*map);
		CHECK(main->viewport->layer == layer);
	}
	map->OnDropdownSelect(WID_SM_MAP_LAYER, to_underlying(MapLayer::Space), 0);
	CHECK(main->viewport->layer == MapLayer::Surface);
	SetViewportLayer(extra, MapLayer::DeepUnderground);
	map->ProcessScheduledInvalidations();
	PickSmallMapCentre(*map);
	CHECK(main->viewport->layer == MapLayer::Space);
	CHECK(extra->viewport->layer == MapLayer::DeepUnderground);
	// Centring explicitly rejoins the main view, even after the map independently previews Space.
	SetViewportLayer(main, MapLayer::Underground);
	map->ProcessScheduledInvalidations();
	map->OnDropdownSelect(WID_SM_MAP_LAYER, to_underlying(MapLayer::Space), 0);
	map->OnClick({}, WID_SM_CENTERMAP, 1);
	PickSmallMapCentre(*map);
	CHECK(main->viewport->layer == MapLayer::Underground);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeTBMProjectDigest() == projects);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK((std::array{_random.state[0], _random.state[1]} == random));
}

/** Auto-height worlds rebuild their contour legend from real terrain; another map mode cannot reserve its rows. */
TEST_CASE("Small map sizes only its selected legend and resolves auto terrain heights", "[layer-construction-controls]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture world;
	LayerMapControlsUI ui(_current_company);
	new UndergroundTestViewport();
	AutoRestoreBackup limit(_settings_game.construction.map_height_limit, uint8_t{0});
	BuildIndustriesLegend();
	BuildLinkStatsLegend();
	const TileIndex peak = TileXYL(32, 32, MapLayer::DeepUnderground);
	SetTileHeight(peak, 200);
	ShowSmallMap();
	Window *map = FindWindowById(WindowClass::SmallMap, 0);
	REQUIRE(map != nullptr);
	map->OnClick({}, WID_SM_CONTOUR, 1);
	const uint contour_rows = map->GetWidget<NWidgetBase>(WID_SM_LEGEND)->current_y;
	const uint legend_width = map->GetWidget<NWidgetBase>(WID_SM_LEGEND)->current_x;
	map->OnClick({}, WID_SM_VEHICLES, 1);
	CHECK(map->GetWidget<NWidgetBase>(WID_SM_LEGEND)->current_y < contour_rows);
	CHECK(map->GetWidget<NWidgetBase>(WID_SM_LEGEND)->current_x == legend_width);
	CloseWindowByClass(WindowClass::SmallMap);
	Window::DeleteClosedWindows();
	// A second world/window can have the same auto setting and lower terrain; the setting alone is not a cache key.
	SetTileHeight(peak, 0);
	ShowSmallMap();
	map = FindWindowById(WindowClass::SmallMap, 0);
	REQUIRE(map != nullptr);
	map->OnClick({}, WID_SM_CONTOUR, 1);
	CHECK(map->GetWidget<NWidgetBase>(WID_SM_LEGEND)->current_y < contour_rows);
	CHECK(_settings_game.construction.map_height_limit == 0);
}

/** Both rock-plane palettes quote the portal's actual default consist even while the main camera stays on Space. */
TEST_CASE("Construction palette uses the real portal default on each rock plane", "[layer-construction-controls]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const MapLayer layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground);
	UndergroundWorkflowWorld world(layer);
	LayerMapControlsUI ui(_current_company);
	Window *main = new UndergroundTestViewport();
	SetViewportLayer(main, MapLayer::Space);
	Window *tools = ShowUndergroundConstructionToolbar(layer, GetCurrentMultiLayerProfileSelection());
	REQUIRE(tools != nullptr);
	const uint16_t model = GetLayerPortalDefaultTBMModel(world.portal);
	const auto consist = GetDefaultTBMConsist(layer);
	const auto delta = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(world.rock));
	const TileIndex end = TileAddWrap(world.rock, 6 * delta.x, 6 * delta.y);
	const auto quote = PreviewTBMCommissionArea(_current_company, world.portal, model, consist.earth_wagons, consist.rock_wagons,
		world.origin->index, world.rock, end, false, ExcavationPlanIntent::StraightTunnel);
	REQUIRE(quote.purchase.Succeeded());
	const auto money = Company::Get(_current_company)->money;
	const auto projects = ComputeTBMProjectDigest();
	const auto vehicles = Vehicle::GetNumItems();
	tools->OnClick({}, WID_UC_AREA, 1);
	tools->OnDropdownSelect(WID_UC_INTENT, to_underlying(ExcavationPlanIntent::StraightTunnel), 0);
	tools->OnPlaceObject({}, world.rock);
	tools->OnPlaceObject({}, end);
	tools->OnPaint();
	CHECK_FALSE(tools->IsWidgetDisabled(WID_UC_AREA_CONFIRM));
	CHECK(tools->GetWidgetString(WID_UC_AREA_PRICE, STR_UNDERGROUND_AREA_EMPTY) ==
		GetString(STR_UNDERGROUND_AREA_PRICE, quote.purchase.GetCost(), quote.area.totals.cost));
	CHECK(GetConstructionPreviewRouteLabel(world.rock).find('!') == std::string_view::npos);
	CHECK(main->viewport->layer == MapLayer::Space);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(ComputeTBMProjectDigest() == projects);
	CHECK(Vehicle::GetNumItems() == vehicles);
	ResetObjectToPlace();
}

/** A refused real quote exposes its owner's blocked tile, and changing/cancelling the active tool clears that marker. */
TEST_CASE("Construction preview marks real refused cells and respects cursor ownership", "[layer-construction-controls]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const MapLayer layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground);
	UndergroundWorkflowWorld world(layer);
	LayerMapControlsUI ui(_current_company);
	Window *main = new UndergroundTestViewport();
	SetViewportLayer(main, layer);
	Window *tools = ShowUndergroundConstructionToolbar(layer, GetCurrentMultiLayerProfileSelection());
	REQUIRE(tools != nullptr);
	const auto consist = GetDefaultTBMConsist(layer);
	const auto model = GetLayerPortalDefaultTBMModel(world.portal);
	CHECK(model == (layer == MapLayer::Underground ? 0 : 1));
	const auto delta = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(world.rock));
	const TileIndex end = TileAddWrap(world.rock, 6 * delta.x, 6 * delta.y);
	const TileIndex depot = TileAddWrap(world.rock, 2 * delta.x, 2 * delta.y);
	std::vector<TileIndex> cavity;
	// Only the pre-existing depot's physical footprint is open; this scenario does not excavate the requested tunnel.
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) cavity.push_back(TileAddWrap(depot, x, y));
	}
	world.Open(cavity);
	REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, depot, RAILTYPE_RAIL, DiagDirection::SE).Succeeded());
	const auto quote = PreviewTBMCommissionArea(_current_company, world.portal, model, consist.earth_wagons, consist.rock_wagons,
		world.origin->index, world.rock, end, false, ExcavationPlanIntent::StraightTunnel);
	REQUIRE(quote.purchase.Failed());
	const TileIndex blocked = IsValidTile(quote.area.failure_tile) ? quote.area.failure_tile : quote.area.plan.failure_tile;
	REQUIRE(IsValidTile(blocked));
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto projects = ComputeTBMProjectDigest();
	const auto money = Company::Get(_current_company)->money;
	const std::array random{_random.state[0], _random.state[1]};
	tools->OnClick({}, WID_UC_AREA, 1);
	tools->OnDropdownSelect(WID_UC_INTENT, to_underlying(ExcavationPlanIntent::StraightTunnel), 0);
	tools->OnPlaceObject({}, world.rock);
	tools->OnPlaceObject({}, end);
	tools->OnPaint();
	CHECK(tools->IsWidgetDisabled(WID_UC_AREA_CONFIRM));
	CHECK(GetConstructionPreviewTile(blocked) == ConstructionPreviewTile::Uncovered);
	CHECK(GetConstructionPreviewRouteLabel(blocked).find('!') != std::string_view::npos);
	CHECK(GetConstructionPreviewTile(MultiLayerMap::TileInLayer(blocked, MapLayer::Surface)) == ConstructionPreviewTile::None);
	// A background window must not replace the active plan's geometry, even when it supplies a valid blocked tile.
	SetConstructionPreview(main, {}, {}, {}, {}, depot);
	CHECK(GetConstructionPreviewRouteLabel(blocked).find('!') != std::string_view::npos);
	SetViewportLayer(main, MapLayer::Space);
	CHECK(GetConstructionPreviewTile(blocked) == ConstructionPreviewTile::None);
	CHECK(GetConstructionPreviewRouteLabel(blocked).empty());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeTBMProjectDigest() == projects);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK((std::array{_random.state[0], _random.state[1]} == random));
}
