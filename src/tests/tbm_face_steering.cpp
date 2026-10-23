/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file tbm_face_steering.cpp Directed native destinations, ordinary orders and unchanged observation.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical work and view-independent ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group22B; driven face admission follows22C/23.
 * Explicit native component poses isolate destination selection; they are not a driven face-loop acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "../pathfinder/yapf/yapf.hpp"
#include "../pathfinder/yapf/yapf_node_rail.hpp"
#include "../pathfinder/yapf/yapf_destrail.hpp"
#include "../safeguards.h"

/** Native destination adapter under test; only the cache-disable sink is substituted. */
class FaceDestinationProbe;
/** Bind the actual rail destination template to its native node family. */
struct FaceDestinationTypes {
 using Tpf = FaceDestinationProbe;
 using NodeList = CRailNodeList;
};
/** Expose actual native destination recognition without needing not-yet-integrated face admission callbacks. */
class FaceDestinationProbe : public CYapfDestinationTileOrStationRailT<FaceDestinationTypes> {
public:
 bool uncached = false; ///< Whether the real destination owner requires live body/turn observations.
 /** @param value Cache policy selected by the real destination owner. */
 void DisableCache(bool value) { this->uncached = value; }
};

/** Verify actual destination pieces and signal-policy separation without mutating train or project state.
 * @param train Native train whose ordinary origin order must survive.
 * @param tile Expected temporary destination.
 * @param td The one admitted incoming direction for this straight component.
 */
static void CheckDirectedHaulerTarget(Train &train, TileIndex tile, Trackdir td)
{
 const Order order = train.current_order;
 const auto dest = train.dest_tile;
 const auto target = GetTBMHaulerSteeringTarget(train); REQUIRE(target.has_value());
 CHECK(target->tile == tile); CHECK(target->trackdirs == TrackdirToTrackdirBits(td));
 FaceDestinationProbe probe; probe.SetDestination(&train);
 CHECK(probe.uncached); CHECK(probe.IsWorkSteeringDestination()); CHECK_FALSE(probe.IsCutterSteeringDestination());
 CHECK(probe.PfDetectDestination(tile, td)); CHECK_FALSE(probe.PfDetectDestination(tile, ReverseTrackdir(td)));
 CHECK_FALSE(probe.PfDetectDestination(INVALID_TILE, td)); CHECK(train.current_order.Equals(order)); CHECK(train.dest_tile == dest);
}

/** Only the turn holder receives the exact stand; the second empty hauler retains its own bay door and line-end wait. */
TEST_CASE("Hauler steering distinguishes directed face and queued Source without writing origin orders", "[tbm-face-steering]")
{
 TBMFaceRig rig; rig.BoreClearOfHousing(); rig.StandAtFace(); rig.Assign(); auto *second = rig.AssignAnother();
 rig.hauler->current_order.MakeGoToStation(rig.Project().origin); second->current_order.MakeGoToStation(rig.Project().origin);
 const auto stand = FindTBMFaceStand(rig.Project()); REQUIRE(stand.has_value()); const auto before = TBMFaceSnapshot::Capture(rig);
 CheckDirectedHaulerTarget(*rig.hauler, stand->tile, stand->trackdir);
 const auto door_td = DiagDirToDiagTrackdir(ReverseDiagDir(GetLayerPortalDoorDirection(rig.Key())));
 CheckDirectedHaulerTarget(*second, rig.Key(), door_td);
 CHECK_FALSE(IsTBMWaitingAtLineEnd(*rig.hauler, rig.Key())); CHECK(IsTBMWaitingAtLineEnd(*second, rig.Key()));
 CHECK_FALSE(IsTBMWaitingAtLineEnd(*second, stand->tile)); CHECK(IsTBMWaitingAtLineEnd(*rig.front, rig.Key()));
 CHECK(TBMFaceSnapshot::Capture(rig) == before);
}

/** A real completed transfer changes the leg once; subsequent native destination queries follow the unchanged origin order. */
TEST_CASE("Loaded hauler follows ordinary origin with live clearance caching and no Source line-end hold", "[tbm-face-steering]")
{
 TBMFaceTransferRig rig; rig.hauler->current_order.MakeGoToStation(rig.Project().origin);
 FaceDestinationProbe probe; probe.SetDestination(rig.hauler); REQUIRE(probe.IsWorkSteeringDestination());
 const Order order = rig.hauler->current_order; REQUIRE(ExecuteTBMFaceTransfer(rig.Request()).error == TBMExchangeError::None);
 const auto before = TBMFaceSnapshot::Capture(rig);
 CHECK_FALSE(GetTBMHaulerSteeringTarget(*rig.hauler).has_value()); CHECK_FALSE(IsTBMWaitingAtLineEnd(*rig.hauler, rig.Key()));
 probe.uncached = false; probe.SetDestination(rig.hauler);
 CHECK(probe.uncached); CHECK_FALSE(probe.IsWorkSteeringDestination()); CHECK_FALSE(probe.IsCutterSteeringDestination());
 CHECK(rig.hauler->current_order.Equals(order)); CHECK(TBMFaceSnapshot::Capture(rig) == before);
}

/** Closed project state must not resurrect a bay destination while a real tail still clears its key. */
TEST_CASE("Stored and finished work ends steering despite remaining physical Source admission", "[tbm-face-steering]")
{
 TBMFaceRig rig; rig.Assign(); rig.StoreInMagazine(rig.hauler); rig.hauler->current_order.MakeGoToStation(rig.Project().origin);
 const auto assignments = CaptureTBMExchangeAssignments(); const auto sets = CaptureTBMWagonSets();
 const auto next_assignment = GetNextTBMExchangeAssignmentID(), next_set = GetNextTBMWagonSetID();
 // Restore only original admitted owner records after the fixture's public
 // project reload. No future phase, shadow store or const-cast bypass is used.
 for (const auto phase : {TBMPhase::Parked, TBMPhase::Completed, TBMPhase::Cancelled}) {
  rig.Reload(phase); StageLoadedTBMExchange(assignments, next_assignment, sets, next_set); REQUIRE(AcceptLoadedTBMExchange());
  REQUIRE(GetTBMExchangeAssignmentForHauler(rig.hauler->index) != nullptr); const auto before = TBMFaceSnapshot::Capture(rig);
  REQUIRE(IsTBMHaulerAdmittedAtBay(*rig.hauler, rig.Key())); CHECK_FALSE(GetTBMHaulerSteeringTarget(*rig.hauler).has_value());
  CHECK_FALSE(IsTBMWaitingAtLineEnd(*rig.hauler, rig.Key())); FaceDestinationProbe probe; probe.SetDestination(rig.hauler);
  CHECK_FALSE(probe.IsWorkSteeringDestination()); CHECK(TBMFaceSnapshot::Capture(rig) == before);
 }
}

/** A genuine Underground depot uses its real door and native route, while an unrelated train retains ordinary depot routing. */
TEST_CASE("Empty hauler depot target uses native YAPF without face or cutter signal policy", "[tbm-face-steering]")
{
 TBMFaceRig rig; const uint cells = CeilDiv(rig.front->gcache.cached_total_length, TILE_SIZE); const auto depot = rig.BuildDepotBay(cells);
 /** @param project Original staged component whose sole declared change is its real validated bay. */
 const auto set_bay = [depot](TBMProject &project) { project.exchange_bay = depot; };
 rig.Reload(TBMPhase::Bore, set_bay); rig.Assign(); rig.hauler->current_order.MakeGoToStation(rig.Project().origin);
 const auto td = DiagDirToDiagTrackdir(ReverseDiagDir(GetRailDepotDirection(depot)));
 CheckDirectedHaulerTarget(*rig.hauler, depot, td);
 rig.PlaceOnTrack(rig.hauler, TileXYL(42, 40, MapLayer::Underground));
 // Orient the declared whole component toward the real depot; native YAPF
 // follows the actual rail and destination without reserving or moving parts.
 for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->direction = DiagDirToDir(TrackdirToExitdir(td));
 const auto before = TBMFaceSnapshot::Capture(rig); const Order order = rig.hauler->current_order;
 bool found = false; const auto next = TileXYL(41, 40, MapLayer::Underground);
 CHECK(YapfTrainChooseTrack(rig.hauler, next, TrackdirToExitdir(td), Track::X, found, false, nullptr, nullptr) == Track::X); CHECK(found);
 CHECK(rig.hauler->current_order.Equals(order)); CHECK(TBMFaceSnapshot::Capture(rig) == before);
 auto *ordinary = rig.BuildHauler(rig.depot, 2, 2); ordinary->dest_tile = depot;
 FaceDestinationProbe probe; probe.SetDestination(ordinary); CHECK_FALSE(probe.uncached); CHECK_FALSE(probe.IsWorkSteeringDestination());
 CHECK(probe.PfDetectDestination(depot, td)); CHECK_FALSE(GetTBMHaulerSteeringTarget(*ordinary).has_value());
}
