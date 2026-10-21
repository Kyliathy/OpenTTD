/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_fleet_paid_review_fixture.h Shared actual paid queue scenario, without duplicate test-owned production.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups62JD/62JDB
 * The unchanged original scenario body pays finite stock/capital and advances
 * normal StateGameLoop opportunities. This interface grants no native state.
 */
#ifndef TESTS_LAUNCH_FLEET_PAID_REVIEW_FIXTURE_H
#define TESTS_LAUNCH_FLEET_PAID_REVIEW_FIXTURE_H
#include "../3rdparty/nlohmann/json.hpp"

/** Actual paid native facility. */
struct Station;
/** Existing repository-local invocation artifact destinations. */
struct TestArtifactPaths;

/** Earn the original real paid assembly/pause/FIFO scenario through native commands.
 * @param center Existing paid center with its own finite station packets and owner money.
 * @param paths Current invocation-owned native work/pause captures.
 * @return Actual paid command, debit and normal work observations; never assigned work or stock.
 */
nlohmann::json EarnPaidLaunchQueue(Station &center, const TestArtifactPaths &paths);

#endif /* TESTS_LAUNCH_FLEET_PAID_REVIEW_FIXTURE_H */
