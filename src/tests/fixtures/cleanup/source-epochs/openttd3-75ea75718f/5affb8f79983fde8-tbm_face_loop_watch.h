/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_loop_watch.h An observer of a whole native boring loop: where each exchange happened, whether the
 * cutter held still while it waited at its face, backed when it left that wait and stood still while a hauler was in
 * its corridor, and whether the hauler took each load to the origin before coming back for the next.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 6 (tasks 6.3, 6.5 and 6.6)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-a-face-exchange-design.md (rules F2, F3, F6 and F7)
 */

#ifndef TESTS_TBM_FACE_LOOP_WATCH_H
#define TESTS_TBM_FACE_LOOP_WATCH_H

#include "../tbm.h"
#include "../tbm_exchange.h"
#include "../tbm_controls.h"
#include "../train.h"
#include "../excavation.h"
#include <array>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

/** Reads both controllers after every native step of a loop and keeps what the face rules forbid or require. */
class TBMFaceLoopWatch {
public:
	std::vector<std::string> lines; ///< One line per change of either controller's state, for a failed expectation.
	std::vector<uint32_t> face_transfers; ///< Cut cursor at each exchange made where the cutter met its hauler at the face.
	std::vector<uint32_t> bay_exchanges; ///< Cut cursor at each exchange made where the cutter met its hauler in its bay.
	std::vector<uint32_t> bay_returns; ///< Cut cursor at each return to the bay for an exchange with cuts still to come.
	bool moved_while_waiting = false; ///< The cutter moved while it waited at its face.
	bool kept_heading_on_return = false; ///< A return from the face wait began without the cutter backing.
	uint32_t obstructed_ticks = 0; ///< Ticks a return waited for a hauler in the corridor (rule F7).
	bool moved_while_obstructed = false; ///< The cutter moved while its return waited for a hauler in its corridor.
	bool unserved_load = false; ///< A hauler took a new load without having taken the last one to the origin.
	bool order_problem = false; ///< Either train reported an order problem while the face cycle ran as designed.

	/**
	 * Read both controllers after one native step.
	 * @param tick Steps run so far.
	 * @param project The loop's project.
	 * @param assignment Its hauler's assignment.
	 * @param cutter Cutter front engine.
	 * @param hauler Hauler front engine.
	 */
	void Observe(uint tick, const TBMProject &project, const TBMExchangeAssignment &assignment, const Train &cutter, const Train &hauler)
	{
		const ExcavationJob *job = GetExcavationJob(project.job);
		const uint32_t cursor = job == nullptr ? 0 : job->cursor;
		const bool route_left = job != nullptr && job->cursor < job->route.size();
		std::vector<std::pair<int, int>> positions;
		for (const Train *part = &cutter; part != nullptr; part = part->Next()) positions.emplace_back(part->x_pos, part->y_pos);
		if (this->previous.has_value()) {
			const TBMPhase before = this->previous->phase;
			// An exchange belongs to where the cutter met its hauler on the step that made it: the face wait, or a return
			// held at the face for a hauler already there (rule F8), or the bay.
			if (assignment.exchanges != this->previous->exchanges) {
				(this->previous->meeting == TBMMeetingKind::Face ? this->face_transfers : this->bay_exchanges).push_back(cursor);
				// The hauler must have taken the previous load to the origin and come back empty before this one.
				if (this->exchanges_seen != 0 && !this->Served()) this->unserved_load = true;
				++this->exchanges_seen;
				this->hauler_phases.clear();
			}
			// A return to the bay from a working phase, with cuts still to come, is a mid-route bay exchange.
			const bool working = before == TBMPhase::Bore || before == TBMPhase::AdvanceSupport || IsTBMAwaitingFaceTransfer(before);
			if (project.phase == TBMPhase::ReturnForExchange && working && route_left) this->bay_returns.push_back(cursor);
			// The cutter holds still at its face while it waits (IsTBMHoldingTrain).
			if (IsTBMAwaitingFaceTransfer(project.phase) && IsTBMAwaitingFaceTransfer(before) && positions != this->previous->positions) this->moved_while_waiting = true;
			// Leaving the wait for the bay backs the consist on that very step (task 6.3).
			if (IsTBMAwaitingFaceTransfer(before) && IsTBMReturning(project.phase) && !cutter.vehicle_flags.Test(VehicleFlag::DrivingBackwards)) this->kept_heading_on_return = true;
			// Rule F7: a return waits, standing, while a hauler of the project is in the corridor.
			if (project.blocked == TBMBlockedReason::Obstruction && IsTBMReturning(project.phase) && IsTBMHaulerInCorridor(project, hauler.index)) {
				++this->obstructed_ticks;
				if (before == project.phase && positions != this->previous->positions) this->moved_while_obstructed = true;
			}
		}
		if (this->hauler_phases.empty() || this->hauler_phases.back() != assignment.phase) this->hauler_phases.push_back(assignment.phase);
		// A healthy face cycle needs no order advice: the waiting cutter, and a hauler held at its meeting point.
		if (IsTBMAwaitingFaceTransfer(project.phase) && GetTBMOrderProblem(cutter) != std::optional<StringID>{INVALID_STRING_ID}) this->order_problem = true;
		if (IsTBMHaulerHeld(hauler) && assignment.wait == TBMHaulerWait::None && GetTBMOrderProblem(hauler) != std::optional<StringID>{INVALID_STRING_ID}) this->order_problem = true;

		State state{project.phase, project.blocked, assignment.phase, assignment.wait, assignment.exchanges, GetTBMMeetingKind(project), cursor, std::move(positions)};
		if (!this->previous.has_value() || std::tie(state.phase, state.blocked, state.hauler, state.wait, state.exchanges, state.meeting, state.cursor) !=
				std::tie(this->previous->phase, this->previous->blocked, this->previous->hauler, this->previous->wait, this->previous->exchanges, this->previous->meeting, this->previous->cursor)) {
			this->lines.push_back(fmt::format("tick {}: project {} hold {} hauler {} wait {} exchanges {} meeting {} cursor {}", tick,
				static_cast<int>(to_underlying(state.phase)), static_cast<int>(to_underlying(state.blocked)), static_cast<int>(to_underlying(state.hauler)),
				static_cast<int>(to_underlying(state.wait)), state.exchanges, static_cast<int>(to_underlying(state.meeting)), state.cursor));
		}
		this->previous = std::move(state);
	}

	/** @return Every recorded line, for a failed expectation. */
	std::string Describe() const
	{
		std::string text;
		for (const auto &line : this->lines) text += line + "\n";
		return text;
	}

private:
	/** Both controllers' states and the cutter's part positions at one step. */
	struct State {
		TBMPhase phase; ///< Project phase.
		TBMBlockedReason blocked; ///< Project hold.
		TBMHaulerPhase hauler; ///< Hauler phase.
		TBMHaulerWait wait; ///< Hauler wait.
		uint32_t exchanges; ///< Hauler's completed exchanges.
		TBMMeetingKind meeting; ///< Where the cutter meets its hauler.
		uint32_t cursor; ///< Completed cuts.
		std::vector<std::pair<int, int>> positions; ///< Cutter part positions.
	};

	std::optional<State> previous; ///< State at the previous step.
	uint32_t exchanges_seen = 0; ///< Exchanges seen so far.
	std::vector<TBMHaulerPhase> hauler_phases; ///< Hauler phases since the last exchange, without repeats.

	/** @return Whether the hauler visited the origin, unloaded there and returned empty since the last exchange. */
	bool Served() const
	{
		static constexpr std::array<TBMHaulerPhase, 3> CYCLE{TBMHaulerPhase::VisitOrigin, TBMHaulerPhase::DisposeOrTransfer, TBMHaulerPhase::ReturnEmpties};
		size_t matched = 0;
		// The three steps must appear in this order among the phases seen; other phases may come between them.
		for (TBMHaulerPhase phase : this->hauler_phases) {
			if (matched < CYCLE.size() && phase == CYCLE[matched]) ++matched;
		}
		return matched == CYCLE.size();
	}
};

#endif /* TESTS_TBM_FACE_LOOP_WATCH_H */
