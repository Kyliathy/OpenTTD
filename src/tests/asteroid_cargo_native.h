/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_cargo_native.h Targeted original CAPA load/preview/save observers inside the retained native application harness.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups21-28B.
 */
#ifndef TESTS_ASTEROID_CARGO_NATIVE_H
#define TESTS_ASTEROID_CARGO_NATIVE_H
/** Observe the actual loaded packet owners without running financial/procurement trials.
 * @param source Immutable repository save already loaded by the ordinary application.
 * @param output Invocation-owned repository test-cache directory for one native resave.
 * Optional OTTD_SPACE_FIXES_CARGO_BAD selects a corrupt candidate; OTTD_SPACE_FIXES_CARGO_FULL requests destructive-load rejection in this isolated process, which exits afterward.
 * Optional OTTD_SPACE_FIXES_MINING_POLICY exercises the registered command on an existing Mine before saving; every other original owner and work receipt remains unchanged.
 * Optional OTTD_SPACE_FIXES_LEGACY_DEPARTURE publishes one test-only local departure on an actual leased unordered miner before saving; running/stopped is selected by the actual command, with no body event or movement.
 * Optional OTTD_SPACE_FIXES_SURVEY_DEPARTURE releases one actual saved ownerless attachment into no-target Return as a test-only pre-erasure transport component; the body remains live and no scan is earned.
 * Optional OTTD_SPACE_FIXES_ASTEROID_RETIREMENT projects only body0's declared final-contained geometry, then exercises the actual atomic retirement; original native cargo/work/pose/clock remain unchanged and no normal-loop event is claimed.
 * Optional OTTD_SPACE_FIXES_ASTEROID_CREATION declares one due checkpoint using the original save's positive credit, then commits exactly one actual replacement; no elapsed wait or normal-loop birth is claimed.
 */
void VerifyAsteroidCargoNative(const char *source, const char *output);
/** Check another candidate's source/field/mission families with a distinct actual world open.
 * @param source Immutable source already loaded by the application; it must differ semantically from OTTD_SPACE_FIXES_PREVIEW_CANDIDATE.
 * Optional OTTD_SPACE_FIXES_CARGO_BAD is a corrupt copy of that candidate. Valid-corrupt-valid and a final original-source preview must rebuild complete copied evidence while native owners, clocks and RNG remain unchanged.
 */
void VerifySpaceTransportIsolation(const char *source);
#endif /* TESTS_ASTEROID_CARGO_NATIVE_H */
