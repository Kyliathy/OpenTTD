/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file tbm_face_meeting.cpp Pure Source/Bay policy over original phases and actual spoil/consist owners.
	* @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native work and physical train ownership.
	* @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group20C; Deep-size qualification follows in20D.
	* Poses and source emptying are disclosed components; phase copies are never installed as saved face waits.
	*/
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "../safeguards.h"

/** Ask a pure policy question without changing the installed project.
	* @param project Original live project.
	* @param phase Candidate native phase, including the not-yet-activated face wait.
	* @return Detached query input; no saved-state or controller transition occurs.
	*/
static TBMProject MeetingPhase(const TBMProject &project, TBMPhase phase)
{
	auto copy = project; copy.phase = phase; return copy;
}

/** Every original phase and the appended face wait keep their separate work, readiness and empty-return meaning. */
TEST_CASE("Face meeting phase queries preserve empty returns and current or previous working approaches", "[tbm-face-meeting]")
{
	TBMFaceRig rig; rig.BoreClearOfHousing(); rig.Assign(); rig.StandAtFace();
	const std::array phases{TBMPhase::Commissioning, TBMPhase::TravelToFace, TBMPhase::Bore, TBMPhase::AdvanceSupport,
	 TBMPhase::ReturnForExchange, TBMPhase::WaitForEmpties, TBMPhase::Resume, TBMPhase::Completed, TBMPhase::Cancelled,
	 TBMPhase::ReturnToPark, TBMPhase::WaitToPark, TBMPhase::Parked, TBMPhase::ReturnToRelease, TBMPhase::WaitToRelease,
	 TBMPhase::ReleasePending, TBMPhase::ReturnForHangar, TBMPhase::WaitForHangar, TBMPhase::TravelToHangar, TBMPhase::ParkedHangar,
	 TBMPhase::FollowOrders, TBMPhase::AwaitFaceTransfer};
	const auto path = rig.Path(); const size_t face = path.size() - 1; const auto before = rig.Digest();
	const auto original_projects = CaptureTBMProjects(); const auto original_assignments = CaptureTBMExchangeAssignments();
	// Empty returning machinery must never invite another hauler; only the three
	// working phases may select a face here, and none is transfer-ready without spoil.
	for (const auto phase : phases) {
	 const auto query = MeetingPhase(rig.Project(), phase); const bool working = phase == TBMPhase::Bore || phase == TBMPhase::AdvanceSupport || phase == TBMPhase::AwaitFaceTransfer;
	 CAPTURE(phase); CHECK(GetTBMMeetingKind(query) == (working ? TBMMeetingKind::Face : TBMMeetingKind::Bay)); CHECK_FALSE(IsTBMReadyForFaceTransfer(query));
	}
	CHECK(ValidateTBMProject(MeetingPhase(rig.Project(), TBMPhase::AwaitFaceTransfer)) == TBMProjectError::InvalidProject);
	CHECK(rig.Digest() == before); CHECK(CaptureTBMProjects() == original_projects); CHECK(CaptureTBMExchangeAssignments() == original_assignments);
	PlaceTrainOnPath(rig.front, path, face - 1, face - rig.parts);
	CHECK(GetTBMMeetingKind(MeetingPhase(rig.Project(), TBMPhase::Bore)) == TBMMeetingKind::Face);
	CHECK(GetTBMMeetingKind(MeetingPhase(rig.Project(), TBMPhase::AdvanceSupport)) == TBMMeetingKind::Face);
	CHECK(GetTBMMeetingKind(MeetingPhase(rig.Project(), TBMPhase::ReturnToPark)) == TBMMeetingKind::Bay);
	PlaceTrainOnPath(rig.front, path, face - 2, face - rig.parts - 1); REQUIRE(FindTBMFaceStand(rig.Project()).has_value());
	// A safe stand behind a cutter already withdrawing is not enough to select
	// Face; no working or returning phase may lend the older approach a new meeting.
	for (const auto phase : phases) CHECK(GetTBMMeetingKind(MeetingPhase(rig.Project(), phase)) == TBMMeetingKind::Bay);
}

/** Loaded return can finish an existing face encounter, while invalid references and real underground depots retain Bay. */
TEST_CASE("Loaded face returns keep only an already-inside hauler and never borrow an invalid or depot bay", "[tbm-face-meeting]")
{
	TBMFaceRig rig; rig.BoreClearOfHousing(); rig.Assign(); rig.BoreOneCut(); rig.StandAtFace();
	const auto path = rig.Path(); const size_t face = path.size() - 1;
	// Parking/release/hangar returns owe their actual partial spoil regardless
	// of whether this particular cursor is a frozen full-pass breakpoint.
	for (const auto phase : {TBMPhase::ReturnToPark, TBMPhase::ReturnToRelease, TBMPhase::ReturnForHangar}) {
	 const auto query = MeetingPhase(rig.Project(), phase); REQUIRE(IsTBMExchangeDue(query)); CHECK(GetTBMMeetingKind(query) == TBMMeetingKind::Face); CHECK(IsTBMReadyForFaceTransfer(query));
	}
	const auto returning = MeetingPhase(rig.Project(), TBMPhase::ReturnToPark);
	PlaceTrainOnPath(rig.front, path, face - 1, face - rig.parts); CHECK(GetTBMMeetingKind(returning) == TBMMeetingKind::Bay);
	rig.PlaceOnTrack(rig.hauler, path[1]); CHECK(GetTBMMeetingKind(returning) == TBMMeetingKind::Face); CHECK(IsTBMReadyForFaceTransfer(returning));
	rig.StoreInMagazine(rig.hauler); CHECK(GetTBMMeetingKind(returning) == TBMMeetingKind::Bay); CHECK_FALSE(IsTBMReadyForFaceTransfer(returning));
	rig.StandAtFace(); const auto source_query = MeetingPhase(rig.Project(), TBMPhase::Bore);
	REQUIRE(GetTBMMeetingKind(source_query) == TBMMeetingKind::Face);
	auto missing = source_query; missing.cutter = VehicleID::Invalid(); CHECK(GetTBMMeetingKind(missing) == TBMMeetingKind::Bay);
	missing = source_query; missing.job = ExcavationJobID{UINT32_MAX}; CHECK(GetTBMMeetingKind(missing) == TBMMeetingKind::Bay);
	missing = source_query; missing.origin = StationID::Invalid(); CHECK(GetTBMMeetingKind(missing) == TBMMeetingKind::Bay);
	missing = source_query; missing.exchange_bay = INVALID_TILE; CHECK(GetTBMMeetingKind(missing) == TBMMeetingKind::Bay);
	missing = source_query; missing.portal = INVALID_TILE; CHECK(GetTBMMeetingKind(missing) == TBMMeetingKind::Bay);
	missing = source_query; missing.company = CompanyID::Invalid(); CHECK(GetTBMMeetingKind(missing) == TBMMeetingKind::Bay);
	const uint cells = (rig.front->gcache.cached_total_length + TILE_SIZE - 1) / TILE_SIZE;
	const auto depot = rig.BuildDepotBay(cells); auto depot_query = source_query; depot_query.exchange_bay = depot;
	REQUIRE(GetTBMExchangeBay(depot_query).error == TBMExchangeError::None); REQUIRE(GetTBMExchangeBay(depot_query).kind == TBMExchangeBayKind::Depot);
	CHECK(GetTBMMeetingKind(depot_query) == TBMMeetingKind::Bay); CHECK_FALSE(IsTBMReadyForFaceTransfer(MeetingPhase(depot_query, TBMPhase::ReturnToPark)));
	// This component's staged project reload explicitly clears assignments;
	// restore the actual assignment before testing an already-inside empty return.
	rig.ExchangeAndReturn(); rig.Assign(); rig.StandAtFace(); rig.PlaceOnTrack(rig.hauler, path[1]);
	REQUIRE(FindTBMHaulerInCorridor(rig.Project()) == rig.hauler->index);
	CHECK(GetTBMMeetingKind(MeetingPhase(rig.Project(), TBMPhase::ReturnToPark)) == TBMMeetingKind::Bay);
	CHECK_FALSE(IsTBMReadyForFaceTransfer(MeetingPhase(rig.Project(), TBMPhase::ReturnToPark)));
}

/** Every assigned native hauler must clear the housing, including a longer second consist; the exact spare-cell boundary remains inclusive. */
TEST_CASE("Face meeting requires the longest actual assigned hauler plus two whole clearance cells", "[tbm-face-meeting]")
{
	TBMFaceRig rig(28); rig.Assign(); const auto initial_room = GetTBMHaulerRoomCells(rig.project.id);
	auto *longer = rig.AssignAnother(5, 5); const uint room = (longer->gcache.cached_total_length + TILE_SIZE - 1) / TILE_SIZE;
	REQUIRE(room > initial_room); CHECK(GetTBMHaulerRoomCells(rig.project.id) == room);
	const auto cuts = room + 2 + rig.parts - 1; REQUIRE(cuts < rig.Job().route.size()); rig.Bore(static_cast<uint>(cuts));
	// Bore's declared emptying/reload clears original assignment stores. Reissue
	// these exact haulers through native admission before observing clearance.
	rig.Assign(); REQUIRE(AssignTBMHauler(rig.project.id, longer->index, true) == TBMExchangeError::None);
	REQUIRE(GetTBMHaulerRoomCells(rig.project.id) == room);
	const auto path = rig.Path(); const auto face = path.size() - 1;
	PlaceTrainOnPath(rig.front, path, face, room + 1); REQUIRE(FindTBMFaceStand(rig.Project()).has_value());
	CHECK(GetTBMMeetingKind(MeetingPhase(rig.Project(), TBMPhase::Bore)) == TBMMeetingKind::Bay);
	PlaceTrainOnPath(rig.front, path, face, room + 2); REQUIRE(FindTBMFaceStand(rig.Project()).has_value());
	const auto before = rig.Digest(); CHECK(GetTBMMeetingKind(MeetingPhase(rig.Project(), TBMPhase::Bore)) == TBMMeetingKind::Face); CHECK(rig.Digest() == before);
}
