/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file tbm_face_cycle.cpp Existing hauler step/transfer/reversal over admitted loaded-return components.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical movement and view-independent ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group22D; actual installed face-wait replay remains23.
 * These headless components open no camera. Pose and explicit disposal inputs are disclosed; no original save or full route is replayed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "../train_cmd.h"
#include "../safeguards.h"

/** @param rig Native component fixture. @return Current immutable hauler assignment. */
static const TBMExchangeAssignment &FaceCycle(const TBMExchangeRig &rig)
{
 const auto *cycle = GetTBMExchangeAssignmentForHauler(rig.hauler->index); REQUIRE(cycle != nullptr); return *cycle;
}

/** The existing hauler owner performs one complete transfer, one turnaround and the original origin/disposal/empty cycle. */
TEST_CASE("Loaded return face encounter advances the existing hauler cycle once without changing orders or chains", "[tbm-face-cycle]")
{
 TBMFaceTransferRig rig; rig.hauler->vehstatus.Reset(VehState::Stopped); rig.hauler->current_order.MakeGoToStation(rig.Project().origin);
 const Order order = rig.hauler->current_order; const auto cutter_chain = GetTBMChainIDs(rig.front), hauler_chain = GetTBMChainIDs(rig.hauler);
 const auto cargo = CountTBMPackets({rig.front, rig.hauler}); const auto load = TBMExchangeRig::Aboard(rig.front); REQUIRE(load != 0);
 const auto facing = rig.hauler->direction;
 REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange); CHECK(IsTBMHaulerHeld(*rig.hauler));
 const auto collected = rig.StepHauler(); CHECK(collected.phase == TBMHaulerPhase::CollectLoaded); CHECK(collected.exchanges == 1);
 CHECK(TBMExchangeRig::Aboard(rig.front) == 0); CHECK(TBMExchangeRig::Aboard(rig.hauler) == load); CHECK(CountTBMPackets({rig.front, rig.hauler}) == cargo);
 CHECK(GetTBMChainIDs(rig.front) == cutter_chain); CHECK(GetTBMChainIDs(rig.hauler) == hauler_chain);
 CHECK(rig.hauler->direction == ReverseDir(facing)); CHECK_FALSE(IsTBMHaulerHeld(*rig.hauler)); CHECK_FALSE(GetTBMHaulerSteeringTarget(*rig.hauler));
 REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin);
 CHECK(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin); CHECK(rig.hauler->direction == ReverseDir(facing)); CHECK(FaceCycle(rig).exchanges == 1);
 CHECK(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::Obstruction); CHECK(GetTBMMeetingKind(rig.Project()) == TBMMeetingKind::Bay);
 // A declared arrival uses an actual native origin station tile. This is a
 // cycle component, not evidence that an unbuilt Surface route was driven.
 rig.PlaceOnTrack(rig.hauler, rig.MakeOriginStationTile()); REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer);
 REQUIRE(GetTBMWagonSet(FaceCycle(rig).set)->visited_origin); CHECK(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::None);
 // Declared receiver disposal removes the actual carried packets; no credit,
 // replacement stock or fabricated receipt advances the original cycle.
 for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->cargo.Truncate();
 REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::ReturnEmpties); CHECK(GetTBMWagonSet(FaceCycle(rig).set)->state == TBMWagonSetState::ReturningEmpty);
 CHECK_FALSE(GetTBMWagonSet(FaceCycle(rig).set)->visited_origin); CHECK(FaceCycle(rig).exchanges == 1); CHECK(rig.hauler->current_order.Equals(order));
 CHECK(GetTBMChainIDs(rig.hauler) == hauler_chain); CHECK(GetTBMChainIDs(rig.front) == cutter_chain);
}

/** A rejected whole-load transfer records the normal cycle hold while leaving real packets and physical directions untouched. */
TEST_CASE("Hauler cycle reports insufficient complete face room without a partial transfer or turnaround", "[tbm-face-cycle]")
{
 TBMFaceTransferRig rig; REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
 uint64_t room = 0; for (const Train *wagon : rig.SetWagons(TBMConsistRole::EarthWagon)) room += wagon->cargo_cap;
 REQUIRE(room + 1 < CargoPacket::MAX_COUNT); TBMFaceTransferRig::AddPacket(rig.Wagons(TBMConsistRole::EarthWagon).front(), static_cast<uint16_t>(room + 1));
 const auto before = TBMFaceSnapshot::Capture(rig); const auto held = rig.StepHauler();
 CHECK(held.wait == TBMHaulerWait::NoEmpties); CHECK(held.phase == TBMHaulerPhase::AwaitExchange); CHECK(held.exchanges == 0);
 const auto after = TBMFaceSnapshot::Capture(rig); CHECK(after.packets == before.packets); CHECK(after.facts == before.facts); CHECK(after.trains == before.trains);
 CHECK(after.cutter == before.cutter); CHECK(after.hauler == before.hauler); CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::NoEmpties);
}

/** An inside empty hauler clears a returning empty cutter without creating a new collection obligation. */
TEST_CASE("Empty cutter return sends its already-inside empty hauler outward once", "[tbm-face-cycle]")
{
 TBMFaceTransferRig rig; rig.hauler->vehstatus.Reset(VehState::Stopped);
 // Explicit component emptying changes actual spoil, never the geological
 // cursor or a synthetic exchange-completed flag.
 for (Train *part = rig.front; part != nullptr; part = part->Next()) part->cargo.Truncate();
 const auto input = ObserveTBMHaulerInputs(FaceCycle(rig)); REQUIRE(input.inside); CHECK(input.exchange_finished); CHECK_FALSE(input.cutter_waiting);
 const auto facing = rig.hauler->direction; const auto before = CountTBMPackets({rig.front, rig.hauler});
 CHECK(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin); CHECK(rig.hauler->direction == ReverseDir(facing));
 CHECK_FALSE(IsTBMHaulerHeld(*rig.hauler)); CHECK(FaceCycle(rig).exchanges == 0); CHECK(CountTBMPackets({rig.front, rig.hauler}) == before);
 CHECK(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::Obstruction); rig.PlaceInDepot(rig.hauler, rig.depot);
 CHECK(GetTBMTravelHoldReason(*rig.front) == TBMBlockedReason::None); CHECK_FALSE(IsTBMHaulerAdmittedThroughPortal(*rig.hauler, rig.Key()));
}

/** A physical native turnaround cannot strand even one follower still in magazine storage. */
TEST_CASE("Face departure and direct backing wait for every stored follower", "[tbm-face-cycle]")
{
 TBMFaceTransferRig rig; auto *tail = rig.hauler->Last(); const auto tile = tail->tile; const auto track = tail->track;
 const auto x = tail->x_pos, y = tail->y_pos; const auto facing = rig.hauler->direction;
 tail->tile = rig.Key(); tail->track = Track::Depot; tail->x_pos = TileX(rig.Key()) * TILE_SIZE + 8; tail->y_pos = TileY(rig.Key()) * TILE_SIZE + 8; tail->UpdatePosition();
 const auto before = TBMFaceSnapshot::Capture(rig); BackUpTBMHauler(*rig.hauler); CHECK(TBMFaceSnapshot::Capture(rig) == before);
 REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange); REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
 CHECK(rig.hauler->direction == facing); CHECK(IsTBMHaulerHeld(*rig.hauler)); CHECK(FaceCycle(rig).exchanges == 1);
 // Finish the declared storage transition before the original cycle is
 // allowed to ask the native turnaround owner to move the complete body.
 tail->tile = tile; tail->track = track; tail->x_pos = x; tail->y_pos = y; tail->UpdatePosition(); rig.hauler->vehstatus.Reset(VehState::Stopped);
 rig.StepHauler(); CHECK(rig.hauler->direction == ReverseDir(facing)); CHECK_FALSE(IsTBMHaulerHeld(*rig.hauler)); CHECK(FaceCycle(rig).exchanges == 1);
}

/** Stale stored-route loss and a genuinely blocked running line must not collapse into one logistics hold. */
TEST_CASE("Hauler observation ignores stale stored or occupied Source loss but keeps real running loss", "[tbm-face-cycle]")
{
 TBMFaceRig rig; rig.BoreClearOfHousing(); rig.StandAtFace(); rig.Assign(); rig.hauler->vehicle_flags.Set(VehicleFlag::PathfinderLost);
 auto before = TBMFaceSnapshot::Capture(rig); CHECK_FALSE(ObserveTBMHaulerInputs(FaceCycle(rig)).lost); CHECK(TBMFaceSnapshot::Capture(rig) == before);
 rig.PlaceOnTrack(rig.hauler, rig.Path()[1]); CHECK(ObserveTBMHaulerInputs(FaceCycle(rig)).lost);
 auto *other = rig.BuildHauler(rig.depot, 2, 2); rig.PlaceOnTrack(other, rig.Key()); CHECK_FALSE(ObserveTBMHaulerInputs(FaceCycle(rig)).lost);
 rig.StoreInMagazine(other); CHECK(ObserveTBMHaulerInputs(FaceCycle(rig)).lost);
 rig.StoreInMagazine(rig.hauler); before = TBMFaceSnapshot::Capture(rig); CHECK_FALSE(ObserveTBMHaulerInputs(FaceCycle(rig)).lost);
 ChooseTBMHaulerRouteDirection(*rig.hauler); CHECK(TBMFaceSnapshot::Capture(rig) == before); // Stored/stop authority prevents a recovery turn.
}

/** Native reversal schedules braking once and never swaps parts again while its pending turn is already owned by movement. */
TEST_CASE("Moving face hauler schedules one native braking reversal", "[tbm-face-cycle]")
{
 TBMFaceTransferRig rig; rig.hauler->cur_speed = 12; const auto chain = GetTBMChainIDs(rig.hauler); const auto facing = rig.hauler->direction;
 BackUpTBMHauler(*rig.hauler); CHECK(rig.hauler->flags.Test(VehicleRailFlag::Reversing)); CHECK(rig.hauler->direction == facing);
 const auto before = TBMFaceSnapshot::Capture(rig); BackUpTBMHauler(*rig.hauler); CHECK(TBMFaceSnapshot::Capture(rig) == before); CHECK(GetTBMChainIDs(rig.hauler) == chain);
}
