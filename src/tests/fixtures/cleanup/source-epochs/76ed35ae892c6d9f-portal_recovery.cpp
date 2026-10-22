/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file portal_recovery.cpp Assigned commissioning pairs through the real TBM controller (r2mlvs3 Group 11.3/11.4):
 * the pair is withdrawn only once its job is closed and its rock side empty, a retired project leaves an empty pair the
 * ordinary lifecycle removes from either end, a merger moves an assigned pair with its project, and company removal
 * releases and clears it. Once its TBM is withdrawn, a pair connects for ordinary freight, and a consist spanning both
 * ends refuses the change and keeps its place (Group 9.3). No case leaves a half-pair.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Groups 9 and 11
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-j-lifecycle-design.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"
#include "../vehicle_cmd.h"
#include "../landscape_cmd.h"
#include "../tile_cmd.h"
#include "../signal_func.h"
#include "../company_base.h"
#include "../station_base.h"
#include "../layer_portal_map.h"
#include "../rail_cmd.h"
#include "../track_func.h"

#include "../safeguards.h"

/**
 * Cancel the rig's project and run it to Cancelled; with nothing aboard it ends after its return.
 * @param rig Rig whose project is cancelled.
 */
static void CancelToEnd(const TBMWorkRig &rig)
{
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Cancel).Succeeded());
	REQUIRE(rig.Tick().phase == TBMPhase::WaitToRelease);
	REQUIRE(rig.Tick().phase == TBMPhase::Cancelled);
}

/**
 * Put a whole train on one tile as a part standing there would be, for the ground checks.
 * @param head Train to place.
 * @param tile Tile to stand on.
 */
static void StandOn(Train *head, TileIndex tile)
{
	for (Train *part = head; part != nullptr; part = part->Next()) {
		part->tile = tile;
		part->x_pos = TileX(tile) * TILE_SIZE + TILE_SIZE / 2;
		part->y_pos = TileY(tile) * TILE_SIZE + TILE_SIZE / 2;
		part->z_pos = 0;
		part->UpdatePosition();
	}
}

/** The controller's withdrawal needs exactly its own binding, a closed job and an empty rock side. */
TEST_CASE("A commissioning pair is withdrawn only after its work and with an empty rock side", "[portal-recovery]")
{
	TBMExchangeRig rig;
	const TileIndex portal = rig.fixture.request.portal;
	const TileIndex rock = GetLayerPortalPairTile(portal);
	const ExcavationJobID job = rig.project.job;
	const VehicleID cutter = rig.front->index;
	CHECK(WithdrawLayerPortalWorkPair(portal, job, cutter, false) == LayerPortalWorkError::Conflict); // The job is open.
	CHECK(WithdrawLayerPortalWorkPair(portal, ExcavationJobID{job.base() + 1}, cutter, false) == LayerPortalWorkError::Conflict);
	CHECK(WithdrawLayerPortalWorkPair(portal, job, VehicleID{cutter.base() + 1000}, false) == LayerPortalWorkError::Conflict);
	CancelToEnd(rig);

	// Any part on the rock-side end keeps the pair, whichever train it belongs to.
	Train *other = rig.BuildHauler(rig.depot, 1, 0);
	const TileIndex home = other->tile;
	StandOn(other, rock);
	CHECK(WithdrawLayerPortalWorkPair(portal, job, cutter, false) == LayerPortalWorkError::Conflict);
	StandOn(other, home);

	const uint64_t revision = GetLayerPortalWorkRecord(portal)->revision;
	CHECK(WithdrawLayerPortalWorkPair(rock, job, cutter, false) == LayerPortalWorkError::None); // Either end names the pair.
	CHECK(GetLayerPortalWorkRecord(portal)->revision == revision); // An estimate changes nothing.
	REQUIRE(WithdrawLayerPortalWorkPair(portal, job, cutter, true) == LayerPortalWorkError::None);
	const auto *record = GetLayerPortalWorkRecord(portal);
	CHECK(record->state == LayerPortalWorkState::Sealed);
	CHECK(record->project == ExcavationJobID{UINT32_MAX});
	CHECK(record->cutter == VehicleID::Invalid());
	CHECK(record->origin == StationID::Invalid());
	CHECK(record->revision == revision + 1);
	CHECK(GetLayerPortalWorkRecord(rock) == record); // One record for both ends.
	CHECK(WithdrawLayerPortalWorkPair(portal, job, cutter, false) == LayerPortalWorkError::Conflict); // Nothing is left to release.
	CHECK(ValidateLayerPortalWorkRecords(CaptureLayerPortalWorkRecords(), GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
}

/** After retirement the pair is an ordinary empty pair: it is removed whole from either end once the train has gone. */
TEST_CASE("A retired project leaves an empty pair that is removed whole", "[portal-recovery]")
{
	TBMWorkRig rig;
	const TileIndex portal = rig.fixture.request.portal;
	const TileIndex rock = GetLayerPortalPairTile(portal);
	CancelToEnd(rig);
	rig.front->vehstatus.Set(VehState::Stopped);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Retire).Succeeded());
	REQUIRE(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Sealed);
	CHECK(CheckLayerPortalLifecycle(portal) == LayerPortalWorkError::Conflict); // The withdrawn consist still stands at the Surface key.
	CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rock).Failed());
	CHECK(IsLayerPortalTile(portal));
	CHECK(IsLayerPortalTile(rock)); // A refusal at either end leaves both.

	REQUIRE(Command<Commands::SellVehicle>::Do(DoCommandFlag::Execute, rig.front->index, true, false, ClientID::Invalid).Succeeded());
	rig.front = nullptr;
	CHECK(CheckLayerPortalLifecycle(portal) == LayerPortalWorkError::None);
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rock).Succeeded());
	CHECK_FALSE(IsLayerPortalTile(portal));
	CHECK_FALSE(IsLayerPortalTile(rock));
	CHECK(GetLayerPortalWorkRecord(portal) == nullptr);
}

/** A merger moves an assigned pair together with its project, job, trains and stations, and only once. */
TEST_CASE("A merger moves an assigned pair with its project", "[portal-recovery]")
{
	TBMExchangeRig rig;
	rig.MakeOriginStationTile();
	const TileIndex portal = rig.fixture.request.portal;
	const TileIndex rock = GetLayerPortalPairTile(portal);
	const CompanyID old_owner = _current_company;
	REQUIRE(Company::CanAllocateItem());
	std::unique_ptr<Company> buyer{Company::Create()};
	// The company change moves vehicles and stations before its tile loop; only that part of it is reproduced here.
	for (Vehicle *v : Vehicle::Iterate()) {
		if (v->owner == old_owner) v->owner = buyer->index;
	}
	for (Station *st : Station::Iterate()) {
		if (st->owner == old_owner) st->owner = buyer->index;
	}
	ChangeTBMOwnership(old_owner, buyer->index);
	_tile_type_procs[TileType::LayerPortal]->change_tile_owner_proc(rock, old_owner, buyer->index);
	CHECK(GetTileOwner(portal) == buyer->index);
	CHECK(GetTileOwner(rock) == buyer->index);
	const auto *record = GetLayerPortalWorkRecord(portal);
	REQUIRE(record != nullptr);
	CHECK(record->company == buyer->index);
	CHECK(record->state == LayerPortalWorkState::Commissioning);
	CHECK(record->project == rig.project.job);
	CHECK(record->cutter == rig.front->index);
	const uint64_t revision = record->revision;
	_tile_type_procs[TileType::LayerPortal]->change_tile_owner_proc(portal, old_owner, buyer->index);
	CHECK(GetLayerPortalWorkRecord(portal)->revision == revision); // The second end's callback does not move it twice.
	CHECK(ValidateLayerPortalWorkRecords(CaptureLayerPortalWorkRecords(), GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
	CHECK(FindTBMProjectReferenceLoss(rig.Project()) == TBMBlockedReason::None); // The project still works there.
	UpdateSignalsInBuffer();
}

/**
 * Put one train part on a tile, as a part standing there would be, for the ground checks.
 * @param part Part to place.
 * @param tile Tile to stand on.
 */
static void StandPartOn(Train *part, TileIndex tile)
{
	part->tile = tile;
	part->x_pos = TileX(tile) * TILE_SIZE + TILE_SIZE / 2;
	part->y_pos = TileY(tile) * TILE_SIZE + TILE_SIZE / 2;
	part->z_pos = 0;
	part->UpdatePosition();
}

/** After its TBM is withdrawn the pair connects for ordinary freight; a consist across both ends refuses and keeps its place. */
TEST_CASE("A withdrawn pair connects for freight and refuses a spanning consist", "[portal-recovery]")
{
	TBMExchangeRig rig;
	rig.MakeOriginStationTile();
	const TileIndex portal = rig.fixture.request.portal;
	const TileIndex rock = GetLayerPortalPairTile(portal);
	// One complete project: its cut lays support out of the door, and the exchange takes the last load.
	rig.BoreOneCut();
	REQUIRE_FALSE(CaptureTBMSupport().empty());
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
	REQUIRE(rig.Tick().phase == TBMPhase::Resume);
	REQUIRE(rig.Tick().phase == TBMPhase::Completed);
	REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
	for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->cargo.Truncate(); // Stand-in for disposal.
	rig.PlaceInDepot(rig.hauler, rig.depot);
	rig.front->vehstatus.Set(VehState::Stopped);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Retire).Succeeded());
	REQUIRE(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Sealed);
	CHECK(ChangeLayerPortalConnection(portal, _current_company, true, false) == LayerPortalWorkError::Conflict); // The withdrawn consist still stands at the Surface end.
	REQUIRE(Command<Commands::SellVehicle>::Do(DoCommandFlag::Execute, rig.front->index, true, false, ClientID::Invalid).Succeeded());
	rig.front = nullptr;

	// The released support becomes permanent track, and the Surface approach gets its own axis track.
	for (const auto &support : CaptureTBMSupport()) REQUIRE(Command<Commands::ConvertTBMSupport>::Do(DoCommandFlag::Execute, support.tile).Succeeded());
	CHECK(CaptureTBMSupport().empty());
	REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, GetLayerPortalDoorTile(portal), RAILTYPE_RAIL, DiagDirToDiagTrack(GetLayerPortalShaftDirection(portal)), false).Succeeded());

	// A consist with parts at both ends keeps the pair sealed and every part where it stands.
	Train *head = rig.hauler;
	Train *tail = head->Last();
	StandPartOn(head, portal);
	StandPartOn(tail, rock);
	CHECK(ChangeLayerPortalConnection(portal, _current_company, true, true) == LayerPortalWorkError::Conflict);
	CHECK(head->tile == portal);
	CHECK(tail->tile == rock);
	CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Sealed);

	// Once it has gone, the route opens: ordinary routing sees the shaft at both ends.
	rig.PlaceInDepot(head, rig.depot);
	REQUIRE(ChangeLayerPortalConnection(portal, _current_company, true, false) == LayerPortalWorkError::None);
	REQUIRE(ChangeLayerPortalConnection(portal, _current_company, true, true) == LayerPortalWorkError::None);
	CHECK(GetLayerPortalWorkState(rock) == LayerPortalWorkState::Connected);
	CHECK(GetTileTrackStatus(portal, TransportType::Rail, RoadTramType::Invalid).trackdirs.Any());
	CHECK(GetTileTrackStatus(rock, TransportType::Rail, RoadTramType::Invalid).trackdirs.Any());
	CHECK(ValidateLayerPortalWorkRecords(CaptureLayerPortalWorkRecords(), GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
	UpdateSignalsInBuffer();
}

/** Company removal releases the assigned pair of an erased project, and the rail pair then goes with the company. */
TEST_CASE("Company removal releases and clears an assigned pair", "[portal-recovery]")
{
	TBMWorkRig rig;
	const TileIndex portal = rig.fixture.request.portal;
	const TileIndex rock = GetLayerPortalPairTile(portal);
	const CompanyID old_owner = _current_company;
	REQUIRE(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Commissioning);
	delete rig.front; // Company removal deletes the trains first.
	rig.front = nullptr;
	ChangeTBMOwnership(old_owner, INVALID_OWNER);
	CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Sealed);
	CHECK(GetExcavationJob(rig.project.job)->state == ExcavationJobState::Cancelled);
	CHECK(CheckLayerPortalLifecycle(portal) == LayerPortalWorkError::None);
	_tile_type_procs[TileType::LayerPortal]->change_tile_owner_proc(portal, old_owner, INVALID_OWNER);
	CHECK_FALSE(IsLayerPortalTile(portal));
	CHECK_FALSE(IsLayerPortalTile(rock));
	CHECK(GetLayerPortalWorkRecord(portal) == nullptr);
	UpdateSignalsInBuffer();
}
