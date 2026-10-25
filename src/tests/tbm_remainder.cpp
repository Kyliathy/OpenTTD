/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_remainder.cpp Replacing an unfinished job's remainder: the completed cuts stay, the planned rest is swapped
 * for new cuts from the last completed position, atomically and without undoing anything physical.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23C native geological remainder owner.
 * Explicit component exchanges empty earlier loads; command and driven lifecycle checks belong to subsequent batches.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_remainder_fixture.h"
#include "excavation_claim_fixture.h"
#include "../safeguards.h"

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
TEST_CASE("A replacement keeps the completed cuts and swaps the remainder", "[tbm-remainder]")
{
	TBMRemainderRig rig;
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
TEST_CASE("A replacement abandons partial face work only when the next cut changes", "[tbm-remainder]")
{
	TBMRemainderRig rig;
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
TEST_CASE("Spoil aboard seeds the first exchange of the new remainder", "[tbm-remainder]")
{
	TBMRemainderRig rig;
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

/** Commissioning retains the first cut; an untouched job cannot use replacement to bypass its original admission. */
TEST_CASE("A replacement refuses before the first completed cut", "[tbm-remainder]")
{
	TBMRemainderRig rig;
	const auto job = rig.Job();
	REQUIRE(job.cursor == 0);
	const std::vector<ExcavationCutGeometry> remainder(job.route.begin(), job.route.end());
	const auto before = rig.Digest();
	CHECK(PlanExcavationJobReplacement(job.id, rig.Same(), rig.points).error == ExcavationError::InvalidRoute);
	CHECK(ReplaceExcavationJobRemainder(job.id, 0, rig.Same(), remainder, {}, true).error == ExcavationError::InvalidRoute);
	CHECK(rig.Digest() == before);
	CHECK(rig.Job() == job);
}

/** All refusal sections must be discovered; an earlier returning SECTION must not hide stale or foreign-owner coverage. */
TEST_CASE("A replacement refuses stale closed or foreign work unchanged", "[tbm-remainder]")
{
	TBMRemainderRig rig;
	rig.Bore(2);
	const auto job = rig.Job();
	// The foreign-job case needs an untouched starter and its full two-cut footprint beyond the old paid/claimed
	// corridor. A longer proposed remainder provides that real space without overwriting another owner's cells.
	const auto outline = rig.Turn(Direction::S, 8);
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
		const auto before = rig.Digest();
		{
			AutoRestoreBackup actor(_current_company, CompanyID{1});
			CHECK(PlanExcavationJobReplacement(job.id, outline, rig.points).error == ExcavationError::InvalidOwner);
			CHECK(ReplaceExcavationJobRemainder(job.id, job.cursor, outline, plan.route, {}, true).error == ExcavationError::InvalidOwner);
		}
		CHECK(rig.Digest() == before);
		CHECK(rig.Job() == job);
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
		// Find a new cell whose other job's starter and two complete sweeps disturb no existing work.
		// Native claims cover open swept cells too: the claimed x..x+3 span protects physical working clearance.
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
				for (int x = 0; x <= 3; ++x) claimed.push_back(TileAddWrap(candidate, x, y));
			}
			break;
		}
		REQUIRE(anchor != INVALID_TILE);
		const auto other = ClaimSeparateExcavationWork(rig.fixture.world, anchor);
		const auto before = rig.Digest();
		const auto saved = rig.Job();
		const auto refused = ReplaceExcavationJobRemainder(job.id, job.cursor, outline, plan.route, rig.Load(), true);
		CHECK(refused.error == ExcavationError::Conflict);
		CHECK(std::ranges::find(claimed, refused.failure_tile) != claimed.end());
		REQUIRE(GetGeologyWorldState().job_claims.contains(refused.failure_tile));
		CHECK(GetGeologyWorldState().job_claims.at(refused.failure_tile) == other);
		CHECK(rig.Digest() == before);
		CHECK(rig.Job() == saved);
	}
}

/** Geological replanning cannot consume or move the actual spoil already earned by paid cuts. */
TEST_CASE("Replacing a loaded cutter remainder preserves native cargo cash and deposits", "[tbm-remainder]")
{
	TBMRemainderRig rig;
	rig.Bore(1);
	rig.BoreOneCut(); // Keep this actual paid cut's spoil aboard; there is no component exchange afterward.
	const auto job = rig.Job();
	const auto load = rig.Load();
	REQUIRE(load.earth_t + load.rock_t > 0);
	const auto packets = GetTBMPacketOwners(rig.front);
	const auto chain = GetTBMChainIDs(rig.front);
	const auto cash = Company::Get(_current_company)->money;
	const auto deposits = CaptureGeologyState().deposits;
	const auto random0 = _random.state[0], random1 = _random.state[1];
	const std::vector<ExcavationCutGeometry> remainder(job.route.begin() + job.cursor, job.route.end());
	REQUIRE(ReplaceExcavationJobRemainder(job.id, job.cursor, rig.Same(), remainder, load, true).error == ExcavationError::None);
	CHECK(GetTBMPacketOwners(rig.front) == packets);
	CHECK(GetTBMChainIDs(rig.front) == chain);
	CHECK(rig.Load().earth_t == load.earth_t);
	CHECK(rig.Load().rock_t == load.rock_t);
	CHECK(Company::Get(_current_company)->money == cash);
	const auto after = CaptureGeologyState().deposits;
	REQUIRE(after.size() == deposits.size());
	// Every original deposit keeps both its finite reserve and exact extent; matching aggregate tonnes alone
	// could conceal reallocating previously consumed material into a different seam or claimant.
	for (size_t i = 0; i < deposits.size(); ++i) {
		const auto &before = deposits[i];
		const auto &current = after[i];
		CHECK(std::tie(current.id, current.layer, current.resource, current.hardness, current.claimant,
			current.initial_reserve, current.remaining_reserve, current.surveyed_companies) ==
			std::tie(before.id, before.layer, before.resource, before.hardness, before.claimant,
			before.initial_reserve, before.remaining_reserve, before.surveyed_companies));
		REQUIRE(current.spans.size() == before.spans.size());
		// Preserve each canonical row span, including gaps; no new rock can be manufactured by changing a seam's shape.
		for (size_t j = 0; j < before.spans.size(); ++j) {
			CHECK(std::tie(current.spans[j].y, current.spans[j].first_x, current.spans[j].last_x) ==
				std::tie(before.spans[j].y, before.spans[j].first_x, before.spans[j].last_x));
		}
	}
	CHECK(_random.state[0] == random0);
	CHECK(_random.state[1] == random1);
	CHECK(rig.Job().completed_work_cost == job.completed_work_cost);
}

