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
	/** @param other Plane point to compare. @return Whether both saved coordinates agree. */
	bool operator==(const SpacePoint &other) const = default;
};

/** Authoritative per-axis magnitude/day and saved unsigned fractional travel. */
struct SpaceMotion {
	SpacePoint position;
	int32_t velocity_x = 0, velocity_y = 0;
	uint8_t remainder_x = 0, remainder_y = 0;
	/** @param other Motion to compare. @return Whether position, velocity and fractional travel agree. */
	bool operator==(const SpaceMotion &other) const = default;
};

/** Inclusive continuous reflection bounds joining admissible tile centers. */
struct SpaceTravelBand {
	SpacePoint min, max;
	/** @param point Plane point to validate. @return Whether both axes lie inside the inclusive reflection bounds. */
	bool Contains(SpacePoint point) const;
	/** @param other Band to compare. @return Whether the inclusive bounds agree. */
	bool operator==(const SpaceTravelBand &other) const = default;
};

/** @param width Plane width in supported tiles. @param height Plane height in supported tiles. @return Shared middle-half envelope bounds, or an empty band for unsupported dimensions. */
SpaceTravelBand GetSpaceTravelBand(uint32_t width, uint32_t height);
/** @param motion Validated in-band motion with bounded per-day velocity and remainders. @param band Shared inclusive bounds wider than a single step. */
void AdvanceSpaceMotion(SpaceMotion &motion, const SpaceTravelBand &band);
/** @param a First bounded plane point. @param b Second bounded plane point. @return Exact squared Q16 distance without floating point. */
uint64_t SpaceDistanceSquared(SpacePoint a, SpacePoint b);
/** @param position Actual craft/preview point to advance. @param target Current authoritative destination. @param remainder Saved unsigned fraction of the four-tile/day speed. @return Whether this step reaches the destination. */
bool MoveSpaceSurveyTowards(SpacePoint &position, SpacePoint target, uint8_t &remainder);

/** Ownerless observation phases; these confer no native vehicle, resource or cargo rights. */
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
	/** @param other Preview to compare. @return Whether all saved observation fields agree. */
	bool operator==(const SpaceSurveyPreview &other) const = default;
};

/** Derived local-query index. Save records and stable-ID iteration own simulation. */
class SpaceSpatialIndex {
	uint32_t columns = 0, rows = 0;
	std::vector<std::vector<OrbitalBodyID>> buckets;
	std::vector<uint32_t> locations;
	std::vector<uint32_t> slots;
public:
	/** @param width Plane width in tiles. @param height Plane height in tiles. @param count Number of monotonic body identities to index. */
	void Reset(uint32_t width, uint32_t height, uint32_t count);
	/** @param id Valid monotonic body identity. @param position Current bounded plane point published by body motion. */
	void Update(OrbitalBodyID id, SpacePoint position);
	/** @param min Minimum query coordinate. @param max Maximum query coordinate. @return Stable IDs in intersecting buckets; callers check their precise geometry. */
	std::vector<OrbitalBodyID> Query(SpacePoint min, SpacePoint max) const;
};

#endif /* SPACE_H */
