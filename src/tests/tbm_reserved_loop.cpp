/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23EB.
 * @file tbm_reserved_loop.cpp A whole driven source loop with path reservation on from the first tick. R2DF2 saw the
 * hauler, heading for its Source bay during the first cut, find no path while the cutter's tail still stood on the
 * key, turn BlockedLine and so stall the very cut that would free the key: a deadlock at cursor 0
 * (r2df2-c-execution-notes.md, Group 4). Its face tests therefore switched reservation on only at the first face
 * meeting; this case keeps it on throughout.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 10R (task 10R.2)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13D (task 13D.2: the rest of the loop)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md, "Paid boring and real spoil haulage"
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_source_loop_fixture.h"
#include "tbm_face_loop_watch.h"
#include "../tbm_exchange.h"

#include "../safeguards.h"

/**
 * @param loop Loop whose hauler is described.
 * @return One line with the hauler's cycle and the inputs its next step reads, for a stalled expectation.
 */
static std::string DescribeHaulerInputs(const SourceLoop &loop)
{
	const auto cycle = loop.Cycle();
	const auto inputs = ObserveTBMHaulerInputs(cycle);
	const auto target = GetTBMHaulerSteeringTarget(*loop.hauler);
	const auto stand = FindTBMFaceStand(loop.Project());
	return fmt::format("tick {} cutter phase {} cursor {} meeting {} turn_holder {} | hauler phase {} wait {} at_bay {} lost {} inside {} waiting {} finished {} empty {} stopped {}",
		loop.ticks, to_underlying(loop.Project().phase), loop.Job().cursor, to_underlying(GetTBMMeetingKind(loop.Project())), IsTBMFaceTurnHolder(cycle),
		to_underlying(cycle.phase), to_underlying(cycle.wait), inputs.at_bay, inputs.lost, inputs.inside, inputs.cutter_waiting, inputs.exchange_finished,
		inputs.set_empty, inputs.stopped) + fmt::format(" | lead tile {} track {} target {} | bay {} stand {}",
		loop.hauler->GetMovingFront()->tile.base(), loop.hauler->GetMovingFront()->track.base(),
		target.has_value() ? target->tile.base() : 0u, loop.source.base(), stand.has_value() ? stand->tile.base() : 0u) +
		fmt::format(" | reserved key {} rock end {} stand {} | hauler stuck {} order {} dest {}", HasLayerPortalReservation(loop.source), HasLayerPortalReservation(loop.rock_end),
		stand.has_value() && IsPlainRailTile(stand->tile) ? GetRailReservationTrackBits(stand->tile).base() : 0u,
		loop.hauler->flags.Test(VehicleRailFlag::Stuck), to_underlying(loop.hauler->current_order.GetType()), loop.hauler->dest_tile.base());
}

/**
 * Run one game tick of the loop's receivers and trains.
 * @param loop Loop to advance.
 */
static void TickLoop(SourceLoop &loop)
{
	AdvanceSpoilReceiver(*loop.earth);
	AdvanceSpoilReceiver(*loop.rock);
	CallVehicleTicks();
	++loop.ticks;
	REQUIRE_FALSE(loop.cutter->vehstatus.Test(VehState::Crashed));
	REQUIRE_FALSE(loop.hauler->vehstatus.Test(VehState::Crashed));
}

/**
 * With path reservation on from the start, the first cut is never stalled by the hauler, and its first breakpoint is
 * served at the face. The loop then runs to the end of its route with reservation still on: every frozen breakpoint is
 * exchanged once and every tonne reaches a receiver. Before r2mlvs5 task 13D.2 the loaded hauler could end its origin
 * visit in the Surface depot at the end of the line and stay there.
 */
TEST_CASE("A source loop with path reservation on from the start serves its first breakpoint at the face", "[tbm-reserved-loop]")
{
	SourceLoop loop(LoopStart::Long);
	AutoRestoreBackup reserve_paths(_settings_game.pf.reserve_paths, true); // Set before the first tick moves either train.
	TBMFaceLoopWatch watch;
	uint stalled_at_start = 0;
	// Watch the first cut: while it is unfinished the hauler may wait, but its logistics must never stall the cutter.
	while (loop.Job().cursor == 0 && loop.ticks < 40000) {
		TickLoop(loop);
		watch.Observe(loop.ticks, loop.Project(), loop.Cycle(), *loop.cutter, *loop.hauler);
		if (loop.Project().blocked == TBMBlockedReason::LogisticsStall) ++stalled_at_start;
	}
	INFO(DescribeHaulerInputs(loop));
	REQUIRE(loop.Job().cursor >= 1);
	CHECK(stalled_at_start == 0);
	// Then the first exchange of the route must happen; the last few ticks before it explain a stall.
	std::deque<std::string> recent;
	while (loop.Cycle().exchanges == 0 && loop.ticks < 120000) {
		TickLoop(loop);
		watch.Observe(loop.ticks, loop.Project(), loop.Cycle(), *loop.cutter, *loop.hauler);
		recent.push_back(DescribeHaulerInputs(loop));
		if (recent.size() > 6) recent.pop_front();
	}
	std::string last_ticks;
	// Keep only the bounded recent native states for a stalled first-exchange diagnostic.
	for (const auto &line : recent) last_ticks += "\n" + line;
	INFO("before the first exchange:" << last_ticks);
	REQUIRE(loop.Cycle().exchanges != 0);
	// One tick more lets the watch record where that exchange happened.
	TickLoop(loop);
	watch.Observe(loop.ticks, loop.Project(), loop.Cycle(), *loop.cutter, *loop.hauler);
	INFO(watch.Describe());
	const auto &breakpoints = loop.Job().pass_terms->exchange_after;
	REQUIRE_FALSE(breakpoints.empty());
	CHECK(watch.face_transfers == std::vector<uint32_t>{breakpoints.front()});
	CHECK(watch.bay_exchanges.empty());
	CHECK(loop.IsConserved());
	// The rest of the route, still with reservation on: Run fails with its journal if the last load is never delivered.
	loop.Run();
	CheckDelivered(loop);
}
