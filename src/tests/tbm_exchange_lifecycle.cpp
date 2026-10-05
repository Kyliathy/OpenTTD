/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange_lifecycle.cpp Fleet and company lifecycle around assignments (Plan 5 Group 11): ordinary commands
 * cannot change a cutter consist, spoil-carrying set units cannot be sold, set units are not refitted or renewed,
 * deleted trains leave the exchange store, a depot bay goes only with a fallback, and a merger or company removal keeps
 * every store loadable. After each event the exchange store reloads unchanged.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 11
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-j-lifecycle-design.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"
#include "../vehicle_cmd.h"
#include "../autoreplace_cmd.h"
#include "../landscape_cmd.h"
#include "../company_base.h"
#include "../station_base.h"
#include "../table/strings.h"

#include "../safeguards.h"

/**
 * Sell through the ordinary command, as the depot window does.
 * @param id Vehicle to sell.
 * @param chain Sell every unit behind it too.
 * @return The command result.
 */
static CommandCost Sell(VehicleID id, bool chain)
{
	return Command<Commands::SellVehicle>::Do(DoCommandFlag::Execute, id, chain, false, ClientID::Invalid);
}

/**
 * Refit through the ordinary command to passengers.
 * @param id Vehicle to refit.
 * @param only_this Refit this unit alone rather than the train from it.
 * @return The command result.
 */
static CommandCost Refit(VehicleID id, bool only_this)
{
	return std::get<0>(Command<Commands::RefitVehicle>::Do(DoCommandFlag::Execute, id, GetCargoTypeByLabel(CT_PASSENGERS), 0, false, only_this, 0));
}

/** No ordinary move, sale, refit or replacement changes a project's cutter consist; other trains are unaffected. */
TEST_CASE("Ordinary commands cannot change a cutter consist", "[tbm-exchange-lifecycle]")
{
	TBMExchangeRig rig;
	rig.PlaceInDepot(rig.front, rig.depot); // A genuine depot passes every ordinary depot check, so the lock decides.
	const auto cutter = GetTBMChainIDs(rig.front);
	CHECK(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, rig.front->Last()->index, VehicleID::Invalid(), false).GetErrorMessage() == STR_ERROR_TBM_CONSIST_LOCKED);
	CHECK(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, rig.hauler->Last()->index, rig.front->Last()->index, false).GetErrorMessage() == STR_ERROR_TBM_CONSIST_LOCKED);
	CHECK(Sell(rig.front->index, true).GetErrorMessage() == STR_ERROR_TBM_CONSIST_LOCKED);
	CHECK(Sell(rig.front->Last()->index, false).GetErrorMessage() == STR_ERROR_TBM_CONSIST_LOCKED);
	CHECK(Refit(rig.front->Last()->index, true).GetErrorMessage() == STR_ERROR_TBM_CONSIST_LOCKED);
	CHECK(IsTBMReplacementLocked(*rig.front->Last()));
	CHECK(Command<Commands::AutoreplaceVehicle>::Do(DoCommandFlag::Execute, rig.front->index).Failed());
	CHECK(GetTBMChainIDs(rig.front) == cutter);
	CHECK(GetTBMProjectForCutter(rig.front->index) != nullptr);

	// An ordinary train in the same depot is unaffected.
	const VehicleID last = rig.hauler->Last()->index;
	REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, last, VehicleID::Invalid(), false).Succeeded());
	CHECK(Train::Get(last)->IsFreeWagon());
}

/** Spoil aboard set units blocks their sale; the locomotive may go, and its loaded wagons are recovered by another. */
TEST_CASE("Loaded set units cannot be sold, and a sold locomotive's set is recovered", "[tbm-exchange-lifecycle]")
{
	TBMExchangeLoadRig rig;
	rig.PlaceInDepot(rig.hauler, rig.depot);
	const auto chain = GetTBMChainIDs(rig.hauler);
	const TBMWagonSetID set = GetTBMExchangeAssignmentForHauler(rig.hauler->index)->set;
	CHECK(Sell(rig.hauler->Next()->index, false).GetErrorMessage() == STR_ERROR_TBM_SPOIL_ABOARD);
	CHECK(Sell(rig.hauler->index, true).GetErrorMessage() == STR_ERROR_TBM_SPOIL_ABOARD);
	CHECK(GetTBMChainIDs(rig.hauler) == chain);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == rig.spoil);

	// Only the locomotive is sold: the assignment ends, the loaded wagons stay in the depot with their spoil.
	const VehicleID first_wagon = rig.hauler->Next()->index;
	REQUIRE(Sell(rig.hauler->index, false).Succeeded());
	rig.hauler = nullptr;
	CHECK(CaptureTBMExchangeAssignments().empty());
	REQUIRE(GetTBMWagonSet(set) != nullptr);
	CHECK(GetTBMWagonSet(set)->state == TBMWagonSetState::Loaded);
	CHECK(GetTBMWagonSet(set)->wagons == std::vector<VehicleID>(chain.begin() + 1, chain.end()));
	CHECK(TBMExchangeSave{}.Reloads());

	// The declared recovery: couple the wagons to any locomotive and assign it; it adopts the set on the way to the origin.
	Train *loco = rig.BuildHauler(rig.depot, 0, 0);
	REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, first_wagon, loco->Last()->index, true).Succeeded());
	REQUIRE(AssignTBMHauler(rig.project.id, loco->index, true) == TBMExchangeError::None);
	const auto *adopted = GetTBMExchangeAssignmentForHauler(loco->index);
	CHECK(adopted->set == set);
	CHECK(adopted->phase == TBMHaulerPhase::VisitOrigin);
	CHECK(TBMExchangeRig::Aboard(loco) == rig.spoil);
}

/** Empty set units may be sold: the set shrinks, and with its last wagon both the set and the cycle end. */
TEST_CASE("Empty set units can be sold and the set follows", "[tbm-exchange-lifecycle]")
{
	TBMExchangeRig rig;
	rig.MakeOriginStationTile();
	rig.Assign();
	const TBMWagonSetID set = GetTBMExchangeAssignmentForHauler(rig.hauler->index)->set;
	const uint64_t revision = GetTBMWagonSet(set)->revision;
	REQUIRE(GetTBMWagonSet(set)->wagons.size() == 4);
	REQUIRE(Sell(rig.hauler->Last()->index, false).Succeeded());
	CHECK(GetTBMWagonSet(set)->wagons.size() == 3);
	CHECK(GetTBMWagonSet(set)->revision > revision);
	CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index) != nullptr);
	CHECK(TBMExchangeSave{}.Reloads());

	REQUIRE(Sell(rig.hauler->Next()->index, true).Succeeded());
	CHECK(GetTBMWagonSet(set) == nullptr);
	CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index) == nullptr); // Nothing is left to haul.
	CHECK(TBMExchangeSave{}.Reloads());
}

/** Set units keep their capacity and identities: no refit, no autoreplace; the locomotive alone may be refitted. */
TEST_CASE("Set units refuse refits and autoreplace skips haulers and sets", "[tbm-exchange-lifecycle]")
{
	TBMExchangeRig rig;
	rig.MakeOriginStationTile();
	rig.Assign();
	CHECK(Refit(rig.hauler->Next()->index, true).GetErrorMessage() == STR_ERROR_TBM_SET_REFIT);
	CHECK(Refit(rig.hauler->index, false).GetErrorMessage() == STR_ERROR_TBM_SET_REFIT); // The whole train includes the set.
	CHECK(CheckTBMRefit(*rig.hauler).Succeeded());
	CHECK(IsTBMReplacementLocked(*rig.hauler));
	CHECK(IsTBMReplacementLocked(*rig.hauler->Last()));
	CHECK(Command<Commands::AutoreplaceVehicle>::Do(DoCommandFlag::Execute, rig.hauler->index).Failed());

	REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
	CHECK_FALSE(IsTBMReplacementLocked(*rig.hauler)); // No assignment any more,
	CHECK(IsTBMReplacementLocked(*rig.hauler->Last())); // but the Available set keeps its wagon identities.
	const Train *other = rig.BuildHauler(rig.depot, 1, 0);
	CHECK_FALSE(IsTBMReplacementLocked(*other->Last()));
}

/** However a train is deleted, its parts leave the store at once, and new vehicles with reused IDs join nothing. */
TEST_CASE("Deleted trains leave the exchange store", "[tbm-exchange-lifecycle]")
{
	TBMExchangeLoadRig rig;
	const TBMWagonSetID loaded = GetTBMExchangeAssignmentForHauler(rig.hauler->index)->set;
	const TBMExchangeSave before;
	const TBMWagonSetID assigned = std::ranges::find(before.sets, TBMWagonSetState::Assigned, &TBMWagonSet::state)->id;

	SECTION("a hauler deleted with its wagons, as after a crash") {
		delete rig.hauler;
		rig.hauler = nullptr;
		CHECK(CaptureTBMExchangeAssignments().empty());
		CHECK(GetTBMWagonSet(loaded) == nullptr);
		CHECK(GetTBMWagonSet(assigned)->state == TBMWagonSetState::Assigned); // The cutter's set is not touched.
		CHECK(TBMExchangeSave{}.Reloads());
		const Train *fresh = rig.BuildHauler(rig.depot, 2, 2);
		// Freed IDs may be reused at once; a new part never inherits a set or an assignment.
		for (const Train *part = fresh; part != nullptr; part = part->Next()) {
			CHECK(GetTBMWagonSetForWagon(part->index) == nullptr);
			CHECK(GetTBMExchangeAssignmentForHauler(part->index) == nullptr);
		}
	}
	SECTION("a cutter whose wagons outlive it") {
		OnTBMExchangeTrainPartDeleted(*rig.front); // The hook alone, as if only the front engine went.
		CHECK(GetTBMWagonSet(assigned)->state == TBMWagonSetState::Available); // Its empties carry nothing.
		CHECK(GetTBMWagonSet(loaded)->state == TBMWagonSetState::Loaded);
		CHECK(TBMExchangeSave{}.Reloads());
	}
	SECTION("a whole cutter consist") {
		delete rig.front;
		rig.front = nullptr;
		CHECK(GetTBMWagonSet(assigned) == nullptr);
		CHECK(GetTBMWagonSet(loaded) != nullptr); // The hauler and its spoil stay as they are.
		CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->set == loaded);
		CHECK(TBMExchangeSave{}.Reloads());
	}
}

/** A depot bay can go only when its project can fall back on its Source bay; an occupied bay never goes. */
TEST_CASE("Removing a depot bay keeps a supported exchange", "[tbm-exchange-lifecycle]")
{
	TBMExchangeRig rig;
	rig.MakeOriginStationTile();
	const TileIndex depot = rig.BuildDepotBay(3);
	REQUIRE(Command<Commands::SetTBMExchangeBay>::Do(DoCommandFlag::Execute, rig.project.id, depot).Succeeded());
	rig.Assign();
	REQUIRE(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->bay == depot);
	const TileIndex portal = rig.fixture.request.portal;
	const Owner company = GetTileOwner(portal);

	SECTION("with a Source bay, the project and its hauler fall back to it") {
		const uint64_t revision = GetTBMExchangeAssignmentForHauler(rig.hauler->index)->revision;
		REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, depot).Succeeded());
		CHECK_FALSE(IsRailDepotTile(depot));
		CHECK(rig.Project().exchange_bay == portal);
		CHECK(GetTBMExchangeBay(rig.Project()).kind == TBMExchangeBayKind::Source);
		CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->bay == portal);
		CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->revision > revision);
		CHECK(TBMExchangeSave{}.Reloads());
	}
	SECTION("without a Source bay the depot stays") {
		SetTileOwner(portal, OWNER_TOWN); // The portal no longer resolves as this project's bay.
		CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, depot).GetErrorMessage() == STR_ERROR_TBM_EXCHANGE_BAY_IN_USE);
		CHECK(IsRailDepotTile(depot));
		CHECK(rig.Project().exchange_bay == depot);
		CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->bay == depot);
		SetTileOwner(portal, company);
	}
	SECTION("an occupied bay refuses before anything changes") {
		rig.PlaceInDepot(rig.hauler, depot);
		const auto chain = GetTBMChainIDs(rig.hauler);
		const auto result = Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, depot);
		CHECK(result.Failed());
		CHECK(result.GetErrorMessage() != STR_ERROR_TBM_EXCHANGE_BAY_IN_USE); // The ordinary vehicle check refuses first.
		CHECK(IsRailDepotTile(depot));
		CHECK(rig.Project().exchange_bay == depot);
		CHECK(GetTBMChainIDs(rig.hauler) == chain);
	}
	SECTION("company removal clears the depot regardless") {
		SetTileOwner(portal, OWNER_TOWN);
		REQUIRE(Command<Commands::LandscapeClear>::Do({DoCommandFlag::Execute, DoCommandFlag::Bankrupt}, depot).Succeeded());
		CHECK(rig.Project().exchange_bay != depot);
		SetTileOwner(portal, company);
	}
}

/** A merger moves projects and jobs with the trains; a removal ends the projects and leaves only orphan jobs. */
TEST_CASE("Company changes keep projects, jobs and the exchange store loadable", "[tbm-exchange-lifecycle]")
{
	TBMExchangeLoadRig rig;
	const CompanyID old_owner = _current_company;
	const ExcavationJobID job = rig.Project().job;
	REQUIRE(GetExcavationJob(job) != nullptr);
	const ExcavationJobState job_state = GetExcavationJob(job)->state;

	SECTION("merger") {
		REQUIRE(Company::CanAllocateItem());
		std::unique_ptr<Company> buyer{Company::Create()};
		// The company change moves vehicles and stations itself; only that part of it is reproduced here.
		for (Vehicle *v : Vehicle::Iterate()) {
			if (v->owner == old_owner) v->owner = buyer->index;
		}
		for (Station *st : Station::Iterate()) {
			if (st->owner == old_owner) st->owner = buyer->index;
		}
		const TBMExchangeSave before;
		ChangeTBMOwnership(old_owner, buyer->index);
		CHECK(rig.Project().company == buyer->index);
		CHECK(GetExcavationJob(job)->company == buyer->index);
		CHECK(GetExcavationJob(job)->state == job_state);
		const TBMExchangeSave after;
		CHECK(after.assignments == before.assignments); // Assignments and sets take their company from the project.
		CHECK(after.sets == before.sets);
		CHECK(after.Reloads());
		CHECK(TBMExchangeRig::Aboard(rig.hauler) == rig.spoil);
	}
	SECTION("removal") {
		const TileIndex portal = rig.fixture.request.portal;
		REQUIRE(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Commissioning);
		// Company removal deletes the trains first; the deletion hook already empties their sets and assignments.
		delete rig.hauler;
		rig.hauler = nullptr;
		delete rig.front;
		rig.front = nullptr;
		const TBMProjectID project = rig.project.id;
		const TBMProjectID next = GetNextTBMProjectID();
		ChangeTBMOwnership(old_owner, INVALID_OWNER);
		CHECK(GetTBMProject(project) == nullptr);
		CHECK(GetNextTBMProjectID() == next); // Identities are never reused.
		CHECK(CaptureTBMExchangeAssignments().empty());
		CHECK(CaptureTBMWagonSets().empty());
		const ExcavationJob *orphan = GetExcavationJob(job);
		REQUIRE(orphan != nullptr);
		CHECK(orphan->company == CompanyID::Invalid());
		// No controller can resume an erased project's work: an open job is cancelled, closed history stays as it was.
		CHECK(orphan->state == (job_state == ExcavationJobState::Completed ? ExcavationJobState::Completed : ExcavationJobState::Cancelled));
		CHECK(ValidateExcavationJobReferences(*orphan));
		// The pair is released, so the removed company's tiles can then be cleared (r2mlvs3 Group 11.4).
		CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Sealed);
		CHECK(CheckLayerPortalLifecycle(portal) == LayerPortalWorkError::None);
	}
}
