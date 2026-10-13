/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file space.h Shared fixed-point orbital geometry and explicitly staged survey preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
 */
#ifndef SPACE_H
#define SPACE_H

#include "core/strong_typedef_type.hpp"
#include <cstdint>
#include <vector>

using OrbitalBodyID = StrongType::Typedef<uint32_t, struct OrbitalBodyIDTag, StrongType::Compare>;
using SpaceSurveyID = StrongType::Typedef<uint32_t, struct SpaceSurveyIDTag, StrongType::Compare>;
inline constexpr OrbitalBodyID INVALID_ORBITAL_BODY{UINT32_MAX};
inline constexpr SpaceSurveyID INVALID_SPACE_SURVEY{UINT32_MAX};
inline constexpr int32_t SPACE_Q16 = 65536;
inline constexpr uint32_t MAX_ORBITAL_BODIES = 14336;
inline constexpr uint32_t MAX_SPACE_SURVEY_PREVIEWS = 64;
struct SpaceTransition; ///< Actual native rocket transition is defined in space_transition.h.

/** Layer-local tile centers, never stacked world Y or camera coordinates. */
struct SpacePoint {
	int32_t x = 0, y = 0;
	bool operator==(const SpacePoint &) const = default;
};

/** Authoritative per-axis magnitude/day and saved unsigned fractional travel. */
struct SpaceMotion {
	SpacePoint position;
	int32_t velocity_x = 0, velocity_y = 0;
	uint8_t remainder_x = 0, remainder_y = 0;
	bool operator==(const SpaceMotion &) const = default;
};

/** Inclusive continuous reflection bounds joining admissible tile centers. */
struct SpaceTravelBand {
	SpacePoint min, max;
	bool Contains(SpacePoint point) const;
	bool operator==(const SpaceTravelBand &) const = default;
};

SpaceTravelBand GetSpaceTravelBand(uint32_t width, uint32_t height);
void AdvanceSpaceMotion(SpaceMotion &motion, const SpaceTravelBand &band);
uint64_t SpaceDistanceSquared(SpacePoint a, SpacePoint b);
bool MoveSpaceSurveyTowards(SpacePoint &position, SpacePoint target, uint8_t &remainder);

enum class SpaceSurveyPhase : uint8_t { Approach, Attached, Return, Waiting };

/** Explicit demo controller for model15; no Engine/Vehicle/cargo/Surface facility identity.
 * Retire these records explicitly when native paid fleet commissioning is implemented.
 */
struct SpaceSurveyPreview {
	SpaceSurveyID id{0};
	uint16_t model = 15;
	SpacePoint position, home;
	OrbitalBodyID target{0};
	SpaceSurveyPhase phase = SpaceSurveyPhase::Approach;
	uint16_t remaining_ticks = 0;
	uint8_t travel_remainder = 0;
	uint32_t completed_surveys = 0;
	bool operator==(const SpaceSurveyPreview &) const = default;
};

/** Derived local-query index. Save records and stable-ID iteration own simulation. */
class SpaceSpatialIndex {
	uint32_t columns = 0, rows = 0;
	std::vector<std::vector<OrbitalBodyID>> buckets;
	std::vector<uint32_t> locations;
	std::vector<uint32_t> slots;
public:
	void Reset(uint32_t width, uint32_t height, uint32_t count);
	void Update(OrbitalBodyID id, SpacePoint position);
	std::vector<OrbitalBodyID> Query(SpacePoint min, SpacePoint max) const;
};

#endif /* SPACE_H */
