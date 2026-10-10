/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_type.h Shared geothermal admission result without a dependency on the public producer facade.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups1/7
 */

#ifndef GEOTHERMAL_TYPE_H
#define GEOTHERMAL_TYPE_H

#include <cstdint>

/** Site/claim failure, with no generated heat, paid construction, cargo debit or world mutation on refusal. */
enum class GeothermalError : uint8_t { None, InvalidOwner, Unavailable, InvalidKind, InvalidVent, Footprint, Endpoint, Occupied, Claimed, Limit, Refused };

#endif /* GEOTHERMAL_TYPE_H */
