/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file space.h Shared fixed-point orbital geometry and explicitly staged survey preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups9/13/26; terminal geometry, live-only spatial membership and ownerless departure.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
 */
#ifndef SPACE_H
#define SPACE_H

#include "core/strong_typedef_type.hpp"
#include "core/enum_type.hpp"
#include <cstdint>
#include <optional>
#include <vector>
#include <unordered_map>

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
/** Inclusive physical plane-centre bounds from (0,0), independent of initial placement and static construction policy. */
struct SpacePlaneBounds {
	SpacePoint max{-1, -1}; ///< Last legal tile centre; the default is invalid.
	/** @return Whether these are exactly the last centres of a supported physical plane. */
	bool IsValid() const;
	/** @param point Proposed physical centre. @return Whether both axes remain on the plane. */
	bool Contains(SpacePoint point) const;
	/** @param other Physical bounds to compare. @return Whether both maximum centres agree. */
	bool operator==(const SpacePlaneBounds &other) const = default;
};
/** Physical edges crossed by an outward step; a corner may cross two edges simultaneously. */
enum class SpacePlaneEdge : uint8_t { West, East, North, South, End };
using SpacePlaneEdges = EnumBitSet<SpacePlaneEdge, uint8_t>;
/** Pure proposal only; an exiting position must never be published into a live body or attached hull. */
struct SpaceStraightMotionStep {
	SpaceMotion proposed; ///< Signed velocity is unchanged, including on an exiting proposal.
	SpacePlaneEdges crossed_edges; ///< Empty while the proposed centre remains inside the inclusive plane.
};
/** Analytic future boundary using the exact same fractional travel as one-step proposals. */
struct SpaceMotionExitForecast {
	std::optional<uint64_t> ticks_until_exit; ///< First outside opportunity, or absence for stationary motion.
	SpaceMotion last_safe_motion; ///< State one tick before exit; original motion if stationary.
	SpacePlaneEdges crossed_edges; ///< Edges crossed on the first outside opportunity.
};
/** @param width Supported plane width in tiles. @param height Supported plane height in tiles. @return Physical last-centre bounds, or absence for unsupported dimensions. */
std::optional<SpacePlaneBounds> GetSpacePlaneBounds(uint32_t width, uint32_t height);
/**
 * Propose exactly one straight simulation step without changing any owner or reversing at an edge.
 * @param motion Original in-plane coordinates, signed per-day components and canonical fractional remainders.
 * @param bounds Explicit physical plane bounds; generation's middle-half band is not a terminal boundary.
 * @return Proposed motion and crossed edges, or absence for invalid bounds, position or remainders.
 */
std::optional<SpaceStraightMotionStep> StepSpaceMotionStraight(const SpaceMotion &motion, const SpacePlaneBounds &bounds);
/** @param motion Valid in-plane motion to forecast. @param bounds Explicit physical plane bounds. @return Exact exit/last-safe state, stationary no-exit result, or absence for invalid input. */
std::optional<SpaceMotionExitForecast> ForecastSpaceMotionExit(const SpaceMotion &motion, const SpacePlaneBounds &bounds);
/** @param motion Original in-plane motion. @param bounds Physical plane. @param ticks Future opportunities, at most the first outside tick; stationary motion permits any horizon. @return Exact bounded proposal using the shared carry arithmetic, or absence for invalid input/excess horizon. */
std::optional<SpaceStraightMotionStep> ProjectSpaceMotionStraight(const SpaceMotion &motion, const SpacePlaneBounds &bounds, uint64_t ticks);
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
	OrbitalBodyID target{0}; ///< Actual live target for Approach/Attached; explicit invalid sentinel is permitted only during released Return/Waiting from source445.
	SpaceSurveyPhase phase = SpaceSurveyPhase::Approach;
	uint16_t remaining_ticks = 0;
	uint8_t travel_remainder = 0;
	uint32_t completed_surveys = 0;
	/** @param other Preview to compare. @return Whether all saved observation fields agree. */
	bool operator==(const SpaceSurveyPreview &other) const = default;
};

/** Derived local-query index. Save records and stable-ID iteration own simulation. */
class SpaceSpatialIndex {
	/** One live body's bucket and position within that bucket; never a resource owner. */
	struct Membership { uint32_t bucket = 0, slot = 0; };
	uint32_t columns = 0, rows = 0;
	SpacePlaneBounds bounds;
	std::vector<std::vector<OrbitalBodyID>> buckets;
	std::unordered_map<uint32_t, Membership> membership;
	/** @param entry Original membership to unlink; caller subsequently replaces or erases its map row. Updates the moved bucket occupant in constant work. */
	void Unlink(Membership entry);
public:
	/** Read-only container occupancy/capacity, excluding allocator-specific node overhead; not a gameplay or saved owner. */
	struct StorageUsage {
		size_t membership_rows = 0, membership_buckets = 0, bucket_headers = 0;
		size_t live_slots = 0, retained_slots = 0, empty_retained_slots = 0;
	};
	/** @return Actual live rows, fixed geometric bucket headers and retained slot capacity for explicit diagnostics; performs no allocation or gameplay mutation. */
	StorageUsage InspectStorage() const;
	/** @param width Plane width in tiles, or zero for an empty owner. @param height Plane height in tiles. @param count Bounded live-capacity hint, never the highest lifetime ID. Invalid dimensions reset to an empty index. */
	void Reset(uint32_t width, uint32_t height, uint32_t count);
	/** @param id Fresh live identity absent from this index. @param position Actual physical-plane point. @return Whether a bounded new membership was added; duplicate/invalid/allocation refusal leaves membership unchanged. */
	bool Insert(OrbitalBodyID id, SpacePoint position);
	/** @param id Original live identity. @return Whether it was unlinked without touching field records, credit, leases or packets. */
	bool Remove(OrbitalBodyID id);
	/** @param id Valid live identity. @param position Current bounded plane point published by body motion. @return Whether membership was moved or inserted; invalid/allocation refusal preserves the old membership. */
	bool Update(OrbitalBodyID id, SpacePoint position);
	/** @return Number of current live memberships, independent of lifetime issuance. */
	size_t Size() const { return this->membership.size(); }
	/** @param min Minimum query coordinate. @param max Maximum query coordinate. @return Stable IDs in intersecting buckets; callers check their precise geometry. */
	std::vector<OrbitalBodyID> Query(SpacePoint min, SpacePoint max) const;
};

#endif /* SPACE_H */
