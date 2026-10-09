/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_widget.h Native project browser actions. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group2 */

#ifndef WIDGETS_TBM_WIDGET_H
#define WIDGETS_TBM_WIDGET_H

/** Actual project selection and native vehicle/portal navigation; these actions allocate no work. */
enum TBMProjectsWidgets : WidgetID {
	WID_TP_CAPTION, ///< Explicit work-layer context.
	WID_TP_LIST, ///< Scrollable actual owned project rows.
	WID_TP_SCROLL, ///< Native scrollbar; project count does not enlarge the screen.
	WID_TP_SELECT, ///< Inspect the selected project's current geological footprint.
	WID_TP_CENTRE, ///< Centre on the actual work face or original anchor.
	WID_TP_VEHICLE, ///< Open the same real native cutter vehicle.
	WID_TP_CONTROLS, ///< Open the existing real commissioning/retirement controls.
	WID_TP_EXTEND, ///< Pause through the ordinary command, then plan additional work for this same cutter.
	WID_TP_PARK_DESTINATION, ///< Actual current original hangar/exchange location, independent of camera layer.
	WID_TP_PARK, ///< Withdraw, settle the load and park the whole native machine without cancelling work.
	WID_TP_RESUME, ///< Resume a paused or physically parked project through the controller command.
	WID_TP_RELEASE, ///< Finish/cancel and release only after complete physical withdrawal and real haulage settlement.
	WID_TP_RELEASE_REASON, ///< Actual release prerequisite and blocking train, derived from the owning preflight.
	WID_TP_HANGAR, ///< Choose another real compatible destination through ordinary native depot orders.
};

#endif /* WIDGETS_TBM_WIDGET_H */
