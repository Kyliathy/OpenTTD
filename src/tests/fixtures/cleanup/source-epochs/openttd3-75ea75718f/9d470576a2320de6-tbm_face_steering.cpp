/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_steering.cpp The hauler driving itself to the face: through its project's pair to the stand behind the
 * cutter, stopping there and following the stand as the cutter advances, never into the next cut's sweep; and the way
 * out again, with the cutter's return waiting until it has left. Both trains move only through the game's vehicle step.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 4 (task 4.6)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-a-face-exchange-design.md (rules F1, F4, F6 and F7)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_source_loop_fixture.h"
#include "../tbm_exchange.h"
#include "../tbm_cmd.h"
#include "../safeguards.h"

/** What the face checks saw while a source loop ran. */
struct FaceWatch {
	std::vector<TileIndex> stands; ///< Distinct stand tiles the hauler stopped on, in order.
	bool entered_sweep = false; ///< A hauler part stood in the next cut's sweep.
	bool passed_stand = false; ///< A hauler part stood beyond the current stand, towards the cutter.
	bool shared_tile = false; ///< A hauler part stood on a tile of the cutter consist.
	bool through_pair = false; ///< A hauler part stood on the pair's rock-side housing.
	uint stand_ticks = 0; ///< Ticks the hauler waited at a stand.
	uint16_t top_speed_inside = 0; ///< Highest speed of the hauler with a part on the support, where the stand stops it.
};

/**
 * @param train Front engine.
 * @param tile Tile to look at.
 * @return Whether a running part of the train stands on the tile; magazine storage is off its public running track.
 */
static bool HasPartOn(const Train *train, TileIndex tile)
{
	for (const Train *part = train; part != nullptr; part = part->Next()) {
		// Stored machinery shares the housing's identity with its independent
		// public track. It does not occupy that running line or collide with an
		// actual departing hauler (Plan18 8FA/Plan3 native magazine contract).
		if (part->track != Track::Depot && part->tile == tile) return true;
	}
	return false;
}

/**
 * Run one game tick of the loop and check the face rules on the trains' new positions.
 * @param loop Loop to run.
 * @param watch What the checks saw so far.
 */
static void TickAndWatch(SourceLoop &loop, FaceWatch &watch)
{
	AdvanceSpoilReceiver(*loop.earth);
	AdvanceSpoilReceiver(*loop.rock);
	CallVehicleTicks();
	++loop.ticks;
	REQUIRE_FALSE(loop.cutter->vehstatus.Test(VehState::Crashed));
	REQUIRE_FALSE(loop.hauler->vehstatus.Test(VehState::Crashed));
	const ExcavationJob &job = loop.Job();
	// The next cut's sweep must stay free for the cut, and the hauler never shares a cell with the cutter consist.
	for (const Train *part = loop.hauler; part != nullptr; part = part->Next()) {
		if (part->track == Track::Depot) continue; // Native stored parts are off the running corridor, even when a parked cutter uses the same housing identity.
		if (job.cursor < job.route.size() && std::ranges::find(job.route[job.cursor].swept_tiles, part->tile) != job.route[job.cursor].swept_tiles.end()) watch.entered_sweep = true;
		if (HasPartOn(loop.cutter, part->tile)) watch.shared_tile = true;
		if (part->tile == loop.rock_end) watch.through_pair = true;
		if (GetTBMSupportTile(part->tile) != nullptr) watch.top_speed_inside = std::max(watch.top_speed_inside, loop.hauler->cur_speed);
	}
	// The hauler never goes past its stand: every part on the work path stays at or behind the stand's cell.
	if (const auto stand = FindTBMFaceStand(loop.Project()); stand.has_value() && job.cursor > 0) {
		std::vector<size_t> ends;
		const auto path = GetTBMWorkRailPath(job, job.cursor, ends);
		const auto at = std::ranges::find(path, stand->tile);
		for (const Train *part = loop.hauler; part != nullptr && at != path.end(); part = part->Next()) {
			const auto on = std::ranges::find(path, part->tile);
			if (on != path.end() && on > at) watch.passed_stand = true;
		}
		// A hauler held at its meeting point on the stand is waiting there; note each new stand it reached.
		if (loop.Cycle().phase == TBMHaulerPhase::AwaitExchange && loop.hauler->GetMovingFront()->tile == stand->tile) {
			++watch.stand_ticks;
			if (watch.stands.empty() || watch.stands.back() != stand->tile) watch.stands.push_back(stand->tile);
		}
	}
}

/**
 * @param loop Loop to describe.
 * @return Both trains' phases and positions, for a failed expectation.
 */
static std::string DescribeFace(const SourceLoop &loop)
{
	const auto cycle = loop.Cycle();
	return fmt::format("tick {} cutter: {}; hauler phase {} wait {}; meeting {}\ncutter {}\nhauler {}", loop.ticks,
		DescribeProject(loop.Project()), to_underlying(cycle.phase), to_underlying(cycle.wait), to_underlying(GetTBMMeetingKind(loop.Project())),
		DescribeTrain(loop.cutter), DescribeTrain(loop.hauler));
}

/**
 * Run the loop until the hauler has waited on a given number of stands, stopping before the cut that makes the first
 * exchange due: the face wait and transfer belong to later groups.
 * @param loop Loop to run.
 * @param watch What the checks saw.
 * @param stands Number of distinct stands to reach.
 * @param reserve Turn path reservation on for every train once the cutter first meets at the face, so the hauler's
 *                way to the stand is reserved. Before then the loop runs as at the bay, without reservation.
 */
static void DriveToStands(SourceLoop &loop, FaceWatch &watch, size_t stands, bool reserve = false)
{
	const uint16_t first = loop.Job().pass_terms->exchange_after.front();
	// Each loop step is one game tick; the bound only ends a stalled run.
	while (watch.stands.size() < stands && loop.ticks < 60000) {
		if (loop.Job().cursor + 1 >= first && loop.Project().phase == TBMPhase::Bore) break;
		if (reserve && GetTBMMeetingKind(loop.Project()) == TBMMeetingKind::Face) _settings_game.pf.reserve_paths = true;
		TickAndWatch(loop, watch);
	}
}

/** Task 4.6: the turn holder finds its stand through the pair, stops on it and follows it, with reservation off and on. */
TEST_CASE("The hauler drives through the pair to its stand and follows it", "[demo-fix-face-steering]")
{
	for (const bool reserve : {false, true}) {
		INFO("reserve_paths " << reserve);
		SourceLoop loop(LoopStart::Long);
		AutoRestoreBackup reserve_paths(_settings_game.pf.reserve_paths, false);
		FaceWatch watch;
		DriveToStands(loop, watch, 2, reserve);
		CHECK(_settings_game.pf.reserve_paths == reserve);
		INFO(DescribeFace(loop));
		INFO("breakpoints " << loop.Job().pass_terms->exchange_after.size() << " first " << loop.Job().pass_terms->exchange_after.front());
		CHECK(watch.stands.size() >= 2);
		CHECK(watch.through_pair);
		CHECK_FALSE(watch.entered_sweep);
		CHECK_FALSE(watch.passed_stand);
		// The hold stops a hauler arriving at support speed on the stand tile itself, never in the gap before the cutter.
		INFO("top speed on support " << watch.top_speed_inside);
		CHECK(watch.top_speed_inside >= TBM_SUPPORT_MAX_SPEED / 2);
		CHECK(watch.top_speed_inside <= TBM_SUPPORT_MAX_SPEED);
		CHECK_FALSE(watch.shared_tile);
		CHECK(loop.hauler->GetNumOrders() == 1); // The cycle never wrote its orders.
	}
}

/** Task 4.6: sent back with nothing aboard, the cutter waits for the hauler, which backs out and leaves (rules F6 and F7). */
TEST_CASE("A hauler backs out and the cutter's return waits for it", "[demo-fix-face-steering]")
{
	SourceLoop loop(LoopStart::Long);
	FaceWatch watch;
	DriveToStands(loop, watch, 1);
	INFO(DescribeFace(loop));
	REQUIRE(watch.stands.size() == 1);
	// The face transfer arrives with Group 5; here the cutter has handed its load over already.
	for (Train *part = loop.cutter; part != nullptr; part = part->Next()) part->cargo.Truncate();
	loop.cutter->CargoChanged();
	REQUIRE_FALSE(IsTBMExchangeDue(loop.Project()));
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, loop.project, TBMProjectAction::ReturnAndPark).Succeeded());
	REQUIRE(loop.Project().phase == TBMPhase::ReturnToPark);
	TickAndWatch(loop, watch);
	/** @return The cutter's part positions. */
	const auto positions = [&loop] {
		std::vector<std::pair<int, int>> result;
		for (const Train *part = loop.cutter; part != nullptr; part = part->Next()) result.emplace_back(part->x_pos, part->y_pos);
		return result;
	};
	const auto held = positions();
	bool named = false, moved_while_inside = false, left = false;
	// Until the hauler is out of the corridor the cutter must not move; afterwards it backs to its magazine and parks.
	while (loop.Project().phase != TBMPhase::Parked && loop.ticks < 60000) {
		const bool inside = IsTBMHaulerInCorridor(loop.Project(), loop.hauler->index);
		if (inside) {
			moved_while_inside |= positions() != held;
			named |= loop.Project().blocked == TBMBlockedReason::Obstruction && FindTBMObstruction(loop.Project()).vehicle == loop.hauler->index;
		} else {
			left = true;
		}
		TickAndWatch(loop, watch);
	}
	// Parking can finish while the managed empty hauler starts leaving the
	// same Source magazine. Observe that real ordinary departure too before
	// claiming complete corridor clearance; the parked cutter remains stored.
	while (IsTBMHaulerInCorridor(loop.Project(), loop.hauler->index) && loop.ticks < 60000) TickAndWatch(loop, watch);
	INFO(DescribeFace(loop));
	CHECK(named);
	CHECK_FALSE(moved_while_inside);
	CHECK(left);
	CHECK(loop.Project().phase == TBMPhase::Parked);
	CHECK(loop.Cycle().phase != TBMHaulerPhase::AwaitExchange);
	CHECK_FALSE(IsTBMHaulerInCorridor(loop.Project(), loop.hauler->index));
	CHECK_FALSE(watch.shared_tile);
}
