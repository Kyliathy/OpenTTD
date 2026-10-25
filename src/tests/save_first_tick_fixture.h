/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_first_tick_fixture.h Shared declaration for one actual original-versus-merged native tick.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 */
#ifndef TESTS_SAVE_FIRST_TICK_FIXTURE_H
#define TESTS_SAVE_FIRST_TICK_FIXTURE_H
#include "../saveload/save_lineage.h"
#include <filesystem>
#include <optional>

/** Compare actual original-first versus merged-reload-first native ticks from the same independently identified input.
 * @param source Immutable component original, explicitly loaded again before capturing its starting state.
 * @param origin Optional exact-digest original interpretation; never alters the file's version or bytes.
 * @param output Invocation-owned test-cache prefix for before/direct/reloaded441saves.
 * Both branches advance exactly one actual native tick, allowing callbacks paused
 * by background link-graph work; the external producer then compares every
 * independently required owner byte. Native observations also compare simulation
 * RNG, clock, real financial accounts, packet fragments and canonical domains.
 */
void CheckFirstNativeTick(const char *source, const std::optional<SaveOriginHint> &origin, const std::filesystem::path &output);
#endif /* TESTS_SAVE_FIRST_TICK_FIXTURE_H */
