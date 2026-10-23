/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file tbm_face_stand.cpp Pure native stand geometry over declared actual cutter placements.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md paid work-path ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group20B.
 * Adapted donor stand cases retain independent quantities/geometry; no controller or saved phase is activated.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "../safeguards.h"

/**
 * Check a stand against the path rules: two cells behind the rearmost part, plain rail, and the laid piece's forward
 * direction towards the cutter.
 * @param stand Found stand.
 * @param path Work path through the completed cuts.
 * @param tail Path index of the rearmost part.
 */
static void CheckStand(const TBMFaceStand &stand, const std::vector<TileIndex> &path, size_t tail)
{
	REQUIRE(tail >= 3);
	CHECK(stand.tile == path[tail - 2]);
	CHECK(IsPlainRailTile(stand.tile));
	CHECK(GetTileTrackStatus(stand.tile, TransportType::Rail, RoadTramType::Invalid).trackdirs.Test(stand.trackdir));
	CHECK(TrackdirToExitdir(stand.trackdir) == DiagdirBetweenTiles(stand.tile, path[tail - 1]));
}

/** Task 2.6: on a straight route the stand is one clear cell behind the consist, laid towards it, and pure. */
TEST_CASE("The stand lies one clear cell behind the consist on a straight route", "[tbm-face-stand]")
{
	TBMExchangeRig rig(18);
	const size_t parts = CountTrainParts(*rig.front);
	// Enough cuts that a stand still exists with the front two cuts short of the face.
	const uint cuts = static_cast<uint>(parts) + 5;
	REQUIRE(cuts < rig.Job().route.size());
	for (uint i = 0; i < cuts; ++i) {
		rig.BoreOneCut();
		rig.ExchangeAndReturn();
	}
	const auto path = CompletedWorkPath(rig);
	const size_t face = path.size() - 1;

	SECTION("One part per cell") {
		const size_t tail = face - (parts - 1);
		PlaceTrainOnPath(rig.front, path, face, tail);
		const auto before = rig.Digest();
		const auto stand = FindTBMFaceStand(rig.Project());
		REQUIRE(stand.has_value());
		CheckStand(*stand, path, tail);
		CHECK(rig.Digest() == before); // The query changes nothing.
	}
	SECTION("Two parts per cell move the stand forward") {
		const size_t tail = face - (parts - 1) / 2;
		PlaceTrainOnPath(rig.front, path, face, tail);
		const auto stand = FindTBMFaceStand(rig.Project());
		REQUIRE(stand.has_value());
		CheckStand(*stand, path, tail);
	}
}

/** Task 2.6: after 45-degree bends the stand can be a side cell, and carries that cell's laid corner piece. */
TEST_CASE("After two bends the stand follows the side cells", "[tbm-face-stand]")
{
	// Two diagonal advances between the bends, then a straight run long enough for the consist to pass them.
	std::vector<DirDiff> turns{DirDiff::Same, DirDiff::Same, DirDiff::Right45, DirDiff::Same, DirDiff::Left45};
	turns.insert(turns.end(), 14, DirDiff::Same);
	TBMExchangeRig rig(turns);
	const size_t parts = CountTrainParts(*rig.front);
	REQUIRE(parts + 4 <= 14);
	bool side_stand = false;
	bool any_stand = false;
	// Each further cut moves the face, so the stand behind a whole consist passes over every path cell, the side cells included.
	while (rig.Job().cursor + 1 < rig.Job().route.size()) {
		rig.BoreOneCut();
		rig.ExchangeAndReturn();
		const auto path = CompletedWorkPath(rig);
		const size_t face = path.size() - 1;
		if (face < parts + 2) continue;
		const size_t tail = face - (parts - 1);
		PlaceTrainOnPath(rig.front, path, face, tail);
		const auto stand = FindTBMFaceStand(rig.Project());
		if (!stand.has_value()) continue; // The next cut's sweep may cover the cell; another case covers that rule.
		any_stand = true;
		CheckStand(*stand, path, tail);
		// A side cell is a path cell that is no cutter position: the diagonal advance only passes through it.
		const auto &route = rig.Job().route;
		const bool position = std::ranges::any_of(route, [&stand](const ExcavationCutGeometry &cut) { return cut.cutter_position == stand->tile; });
		if (!position) {
			side_stand = true;
			CHECK(IsDiagonalTrackdir(stand->trackdir) == false); // A side cell carries a corner piece.
		}
	}
	CHECK(any_stand);
	CHECK(side_stand);
}

/** Task 2.6: no stand while the consist is in the housing, too close to it, off its path, or in front of the next sweep. */
TEST_CASE("No stand in the housing, on a short route or in the next sweep", "[tbm-face-stand]")
{
	TBMExchangeRig rig(14);
	const size_t parts = CountTrainParts(*rig.front);
	for (uint i = 0; i < 5; ++i) {
		rig.BoreOneCut();
		rig.ExchangeAndReturn();
	}
	auto path = CompletedWorkPath(rig);
	const size_t face = path.size() - 1;
	REQUIRE(face == 5);
	REQUIRE(face - 3 <= parts + 1); // Both short placements stay within the searched tail.

	SECTION("Short route") {
		PlaceTrainOnPath(rig.front, path, face, 3);
		const auto stand = FindTBMFaceStand(rig.Project());
		REQUIRE(stand.has_value());
		CheckStand(*stand, path, 3); // The first cell after the housing.
		PlaceTrainOnPath(rig.front, path, face, 2);
		CHECK_FALSE(FindTBMFaceStand(rig.Project()).has_value()); // The stand would be the housing.
	}
	SECTION("Partly in the housing") {
		PlaceTrainOnPath(rig.front, path, face, 0);
		CHECK_FALSE(FindTBMFaceStand(rig.Project()).has_value());
	}
	SECTION("A part off the path") {
		PlaceTrainOnPath(rig.front, path, face, 3);
		Train *last = rig.front->Last();
		last->tile = rig.fixture.request.portal; // Still on the Surface side of the shaft.
		last->UpdatePosition();
		CHECK_FALSE(FindTBMFaceStand(rig.Project()).has_value());
	}
	SECTION("The next cut's sweep") {
		for (uint i = 0; i < 6; ++i) {
			rig.BoreOneCut();
			rig.ExchangeAndReturn();
		}
		path = CompletedWorkPath(rig);
		const size_t end = path.size() - 1;
		const auto &swept = rig.Job().route[rig.Job().cursor].swept_tiles;
		bool inside = false, outside = false;
		// Packing the consist onto fewer cells moves the candidate cell forward into the planned trail of the next cut.
		for (size_t tail = end - std::min(end, parts - 1); tail <= end; ++tail) {
			if (tail < 3) continue;
			PlaceTrainOnPath(rig.front, path, end, tail);
			const bool swept_cell = std::ranges::find(swept, path[tail - 2]) != swept.end();
			const auto stand = FindTBMFaceStand(rig.Project());
			INFO("tail " << tail);
			CHECK(stand.has_value() == !swept_cell);
			(swept_cell ? inside : outside) = true;
		}
		CHECK(inside);
		CHECK(outside);
	}
}


/** A folded paid prefix needs a physically clear native approach, not just a clear candidate endpoint.
	* Extra branch rail/support rows below are explicit retained-world components, not a construction or payment claim.
	*/
TEST_CASE("Folded face stand refuses a blocked prefix and accepts only an owned declared corridor approach", "[tbm-face-stand][tbm-face-folded-stand]")
{
	AutoRestoreBackup turns_allowed(_settings_game.pf.forbid_90_deg, false);
	std::vector<DirDiff> turns(15, DirDiff::Same);
	const std::array bend{DirDiff::Right45, DirDiff::Same, DirDiff::Right45, DirDiff::Same, DirDiff::Right45, DirDiff::Same, DirDiff::Right45, DirDiff::Same, DirDiff::Same,
	 DirDiff::Right45, DirDiff::Same, DirDiff::Right45, DirDiff::Same, DirDiff::Right45, DirDiff::Same, DirDiff::Right45, DirDiff::Same};
	turns.insert(turns.end(), bend.begin(), bend.end()); TBMExchangeRig rig(turns);
	// Complete the native folded support prefix. Explicit placement puts the
	// six actual cutter parts around its returning bend, blocking the earlier stem.
	for (size_t i = 0; i + 1 < turns.size(); ++i) { rig.BoreOneCut(); rig.ExchangeAndReturn(); }
	const auto path = CompletedWorkPath(rig); REQUIRE(path.size() == 40); REQUIRE(CountTrainParts(*rig.front) == 6);
	REQUIRE(path[32] == TileXYL(24, 30, MapLayer::Underground)); REQUIRE(path[34] == TileXYL(22, 30, MapLayer::Underground));
	REQUIRE(path[39] == TileXYL(20, 33, MapLayer::Underground)); PlaceTrainOnPath(rig.front, path, 39, 34);
	CHECK_FALSE(FindTBMFaceStand(rig.Project()).has_value());
	const std::array branch{TileXYL(20, 28, MapLayer::Underground), TileXYL(21, 28, MapLayer::Underground),
	 TileXYL(22, 28, MapLayer::Underground), TileXYL(23, 28, MapLayer::Underground), TileXYL(24, 28, MapLayer::Underground),
	 TileXYL(24, 29, MapLayer::Underground), TileXYL(24, 30, MapLayer::Underground)};
	rig.fixture.world.Open(branch); auto support = CaptureTBMSupport(); REQUIRE_FALSE(support.empty()); const auto original = support.front();
	// These explicit native snapshot components provide the alternate branch.
	// Bare owned rail alone must not borrow the current project's paid corridor.
	for (const auto tile : branch) MakeRailNormal(tile, rig.Project().company, TRACK_BIT_ALL, RAILTYPE_RAIL);
	CHECK_FALSE(FindTBMFaceStand(rig.Project()).has_value());
	// Publish original support facts through their native restoration boundary;
	// no private registry edits, duplicate geometry owner or stand cache exists.
	for (const auto tile : branch) {
	 auto at = std::ranges::lower_bound(support, tile, {}, &TBMSupportTile::tile);
	 // A missing branch cell needs its own original support row; an existing
	 // one retains identity and only gains the declared native junction pieces.
	 if (at == support.end() || at->tile != tile) { auto row = original; row.tile = tile; row.track = TRACK_BIT_ALL; support.insert(at, row); }
	 else at->track = TRACK_BIT_ALL;
	}
	StageLoadedTBMSupport(support); REQUIRE(AcceptLoadedTBMSupport(GetNextTBMProjectID()));
	const auto before = rig.Digest(); const auto original_support = CaptureTBMSupport();
	const auto stand = FindTBMFaceStand(rig.Project()); REQUIRE(stand.has_value()); CheckStand(*stand, path, 34);
	CHECK(rig.Digest() == before); CHECK(CaptureTBMSupport() == original_support);
	auto limited = support; auto middle = std::ranges::find(limited, branch[3], &TBMSupportTile::tile); REQUIRE(middle != limited.end());
	middle->allowed.Reset(TBMSupportClass::Hauler); StageLoadedTBMSupport(limited); REQUIRE(AcceptLoadedTBMSupport(GetNextTBMProjectID()));
	CHECK_FALSE(FindTBMFaceStand(rig.Project()).has_value()); StageLoadedTBMSupport(support); REQUIRE(AcceptLoadedTBMSupport(GetNextTBMProjectID()));
	CHECK(FindTBMFaceStand(rig.Project()).has_value());
	// An ownership mismatch cannot lend a route even when old support metadata
	// still names this project; observing it does not repair the damaged input.
	SetTileOwner(branch[3], OWNER_NONE); CHECK_FALSE(FindTBMFaceStand(rig.Project()).has_value()); SetTileOwner(branch[3], rig.Project().company);
	CHECK(FindTBMFaceStand(rig.Project()).has_value());
}
