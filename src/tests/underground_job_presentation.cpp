/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file underground_job_presentation.cpp Actual selected-job footprint/progress UI without geological mutation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md task16.1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "tbm_fixture.h"
#include "../underground_gui.h"
#include "../tbm_gui.h"
#include "../widgets/underground_widget.h"
#include "../widgets/tbm_widget.h"
#include "../construction_preview.h"
#include "../tilehighlight_func.h"
#include "../table/strings.h"

#include "../safeguards.h"

/** A real clicked job displays its canonical authorization and sole progress cursor; cancelling placement removes its transient overlays. */
TEST_CASE("Underground project inspection uses actual job footprint and progress", "[underground-job-presentation]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMWorkRig rig(3);
	LayerWindowTestUI ui(WindowClass::UndergroundConstruction, _current_company);
	LayerWindowTestUI portal_ui(WindowClass::LayerPortal, _current_company);
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr);
	window->OnPaint();
	const auto job = rig.Job();
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto project = rig.Project();
	InspectUndergroundProject(project.id);
	window->OnPaint();
	CHECK(window->GetWidgetString(WID_UC_JOB_PROGRESS, STR_UNDERGROUND_JOB_NONE) ==
		GetString(STR_UNDERGROUND_JOB_PROGRESS, job.id.base(), job.cursor, job.route.size(), job.partial_work_ticks));
	// The accepted authorization, not a drawing rectangle, determines which
	// requested cells are floor and which additional cells are charged working space.
	for (const TileIndex tile : job.authorized_tiles) {
		const auto role = std::ranges::find(job.requested_tiles, tile) == job.requested_tiles.end() ? ConstructionPreviewTile::WorkingSpace : ConstructionPreviewTile::Floor;
		CHECK(GetConstructionPreviewTile(tile) == role);
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(rig.Project() == project);
	ResetObjectToPlace();
	window->OnPaint();
	for (const TileIndex tile : job.authorized_tiles) CHECK(GetConstructionPreviewTile(tile) == ConstructionPreviewTile::None);
}

/** Browser rows and action eligibility follow current ownership; selection cannot allocate work or expose a foreign project. */
TEST_CASE("TBM browser filters real jobs and revalidates stale ownership", "[tbm-project-browser]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMWorkRig rig(3);
	LayerWindowTestUI browser_ui(WindowClass::TBMProjects, _current_company);
	LayerWindowTestUI palette_ui(WindowClass::UndergroundConstruction, _current_company);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto project = rig.Project();
	Window *browser = ShowTBMProjectsWindow(MapLayer::Underground);
	REQUIRE(browser != nullptr);
	browser->OnPaint();
	CHECK(browser->GetScrollbar(WID_TP_SCROLL)->GetCount() == 1);
	CHECK_FALSE(browser->IsWidgetDisabled(WID_TP_SELECT));
	browser->OnClick({}, WID_TP_SELECT, 1);
	CHECK(_thd.GetCallbackWnd()->window_class == WindowClass::UndergroundConstruction);
	ResetObjectToPlace();
	Window *empty = ShowTBMProjectsWindow(MapLayer::DeepUnderground);
	REQUIRE(empty != nullptr);
	empty->OnPaint();
	CHECK(empty->GetScrollbar(WID_TP_SCROLL)->GetCount() == 0);
	CHECK(empty->IsWidgetDisabled(WID_TP_SELECT));
	{
		AutoRestoreBackup spectator(_local_company, CompanyID::Invalid());
		browser->OnPaint();
		CHECK(browser->GetScrollbar(WID_TP_SCROLL)->GetCount() == 0);
		CHECK(browser->IsWidgetDisabled(WID_TP_VEHICLE));
		browser->OnClick({}, WID_TP_SELECT, 1);
		CHECK(_thd.GetCallbackWnd() == nullptr);
		CHECK(ShowTBMProjectsWindow(MapLayer::Underground) == nullptr);
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(rig.Project() == project);
}
