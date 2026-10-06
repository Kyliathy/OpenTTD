/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_area.cpp Real area-selection widgets, native material quotes and world-tile highlight roles. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 3C */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "tbm_commission_fixture.h"
#include "../underground_gui.h"
#include "../tbm_area_preview.h"
#include "../construction_preview.h"
#include "../map_layer_gui.h"
#include "../tilehighlight_func.h"
#include "../widgets/underground_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Actual UI selection shows the native quote and its exact floor/working-space roles without charging or reserving. */
TEST_CASE("Underground area selection displays canonical material and work without simulation", "[underground-area-window]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMCommissionRig c;
	LayerWindowTestUI ui(WindowClass::UndergroundConstruction, _current_company);
	Window *tools = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(tools != nullptr);
	tools->OnPaint();
	REQUIRE(GetLayerPortalDefaultTBMModel(c.rock) == 0);
	REQUIRE(GetLayerPortalDefaultTBMOrigin(c.rock, _current_company) == c.rig.fixture.origin->index);
	const auto before = c.rig.Digest();
	tools->OnClick({}, WID_UC_AREA, 1);
	tools->OnPlaceObject({}, c.rock);
	tools->OnPlaceObject({}, c.start);
	tools->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, c.start, c.end);
	tools->OnPaint();
	CHECK_FALSE(tools->IsWidgetDisabled(WID_UC_AREA_CONFIRM));
	const auto quote = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, c.rig.fixture.origin->index, c.start, c.end, false);
	REQUIRE(quote.purchase.Succeeded());
	CHECK(tools->GetWidgetString(WID_UC_AREA_PRICE, STR_UNDERGROUND_AREA_EMPTY) == GetString(STR_UNDERGROUND_AREA_PRICE, quote.purchase.GetCost(), quote.area.totals.cost));
	CHECK(tools->GetWidgetString(WID_UC_AREA_MATERIAL, STR_UNDERGROUND_AREA_EMPTY) == GetString(STR_UNDERGROUND_AREA_MATERIAL,
		quote.area.totals.earth_t, quote.area.totals.rock_t, quote.area.totals.support_units, quote.area.totals.work_ticks));
	// Every canonical final-use cell is white and every actually swept extra cell is blue, independent of plane overlap.
	for (TileIndex tile : quote.area.proposal.requested_tiles) CHECK(GetConstructionPreviewTile(tile) == ConstructionPreviewTile::Floor);
	for (TileIndex tile : quote.area.turn_envelope) CHECK(GetConstructionPreviewTile(tile) == ConstructionPreviewTile::WorkingSpace);
	CHECK(GetConstructionPreviewTile(MultiLayerMap::TileInLayer(c.start, MapLayer::Surface)) == ConstructionPreviewTile::None);
	CHECK(c.rig.Digest() == before);
	ResetObjectToPlace();
	CHECK(GetConstructionPreviewTile(c.start) == ConstructionPreviewTile::None);
	tools->OnPaint();
	CHECK(tools->IsWidgetDisabled(WID_UC_AREA_CONFIRM));
	CHECK(c.rig.Digest() == before);
}

/** A chamber requiring canonical advancing bends shows their exact extra sweep and any uncovered floor as a refusal. */
TEST_CASE("Underground chamber preview preserves legal turn envelopes and partial refusal", "[underground-area-turns]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMCommissionRig c;
	LayerWindowTestUI ui(WindowClass::UndergroundConstruction, _current_company);
	Window *tools = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(tools != nullptr);
	tools->OnPaint();
	const Direction heading = DiagDirToDir(GetLayerPortalDoorDirection(c.rock));
	const auto ahead = TileIndexDiffCByDir(heading);
	const auto side = TileIndexDiffCByDir(ChangeDir(heading, DirDiff::Right90));
	const TileIndex first = TileAddWrap(c.rock, ahead.x - 3 * side.x, ahead.y - 3 * side.y);
	const TileIndex last = TileAddWrap(c.rock, 9 * ahead.x + 3 * side.x, 9 * ahead.y + 3 * side.y);
	const auto before = c.rig.Digest();
	tools->OnClick({}, WID_UC_AREA, 1);
	tools->OnPlaceObject({}, c.rock);
	tools->OnPlaceObject({}, first);
	tools->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, first, last);
	tools->OnPaint();
	const auto quote = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, c.rig.fixture.origin->index, first, last, false);
	REQUIRE_FALSE(quote.area.plan.route.empty());
	bool bends = false;
	Direction previous = heading;
	// Every observed change of heading must be a legal single advancing 45-degree step, rather than a reversed cutter.
	for (const auto &cut : quote.area.plan.route) {
		if (cut.heading != previous) {
			CHECK((cut.heading == ChangeDir(previous, DirDiff::Right45) || cut.heading == ChangeDir(previous, DirDiff::Left45)));
			bends = true;
		}
		previous = cut.heading;
	}
	CHECK(bends);
	// Drawing consumes the canonical covered/remaining floor and swept difference; it must never invent extra excavated cells.
	for (TileIndex tile : quote.area.plan.covered_targets) CHECK(GetConstructionPreviewTile(tile) == ConstructionPreviewTile::Floor);
	for (TileIndex tile : quote.area.turn_envelope) CHECK(GetConstructionPreviewTile(tile) == ConstructionPreviewTile::WorkingSpace);
	for (TileIndex tile : quote.area.plan.remaining_targets) CHECK(GetConstructionPreviewTile(tile) == ConstructionPreviewTile::Uncovered);
	CHECK(tools->IsWidgetDisabled(WID_UC_AREA_CONFIRM) == quote.purchase.Failed());
	CHECK(c.rig.Digest() == before);
	ResetObjectToPlace();
}
