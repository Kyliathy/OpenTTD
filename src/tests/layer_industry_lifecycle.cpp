/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_industry_lifecycle.cpp Real viewport, stopped/abandoned material recovery and stock-preserving recipe control. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 16C */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "mock_environment.h"
#include "../map_layer_gui.h"
#include "../viewport_type.h"
#include "../vehicle_base.h"
#include "mine_production_fixture.h"
#include "processor_fixture.h"

#include "../safeguards.h"

static const NWidgetPart _mine_test_viewport_widgets[] = {
	NWidget(NWID_VIEWPORT, Colours::Invalid, 0), SetMinimalSize(320, 200), SetResize(1, 1),
};
static WindowDesc _mine_test_viewport_desc(
	WindowPosition::Manual, {}, 320, 200, WindowClass::ExtraViewport, WindowClass::None, {}, _mine_test_viewport_widgets);

/** Real viewport owner exercises public layer selection without loading raster assets or creating a separate simulated layer. */
class MineTestViewport : public Window {
public:
	/** Initialize an actual viewport over the existing fixture map. */
	MineTestViewport() : Window(_mine_test_viewport_desc)
	{
		this->InitNested(0);
		this->GetWidget<NWidgetViewport>(0)->InitializeViewport(this, TileXY(32, 32), ZoomLevel::Viewport);
	}
};

/** Rendering another plane cannot stop extraction, leak station supply or run inherited Surface mine effects on Deep tiles. */
TEST_CASE("A hidden Deep mine runs identically across real viewport layer changes", "[layer-industry-lifecycle]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	MineProductionRig rig(5);
	LayerWindowTestUI ui(WindowClass::ExtraViewport, _current_company);
	Window *view = new MineTestViewport();
	const uint64_t original = rig.Reserve();
	const auto vehicles = Vehicle::GetNumItems();
	Station *surface = rig.StationAt(TileXYL(TileX(rig.request.tile), TilePlaneY(rig.request.tile), MapLayer::Surface));
	CHECK_FALSE(rig.industry->stations_near.contains(surface));
	// Each selected camera plane receives exactly one frozen work interval through the real economy dispatcher.
	for (MapLayer layer : {MapLayer::Surface, MapLayer::DeepUnderground, MapLayer::Space}) {
		SetViewportLayer(view, layer);
		REQUIRE(view->viewport->layer == layer);
		const uint64_t before = rig.Reserve();
		for (uint32_t tick = 0; tick < 74; ++tick) OnTick_Industry();
		CHECK(before - rig.Reserve() == 4);
		_tile_type_procs[TileType::Industry]->tile_loop_proc(rig.request.tile);
		CHECK(Vehicle::GetNumItems() == vehicles);
		CHECK(surface->goods[rig.industry->produced[0].cargo].TotalCount() == 0);
	}
	CHECK(original - rig.Reserve() == 12);
	CHECK(rig.industry->produced[0].waiting == 12);
	CHECK(GetLayerMineState(rig.industry->index)->extracted == 12);
}

/** Abandoned stock can leave through a successor's real same-layer station, then whole-site rebuild preserves the original finite debit. */
TEST_CASE("Abandoned mine material recovers before an unchanged-reserve rebuild", "[layer-industry-lifecycle]")
{
	MineProductionRig rig;
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	const uint64_t remaining = rig.Reserve();
	const uint64_t initial = GetGeologyWorldState().deposits.Get(rig.request.deposit)->initial_reserve;
	ChangeLayerIndustryOwnership(rig.request.company, CompanyID::Invalid());
	REQUIRE(GetLayerMineState(rig.industry->index)->phase == LayerMinePhase::Abandoned);
	REQUIRE(AcceptLoadedLayerMines());
	CHECK(ProduceLayerMineCargo(*rig.industry, 74, true).error == LayerIndustryError::InvalidCompany);
	CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Failed());
	REQUIRE(Company::CanAllocateItem());
	std::unique_ptr<Company> successor(Company::Create());
	successor->money = 1000000;
	successor->clear_limit = 64U << 16;
	AutoRestoreBackup<CompanyID> actor(_current_company, successor->index);
	Station *station = rig.StationAt(TileAddWrap(rig.request.tile, 3, 0));
	GoodsEntry &goods = station->goods[rig.industry->produced[0].cargo];
	goods.rating = UINT8_MAX;
	goods.last_speed = 100;
	rig.industry->exclusive_consumer = rig.request.company;
	CHECK(DispatchLayerMineCargo(*rig.industry) == 0); // Abandonment releases mining control, not an independently selected script transport restriction.
	CHECK(rig.industry->produced[0].waiting == 4);
	rig.industry->exclusive_consumer = INVALID_OWNER;
	REQUIRE(DispatchLayerMineCargo(*rig.industry) == 4);
	CHECK(goods.TotalCount() == 4);
	CHECK(rig.industry->produced[0].waiting == 0);
	CHECK(rig.Reserve() == remaining);
	const auto geology = CaptureGeologyState().packed;
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Succeeded());
	rig.industry = nullptr;
	CHECK(Industry::GetNumItems() == 0);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->claimant == CompanyID::Invalid());
	rig.request.company = successor->index;
	REQUIRE(rig.Fund(DoCommandFlag::Execute).Succeeded());
	rig.industry = *Industry::Iterate().begin();
	CHECK(GetLayerMineState(rig.industry->index)->company == successor->index);
	CHECK(GetLayerMineState(rig.industry->index)->extracted == 0);
	CHECK(rig.Reserve() == remaining);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->initial_reserve == initial);
	CHECK(CaptureGeologyState().packed == geology);
	CHECK(goods.TotalCount() == 4);
	delete rig.industry; // Fixture cleanup releases the new empty claim while its real successor company still exists.
	rig.industry = nullptr;
}

/** Frozen work/output survive explicit closure, and reopening cannot duplicate extraction from an already earned interval. */
TEST_CASE("Stopped mine machinery preserves work reserve and existing output", "[layer-industry-lifecycle]")
{
	MineProductionRig rig;
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	const auto saved = CaptureLayerMines();
	const auto reserve = rig.Reserve();
	rig.industry->prod_level = PRODLEVEL_CLOSURE;
	// Both tick production and actual machinery loops must hold extraction while a stopped industry's stock remains recoverable.
	for (uint32_t tick = 0; tick < 74; ++tick) {
		OnTick_Industry();
		_tile_type_procs[TileType::Industry]->tile_loop_proc(rig.request.tile);
	}
	CHECK(CaptureLayerMines() == saved);
	CHECK(rig.Reserve() == reserve);
	CHECK(rig.industry->produced[0].waiting == 4);
	CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Failed());
	rig.industry->prod_level = PRODLEVEL_DEFAULT;
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	CHECK(rig.industry->produced[0].waiting == 8);
	CHECK(reserve - rig.Reserve() == 4);
}

/** A successor can explicitly recover abandoned recipe control; quotes/deity actions preserve physical stocks and grant no implicit company. */
TEST_CASE("Processor controller recovery preserves every delivered alternative", "[layer-industry-lifecycle]")
{
	ProcessorRig rig;
	REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(CargoLabel{"CORE"}), 4) == 4);
	ChangeLayerIndustryOwnership(_current_company, CompanyID::Invalid());
	REQUIRE(GetLayerProcessorState(rig.industry->index)->controller == CompanyID::Invalid());
	REQUIRE(AcceptLoadedLayerProcessors());
	REQUIRE(Company::CanAllocateItem());
	std::unique_ptr<Company> successor(Company::Create());
	const auto saved = CaptureLayerProcessors();
	CHECK(SelectLayerProcessorRecipe(*rig.industry, MultiLayerRecipeID{"BFMT"}, successor->index, false));
	CHECK(CaptureLayerProcessors() == saved);
	REQUIRE(SelectLayerProcessorRecipe(*rig.industry, MultiLayerRecipeID{"BFMT"}, successor->index, true));
	CHECK(GetLayerProcessorState(rig.industry->index)->controller == successor->index);
	CHECK(rig.Input(CargoLabel{"CORE"}).waiting == 4);
	CHECK(rig.Output(CargoLabel{"RFMT"}).waiting == 0);
	const auto controlled = CaptureLayerProcessors();
	RegisterLayerProcessorIndustry(*rig.industry);
	CHECK(CaptureLayerProcessors() == controlled); // Repeated creation/activation hooks cannot restore the old founder's control or zero history.
	CHECK_FALSE(SelectLayerProcessorRecipe(*rig.industry, MultiLayerRecipeID{"CFMT"}, _current_company, true));
	ChangeLayerProcessorControl(successor->index, CompanyID::Invalid());
	REQUIRE(SelectLayerProcessorRecipe(*rig.industry, MultiLayerRecipeID{"CFMT"}, OWNER_DEITY, true));
	CHECK(GetLayerProcessorState(rig.industry->index)->controller == CompanyID::Invalid());
	CHECK(rig.Input(CargoLabel{"CORE"}).waiting == 4);
	CHECK(AcceptLoadedLayerProcessors());
}

/** Scenario founding history can name deity while saved recipe control remains an optional real company reference. */
TEST_CASE("Deity founded processor initialization does not save a nonexistent company", "[layer-industry-lifecycle]")
{
	ProcessorRig rig;
	ReleaseLayerProcessorIndustry(rig.industry->index); // The fixture is still empty; exercise the new-industry registration boundary with actual deity founder metadata.
	AutoRestoreBackup<Owner> founder(rig.industry->founder, OWNER_DEITY);
	RegisterLayerProcessorIndustry(*rig.industry);
	CHECK(GetLayerProcessorState(rig.industry->index)->controller == CompanyID::Invalid());
	CHECK(GetLayerProcessorState(rig.industry->index)->recipe == MultiLayerRecipeID{"CFMT"});
	CHECK(AcceptLoadedLayerProcessors());
}
