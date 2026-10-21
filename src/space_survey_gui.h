/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file space_survey_gui.h Read-only native orbital inspector selection.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JDB
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md task16.2
 */
#ifndef SPACE_SURVEY_GUI_H
#define SPACE_SURVEY_GUI_H

/** Existing paid native hull incarnation, never an inspector-owned second identity. */
struct SpaceCraftRef;

/** Read the current surviving native orbital inspector selection.
 * @return The actual deployed hull selected by the existing inspector, or an empty retired/absent reference.
 */
SpaceCraftRef GetSpaceSurveySelection();

#endif /* SPACE_SURVEY_GUI_H */
