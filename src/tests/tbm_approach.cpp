/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file tbm_approach.cpp Native commission/extension quotes for paid approach cells and exact endpoints.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md original command/geological owners.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23CB.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_commission_fixture.h"
#include "excavation_claim_fixture.h"
#include "../tbm_area_preview.h"
#include "../tbm_replanning.h"
#include "../tbm_cmd.h"
#include "../command_func.h"
#include "../table/strings.h"
#include "../safeguards.h"

/**
 * The demo's beside-exit row at the TBM rig's second pair: the door faces SE, so the row runs SW (+x) from a cell.
 * @param first First cell of the row.
 * @return Its eighth cell.
 */
static TileIndex RowEnd(TileIndex first)
{
	return TileAddWrap(first, 7, 0);
}

/**
 * Require a TBM job proposal to be accepted by the pass planner and, under the current turn rule, by the support rail.
 * @param proposal Job proposal of an accepted quote.
 */
static void CheckDrivable(const ExcavationJob &proposal)
{
	REQUIRE(proposal.pass_terms.has_value());
	CHECK(PrepareExcavationPass(proposal, *proposal.pass_terms).error == ExcavationError::None);
	CHECK(CheckTBMSupportRailTurns(proposal));
}

/** Screenshot 05 at the TBM quote: the demo's request is accepted, its approach cells charged, and it can be bought. */
TEST_CASE("Screenshot 05's commission quote is accepted with charged approach cells", "[tbm-approach-quote]")
{
	bool forbid = false;
	SECTION("90-degree turns allowed") { forbid = false; }
	SECTION("90-degree turns forbidden") { forbid = true; }
	AutoRestoreBackup rule(_settings_game.pf.forbid_90_deg, forbid);
	TBMCommissionRig c;
	const TileIndex first = c.start;
	const TileIndex last = RowEnd(first);
	const StationID origin = c.rig.fixture.origin->index;
	const auto before = c.rig.Digest();
	const auto quote = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, origin, first, last, false);
	INFO("purchase " << quote.purchase.GetErrorMessage().base() << " error " << int(to_underlying(quote.area.error)));
	REQUIRE(quote.purchase.Succeeded());
	const auto &area = quote.area;
	CHECK_FALSE(area.partial);
	CHECK(area.plan.remaining_targets.empty());
	CHECK_FALSE(area.approach.tiles.empty());
	CHECK(area.approach.estimate.cost.base() > 0);
	// The approach cells are part of the reviewed authorization, outside the floor.
	CHECK(std::ranges::includes(quote.request.proposal.authorized_tiles, area.approach.tiles));
	for (TileIndex tile : area.approach.tiles) CHECK_FALSE(std::ranges::binary_search(quote.request.proposal.requested_tiles, tile));
	CHECK(area.plan.route.front().heading == DiagDirToDir(GetLayerPortalDoorDirection(c.rock)));
	CheckDrivable(quote.request.proposal);
	CHECK(c.rig.Digest() == before); // Quoting buys, digs, claims and draws nothing.

	// The reviewed quote buys the machine for exactly that route and authorization.
	const size_t projects = CaptureTBMProjects().size();
	REQUIRE(CmdConfirmTBMCommission(DoCommandFlag::Execute, c.portal, 0, 2, 2, origin, first, last, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	REQUIRE(CaptureTBMProjects().size() == projects + 1);
	const auto *job = GetExcavationJob(CaptureTBMProjects().back().job);
	REQUIRE(job != nullptr);
	CHECK(job->route == quote.request.proposal.route);
	CHECK(job->authorized_tiles == quote.request.proposal.authorized_tiles);
}

/** Task 9.5: the approach is part of the reviewed quote, so a change to one of its cells refuses the confirmation. */
TEST_CASE("A changed approach cell makes the reviewed commission stale", "[tbm-approach-quote]")
{
	TBMCommissionRig c;
	const TileIndex first = c.start;
	const TileIndex last = RowEnd(first);
	const StationID origin = c.rig.fixture.origin->index;
	const auto quote = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, origin, first, last, false);
	REQUIRE(quote.purchase.Succeeded());
	// Open one approach cell that is still solid rock: what the reviewed quote charged is no longer what would be dug.
	/** @param tile Quoted approach cell. @return Whether it still carries the original paid-rock estimate. */
	const auto solid = std::ranges::find_if(quote.area.approach.tiles, [](TileIndex tile) { return GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Solid; });
	REQUIRE(solid != quote.area.approach.tiles.end());
	c.rig.fixture.world.Open(std::array{*solid});
	const auto before = c.rig.Digest();
	const size_t projects = CaptureTBMProjects().size();
	CHECK(CmdConfirmTBMCommission(DoCommandFlag::Execute, c.portal, 0, 2, 2, origin, first, last, quote.fingerprint, ExcavationPlanIntent::Area).GetErrorMessage() == STR_ERROR_TBM_EXTEND_STALE);
	CHECK(CaptureTBMProjects().size() == projects);
	CHECK(c.rig.Digest() == before);
	// A fresh review of the changed ground is accepted again.
	const auto fresh = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, origin, first, last, false);
	REQUIRE(fresh.purchase.Succeeded());
	CHECK(fresh.fingerprint != quote.fingerprint);
}

/** Task 9.3: more work beside the accepted route's end gets its approach from that end, and is appended as reviewed. */
TEST_CASE("An extension beside the route end gets its approach from the route end", "[tbm-approach-quote]")
{
	TBMCommissionRig c;
	REQUIRE(c.Commission(DoCommandFlag::Execute).Succeeded());
	const TBMProjectID id = CaptureTBMProjects().back().id;
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Pause).Succeeded());
	const auto *job = GetExcavationJob(GetTBMProject(id)->job);
	REQUIRE(job != nullptr);
	const auto old = *job;
	// The demo's extension request: a row sideways from the cell after the accepted route's end.
	const auto ahead = TileIndexDiffCByDir(old.route.back().heading);
	const TileIndex first = TileAddWrap(old.route.back().cutter_position, ahead.x, ahead.y);
	const TileIndex last = RowEnd(first);
	const auto before = c.rig.Digest();
	const auto quote = PreviewTBMExtension(_current_company, id, first, last, ExcavationPlanIntent::Area);
	INFO("result " << quote.result.GetErrorMessage().base() << " error " << int(to_underlying(quote.plan.error)));
	REQUIRE(quote.result.Succeeded());
	CHECK_FALSE(quote.approach.tiles.empty());
	CHECK(quote.plan.remaining_targets.empty());
	CHECK(std::ranges::includes(quote.outline.authorized_tiles, quote.approach.tiles));
	CHECK(c.rig.Digest() == before);
	// The new cuts continue the accepted route, and the joined route stays drivable.
	ExcavationJob combined = old;
	combined.route.insert(combined.route.end(), quote.plan.route.begin(), quote.plan.route.end());
	CHECK(CheckTBMSupportRailTurns(combined));
	CHECK(DistanceMax(quote.plan.route.front().cutter_position, old.route.back().cutter_position) == 1);
	REQUIRE(Command<Commands::ExtendTBMProject>::Do(DoCommandFlag::Execute, id, first, last, quote.job_revision, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	const auto *extended = GetExcavationJob(GetTBMProject(id)->job);
	CHECK(extended->route.size() == old.route.size() + quote.plan.route.size());
	CHECK(std::ranges::includes(extended->authorized_tiles, quote.approach.tiles));
}

/** Task 9.4: a tunnel end off the door's axis is reached through 45-degree bends, its centres becoming the floor. */
TEST_CASE("An angled tunnel follows an exact-goal approach to its end", "[tbm-approach-quote]")
{
	TBMCommissionRig c;
	const auto ahead = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(c.rock));
	REQUIRE(ahead.x == 0); // The door faces SE: an end 4 cells to the side and 6 ahead is off its axis.
	const TileIndex end = TileAddWrap(c.rock, 4, 6 * ahead.y);
	const StationID origin = c.rig.fixture.origin->index;
	const auto quote = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, origin, c.rock, end, false, ExcavationPlanIntent::StraightTunnel);
	INFO("purchase " << quote.purchase.GetErrorMessage().base() << " error " << int(to_underlying(quote.area.error)));
	REQUIRE(quote.purchase.Succeeded());
	const auto &route = quote.area.plan.route;
	REQUIRE_FALSE(route.empty());
	CHECK(route.back().cutter_position == end);
	CHECK(route.front().heading == DiagDirToDir(GetLayerPortalDoorDirection(c.rock)));
	/** @param cut Actual proposed advance. @return Whether the exact endpoint route needs a 45-degree bend. */
	CHECK(std::ranges::any_of(route, [](const ExcavationCutGeometry &cut) { return !IsDiagonalDirection(cut.heading); })); // It bends.
	// The requested floor is the cutter centres.
	std::vector<TileIndex> centres;
	for (const auto &cut : route) centres.push_back(cut.cutter_position);
	std::ranges::sort(centres);
	CHECK(quote.request.proposal.requested_tiles == centres);
	CheckDrivable(quote.request.proposal);
	REQUIRE(CmdConfirmTBMCommission(DoCommandFlag::Execute, c.portal, 0, 2, 2, origin, c.rock, end, quote.fingerprint, ExcavationPlanIntent::StraightTunnel).Succeeded());
}

/** An impossible request is refused before purchase with its blocking tile, and the quote changes nothing. */
TEST_CASE("An impossible commission request names its blocking tile", "[tbm-approach-quote]")
{
	TBMCommissionRig c;
	const TileIndex first = c.start;
	const TileIndex last = RowEnd(first);
	// Another project's claimed work on the row's last cell: no route may open the whole row.
	ClaimSeparateExcavationWork(c.rig.fixture.world, TileAddWrap(last, -2, 0));
	REQUIRE(GetGeologyWorldState().job_claims.contains(last));
	const auto before = c.rig.Digest();
	const auto quote = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, c.rig.fixture.origin->index, first, last, false);
	REQUIRE(quote.purchase.Failed());
	CHECK(quote.purchase.GetErrorMessage() == STR_ERROR_TBM_APPROACH_OTHER_WORK); // Task 10.4: the reason is named.
	CHECK(quote.area.error == ExcavationError::ActiveCut);
	CHECK(GetGeologyWorldState().job_claims.contains(quote.area.failure_tile));
	CHECK_FALSE(quote.area.plan.remaining_targets.empty());
	CHECK(c.rig.Digest() == before);
}

/** Task 10.4: each kind of approach refusal reads as its short reason. */
TEST_CASE("Approach refusals read as short reasons", "[tbm-approach-quote]")
{
	TBMCommissionRig c;
	CHECK(GetTBMApproachRefusal(ExcavationError::InvalidTile, INVALID_TILE) == STR_ERROR_TBM_APPROACH_MAP_EDGE);
	CHECK(GetTBMApproachRefusal(ExcavationError::Conflict, c.rock) == STR_ERROR_TBM_APPROACH_BUILDING); // The portal housing is a building.
	CHECK(GetTBMApproachRefusal(ExcavationError::Conflict, c.start) == STR_ERROR_TBM_APPROACH_OTHER_WORK); // Open rock in conflict: a foreign seam.
	CHECK(GetTBMApproachRefusal(ExcavationError::ActiveCut, c.start) == STR_ERROR_TBM_APPROACH_OTHER_WORK);
	CHECK(GetTBMApproachRefusal(ExcavationError::InvalidOwner, c.start) == STR_ERROR_TBM_APPROACH_OTHER_WORK);
	CHECK(GetTBMApproachRefusal(ExcavationError::Limit, c.start) == STR_ERROR_TBM_APPROACH_TOO_FAR);
	CHECK(GetTBMApproachRefusal(ExcavationError::MissingSupport, c.start) == STR_ERROR_TBM_COMMISSION_ROUTE);
}
