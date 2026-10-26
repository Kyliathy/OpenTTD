/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_lifecycle_native.h Actual StateGameLoop lifecycle acceptance in a disposable retained application.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups29/30B.
 */
#ifndef TESTS_ASTEROID_LIFECYCLE_NATIVE_H
#define TESTS_ASTEROID_LIFECYCLE_NATIVE_H
struct SpaceWorkSample;
/** Offer actual StateGameLoop calls until precisely one normal field opportunity completes.
 * The caller supplies a running active native world. Ordinary CargoDist pauses
 * preserve the field/clock and are retried within a30-second process-local
 * deadline; no field producer or simulation clock is advanced independently.
 * @param sample Optional caller-owned native work counters; only the actual StateGameLoop call is measured, excluding observer copies/validation/assertions.
 */
void RunSpaceLifecycleGameOpportunity(SpaceWorkSample *sample = nullptr);
/** Check paused/editor/hidden/fast-forward field behavior and original stopped-miner cargo across actual ordinary opportunities.
 * @param output Repository-private artifact directory receiving lifecycle-current.sav after actual exit and renewal. Geometry and due timers are declared checkpoints; elapsed opportunities, safety, retirement, creation and ordinary craft upkeep use StateGameLoop.
 */
void VerifyAsteroidLifecycleNative(const char *output);
#endif /* TESTS_ASTEROID_LIFECYCLE_NATIVE_H */
