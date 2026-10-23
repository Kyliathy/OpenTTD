/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_meeting.cpp The meeting point behind the cutter: where the stand lies on the work path, which phases
 * meet at the face, and which haulers count as inside the project's corridor.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 2 (task 2.6)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-a-face-exchange-design.md (rules F1, F5 and F7)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include <chrono>
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

/**
 * @param project Project to copy.
 * @param phase Phase to give the copy.
 * @return The copy, read by the pure queries as if the project were in that phase.
 */
static TBMProject InPhase(const TBMProject &project, TBMPhase phase)
{
	TBMProject copy = project;
	copy.phase = phase;
	return copy;
}

/** Every saved phase, oldest first. */
static constexpr std::array<TBMPhase, 21> ALL_PHASES{
	TBMPhase::Commissioning, TBMPhase::TravelToFace, TBMPhase::Bore, TBMPhase::AdvanceSupport, TBMPhase::ReturnForExchange,
	TBMPhase::WaitForEmpties, TBMPhase::Resume, TBMPhase::Completed, TBMPhase::Cancelled, TBMPhase::ReturnToPark,
	TBMPhase::WaitToPark, TBMPhase::Parked, TBMPhase::ReturnToRelease, TBMPhase::WaitToRelease, TBMPhase::ReleasePending,
	TBMPhase::ReturnForHangar, TBMPhase::WaitForHangar, TBMPhase::TravelToHangar, TBMPhase::ParkedHangar,
	TBMPhase::FollowOrders, TBMPhase::AwaitFaceTransfer,
};

/**
 * @param phase Phase.
 * @return Whether the cutter works at its face in it.
 */
static bool IsWorkingPhase(TBMPhase phase)
{
	return phase == TBMPhase::Bore || phase == TBMPhase::AdvanceSupport || phase == TBMPhase::AwaitFaceTransfer;
}

/** Task 2.6: on a straight route the stand is one clear cell behind the consist, laid towards it, and pure. */
TEST_CASE("The stand lies one clear cell behind the consist on a straight route", "[demo-fix-face-meeting]")
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
	SECTION("Every phase") {
		PlaceTrainOnPath(rig.front, path, face, face - (parts - 1));
		// On the face: working and returning phases meet there, every other phase at the bay.
		for (TBMPhase phase : ALL_PHASES) {
			INFO("phase " << to_underlying(phase));
			const bool face_phase = IsWorkingPhase(phase) || IsTBMReturning(phase);
			CHECK(GetTBMMeetingKind(InPhase(rig.Project(), phase)) == (face_phase ? TBMMeetingKind::Face : TBMMeetingKind::Bay));
		}
		// One cut short of the face, still rolling onto the new support: only working phases meet at the face.
		PlaceTrainOnPath(rig.front, path, face - 1, face - parts);
		for (TBMPhase phase : ALL_PHASES) {
			INFO("phase " << to_underlying(phase));
			CHECK(GetTBMMeetingKind(InPhase(rig.Project(), phase)) == (IsWorkingPhase(phase) ? TBMMeetingKind::Face : TBMMeetingKind::Bay));
		}
		// Two cuts short: the consist has left the face; every phase meets at the bay, although a stand exists.
		PlaceTrainOnPath(rig.front, path, face - 2, face - 1 - parts);
		CHECK(FindTBMFaceStand(rig.Project()).has_value());
		for (TBMPhase phase : ALL_PHASES) {
			INFO("phase " << to_underlying(phase));
			CHECK(GetTBMMeetingKind(InPhase(rig.Project(), phase)) == TBMMeetingKind::Bay);
		}
	}
}

/** Task 2.6: readiness needs the face, the right phase and spoil that makes the exchange due. */
TEST_CASE("A face transfer is ready only at the face with spoil aboard", "[demo-fix-face-meeting]")
{
	TBMExchangeRig rig(14);
	const size_t parts = CountTrainParts(*rig.front);
	const uint cuts = static_cast<uint>(parts) + 3;
	REQUIRE(cuts < rig.Job().route.size());
	for (uint i = 0; i + 1 < cuts; ++i) {
		rig.BoreOneCut();
		rig.ExchangeAndReturn();
	}
	rig.BoreOneCut(); // The last cut's spoil stays aboard.
	const auto path = CompletedWorkPath(rig);
	PlaceTrainOnPath(rig.front, path, path.size() - 1, path.size() - parts);
	const TBMProject &project = rig.Project();
	// Parking settles any spoil aboard, so a cutter sent back from its face is ready while a hauler can reach it.
	CHECK(IsTBMReadyForFaceTransfer(InPhase(project, TBMPhase::ReturnToPark)));
	CHECK(IsTBMReadyForFaceTransfer(InPhase(project, TBMPhase::ReturnToRelease)));
	// The face wait is ready exactly when its breakpoint makes the exchange due.
	const auto waiting = InPhase(project, TBMPhase::AwaitFaceTransfer);
	CHECK(IsTBMReadyForFaceTransfer(waiting) == IsTBMExchangeDue(waiting));
	// Cutting, travelling and bay phases never take a face transfer.
	for (TBMPhase phase : {TBMPhase::Bore, TBMPhase::AdvanceSupport, TBMPhase::TravelToFace, TBMPhase::WaitToPark, TBMPhase::Parked}) {
		INFO("phase " << to_underlying(phase));
		CHECK_FALSE(IsTBMReadyForFaceTransfer(InPhase(project, phase)));
	}
	// Away from the face the bay settles the spoil instead.
	PlaceTrainOnPath(rig.front, path, path.size() - 2, path.size() - 1 - parts);
	CHECK_FALSE(IsTBMReadyForFaceTransfer(InPhase(project, TBMPhase::ReturnToPark)));
	// Without spoil there is nothing to transfer.
	PlaceTrainOnPath(rig.front, path, path.size() - 1, path.size() - parts);
	rig.ExchangeAndReturn();
	CHECK_FALSE(IsTBMReadyForFaceTransfer(InPhase(rig.Project(), TBMPhase::ReturnToPark)));
}

/** Task 2.6: after 45-degree bends the stand can be a side cell, and carries that cell's laid corner piece. */
TEST_CASE("After two bends the stand follows the side cells", "[demo-fix-face-meeting]")
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
TEST_CASE("No stand in the housing, on a short route or in the next sweep", "[demo-fix-face-meeting]")
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
		CHECK(GetTBMMeetingKind(InPhase(rig.Project(), TBMPhase::Bore)) == TBMMeetingKind::Bay);
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
		CHECK(GetTBMMeetingKind(InPhase(rig.Project(), TBMPhase::Bore)) == TBMMeetingKind::Bay);
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

/** Task 2.6: only the project's assigned haulers on the running line count as inside; the lowest is named. */
TEST_CASE("Haulers inside the corridor", "[demo-fix-face-meeting]")
{
	TBMExchangeRig rig(10);
	for (uint i = 0; i < 4; ++i) {
		rig.BoreOneCut();
		rig.ExchangeAndReturn();
	}
	rig.Assign();
	Train *second = rig.BuildHauler(rig.depot, 2, 2);
	REQUIRE(AssignTBMHauler(rig.project.id, second->index, true) == TBMExchangeError::None);
	Train *stranger = rig.BuildHauler(rig.depot, 2, 2); // An ordinary train, not assigned to the project.
	const TBMProject &project = rig.Project();
	const TileIndex key = rig.fixture.request.portal;
	const TileIndex housing = GetLayerPortalPairTile(key);
	const TileIndex support = rig.Job().route[1].cutter_position;

	// In their depot every train is outside.
	CHECK_FALSE(IsTBMHaulerInCorridor(project, rig.hauler->index));
	CHECK(FindTBMHaulerInCorridor(project) == VehicleID::Invalid());
	// Stored in the bay's magazine, as a queued hauler waits, it is still off the running line.
	rig.HaulerToSourceBay();
	CHECK_FALSE(IsTBMHaulerInCorridor(project, rig.hauler->index));
	// On the support, or on either end of the pair, it is inside.
	for (TileIndex tile : {support, key, housing}) {
		rig.PlaceOnTrack(rig.hauler, tile);
		CHECK(IsTBMHaulerInCorridor(project, rig.hauler->index));
		CHECK(FindTBMHaulerInCorridor(project) == rig.hauler->index);
	}
	// An unassigned train inside is not one of the project's haulers.
	rig.PlaceInDepot(rig.hauler, rig.depot);
	rig.PlaceOnTrack(stranger, support);
	CHECK_FALSE(IsTBMHaulerInCorridor(project, stranger->index));
	CHECK(FindTBMHaulerInCorridor(project) == VehicleID::Invalid());
	// With two haulers inside, the lowest vehicle ID is named.
	rig.PlaceInDepot(stranger, rig.depot);
	rig.PlaceOnTrack(second, rig.Job().route[2].cutter_position);
	CHECK(FindTBMHaulerInCorridor(project) == second->index);
	rig.PlaceOnTrack(rig.hauler, support);
	CHECK(FindTBMHaulerInCorridor(project) == std::min(rig.hauler->index, second->index));
}

/**
 * Task 2.6 performance bound. The 64x64 fixture map cannot hold a live job with 1,024 completed cuts, so this times the
 * only part of every face query that grows with the route: the work-rail path, here over a synthetic 1,024-cut job
 * that snakes across one layer with straight and diagonal advances. The stand search itself reads only the last
 * N + 2 path cells for a consist of N parts.
 */
TEST_CASE("Face queries stay bounded on a 1,024-cut route", "[demo-fix-face-meeting]")
{
	ExcavationFixture fixture;
	ExcavationJob job;
	job.commissioning_anchor = TileXYL(2, 2, MapLayer::Underground);
	int x = 3, y = 2, step = 1;
	// Rows run east and west; each row ends with a diagonal advance onto the next row, which needs a side cell.
	while (job.route.size() < 1024) {
		ExcavationCutGeometry cut;
		cut.cutter_position = TileXYL(x, y, MapLayer::Underground);
		job.route.push_back(cut);
		if ((step > 0 && x == 60) || (step < 0 && x == 3)) {
			x += step;
			++y;
			step = -step;
		} else {
			x += step;
		}
	}
	job.cursor = 1024;
	std::vector<size_t> ends;
	const auto start = std::chrono::steady_clock::now();
	for (int i = 0; i < 1000; ++i) {
		const auto path = GetTBMWorkRailPath(job, job.cursor, ends);
		REQUIRE(ends.size() == 1024);
	}
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
	INFO("1000 paths over 1,024 cuts took " << elapsed << " ms");
	CHECK(elapsed < 4000); // Under 4 ms per query even in Debug; the release build measures far less.
}
