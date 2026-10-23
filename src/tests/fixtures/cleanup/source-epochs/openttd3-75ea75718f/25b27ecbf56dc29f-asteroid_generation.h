/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_generation.h Saved starter inputs and bounded initial-access queries owned by the asteroid field.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
 */
#ifndef ASTEROID_GENERATION_H
#define ASTEROID_GENERATION_H
#include "space.h"
#include <array>
#include <optional>

/** Explicit historical input provenance; absence of a legal site never becomes a fabricated anchor. */
enum class AsteroidStarterKind : uint8_t {
	Legacy, ///< Older fields have no recorded initial site; loading must not reconstruct their history.
	PureField, ///< Explicit geometry-only generation without inspecting Surface terrain.
	NoSite, ///< Actual generation searched Surface terrain and found no currently legal geometric site.
	Site, ///< Actual generation selected an initial site using the real launch placement geometry producer.
};

/** Saved generator preference and initial approach bound, independent of later motion or Surface construction. */
struct AsteroidStarterInput {
	AsteroidStarterKind kind = AsteroidStarterKind::Legacy; ///< How the generation-time preference was authorized.
	uint32_t site_x = UINT32_MAX, site_y = UINT32_MAX; ///< Surface anchor coordinates; both invalid when no site was recorded.
	uint8_t rotation = UINT8_MAX; ///< Original quarter-turn layout; invalid when no site was recorded.
	SpacePoint preference; ///< Actual projected generation preference, with no camera or world-Y offset.
	uint64_t initial_approach_distance_squared = 0; ///< Maximum of the three basic-probe approach distances in squared Q16 tiles.
	/** @param other Saved input to compare. @return Whether every authoritative generation input and bound matches. */
	bool operator==(const AsteroidStarterInput &other) const = default;
};

/** Derived closest grid triple; these points do not constitute a second persistent body or cargo owner. */
struct AsteroidStarterTriple {
	std::array<SpacePoint, 3> positions; ///< The three stable starter-class centres in generator selection order.
	uint64_t approach_distance_squared = 0; ///< Actual maximum approach distance, including the canonical probe offset.
};

/** Disposable generation-work counters used by bounded producer acceptance, never by gameplay decisions. */
struct AsteroidStarterSearchWork {
	uint64_t anchors = 0; ///< Surface anchors visited before finding a preferred site or exhausting the plane.
	uint64_t geometry_queries = 0; ///< Complete native site queries; at most two per visited anchor.
	uint64_t grid_candidates = 0; ///< Constant-size local grid points examined for valid geometric sites.
};

/**
 * Find the exact generator's first three nearest grid points without scanning the entire grid.
 * @param width Supported Surface-plane width in tiles.
 * @param height Supported Surface-plane height in tiles.
 * @param preference Explicit projected point used to choose initial body centres.
 * @param candidates_examined Optional exact count of local points examined, reset before calculation.
 * @return Nearest triple and actual basic-probe approach bound, or none for unsupported geometry.
 */
std::optional<AsteroidStarterTriple> GetAsteroidStarterTriple(uint32_t width, uint32_t height, SpacePoint preference, uint32_t *candidates_examined = nullptr);

/**
 * Publish the upward-rounded historical maximum approach distance without floating-point rounding.
 * @param input Saved initial starter input; later body motion does not change this value.
 * @return Maximum Q16 distance, or none for legacy fields with no recorded initial evidence.
 */
std::optional<uint32_t> GetAsteroidInitialApproachDistance(const AsteroidStarterInput &input);

/**
 * Search actual Surface geometry before companies exist, preferring any feasible32-tile starter triple.
 * This query neither changes terrain nor grants paid construction permission.
 * @param width Actual supported Surface-plane width.
 * @param height Actual supported Surface-plane height.
 * @param work Optional disposable counters, reset before the search.
 * @return Saved Site/NoSite input, or none when map dimensions do not support the requested field.
 */
std::optional<AsteroidStarterInput> FindWorldAsteroidStarterInput(uint32_t width, uint32_t height, AsteroidStarterSearchWork *work = nullptr);

#endif /* ASTEROID_GENERATION_H */
