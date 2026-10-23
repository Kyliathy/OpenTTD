/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_corridor.cpp Work reservations separate from ordinary freight (Group 14): the corridor by project, path
 * finding and tile entry that refuse other trains even when forced, and release that waits for withdrawal.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 14
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../tile_cmd.h"
#include "../train_cmd.h"
#include "../rail_cmd.h"
#include "../command_func.h"
#include "../pathfinder/follow_track.hpp"

#include "../safeguards.h"

/**
 * Whether the train's moving front may enter a tile.
 * @param front Train.
 * @param tile Tile to enter.
 * @return Whether the tile accepts it.
 */
static bool CanEnter(Train *front, TileIndex tile)
{
	return !VehicleEnterTile(front, tile, TileX(tile) * TILE_SIZE + TILE_SIZE / 2, TileY(tile) * TILE_SIZE + TILE_SIZE / 2).Test(VehicleEnterTileState::CannotEnter);
}

/**
 * Build a second consist on another pair: the same company and model, but not the rig's project.
 * @param rig Rig whose request is copied.
 * @return The second consist's front engine.
 */
static Train *BuildOtherConsist(const TBMProjectRig &rig)
{
	const auto second = TileXYL(30, 20, MapLayer::Surface);
	REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, second, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
	auto request = rig.fixture.request;
	request.portal = second;
	request.proposal = rig.base;
	request.proposal.commissioning_anchor = GetLayerPortalPairTile(second);
	Train *other = nullptr;
	const std::array<const Engine *, 1> wagons{Engine::Get(GetTBMEngine(2))};
	REQUIRE(BuildTBMConsistAtSite(request, Engine::Get(GetTBMEngine(0)), 0, GetFreeUnitNumber(VehicleType::Train), wagons, &other).Succeeded());
	return other;
}

/** The project's consist follows and enters its support; the same train without a project is refused both ways. */
TEST_CASE("Ordinary trains can neither route nor drive into a work corridor", "[tbm-corridor]")
{
	TBMWorkRig rig(2);
	rig.BoreOneCut();
	rig.ExchangeAndReturn();
	rig.BoreOneCut();
	const TileIndex first = rig.Job().route[0].cutter_position;
	const TileIndex second = rig.Job().route[1].cutter_position;
	const auto corridor = GetTBMWorkCorridor(rig.Project());
	for (TileIndex tile : {first, second, rig.proof.destination}) CHECK(std::ranges::binary_search(corridor, tile));

	const Trackdir along = DiagDirToDiagTrackdir(GetLayerPortalDoorDirection(rig.proof.destination));
	CFollowTrackRail own(rig.front);
	CHECK(own.Follow(first, along));
	CHECK(own.new_tile == second);
	CHECK(CanEnter(rig.front, second));

	/* Without its project the same train is ordinary freight: no route, no entry, and forcing does not help. */
	const auto saved = CaptureTBMProjects();
	const auto support = CaptureTBMSupport();
	const auto next = GetNextTBMProjectID();
	ResetTBMProjectState();
	StageLoadedTBMSupport(support);
	StageLoadedTBMProjects({}, next);
	REQUIRE(AcceptLoadedTBMProjects()); // The support stays, as if its project were gone.
	CFollowTrackRail freight(rig.front);
	CHECK_FALSE(freight.Follow(first, along));
	CHECK(freight.err == CFollowTrackRail::ErrorCode::NoWay);
	CHECK_FALSE(CanEnter(rig.front, second));
	rig.front->force_proceed = TFP_SIGNAL;
	CHECK_FALSE(CanEnter(rig.front, second));
	rig.front->force_proceed = TFP_NONE;
	StageLoadedTBMSupport(support);
	StageLoadedTBMProjects(saved, next);
	REQUIRE(AcceptLoadedTBMProjects());
	CHECK(CanEnter(rig.front, second));
}

/** Release waits for withdrawal: the corridor sees any train on it, and released support under a train stays. */
TEST_CASE("Released support stays while a train stands on it", "[tbm-corridor]")
{
	TBMWorkRig rig;
	rig.BoreOneCut();
	const TileIndex cell = rig.Job().route[0].cutter_position;
	const Track laid = *GetTrackBits(cell).begin();
	CHECK_FALSE(IsTBMCorridorOccupied(rig.Project())); // The consist is still at the source bay.

	Train *other = BuildOtherConsist(rig);
	Train *intruder = other->Last();
	const TileIndex home = intruder->tile;
	const TrackBits home_track = intruder->track;
	intruder->tile = cell;
	intruder->track = TrackBits{laid};
	intruder->UpdatePosition();
	CHECK(IsTBMCorridorOccupied(rig.Project()));

	rig.Reload(TBMPhase::Completed);
	// Since R2TBMF (2026-10-09) a train standing on released support keeps it in use, so a hidden tail is never
	// converted under a train; the removal refusal below is the same protection seen from the ordinary rail tool.
	CHECK(IsTBMSupportInUse(cell));
	CHECK(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, cell, laid).Failed()); // A train is on it.
	CHECK(GetTBMSupportTile(cell) != nullptr);
	intruder->tile = home;
	intruder->track = home_track;
	intruder->UpdatePosition();
	CHECK_FALSE(IsTBMSupportInUse(cell)); // Released once the train has gone.
	CHECK(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, cell, laid).Succeeded());
	CHECK(GetTBMSupportTile(cell) == nullptr);
}
