/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_status_presentation.cpp What the TBM windows say: the wait line, the carried cargo, the release
 * requirement, New area on a busy portal and refused quotes.
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 4 (R4, R6, R8; D3, D5, D7)
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md task 9.6 (the refused requests stay impossible)
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md tasks 10.2-10.5 (the approach row, its numbering and refusal reasons)
 * @see docs/zz-reach2/architecture/gui/archi-gui.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "tbm_fixture.h"
#include "excavation_claim_fixture.h"
#include "tbm_commission_fixture.h"
#include "underground_view_fixture.h"
#include "../underground_gui.h"
#include "../tbm_gui.h"
#include "../tbm_controls.h"
#include "../tbm_area_preview.h"
#include "../tbm_replanning.h"
#include "../construction_preview.h"
#include "../tilehighlight_func.h"
#include "../viewport_func.h"
#include "../widgets/underground_widget.h"
#include "../widgets/tbm_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/**
 * Claim the last cell of a request for another project's work, so no route may open the whole request. Group 9 made
 * the demo's beside-exit request itself legal: the planner now adds its approach and turning space.
 * @param c Commissioning rig whose geology owns the cell.
 * @param last Last cell of a row running SW (+x); the claiming job starts two cells before it.
 */
static void ClaimLastCell(TBMCommissionRig &c, TileIndex last)
{
	ClaimSeparateExcavationWork(c.rig.fixture.world, TileAddWrap(last, -2, 0));
	REQUIRE(GetGeologyWorldState().job_claims.contains(last));
}

/** R4/D3: the wait line names the wait, says the machine works, or says nothing holds a stored machine; never "Held". */
TEST_CASE("TBM wait line says what the machine waits for", "[demo-fix-tbm-status]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	CHECK(GetTBMWaitLine(TBMPhase::Bore, TBMBlockedReason::None) == GetString(STR_PORTAL_PROJECT_NO_HOLD));
	CHECK(GetTBMWaitLine(TBMPhase::Bore, TBMBlockedReason::None).find("Working normally") != std::string::npos);
	// A returning machine is still at work; a parked, finished or order-following one waits for nothing.
	CHECK(GetTBMWaitLine(TBMPhase::ReturnForExchange, TBMBlockedReason::None) == GetString(STR_PORTAL_PROJECT_NO_HOLD));
	// Each nonworking phase must read as idle rather than imply that cutting continues.
	for (TBMPhase phase : {TBMPhase::Parked, TBMPhase::ParkedHangar, TBMPhase::Completed, TBMPhase::FollowOrders}) {
		CHECK(GetTBMWaitLine(phase, TBMBlockedReason::None) == GetString(STR_PORTAL_PROJECT_IDLE));
	}
	const std::string full = GetTBMWaitLine(TBMPhase::Bore, TBMBlockedReason::EarthFull);
	CHECK(full == GetString(STR_PORTAL_PROJECT_HOLD, STR_PORTAL_HOLD_EARTH_FULL));
	CHECK(full.find("Waiting: the Earth hoppers are full") != std::string::npos);
	CHECK(GetTBMWaitLine(TBMPhase::Bore, TBMBlockedReason::Paused).find("Waiting: the project is paused") != std::string::npos);
	// The old "Held:" wording, which the demo read as "carrying nothing", is gone from every line.
	// Every displayed state uses explicit activity/wait wording, avoiding the ambiguous legacy Held label.
	for (TBMBlockedReason reason : {TBMBlockedReason::None, TBMBlockedReason::Obstruction, TBMBlockedReason::Stopped}) {
		CHECK(GetTBMWaitLine(TBMPhase::Bore, reason).find("Held") == std::string::npos);
	}
}

/** R4: the projects list shows the spoil the cutter actually carries, and the hauler line, in rows tall enough for them. */
TEST_CASE("TBM projects list shows the carried spoil", "[demo-fix-tbm-status]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMWorkRig rig(3);
	LayerWindowTestUI browser_ui(WindowClass::TBMProjects, _current_company);
	const auto earth = rig.Wagons(TBMConsistRole::EarthWagon);
	REQUIRE_FALSE(earth.empty());
	rig.Preload(earth.front(), 10);
	const TBMSpoilLoad spoil = SumTBMCutterSpoil(*rig.front, rig.Project());
	CHECK(spoil.earth_loaded_t == 10);
	CHECK(spoil.rock_loaded_t == 0);
	CHECK(spoil.earth_capacity_t > 0);
	CHECK(spoil.rock_capacity_t > 0);
	CHECK(spoil.earth_hoppers == earth.size());
	// The per-cargo sum used for haulers agrees with the per-role sum over the same train.
	const TBMSpoilLoad by_cargo = SumTrainSpoil(*rig.front);
	CHECK(by_cargo.earth_loaded_t == spoil.earth_loaded_t);
	CHECK(by_cargo.earth_capacity_t == spoil.earth_capacity_t);
	CHECK(by_cargo.rock_capacity_t == spoil.rock_capacity_t);
	// The portal window's attached-wagon lines read the same helper.
	const TBMPairControls controls = GetTBMPairControls(rig.Project().portal, _current_company);
	CHECK(controls.earth_loaded_t == spoil.earth_loaded_t);
	CHECK(controls.rock_capacity_t == spoil.rock_capacity_t);

	const auto lines = GetTBMProjectCargoLines(rig.Project());
	REQUIRE(lines.size() == 2);
	CHECK(lines[0] == GetString(STR_TBM_PROJECT_LIST_CARRYING, spoil.earth_loaded_t, spoil.earth_capacity_t, spoil.rock_loaded_t, spoil.rock_capacity_t));
	CHECK(lines[1] == GetString(STR_TBM_PROJECT_LIST_NO_HAULER));

	// Each row holds the name, two progress lines, two wait lines, the cutter line and one hauler line.
	Window *browser = ShowTBMProjectsWindow(MapLayer::Underground);
	REQUIRE(browser != nullptr);
	browser->OnPaint();
	CHECK(browser->GetWidget<NWidgetCore>(WID_TP_LIST)->resize_y == 7u * GetCharacterHeight(FontSize::Normal) + WidgetDimensions::scaled.vsep_normal);
}

/** R6: the release requirement is labelled as the requirement of releasing, not of every action. */
TEST_CASE("TBM release requirement is labelled as such", "[demo-fix-tbm-status]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMWorkRig rig(3);
	const StringID reason = GetTBMProjectReleaseProblem(rig.Project().id);
	REQUIRE(reason == STR_ERROR_TBM_RELEASE_RETURN);
	const std::string line = GetTBMProjectReleaseLine(rig.Project().id);
	CHECK(line == GetString(STR_TBM_PROJECT_RELEASE_REQUIREMENT, GetString(reason)));
	// The requirement keeps its own text (and colour code, which the list overrides) after the label.
	CHECK(line.starts_with(GetString(STR_TBM_PROJECT_RELEASE_READY).substr(0, line.find(':') + 1)));
	CHECK(line.find("To finish and release:") != std::string::npos);
	CHECK(line.find("Return and park the whole machine first") != std::string::npos);
}

/** R6/D5 (task 4.4): New area on the portal of the company's own working project opens its Pause and extend flow. */
TEST_CASE("New area on a busy portal plans more work for its project", "[demo-fix-tbm-status]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMWorkRig rig(3);
	UndergroundToolbarTestUI ui(_current_company);
	new UndergroundTestViewport(); // Planning more work centres the main view on the route's end.
	const TBMProject project = rig.Project();
	REQUIRE(CanPlanTBMProjectExtension(project));
	REQUIRE(project.blocked != TBMBlockedReason::Paused);
	// Finished, parking and stored projects cannot take more work; New area keeps its refusal for them.
	for (TBMPhase phase : {TBMPhase::Completed, TBMPhase::Cancelled, TBMPhase::ReturnToPark, TBMPhase::Parked, TBMPhase::ReturnToRelease}) {
		TBMProject other = project;
		other.phase = phase;
		CHECK_FALSE(CanPlanTBMProjectExtension(other));
	}

	Window *palette = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(palette != nullptr);
	palette->OnPaint();
	// The player's game runs; the fixture's pause would make the posted Pause command wait for an unpause.
	AutoRestoreBackup running(_pause_mode, PauseModes{});
	palette->OnClick({}, WID_UC_AREA, 1);
	palette->OnPlaceObject({}, rig.Job().commissioning_anchor);
	CHECK(GetTBMProject(project.id)->blocked == TBMBlockedReason::Paused);
	CHECK(_thd.GetCallbackWnd() == palette);
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_TBM_EXTEND_SELECT_FLOOR, project.id.base()));
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) != GetString(STR_UNDERGROUND_PORTAL_HAS_PROJECT));
	ResetObjectToPlace();
}

/** R8 (task 4.5): a refused quote hides the price, material and time rows and names and marks where planning stopped. */
TEST_CASE("Refused TBM quotes show where planning stopped", "[demo-fix-tbm-status]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMCommissionRig c;
	LayerWindowTestUI ui(WindowClass::UndergroundConstruction, _current_company);
	Window *tools = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(tools != nullptr);
	tools->OnPaint();
	// The demo's request: a row of floor beside the door, sideways from the first cell outside it, whose last cell another
	// project has claimed, so that no route can open the whole row.
	const auto ahead = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(c.rock));
	const TileIndexDiffC side{ahead.y, ahead.x};
	const TileIndex first = c.start;
	const TileIndex last = TileAddWrap(c.start, 7 * side.x, 7 * side.y);
	REQUIRE(IsValidTile(last));
	REQUIRE(side.x == 1); // The rig's door faces SE, so the row runs SW (+x).
	ClaimLastCell(c, last);
	tools->OnClick({}, WID_UC_AREA, 1);
	tools->OnPlaceObject({}, c.rock);
	tools->OnPlaceObject({}, first);
	tools->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, first, last);
	tools->OnPaint();
	const auto quote = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, c.rig.fixture.origin->index, first, last, false);
	REQUIRE(quote.purchase.Failed());
	CHECK(tools->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(quote.purchase.GetErrorMessage()));
	CHECK(tools->IsWidgetDisabled(WID_UC_AREA_CONFIRM));

	// No "£0" lines: the price, material and time rows are hidden and empty.
	CHECK(tools->GetWidget<NWidgetStacked>(WID_UC_QUOTE_COST_STACK)->shown_plane == SZSP_NONE);
	CHECK(tools->GetWidgetString(WID_UC_AREA_PRICE, STR_UNDERGROUND_AREA_EMPTY).empty());
	CHECK(tools->GetWidgetString(WID_UC_AREA_MATERIAL, STR_UNDERGROUND_AREA_EMPTY).empty());
	CHECK(tools->GetWidgetString(WID_UC_AREA_TIME, STR_UNDERGROUND_AREA_EMPTY).empty());

	// The floor row names the stopping tile and the reachable part; the map marks the tile red with "!".
	const TileIndex failure = IsValidTile(quote.area.failure_tile) ? quote.area.failure_tile : quote.area.plan.failure_tile;
	const size_t reachable = quote.area.plan.covered_targets.size();
	const size_t total = reachable + quote.area.plan.remaining_targets.size();
	REQUIRE(total > 0);
	INFO("failure=" << failure.base() << " reachable=" << reachable << " total=" << total);
	if (IsValidTile(failure)) {
		CHECK(tools->GetWidgetString(WID_UC_AREA_FLOOR, STR_UNDERGROUND_AREA_EMPTY) == GetString(STR_UNDERGROUND_AREA_REFUSED_AT, TileX(failure), TilePlaneY(failure), reachable, total));
		CHECK(GetConstructionPreviewTile(failure) == ConstructionPreviewTile::Uncovered);
		CHECK(GetConstructionPreviewRouteLabel(failure).find('!') != std::string_view::npos);
	} else {
		CHECK(tools->GetWidgetString(WID_UC_AREA_FLOOR, STR_UNDERGROUND_AREA_EMPTY) == GetString(STR_UNDERGROUND_AREA_REFUSED, reachable, total));
	}

	// An accepted quote shows the rows again, with solid cells and cutter positions counted separately.
	tools->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, c.start, c.end);
	tools->OnPaint();
	const auto accepted = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, c.rig.fixture.origin->index, c.start, c.end, false);
	REQUIRE(accepted.purchase.Succeeded());
	CHECK(tools->GetWidget<NWidgetStacked>(WID_UC_QUOTE_COST_STACK)->shown_plane == 0);
	CHECK(tools->GetWidgetString(WID_UC_AREA_FLOOR, STR_UNDERGROUND_AREA_EMPTY) == GetString(STR_UNDERGROUND_AREA_FLOOR, accepted.area.plan.covered_targets.size(),
		accepted.area.turn_envelope.size(), accepted.area.totals.new_cells, accepted.area.plan.route.size(), accepted.area.plan.remaining_targets.size()));
	ResetObjectToPlace();
}

/** Task 4.5: the extension flow reports a planner refusal with the commissioning flow's message, not as partial cover. */
TEST_CASE("Extension refusals read as in the commissioning quote", "[demo-fix-tbm-status]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	// A project commissioned through the real command, with spoil receivers, so an extension reaches the planner.
	TBMCommissionRig c;
	REQUIRE(c.Commission(DoCommandFlag::Execute).Succeeded());
	const TBMProjectID id = CaptureTBMProjects().back().id;
	UndergroundToolbarTestUI ui(_current_company);
	new UndergroundTestViewport(); // Planning more work centres the main view on the route's end.
	AutoRestoreBackup running(_pause_mode, PauseModes{}); // As above: the posted Pause needs a running game.
	REQUIRE(PauseAndPlanTBMProjectExtension(id));
	Window *palette = FindWindowById(WindowClass::UndergroundConstruction, MapLayer::Underground);
	REQUIRE(palette != nullptr);
	// The demo's request again: a row of floor sideways, starting at the cell after the accepted route's end, whose last cell
	// another project has claimed, so that no route can open the whole row.
	const auto &end = GetExcavationJob(GetTBMProject(id)->job)->route.back();
	const auto ahead = TileIndexDiffCByDir(end.heading);
	const TileIndex first = TileAddWrap(end.cutter_position, ahead.x, ahead.y);
	const TileIndexDiffC side{ahead.y, ahead.x};
	const TileIndex last = TileAddWrap(first, 7 * side.x, 7 * side.y);
	REQUIRE(IsValidTile(first));
	REQUIRE(IsValidTile(last));
	REQUIRE(side.x == 1); // The rig's door faces SE, so the row runs SW (+x).
	ClaimLastCell(c, last);
	palette->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, first, last);
	palette->OnPaint();
	const auto quote = PreviewTBMExtension(_current_company, id, first, last, ExcavationPlanIntent::Area);
	INFO("error=" << static_cast<int>(quote.plan.error) << " partial=" << quote.plan.partial << " remaining=" << quote.plan.remaining_targets.size() << " message=" << GetString(quote.result.GetErrorMessage()));
	REQUIRE(quote.result.Failed());
	// Before task 4.5 this planner refusal, which leaves floor uncovered, read "can cover only part of this area".
	REQUIRE(quote.plan.error != ExcavationError::None);
	REQUIRE_FALSE(quote.plan.remaining_targets.empty());
	// Since task 10.4 the refusal names its reason: the claimed cell is another project's work.
	CHECK(quote.result.GetErrorMessage() == STR_ERROR_TBM_APPROACH_OTHER_WORK);
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_ERROR_TBM_APPROACH_OTHER_WORK));
	CHECK(palette->GetWidget<NWidgetStacked>(WID_UC_QUOTE_COST_STACK)->shown_plane == SZSP_NONE);
	CHECK(palette->GetWidgetString(WID_UC_AREA_PRICE, STR_UNDERGROUND_AREA_EMPTY).empty());
	ResetObjectToPlace();
}

/**
 * R8 (tasks 10.2, 10.3 and 10.5): the demo's beside-exit request is accepted, and the quote shows its approach on a line
 * of its own. The numbered cutter positions run on through the approach, which is shaded as working space; a request
 * that needs no approach shows no such line.
 */
TEST_CASE("The quote shows the approach on its own line and numbers the cutter through it", "[demo-fix-approach-gui]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMCommissionRig c;
	LayerWindowTestUI ui(WindowClass::UndergroundConstruction, _current_company);
	Window *tools = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(tools != nullptr);
	tools->OnPaint();
	// The demo's request: a row of floor beside the door, sideways from the first cell outside it.
	const auto ahead = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(c.rock));
	const TileIndexDiffC side{ahead.y, ahead.x};
	const TileIndex first = c.start;
	const TileIndex last = TileAddWrap(c.start, 7 * side.x, 7 * side.y);
	tools->OnClick({}, WID_UC_AREA, 1);
	tools->OnPlaceObject({}, c.rock);
	tools->OnPlaceObject({}, first);
	tools->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, first, last);
	tools->OnPaint();
	const auto quote = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, c.rig.fixture.origin->index, first, last, false);
	REQUIRE(quote.purchase.Succeeded());
	const auto &approach = quote.area.approach;
	REQUIRE_FALSE(approach.tiles.empty());
	CHECK(approach.cuts > 0);
	CHECK_FALSE(tools->IsWidgetDisabled(WID_UC_AREA_CONFIRM));

	// Its own line, apart from the totals that already include it.
	CHECK(tools->GetWidget<NWidgetStacked>(WID_UC_APPROACH_STACK)->shown_plane == 0);
	CHECK(tools->GetWidgetString(WID_UC_AREA_APPROACH, STR_UNDERGROUND_AREA_EMPTY) == GetString(STR_UNDERGROUND_AREA_APPROACH, approach.tiles.size(), approach.estimate.cost,
		approach.estimate.earth_t, approach.estimate.rock_t, approach.cuts));
	CHECK(approach.estimate.cost <= quote.area.totals.cost);

	// The cutter positions are numbered in order through the approach, and the approach is shaded working space.
	const auto &route = quote.area.plan.route;
	// Number every canonical visit, including approach turns and revisits, without generating another route.
	for (size_t index = 0; index < route.size(); ++index) {
		const std::string label{GetConstructionPreviewRouteLabel(route[index].cutter_position)};
		CHECK(label.find(fmt::format("{}", index + 1)) != std::string::npos);
	}
	// The approach remains additional working space; it is never presented as promised final-use floor.
	for (TileIndex tile : approach.tiles) CHECK(GetConstructionPreviewTile(tile) == ConstructionPreviewTile::WorkingSpace);
	// At least one numbered position stands outside the floor and its ring: the cutter's way round.
	/** @param cut Canonical quoted cutter visit. @return Whether its actual sweep reaches the approach. */
	CHECK(std::ranges::any_of(route, [&](const ExcavationCutGeometry &cut) {
		/** @param tile Actual swept world tile. @return Whether it belongs to the quoted approach. */
		return std::ranges::any_of(cut.swept_tiles, [&](TileIndex tile) { return std::ranges::binary_search(approach.tiles, tile); });
	}));

	// A straight corridor ahead of the door needs no approach: no line.
	tools->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, c.start, c.end);
	tools->OnPaint();
	const auto straight = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, c.rig.fixture.origin->index, c.start, c.end, false);
	REQUIRE(straight.purchase.Succeeded());
	CHECK(straight.area.approach.tiles.empty());
	CHECK(tools->GetWidget<NWidgetStacked>(WID_UC_APPROACH_STACK)->shown_plane == SZSP_NONE);
	CHECK(tools->GetWidgetString(WID_UC_AREA_APPROACH, STR_UNDERGROUND_AREA_EMPTY).empty());
	ResetObjectToPlace();
}
