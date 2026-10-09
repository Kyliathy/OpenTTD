/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_gui.h Shared native project presentation and explicit selection. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Groups2-3/6 */

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
/** @param phase Actual hauler cycle. @return Its shared short localized activity. */
StringID GetTBMHaulerPhaseString(TBMHaulerPhase phase);
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

#endif /* TBM_GUI_H */
