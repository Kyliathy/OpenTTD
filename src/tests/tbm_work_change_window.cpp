/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file tbm_work_change_window.cpp Actual change-work and release-confirmation window actions.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native command ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group12.
 * Paid cuts are real; component exchanges and the cancelled release pose are declared inputs, not driven arrival.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_remainder_fixture.h"
#include "underground_view_fixture.h"
#include "../underground_gui.h"
#include "../tbm_gui.h"
#include "../tbm_replanning.h"
#include "../widgets/tbm_widget.h"
#include "../widgets/underground_widget.h"
#include "../widgets/misc_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** @return First world corner of the new floor beside the fixture's planned corridor. */
static TileIndex ChangeFrom() { return TileXYL(22, 25, MapLayer::Underground); }
/** @return Opposite corner of that same physical rock-plane floor. */
static TileIndex ChangeTo() { return TileXYL(23, 26, MapLayer::Underground); }
/** @param quote Actual native change quote. @return Localized refusal or empty text for an accepted quote. */
static std::string Refusal(const TBMWorkChangePreview &quote)
{
    return quote.result.Succeeded() ? std::string{} : GetString(quote.result.GetErrorMessage());
}

/** Tasks 12.3 and 12.5: the projects window and New area open Change work area; the palette quotes and confirms it. */
TEST_CASE("Change work area from the projects window and New area, without a pause", "[demo-fix-change-work-ui]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMRemainderRig rig;
	rig.Bore(2);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	UndergroundToolbarTestUI ui(_current_company);
	LayerWindowTestUI browser_ui(WindowClass::TBMProjects, _current_company);
	new UndergroundTestViewport(); // Planning centres the main view on the face.
	// The player's game runs; the fixture's pause would hold the posted command.
	AutoRestoreBackup running(_pause_mode, PauseModes{});
	const TBMProjectID id = rig.Project().id;
	Window *browser = ShowTBMProjectsWindow(MapLayer::Underground);
	REQUIRE(browser != nullptr);
	browser->OnPaint();
	REQUIRE_FALSE(browser->IsWidgetDisabled(WID_TP_CHANGE));
	browser->OnClick({}, WID_TP_CHANGE, 1);
	Window *palette = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(palette != nullptr);
	palette->OnPaint();
	CHECK(_thd.GetCallbackWnd() == palette);
	CHECK(palette->GetWidgetString(WID_UC_AREA, STR_UNDERGROUND_AREA) == GetString(STR_TBM_PROJECT_CHANGE_WORK));
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_TBM_CHANGE_SELECT_FLOOR, id.base()));
	CHECK(GetTBMProject(id)->blocked != TBMBlockedReason::Paused); // No pause was posted.

	palette->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, ChangeFrom(), ChangeTo());
	palette->OnPaint();
	const auto quote = PreviewTBMWorkChange(_current_company, id, ChangeFrom(), ChangeTo());
	INFO("refusal: " << Refusal(quote));
	REQUIRE(quote.result.Succeeded());
	REQUIRE(quote.abandoned_work_ticks > 0);
	const uint64_t done = std::min<uint64_t>(100, CeilDiv(quote.abandoned_work_ticks * 100, quote.cut_work_ticks));
	CHECK(palette->GetWidget<NWidgetStacked>(WID_UC_CHANGE_STACK)->shown_plane == 0);
	CHECK(palette->GetWidgetString(WID_UC_AREA_CHANGE, STR_UNDERGROUND_AREA_EMPTY) ==
		GetString(STR_TBM_CHANGE_SUMMARY_ABANDON, quote.cursor, quote.dropped_cuts, quote.plan.route.size(), done));
	CHECK(palette->GetWidgetString(WID_UC_AREA_CONFIRM, STR_UNDERGROUND_AREA_CONFIRM) == GetString(STR_TBM_CHANGE_CONFIRM));
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_TBM_CHANGE_READY));
	REQUIRE_FALSE(palette->IsWidgetDisabled(WID_UC_AREA_CONFIRM));
	const uint32_t cursor = rig.Job().cursor;
	palette->OnClick({}, WID_UC_AREA_CONFIRM, 1);
	CHECK(rig.Job().route[cursor] == quote.plan.route.front());
	CHECK(rig.Project().phase == TBMPhase::Bore);
	CHECK(_thd.GetCallbackWnd() != palette);

	// Task 12.5: New area on the project's own portal now changes its work area too, still without a pause.
	palette->OnClick({}, WID_UC_AREA, 1);
	palette->OnPlaceObject({}, rig.Job().commissioning_anchor);
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_TBM_CHANGE_SELECT_FLOOR, id.base()));
	CHECK(GetTBMProject(id)->blocked != TBMBlockedReason::Paused);
	// Cancelling leaves the machine working on its plan.
	ResetObjectToPlace();
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_TBM_CHANGE_CANCELLED));
}

/** Task 12.5: New area on a finished project's portal offers release and new work with the same machine. */
TEST_CASE("New area on a finished project's portal offers release and new work", "[demo-fix-change-work-ui]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMRemainderRig rig;
	UndergroundToolbarTestUI ui(_current_company);
	LayerWindowTestUI query_ui(WindowClass::ConfirmPopupQuery, _current_company);
	AutoRestoreBackup running(_pause_mode, PauseModes{});
	// A cancelled job leaves a finished project at the portal, ready to be released.
	const TBMProjectID id = rig.Project().id;
	const TileIndex anchor = rig.Job().commissioning_anchor;
	REQUIRE(CancelExcavationJob(rig.Job().id) == ExcavationError::None);
	rig.Reload(TBMPhase::Cancelled);
	REQUIRE(IsTBMProjectFinished(rig.Project().phase));
	REQUIRE_FALSE(CanChangeTBMWork(rig.Project()));
	Window *palette = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(palette != nullptr);
	palette->OnPaint();
	palette->OnClick({}, WID_UC_AREA, 1);
	palette->OnPlaceObject({}, anchor);
	Window *query = FindWindowByClass(WindowClass::ConfirmPopupQuery);
	REQUIRE(query != nullptr);
	CHECK(rig.Project().phase == TBMPhase::Cancelled); // Asking changes nothing.
	// Declining keeps the project; the portal is not armed for new work.
	query->OnClick({}, WID_Q_NO, 1);
	Window::DeleteClosedWindows();
	CHECK(GetTBMProject(id) != nullptr);
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) != GetString(STR_UNDERGROUND_AREA_SELECT_FLOOR));
	// Accepting releases it through the ordinary Retire command and arms the same portal for new work.
	palette->OnPlaceObject({}, anchor);
	query = FindWindowByClass(WindowClass::ConfirmPopupQuery);
	REQUIRE(query != nullptr);
	query->OnClick({}, WID_Q_YES, 1);
	Window::DeleteClosedWindows();
	CHECK(GetTBMProject(id) == nullptr);
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_UNDERGROUND_AREA_SELECT_FLOOR));
	ResetObjectToPlace();
}
