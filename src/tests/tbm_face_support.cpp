/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file tbm_face_support.cpp Whole occupied-face refusal before a work-change support junction is prepared.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical track/vehicle ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23CB.
 * Train poses and a missing track piece are explicit fault inputs; no driven-world acceptance is claimed here.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "tbm_face_support_fixture.h"
#include "../safeguards.h"

/** An occupied face cannot switch its track piece even when its original support owner permits an empty-face junction. */
TEST_CASE("A changed face piece waits for actual whole train clearance", "[tbm-face-support]")
{
	TBMFaceRig rig(8);
	rig.Bore(2);
	const auto job = rig.Job();
	const TileIndex face = job.route[job.cursor - 1].cutter_position;
	/** @param vehicle Actual native occupant. @return Whether any physical vehicle occupies the fault target. */
	const auto occupied = [](const Vehicle *vehicle) { return vehicle != nullptr; };
	REQUIRE_FALSE(HasVehicleOnTile(face, occupied));
	CHECK(PrepareTBMFaceSupport(rig.Project(), job).pieces.empty());
	TBMFacePieceFault fault(face);
	const auto unoccupied = PrepareTBMFaceSupport(rig.Project(), job);
	REQUIRE(unoccupied.error == TBMBlockedReason::None);
	REQUIRE(unoccupied.pieces.size() == 1);
	CHECK(unoccupied.pieces.front().tile == face);
	rig.PlaceOnTrack(rig.hauler, face);
	const auto before = TBMFaceSnapshot::Capture(rig);
	const auto blocked = PrepareTBMFaceSupport(rig.Project(), job);
	CHECK(blocked.error == TBMBlockedReason::Obstruction);
	CHECK(blocked.failure_tile == face);
	CHECK(blocked.pieces.empty());
	CHECK(TBMFaceSnapshot::Capture(rig) == before);
	rig.PlaceInDepot(rig.hauler, rig.depot);
	const auto clear = PrepareTBMFaceSupport(rig.Project(), job);
	CHECK(clear.error == TBMBlockedReason::None);
	REQUIRE(clear.pieces.size() == 1);
	CHECK(clear.pieces.front().tile == unoccupied.pieces.front().tile);
	CHECK(clear.pieces.front().track == unoccupied.pieces.front().track);
}
