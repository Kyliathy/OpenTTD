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
	WID_UC_PROJECT, ///< Browse actual owned projects and select their vehicle or footprint.
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
	WID_UC_DETAILS, ///< Toggle optional construction requirements without changing the active tool.
	WID_UC_DETAILS_STACK, ///< Requirements/calendar are hidden until requested.
	WID_UC_QUOTE_STACK, ///< Actual quote/material/confirmation rows, hidden before a floor exists.
	WID_UC_PROJECT_STACK, ///< Selected project progress, hidden until a real project is inspected.
	WID_UC_INTENT_STACK, ///< Work intent is shown only while the area/tunnel placement tool owns the cursor.
	WID_UC_INTENT, ///< Choose area coverage or exact straight cutter-centre continuation.
	WID_UC_AREA_TIME, ///< Approximate uninterrupted cutting time at the actual cutter model's work rate.
	WID_UC_QUOTE_COST_STACK, ///< Price, material and time rows, hidden for a refused quote (r2df task 4.5).
	WID_UC_APPROACH_STACK, ///< Approach row, hidden when the quote needs no approach (r2df task 10.2).
	WID_UC_AREA_APPROACH, ///< Approach and turning space beyond floor, ring and housing: cells, cutter positions, price, spoil.
	WID_UC_CHANGE_STACK, ///< Change-work row, shown only for an accepted change of work area (r2df task 12.3).
	WID_UC_AREA_CHANGE, ///< Completed cuts kept, planned cuts replaced, new cuts and any abandoned face work.
	WID_UC_TUBES, ///< Plan17 Group12C: expand dedicated tube controls inside the existing underground window.
	WID_UC_TUBE_STACK, ///< Optional tube controls, hidden until requested.
	WID_UC_TUBE_GUIDEWAY, ///< Native tube rail construction; no excavation or conventional rail admission.
	WID_UC_TUBE_TERMINAL, ///< Existing native station picker with exact tube guideway selected.
	WID_UC_TUBE_DEPOT, ///< Existing native service-depot picker with exact tube guideway selected.
	WID_UC_TUBE_ROUTE, ///< Explicit commissioned portal and adjacent utility path selection.
	WID_UC_TUBE_GEOTHERMAL, ///< Select original completed Deep plant and actual inlet through existing purchased cable.
	WID_UC_TUBE_CONNECT, ///< Confirm the current pure quote through synchronized paid service command.
	WID_UC_TUBE_SOURCE, ///< Explicit existing owned paid inlet choice.
	WID_UC_TUBE_CLOSE, ///< Close the selected inlet through its original settlement owner.
	WID_UC_TUBE_FLEET, ///< Query actual same-layer owned native capsule fronts.
	WID_UC_TUBE_VIEW, ///< Open the selected capsule's native order/refit/vehicle controls.
	WID_UC_TUBE_BIND, ///< Explicit initial binding or source recovery through synchronized owner checks.
	WID_UC_TUBE_MODELS, ///< Canonical five-model calendar/cargo preview, including future introductions.
	WID_UC_TUBE_STATUS, ///< Actual request/refusal and selected capsule's live packets/power wait.
	WID_UC_TUBE_BOUND, ///< Actual bound provider identity, separate from the proposed dropdown selection.
	WID_UC_TUBE_PREVIEW, ///< Selected canonical model's date, capacity and exact cargo labels.
	WID_UC_GEOTHERMAL, ///< Plan18 Group15CB: expand native-backed geothermal controls.
	WID_UC_GEO_STACK, ///< Optional geothermal rows; hiding cancels their placement draft.
	WID_UC_GEO_SURVEY, ///< Inspect actual natural vent/site/machinery without discovering or producing anything.
	WID_UC_GEO_FAMILY, ///< Immutable collector/1980/2000 family selection.
	WID_UC_GEO_VENT, ///< Explicit actual original natural source and public capacity.
	WID_UC_GEO_ACCESS, ///< Explicit blueprint access direction, independent of viewport.
	WID_UC_GEO_PLAN, ///< Arm uncommitted anchor/boundary planning input.
	WID_UC_GEO_CONFIRM, ///< Post synchronized whole cold-site planning after pure admission.
	WID_UC_GEO_SITE, ///< Explicit actual issued active owned plan, never a predicted identity.
	WID_UC_GEO_FUND, ///< Fund native machinery through the original capital/stock/work owner.
	WID_UC_GEO_CANCEL, ///< Cancel only the selected owned active cold reservation.
	WID_UC_GEO_PAGE, ///< Site, physical-utility or town-service presentation page.
	WID_UC_GEO_PAGE_STACK, ///< Site controls; independent zero-size pages prevent hidden rows consuming height.
	WID_UC_GEO_HEAT, ///< Arm paid Deep Heat pipe installation.
	WID_UC_GEO_ELECTRIC, ///< Arm paid explicit-layer cable installation.
	WID_UC_GEO_REMOVE_HEAT, ///< Explicit Heat-cell removal, preserving other domain/track metadata.
	WID_UC_GEO_REMOVE_ELECTRIC, ///< Explicit electricity-cell removal with no capital or energy refund.
	WID_UC_GEO_RISER, ///< Select original portal, machinery corner and boundary port.
	WID_UC_GEO_RISER_CONFIRM, ///< Fund complete current original-portal riser quote.
	WID_UC_GEO_GENERATOR, ///< Actual completed native output machinery choice.
	WID_UC_GEO_FIRST_RISER, ///< Actual owned completed Deep sending riser.
	WID_UC_GEO_SECOND_RISER, ///< Actual Underground sender or explicit direct-to-Surface route.
	WID_UC_GEO_TERMINAL, ///< Actual owned occupied Surface freight terminal.
	WID_UC_GEO_TOWN, ///< Actual native town choice; authority is revalidated by the command.
	WID_UC_GEO_CONNECT, ///< Atomic synchronized first connection or retained-service recovery.
	WID_UC_GEO_ACCESS_STACK, ///< Geometry access is visible only on the site/utility pages.
	WID_UC_GEO_INFORMATION, ///< Bounded native construction, energy or town information view.
	WID_UC_GEO_UTILITIES_STACK, ///< Utility rows disappear completely on another page.
	WID_UC_GEO_TOWN_STACK, ///< Recipient selectors disappear completely on another page.
	WID_UC_GEO_INFORMATION_STACK, ///< Information selector has only its actual one-row height.
	WID_UC_GEO_STATUS, ///< Native-backed public survey, planning/refusal and funding information.
};

#endif /* WIDGETS_UNDERGROUND_WIDGET_H */
