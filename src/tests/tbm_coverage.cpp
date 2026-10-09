/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_coverage.cpp Area coverage and turning through the actual train (Group 11): straight, two-bend and
 * irregular floors bored to completion against the accepted outline; tight turns, solid diagonal corners and a late
 * obstruction that pauses at the previous safe support; reversing and a reload during a bend.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 11
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../train_cmd.h"
#include "../direction_func.h"
#include "../command_func.h"

#include "../safeguards.h"

/** Floor cell relative to the housing: steps along the exit heading and to its right. */
struct FloorCell {
	int along; ///< Steps along the exit heading.
	int right; ///< Steps to the right of it.
};

/**
 * @param rig Rig with an issued proof.
 * @param along Steps along the exit heading.
 * @param right Steps to its right.
 * @return The rock-layer tile there.
 */
static TileIndex GetFloorTile(const TBMAcquisitionRig &rig, int along, int right)
{
	const auto ahead = TileIndexDiffCByDir(rig.proof.exit_heading);
	const auto side = TileIndexDiffCByDir(ChangeDir(rig.proof.exit_heading, DirDiff::Right90));
	const TileIndex tile = TileAddWrap(rig.proof.destination, ahead.x * along + side.x * right, ahead.y * along + side.y * right);
	REQUIRE(IsValidTile(tile));
	return tile;
}

/**
 * Area request for a floor, with an explicitly accepted working space around it.
 * @param rig Rig with an issued proof.
 * @param floor Requested floor cells.
 * @param width Accepted working space to each side of the exit line.
 * @param behind Accepted rows behind the housing.
 * @return The request for two Earth and two Rock hoppers from the housing.
 */
static TBMAreaRequest MakeFloorRequest(const TBMAcquisitionRig &rig, std::span<const FloorCell> floor, int width, int behind = 1)
{
	std::vector<TileIndex> requested;
	std::vector<TileIndex> extra;
	for (const auto &cell : floor) requested.push_back(GetFloorTile(rig, cell.along, cell.right));
	// The player accepts a rectangle from behind the housing to beyond the floor; the planner may not invent margins.
	for (int along = -behind; along <= 12; ++along) {
		for (int right = -width; right <= width; ++right) extra.push_back(GetFloorTile(rig, along, right));
	}
	const auto base = NormalizeExcavationOutline(MapLayer::Underground, requested);
	REQUIRE(base.error == ExcavationError::None);
	TBMAreaRequest area;
	area.outline = AuthorizeExcavationOutline(base, extra, 0);
	REQUIRE(area.outline.error == ExcavationError::None);
	area.consist = {2, 2};
	area.model_id = 0;
	area.company = _current_company;
	area.origin = rig.fixture.origin->index;
	area.start = {rig.proof.destination, rig.proof.exit_heading, std::vector<TileIndex>(4, rig.proof.destination)};
	return area;
}

/**
 * Bore every cut of the rig's job, standing in for the exchange between cuts.
 * @param rig Work rig in Bore at the first cut.
 * @return Total estimated new cells of all cuts.
 */
static uint64_t BoreWholeJob(const TBMWorkRig &rig)
{
	uint64_t cells = 0;
	const size_t cuts = rig.Job().route.size();
	for (size_t index = 0; index < cuts; ++index) {
		if (index != 0) rig.ExchangeAndReturn();
		cells += rig.BoreOneCut().new_cells;
	}
	REQUIRE(rig.Job().state == ExcavationJobState::Completed);
	return cells;
}

/** The same canonical route implementation covers each floor, and the train opens exactly the swept cells. */
TEST_CASE("Straight, two-bend and irregular floors are bored to the accepted outline", "[tbm-coverage]")
{
	std::vector<FloorCell> floor;
	SECTION("straight pass") {
		for (int along = 1; along <= 6; ++along) floor.push_back({along, 0});
	}
	SECTION("a 90-degree change made of two advancing bends") {
		// Outlines are 8-connected: the straight leg's last cell (5, 0) touches the side leg's first cell (6, 1).
		for (int along = 1; along <= 5; ++along) floor.push_back({along, 0});
		for (int right = 1; right <= 4; ++right) floor.push_back({6, right});
	}
	SECTION("irregular chamber with a branch") {
		floor = {{4, 0}, {5, 0}, {4, 1}, {5, 1}, {6, -1}, {7, -1}};
	}
	TBMAreaPreview shown;
	TBMWorkRig rig([&](const TBMAcquisitionRig &base) {
		shown = PreviewTBMArea(MakeFloorRequest(base, floor, 6));
		REQUIRE(shown.consist_error == TBMAcquisitionError::None);
		REQUIRE(shown.error == ExcavationError::None);
		return shown.proposal;
	});
	CHECK_FALSE(shown.partial);
	// The controller binds the planner's route unchanged; it never generates a route of its own.
	CHECK(rig.Job().route == shown.plan.route);
	CHECK(rig.Job().requested_tiles == shown.proposal.requested_tiles);
	std::set<TileIndex> swept;
	for (const auto &cut : rig.Job().route) swept.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());

	CHECK(BoreWholeJob(rig) == swept.size()); // Each swept cell is charged once, turn space included.
	const auto &geology = GetGeologyWorldState();
	// Every requested floor cell is open; accepted cells the route never swept are still solid.
	for (TileIndex tile : rig.Job().requested_tiles) CHECK(geology.terrain.Get(tile) == ExcavationState::Excavated);
	for (TileIndex tile : rig.Job().authorized_tiles) {
		CHECK((geology.terrain.Get(tile) == ExcavationState::Excavated) == swept.contains(tile));
	}
}

/** A U-turn needs more room than a three-cell corridor; the refusal changes nothing. */
TEST_CASE("Too-tight turns are refused before anything is reserved", "[tbm-coverage]")
{
	TBMAcquisitionRig rig;
	const auto before = rig.Digest();
	// In a corridor three cells wide the cutter's own body covers both side rows, so only cells behind the housing
	// need a turn: a 180-degree change, while every 45-degree step would sweep outside the accepted corridor. The
	// side cells beside the housing join the two legs into one 8-connected outline.
	const std::array<FloorCell, 9> floor{{{1, 0}, {2, 0}, {3, 0}, {1, 1}, {0, 1}, {-1, 1}, {-2, 0}, {-3, 0}, {-4, 0}}};
	const auto preview = PreviewTBMArea(MakeFloorRequest(rig, floor, 1, 5));
	CHECK((preview.error != ExcavationError::None || preview.partial));
	CHECK(rig.Digest() == before);
	CHECK(GetGeologyWorldState().job_claims.empty());
}

/** A 45-degree advance also cuts both solid corner cells beside its step; it never passes through rock. */
TEST_CASE("Diagonal advances cut their solid corner cells", "[tbm-coverage]")
{
	TBMWorkRig rig({DirDiff::Same, DirDiff::Right45, DirDiff::Right45});
	const ExcavationJob planned = rig.Job();
	BoreWholeJob(rig);
	// For each corner advance, both cells beside the diagonal step lie in its sweep and are open now.
	for (size_t index = 1; index < planned.route.size(); ++index) {
		const auto &cut = planned.route[index];
		if (IsDiagonalDirection(cut.heading)) continue; // Axis advances cross a tile edge and have no corner cells.
		const TileIndex from = planned.route[index - 1].cutter_position;
		const int dx = static_cast<int>(TileX(cut.cutter_position)) - static_cast<int>(TileX(from));
		const int dy = static_cast<int>(TilePlaneY(cut.cutter_position)) - static_cast<int>(TilePlaneY(from));
		for (const TileIndex corner : {TileAddWrap(from, dx, 0), TileAddWrap(from, 0, dy)}) {
			CHECK(std::ranges::binary_search(cut.swept_tiles, corner));
			CHECK(GetGeologyWorldState().terrain.Get(corner) == ExcavationState::Excavated);
		}
	}
}

/** An obstruction introduced after planning pauses at the previous safe support, with the retreat still usable. */
TEST_CASE("A late obstruction pauses at the previous safe support with a valid retreat", "[tbm-coverage]")
{
	TBMWorkRig rig(2);
	rig.BoreOneCut();
	rig.ExchangeAndReturn();
	// A second consist on another pair stands in the next cut's envelope.
	const auto second = TileXYL(30, 20, MapLayer::Surface);
	REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, second, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
	auto request = rig.fixture.request;
	request.portal = second;
	request.proposal = rig.base;
	request.proposal.commissioning_anchor = GetLayerPortalPairTile(second);
	Train *other = nullptr;
	const std::array<const Engine *, 1> wagons{Engine::Get(GetTBMEngine(2))};
	REQUIRE(BuildTBMConsistAtSite(request, Engine::Get(GetTBMEngine(0)), 0, GetFreeUnitNumber(VehicleType::Train), wagons, &other).Succeeded());
	Train *intruder = other->Last();
	const TileIndex home = intruder->tile;
	intruder->tile = rig.Job().route[1].cutter_position;
	intruder->UpdatePosition();

	const auto estimate = EstimateExcavationCut(rig.project.job);
	REQUIRE(estimate.error == ExcavationError::None);
	for (uint64_t tick = 1; tick < estimate.work_ticks * 2; ++tick) REQUIRE(rig.Tick().blocked == TBMBlockedReason::None); // Reach the unchanged obstruction/commit boundary at half cutting speed.
	CHECK(rig.Tick().blocked == TBMBlockedReason::Obstruction);
	// The cutter is still at the first cut's position, and every cell back to the housing carries usable support.
	CHECK(rig.Job().cursor == 1);
	for (TileIndex tile : GetExcavationRetreatPath(rig.Job(), rig.Job().cursor)) CHECK(IsTBMSupportUsable(rig.Project(), tile));
	TBMCutParticipant participant(rig.Project());
	CHECK(participant.IsSupported(rig.Job(), rig.Job().route[1]));
	intruder->tile = home;
	intruder->UpdatePosition();
	CHECK(rig.Tick().phase == TBMPhase::ReturnForExchange); // The last cut commits, and its final exchange is due.
	CHECK(rig.Job().cursor == 2);
}

/** Reversing the train changes train state only: the saved heading stays and the face is still cut forwards. */
TEST_CASE("Reversing keeps the cutting heading and never cuts backwards", "[tbm-coverage]")
{
	TBMWorkRig rig(2);
	const Direction heading = rig.Project().heading;
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	REQUIRE(Command<Commands::ReverseTrainDirection>::Do(DoCommandFlag::Execute, rig.front->index, false).Succeeded());
	CHECK(rig.Tick().blocked == TBMBlockedReason::None);
	CHECK(rig.Project().heading == heading);
	CHECK(rig.Job().cursor == 0);
	CHECK(rig.Job().partial_work_ticks == 1); // Two half-speed ticks continue the same forward cut.
	CHECK(InspectTBMConsist(*rig.front, rig.Project()).error == TBMBlockedReason::None); // Roles do not follow the reversal.
}

/** A save in the middle of a bend reloads to the same cursor and spoil ledger, and commits on the same tick. */
TEST_CASE("A reload during a bend continues the same cursor and ledger", "[tbm-coverage]")
{
	TBMWorkRig rig({DirDiff::Same, DirDiff::Right45, DirDiff::Right45});
	rig.BoreOneCut();
	rig.ExchangeAndReturn();
	REQUIRE_FALSE(IsDiagonalDirection(rig.Job().route[1].heading)); // The second cut is the first bend.
	const auto estimate = EstimateExcavationCut(rig.project.job);
	REQUIRE(estimate.error == ExcavationError::None);
	const uint64_t half = estimate.work_ticks / 2;
	for (uint64_t tick = 0; tick < half * 2; ++tick) REQUIRE(rig.Tick().blocked == TBMBlockedReason::None); // Reach the same geological midpoint at half speed.
	const auto projects = ComputeTBMProjectDigest();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto earth = rig.Job().completed_earth_t;
	rig.Reload();
	CHECK(ComputeTBMProjectDigest() == projects);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(rig.Job().cursor == 1);
	CHECK(rig.Job().partial_work_ticks == half);
	// The rest of the work takes exactly the remaining ticks, and the ledger grows by this cut alone.
	for (uint64_t tick = half * 2 + 1; tick < estimate.work_ticks * 2; ++tick) REQUIRE(rig.Tick().phase == TBMPhase::Bore);
	CHECK(rig.Tick().phase == TBMPhase::ReturnForExchange); // The wagons cannot take the third cut as well: a frozen breakpoint.
	CHECK(rig.Job().cursor == 2);
	CHECK(rig.Job().completed_earth_t == earth + estimate.earth_t);
	CHECK(rig.Project().heading == rig.Job().route[1].heading);
}
