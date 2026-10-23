/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tbm_face_controller_loop.cpp Isolated actual long Standard face loop with a bounded no-progress diagnostic.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group6.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FA.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_source_loop_fixture.h"
#include "tbm_face_loop_watch.h"

/**
 * Task 6.6: a Standard project on a long straight route, driven only by the game's vehicle step, serves every frozen
 * breakpoint at its face: cut, wait, hauler arrives, transfer, disposal, empty return, next breakpoint. The only bay
 * visit is the empty final return.
 */
TEST_CASE("A Standard loop serves every breakpoint at the face", "[demo-fix-face-controller]")
{
	SourceLoop loop(LoopStart::Long);
	TBMFaceLoopWatch watch;
	bool lost_checked = false;
	uint last_progress = 0;
	auto previous_progress = std::pair{loop.Job().cursor, loop.Cycle().exchanges};
	// Each loop step is one game tick; the bound only ends a stalled run.
	while (!loop.Delivered() && loop.ticks < 200000) {
		AdvanceSpoilReceiver(*loop.earth);
		AdvanceSpoilReceiver(*loop.rock);
		CallVehicleTicks();
		++loop.ticks;
		REQUIRE_FALSE(loop.cutter->vehstatus.Test(VehState::Crashed));
		REQUIRE_FALSE(loop.hauler->vehstatus.Test(VehState::Crashed));
		watch.Observe(loop.ticks, loop.Project(), loop.Cycle(), *loop.cutter, *loop.hauler);
		const auto progress = std::pair{loop.Job().cursor, loop.Cycle().exchanges};
		if (progress != previous_progress) { previous_progress = progress; last_progress = loop.ticks; }
		// A healthy cut/haulage interval is far shorter than12,000 native frames.
		// Preserve the exact physical hold early instead of spending a200,000-frame
		// timeout repeating an already stalled cycle without any paid progress.
		if (loop.ticks - last_progress >= 12000) {
			INFO(watch.Describe());
			INFO("cutter " << DescribeTrain(loop.cutter) << "\nhauler " << DescribeTrain(loop.hauler));
			const auto goal = GetTBMHaulerSteeringTarget(*loop.hauler);
			INFO("goal=" << (goal ? goal->tile.base() : UINT32_MAX) << " dirs=" << (goal ? goal->trackdirs.base() : 0));
			FAIL("No paid cut or actual exchange during12,000 native frames");
		}
		// Task 6.5: a hauler held on its stand is at its managed destination, even if a path search had called it lost.
		if (!lost_checked && IsTBMAwaitingFaceTransfer(loop.Project().phase) && IsTBMHaulerHeld(*loop.hauler) && loop.Cycle().phase == TBMHaulerPhase::AwaitExchange) {
			loop.hauler->vehicle_flags.Set(VehicleFlag::PathfinderLost);
			CHECK(GetTBMOrderProblem(*loop.hauler) == std::optional<StringID>{INVALID_STRING_ID});
			loop.hauler->vehicle_flags.Reset(VehicleFlag::PathfinderLost);
			lost_checked = true;
		}
	}
	INFO(watch.Describe());
	REQUIRE(loop.Delivered());
	const auto &breakpoints = loop.Job().pass_terms->exchange_after;
	REQUIRE(breakpoints.size() >= 2);
	CHECK(lost_checked);
	// Every breakpoint was served at the face, the final one included; the cutter never went to the bay before then.
	CHECK(std::vector<uint32_t>(breakpoints.begin(), breakpoints.end()) == watch.face_transfers);
	CHECK(watch.bay_exchanges.empty());
	CHECK(watch.bay_returns.empty());
	CHECK_FALSE(watch.unserved_load);
	CHECK_FALSE(watch.moved_while_waiting);
	CHECK_FALSE(watch.kept_heading_on_return);
	CHECK_FALSE(watch.moved_while_obstructed);
	CHECK_FALSE(watch.order_problem);
	CHECK(loop.Project().phase == TBMPhase::Completed);
	CHECK(SourceLoop::Aboard(loop.cutter) == SpoilTonnes{});
	CHECK(SourceLoop::Aboard(loop.hauler) == SpoilTonnes{});
	CHECK(loop.Received() == loop.Committed());
	CHECK(loop.hauler->GetNumOrders() == 1); // The cycle never wrote the player's orders.
}
