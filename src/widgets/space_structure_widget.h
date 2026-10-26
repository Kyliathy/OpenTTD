/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure_widget.h Stable native orbital action and presentation identities.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group2.
 */
#ifndef WIDGETS_SPACE_STRUCTURE_WIDGET_H
#define WIDGETS_SPACE_STRUCTURE_WIDGET_H

/** Widgets of SpaceConstructionWindow; original command identities 0-38 remain stable. */
enum SpaceConstructionWidgets : WidgetID {
	WID_SC_DETAILS = 0, ///< Details control.
	WID_SC_PLACE = 1, ///< Place control.
	WID_SC_NEXT_BUILDER = 2, ///< Next builder control.
	WID_SC_QUEUE = 3, ///< Queue control.
	WID_SC_CANCEL = 4, ///< Cancel control.
	WID_SC_COLLECT_STEEL = 5, ///< Collect steel control.
	WID_SC_COLLECT_GOODS = 6, ///< Collect goods control.
	WID_SC_CLEAR = 7, ///< Clear control.
	WID_SC_LOCATE = 8, ///< Locate control.
	WID_SC_VISIT = 9, ///< Visit control.
	WID_SC_SCROLLBAR = 10, ///< Scrollbar control.
	WID_SC_RECIPE = 11, ///< Recipe control.
	WID_SC_ACTIVATE = 12, ///< Activate control.
	WID_SC_LOAD_STEEL = 13, ///< Load steel control.
	WID_SC_LOAD_GOODS = 14, ///< Load goods control.
	WID_SC_UNLOAD = 15, ///< Unload control.
	WID_SC_NEXT_SERVICE = 16, ///< Next service control.
	WID_SC_START_SERVICE = 17, ///< Start service control.
	WID_SC_PAUSE_SERVICE = 18, ///< Pause service control.
	WID_SC_RESUME_SERVICE = 19, ///< Resume service control.
	WID_SC_CANCEL_SERVICE = 20, ///< Cancel service control.
	WID_SC_TOGGLE_PANEL = 21, ///< Toggle panel control.
	WID_SC_COMMISSION = 22, ///< Commission control.
	WID_SC_UNLOAD_METAL = 23, ///< Unload metal control.
	WID_SC_UNLOAD_ELECTRONICS = 24, ///< Unload electronics control.
	WID_SC_LOAD_METAL = 25, ///< Load metal control.
	WID_SC_LOAD_ELECTRONICS = 26, ///< Load electronics control.
	WID_SC_RESERVE = 27, ///< Reserve control.
	WID_SC_CONNECT = 28, ///< Connect control.
	WID_SC_NEXT_BERTH = 29, ///< Next berth control.
	WID_SC_NEXT_SOURCE = 30, ///< Next source control.
	WID_SC_DISCONNECT = 31, ///< Disconnect control.
	WID_SC_REMOVE = 32, ///< Remove control.
	WID_SC_EVACUATE = 33, ///< Evacuate control.
	WID_SC_NEXT_DESTINATION = 34, ///< Next destination control.
	WID_SC_NEXT_MODEL = 35, ///< Next model control.
	WID_SC_NEXT_INPUT_SOURCE = 36, ///< Next input source control.
	WID_SC_QUEUE_MANUFACTURE = 37, ///< Queue manufacture control.
	WID_SC_CANCEL_MANUFACTURE = 38, ///< Cancel manufacture control.
	WID_SC_PAGE = 39, ///< Page control.
	WID_SC_ACTIONS = 40, ///< Actions control.
	WID_SC_SITE = 41, ///< Site control.
	WID_SC_RESIZE = 42, ///< Resize control.
};

/** Compact presentation pages; selecting one grants no gameplay action. */
enum class SpaceConstructionPage : uint8_t { Construction, Inventory, Service, Connections, Manufacture, End };

#endif /* WIDGETS_SPACE_STRUCTURE_WIDGET_H */
