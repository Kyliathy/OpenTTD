/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_inspection.h Read-only company-authorized asteroid presentation from original native state.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14
 */
#ifndef ASTEROID_INSPECTION_H
#define ASTEROID_INSPECTION_H
#include "asteroid.h"
#include "cargotype.h"

/** Direction components and speed in thousandths of a plane tile per economy day; display rounding never advances motion. */
struct SpaceTrajectoryDisclosure {
	int32_t x_milli = 0, y_milli = 0;
	uint32_t speed_milli = 0;
	bool x_negative = false, y_negative = false; ///< Retain direction even below the displayed thousandth-of-a-tile precision.
};
/** Copied public composition/motion and optional authorized quantities; owns no body, knowledge receipt or stock. */
struct AsteroidInspectionDisclosure {
	CargoType cargo = INVALID_CARGO;
	SpaceTrajectoryDisclosure trajectory;
	std::optional<AsteroidQuantityDisclosure> quantities;
};
/** @param motion Original authoritative per-day fixed-point vector. @return Bounded integer-only display direction/speed with no gameplay mutation. */
SpaceTrajectoryDisclosure GetSpaceTrajectoryDisclosure(const SpaceMotion &motion);
/** @param company Actual observing company; only its completed native survey can reveal quantities.
 * @param body Original monotonic body identity. @return Read-only presentation, or no body after an invalid selection; unknown companies receive no quantities.
 */
std::optional<AsteroidInspectionDisclosure> GetAsteroidInspectionDisclosure(CompanyID company, OrbitalBodyID body);
#endif
