/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_gui.h Shared native project presentation and explicit selection. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Groups2-3/6, docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Groups 4, 7 and 12, docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 6 */

#ifndef TBM_GUI_H
#define TBM_GUI_H

#include "tbm.h"
#include "tbm_exchange.h"
#include "widget_type.h"

/** Actual native window, retaining normal input/close/viewport ownership. */
struct Window;

/** @param phase Actual saved controller phase. @return Localized short progress description; presentation never advances work. */
StringID GetTBMPhaseString(TBMPhase phase);
/** @param reason Actual non-None work hold. @return Localized recoverable reason, shared with the portal inspector. */
StringID GetTBMHoldString(TBMBlockedReason reason);
/**
 * The project's wait line, shared by the projects list and the portal window (R4, D3): what the machine waits for,
 * or that it works normally. A stored, finished or order-following machine waits for nothing but is not working.
 * @param phase The project's controller phase.
 * @param blocked The project's current hold, None when nothing holds it.
 * @return Localized line, for example "Waiting: the Earth hoppers are full" or "Working normally".
 */
std::string GetTBMWaitLine(TBMPhase phase, TBMBlockedReason blocked);
/**
 * Name what is in the cutter's way, when it can be found (r2df task 7.5).
 * @param project Live project.
 * @return For example "Train 3 is in the way" or "the track or building at 13, 11 is in the way"; nothing when no
 *         train or tile can be named, and the generic hold text then applies.
 */
std::optional<std::string> GetTBMObstructionText(const TBMProject &project);
/**
 * The project's wait line with the blocker named when one can be found; otherwise as GetTBMWaitLine(phase, blocked).
 * @param project Live project.
 * @param blocked The project's current hold.
 * @return Localized line.
 */
std::string GetTBMWaitLine(const TBMProject &project, TBMBlockedReason blocked);
/** @param phase Actual hauler cycle. @return Its shared short localized activity. */
StringID GetTBMHaulerPhaseString(TBMHaulerPhase phase);
/**
 * Name a hauler's activity, saying when it serves the cutter at its face rather than at the bay.
 * @param assignment Actual hauler assignment.
 * @return Its shared short localized activity.
 */
StringID GetTBMHaulerPhaseString(const TBMExchangeAssignment &assignment);
/** @param wait Actual non-None hauler hold. @return Its shared localized recovery reason. */
StringID GetTBMHaulerWaitString(TBMHaulerWait wait);
/**
 * Describe a controller-owned activity using current assignment, phase and health.
 * @param front Actual native front train; no orders or controller fields are changed.
 * @return Localized activity/hold with current speed, or no value when normal order status applies.
 */
std::optional<std::string> GetTBMVehicleStatus(const Train &front);
/** @param layer Explicit project work plane. @return Actual owned-project browser, or nullptr for an invalid context. */
Window *ShowTBMProjectsWindow(MapLayer layer);
/**
 * Show actual owned native storage destinations and current return/release actions.
 * @param window Native owner of the dropdown callback.
 * @param button Actual widget anchoring the menu.
 * @param front Real cutter being directed.
 * @param service Whether destination selections request service rather than parking.
 */
void ShowTBMHangarMenu(Window *window, WidgetID button, const Train &front, bool service);
/**
 * Post the synchronized command for a native menu choice; stale targets revalidate at execution.
 * @param front Actual cutter selected by the owning window.
 * @param selection Transient menu action or explicit world tile; never a DepotID cast.
 * @param service Service/park policy shown by the menu.
 */
void SelectTBMHangarMenuItem(const Train &front, int selection, bool service);
/** @param project Actual owned project identity to inspect. Select its canonical footprint in the real construction palette without changing its work, orders or terrain. */
void InspectUndergroundProject(TBMProjectID project);

/** @param project Actual owned unfinished controller. Arm additional-area input for its existing machine; command admission requires it to be paused. */
void PlanUndergroundProjectExtension(TBMProjectID project);

/**
 * Whether the local company may plan more work for a project (the Pause and extend flow): its own live machine
 * with unfinished work that is neither finishing nor returning to park.
 * @param project Project to check.
 * @return Whether Pause and extend and the New-area shortcut accept it.
 */
bool CanPlanTBMProjectExtension(const TBMProject &project);

/**
 * Whether the local company may change a project's work area (r2df Group 12): its own live machine, work not finished
 * or being released, and at least one completed cut to keep. Working, paused, returning and parked projects qualify.
 * @param project Project to check.
 * @return Whether Change work area and the New-area shortcut accept it.
 */
bool CanChangeTBMWork(const TBMProject &project);

/**
 * Arm new-area input that replaces an owned project's unfinished remainder, without pausing it (r2df task 12.3).
 * @param project Project whose work area changes; a foreign or missing project is ignored.
 */
void PlanUndergroundProjectWorkChange(TBMProjectID project);

/**
 * The projects list's cargo lines (R4): what the cutter's attached hoppers carry, then one line per assigned hauler,
 * or a line saying no hauler is assigned.
 * @param project Listed project.
 * @return Localized lines, for example "Carrying: Earth 64/64 t, Rock 124/128 t".
 */
std::vector<std::string> GetTBMProjectCargoLines(const TBMProject &project);

/**
 * The projects list's release line (R6): the requirement of Finish/cancel and release, prefixed "To finish and
 * release:", naming the vehicle that holds up haulage settlement; or that release is ready.
 * @param id Selected project.
 * @return Localized line.
 */
std::string GetTBMProjectReleaseLine(TBMProjectID id);

/**
 * Pause an owned unfinished project if needed and arm additional-area input for it, as the projects list's
 * Pause and extend button and New area on the project's portal do (R6, task 4.4).
 * @param id Project to extend.
 * @return Whether the extension flow started; false for a foreign, finished, parking or machineless project.
 */
bool PauseAndPlanTBMProjectExtension(TBMProjectID id);

#endif /* TBM_GUI_H */
