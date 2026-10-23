/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file newgame_settings.h Normal native next-game settings activation boundary.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Group19C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * The implementation remains the existing single owner in openttd.cpp.
 */
#ifndef NEWGAME_SETTINGS_H
#define NEWGAME_SETTINGS_H

/**
 * Activate the configured next-game settings before creating a fresh world's owners.
 * This is the existing ordinary application startup operation:release previous
 * script configuration,copy next-game settings and initialize legacy vehicle defaults.
 * Callers must finish next-game configuration first and must not use this boundary
 * to convert an existing paid world or bypass a NewgameOnly/NoNetwork setting guard.
 */
void MakeNewgameSettingsLive();

#endif /* NEWGAME_SETTINGS_H */
