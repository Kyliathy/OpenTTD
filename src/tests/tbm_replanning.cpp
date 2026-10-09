/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_replanning.cpp Existing-job continuation geometry and preservation boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group3
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../safeguards.h"

/** Actual published cutter/job plus an explicitly accepted three-advance extension beyond its current route. */
class ReplanningRig : public TBMWorkRig {
public:
	ExcavationOutline outline; ///< New requested floor and accepted old/new approach space, without any reservation changes.
	ExcavationPlanningPose end; ///< Canonical full accepted route's end; the physical cutter has not been moved there.
	uint8_t points = 0; ///< Actual native consist's required planning history.

	/** Publish real owners, then derive future geometry through the same advancing-sweep producer. */
	ReplanningRig() : TBMWorkRig(5)
	{
		const auto job = this->Job();
		const auto dimensions = MeasureTBMConsist(*this->front, this->Project());
		REQUIRE(dimensions.error == TBMAcquisitionError::None);
		this->points = dimensions.planning_points;
		this->end = {job.commissioning_anchor, job.route.front().heading, std::vector<TileIndex>(this->points, job.commissioning_anchor)};
		// Replaying geometry derives the future tail; no fixture writes a vehicle
		// position or pretends that the uncut accepted prefix has been completed.
		for (const auto &cut : job.route) {
			const auto step = AdvanceExcavationPlanningPose(this->end, cut.heading);
			REQUIRE(step.error == ExcavationError::None);
			REQUIRE(step.cut == cut);
			this->end = step.next;
		}
		auto future = this->end;
		auto authorized = job.authorized_tiles;
		// The player explicitly accepts each future straight sweep and the old
		// approach; omitted working space must still be rejected by the planner.
		for (uint i = 0; i < 3; ++i) {
			const auto step = AdvanceExcavationPlanningPose(future, future.heading);
			REQUIRE(step.error == ExcavationError::None);
			authorized.insert(authorized.end(), step.cut.swept_tiles.begin(), step.cut.swept_tiles.end());
			future = step.next;
		}
		this->outline = NormalizeExcavationOutline(GetTileLayer(job.commissioning_anchor), std::array{future.position});
		const auto work = NormalizeExcavationOutline(this->outline.layer, authorized);
		REQUIRE(this->outline.error == ExcavationError::None);
		REQUIRE(work.error == ExcavationError::None);
		this->outline.authorized_tiles = work.requested_tiles;
	}
};

/** Same-job forecasts may use their accepted future floor, while a fresh planner still refuses those reserved cells. */
TEST_CASE("TBM extension preview preserves existing route work and actual vehicles", "[tbm-replanning-preview]")
{
	ReplanningRig rig;
	const auto job = rig.Job();
	const auto project = rig.Project();
	const auto before = rig.Digest();
	const auto fresh = PlanExcavationRoute(rig.outline, rig.end, project.model_id);
	CHECK(fresh.error == ExcavationError::ActiveCut);
	const auto extension = PlanExcavationJobExtension(job.id, rig.outline, rig.points);
	REQUIRE(extension.error == ExcavationError::None);
	CHECK_FALSE(extension.partial);
	REQUIRE_FALSE(extension.route.empty());
	CHECK(extension.route.front().cutter_position != job.route.front().cutter_position);
	CHECK(extension.remaining_targets.empty());
	std::set<TileIndex> previous, new_cells;
	// Quotes must not generate a second yield for either the completed part or
	// the still-unopened prefix that this same job already committed to cutting.
	for (const auto &cut : job.route) previous.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
	for (const auto &cut : extension.route) {
		for (TileIndex tile : cut.swept_tiles) {
			if (!previous.contains(tile)) new_cells.insert(tile);
		}
	}
	const std::vector<TileIndex> fresh_cells(new_cells.begin(), new_cells.end());
	const auto quote = QuoteExcavationCells(fresh_cells, project.model_id);
	REQUIRE(quote.error == ExcavationError::None);
	CHECK(extension.estimate.earth_t == quote.earth_t);
	CHECK(extension.estimate.rock_t == quote.rock_t);
	CHECK(extension.estimate.work_ticks == quote.work_ticks);
	const auto repeat = PlanExcavationJobExtension(job.id, rig.outline, rig.points);
	CHECK(repeat.route == extension.route);
	CHECK(repeat.expansions == extension.expansions);
	CHECK(rig.Job() == job);
	CHECK(rig.Project() == project);
	CHECK(rig.Digest() == before);
}

/** Eligibility and finite saved-route storage are checked before any same-job reservation exemption. */
TEST_CASE("TBM extension refuses foreign or malformed requests", "[tbm-replanning-preview]")
{
	ReplanningRig rig;
	const auto job = rig.Job();
	const auto before = rig.Digest();
	SECTION("foreign caller") {
		AutoRestoreBackup actor(_current_company, CompanyID{1});
		CHECK(PlanExcavationJobExtension(job.id, rig.outline, rig.points).error == ExcavationError::InvalidOwner);
	}
	SECTION("invalid history") {
		CHECK(PlanExcavationJobExtension(job.id, rig.outline, 2).error == ExcavationError::InvalidRoute);
	}
	SECTION("invalid operation budget") {
		ExcavationPlanningLimits limits;
		limits.segments = MAX_EXCAVATION_ROUTE + 1;
		CHECK(PlanExcavationJobExtension(job.id, rig.outline, rig.points, limits).error == ExcavationError::Limit);
	}
	SECTION("working space omitted") {
		auto narrow = rig.outline;
		narrow.authorized_tiles = narrow.requested_tiles;
		CHECK(PlanExcavationJobExtension(job.id, narrow, rig.points).error != ExcavationError::None);
	}
	CHECK(rig.Job() == job);
	CHECK(rig.Digest() == before);
}

/** Cancelled history cannot be silently reopened by a read-only planning request. */
TEST_CASE("TBM extension refuses a cancelled job without reacquiring its claims", "[tbm-replanning-preview]")
{
	ReplanningRig rig;
	REQUIRE(CancelExcavationJob(rig.Job().id) == ExcavationError::None);
	const auto cancelled = rig.Job();
	const auto before = rig.Digest();
	CHECK(PlanExcavationJobExtension(cancelled.id, rig.outline, rig.points).error == ExcavationError::AlreadyCompleted);
	CHECK(rig.Job() == cancelled);
	CHECK(rig.Digest() == before);
}

/** The geological append preserves partial work and old exchange points while atomically adding only future claims. */
TEST_CASE("TBM appended geology retains progress receipts and old return schedule", "[tbm-replanning-append]")
{
	ReplanningRig rig;
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None); // Standard earns one whole work tick in two simulation ticks.
	const auto job = rig.Job();
	REQUIRE(job.partial_work_ticks > 0);
	const auto project = rig.Project();
	const auto before = rig.Digest();
	const auto next_id = GetGeologyWorldState().next_job_id;
	const auto extension = PlanExcavationJobExtension(job.id, rig.outline, rig.points);
	REQUIRE(extension.error == ExcavationError::None);
	CHECK(AppendExcavationJob(job.id, job.revision, rig.outline, extension.route, false).error == ExcavationError::None);
	CHECK(rig.Digest() == before);
	CHECK(rig.Job() == job);
	REQUIRE(AppendExcavationJob(job.id, job.revision, rig.outline, extension.route, true).error == ExcavationError::None);
	const auto appended = rig.Job();
	CHECK(appended.id == job.id);
	CHECK(appended.cutter == job.cutter);
	CHECK(appended.origin == job.origin);
	CHECK(appended.commissioning_anchor == job.commissioning_anchor);
	CHECK(appended.cursor == job.cursor);
	CHECK(appended.partial_work_ticks == job.partial_work_ticks);
	CHECK(appended.completed_earth_t == job.completed_earth_t);
	CHECK(appended.completed_rock_t == job.completed_rock_t);
	CHECK(appended.completed_work_cost == job.completed_work_cost);
	CHECK(appended.revision == job.revision + 1);
	CHECK(appended.route.size() == job.route.size() + extension.route.size());
	CHECK(std::equal(job.route.begin(), job.route.end(), appended.route.begin()));
	REQUIRE(appended.pass_terms->exchange_after.size() > job.pass_terms->exchange_after.size());
	CHECK(std::equal(job.pass_terms->exchange_after.begin(), job.pass_terms->exchange_after.end(), appended.pass_terms->exchange_after.begin()));
	CHECK(appended.pass_terms->exchange_after.back() == appended.route.size());
	CHECK(ValidateExcavationPassTerms(appended, *appended.pass_terms).error == ExcavationError::None);
	CHECK(GetGeologyWorldState().next_job_id == next_id);
	CHECK(rig.Project() == project);
	CHECK(Company::Get(_current_company)->money == before.money);
	const auto committed = rig.Digest();
	CHECK(AppendExcavationJob(job.id, job.revision, rig.outline, extension.route, true).error == ExcavationError::StaleParticipant);
	CHECK(rig.Digest() == committed);
	// The existing saved geometry/claim validator must reconstruct precisely
	// these appended reservations; this is structural proof, not file-load proof.
	auto reconstructed = GetGeologyWorldState();
	const auto claims = reconstructed.job_claims;
	REQUIRE(RebuildExcavationJobClaims(reconstructed) == GeologyError::None);
	CHECK(reconstructed.job_claims == claims);
}

/** Refused canonical admission cannot leave half of a new footprint attached to a running job. */
TEST_CASE("TBM append refusals retain every existing owner", "[tbm-replanning-append]")
{
	ReplanningRig rig;
	const auto job = rig.Job();
	auto extension = PlanExcavationJobExtension(job.id, rig.outline, rig.points);
	REQUIRE(extension.error == ExcavationError::None);
	auto outline = rig.outline;
	SECTION("changed first edge") { extension.route.front().cutter_position = job.commissioning_anchor; }
	SECTION("omitted working space") { outline.authorized_tiles = outline.requested_tiles; }
	SECTION("stale quote") {
		// A controller-only half tick does not revise geological work. Earn a
		// whole work tick so this specifically tests the geological revision guard.
		REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
		REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	}
	const auto before = rig.Digest();
	const auto saved = rig.Job();
	CHECK(AppendExcavationJob(job.id, job.revision, outline, extension.route, false).error != ExcavationError::None);
	CHECK(AppendExcavationJob(job.id, job.revision, outline, extension.route, true).error != ExcavationError::None);
	CHECK(rig.Digest() == before);
	CHECK(rig.Job() == saved);
}
