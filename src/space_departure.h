/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_departure.h Native mining departure through the original shared occupancy policy.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups10-10C/25B/28B; pure safety, bounded departure and copied exclusion geometry for original contained endpoint occupancy.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group11
 */
#ifndef SPACE_DEPARTURE_H
#define SPACE_DEPARTURE_H
#include "space.h"
#include <optional>
#include <span>
#include <array>
#include <utility>

struct SpaceVehicle;

inline constexpr uint8_t SPACE_STAGING_MAX_RADIUS = 8; ///< Original bounded local staging rings, shared by native departure and copied interception forecasts.
inline constexpr std::array<std::pair<int, int>, 8> SPACE_STAGING_DIRECTIONS{{{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}}}; ///< Original deterministic ring order.
/** @return Existing native body waiting/departure clearance in Q16 units; this is not a sprite radius or new collision owner. */
int32_t GetSpaceBodyApproachClearance();

/** Original static geometry that an attached or departing hull must not occupy. */
enum class SpaceAttachmentObstacleKind : uint8_t { Footprint, Approach, ReservedArea, End }; ///< ReservedArea is a copied exclusion rectangle around an original retained native point, never another saved berth owner.
/** Detached geometry view; it owns no module, berth, hull or reservation. Bounds are inclusive Q16 centres. */
struct SpaceAttachmentObstacle {
	SpacePoint min, max; ///< A footprint/reserved exclusion rectangle, or one exact approach point with min == max.
	SpaceAttachmentObstacleKind kind = SpaceAttachmentObstacleKind::Footprint;
	/** @param other Copied geometry to compare. @return Whether both bounds and their original physical meaning agree. */
	bool operator==(const SpaceAttachmentObstacle &other) const = default;
};
/** First physical reason an attachment cannot survive, independent of mining orders, pause or running state. */
enum class SpaceAttachmentHazard : uint8_t { None, BodyExit, MapEdge, Footprint, Approach };
/** Pure pre-translation safety result; no lease release, movement or cargo operation is committed. */
struct SpaceAttachmentSafety {
	std::optional<uint64_t> first_unsafe_tick; ///< Zero means already unsafe; absence means no future hazard for stationary motion.
	std::optional<SpacePoint> last_safe_point; ///< Point immediately before the hazard, current point if safe forever, absent if already unsafe.
	SpaceAttachmentHazard hazard = SpaceAttachmentHazard::None;
	/** @param other Pure safety result to compare. @return Whether hazard timing and the last-safe point agree. */
	bool operator==(const SpaceAttachmentSafety &other) const = default;
};
/**
 * Test a copied physical point against map bounds and original static footprint/berth geometry.
 * @param point Proposed craft centre, never an asteroid sprite extent.
 * @param bounds Explicit physical plane.
 * @param obstacles Detached original static geometry; malformed geometry blocks admission.
 * @return Whether the point is in bounds and outside every static obstacle; grants no berth.
 */
bool IsSpaceAttachmentPointClear(SpacePoint point, const SpacePlaneBounds &bounds, std::span<const SpaceAttachmentObstacle> obstacles);
/** @param from Original or copied physical start. @param to Proposed end of one straight movement segment. @param bounds Actual physical plane. @param obstacles Original static footprints/berths. @param allowed_start_berth Exact currently admitted source berth, if independently proven by the native caller; only its start-point obstacle is exempt, never a footprint. @return Whether the whole swept segment remains clear; valid endpoints alone cannot authorize crossing a paid owner. */
bool IsSpaceMovementSegmentClear(SpacePoint from, SpacePoint to, const SpacePlaneBounds &bounds, std::span<const SpaceAttachmentObstacle> obstacles, std::optional<SpacePoint> allowed_start_berth = std::nullopt);
/** @param body_position Original current body centre. @param bounds Physical plane. @param obstacles Original static footprint/berth geometry. @return The original (+1,-0.5)-tile body approach only when its centre is safe; no slot, lease or movement is granted. */
std::optional<SpacePoint> GetSafeSpaceBodyApproachPoint(SpacePoint body_position, const SpacePlaneBounds &bounds,
	std::span<const SpaceAttachmentObstacle> obstacles);
/**
 * Forecast the first unsafe attached translation with bounded integer interval arithmetic.
 * @param motion Original body's saved motion, without publication of a future point.
 * @param body_forecast Forecast obtained from that exact motion and plane; mismatched forecasts are refused.
 * @param offset Craft's immutable translation offset relative to the body.
 * @param bounds Explicit physical plane bounds.
 * @param obstacles Original static footprints and actual approach points; no live pool lookup occurs here.
 * @return First hazard and last-safe craft point, or absence for malformed inputs. This query ignores stopped flags and never releases a hull.
 */
std::optional<SpaceAttachmentSafety> ForecastSpaceAttachmentSafety(const SpaceMotion &motion, const SpaceMotionExitForecast &body_forecast,
	SpacePoint offset, const SpacePlaneBounds &bounds, std::span<const SpaceAttachmentObstacle> obstacles);

/** @return Copied original static footprints/actual approach points, or absence for malformed native geometry; this creates no second owner. */
std::optional<std::vector<SpaceAttachmentObstacle>> GetSpaceAttachmentObstacles();
/** Local proposal state; even Ready grants neither movement nor a berth. */
enum class SpaceDepartureStatus : uint8_t { Ready, NoClearPoint, InvalidPosition };
/** Bounded read-only departure proposal; all positions remain under the original native hull owner. */
struct SpaceDeparturePlan {
	SpaceDepartureStatus status = SpaceDepartureStatus::InvalidPosition;
	SpacePoint retained_position; ///< Original current point, retained verbatim while blocked.
	std::optional<SpacePoint> destination; ///< A local clear point, never a published position or reservation.
	uint32_t travel_ticks = 0; ///< Exact native movement opportunities on the admitted local path.
	uint8_t candidates_examined = 0; ///< At most the current point plus eight rings of eight directions.
	std::optional<SpaceAttachmentSafety> attachment; ///< Only the original matching live mining lease supplies attachment timing.
	std::optional<uint64_t> release_after_ticks; ///< Zero means release is due now; absence means no predicted attachment hazard.
	/** @param other Read-only plan to compare. @return Whether geometry and timing agree independently of stopped flags. */
	bool operator==(const SpaceDeparturePlan &other) const = default;
};
/**
 * Count actual local native movement and reject a path crossing static/approach geometry.
 * @param origin Original clear physical craft point.
 * @param destination Proposed local endpoint, at most eight tiles away on either axis.
 * @param remainder Original native unsigned travel carry, unchanged by this query.
 * @param bounds Actual plane bounds.
 * @param obstacles Copied original static geometry; segment checks also protect exact approach points between ticks.
 * @return Exact movement count, or absence for invalid/blocked/out-of-range paths.
 */
std::optional<uint32_t> GetSpaceDepartureTravelTicks(SpacePoint origin, SpacePoint destination, uint8_t remainder,
	const SpacePlaneBounds &bounds, std::span<const SpaceAttachmentObstacle> obstacles);
/** @param safety Actual attachment hazard forecast. @param travel_ticks Admitted local clearance duration, or absence when no route is clear. @return Opportunities before release becomes due; blocked routes release before the next unsafe translation, and no hazard yields absence. */
std::optional<uint64_t> GetSpaceDepartureReleaseDelay(const SpaceAttachmentSafety &safety, std::optional<uint32_t> travel_ticks);
/** @param craft Original departing hull, ignored as its own physical occupant. @param point Retained local endpoint. @return Whether current original craft, contained departure points, body envelopes and static geometry still permit it; this grants no movement or berth. */
bool IsSpaceDeparturePointAvailable(const SpaceVehicle &craft, SpacePoint point);
/** @param craft Original actual native hull, including its current position/carry and possible matching mining lease. @param onward Optional authorized next destination used only to rank clear candidates. @return Read-only bounded route/deadline, or an explicit hold/invalid result; stopped flags never waive attachment safety. */
SpaceDeparturePlan PlanSpaceDeparture(const SpaceVehicle &craft, std::optional<SpacePoint> onward = std::nullopt);

/** Select a clear native mining departure destination without moving or reserving any asset.
 * @param craft Actual returning hull retaining its original released body target and cargo.
 * @param onward Actual authorized home/return destination used to prefer an exit on its route.
 * @return Current point after clearing the body envelope, a locally clear shared-policy exit, or no point while blocked.
 */
std::optional<SpacePoint> GetSpaceMiningDepartureDestination(const SpaceVehicle &craft, SpacePoint onward);

#endif /* SPACE_DEPARTURE_H */
