/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_useful_tick_fixture.h Shared complete native clock/vehicle opportunity for actual useful-route writers and ordinary readers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15BC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_TUBE_USEFUL_TICK_FIXTURE_H
#define TESTS_TUBE_USEFUL_TICK_FIXTURE_H
#include "../timer/timer_manager.h"
#include "../timer/timer_game_calendar.h"
#include "../timer/timer_game_economy.h"
#include "../timer/timer_game_tick.h"
#include "../signal_func.h"
#include "../vehicle_func.h"

/** Advance the same complete native clocks and vehicle loop as the useful-route producer; the caller owns pause and company context. */
inline void RunUsefulTubeNativeOpportunity()
{
	UpdateSignalsInBuffer();
	// Daily processing must visit each actual native pool ID once per day.
	// The complete vehicle loop then owns loading, common power, all movement,
	// cargo aging and the deferred depot restart; no private front driver does.
	if (TimerManager<TimerGameCalendar>::Elapsed({})) RunVehicleCalendarDayProc();
	TimerManager<TimerGameEconomy>::Elapsed({}); TimerManager<TimerGameTick>::Elapsed(1);
	CallVehicleTicks();
}
#endif /* TESTS_TUBE_USEFUL_TICK_FIXTURE_H */
