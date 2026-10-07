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
#include "../widgets/underground_widget.h"
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
	window->OnClick({}, WID_UC_PROJECT, 1);
	window->OnPlaceObject({}, job.commissioning_anchor);
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
