/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file engine_func.h Native Engine definition, availability and reliability ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; actual runtime commissioning publishes new definitions through the native catalogue lifecycle.
 */

#ifndef ENGINE_FUNC_H
#define ENGINE_FUNC_H

#include "engine_type.h"
#include "vehicle_type.h"
#include "company_type.h"
#include "timer/timer_game_calendar.h"

/** Set up the native Engine catalogue from its selected content definitions. */
void SetupEngines();
/** Initialise the existing catalogue after content setup through native availability and reliability owners. */
void StartupEngines();
/** Initialise one genuinely uninitialized definition without resetting initialized retained Engines or consuming simulation randomness.
 * @param engine Actual newly allocated native Engine with complete immutable content fields, or a pre429 Space definition whose native load owner proved its entire startup history was never initialized; never an estimate or an initialized retained definition.
 */
void StartupNewEngine(Engine *engine);
/** Check the current native catalogue for available enabled vehicle definitions. */
void CheckEngines();

/* Original engine data counts and offsets */
/** @param type Native vehicle family. @return Number of original built-in definitions for that family. */
uint8_t GetOriginalEngineCount(VehicleType type);
/** @param type Native vehicle family. @return Its original built-in definition offset. */
uint8_t GetOriginalEngineOffset(VehicleType type);

/** @param engine Existing native definition. @param type Required vehicle family. @param company Actual querying company or deity catalogue query. @return Whether native content/availability permits building it. */
bool IsEngineBuildable(EngineID engine, VehicleType type, CompanyID company);
/** @param engine Existing native definition. @return Whether its actual cargo/refit policy permits refitting. */
bool IsEngineRefittable(EngineID engine);
/** Derive the native ageing cutoff from current content definitions. */
void SetYearEngineAgingStops();
/** @param e Actual native definition to update. @param new_month Whether monthly ageing rules apply. */
void CalcEngineReliability(Engine *e, bool new_month);
/** @param e Actual native definition to initialise. @param aging_ymd Authoritative cutoff-qualified ageing date. @param seed Native deterministic reliability/introduction seed; caller simulation randomness is restored. */
void StartupOneEngine(Engine *e, const TimerGameCalendar::YearMonthDay &aging_ymd, uint32_t seed);

/** @param engine Existing articulated native definition. @return Total declared capacity across its parts. */
uint GetTotalCapacityOfArticulatedParts(EngineID engine);

#endif /* ENGINE_FUNC_H */
