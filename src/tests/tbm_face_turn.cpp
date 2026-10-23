/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file tbm_face_turn.cpp Stable native assignment eligibility and exact portal/stand observations.
	* @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md actual corridor and train ownership.
	* @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group22; native rail callback wiring follows22B-22D.
	* Poses and native owner-state controls are explicit components, not a driven crossing or completed-job fixture.
	*/
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "../safeguards.h"

/** Resolve the current original assignment after any admitted owner mutation.
	* @param train Actual assigned native front.
	* @return Current immutable original record; callers must not retain it across store replacement.
	*/
static const TBMExchangeAssignment &FaceTurnAssignment(const Train &train)
{
	const auto *assignment = GetTBMExchangeAssignmentForHauler(train.index); REQUIRE(assignment != nullptr); return *assignment;
}

/** Lowest eligible assignment wins, while actual loaded corridor occupants can keep every other turn closed. */
TEST_CASE("Face turn selects whole empty capacity in assignment order and excludes another corridor occupant", "[tbm-face-turn]")
{
	TBMFaceRig rig; rig.BoreClearOfHousing(); rig.StandAtFace(); auto *shorty = rig.AssignAnother(1, 2); rig.Assign(); auto *third = rig.AssignAnother();
	REQUIRE(FaceTurnAssignment(*shorty).id < FaceTurnAssignment(*rig.hauler).id); REQUIRE(rig.hauler->index < shorty->index);
	const auto before = TBMFaceSnapshot::Capture(rig); const auto assignments = CaptureTBMExchangeAssignments();
	CHECK_FALSE(IsTBMFaceTurnHolder(FaceTurnAssignment(*shorty))); CHECK(IsTBMFaceTurnHolder(FaceTurnAssignment(*rig.hauler))); CHECK_FALSE(IsTBMFaceTurnHolder(FaceTurnAssignment(*third)));
	CHECK(TBMFaceSnapshot::Capture(rig) == before); CHECK(CaptureTBMExchangeAssignments() == assignments);
	rig.StoreInMagazine(third); CHECK_FALSE(IsTBMHaulerInCorridor(rig.Project(), third->index)); CHECK(IsTBMFaceTurnHolder(FaceTurnAssignment(*rig.hauler)));
	// A loaded ineligible hauler still physically occupies the corridor. Its
	// loss of candidate status must not invite the next train into a head-on move.
	TBMFaceTransferRig::AddPacket(rig.hauler->GetNextUnit(), 1); rig.PlaceOnTrack(rig.hauler, rig.Path()[1]);
	CHECK_FALSE(IsTBMFaceTurnHolder(FaceTurnAssignment(*rig.hauler))); CHECK_FALSE(IsTBMFaceTurnHolder(FaceTurnAssignment(*third)));
	CHECK(FindTBMHaulerInCorridor(rig.Project()) == rig.hauler->index);
	rig.PlaceInDepot(rig.hauler, rig.depot); CHECK(FindTBMHaulerInCorridor(rig.Project()) == VehicleID::Invalid());
	CHECK(IsTBMFaceTurnHolder(FaceTurnAssignment(*third))); CHECK_FALSE(IsTBMFaceTurnHolder(FaceTurnAssignment(*shorty)));
}

/** Native observed holds, foreign ownership and crashes remove a candidate without mutating/reordering the original assignment store. */
TEST_CASE("Face turn observes actual stopped foreign and crashed candidates without a shadow queue", "[tbm-face-turn]")
{
	TBMFaceRig rig; rig.BoreClearOfHousing(); rig.StandAtFace(); rig.Assign(); auto *second = rig.AssignAnother();
	REQUIRE(IsTBMFaceTurnHolder(FaceTurnAssignment(*rig.hauler)));
	// The original hauler owner observes the real stop flag and publishes its
	// normal hold; the turn query itself cannot write a synthetic wait or phase.
	rig.hauler->vehstatus.Set(VehState::Stopped); REQUIRE(AdvanceTBMHauler(FaceTurnAssignment(*rig.hauler).id) == TBMExchangeError::None);
	REQUIRE(FaceTurnAssignment(*rig.hauler).wait == TBMHaulerWait::Stopped); CHECK_FALSE(IsTBMFaceTurnHolder(FaceTurnAssignment(*rig.hauler))); CHECK(IsTBMFaceTurnHolder(FaceTurnAssignment(*second)));
	rig.hauler->vehstatus.Reset(VehState::Stopped); REQUIRE(AdvanceTBMHauler(FaceTurnAssignment(*rig.hauler).id) == TBMExchangeError::None);
	REQUIRE(FaceTurnAssignment(*rig.hauler).wait == TBMHaulerWait::None); CHECK(IsTBMFaceTurnHolder(FaceTurnAssignment(*rig.hauler)));
	{
	 AutoRestoreBackup owner(rig.hauler->owner, Owner{OWNER_NONE}); const auto before = TBMFaceSnapshot::Capture(rig);
	 CHECK_FALSE(IsTBMFaceTurnHolder(FaceTurnAssignment(*rig.hauler))); CHECK(IsTBMFaceTurnHolder(FaceTurnAssignment(*second))); CHECK(TBMFaceSnapshot::Capture(rig) == before);
	}
	rig.hauler->vehstatus.Set(VehState::Crashed); const auto before = TBMFaceSnapshot::Capture(rig);
	CHECK_FALSE(IsTBMFaceTurnHolder(FaceTurnAssignment(*rig.hauler))); CHECK(IsTBMFaceTurnHolder(FaceTurnAssignment(*second))); CHECK(TBMFaceSnapshot::Capture(rig) == before);
	rig.hauler->vehstatus.Reset(VehState::Crashed);
}

/** Own-pair inbound turn, actual outbound presence and the exact stand direction remain separate observation contracts. */
TEST_CASE("Face entry and safe stand queries distinguish own turn outbound clearance and unrelated native rails", "[tbm-face-turn]")
{
	TBMFaceRig rig; rig.BoreClearOfHousing(); rig.StandAtFace(); rig.Assign(); auto *second = rig.AssignAnother(); auto *stranger = rig.BuildHauler(rig.depot, 2, 2);
	const auto stand = FindTBMFaceStand(rig.Project()); REQUIRE(stand.has_value()); const auto key = rig.Key(); const auto housing = rig.Housing();
	const auto before = TBMFaceSnapshot::Capture(rig);
	// Both ends belong to one canonical pair; no queued second assignment,
	// unassigned train or cutter may borrow this hauler's inbound through right.
	for (const auto tile : {key, housing}) {
	 CHECK(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, tile)); CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*second, tile));
	 CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*stranger, tile)); CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*rig.front, tile));
	}
	CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, INVALID_TILE)); CHECK_FALSE(IsTBMHaulerFaceStand(*rig.hauler, INVALID_TILE, stand->trackdir));
	CHECK(IsTBMHaulerFaceStand(*rig.hauler, stand->tile, stand->trackdir)); CHECK_FALSE(IsTBMHaulerFaceStand(*rig.hauler, stand->tile, ReverseTrackdir(stand->trackdir)));
	CHECK_FALSE(IsTBMHaulerFaceStand(*second, stand->tile, stand->trackdir)); CHECK_FALSE(IsTBMHaulerFaceStand(*stranger, stand->tile, stand->trackdir));
	CHECK_FALSE(IsTBMHaulerFaceStand(*rig.hauler, rig.Path()[1], stand->trackdir)); CHECK(TBMFaceSnapshot::Capture(rig) == before);
	const auto other_pair = TileXYL(40, 20, MapLayer::Surface);
	REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, other_pair, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
	CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, other_pair)); CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, GetLayerPortalPairTile(other_pair)));
	TBMFaceTransferRig::AddPacket(rig.hauler->GetNextUnit(), 1); rig.StoreInMagazine(rig.hauler);
	CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, key)); CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, housing));
	// One tail still running beyond the Source key retains exit eligibility
	// even though the loaded train no longer qualifies for any inbound turn.
	auto *tail = rig.hauler->Last(); tail->tile = housing; tail->track = Track::X; tail->UpdatePosition();
	CHECK(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, key)); CHECK(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, housing));
	CHECK_FALSE(IsTBMFaceTurnHolder(FaceTurnAssignment(*rig.hauler))); CHECK_FALSE(IsTBMFaceTurnHolder(FaceTurnAssignment(*second)));
	tail->tile = key; tail->UpdatePosition(); CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, key));
	tail->track = Track::Depot; CHECK_FALSE(IsTBMHaulerInCorridor(rig.Project(), rig.hauler->index)); CHECK(IsTBMFaceTurnHolder(FaceTurnAssignment(*second)));
}
