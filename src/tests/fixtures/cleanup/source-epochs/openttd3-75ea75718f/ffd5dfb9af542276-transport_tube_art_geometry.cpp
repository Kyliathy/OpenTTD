/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_art_geometry.cpp Actual native tube TrackBits admitted for source-art composition.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups47D-47E
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_fixture.h"
#include "../track_func.h"
#include "../pathfinder/follow_track.hpp"

#include "../safeguards.h"

/**
 * Characterize every actual flat native layout before selecting junction source variants.
 * Native rail commands/tables remain the physical topology owner; fixture-open floor is explicit input.
 */
TEST_CASE("Tube source art covers actual native junction track combinations", "[transport-tube-art-geometry]")
{
	TubePrototypeRig world;
	uint junctions = 0, independent = 0, single = 0;
	// Every possible nonempty six-track layout gets its own cell. Real quote
	// and execute commands must admit exactly those pieces, so source art does
	// not infer a topology from a caption, shared catchment or fake tile owner.
	for (uint mask = 1; mask < 64; ++mask) {
		CAPTURE(mask);
		const TileIndex tile = TileXYL(48 + (mask - 1) % 8, 48 + (mask - 1) / 8, MapLayer::Underground);
		world.Open(std::array{tile});
		const TrackBits expected{static_cast<uint8_t>(mask)};
		TrackBits built;
		DiagDirectionIndexArray<uint8_t> degree{};
		uint pieces = 0;
		// Quote before every actual piece, checking that estimates retain the
		// already installed native layout. Each successful execution adds only
		// its requested track; physical edge identities come from native tables.
		for (Track track : expected) {
			REQUIRE(CmdBuildSingleRail({}, tile, world.tube, track, false).Succeeded());
			if (pieces != 0) CHECK(GetTrackBits(tile) == built);
			REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, tile, world.tube, track, false).Succeeded());
			built.Set(track);
			CHECK(GetTrackBits(tile) == built);
			CHECK(GetRailType(tile) == world.tube);
			const Trackdir direction = TrackToTrackdir(track);
			++degree[TrackdirToExitdir(direction)];
			++degree[TrackdirToExitdir(ReverseTrackdir(direction))];
			++pieces;
		}
		CHECK(GetTrackBits(tile) == expected);
		// Sharing a native edge requires a junction casing. X/Y crossings and
		// the two independent parallel-curve pairs share no edge and keep their
		// own render cases rather than acquiring an invented connecting arm.
		const bool shared_edge = std::ranges::max(degree) > 1;
		if (shared_edge) {
			++junctions;
		} else if (pieces > 1) {
			++independent;
		} else {
			++single;
		}
	}
	CHECK(junctions == 54);
	CHECK(independent == 3);
	CHECK(single == 6);
}

/**
 * Characterize physical ends for all six native pieces without creating a service or a saved topology owner.
 * The public presentation query must retain track, funds and utility state while neighbours change through commands.
 */
TEST_CASE("Tube end artwork follows actual native physical edge connectivity", "[transport-tube-end-geometry]")
{
	TubePrototypeRig world;
	// Separate sites cover both mouths of every native track. Wrong-facing and
	// conventional neighbours remain ends; a matching actual tube clears only
	// its physical mouth even though this fixture buys no electrical service.
	for (uint index = 0; index < 6; ++index) {
		const auto track = static_cast<Track>(index);
		const TileIndex tile = TileXYL(36 + index * 4, 52, MapLayer::Underground);
		world.BuildTrack(tile, world.tube, track);
		CHECK(std::ranges::count(GetTubeGuidewayEnds(tile), true) == 2);
		const Trackdir first = TrackToTrackdir(track);
		for (Trackdir direction : {first, ReverseTrackdir(first)}) {
			const auto port = TrackdirToExitdir(direction);
			const auto delta = TileIndexDiffCByDiagDir(port);
			const TileIndex neighbour = TileAddWrap(tile, delta.x, delta.y);
			const Track matching = DiagDirToDiagTrack(port);
			const Track wrong = matching == Track::X ? Track::Y : Track::X;
			CHECK(GetTubeGuidewayEnds(tile)[port]);
			world.BuildTrack(neighbour, world.tube, wrong);
			CHECK(GetTubeGuidewayEnds(tile)[port]);
			REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, neighbour, wrong).Succeeded());
			world.BuildTrack(neighbour, RAILTYPE_RAIL, matching);
			CHECK(GetTubeGuidewayEnds(tile)[port]);
			REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, neighbour, matching).Succeeded());
			world.BuildTrack(neighbour, world.tube, matching);
			CHECK_FALSE(GetTubeGuidewayEnds(tile)[port]);
		}
		CHECK(std::ranges::count(GetTubeGuidewayEnds(tile), true) == 0);
		const auto money = Company::Get(_current_company)->money;
		const auto utility = ComputeTubePowerDigest();
		// Repeated viewport-style reads cannot charge a company, allocate power,
		// erase a track or turn visual geometry into a second world-state owner.
		for (uint query = 0; query < 8; ++query) CHECK(std::ranges::count(GetTubeGuidewayEnds(tile), true) == 0);
		CHECK(GetTrackBits(tile) == TrackBits{track});
		CHECK(Company::Get(_current_company)->money == money);
		CHECK(ComputeTubePowerDigest() == utility);
	}
	CHECK(std::ranges::count(GetTubeGuidewayEnds(INVALID_TILE), true) == 0);
	CHECK(std::ranges::count(GetTubeGuidewayEnds(world.depot), true) == 0);
	const TileIndex ordinary = TileXYL(32, 52, MapLayer::Underground);
	world.BuildTrack(ordinary, RAILTYPE_RAIL);
	CHECK(std::ranges::count(GetTubeGuidewayEnds(ordinary), true) == 0);
}

/** Native depot entry sides and tube turn restrictions are distinct from physical end decoration. */
TEST_CASE("Tube end artwork respects depot doors and retains open policy-blocked mouths", "[transport-tube-end-geometry]")
{
	TubePrototypeRig world;
	const auto front = TileAddWrap(world.depot, 0, 1), rear = TileAddWrap(world.depot, 0, -1);
	world.BuildTrack(front, world.tube, Track::Y);
	world.BuildTrack(rear, world.tube, Track::Y);
	CHECK_FALSE(GetTubeGuidewayEnds(front)[DiagDirection::NW]);
	CHECK(GetTubeGuidewayEnds(rear)[DiagDirection::SE]);
	CHECK(std::ranges::count(GetTubeGuidewayEnds(world.depot), true) == 0);
	const auto source = TileXYL(44, 44, MapLayer::Underground);
	const auto next = TileXYL(43, 44, MapLayer::Underground);
	world.BuildTrack(source, world.tube, Track::Upper);
	world.BuildTrack(next, world.tube, Track::Left);
	// A sharp-turn rule may refuse a capsule while the native mouths still
	// physically meet. Showing a sealed cap for that wait would misrepresent
	// topology and let policy/signal/power state redefine the source artwork.
	CHECK_FALSE(GetTubeGuidewayEnds(source)[DiagDirection::NE]);
	Train *capsule = world.BuyCapsule();
	CFollowTrackRail follower(capsule);
	CHECK_FALSE(follower.Follow(source, Trackdir::Upper_E));
	CHECK(follower.err == CFollowTrackRail::ErrorCode::SharpTurn);
}
