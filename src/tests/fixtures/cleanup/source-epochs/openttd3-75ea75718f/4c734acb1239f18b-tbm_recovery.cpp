/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_recovery.cpp Recoverable cancellation and reassignment (Plan 4 Group 17): a cancelled project releases
 * only its unfinished cells, returns to its bay, gives away its spoil through the real exchange and ends Cancelled;
 * breakdown and service are saved pauses; a finished project retires only once nothing depends on it; and a lost
 * origin or bay is a visible block that explicit reassignment clears.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 17
 * @see docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-j-lifecycle-design.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"
#include "../vehicle_cmd.h"
#include "../company_base.h"
#include "../station_base.h"
#include "../layer_portal_cmd.h"

#include "../safeguards.h"

/**
 * Apply one project control through the ordinary command.
 * @param project Project to control.
 * @param action Control.
 * @param flags Command flags; executed by default, empty flags estimate only.
 * @return The command result.
 */
static CommandCost Control(TBMProjectID project, TBMProjectAction action, DoCommandFlags flags = DoCommandFlag::Execute)
{
	return Command<Commands::ControlTBMProject>::Do(flags, project, action);
}

/**
 * @param job Job whose claims are counted.
 * @return Unfinished cells the job still claims.
 */
static size_t CountClaims(ExcavationJobID job)
{
	return std::ranges::count_if(GetGeologyWorldState().job_claims, [job](const auto &claim) { return claim.second == job; });
}

/** Cancelling before any cut turns the consist back, and with nothing aboard the project ends at once. */
TEST_CASE("Cancelling before the first cut ends the project at its bay", "[tbm-recovery]")
{
	TBMWorkRig rig;
	const auto id = rig.project.id;
	const auto job = rig.project.job;
	const auto projects = ComputeTBMProjectDigest();
	REQUIRE(Control(id, TBMProjectAction::Cancel, DoCommandFlags{}).Succeeded());
	CHECK(ComputeTBMProjectDigest() == projects); // An estimate changes nothing.
	CHECK(rig.Job().state != ExcavationJobState::Cancelled);

	REQUIRE(Control(id, TBMProjectAction::Cancel).Succeeded());
	CHECK(GetExcavationJob(job)->state != ExcavationJobState::Cancelled);
	CHECK(CountClaims(job) != 0); // The request keeps future claims until the actual withdrawal check.
	CHECK(rig.Project().phase == TBMPhase::ReturnToRelease);
	CHECK(Control(id, TBMProjectAction::Cancel).Failed()); // The same pending intent cannot be queued twice.
	CHECK(rig.Tick().phase == TBMPhase::WaitToRelease); // The consist stands in its Source bay.
	CHECK(rig.Tick().phase == TBMPhase::Cancelled);
	CHECK(CountClaims(job) == 0);
	CHECK(rig.Project().blocked == TBMBlockedReason::None);
	CHECK(ValidateTBMProject(rig.Project()) == TBMProjectError::None); // The cancelled record still loads.
}

/** A partial cut is abandoned: its unfinished cells return to rock, every earlier cavity stays, no money moves. */
TEST_CASE("Cancelling during a partial cut keeps every cavity and releases only unfinished cells", "[tbm-recovery]")
{
	TBMWorkRig rig(2);
	rig.BoreOneCut();
	rig.ExchangeAndReturn();
	const TileIndex first = rig.Job().route[0].cutter_position;
	REQUIRE(GetGeologyWorldState().terrain.Get(first) == ExcavationState::Excavated);
	const auto estimate = EstimateExcavationCut(rig.project.job);
	REQUIRE(estimate.error == ExcavationError::None);
	for (uint64_t tick = 0; tick < estimate.work_ticks / 2; ++tick) REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	REQUIRE(rig.Job().partial_work_ticks != 0);
	REQUIRE(CountClaims(rig.project.job) != 0);
	const uint32_t cursor = rig.Job().cursor;
	const Money cash = Company::Get(_current_company)->money;

	REQUIRE(Control(rig.project.id, TBMProjectAction::Cancel).Succeeded());
	CHECK(rig.Job().state != ExcavationJobState::Cancelled);
	CHECK(rig.Job().cursor == cursor); // The committed cut stays committed.
	CHECK(rig.Job().partial_work_ticks != 0); // Retain partial work until the actual withdrawal and settlement finish.
	CHECK(CountClaims(rig.project.job) != 0);
	CHECK(GetGeologyWorldState().terrain.Get(first) == ExcavationState::Excavated);
	CHECK(Company::Get(_current_company)->money == cash); // Neither a refund nor a charge.
	CHECK(rig.Project().phase == TBMPhase::ReturnToRelease);
	CHECK(rig.Tick().phase == TBMPhase::WaitToRelease);
	CHECK(rig.Tick().phase == TBMPhase::Cancelled);
	CHECK(rig.Job().partial_work_ticks == 0);
	CHECK(CountClaims(rig.project.job) == 0);
	CHECK(GetGeologyWorldState().terrain.Get(first) == ExcavationState::Excavated);
}

/** Full wagons go to a hauler through the real exchange before the cancelled project ends; short empties are enough. */
TEST_CASE("A cancelled project gives its spoil away through the exchange first", "[tbm-recovery]")
{
	TBMExchangeRig rig(2, 1, 2); // One Earth hopper: too few empties for another pass, enough to take a last load.
	rig.MakeOriginStationTile();
	const auto estimate = rig.BoreOneCut();
	const uint64_t spoil = estimate.earth_t + estimate.rock_t;
	REQUIRE(Control(rig.project.id, TBMProjectAction::Cancel).Succeeded());
	CHECK(rig.Project().phase == TBMPhase::ReturnToRelease);
	CHECK(rig.Tick().phase == TBMPhase::WaitToRelease);
	CHECK(rig.Tick().phase == TBMPhase::WaitToRelease); // The spoil aboard keeps the exchange due.
	CHECK(TBMExchangeRig::Aboard(rig.front) == spoil);

	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	const auto collected = rig.StepHauler();
	CHECK(collected.phase == TBMHaulerPhase::CollectLoaded); // No next pass, so the short empties do not refuse.
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == spoil);
	CHECK(TBMExchangeRig::Aboard(rig.front) == 0);
	CHECK(rig.Tick().phase == TBMPhase::ReleasePending);
	CHECK(rig.Tick().phase == TBMPhase::ReleasePending);
	CHECK(rig.Job().state != ExcavationJobState::Cancelled); // Swapping wagons is not final delivery of the hauler's real load.

	// The finished project's last load still travels to the origin.
	rig.PlaceOnTrack(rig.hauler, TileXYL(25, 20, MapLayer::Surface));
	CHECK(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == spoil);
}

/** A broken-down cutter holds its face as a saved pause: no work accrues, the hold reloads, and work resumes after. */
TEST_CASE("Breakdown holds the face as a saved pause", "[tbm-recovery]")
{
	TBMWorkRig rig;
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	const uint64_t partial = rig.Job().partial_work_ticks;
	rig.front->breakdown_ctr = 1;
	CHECK(rig.Tick().blocked == TBMBlockedReason::Breakdown);
	CHECK(rig.Tick().blocked == TBMBlockedReason::Breakdown);
	CHECK(rig.Job().partial_work_ticks == partial); // A broken-down cutter does not work the face.
	rig.Reload();
	CHECK(rig.Project().blocked == TBMBlockedReason::Breakdown); // The pause is saved with the project.
	CHECK(rig.Project().phase == TBMPhase::Bore);
	rig.front->breakdown_ctr = 0;
	CHECK(rig.Tick().blocked == TBMBlockedReason::None);
	CHECK(rig.Job().partial_work_ticks > partial);
}

/** A finished project retires only once no hauler, spoil or train position depends on it, and withdraws its pair. */
TEST_CASE("A finished project retires only when nothing depends on it", "[tbm-recovery]")
{
	TBMExchangeRig rig;
	rig.MakeOriginStationTile();
	const auto id = rig.project.id;
	CHECK(Control(id, TBMProjectAction::Retire).Failed()); // Unfinished.
	rig.BoreOneCut();
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
	REQUIRE(rig.Tick().phase == TBMPhase::Resume);
	REQUIRE(rig.Tick().phase == TBMPhase::Completed);
	CHECK(Control(id, TBMProjectAction::Retire).Failed()); // A hauler still serves it.
	REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
	CHECK(Control(id, TBMProjectAction::Retire).Failed()); // Its released set still carries the spoil.
	for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->cargo.Truncate(); // Stand-in for disposal.
	CHECK(Control(id, TBMProjectAction::Retire).Failed()); // The released hauler still occupies the bay.
	rig.PlaceOnTrack(rig.hauler, TileXYL(25, 20, MapLayer::Surface)); // Declared owner-fixture departure; native physical release is covered by tbm_release.cpp.
	rig.front->vehstatus.Set(VehState::Stopped);
	const TileIndex portal = rig.fixture.request.portal;
	REQUIRE(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Commissioning);
	const auto cutter = GetTBMChainIDs(rig.front);
	REQUIRE(Control(id, TBMProjectAction::Retire, DoCommandFlags{}).Succeeded());
	REQUIRE(GetTBMProject(id) != nullptr); // An estimate changes nothing.
	CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Commissioning);
	REQUIRE(Control(id, TBMProjectAction::Retire).Succeeded());
	// The pair is withdrawn in the same action: Sealed again, bound to nothing (r2mlvs3 Group 11.3).
	CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Sealed);
	CHECK(GetLayerPortalWorkRecord(portal)->project == ExcavationJobID{UINT32_MAX});
	CHECK(GetLayerPortalWorkRecord(portal)->cutter == VehicleID::Invalid());
	CHECK(GetLayerPortalWorkRecord(portal)->origin == StationID::Invalid());
	CHECK(ValidateLayerPortalWorkRecords(CaptureLayerPortalWorkRecords(), GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
	CHECK(GetTBMProject(id) == nullptr);
	CHECK(GetTBMProjectForCutter(rig.front->index) == nullptr);
	CHECK(CaptureTBMWagonSets().empty()); // Only empty sets were left, and they are gone with it.
	CHECK(GetTBMChainIDs(rig.front) == cutter); // The consist itself is untouched.
	CHECK(TBMExchangeSave{}.Reloads());
	// It is an ordinary train now, and may be sold.
	REQUIRE(Command<Commands::SellVehicle>::Do(DoCommandFlag::Execute, rig.front->index, true, false, ClientID::Invalid).Succeeded());
	rig.front = nullptr;
}

/** A deleted cutter frees its ID at once; its project is still cancelled and retired, and its pair withdrawn. */
TEST_CASE("A deleted cutter frees its ID and its project still ends and retires", "[tbm-recovery]")
{
	TBMExchangeLoadRig rig; // The hauler is on its way with the spoil; the cutter rides its empty Assigned set.
	const auto id = rig.project.id;
	const TileIndex portal = rig.fixture.request.portal;
	const VehicleID cutter = rig.front->index;
	const ExcavationJobState job_state = rig.Job().state;
	delete rig.front; // As after a crash: the whole consist goes.
	rig.front = nullptr;
	CHECK(rig.Project().cutter == VehicleID::Invalid());
	CHECK(rig.Project().permanent_parts.empty());
	CHECK(rig.Project().blocked == TBMBlockedReason::MissingCutter);
	CHECK(GetTBMProjectForCutter(cutter) == nullptr);
	CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Commissioning); // Still bound to its job,
	CHECK(GetLayerPortalWorkRecord(portal)->cutter == VehicleID::Invalid()); // with no cutter.
	CHECK(ValidateTBMProject(rig.Project()) == TBMProjectError::None);
	CHECK(ValidateLayerPortalWorkRecords(CaptureLayerPortalWorkRecords(), GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
	CHECK(TBMExchangeSave{}.Reloads());

	// New trains may take the freed IDs at once; none becomes a cutter or inherits its lock.
	const Train *fresh = rig.BuildHauler(rig.depot, 2, 2);
	for (const Train *part = fresh; part != nullptr; part = part->Next()) {
		CHECK(GetTBMProjectForCutter(part->index) == nullptr);
		CHECK_FALSE(IsTBMReplacementLocked(*part));
	}

	// A lost cutter needs no withdrawal, but the surviving real haulage still has to settle before claims end.
	REQUIRE(Control(id, TBMProjectAction::Cancel).Succeeded());
	CHECK(rig.Project().phase == TBMPhase::ReleasePending);
	CHECK(rig.Job().state == job_state);
	CHECK(Control(id, TBMProjectAction::Retire).Failed()); // Its hauler still carries the spoil.
	REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
	for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->cargo.Truncate(); // Stand-in for disposal.
	REQUIRE(Control(id, TBMProjectAction::Retire).Succeeded());
	CHECK(GetTBMProject(id) == nullptr);
	CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Sealed);
	CHECK(ValidateLayerPortalWorkRecords(CaptureLayerPortalWorkRecords(), GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
}

/** A cancelled project saved in the middle of its return reloads and ends the same way (Plan 15 13.2). */
TEST_CASE("A cancelled project reloads mid-return and ends the same way", "[tbm-recovery]")
{
	TBMExchangeRig rig(2);
	rig.MakeOriginStationTile();
	const auto estimate = rig.BoreOneCut();
	REQUIRE(Control(rig.project.id, TBMProjectAction::Cancel).Succeeded());
	REQUIRE(rig.Tick().phase == TBMPhase::WaitToRelease);
	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	const auto projects = ComputeTBMProjectDigest();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	// Save and reload both stores here: the project reload clears the exchange store, which the save carries across.
	const TBMExchangeSave keep;
	rig.Reload();
	REQUIRE(keep.Load());
	CHECK(ComputeTBMProjectDigest() == projects);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(rig.Project().phase == TBMPhase::WaitToRelease);
	CHECK(rig.Job().state != ExcavationJobState::Cancelled);
	CHECK(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == estimate.earth_t + estimate.rock_t);
	CHECK(rig.Tick().phase == TBMPhase::ReleasePending);
	CHECK(rig.Tick().phase == TBMPhase::ReleasePending);
}

/** A deleted origin is forgotten at once, by the project and its pair alike; a station reusing its ID never rebinds. */
TEST_CASE("A deleted origin is forgotten at once and its ID never rebinds", "[tbm-recovery]")
{
	TBMWorkRig rig;
	const TileIndex portal = rig.fixture.request.portal;
	const StationID origin = rig.fixture.origin->index;
	REQUIRE(rig.Project().origin == origin);
	REQUIRE(GetLayerPortalWorkRecord(portal)->origin == origin);
	delete rig.fixture.origin; // As when a removed station's sign finally goes.
	rig.fixture.origin = nullptr;
	CHECK(rig.Project().origin == StationID::Invalid());
	CHECK(rig.Project().blocked == TBMBlockedReason::MissingOrigin);
	CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Commissioning);
	CHECK(GetLayerPortalWorkRecord(portal)->origin == StationID::Invalid());
	CHECK(ValidateTBMProject(rig.Project()) == TBMProjectError::None);
	CHECK(ValidateLayerPortalWorkRecords(CaptureLayerPortalWorkRecords(), GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None); // The pair still loads.

	// The company's next station takes the freed ID; it becomes the origin only when chosen.
	REQUIRE(Station::CanAllocateItem());
	Station *reuse = Station::Create(TileXYL(26, 24, MapLayer::Surface));
	REQUIRE(reuse->index == origin);
	reuse->owner = _current_company;
	reuse->facilities.Set(StationFacility::Train);
	CHECK(rig.Tick().blocked == TBMBlockedReason::MissingOrigin);
	CHECK(rig.Project().origin == StationID::Invalid());
	REQUIRE(Command<Commands::SetTBMProjectOrigin>::Do(DoCommandFlag::Execute, rig.project.id, reuse->index).Succeeded());
	CHECK(rig.Tick().blocked == TBMBlockedReason::None);
}

/** A lost origin or bay is a visible block with every tonne kept aboard; explicit reassignment clears it. */
TEST_CASE("A lost origin or bay recovers through explicit reassignment", "[tbm-recovery]")
{
	TBMExchangeRig rig;
	rig.MakeOriginStationTile();

	SECTION("origin") {
		rig.fixture.origin->facilities.Reset(StationFacility::Train);
		CHECK(rig.Tick().blocked == TBMBlockedReason::MissingOrigin);
		CHECK(rig.Project().phase == TBMPhase::Bore);
		REQUIRE(Station::CanAllocateItem());
		Station *replacement = Station::Create(TileXYL(26, 24, MapLayer::Surface));
		replacement->owner = _current_company;
		replacement->facilities.Set(StationFacility::Train);
		CHECK(Command<Commands::SetTBMProjectOrigin>::Do(DoCommandFlag::Execute, rig.project.id, rig.fixture.origin->index).Failed()); // Same origin.
		REQUIRE(Command<Commands::SetTBMProjectOrigin>::Do(DoCommandFlag::Execute, rig.project.id, replacement->index).Succeeded());
		CHECK(rig.Tick().blocked == TBMBlockedReason::None);
		CHECK(rig.Project().origin == replacement->index);
	}
	SECTION("bay, with a hauler queued") {
		const TileIndex depot = rig.BuildDepotBay(3);
		REQUIRE(Command<Commands::SetTBMExchangeBay>::Do(DoCommandFlag::Execute, rig.project.id, depot).Succeeded());
		rig.Assign();
		// The approach loses a tile: the bay no longer resolves and the project shows it.
		REQUIRE(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, TileXYL(43, 40, MapLayer::Underground), Track::X).Succeeded());
		CHECK(rig.Tick().blocked == TBMBlockedReason::MissingExchange);
		// A lost bay may be replaced under its queued hauler, which follows it.
		REQUIRE(Command<Commands::SetTBMExchangeBay>::Do(DoCommandFlag::Execute, rig.project.id, rig.fixture.request.portal).Succeeded());
		CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->bay == rig.fixture.request.portal);
		CHECK(rig.Tick().blocked == TBMBlockedReason::None);
		CHECK(TBMExchangeSave{}.Reloads());
		// With the approach whole again the depot would resolve, but a working bay still cannot move under a queued hauler.
		REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, TileXYL(43, 40, MapLayer::Underground), RAILTYPE_RAIL, Track::X, false).Succeeded());
		REQUIRE(ResolveTBMExchangeBay(rig.Project(), depot).error == TBMExchangeError::None);
		CHECK(Command<Commands::SetTBMExchangeBay>::Do(DoCommandFlag::Execute, rig.project.id, depot).Failed());
		CHECK(rig.Project().exchange_bay == rig.fixture.request.portal);
	}
}
