/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file space.cpp Shared integer Space geometry; no camera or simulation RNG.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups9/13; pure straight motion and live-only spatial membership.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group21; opt-in operation diagnostics never govern simulation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8; model15 owns the existing four-tile/day movement definition.
 */
#include "stdafx.h"
#include "space_work_cost.h"
#include "space.h"
#include "multilayer_content.h"
#include <algorithm>
#include "safeguards.h"

/** @param p Plane point to check. @return Whether the full body-centre point lies inside both inclusive axes. */
bool SpaceTravelBand::Contains(SpacePoint p) const
{
	// Both axes must fit the envelope; a valid X alone cannot keep an attached craft out of static construction space.
	return p.x >= this->min.x && p.y >= this->min.y && p.x <= this->max.x && p.y <= this->max.y;
}

/** @param width Plane width in tiles. @param height Plane height in tiles. @return Shared centre envelope, or empty bounds for unsupported geometry. */
SpaceTravelBand GetSpaceTravelBand(uint32_t width, uint32_t height)
{
	// Refuse unsupported world dimensions instead of fabricating a smaller drift field.
	if (width < 64 || height < 64 || width > 4096 || height > 4096) return {{0, 0}, {-1, -1}};
	const auto &tuning = GetMultiLayerSpaceTuning();
	const int32_t margin = tuning.body_center_margin_tiles;
	return {{margin * SPACE_Q16, static_cast<int32_t>(height * tuning.travel_start_quarters / 4 + margin) * SPACE_Q16},
		{static_cast<int32_t>(width - margin - 1) * SPACE_Q16,
			static_cast<int32_t>(height * tuning.travel_end_quarters / 4 - margin - 1) * SPACE_Q16}};
}

/** @copydoc SpacePlaneBounds::Contains */
bool SpacePlaneBounds::Contains(SpacePoint point) const
{
	// The full physical centre must remain inside the map; sprite dimensions and
	// the old spawn/construction strip cannot grant an off-map attachment point.
	return point.x >= 0 && point.y >= 0 && point.x <= this->max.x && point.y <= this->max.y;
}

/** @copydoc GetSpacePlaneBounds */
std::optional<SpacePlaneBounds> GetSpacePlaneBounds(uint32_t width, uint32_t height)
{
	// A forecast must describe an actual supported native plane, including narrow
	// rectangles, rather than silently clamping unsupported dimensions.
	if (width < 64 || height < 64 || width > 4096 || height > 4096 || !std::has_single_bit(width) || !std::has_single_bit(height)) return std::nullopt;
	return SpacePlaneBounds{{static_cast<int32_t>(width - 1) * SPACE_Q16, static_cast<int32_t>(height - 1) * SPACE_Q16}};
}

/** @copydoc SpacePlaneBounds::IsValid */
bool SpacePlaneBounds::IsValid() const
{
	// Refuse malformed bounds before converting signed coordinates to dimensions;
	// a preview query must never hide invalid saved motion behind a clipped point.
	if (this->max.x < 0 || this->max.y < 0 || this->max.x % SPACE_Q16 != 0 || this->max.y % SPACE_Q16 != 0) return false;
	const auto canonical = GetSpacePlaneBounds(this->max.x / SPACE_Q16 + 1, this->max.y / SPACE_Q16 + 1);
	return canonical.has_value() && *canonical == *this;
}

/** @param motion Original proposed body/craft motion. @param bounds Explicit physical bounds. @return Whether straight arithmetic has supported geometry and canonical carry. */
static bool IsStraightMotionInputValid(const SpaceMotion &motion, const SpacePlaneBounds &bounds)
{
	const auto day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	return bounds.IsValid() && bounds.Contains(motion.position) && motion.remainder_x < day && motion.remainder_y < day;
}

/**
 * Preserve exact signed integer travel through one or several known-safe opportunities.
 * @param position Original coordinate, then the proposed coordinate; callers bound the evaluated horizon.
 * @param velocity Signed per-day component, unchanged even for INT32_MIN.
 * @param remainder Original unsigned fractional carry, then the resulting canonical carry.
 * @param ticks One step, or at most the first exit horizon proved by the axis forecast.
 */
static void AdvanceStraightAxis(int32_t &position, int32_t velocity, uint8_t &remainder, uint64_t ticks)
{
	const auto day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	const uint64_t magnitude = static_cast<uint64_t>(std::abs(int64_t{velocity}));
	// At/before first exit, each moving axis travels at most one map span plus
	// one step. Thus its numerator is bounded well below 64-bit range even when
	// a one-Q16-unit/day component needs more than UINT32_MAX opportunities.
	assert(magnitude == 0 || ticks <= (UINT64_MAX - remainder) / magnitude);
	const uint64_t numerator = magnitude * ticks + remainder;
	const int64_t step = static_cast<int64_t>(numerator / day);
	const int64_t candidate = int64_t{position} + (velocity < 0 ? -step : step);
	assert(candidate >= INT32_MIN && candidate <= INT32_MAX);
	position = static_cast<int32_t>(candidate);
	remainder = static_cast<uint8_t>(numerator % day);
}

/** @param point Proposed next centre, possibly outside. @param bounds Inclusive physical centre rectangle. @return Exact crossed edges, including simultaneous corners. */
static SpacePlaneEdges CrossedSpacePlaneEdges(SpacePoint point, const SpacePlaneBounds &bounds)
{
	SpacePlaneEdges edges;
	if (point.x < 0) edges.Set(SpacePlaneEdge::West);
	if (point.x > bounds.max.x) edges.Set(SpacePlaneEdge::East);
	if (point.y < 0) edges.Set(SpacePlaneEdge::North);
	if (point.y > bounds.max.y) edges.Set(SpacePlaneEdge::South);
	return edges;
}

/** @copydoc StepSpaceMotionStraight */
std::optional<SpaceStraightMotionStep> StepSpaceMotionStraight(const SpaceMotion &motion, const SpacePlaneBounds &bounds)
{
	if (!IsStraightMotionInputValid(motion, bounds)) return std::nullopt;
	auto proposed = motion;
	AdvanceStraightAxis(proposed.position.x, proposed.velocity_x, proposed.remainder_x, 1);
	AdvanceStraightAxis(proposed.position.y, proposed.velocity_y, proposed.remainder_y, 1);
	return SpaceStraightMotionStep{proposed, CrossedSpacePlaneEdges(proposed.position, bounds)};
}

/** @param position Original valid coordinate. @param velocity Signed per-day component. @param remainder Canonical carry. @param max Inclusive last centre. @return First outside tick, or UINT64_MAX for a stationary axis. */
static uint64_t StraightAxisExitTick(int32_t position, int32_t velocity, uint8_t remainder, int32_t max)
{
	if (velocity == 0) return UINT64_MAX;
	const uint64_t magnitude = static_cast<uint64_t>(std::abs(int64_t{velocity}));
	const uint64_t distance = velocity < 0 ? position : max - position;
	const uint64_t numerator = (distance + 1) * GetMultiLayerSpaceTuning().ticks_per_economy_day - remainder;
	// Crossing requires one Q16 coordinate unit beyond the inclusive last centre.
	// Quotient plus nonzero remainder is ceiling division without addition overflow.
	return numerator / magnitude + (numerator % magnitude != 0);
}

/** @copydoc ForecastSpaceMotionExit */
std::optional<SpaceMotionExitForecast> ForecastSpaceMotionExit(const SpaceMotion &motion, const SpacePlaneBounds &bounds)
{
	if (!IsStraightMotionInputValid(motion, bounds)) return std::nullopt;
	const uint64_t tick = std::min(StraightAxisExitTick(motion.position.x, motion.velocity_x, motion.remainder_x, bounds.max.x),
		StraightAxisExitTick(motion.position.y, motion.velocity_y, motion.remainder_y, bounds.max.y));
	if (tick == UINT64_MAX) return SpaceMotionExitForecast{std::nullopt, motion, {}};
	auto last = motion;
	AdvanceStraightAxis(last.position.x, last.velocity_x, last.remainder_x, tick - 1);
	AdvanceStraightAxis(last.position.y, last.velocity_y, last.remainder_y, tick - 1);
	const auto outside = StepSpaceMotionStraight(last, bounds);
	assert(outside.has_value() && outside->crossed_edges.Any());
	return SpaceMotionExitForecast{tick, last, outside->crossed_edges};
}

/** @copydoc ProjectSpaceMotionStraight */
std::optional<SpaceStraightMotionStep> ProjectSpaceMotionStraight(const SpaceMotion &motion, const SpacePlaneBounds &bounds, uint64_t ticks)
{
	const auto forecast = ForecastSpaceMotionExit(motion, bounds);
	if (!forecast) return std::nullopt;
	// Only a bounded in-plane horizon or its first exit can be projected. This
	// guards the shared multiplication instead of overflowing a far-future point.
	if (forecast->ticks_until_exit && ticks > *forecast->ticks_until_exit) return std::nullopt;
	auto proposed = motion;
	AdvanceStraightAxis(proposed.position.x, proposed.velocity_x, proposed.remainder_x, ticks);
	AdvanceStraightAxis(proposed.position.y, proposed.velocity_y, proposed.remainder_y, ticks);
	return SpaceStraightMotionStep{proposed, CrossedSpacePlaneEdges(proposed.position, bounds)};
}

/** Preserve unsigned fractional travel when an axis reverses, including exact arrival.
 * @param position Validated in-band coordinate, updated only after the reflected candidate fits.
 * @param velocity Signed per-day Q16 component; one step must be smaller than this axis's span.
 * @param remainder Saved unsigned sub-step travel, strictly below the shared day length.
 * @param min Inclusive lower reflection boundary.
 * @param max Inclusive upper reflection boundary.
 */
static void AdvanceAxis(int32_t &position, int32_t &velocity, uint8_t &remainder, int32_t min, int32_t max)
{
	const uint16_t day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	assert(position >= min && position <= max && remainder < day);
	const uint32_t numerator = static_cast<uint32_t>(std::abs(velocity)) + remainder;
	const int32_t step = numerator / day;
	assert(step < max - min);
	remainder = numerator % day;
	int32_t candidate = position + (velocity < 0 ? -step : step);
	// Reaching/crossing the lower edge while moving outward mirrors overshoot and reverses direction without losing fractional travel.
	if (candidate <= min && velocity < 0) { candidate = min + (min - candidate); velocity = -velocity; }
	// The upper edge follows the same rule; an axis already turned inward must not reflect twice on exact arrival.
	if (candidate >= max && velocity > 0) { candidate = max - (candidate - max); velocity = -velocity; }
	assert(candidate >= min && candidate <= max);
	position = candidate;
}

/** @param motion Validated bounded motion advanced by one synchronized tick. @param band Shared body-centre bounds; each component reflects independently at a corner. */
void AdvanceSpaceMotion(SpaceMotion &motion, const SpaceTravelBand &band)
{
	AdvanceAxis(motion.position.x, motion.velocity_x, motion.remainder_x, band.min.x, band.max.x);
	AdvanceAxis(motion.position.y, motion.velocity_y, motion.remainder_y, band.min.y, band.max.y);
}

/** @param a First bounded plane point. @param b Second bounded plane point. @return Exact squared Q16 distance. */
uint64_t SpaceDistanceSquared(SpacePoint a, SpacePoint b)
{
	const int64_t dx = static_cast<int64_t>(a.x) - b.x, dy = static_cast<int64_t>(a.y) - b.y;
	return static_cast<uint64_t>(dx * dx + dy * dy);
}

/** Exact floor square root; bounded integer work on every supported platform.
 * @param value Squared bounded plane distance. @return Greatest integer whose square does not exceed the input.
 */
static uint64_t IntegerSquareRoot(uint64_t value)
{
	uint64_t result = 0, bit = uint64_t{1} << 62;
	// At most32 base-four digits keep pursuit work independent of map area and avoid platform-specific floating point.
	while (bit > value) bit >>= 2;
	while (bit != 0) {
		if (value >= result + bit) { value -= result + bit; result = (result >> 1) + bit; }
		else result >>= 1;
		bit >>= 2;
	}
	return result;
}

/** @param position Actual point to move. @param target Current authoritative approach. @param remainder Saved unsigned speed fraction. @return Whether arrival occurs this tick. */
bool MoveSpaceSurveyTowards(SpacePoint &position, SpacePoint target, uint8_t &remainder)
{
	const uint16_t day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	const uint32_t numerator = GetMultiLayerMiningProbeTuning().travel_tiles_per_day * SPACE_Q16 + remainder;
	const uint32_t step = numerator / day;
	remainder = numerator % day;
	const uint64_t distance = IntegerSquareRoot(SpaceDistanceSquared(position, target));
	if (distance <= step) { position = target; return true; }
	position.x += static_cast<int32_t>((static_cast<int64_t>(target.x) - position.x) * step / static_cast<int64_t>(distance));
	position.y += static_cast<int32_t>((static_cast<int64_t>(target.y) - position.y) * step / static_cast<int64_t>(distance));
	return false;
}

/** @copydoc SpaceSpatialIndex::Reset */
void SpaceSpatialIndex::Reset(uint32_t width, uint32_t height, uint32_t count)
{
	*this = {};
	const auto plane = GetSpacePlaneBounds(width, height);
	if (!plane) return;
	this->bounds = *plane;
	this->columns = (width + 15) / 16; this->rows = (height + 15) / 16;
	this->buckets.assign(this->columns * this->rows, {});
	this->membership.reserve(std::min(count, MAX_ORBITAL_BODIES));
}

/** @copydoc SpaceSpatialIndex::Insert */
bool SpaceSpatialIndex::Insert(OrbitalBodyID id, SpacePoint position)
{
	// Reject outside points before division/indexing: terminal proposals must be
	// retired by the field owner, never clamped or linked into an unrelated cell.
	if (id == INVALID_ORBITAL_BODY || !this->bounds.Contains(position) ||
		this->membership.size() >= MAX_ORBITAL_BODIES || this->membership.contains(id.base())) return false;
	const uint32_t bucket = (position.y / SPACE_Q16 / 16) * this->columns + position.x / SPACE_Q16 / 16;
	auto &entries = this->buckets[bucket];
	const uint32_t slot = static_cast<uint32_t>(entries.size());
	try {
		entries.push_back(id);
		try {
			this->membership.emplace(id.base(), Membership{bucket, slot});
		} catch (const std::bad_alloc &) {
			entries.pop_back();
			return false;
		}
	} catch (const std::bad_alloc &) {
		return false;
	}
	RecordSpaceWork(SpaceWorkKind::IndexLinks);
	return true;
}

/** @copydoc SpaceSpatialIndex::InspectStorage */
SpaceSpatialIndex::StorageUsage SpaceSpatialIndex::InspectStorage() const
{
	StorageUsage result{this->membership.size(), this->membership.bucket_count(), this->buckets.size()};
	// Empty visited cells must retain no population arrays. Report both actual
	// live slots and backing capacity separately from fixed plane bucket headers.
	for (const auto &bucket : this->buckets) {
		result.live_slots += bucket.size(); result.retained_slots += bucket.capacity();
		if (bucket.empty()) result.empty_retained_slots += bucket.capacity();
	}
	return result;
}

/** @copydoc SpaceSpatialIndex::Unlink */
void SpaceSpatialIndex::Unlink(Membership entry)
{
	RecordSpaceWork(SpaceWorkKind::IndexUnlinks);
	auto &previous = this->buckets[entry.bucket];
	const auto moved = previous.back();
	previous[entry.slot] = moved;
	this->membership.at(moved.base()).slot = entry.slot;
	previous.pop_back();
	// Vacated cells retain no historical cluster allocation. Repeated travel
	// through the map must not leave a full population's storage in every cell.
	if (previous.empty()) std::vector<OrbitalBodyID>{}.swap(previous);
}

/** @copydoc SpaceSpatialIndex::Remove */
bool SpaceSpatialIndex::Remove(OrbitalBodyID id)
{
	const auto found = this->membership.find(id.base());
	if (found == this->membership.end()) return false;
	this->Unlink(found->second);
	this->membership.erase(found);
	return true;
}

/** @copydoc SpaceSpatialIndex::Update */
bool SpaceSpatialIndex::Update(OrbitalBodyID id, SpacePoint position)
{
	RecordSpaceWork(SpaceWorkKind::IndexUpdates);
	// Invalid motion cannot evict an existing membership before the terminal
	// lifecycle owner has released the corresponding original body and leases.
	if (id == INVALID_ORBITAL_BODY || !this->bounds.Contains(position)) return false;
	const auto found = this->membership.find(id.base());
	if (found == this->membership.end()) return this->Insert(id, position);
	const uint32_t bucket = (position.y / SPACE_Q16 / 16) * this->columns + position.x / SPACE_Q16 / 16;
	if (found->second.bucket == bucket) return true;
	auto &destination = this->buckets[bucket];
	const uint32_t slot = static_cast<uint32_t>(destination.size());
	// Allocate the destination first so an allocation refusal preserves the old
	// spatial answer; successful publication then requires only bounded unlink.
	try {
		destination.push_back(id);
	} catch (const std::bad_alloc &) {
		return false;
	}
	this->Unlink(found->second);
	found->second = {bucket, slot};
	RecordSpaceWork(SpaceWorkKind::IndexLinks);
	return true;
}

/** @param min Minimum plane query point. @param max Maximum plane query point. @return Stable IDs from intersecting buckets, without precise-distance or ownership admission. */
std::vector<OrbitalBodyID> SpaceSpatialIndex::Query(SpacePoint min, SpacePoint max) const
{
	RecordSpaceWork(SpaceWorkKind::QueryCalls);
	std::vector<OrbitalBodyID> result;
	// An empty index or impossible query cannot authorize lookup outside the generated field.
	if (this->columns == 0 || max.x < 0 || max.y < 0 || min.x > max.x || min.y > max.y ||
		min.x > this->bounds.max.x || min.y > this->bounds.max.y) return result;
	const int x0 = std::clamp(min.x / SPACE_Q16 / 16, 0, static_cast<int>(this->columns) - 1);
	const int x1 = std::clamp(max.x / SPACE_Q16 / 16, 0, static_cast<int>(this->columns) - 1);
	const int y0 = std::clamp(min.y / SPACE_Q16 / 16, 0, static_cast<int>(this->rows) - 1);
	const int y1 = std::clamp(max.y / SPACE_Q16 / 16, 0, static_cast<int>(this->rows) - 1);
	// Visit only intersecting fixed-size buckets; stable sorting afterward prevents container moves affecting target order.
	for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) {
		const auto &bucket = this->buckets[y * this->columns + x];
		RecordSpaceWork(SpaceWorkKind::QueryBuckets); RecordSpaceWork(SpaceWorkKind::QueryCandidates, bucket.size());
		result.insert(result.end(), bucket.begin(), bucket.end());
	}
	/** @param a First actual candidate. @param b Second actual candidate. @return Native stable-ID order, independent of bucket removal history. */
	std::ranges::sort(result, [](OrbitalBodyID a, OrbitalBodyID b) { RecordSpaceWork(SpaceWorkKind::QueryOrderTests); return a < b; });
	return result;
}
