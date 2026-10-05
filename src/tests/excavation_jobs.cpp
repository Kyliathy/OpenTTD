/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_jobs.cpp Competing reservations and recoverable work/pause/cancel ownership. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 10 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"

#include "../safeguards.h"

/**
 * Build a canonical test pass through the public geometry contract, without inventing a vehicle or station binding.
 * @param anchor Existing supported open floor.
 * @return Two advancing 3x3 cutter envelopes with explicit authorized turn/approach space.
 */
static ExcavationJob JobFixturePass(TileIndex anchor)
{
	ExcavationJob job;
	job.company = _current_company;
	job.model_id = GetTileLayer(anchor) == MapLayer::DeepUnderground ? 1 : 0;
	job.commissioning_anchor = anchor;
	job.requested_tiles = {TileAddWrap(anchor, 2, 0)};
	// Explicit accepted working space contains both overlapping conservative envelopes.
	for (int y = -1; y <= 1; ++y) {
		for (int x = 0; x <= 3; ++x) job.authorized_tiles.push_back(TileAddWrap(anchor, x, y));
	}
	// Both segments advance along the native SW (+x) heading; supports belong to the real pre-existing fixture floor.
	for (int step = 1; step <= 2; ++step) {
		ExcavationCutGeometry cut;
		cut.cutter_position = TileAddWrap(anchor, step, 0);
		cut.heading = Direction::SW;
		cut.support_tiles = {anchor};
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) cut.swept_tiles.push_back(TileAddWrap(cut.cutter_position, x, y));
		}
		job.route.push_back(std::move(cut));
	}
	return job;
}

/**
 * Open the actual complete starter footprint required by validated loaded backing.
 * @param fixture Actual initialized geological owner.
 * @param anchor Explicit starting cutter centre.
 */
static void OpenJobFixtureStarter(ExcavationFixture &fixture, TileIndex anchor)
{
	std::vector<TileIndex> floor;
	// Every occupied initial cutter cell must be real open terrain before the reservation gains a return proof.
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) floor.push_back(TileAddWrap(anchor, x, y));
	}
	fixture.Open(floor);
}

/** Controlled support participant for geometry tests; actual vehicle/rail integration remains Plan 4's producer. */
class JobFixtureSupport final : public ExcavationWorkParticipant {
public:
	bool available = true; ///< Simulate loss/restoration of the external physical support owner.
	/**
	 * Revalidate actual fixture terrain and the controlled support owner without stock or RNG mutation.
	 * @param job Accepted geometry job.
	 * @param cut Exact next segment.
	 * @return True only while the fixture support signal and every required floor cell remain available.
	 */
	bool IsSupported(const ExcavationJob &job, const ExcavationCutGeometry &cut) const override
	{
		if (!this->available) return false;
		if (!Company::IsValidID(job.company)) return false;
		// Physical support cannot be inferred from a requested room or another layer's matching coordinates.
		for (TileIndex tile : cut.support_tiles) {
			if (GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated) return false;
		}
		return true;
	}
};

/** Reservation estimates and conflicting/disconnected routes leave both existing claims and identity cursors untouched. */
TEST_CASE("Excavation jobs reserve one owner per complete advancing footprint", "[excavation-job-reservation]")
{
	ExcavationFixture fixture;
	const auto anchor = TileXYL(20, 20, MapLayer::Underground);
	OpenJobFixtureStarter(fixture, anchor);
	auto proposal = JobFixturePass(anchor);
	const auto before = CaptureGeologyState();
	const auto estimate = ReserveExcavationJob(proposal, false);
	REQUIRE(estimate.error == ExcavationError::None);
	CHECK(GetGeologyWorldState().jobs.empty());
	CHECK(GetGeologyWorldState().next_job_id == 0);
	CHECK(CaptureGeologyState().packed == before.packed);
	const auto reserved = ReserveExcavationJob(proposal, true);
	REQUIRE(reserved.error == ExcavationError::None);
	CHECK(reserved.id == estimate.id);
	CHECK(GetExcavationJob(reserved.id)->state == ExcavationJobState::Reserved);
	CHECK(GetGeologyWorldState().terrain.Get(anchor) == ExcavationState::Excavated);
	CHECK(GetGeologyWorldState().terrain.Get(TileAddWrap(anchor, 2, 0)) == ExcavationState::Excavating);
	CHECK(CheckExcavatedFootprint(std::array{anchor}).error == ExcavationError::ActiveCut);
	const auto held = CaptureGeologyState().packed;
	CHECK(ReserveExcavationJob(proposal, true).error == ExcavationError::Conflict);
	CHECK(CaptureGeologyState().packed == held);
	CHECK(GetGeologyWorldState().next_job_id == 1);
	CHECK(GetGeologyWorldState().jobs.size() == 1);
	CHECK(GetGeologyWorldState().deposits.Records().front().remaining_reserve == before.deposits.front().remaining_reserve);
	auto disconnected = proposal;
	disconnected.route[1].cutter_position = TileAddWrap(anchor, 5, 0);
	CHECK(ValidateExcavationJobGeometry(GetGeologyWorldState(), disconnected).error == ExcavationError::InvalidRoute);
	auto pivot = proposal;
	pivot.route[1].heading = Direction::NW;
	pivot.route[1].cutter_position = TileAddWrap(proposal.route[0].cutter_position, 0, -1);
	CHECK(ValidateExcavationJobGeometry(GetGeologyWorldState(), pivot).error == ExcavationError::IllegalTurn);
	auto outside = proposal;
	outside.authorized_tiles.pop_back();
	CHECK(ValidateExcavationJobGeometry(GetGeologyWorldState(), outside).error == ExcavationError::OutsideAuthorization);
	auto restored = GetGeologyWorldState();
	restored.job_claims.clear();
	CHECK(RebuildExcavationJobClaims(restored) == GeologyError::None);
	CHECK(restored.job_claims == GetGeologyWorldState().job_claims);
}

/** Support loss preserves partial work and reservations; cancellation releases only unfinished solid ground. */
TEST_CASE("Excavation work pauses and cancels without refilling completed floor", "[excavation-job-control]")
{
	ExcavationFixture fixture;
	const auto anchor = TileXYL(20, 20, MapLayer::Underground);
	OpenJobFixtureStarter(fixture, anchor);
	const auto reserved = ReserveExcavationJob(JobFixturePass(anchor), true);
	REQUIRE(reserved.error == ExcavationError::None);
	JobFixtureSupport support;
	const auto money = Company::Get(_current_company)->money;
	CHECK(AdvanceExcavationWork(reserved.id, 77, support) == ExcavationError::None);
	CHECK(GetExcavationJob(reserved.id)->partial_work_ticks == 77);
	CHECK(GetExcavationJob(reserved.id)->state == ExcavationJobState::Working);
	support.available = false;
	CHECK(AdvanceExcavationWork(reserved.id, 3, support) == ExcavationError::MissingSupport);
	CHECK(GetExcavationJob(reserved.id)->state == ExcavationJobState::Paused);
	CHECK(GetExcavationJob(reserved.id)->partial_work_ticks == 77);
	const auto revision = GetExcavationJob(reserved.id)->revision;
	CHECK(AdvanceExcavationWork(reserved.id, 0, support) == ExcavationError::None);
	CHECK(GetExcavationJob(reserved.id)->revision == revision);
	support.available = true;
	CHECK(AdvanceExcavationWork(reserved.id, 3, support) == ExcavationError::None);
	CHECK(GetExcavationJob(reserved.id)->partial_work_ticks == 80);
	CHECK(AdvanceExcavationWork(reserved.id, MAX_EXCAVATION_WORK_TICKS, support) == ExcavationError::Limit);
	CHECK(GetExcavationJob(reserved.id)->partial_work_ticks == 80);
	CHECK(PauseExcavationJob(reserved.id, ExcavationError::MissingSupport) == ExcavationError::None);
	CHECK(CancelExcavationJob(reserved.id) == ExcavationError::None);
	CHECK(GetExcavationJob(reserved.id)->state == ExcavationJobState::Cancelled);
	CHECK(GetGeologyWorldState().job_claims.empty());
	CHECK(GetGeologyWorldState().terrain.Get(anchor) == ExcavationState::Excavated);
	CHECK(GetGeologyWorldState().terrain.Get(TileAddWrap(anchor, 2, 0)) == ExcavationState::Solid);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(GetExcavationJob(reserved.id)->completed_earth_t == 0);
	CHECK(CancelExcavationJob(reserved.id) == ExcavationError::None);
	const auto again = ReserveExcavationJob(JobFixturePass(anchor), true);
	REQUIRE(again.error == ExcavationError::None);
	CHECK(again.id.base() == reserved.id.base() + 1);
	CHECK(GetExcavationJob(reserved.id) != nullptr);
	CHECK(RetireExcavationJob(again.id) == ExcavationError::Conflict);
	const auto claims = GetGeologyWorldState().job_claims;
	CHECK(RetireExcavationJob(reserved.id) == ExcavationError::None);
	CHECK(GetExcavationJob(reserved.id) == nullptr);
	CHECK(GetGeologyWorldState().next_job_id == 2);
	CHECK(GetGeologyWorldState().job_claims == claims);
}

/** Removed ownership pauses explicitly and cannot borrow a newly created company through a stale numeric reference. */
TEST_CASE("Removed excavation ownership remains paused until explicit lifecycle cleanup", "[excavation-job-owner]")
{
	ExcavationFixture fixture;
	const auto anchor = TileXYL(20, 20, MapLayer::Underground);
	OpenJobFixtureStarter(fixture, anchor);
	const auto reserved = ReserveExcavationJob(JobFixturePass(anchor), true);
	REQUIRE(reserved.error == ExcavationError::None);
	JobFixtureSupport support;
	CHECK(AdvanceExcavationWork(reserved.id, 13, support) == ExcavationError::None);
	const auto before = CaptureGeologyState().packed;
	fixture.RemoveCompany();
	CHECK(AdvanceExcavationWork(reserved.id, 7, support) == ExcavationError::InvalidOwner);
	CHECK(GetExcavationJob(reserved.id)->company == CompanyID::Invalid());
	CHECK(GetExcavationJob(reserved.id)->state == ExcavationJobState::Paused);
	CHECK(GetExcavationJob(reserved.id)->pause_reason == ExcavationError::InvalidOwner);
	CHECK(GetExcavationJob(reserved.id)->partial_work_ticks == 13);
	CHECK(ValidateExcavationJobReferences(*GetExcavationJob(reserved.id)));
	CHECK(CaptureGeologyState().packed == before);
	CHECK(CancelExcavationJob(reserved.id) == ExcavationError::None);
	CHECK(GetGeologyWorldState().job_claims.empty());
	CHECK(ValidateExcavationJobReferences(*GetExcavationJob(reserved.id)));
}

/** Active progress snapshots restore actual route/claims and refuse corrupt/orphan/competing references atomically. */
TEST_CASE("Excavation snapshots rebuild active progress without accepting orphan packed cells", "[excavation-job-state]")
{
	ExcavationFixture fixture;
	const auto anchor = TileXYL(20, 20, MapLayer::Underground);
	OpenJobFixtureStarter(fixture, anchor);
	const auto reserved = ReserveExcavationJob(JobFixturePass(anchor), true);
	REQUIRE(reserved.error == ExcavationError::None);
	JobFixtureSupport support;
	REQUIRE(AdvanceExcavationWork(reserved.id, 77, support) == ExcavationError::None);
	const auto snapshot = CaptureGeologyState();
	GeologyWorldState restored;
	REQUIRE(PrepareGeologyRestoration(snapshot, restored) == GeologyError::None);
	CHECK(restored.jobs.size() == 1);
	CHECK(restored.jobs[0].partial_work_ticks == 77);
	CHECK(restored.jobs[0].cursor == 0);
	CHECK(restored.jobs[0].revision == GetExcavationJob(reserved.id)->revision);
	CHECK(restored.jobs[0].route[1].cutter_position == TileAddWrap(anchor, 2, 0));
	CHECK(restored.job_claims == GetGeologyWorldState().job_claims);
	CHECK(restored.next_job_id == 1);
	auto bad = snapshot;
	bad.jobs.clear();
	CHECK(PrepareGeologyRestoration(bad, restored) == GeologyError::ClaimConflict);
	CHECK(restored.jobs[0].partial_work_ticks == 77);
	bad = snapshot;
	bad.jobs.push_back(bad.jobs.front());
	bad.jobs.back().id = ExcavationJobID{1};
	bad.next_job_id = 2;
	CHECK(PrepareGeologyRestoration(bad, restored) == GeologyError::ClaimConflict);
	bad = snapshot;
	bad.jobs[0].cutter = VehicleID{123456};
	CHECK(PrepareGeologyRestoration(bad, restored) == GeologyError::ClaimConflict);
	bad = snapshot;
	bad.jobs[0].route[1].swept_tiles.pop_back();
	CHECK(PrepareGeologyRestoration(bad, restored) != GeologyError::None);
	CHECK(restored.job_claims == GetGeologyWorldState().job_claims);
	REQUIRE(CancelExcavationJob(reserved.id) == ExcavationError::None);
	REQUIRE(RetireExcavationJob(reserved.id) == ExcavationError::None);
	const auto retired = CaptureGeologyState();
	CHECK(PrepareGeologyRestoration(retired, restored) == GeologyError::None);
	CHECK(restored.jobs.empty());
	CHECK(restored.next_job_id == 1);
}
