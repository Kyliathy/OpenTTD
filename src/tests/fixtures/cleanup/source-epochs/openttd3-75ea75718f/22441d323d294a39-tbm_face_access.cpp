/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_access.cpp The assigned hauler's way to the face: its pass through its own project's pair, the turn
 * that lets one hauler in at a time, the magazine queue, the support it may run on and the stand where a reserved path
 * may end. Ordinary and foreign trains and the cutter keep their rules.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 3 (task 3.6)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-a-face-exchange-design.md (rules F5 and F7)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "../layer_portal_cmd.h"
#include "../pbs.h"
#include "../tile_cmd.h"
#include "../settings_type.h"
#include "../safeguards.h"

/**
 * @param train Train whose parts are checked.
 * @param tile Support tile.
 * @return Whether the support carries every part of the train.
 */
static bool CarriesEveryPart(const Train &train, TileIndex tile)
{
	for (const Train *part = &train; part != nullptr; part = part->Next()) {
		if (!MayTBMSupportCarry(*part, tile)) return false;
	}
	return true;
}

/**
 * @param train Train to check.
 * @return The assignment of the train, which must exist.
 */
static const TBMExchangeAssignment &AssignmentOf(const Train &train)
{
	const auto *assignment = GetTBMExchangeAssignmentForHauler(train.index);
	REQUIRE(assignment != nullptr);
	return *assignment;
}

/** Task 3.6: the turn holder passes both ends of its own pair; a queuing hauler stays at the key; strangers stay out. */
TEST_CASE("Only the face turn holder passes its own pair", "[demo-fix-face-access]")
{
	TBMFaceRig rig;
	rig.BoreClearOfHousing();
	rig.StandAtFace();
	rig.Assign();
	Train *second = rig.AssignAnother();
	Train *stranger = rig.BuildHauler(rig.depot, 2, 2);
	const TileIndex key = rig.Key();
	const TileIndex housing = rig.Housing();
	const DiagDirection key_door = GetLayerPortalDoorDirection(key);
	const DiagDirection housing_door = GetLayerPortalDoorDirection(housing);
	const auto cutter_key = GetLayerPortalAdmittedTrackdirs(key, *rig.front, key_door);
	const auto cutter_housing = GetLayerPortalAdmittedTrackdirs(housing, *rig.front, DiagDirection::Invalid);
	REQUIRE(GetTBMMeetingKind(rig.Project()) == TBMMeetingKind::Face);
	REQUIRE(IsTBMFaceTurnHolder(AssignmentOf(*rig.hauler)));
	CHECK_FALSE(IsTBMFaceTurnHolder(AssignmentOf(*second)));

	for (const bool reserve : {false, true}) {
		INFO("reserve_paths " << reserve);
		AutoRestoreBackup reserve_paths(_settings_game.pf.reserve_paths, reserve);
		// The turn holder enters the key through its door and the housing by the shaft step, and is never stored.
		CHECK(GetLayerPortalAdmittedTrackdirs(key, *rig.hauler, key_door).Any());
		CHECK(GetLayerPortalAdmittedTrackdirs(housing, *rig.hauler, DiagDirection::Invalid).Any());
		CHECK(GetLayerPortalMagazineExit(key, *rig.hauler) == DiagDirection::Invalid);
		// The next hauler may only queue in the magazine, which it leaves through the door again.
		CHECK(GetLayerPortalAdmittedTrackdirs(key, *second, key_door).Any());
		CHECK_FALSE(GetLayerPortalAdmittedTrackdirs(housing, *second, DiagDirection::Invalid).Any());
		CHECK(GetLayerPortalMagazineExit(key, *second) == key_door);
		CHECK(IsSafeWaitingPosition(second, key, DiagDirToDiagTrackdir(ReverseDiagDir(key_door)), true, _settings_game.pf.forbid_90_deg));
		CHECK_FALSE(IsSafeWaitingPosition(second, key, DiagDirToDiagTrackdir(key_door), true, _settings_game.pf.forbid_90_deg)); // Only inward storage is a safe reservation endpoint; crossing or departing the housing never waits there.
		CHECK_FALSE(IsSafeWaitingPosition(rig.hauler, key, DiagDirToDiagTrackdir(ReverseDiagDir(key_door)), true, _settings_game.pf.forbid_90_deg)); // The face turn holder traverses the shaft instead of entering storage.
		// An ordinary train is refused at both ends.
		CHECK_FALSE(GetLayerPortalAdmittedTrackdirs(key, *stranger, key_door).Any());
		CHECK_FALSE(GetLayerPortalAdmittedTrackdirs(housing, *stranger, DiagDirection::Invalid).Any());
		// The cutter's admission is unchanged by haulers around it.
		CHECK(GetLayerPortalAdmittedTrackdirs(key, *rig.front, key_door) == cutter_key);
		CHECK(GetLayerPortalAdmittedTrackdirs(housing, *rig.front, DiagDirection::Invalid) == cutter_housing);
	}

	SECTION("The real tile entry checks the same rule") {
		/**
		 * Enter the housing from the shaft, heading out of its door onto the support.
		 * @param train Train whose front enters.
		 * @return The tile's answer.
		 */
		const auto enter = [housing, housing_door](Train &train) {
			train.direction = DiagDirToDir(housing_door);
			train.z_pos = GetSlopePixelZ(TileX(housing) * TILE_SIZE + TILE_SIZE / 2, TileY(housing) * TILE_SIZE + TILE_SIZE / 2, true);
			return VehicleEnterTile(&train, housing, TileX(housing) * TILE_SIZE + TILE_SIZE / 2, TileY(housing) * TILE_SIZE + TILE_SIZE / 2);
		};
		CHECK_FALSE(enter(*rig.hauler).Test(VehicleEnterTileState::CannotEnter));
		CHECK(enter(*second).Test(VehicleEnterTileState::CannotEnter));
		CHECK(enter(*stranger).Test(VehicleEnterTileState::CannotEnter));
	}
	SECTION("Away from the face the pair stays closed inbound, but a hauler inside gets out") {
		// Two cuts short of the face the cutter no longer meets there.
		const auto path = rig.Path();
		PlaceTrainOnPath(rig.front, path, path.size() - 3, path.size() - 2 - rig.parts);
		REQUIRE(GetTBMMeetingKind(rig.Project()) == TBMMeetingKind::Bay);
		CHECK_FALSE(GetLayerPortalAdmittedTrackdirs(housing, *rig.hauler, DiagDirection::Invalid).Any());
		CHECK(GetLayerPortalMagazineExit(key, *rig.hauler) == key_door); // It queues like any other hauler.
		// A part beyond the key, on the support, always lets it leave through both ends.
		rig.PlaceOnTrack(rig.hauler, rig.Job().route[0].cutter_position);
		CHECK(GetLayerPortalAdmittedTrackdirs(housing, *rig.hauler, housing_door).Any());
		CHECK(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, key));
		CHECK(GetLayerPortalMagazineExit(key, *rig.hauler) == DiagDirection::Invalid);
	}
}

/** Task 3.6: the turn goes to the lowest eligible hauler, never to a short one, and only while the corridor is free. */
TEST_CASE("The face turn and the magazine queue", "[demo-fix-face-access]")
{
	TBMFaceRig rig;
	rig.BoreClearOfHousing();
	rig.StandAtFace();
	Train *shorty = rig.AssignAnother(1, 2); // Lowest assignment ID, but its Earth hoppers cannot cover a pass.
	rig.Assign();
	Train *third = rig.AssignAnother();
	const TileIndex key = rig.Key();
	CHECK_FALSE(IsTBMFaceTurnHolder(AssignmentOf(*shorty)));
	CHECK(IsTBMFaceTurnHolder(AssignmentOf(*rig.hauler)));
	CHECK_FALSE(IsTBMFaceTurnHolder(AssignmentOf(*third)));

	// The third hauler queues in the magazine; it may not leave while the turn is elsewhere.
	rig.StoreInMagazine(third);
	CHECK_FALSE(MayTBMTrainLeaveMagazine(*third));
	CHECK(GetLayerPortalMagazineExit(key, *third) == GetLayerPortalDoorDirection(key));

	// The turn holder takes the load and drives out with it: while it is in the corridor nobody else gets the turn.
	Train *loaded_wagon = rig.hauler->GetNextUnit();
	REQUIRE(loaded_wagon != nullptr);
	REQUIRE(CargoPacket::CanAllocateItem());
	loaded_wagon->cargo.Append(CargoPacket::Create(StationID::Invalid(), 1, Source{Source::Invalid, SourceType::Industry}), VehicleCargoList::MoveToAction::Keep);
	rig.PlaceOnTrack(rig.hauler, rig.Job().route[0].cutter_position);
	CHECK_FALSE(IsTBMFaceTurnHolder(AssignmentOf(*rig.hauler))); // A loaded set is no empty for the face.
	CHECK_FALSE(IsTBMFaceTurnHolder(AssignmentOf(*third))); // Another hauler is still inside.
	CHECK_FALSE(MayTBMTrainLeaveMagazine(*third));

	// Once it has left, the queued hauler holds the turn and leaves the magazine through the shaft towards the face.
	rig.PlaceInDepot(rig.hauler, rig.depot);
	CHECK(IsTBMFaceTurnHolder(AssignmentOf(*third)));
	CHECK(MayTBMTrainLeaveMagazine(*third));
	CHECK(GetLayerPortalMagazineExit(key, *third) == GetLayerPortalShaftDirection(key));
	loaded_wagon->cargo.Truncate();
}

/** A changing face meeting cannot redirect only half of an actual magazine arrival/departure; the whole retained chain keeps its physical storage orientation (Plan18 8FA). */
TEST_CASE("A hauler completes partial magazine movement before changing its exit", "[tbm-hauler-magazine-transition]")
{
	TBMFaceRig rig; rig.BoreClearOfHousing(); rig.StandAtFace(); rig.Assign();
	const auto key = rig.Key(); const auto shaft = GetLayerPortalShaftDirection(key), door = GetLayerPortalDoorDirection(key);
	rig.PlaceOnTrack(rig.hauler, GetLayerPortalDoorTile(key));
	rig.hauler->tile = key; rig.hauler->track = Track::Depot; rig.hauler->direction = DiagDirToDir(shaft);
	REQUIRE(GetTBMMeetingKind(rig.Project()) == TBMMeetingKind::Face);
	CHECK(GetLayerPortalMagazineExit(key, *rig.hauler) == door);
	// Native whole-chain departure orients every stored part together. A
	// still-stored tail keeps that chosen shaft exit even if the live meeting
	// switches back to the bay while its engine is already moving out.
	for (Train *part = rig.hauler; part != nullptr; part = part->Next()) {
		part->tile = key; part->track = Track::Depot; part->direction = DiagDirToDir(shaft);
	}
	CHECK(GetLayerPortalMagazineExit(key, *rig.hauler) == shaft);
	rig.hauler->track = DiagDirToDiagTrack(shaft);
	const auto path = rig.Path(); PlaceTrainOnPath(rig.front, path, path.size() - 3, path.size() - 2 - rig.parts);
	REQUIRE(GetTBMMeetingKind(rig.Project()) == TBMMeetingKind::Bay);
	CHECK(GetLayerPortalMagazineExit(key, *rig.hauler) == shaft);
}

/** Task 3.6: support carries the project's own haulers and machinery, never an ordinary or foreign train. */
TEST_CASE("Support carries only the project's own trains", "[demo-fix-face-access]")
{
	TBMFaceRig rig;
	rig.Bore(3);
	rig.Assign();
	Train *stranger = rig.BuildHauler(rig.depot, 2, 2);
	const TileIndex support = rig.Job().route[1].cutter_position;
	REQUIRE(GetTBMSupportTile(support) != nullptr);
	CHECK(CarriesEveryPart(*rig.hauler, support));
	CHECK(CarriesEveryPart(*rig.front, support));
	CHECK(GetTBMSupportClass(*rig.hauler) == TBMSupportClass::Hauler);
	CHECK(GetTBMSupportClass(*rig.hauler->Last()) == TBMSupportClass::Hauler);
	CHECK(GetTBMSupportClass(*rig.front) == TBMSupportClass::Cutter);
	CHECK_FALSE(CarriesEveryPart(*stranger, support));
	CHECK_FALSE(GetTBMSupportClass(*stranger).has_value());

	// The same hauler under another company is no longer the owner's train.
	REQUIRE(Company::CanAllocateItem());
	Company *other = Company::Create();
	for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->owner = other->index;
	CHECK_FALSE(CarriesEveryPart(*rig.hauler, support));
	CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, rig.Key()));
	for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->owner = _current_company;
	delete other;
}

/** A native chamber shortcut cannot take an assigned hauler through its own project's actual cutter body; shared support and physical entry enforce the same clearance (Plan18 8FA). */
TEST_CASE("A hauler cannot enter support occupied by its own cutter", "[tbm-hauler-cutter-clearance]")
{
	TBMFaceRig rig; rig.BoreClearOfHousing(); rig.StandAtFace(); rig.Assign();
	const auto occupied = rig.front->tile; REQUIRE(GetTBMSupportTile(occupied) != nullptr);
	CHECK_FALSE(CarriesEveryPart(*rig.hauler, occupied)); CHECK(CarriesEveryPart(*rig.front, occupied));
	const auto result = VehicleEnterTile(rig.hauler, occupied, TileX(occupied) * TILE_SIZE + TILE_SIZE / 2,
		TileY(occupied) * TILE_SIZE + TILE_SIZE / 2);
	CHECK(result.Test(VehicleEnterTileState::CannotEnter));
	const auto stand = FindTBMFaceStand(rig.Project()); REQUIRE(stand.has_value());
	CHECK(CarriesEveryPart(*rig.hauler, stand->tile));
	const auto path = rig.Path(); const auto at = std::ranges::find(path, stand->tile); REQUIRE(at != path.end());
	REQUIRE(at + 1 != path.end()); CHECK_FALSE(CarriesEveryPart(*rig.hauler, *(at + 1))); // The physically clear cell immediately behind the tail is a safety gap, not another hauler route through a folded chamber.
}

/** Task 3.6: a reserved path may end on the turn holder's stand, and only there and only for it. */
TEST_CASE("A reserved path may end on the face stand", "[demo-fix-face-access]")
{
	TBMFaceRig rig;
	rig.BoreClearOfHousing();
	rig.StandAtFace();
	rig.Assign();
	Train *second = rig.AssignAnother();
	const auto stand = FindTBMFaceStand(rig.Project());
	REQUIRE(stand.has_value());
	const auto path = rig.Path();
	const auto at = std::ranges::find(path, stand->tile);
	REQUIRE(at != path.end());
	REQUIRE(at != path.begin());
	const TileIndex before = *(at - 1);
	for (const bool reserve : {false, true}) {
		INFO("reserve_paths " << reserve);
		AutoRestoreBackup reserve_paths(_settings_game.pf.reserve_paths, reserve);
		CHECK(IsSafeWaitingPosition(rig.hauler, stand->tile, stand->trackdir, false, true));
		// The same cell is no waiting place for another train, and the cell before it is none for the turn holder.
		CHECK_FALSE(IsSafeWaitingPosition(second, stand->tile, stand->trackdir, false, true));
		CHECK_FALSE(IsTBMHaulerFaceStand(*rig.hauler, before, stand->trackdir));
		CHECK_FALSE(IsTBMHaulerFaceStand(*rig.hauler, stand->tile, ReverseTrackdir(stand->trackdir)));
	}
}
