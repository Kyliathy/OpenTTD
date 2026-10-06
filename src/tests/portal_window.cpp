/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file portal_window.cpp Exercise the actual portal click and window initialization that backend tests omit.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, portal controls
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, paused crash investigation
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "../core/backup_type.hpp"
#include "../fileio_func.h"
#include "../gfx_func.h"
#include "../language.h"
#include "../layer_portal_cmd.h"
#include "../rail.h"
#include "../strings_func.h"
#include "../tile_cmd.h"
#include "../window_func.h"
#include "../window_gui.h"

#include "../safeguards.h"

/** A fresh sealed portal must open its real control window, refresh it, and reuse it from either endpoint. */
TEST_CASE("Clicking a fresh sealed portal opens its controls", "[portal-window]")
{
	// Language changes rebuild cargo/industry presentation before the focused world installs its reduced registry.
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture fixture;
	ResetLayerPortalWorkState();
	ResetRailTypes();
	LayerWindowTestUI ui(WindowClass::LayerPortal, _current_company);
	const TileIndex source = TileXYL(20, 20, MapLayer::Surface);
	const TileIndex target = MultiLayerMap::TileInLayer(source, MapLayer::Underground);
	REQUIRE(CmdBuildLayerPortal({DoCommandFlag::Execute}, source, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE).Succeeded());
	REQUIRE(ClickTile(source));
	Window *window = FindWindowById(WindowClass::LayerPortal, source.base());
	REQUIRE(window != nullptr);
	CHECK(window->width > 0);
	CHECK(window->height > 0);
	window->OnPaint();
	REQUIRE(ClickTile(target));
	CHECK(FindWindowById(WindowClass::LayerPortal, source.base()) == window);
	window->OnPaint();
	ResetLayerPortalWorkState();
}
