/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_change_work.cpp Replacing an unfinished job's remainder: the completed cuts stay, the planned rest is swapped
 * for new cuts from the last completed position, atomically and without undoing anything physical.
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 11 (task 11.6)
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 12 (task 12.6: command and windows)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_view_fixture.h"
#include "../underground_gui.h"
#include "../tbm_gui.h"
#include "../tbm_cmd.h"
#include "../tbm_replanning.h"
#include "../tilehighlight_func.h"
#include "../viewport_func.h"
#include "../widgets/underground_widget.h"
#include "../widgets/tbm_widget.h"
#include "../widgets/misc_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/**
 * A working project on six straight cuts out of the door, whose origin has both spoil receivers as a real quote needs,
 * with helpers to bore and to draw a new remainder.
 */
class ChangeWorkRig : public SpoilReceiverRig {
public:
	uint8_t points = 0; ///< Measured planning history of the real consist.

	/** Publish the project on six straight cuts, give its origin both receivers and measure its consist. */
	ChangeWorkRig() : SpoilReceiverRig(6)
	{
		this->BuildReceiver(SpoilReceiverKind::Earth, 15, 22);
		this->BuildReceiver(SpoilReceiverKind::Rock, 14, 16);
		this->RefreshOrigin();
		const auto dimensions = MeasureTBMConsist(*this->front, this->Project());
		REQUIRE(dimensions.error == TBMAcquisitionError::None);
		this->points = dimensions.planning_points;
	}

	/**
	 * Complete cuts. After each one the rig empties the wagons and returns to Bore, standing in for the exchange and the
	 * drive onto the new support that this rig cannot perform.
	 * @param cuts Number of cuts to complete.
	 */
	void Bore(uint cuts) const
	{
		for (uint i = 0; i < cuts; ++i) {
			this->BoreOneCut();
			this->ExchangeAndReturn();
		}
	}

	/**
	 * A new floor beside the planned one: from the last completed cut, turn once and drive straight on.
	 * @param heading Heading of the first new advance, 45 degrees from the current one.
	 * @param cuts Number of advances.
	 * @return Outline whose floor is the last advance's cell and whose working space is every advance's sweep.
	 */
	ExcavationOutline Turn(Direction heading, uint cuts) const
	{
		auto pose = ReplayExcavationRoute(this->Job(), this->points, this->Job().cursor).next;
		std::vector<TileIndex> authorized;
		// Each advance keeps the turned heading; its sweep is accepted working space.
		for (uint i = 0; i < cuts; ++i) {
			const auto step = AdvanceExcavationPlanningPose(pose, heading);
			REQUIRE(step.error == ExcavationError::None);
			authorized.insert(authorized.end(), step.cut.swept_tiles.begin(), step.cut.swept_tiles.end());
			pose = step.next;
		}
		auto outline = NormalizeExcavationOutline(GetTileLayer(pose.position), std::array{pose.position});
		const auto work = NormalizeExcavationOutline(outline.layer, authorized);
		REQUIRE(outline.error == ExcavationError::None);
		REQUIRE(work.error == ExcavationError::None);
		outline.authorized_tiles = work.requested_tiles;
		return outline;
	}

	/** @return The job's own floor and working space, which its planned remainder already covers. */
	ExcavationOutline Same() const
	{
		ExcavationOutline outline;
		outline.layer = GetTileLayer(this->Job().commissioning_anchor);
		outline.requested_tiles = this->Job().requested_tiles;
		outline.authorized_tiles = this->Job().authorized_tiles;
		return outline;
	}

	/** @return Spoil aboard the wagons now, as the TBM owner measures it. */
	ExcavationLoadStart Load() const
	{
		return {this->Spoil(TBMConsistRole::EarthWagon), this->Spoil(TBMConsistRole::RockWagon)};
	}
};

/**
 * @param route Cuts.
 * @return Every cell they sweep.
 */
static std::set<TileIndex> Swept(std::span<const ExcavationCutGeometry> route)
{
	std::set<TileIndex> tiles;
	for (const auto &cut : route) tiles.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
	return tiles;
}

/** Task 11.6: mid-route replacement keeps the completed prefix, swaps the remainder and claims exactly the new cuts. */
TEST_CASE("A replacement keeps the completed cuts and swaps the remainder", "[demo-fix-change-work]")
{
	ChangeWorkRig rig;
	rig.Bore(2);
	const auto job = rig.Job();
	REQUIRE(job.cursor == 2);
	const auto outline = rig.Turn(Direction::S, 4);
	const auto before = rig.Digest();
	const auto next_id = GetGeologyWorldState().next_job_id;
	const auto plan = PlanExcavationJobReplacement(job.id, outline, rig.points);
	REQUIRE(plan.error == ExcavationError::None);
	REQUIRE_FALSE(plan.partial);
	REQUIRE_FALSE(plan.route.empty());
	CHECK(plan.route.front() != job.route[job.cursor]);
	CHECK(rig.Digest() == before);

	const auto &geology = GetGeologyWorldState();
	const std::span<const ExcavationCutGeometry> completed{job.route.data(), job.cursor};
	const std::span<const ExcavationCutGeometry> remainder{job.route.data() + job.cursor, job.route.size() - job.cursor};
	const auto kept = Swept(completed), dropped = Swept(remainder), footprint = Swept(plan.route);
	// The quote charges every new cell that is not open yet, including the dropped cuts' unopened cells it reuses.
	size_t unopened = 0;
	bool reuses_dropped = false;
	for (TileIndex tile : footprint) {
		unopened += geology.terrain.Get(tile) != ExcavationState::Excavated;
		// A reused cell of the dropped remainder that no completed cut opened is still claimed, active rock.
		if (dropped.contains(tile) && !kept.contains(tile)) reuses_dropped = true;
	}
	CHECK(plan.estimate.new_cells == unopened);
	REQUIRE(reuses_dropped);

	// The estimate admits the replacement without touching anything.
	CHECK(ReplaceExcavationJobRemainder(job.id, job.cursor, outline, plan.route, rig.Load(), false).error == ExcavationError::None);
	CHECK(rig.Digest() == before);
	CHECK(rig.Job() == job);

	REQUIRE(ReplaceExcavationJobRemainder(job.id, job.cursor, outline, plan.route, rig.Load(), true).error == ExcavationError::None);
	const auto after = rig.Job();
	CHECK(after.id == job.id);
	CHECK(after.cutter == job.cutter);
	CHECK(after.origin == job.origin);
	CHECK(after.cursor == job.cursor);
	CHECK(after.revision == job.revision + 1);
	CHECK(after.completed_earth_t == job.completed_earth_t);
	CHECK(after.completed_rock_t == job.completed_rock_t);
	CHECK(after.completed_work_cost == job.completed_work_cost);
	REQUIRE(after.route.size() == job.cursor + plan.route.size());
	CHECK(std::equal(completed.begin(), completed.end(), after.route.begin()));
	CHECK(std::equal(plan.route.begin(), plan.route.end(), after.route.begin() + job.cursor));
	REQUIRE(after.pass_terms.has_value());
	CHECK(ValidateExcavationPassTerms(after, *after.pass_terms).error == ExcavationError::None);
	CHECK(after.pass_terms->exchange_after.back() == after.route.size());
	// Exchanges planned for the kept cuts stay.
	for (uint16_t prefix : job.pass_terms->exchange_after) {
		if (prefix <= job.cursor) CHECK(std::ranges::binary_search(after.pass_terms->exchange_after, prefix));
	}

	// Completed floor never reverts; the new cuts hold the claims; cells only the dropped cuts claimed are rock again.
	size_t claims = 0;
	for (const auto &[tile, owner] : geology.job_claims) claims += owner == job.id;
	CHECK(claims == footprint.size());
	for (TileIndex tile : kept) CHECK(geology.terrain.Get(tile) == ExcavationState::Excavated);
	for (TileIndex tile : footprint) {
		CHECK(geology.job_claims.at(tile) == job.id);
		CHECK(geology.terrain.Get(tile) != ExcavationState::Solid);
	}
	// A dropped cell is released to rock unless a completed cut opened it or a new cut reuses it, which keeps it active.
	for (TileIndex tile : dropped) {
		if (kept.contains(tile)) continue;
		if (footprint.contains(tile)) {
			CHECK(geology.terrain.Get(tile) == ExcavationState::Excavating);
		} else {
			CHECK_FALSE(geology.job_claims.contains(tile));
			CHECK(geology.terrain.Get(tile) == ExcavationState::Solid);
		}
	}
	CHECK(Company::Get(_current_company)->money == before.money);
	CHECK(GetGeologyWorldState().next_job_id == next_id); // Same identity: no job is allocated.

	// A reload rebuilds exactly these claims.
	auto rebuilt = GetGeologyWorldState();
	const auto published = rebuilt.job_claims;
	REQUIRE(RebuildExcavationJobClaims(rebuilt) == GeologyError::None);
	CHECK(rebuilt.job_claims == published);
}

/** Task 11.6: face work on the cut in progress is abandoned by a different first cut and kept by the same one. */
TEST_CASE("A replacement abandons partial face work only when the next cut changes", "[demo-fix-change-work]")
{
	ChangeWorkRig rig;
	rig.Bore(2);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None); // Standard earns one whole work tick in two simulation ticks.
	const auto job = rig.Job();
	REQUIRE(job.partial_work_ticks > 0);

	SECTION("a different first cut") {
		const auto outline = rig.Turn(Direction::S, 4);
		const auto plan = PlanExcavationJobReplacement(job.id, outline, rig.points);
		REQUIRE(plan.error == ExcavationError::None);
		REQUIRE(ReplaceExcavationJobRemainder(job.id, job.cursor, outline, plan.route, rig.Load(), true).error == ExcavationError::None);
		CHECK(rig.Job().partial_work_ticks == 0);
	}
	SECTION("the same remainder") {
		// Replacing the remainder by itself keeps the same cuts, floor and claims. Its exchanges after the cursor are planned
		// again from the load really aboard, which this rig emptied.
		const std::vector<ExcavationCutGeometry> remainder(job.route.begin() + job.cursor, job.route.end());
		const auto claims = GetGeologyWorldState().job_claims;
		REQUIRE(ReplaceExcavationJobRemainder(job.id, job.cursor, rig.Same(), remainder, rig.Load(), true).error == ExcavationError::None);
		const auto after = rig.Job();
		CHECK(after.partial_work_ticks == job.partial_work_ticks);
		CHECK(after.route == job.route);
		CHECK(after.requested_tiles == job.requested_tiles);
		CHECK(after.authorized_tiles == job.authorized_tiles);
		REQUIRE(after.pass_terms.has_value());
		CHECK(ValidateExcavationPassTerms(after, *after.pass_terms).error == ExcavationError::None);
		for (uint16_t prefix : job.pass_terms->exchange_after) {
			if (prefix <= job.cursor) CHECK(std::ranges::binary_search(after.pass_terms->exchange_after, prefix));
		}
		CHECK(after.revision == job.revision + 1);
		CHECK(GetGeologyWorldState().job_claims == claims);
	}
}

/** Task 11.6: spoil already aboard counts toward the first exchange of the new remainder. */
TEST_CASE("Spoil aboard seeds the first exchange of the new remainder", "[demo-fix-change-work]")
{
	ChangeWorkRig rig;
	rig.Bore(2);
	const auto job = rig.Job();
	const auto &terms = *job.pass_terms;
	// No exchange is planned at this cursor, so only the load can force one.
	std::string breakpoints;
	for (uint16_t prefix : terms.exchange_after) breakpoints += fmt::format("{} ", prefix);
	INFO("exchange_after " << breakpoints);
	REQUIRE_FALSE(std::ranges::binary_search(terms.exchange_after, static_cast<uint16_t>(job.cursor)));
	const auto outline = rig.Turn(Direction::S, 4);
	const auto plan = PlanExcavationJobReplacement(job.id, outline, rig.points);
	REQUIRE(plan.error == ExcavationError::None);
	SECTION("empty wagons") {
		REQUIRE(ReplaceExcavationJobRemainder(job.id, job.cursor, outline, plan.route, {}, true).error == ExcavationError::None);
		CHECK_FALSE(std::ranges::binary_search(rig.Job().pass_terms->exchange_after, static_cast<uint16_t>(job.cursor)));
	}
	SECTION("full wagons") {
		// Full wagons cannot take the first new cut's spoil: the exchange is due before it, at once.
		REQUIRE(ReplaceExcavationJobRemainder(job.id, job.cursor, outline, plan.route, {terms.earth_capacity_t, terms.rock_capacity_t}, true).error == ExcavationError::None);
		CHECK(std::ranges::binary_search(rig.Job().pass_terms->exchange_after, static_cast<uint16_t>(job.cursor)));
	}
}

/** Task 11.6: refusals leave every owner unchanged. */
TEST_CASE("A replacement refuses without completed work, when stale, or through another job's claim", "[demo-fix-change-work]")
{
	ChangeWorkRig rig;
	SECTION("no completed cut") {
		const auto job = rig.Job();
		REQUIRE(job.cursor == 0);
		const std::vector<ExcavationCutGeometry> remainder(job.route.begin(), job.route.end());
		const auto before = rig.Digest();
		CHECK(PlanExcavationJobReplacement(job.id, rig.Same(), rig.points).error == ExcavationError::InvalidRoute);
		CHECK(ReplaceExcavationJobRemainder(job.id, 0, rig.Same(), remainder, {}, true).error == ExcavationError::InvalidRoute);
		CHECK(rig.Digest() == before);
		CHECK(rig.Job() == job);
		return;
	}
	rig.Bore(2);
	const auto job = rig.Job();
	const auto outline = rig.Turn(Direction::S, 4);
	const auto plan = PlanExcavationJobReplacement(job.id, outline, rig.points);
	REQUIRE(plan.error == ExcavationError::None);
	SECTION("stale cursor") {
		CHECK(ReplaceExcavationJobRemainder(job.id, job.cursor - 1, outline, plan.route, rig.Load(), true).error == ExcavationError::StaleParticipant);
		// Another completed cut makes the reviewed quote stale.
		rig.Bore(1);
		const auto before = rig.Digest();
		const auto saved = rig.Job();
		CHECK(ReplaceExcavationJobRemainder(job.id, job.cursor, outline, plan.route, rig.Load(), true).error == ExcavationError::StaleParticipant);
		CHECK(rig.Digest() == before);
		CHECK(rig.Job() == saved);
	}
	SECTION("another company") {
		AutoRestoreBackup actor(_current_company, CompanyID{1});
		CHECK(PlanExcavationJobReplacement(job.id, outline, rig.points).error == ExcavationError::InvalidOwner);
		CHECK(ReplaceExcavationJobRemainder(job.id, job.cursor, outline, plan.route, {}, true).error == ExcavationError::InvalidOwner);
	}
	SECTION("empty remainder and closed job") {
		CHECK(ReplaceExcavationJobRemainder(job.id, job.cursor, outline, {}, {}, true).error == ExcavationError::InvalidRoute);
		REQUIRE(CancelExcavationJob(job.id) == ExcavationError::None);
		CHECK(PlanExcavationJobReplacement(job.id, outline, rig.points).error == ExcavationError::AlreadyCompleted);
		CHECK(ReplaceExcavationJobRemainder(job.id, job.cursor, outline, plan.route, {}, true).error == ExcavationError::AlreadyCompleted);
	}
	SECTION("another job's claim") {
		const auto &geology = GetGeologyWorldState();
		/**
		 * @param tile Cell to check.
		 * @return Whether it is untouched rock: no other work may be disturbed by placing the other job there.
		 */
		const auto untouched = [&geology](TileIndex tile) {
			return IsValidTile(tile) && geology.terrain.Get(tile) == ExcavationState::Solid && !geology.job_claims.contains(tile);
		};
		TileIndex anchor = INVALID_TILE;
		std::vector<TileIndex> claimed;
		// Find a new cell whose other job (opened 3x3 starter, then cells x+2..x+3 claimed) disturbs no existing work.
		for (TileIndex tile : Swept(plan.route)) {
			const TileIndex candidate = TileAddWrap(tile, -2, 0);
			if (candidate == INVALID_TILE) continue;
			bool clear = true;
			for (int y = -1; y <= 1 && clear; ++y) {
				for (int x = -1; x <= 3 && clear; ++x) clear = untouched(TileAddWrap(candidate, x, y));
			}
			if (!clear) continue;
			anchor = candidate;
			for (int y = -1; y <= 1; ++y) {
				for (int x = 2; x <= 3; ++x) claimed.push_back(TileAddWrap(candidate, x, y));
			}
			break;
		}
		REQUIRE(anchor != INVALID_TILE);
		rig.fixture.world.ClaimStraightWork(anchor);
		const auto before = rig.Digest();
		const auto saved = rig.Job();
		const auto refused = ReplaceExcavationJobRemainder(job.id, job.cursor, outline, plan.route, rig.Load(), true);
		CHECK(refused.error == ExcavationError::Conflict);
		CHECK(std::ranges::find(claimed, refused.failure_tile) != claimed.end());
		CHECK(rig.Digest() == before);
		CHECK(rig.Job() == saved);
	}
}

/** @return First corner of a new floor beside the planned corridor, SW of the cutter's line: reaching it takes a bend. */
static TileIndex ChangeFrom() { return TileXYL(22, 25, MapLayer::Underground); }
/** @return Opposite corner of that floor. Both are computed on the live map, never at static initialization. */
static TileIndex ChangeTo() { return TileXYL(23, 26, MapLayer::Underground); }

/**
 * @param quote A refused or accepted change quote.
 * @return Its refusal text for test output, or nothing when accepted.
 */
static std::string Refusal(const TBMWorkChangePreview &quote)
{
	if (quote.result.Succeeded()) return {};
	return fmt::format("{} (outline error {} at {}, plan error {} at {})", GetString(quote.result.GetErrorMessage()), to_underlying(quote.outline.error),
		quote.outline.failure_tile.base(), to_underlying(quote.plan.error), quote.plan.failure_tile.base());
}

/** Task 12.6: the command matches its quote, needs no pause, keeps the phase and the next cut is the new route's first. */
TEST_CASE("Change work area keeps a boring machine working on the new cuts", "[demo-fix-change-work-ui]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ChangeWorkRig rig;
	rig.Bore(2);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	const auto project = rig.Project();
	const auto job = rig.Job();
	REQUIRE(project.phase == TBMPhase::Bore);
	REQUIRE(project.blocked != TBMBlockedReason::Paused);
	REQUIRE(job.partial_work_ticks > 0);
	const auto quote = PreviewTBMWorkChange(_current_company, project.id, ChangeFrom(), ChangeTo());
	INFO("refusal: " << Refusal(quote));
	REQUIRE(quote.result.Succeeded());
	CHECK(quote.cursor == job.cursor);
	CHECK(quote.dropped_cuts == job.route.size() - job.cursor);
	REQUIRE(quote.plan.route.front() != job.route[job.cursor]);
	CHECK(quote.abandoned_work_ticks == job.partial_work_ticks); // The cut in progress is abandoned, and the quote says so.
	CHECK(quote.cut_work_ticks >= job.partial_work_ticks);

	// Parity: the command's estimate is the quote and changes nothing.
	const auto before = rig.Digest();
	const auto estimate = CmdChangeTBMWork(DoCommandFlags{}, project.id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area);
	CHECK(estimate.Succeeded());
	CHECK(estimate.GetCost() == quote.result.GetCost());
	CHECK(rig.Digest() == before);
	CHECK(rig.Job() == job);
	CHECK(rig.Project() == project);

	REQUIRE(CmdChangeTBMWork(DoCommandFlag::Execute, project.id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	const auto after = rig.Project();
	CHECK(after.phase == project.phase);
	CHECK(after.blocked == project.blocked);
	CHECK(after.heading == project.heading);
	CHECK(after.work_remainder == project.work_remainder);
	CHECK(after.revision == project.revision + 1);
	CHECK(rig.Job().cursor == job.cursor);
	CHECK(rig.Job().route[job.cursor] == quote.plan.route.front());
	CHECK(rig.Job().partial_work_ticks == 0);
	CHECK(Company::Get(_current_company)->money == before.money); // Changing the plan costs nothing; the cuts pay as they go.
	// The face's laid track leads on to the new first cut, so the machine can drive back to its face from anywhere.
	std::vector<size_t> ends;
	const auto path = GetTBMWorkRailPath(rig.Job(), job.cursor + 1, ends);
	REQUIRE(ends.size() > job.cursor);
	const size_t face = ends[job.cursor - 1];
	REQUIRE(IsPlainRailTile(path[face]));
	const DiagDirection entry = DiagdirBetweenTiles(path[face], path[face - 1]);
	const DiagDirection exit = DiagdirBetweenTiles(path[face], path[face + 1]);
	bool joined = false;
	for (Trackdir trackdir : DiagdirReachesTrackdirs(ReverseDiagDir(entry))) {
		joined |= TrackdirToExitdir(trackdir) == exit && GetTrackBits(path[face]).Test(TrackdirToTrack(trackdir));
	}
	CHECK(joined);

	// The machine simply carries on: its next completed cut is the new route's first.
	rig.BoreOneCut();
	CHECK(rig.Job().cursor == job.cursor + 1);
	for (TileIndex tile : quote.plan.route.front().swept_tiles) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
}

/** Task 12.6: the quote is bound to the completed-cut count, not to the work ticks of the cut in progress. */
TEST_CASE("A change quote goes stale only when another cut completes", "[demo-fix-change-work-ui]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ChangeWorkRig rig;
	rig.Bore(2);
	const auto id = rig.Project().id;
	const auto quote = PreviewTBMWorkChange(_current_company, id, ChangeFrom(), ChangeTo());
	INFO("refusal: " << Refusal(quote));
	REQUIRE(quote.result.Succeeded());
	// Work on the cut in progress moves the job revision every work tick; the quote stays valid.
	for (int tick = 0; tick < 4; ++tick) REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	REQUIRE(rig.Job().partial_work_ticks > 0);
	CHECK(CmdChangeTBMWork(DoCommandFlags{}, id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	// Another completed cut makes it stale.
	rig.BoreOneCut();
	const auto before = rig.Digest();
	const auto stale = CmdChangeTBMWork(DoCommandFlag::Execute, id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area);
	CHECK(stale.Failed());
	CHECK(stale.GetErrorMessage() == STR_ERROR_TBM_CHANGE_STALE);
	CHECK(rig.Digest() == before);
}

/** Task 12.6: returning and stored machines accept the change and keep their phase; refusals change nothing. */
TEST_CASE("Change work area accepts returning and parked machines and refuses the rest", "[demo-fix-change-work-ui]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ChangeWorkRig rig;
	SECTION("no completed cut") {
		CHECK(PreviewTBMWorkChange(_current_company, rig.Project().id, ChangeFrom(), ChangeTo()).result.GetErrorMessage() == STR_ERROR_TBM_CHANGE_NO_CUT);
		CHECK_FALSE(CanChangeTBMWork(rig.Project()));
		return;
	}
	rig.Bore(2);
	const auto id = rig.Project().id;
	SECTION("returning to park") {
		REQUIRE(ControlTBMProject(id, TBMProjectAction::ReturnAndPark, true) == TBMProjectError::None);
	}
	SECTION("returning for an exchange") {
		rig.Reload(TBMPhase::ReturnForExchange);
	}
	SECTION("parked") {
		rig.Reload(TBMPhase::Parked);
	}
	SECTION("another company") {
		AutoRestoreBackup actor(_current_company, CompanyID{1});
		const auto before = rig.Digest();
		CHECK(PreviewTBMWorkChange(CompanyID{1}, id, ChangeFrom(), ChangeTo()).result.GetErrorMessage() == STR_ERROR_TBM_EXTEND_REFERENCES);
		CHECK(CmdChangeTBMWork(DoCommandFlag::Execute, id, ChangeFrom(), ChangeTo(), 2, 0, ExcavationPlanIntent::Area).Failed());
		CHECK(rig.Digest() == before);
		return;
	}
	SECTION("being released") {
		REQUIRE(ControlTBMProject(id, TBMProjectAction::Cancel, true) == TBMProjectError::None);
		CHECK(PreviewTBMWorkChange(_current_company, id, ChangeFrom(), ChangeTo()).result.GetErrorMessage() == STR_ERROR_TBM_PROJECT_FINISHED);
		CHECK_FALSE(CanChangeTBMWork(rig.Project()));
		return;
	}
	const auto project = rig.Project();
	CHECK(CanChangeTBMWork(project));
	const auto quote = PreviewTBMWorkChange(_current_company, id, ChangeFrom(), ChangeTo());
	INFO("phase " << to_underlying(project.phase) << " refusal: " << Refusal(quote));
	REQUIRE(quote.result.Succeeded());
	CHECK(quote.abandoned_work_ticks == 0); // Nothing was cut since the last completed cut.
	REQUIRE(CmdChangeTBMWork(DoCommandFlag::Execute, id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	CHECK(rig.Project().phase == project.phase);
	CHECK(rig.Project().blocked == project.blocked);
	// Whenever the machine goes back to work, the cut it starts is the new route's first.
	CHECK(rig.Job().route[rig.Job().cursor] == quote.plan.route.front());
}

/** Tasks 12.3 and 12.5: the projects window and New area open Change work area; the palette quotes and confirms it. */
TEST_CASE("Change work area from the projects window and New area, without a pause", "[demo-fix-change-work-ui]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ChangeWorkRig rig;
	rig.Bore(2);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	UndergroundToolbarTestUI ui(_current_company);
	LayerWindowTestUI browser_ui(WindowClass::TBMProjects, _current_company);
	new UndergroundTestViewport(); // Planning centres the main view on the face.
	// The player's game runs; the fixture's pause would hold the posted command.
	AutoRestoreBackup running(_pause_mode, PauseModes{});
	const TBMProjectID id = rig.Project().id;
	Window *browser = ShowTBMProjectsWindow(MapLayer::Underground);
	REQUIRE(browser != nullptr);
	browser->OnPaint();
	REQUIRE_FALSE(browser->IsWidgetDisabled(WID_TP_CHANGE));
	browser->OnClick({}, WID_TP_CHANGE, 1);
	Window *palette = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(palette != nullptr);
	palette->OnPaint();
	CHECK(_thd.GetCallbackWnd() == palette);
	CHECK(palette->GetWidgetString(WID_UC_AREA, STR_UNDERGROUND_AREA) == GetString(STR_TBM_PROJECT_CHANGE_WORK));
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_TBM_CHANGE_SELECT_FLOOR, id.base()));
	CHECK(GetTBMProject(id)->blocked != TBMBlockedReason::Paused); // No pause was posted.

	palette->OnPlaceMouseUp(VPM_X_AND_Y, DDSP_DEMOLISH_AREA, {}, ChangeFrom(), ChangeTo());
	palette->OnPaint();
	const auto quote = PreviewTBMWorkChange(_current_company, id, ChangeFrom(), ChangeTo());
	INFO("refusal: " << Refusal(quote));
	REQUIRE(quote.result.Succeeded());
	REQUIRE(quote.abandoned_work_ticks > 0);
	const uint64_t done = std::min<uint64_t>(100, CeilDiv(quote.abandoned_work_ticks * 100, quote.cut_work_ticks));
	CHECK(palette->GetWidget<NWidgetStacked>(WID_UC_CHANGE_STACK)->shown_plane == 0);
	CHECK(palette->GetWidgetString(WID_UC_AREA_CHANGE, STR_UNDERGROUND_AREA_EMPTY) ==
		GetString(STR_TBM_CHANGE_SUMMARY_ABANDON, quote.cursor, quote.dropped_cuts, quote.plan.route.size(), done));
	CHECK(palette->GetWidgetString(WID_UC_AREA_CONFIRM, STR_UNDERGROUND_AREA_CONFIRM) == GetString(STR_TBM_CHANGE_CONFIRM));
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_TBM_CHANGE_READY));
	REQUIRE_FALSE(palette->IsWidgetDisabled(WID_UC_AREA_CONFIRM));
	const uint32_t cursor = rig.Job().cursor;
	palette->OnClick({}, WID_UC_AREA_CONFIRM, 1);
	CHECK(rig.Job().route[cursor] == quote.plan.route.front());
	CHECK(rig.Project().phase == TBMPhase::Bore);
	CHECK(_thd.GetCallbackWnd() != palette);

	// Task 12.5: New area on the project's own portal now changes its work area too, still without a pause.
	palette->OnClick({}, WID_UC_AREA, 1);
	palette->OnPlaceObject({}, rig.Job().commissioning_anchor);
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_TBM_CHANGE_SELECT_FLOOR, id.base()));
	CHECK(GetTBMProject(id)->blocked != TBMBlockedReason::Paused);
	// Cancelling leaves the machine working on its plan.
	ResetObjectToPlace();
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_TBM_CHANGE_CANCELLED));
}

/** Task 12.5: New area on a finished project's portal offers release and new work with the same machine. */
TEST_CASE("New area on a finished project's portal offers release and new work", "[demo-fix-change-work-ui]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ChangeWorkRig rig;
	UndergroundToolbarTestUI ui(_current_company);
	LayerWindowTestUI query_ui(WindowClass::ConfirmPopupQuery, _current_company);
	AutoRestoreBackup running(_pause_mode, PauseModes{});
	// A cancelled job leaves a finished project at the portal, ready to be released.
	const TBMProjectID id = rig.Project().id;
	const TileIndex anchor = rig.Job().commissioning_anchor;
	REQUIRE(CancelExcavationJob(rig.Job().id) == ExcavationError::None);
	rig.Reload(TBMPhase::Cancelled);
	REQUIRE(IsTBMProjectFinished(rig.Project().phase));
	REQUIRE_FALSE(CanChangeTBMWork(rig.Project()));
	Window *palette = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(palette != nullptr);
	palette->OnPaint();
	palette->OnClick({}, WID_UC_AREA, 1);
	palette->OnPlaceObject({}, anchor);
	Window *query = FindWindowByClass(WindowClass::ConfirmPopupQuery);
	REQUIRE(query != nullptr);
	CHECK(rig.Project().phase == TBMPhase::Cancelled); // Asking changes nothing.
	// Declining keeps the project; the portal is not armed for new work.
	query->OnClick({}, WID_Q_NO, 1);
	Window::DeleteClosedWindows();
	CHECK(GetTBMProject(id) != nullptr);
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) != GetString(STR_UNDERGROUND_AREA_SELECT_FLOOR));
	// Accepting releases it through the ordinary Retire command and arms the same portal for new work.
	palette->OnPlaceObject({}, anchor);
	query = FindWindowByClass(WindowClass::ConfirmPopupQuery);
	REQUIRE(query != nullptr);
	query->OnClick({}, WID_Q_YES, 1);
	Window::DeleteClosedWindows();
	CHECK(GetTBMProject(id) == nullptr);
	CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_UNDERGROUND_AREA_SELECT_FLOOR));
	ResetObjectToPlace();
}
