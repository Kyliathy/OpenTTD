/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file ground_render_camera_fixture.cpp One actual camera observer for conventional Ground consumers.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "ground_render_camera_fixture.h"
#include "mock_environment.h"
#include "underground_view_fixture.h"
#include "save_candidate_lifecycle_fixture.h"
#include "../transparency.h"
#include "../map_layer_gui.h"
#include "../safeguards.h"
/** @copydoc PrepareGroundRenderCameraLanguage */
void PrepareGroundRenderCameraLanguage()
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
}
/** @copydoc CheckGroundRenderCameraPurity */
void CheckGroundRenderCameraPurity(const std::function<std::vector<SpriteID>()> &query)
{
	const auto before = CaptureCandidateLiveState();
	const auto selected = query(); REQUIRE_FALSE(selected.empty());
	{
		UndergroundToolbarTestUI ui(_current_company);
		Window *main = new UndergroundTestViewport();
		Window *extra = new UndergroundTestViewport(_underground_extra_view_desc);
		AutoRestoreBackup transparency(_transparency_opt); AutoRestoreBackup hidden(_invisibility_opt);
		// Real camera notifications and hidden presentation cannot select another
		// industry's cell, station parent or body, or grant work/cargo authority
		// while the physical operating layer is out of view.
		for (MapLayer layer : {MapLayer::Surface, MapLayer::Underground, MapLayer::DeepUnderground, MapLayer::Space}) {
			SetViewportLayer(main, layer); SetViewportLayer(extra, MapLayer::Space);
			CHECK(query() == selected);
			_transparency_opt = {TransparencyOption::Industries, TransparencyOption::Buildings};
			_invisibility_opt = _transparency_opt; CHECK(query() == selected);
			_transparency_opt = {}; _invisibility_opt = {};
			const auto actual = CaptureCandidateLiveState();
			CHECK(actual.book == before.book); CHECK(actual.packets == before.packets); CHECK(actual.accounts == before.accounts);
			CHECK(actual.domains == before.domains); CHECK(actual.random == before.random); CHECK(actual.tick == before.tick);
		}
	}
	CHECK(CaptureCandidateLiveState() == before);
}
