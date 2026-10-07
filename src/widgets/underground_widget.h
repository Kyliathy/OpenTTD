/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_widget.h Underground construction controls. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md */

#ifndef WIDGETS_UNDERGROUND_WIDGET_H
#define WIDGETS_UNDERGROUND_WIDGET_H

/** Widgets of the underground construction window; feature commands retain all mutation authority. */
enum UndergroundConstructionWidgets : WidgetID {
	WID_UC_CAPTION, ///< Names the explicitly requested layer.
	WID_UC_REQUIREMENTS, ///< Calendar and whole-footprint construction requirements.
	WID_UC_SURVEY, ///< Inspect one world tile's public geological planning information.
	WID_UC_PROJECT, ///< Select an actual portal for its commissioning/project controls.
	WID_UC_MINE, ///< Open the native industry's funding controls.
	WID_UC_EXCHANGE, ///< Select a real project, then place its exchange at a validated bay.
	WID_UC_RAIL, ///< Open conventional rail construction.
	WID_UC_ROAD, ///< Open conventional road construction.
	WID_UC_RAIL_STATION, ///< Open the ordinary railway station picker.
	WID_UC_ROAD_TERMINAL, ///< Open the ordinary freight road-stop picker.
	WID_UC_RAIL_SERVICE, ///< Open the ordinary rail service-depot picker.
	WID_UC_ROAD_SERVICE, ///< Open the ordinary road service-depot picker.
	WID_UC_STATUS, ///< Selected tile information or a visible placement refusal.
	WID_UC_AREA, ///< Select a real portal, then drag a requested excavation floor.
	WID_UC_AREA_CONFIRM, ///< Submit the owning commissioning command after a complete current quote.
	WID_UC_AREA_PRICE, ///< Separate purchase and future geological-work charges.
	WID_UC_AREA_MATERIAL, ///< Canonical Earth, Rock, support and work requirements.
	WID_UC_AREA_FLOOR, ///< Final floor and actual additional swept work space.
	WID_UC_MODEL_DATES, ///< Compatible cutter names and introduction years from the canonical catalogue.
	WID_UC_JOB_PROGRESS, ///< Live selected job cut/partial-work progress; geology retains its sole cursor.
};

#endif /* WIDGETS_UNDERGROUND_WIDGET_H */
