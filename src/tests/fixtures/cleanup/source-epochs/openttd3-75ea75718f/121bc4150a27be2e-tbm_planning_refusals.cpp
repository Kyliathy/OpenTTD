/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_planning_refusals.cpp Both-layer reviewed-intent refusals and visible admission parity. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group3C; docs/zz-reach2/architecture/gui/archi-gui.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "../tbm_area_preview.h"
#include "../underground_gui.h"
#include "../map_layer_gui.h"
#include "../tilehighlight_func.h"
#include "../widgets/underground_widget.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/** Refused layer, calendar, physical occupancy or cargo capacity cannot partially publish a reviewed job or purchase. */
TEST_CASE("Reviewed TBM intents refuse invalid work atomically on both rock layers", "[tbm-planning-refusal]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const MapLayer layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground);
	const ExcavationPlanIntent intent = GENERATE(ExcavationPlanIntent::Area, ExcavationPlanIntent::StraightTunnel);
	UndergroundWorkflowWorld world(layer);
	LayerWindowTestUI ui(WindowClass::UndergroundConstruction, _current_company);
	const auto model = GetLayerPortalDefaultTBMModel(world.portal);
	auto consist = GetDefaultTBMConsist(layer);
	const auto delta = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(world.rock));
	const TileIndex start = intent == ExcavationPlanIntent::StraightTunnel ? world.rock : TileAddWrap(world.rock, delta.x, delta.y);
	TileIndex end = TileAddWrap(world.rock, 6 * delta.x, 6 * delta.y);
	const auto original = PreviewTBMCommissionArea(_current_company, world.portal, model, consist.earth_wagons, consist.rock_wagons,
		world.origin->index, start, end, false, intent);
	REQUIRE(original.purchase.Succeeded());
	AutoRestoreBackup date(TimerGameCalendar::date);
	bool capacity = false;
	SECTION("another layer") { end = MultiLayerMap::TileInLayer(end, MapLayer::Surface); }
	SECTION("unavailable calendar") {
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{layer == MapLayer::DeepUnderground ? 1979 : 1949}, 11, 31);
	}
	SECTION("an existing depot in the cutter sweep") {
		std::vector<TileIndex> floor;
		const TileIndex depot = TileAddWrap(world.rock, 2 * delta.x, 2 * delta.y);
		// This rejection fixture declares a pre-existing cavity, then builds an
		// actual depot there. The proposed cutter cannot silently demolish it.
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) floor.push_back(TileAddWrap(depot, x, y));
		}
		world.Open(floor);
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, depot, RAILTYPE_RAIL, DiagDirection::SE).Succeeded());
	}
	SECTION("too little independent Rock capacity") {
		consist.rock_wagons = 1;
		capacity = true;
		// Reduced Soft output fits one hopper. Declare solid Hard rock around
		// the Underground starter so a real first cut exceeds Rock capacity;
		// Deep already has Hard rock. This creates no floor or cargo.
		if (layer == MapLayer::Underground) {
			auto snapshot = CaptureGeologyState();
			GeologyDeposit patch;
			patch.id = GeologyDepositID{0};
			patch.layer = layer;
			patch.resource = world.resource;
			patch.hardness = GeologyHardness::Hard;
			patch.initial_reserve = patch.remaining_reserve = 4900;
			for (uint y = TilePlaneY(world.rock) - 3; y <= TilePlaneY(world.rock) + 3; ++y) {
				patch.spans.push_back({static_cast<uint16_t>(y), static_cast<uint16_t>(TileX(world.rock) - 3), static_cast<uint16_t>(TileX(world.rock) + 3)});
			}
			snapshot.deposits = {patch};
			snapshot.next_deposit_id = 1;
			GeologyWorldState restored;
			REQUIRE(PrepareGeologyRestoration(snapshot, restored) == GeologyError::None);
			StageLoadedGeologyState(std::move(restored));
			AcceptLoadedGeologyState();
		}
	}
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto projects = ComputeTBMProjectDigest();
	const auto portals = ComputeLayerPortalWorkDigest();
	const auto money = Company::Get(_current_company)->money;
	const auto vehicles = Vehicle::GetNumItems();
	const std::array random{_random.state[0], _random.state[1]};
	const auto refused = PreviewTBMCommissionArea(_current_company, world.portal, model, consist.earth_wagons, consist.rock_wagons,
		world.origin->index, start, end, false, intent);
	REQUIRE(refused.purchase.Failed());
	if (capacity) CHECK(refused.purchase.GetErrorMessage() == STR_ERROR_TBM_COMMISSION_CONSIST);
	// Both estimate and execution must revalidate current participants before
	// the old reviewed token can reserve cells, charge money or buy machinery.
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::ConfirmTBMCommission>::Do(flags, world.portal, model, consist.earth_wagons, consist.rock_wagons,
			world.origin->index, start, end, original.fingerprint, intent).Failed());
	}
	if (!capacity) {
		// The palette exposes location/date/occupancy inputs. Hopper counts are
		// fixed defaults there; malformed script counts are checked above.
		Window *tools = ShowUndergroundConstructionToolbar(layer, GetCurrentMultiLayerProfileSelection());
		REQUIRE(tools != nullptr);
		tools->OnPaint();
		if (!tools->IsWidgetDisabled(WID_UC_AREA)) {
			tools->OnClick({}, WID_UC_AREA, 1);
			tools->OnDropdownSelect(WID_UC_INTENT, to_underlying(intent), 0);
			tools->OnPlaceObject({}, world.rock);
			if (intent == ExcavationPlanIntent::StraightTunnel) tools->OnPlaceObject({}, end);
			else tools->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, start, end);
		}
		tools->OnPaint();
		CHECK(tools->IsWidgetDisabled(WID_UC_AREA_CONFIRM));
		ResetObjectToPlace();
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeTBMProjectDigest() == projects);
	CHECK(ComputeLayerPortalWorkDigest() == portals);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(Vehicle::GetNumItems() == vehicles);
	CHECK((std::array{_random.state[0], _random.state[1]} == random));
}
