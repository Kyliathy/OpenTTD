/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file cheat_func.h Functions related to cheating.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FCA dated same-world acceptance.
 */

#ifndef CHEAT_FUNC_H
#define CHEAT_FUNC_H

/** Open the ordinary single-player cheat controls. */
void ShowCheatWindow();

/**
 * Apply the ordinary calendar-year cheat, retaining its native economic-date policy and engine refresh.
 * @param year Requested calendar year, clamped to the supported native range.
 * @return Actual resulting calendar year; no money, elapsed work, material or energy is granted.
 */
int32_t ChangeCheatCalendarYear(int32_t year);


#endif /* CHEAT_FUNC_H */
