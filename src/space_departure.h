/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_departure.h Native mining departure through the original shared occupancy policy.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group11
 */
#ifndef SPACE_DEPARTURE_H
#define SPACE_DEPARTURE_H
#include "space.h"
#include <optional>

struct SpaceVehicle;

/** Select a clear native mining departure destination without moving or reserving any asset.
 * @param craft Actual returning hull retaining its original released body target and cargo.
 * @param onward Actual authorized home/return destination used to prefer an exit on its route.
 * @return Current point after clearing the body envelope, a locally clear shared-policy exit, or no point while blocked.
 */
std::optional<SpacePoint> GetSpaceMiningDepartureDestination(const SpaceVehicle &craft, SpacePoint onward);

#endif /* SPACE_DEPARTURE_H */
