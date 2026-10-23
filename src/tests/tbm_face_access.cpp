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
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group22C. Poses are component inputs, not driven face-wait proof.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "../layer_portal_cmd.h"
#include "../pbs.h"
#include "../tile_cmd.h"
#include "../settings_type.h"
#include "../table/strings.h"
#include "../pathfinder/yapf/yapf.h"
#include "../safeguards.h"

/**
 * @param train Train whose parts are checked.
 * @param tile Support tile.
 * @return Whether the support carries every part of the train.
 */
static bool CarriesEveryPart(const Train &train, TileIndex tile)
{
	// Admission applies to every physical part; an allowed engine cannot lend permission to a refused follower.
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
TEST_CASE("Only the face turn holder passes its own pair", "[tbm-face-access]")
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

	// Admission and safe endpoints must agree under both native reservation policies.
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

	SECTION("Native YAPF reaches the actual stand through admitted paid support") {
		const auto path = rig.Path(); const auto stand = FindTBMFaceStand(rig.Project()); REQUIRE(stand.has_value());
		PlaceTrainOnPath(rig.hauler, path, 2, 1);
		const auto direction = DiagdirBetweenTiles(path[2], path[3]);
		// Orient every actual part with the declared straight approach; the
		// search itself must reserve nothing and preserve cargo, orders and RNG.
		for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->direction = DiagDirToDir(direction);
		const auto before = TBMFaceSnapshot::Capture(rig); const Order order = rig.hauler->current_order;
		bool found = false;
		CHECK(YapfTrainChooseTrack(rig.hauler, path[3], direction, GetTrackBits(path[3]), found, false, nullptr, nullptr) != Track::Invalid);
		CHECK(found); CHECK(rig.hauler->current_order.Equals(order)); CHECK(TBMFaceSnapshot::Capture(rig) == before);
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
TEST_CASE("The face turn and the magazine queue", "[tbm-face-access]")
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
	TBMFaceTransferRig::AddPacket(loaded_wagon, 1);
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
TEST_CASE("A hauler completes partial magazine movement before changing its exit", "[tbm-face-access]")
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
TEST_CASE("Support carries only the project's own trains", "[tbm-face-access]")
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
	// The whole declared consist changes company together; no foreign follower may inherit the project admission.
	for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->owner = other->index;
	CHECK_FALSE(CarriesEveryPart(*rig.hauler, support));
	CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, rig.Key()));
	// The whole declared consist changes company together; no foreign follower may inherit the project admission.
	for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->owner = _current_company;
	delete other;
}

/** A native chamber shortcut cannot take an assigned hauler through its own project's actual cutter body; shared support and physical entry enforce the same clearance (Plan18 8FA). */
TEST_CASE("A hauler cannot enter support occupied by its own cutter", "[tbm-face-access]")
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
TEST_CASE("A reserved path may end on the face stand", "[tbm-face-access]")
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
	// Admission and safe endpoints must agree under both native reservation policies.
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

/** Actual Source occupancy closes inbound access without confusing stored stock with a running obstruction. */
TEST_CASE("Busy Source refuses another front but stored queued stock does not close its own turn", "[tbm-face-access]")
{
 TBMFaceRig rig; rig.BoreClearOfHousing(); rig.StandAtFace(); rig.Assign(); auto *other = rig.BuildHauler(rig.depot, 2, 2);
 const auto key = rig.Key(); const auto door = GetLayerPortalDoorDirection(key);
 rig.PlaceOnTrack(other, key); REQUIRE(IsLayerPortalMagazineBusy(key, *rig.hauler));
 CHECK_FALSE(GetLayerPortalAdmittedTrackdirs(key, *rig.hauler, door).Any());
 rig.StoreInMagazine(other); CHECK_FALSE(IsLayerPortalMagazineBusy(key, *rig.hauler)); CHECK(GetLayerPortalAdmittedTrackdirs(key, *rig.hauler, door).Any());
 // Remote storage in a genuine depot cannot preserve an unrelated incoming
 // orientation at a future Source destination.
 rig.PlaceInDepot(rig.hauler, rig.depot); CHECK(GetLayerPortalMagazineExit(key, *rig.hauler) == DiagDirection::Invalid);
}

/** Return and post-transfer work hold while the actual loaded carrier clears, but native service ownership still has precedence. */
TEST_CASE("Cutter return and work holds protect the departing loaded hauler and ordinary service orders", "[tbm-face-access]")
{
 TBMFaceTransferRig rig; REQUIRE(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::Obstruction); CHECK(IsTBMHoldingTrain(*rig.front));
 REQUIRE(ExecuteTBMFaceTransfer(rig.Request()).error == TBMExchangeError::None);
 CHECK(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::Obstruction);
 const auto assignments = CaptureTBMExchangeAssignments(); const auto sets = CaptureTBMWagonSets();
 const auto next_assignment = GetNextTBMExchangeAssignmentID(), next_set = GetNextTBMWagonSetID();
 rig.Reload(TBMPhase::Bore); StageLoadedTBMExchange(assignments, next_assignment, sets, next_set); REQUIRE(AcceptLoadedTBMExchange());
 CHECK(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::Obstruction); CHECK(IsTBMHoldingTrain(*rig.front));
 rig.PlaceInDepot(rig.hauler, rig.depot); CHECK(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::None);
 rig.PlaceOnTrack(rig.hauler, rig.Path()[1]); REQUIRE(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::Obstruction);
 rig.front->current_order.MakeGoToDepot(DestinationID{0}, {}); CHECK(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::None); CHECK_FALSE(IsTBMHoldingTrain(*rig.front));
}

/** A physically remaining hauler blocks paid exit replacement until it clears the original corridor. */
TEST_CASE("Far exit handover refuses the real remaining hauler", "[tbm-face-access]")
{
 TBMFaceRig rig(24); rig.Bore(static_cast<uint>(rig.Job().route.size()) - 1); rig.BoreOneCut();
 REQUIRE(rig.Job().state == ExcavationJobState::Completed); rig.Assign(); const auto path = rig.Path();
 const auto exit = path.back(); const auto shaft = DiagdirBetweenTiles(path[path.size() - 2], exit);
 rig.PlaceOnTrack(rig.hauler, path[path.size() / 2]); const auto before = TBMFaceSnapshot::Capture(rig);
 CHECK(CheckTBMExitHandover(exit, shaft, RAILTYPE_RAIL).GetErrorMessage() == STR_ERROR_TBM_EXIT_HAULER); CHECK(TBMFaceSnapshot::Capture(rig) == before);
 rig.PlaceInDepot(rig.hauler, rig.depot); CHECK(CheckTBMExitHandover(exit, shaft, RAILTYPE_RAIL).GetErrorMessage() != STR_ERROR_TBM_EXIT_HAULER);
}

/** PBS consumes the real corner arrival pieces instead of accepting every direction on a diagonal stand's tile. */
TEST_CASE("Diagonal face stand is a directed native PBS endpoint", "[tbm-face-access]")
{
 std::vector<DirDiff> turns{DirDiff::Same, DirDiff::Same, DirDiff::Right45, DirDiff::Same, DirDiff::Left45}; turns.insert(turns.end(), 14, DirDiff::Same);
 TBMExchangeRig rig(turns); const auto parts = CountTrainParts(*rig.front); bool checked = false;
 // Earn each cut in order, then inspect only a real complete face meeting.
 // The stand must pass a corner piece behind the actual whole cutter body.
 while (rig.Job().cursor + 1 < rig.Job().route.size()) {
  rig.BoreOneCut(); rig.ExchangeAndReturn(); rig.Assign(); const auto path = CompletedWorkPath(rig);
  if (path.size() < parts + 5) continue; // Whole cutter and hauler room must fit beyond the housing before a face is meaningful.
  PlaceTrainOnPath(rig.front, path, path.size() - 1, path.size() - parts);
  const auto stand = FindTBMFaceStand(rig.Project());
  // Only a real corner stand meeting the full room policy exercises this case.
  if (!stand || IsDiagonalTrackdir(stand->trackdir) || GetTBMMeetingKind(rig.Project()) != TBMMeetingKind::Face) continue;
  const auto before = TBMFaceSnapshot::Capture(rig); const auto allowed = GetTBMFaceStandTrackdirs(*stand);
  CHECK(IsSafeWaitingPosition(rig.hauler, stand->tile, stand->trackdir, false, true));
  CHECK_FALSE(IsTBMHaulerFaceStand(*rig.hauler, stand->tile, ReverseTrackdir(stand->trackdir)));
  CHECK(GetTBMHaulerSteeringTarget(*rig.hauler)->trackdirs == allowed); CHECK(TBMFaceSnapshot::Capture(rig) == before); checked = true; break;
 }
 CHECK(checked);
}
