/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_acquisition.cpp Whole-consist preparation, unpublished acquisition, rollback and the role contract,
 * with actual pool-owned parts. These are fixture constructions, not a normal-game first cut or disposal claim;
 * production commissioning stays guarded (CommissionLayerPortalTBM still reports ControllerUnavailable).
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 2B
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../group.h"
#include "../company_base.h"
#include "../engine_func.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"

#include "../safeguards.h"

/** Preparation quotes the actual consist and the pocket's first cut at its real capacities, changing nothing. */
TEST_CASE("Whole-consist preparation binds the actual consist without changing the world", "[tbm-acquisition]")
{
	TBMAcquisitionRig rig;
	const auto &request = rig.fixture.request;
	const auto before = rig.Digest();
	const auto plan = PrepareTBMAcquisition(request, {2, 2});
	REQUIRE(plan.error == TBMAcquisitionError::None);
	CHECK(plan.proof == rig.proof);
	CHECK(plan.cutter_engine == GetTBMEngine(0));
	CHECK(plan.earth_engine == GetTBMEngine(2));
	CHECK(plan.rock_engine == GetTBMEngine(3));
	CHECK(plan.vehicle_parts == 6); // Cutter, its permanent support and four hoppers.
	CHECK(plan.consist_length == 48); // Three tiles of eight-unit parts.
	CHECK(plan.planning_points == 4);
	CHECK(plan.earth_capacity_t == 128);
	CHECK(plan.rock_capacity_t == 256);
	CHECK(plan.unit_number == before.next_unit);
	CHECK(plan.machinery_cost == GetTBMCutterModel(0)->purchase_cost);
	CHECK(plan.wagon_cost == Engine::Get(GetTBMEngine(2))->GetCost() * 2 + Engine::Get(GetTBMEngine(3))->GetCost() * 2);
	REQUIRE(plan.job.pass_terms.has_value());
	CHECK(plan.job.pass_terms->earth_capacity_t == 128);
	CHECK(plan.job.pass_terms->rock_capacity_t == 256);
	CHECK(plan.job.pass_terms->exchange_after == std::vector<uint16_t>{1});
	CHECK(plan.prospective_job == ExcavationJobID{GetGeologyWorldState().next_job_id});
	// The first cut needs more Rock than one hopper holds, but fits the two requested hoppers of each kind.
	const auto quote = QuoteExcavationCells(plan.job.route[0].swept_tiles, 0);
	CHECK(quote.rock_t > 128);
	CHECK(quote.earth_t <= plan.earth_capacity_t);
	CHECK(quote.rock_t <= plan.rock_capacity_t);
	CHECK(PrepareTBMAcquisition(request, {2, 2}) == plan);
	CHECK(rig.Digest() == before);
	CHECK(GetLayerPortalWorkRecord(request.portal)->state == LayerPortalWorkState::Sealed);
	CHECK(CommissionLayerPortalTBM(request, true).error == LayerPortalWorkError::ControllerUnavailable);
}

/** Acquisition builds actual stopped parts on the source site, and every rollback path removes them completely. */
TEST_CASE("Acquisition builds actual pool-owned parts and rolls back completely", "[tbm-acquisition]")
{
	TBMAcquisitionRig rig;
	const auto &request = rig.fixture.request;
	const auto plan = PrepareTBMAcquisition(request, {2, 2});
	REQUIRE(plan.error == TBMAcquisitionError::None);
	const auto before = rig.Digest();
	std::optional<TBMProject> released;
	{
		TBMAcquisition held;
		REQUIRE(AcquireTBMConsist(request, plan, held) == TBMAcquisitionError::None);
		REQUIRE(held.IsHeld());
		Train *front = held.GetFront();
		REQUIRE(front != nullptr);
		CHECK(Vehicle::GetNumItems() == 6);
		CHECK(front->IsFrontEngine());
		CHECK(front->vehstatus.Test(VehState::Stopped));
		CHECK(front->vehstatus.Test(VehState::Hidden));
		CHECK(front->unitnumber == plan.unit_number);
		CHECK(front->value == plan.machinery_cost);
		CHECK(front->gcache.cached_total_length == plan.consist_length);
		// Every part stays stopped on the source site's assembly track; nothing has entered the shaft.
		for (const Train *part = front; part != nullptr; part = part->Next()) {
			CHECK(part->tile == request.portal);
			CHECK(part->track == Track::Depot);
		}
		const auto &project = held.GetProject();
		CHECK(project.id == TBMProjectID{UINT32_MAX});
		CHECK(project.job == ExcavationJobID{UINT32_MAX});
		CHECK(project.company == _current_company);
		CHECK(project.cutter == front->index);
		CHECK(project.model_id == 0);
		CHECK(project.permanent_parts == std::vector<VehicleID>{front->index, front->Next()->index});
		CHECK(project.heading == rig.proof.exit_heading);
		CHECK(project.origin == request.proposal.origin);
		CHECK(project.portal == rig.proof.pair_key);
		CHECK(project.exchange_bay == rig.proof.source);
		const auto layout = InspectTBMConsist(*front, project);
		REQUIRE(layout.error == TBMBlockedReason::None);
		REQUIRE(layout.exchangeable_units.size() == 4);
		const std::array expected{TBMConsistRole::EarthWagon, TBMConsistRole::EarthWagon, TBMConsistRole::RockWagon, TBMConsistRole::RockWagon};
		// Units keep the requested Earth-then-Rock order, and the role contract agrees with the layout view.
		for (size_t i = 0; i < expected.size(); ++i) {
			CHECK(layout.exchangeable_units[i].role == expected[i]);
			CHECK(GetTBMConsistRole(project, *Train::Get(layout.exchangeable_units[i].vehicle)) == expected[i]);
		}
		CHECK(GetTBMConsistRole(project, *front) == TBMConsistRole::Cutter);
		CHECK(GetTBMConsistRole(project, *front->Next()) == TBMConsistRole::PermanentSupport);
		auto foreign = project;
		foreign.cutter = VehicleID::Invalid();
		CHECK(GetTBMConsistRole(foreign, *front) == TBMConsistRole::Invalid);
		// Nothing was charged, reserved or published: geology, pair state and cash are unchanged.
		const auto now = rig.Digest();
		CHECK(now.geology == before.geology);
		CHECK(now.portal == before.portal);
		CHECK(now.money == before.money);
		CHECK(now.trains == before.trains + 1);
		CHECK(now.engines == std::array<uint16_t, 3>{uint16_t(before.engines[0] + 1), uint16_t(before.engines[1] + 2), uint16_t(before.engines[2] + 2)});
		CHECK(GetLayerPortalWorkRecord(request.portal)->cutter == VehicleID::Invalid());
		// The occupied site refuses a second consist, and a holding owner cannot take one either.
		TBMAcquisition other;
		CHECK(AcquireTBMConsist(request, plan, other) == TBMAcquisitionError::Portal);
		CHECK_FALSE(other.IsHeld());
		CHECK(AcquireTBMConsist(request, plan, held) == TBMAcquisitionError::AllocationFailure);
		CHECK(Vehicle::GetNumItems() == 6);

		SECTION("explicit rollback") {
			held.Rollback();
			CHECK_FALSE(held.IsHeld());
			CHECK(held.GetFront() == nullptr);
			CHECK(rig.Digest() == before);
		}
		SECTION("rollback on destruction") {
			// Leaving the scope destroys the owner, which must remove everything it holds.
		}
		SECTION("release hands the parts over") {
			released = held.Release();
			CHECK_FALSE(held.IsHeld());
			CHECK(released->cutter == front->index); // `project` refers to the holder's copy, which Release() empties.
		}
	}
	if (released.has_value()) {
		CHECK(Vehicle::GetNumItems() == 6); // Released parts now belong to the publishing owner (the fixture pool here).
	} else {
		CHECK(rig.Digest() == before);
	}
}

/** Every preflight boundary refuses before allocating anything, and a changed world makes an accepted plan stale. */
TEST_CASE("Acquisition refuses each preflight boundary and stale plans without allocating", "[tbm-acquisition]")
{
	TBMAcquisitionRig rig;
	auto &request = rig.fixture.request;
	const auto before = rig.Digest();
	/**
	 * Prepare and attempt acquisition, which must refuse identically and allocate nothing.
	 * @param consist Requested wagons.
	 * @return The preparation refusal.
	 */
	const auto refuse = [&](TBMConsistRequest consist) {
		const auto plan = PrepareTBMAcquisition(request, consist);
		TBMAcquisition held;
		CHECK(AcquireTBMConsist(request, plan, held) == plan.error);
		CHECK_FALSE(held.IsHeld());
		CHECK(Vehicle::GetNumItems() == 0);
		return plan;
	};

	SECTION("both spoil roles need a wagon") {
		CHECK(refuse({0, 2}).error == TBMAcquisitionError::InvalidConsist);
		CHECK(refuse({2, 0}).error == TBMAcquisitionError::InvalidConsist);
	}
	SECTION("one Rock hopper cannot hold the pocket's rock") {
		const auto plan = refuse({2, 1});
		CHECK(plan.error == TBMAcquisitionError::Geology);
		CHECK(plan.geology_error == ExcavationError::InsufficientCapacity);
		CHECK(plan.failure_tile == request.proposal.route[0].cutter_position);
	}
	SECTION("the train-length setting bounds the consist") {
		AutoRestoreBackup shorter(_settings_game.vehicle.max_train_length, uint8_t{2});
		CHECK(refuse({2, 2}).error == TBMAcquisitionError::InvalidConsist);
	}
	SECTION("a route planned for a shorter consist does not cover this one") {
		request.proposal = rig.Proposal(4, 3);
		const auto plan = refuse({2, 2});
		CHECK(plan.error == TBMAcquisitionError::InvalidConsist);
		CHECK(plan.failure_tile == request.proposal.route[3].cutter_position);
		request.proposal = rig.Proposal(4, 4);
		CHECK(PrepareTBMAcquisition(request, {2, 2}).error != TBMAcquisitionError::InvalidConsist);
	}
	SECTION("an unavailable hopper refuses the whole consist") {
		Engine::Get(GetTBMEngine(3))->company_avail.Reset(_current_company);
		CHECK(refuse({2, 2}).error == TBMAcquisitionError::UnavailableModel);
	}
	SECTION("the company train limit leaves no unit number") {
		AutoRestoreBackup none(_settings_game.vehicle.max_trains, UnitID{0});
		CHECK(refuse({2, 2}).error == TBMAcquisitionError::UnitLimit);
	}
	SECTION("funds must cover machinery and every wagon") {
		const auto plan = PrepareTBMAcquisition(request, {2, 2});
		REQUIRE(plan.error == TBMAcquisitionError::None);
		Company::Get(_current_company)->money = plan.machinery_cost + plan.wagon_cost - 1;
		CHECK(refuse({2, 2}).error == TBMAcquisitionError::InsufficientFunds);
		Company::Get(_current_company)->money = before.money;
	}
	SECTION("an occupied source site is a portal conflict") {
		// The blocker gets full purchase bookkeeping from the site builder, so deleting it restores every count.
		Train *blocker = nullptr;
		REQUIRE(BuildTBMConsistAtSite(request, Engine::Get(GetTBMEngine(0)), 0, before.next_unit, std::span<const Engine *const>{}, &blocker).Succeeded());
		REQUIRE(blocker != nullptr);
		const auto plan = PrepareTBMAcquisition(request, {2, 2});
		CHECK(plan.error == TBMAcquisitionError::Portal);
		CHECK(plan.portal_error == LayerPortalWorkError::Conflict);
		delete blocker;
		_random.state[0] = before.random0; // The blocker's construction drew random bits; only the world under test is compared.
		_random.state[1] = before.random1;
	}
	SECTION("a changed unit number makes the accepted plan stale") {
		const auto plan = PrepareTBMAcquisition(request, {2, 2});
		REQUIRE(plan.error == TBMAcquisitionError::None);
		auto &units = Company::Get(_current_company)->freeunits[VehicleType::Train];
		units.UseID(plan.unit_number);
		TBMAcquisition held;
		CHECK(AcquireTBMConsist(request, plan, held) == TBMAcquisitionError::Stale);
		CHECK_FALSE(held.IsHeld());
		CHECK(Vehicle::GetNumItems() == 0);
		units.ReleaseID(plan.unit_number);
	}
	CHECK(rig.Digest() == before);
}

/** The site builder deletes every part it made when a later coupling fails, so no partial consist survives. */
TEST_CASE("A coupling failure inside site assembly deletes every built part", "[tbm-acquisition]")
{
	TBMAcquisitionRig rig;
	const auto &request = rig.fixture.request;
	const auto before = rig.Digest();
	AutoRestoreBackup shorter(_settings_game.vehicle.max_train_length, uint8_t{2}); // Room for the cutter and two hoppers only.
	const Engine *earth = Engine::Get(GetTBMEngine(2));
	const std::array<const Engine *, 3> wagons{earth, earth, earth};
	Train *front = nullptr;
	CHECK(BuildTBMConsistAtSite(request, Engine::Get(GetTBMEngine(0)), GetTBMCutterModel(0)->purchase_cost, before.next_unit, wagons, &front).Failed());
	CHECK(front == nullptr);
	CHECK(Vehicle::GetNumItems() == 0);
	const auto after = rig.Digest();
	// The builder undoes pool items, unit number and group counts; restoring random seeds is the acquisition owner's job.
	CHECK(after.geology == before.geology);
	CHECK(after.portal == before.portal);
	CHECK(after.money == before.money);
	CHECK(after.next_unit == before.next_unit);
	CHECK(after.trains == before.trains);
	CHECK(after.engines == before.engines);
	// A unit number other than the next free one refuses before anything is built.
	CHECK(BuildTBMConsistAtSite(request, Engine::Get(GetTBMEngine(0)), 0, static_cast<UnitID>(before.next_unit + 1), std::span<const Engine *const>{}, &front).Failed());
	CHECK(Vehicle::GetNumItems() == 0);
}
