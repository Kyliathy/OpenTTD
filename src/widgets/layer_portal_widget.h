/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_portal_widget.h Existing portal/project controls shared with functional workflow checks. @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md and r2mlvs7-underground-construction-and-fleet.md, Group 9B */

#ifndef WIDGETS_LAYER_PORTAL_WIDGET_H
#define WIDGETS_LAYER_PORTAL_WIDGET_H

/** Native pair window widgets; status always queries canonical saved authority. */
enum PortalWidgets : WidgetID {
	PW_STATUS, ///< Pair state line.
	PW_CONNECT, ///< Connect the pair for ordinary trains.
	PW_SEAL, ///< Seal it again.
	PW_COMMISSION, ///< Buy and commission a boring machine.
	PW_CORRIDOR_DOWN, ///< Shorter corridor to commission.
	PW_CORRIDOR, ///< Corridor length to commission.
	PW_CORRIDOR_UP, ///< Longer corridor to commission.
	PW_INFO, ///< Project lines: progress, hold, spoil dug, hauler and the next step.
	PW_SHOW_MACHINE, ///< Open the machine's vehicle window.
	PW_START_STOP, ///< Start or stop the machine.
	PW_PAUSE, ///< Pause or resume the project.
	PW_CANCEL, ///< Cancel the project, after confirmation.
	PW_RETIRE, ///< Retire the finished project.
	PW_ASSIGN, ///< Pick a train to assign as hauler.
	PW_RELEASE, ///< Release the hauler.
	PW_SELL, ///< Sell the withdrawn train in the portal.
	PW_CONVERT, ///< Convert the released temporary track.
};

#endif /* WIDGETS_LAYER_PORTAL_WIDGET_H */
