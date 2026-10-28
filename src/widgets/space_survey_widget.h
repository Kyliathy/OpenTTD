/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_survey_widget.h Stable native asteroid/fleet inspector actions for synchronized order controls and actual input checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group30.
 */
#ifndef WIDGETS_SPACE_SURVEY_WIDGET_H
#define WIDGETS_SPACE_SURVEY_WIDGET_H
#include "../window_type.h"
/** SpaceSurveyWindow identities; existing0-11 retain their original meaning. */
enum SpaceSurveyWidgets : WidgetID {
	WID_SS_DETAILS = 0, WID_SS_MINE = 1, WID_SS_RECALL = 2, WID_SS_SURVEY = 3, WID_SS_VISIT = 4,
	WID_SS_CLEAR = 5, WID_SS_ERASE = 6, WID_SS_UP = 7, WID_SS_DOWN = 8, WID_SS_SCROLLBAR = 9,
	WID_SS_SERVICE = 10, WID_SS_RESIZE = 11,
	WID_SS_POLICY_LEGACY = 12, ///< Existing Mine follows its explicit route/home behavior.
	WID_SS_POLICY_RETURN = 13, ///< Existing Mine returns cargo to an actual nearest compatible receiver.
	WID_SS_POLICY_FULL = 14, ///< Existing Mine seeks the same mineral until its real hold is full.
};
#endif /* WIDGETS_SPACE_SURVEY_WIDGET_H */
