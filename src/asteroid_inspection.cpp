/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_inspection.cpp Pure fixed-point motion/composition disclosure without another inventory or survey authority.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14
 */
#include "stdafx.h"
#include "asteroid_inspection.h"
#include "multilayer_content.h"
#include "safeguards.h"

/** @copydoc GetSpaceTrajectoryDisclosure */
SpaceTrajectoryDisclosure GetSpaceTrajectoryDisclosure(const SpaceMotion &motion)
{
	const int64_t x = motion.velocity_x, y = motion.velocity_y;
	const uint64_t square = static_cast<uint64_t>(x * x) + static_cast<uint64_t>(y * y); uint32_t root = 0;
	// Exactly32 candidate bits measure the original vector without floating-point drift or unbounded search. This rounded display magnitude cannot move a body or grant work.
	for (uint32_t bit = uint32_t{1} << 31; bit != 0; bit >>= 1) {
		const uint64_t candidate = root | bit;
		if (candidate * candidate <= square) root |= bit;
	}
	return {static_cast<int32_t>(x * 1000 / SPACE_Q16), static_cast<int32_t>(y * 1000 / SPACE_Q16), static_cast<uint32_t>(uint64_t{root} * 1000 / SPACE_Q16), x < 0, y < 0};
}

/** @copydoc GetAsteroidInspectionDisclosure */
std::optional<AsteroidInspectionDisclosure> GetAsteroidInspectionDisclosure(CompanyID company, OrbitalBodyID id)
{
	const auto *body = GetMutableAsteroidBody(id); if (body == nullptr) return std::nullopt;
	// Motion/composition are public. Only the original company survey authority may supply amounts; copying raw reserve fields here would reveal unknown deposits.
	return AsteroidInspectionDisclosure{GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo),
		GetSpaceTrajectoryDisclosure(body->motion), GetAsteroidQuantityDisclosure(company, id)};
}
