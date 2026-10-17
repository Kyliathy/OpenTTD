/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file space.cpp Shared integer Space geometry; no camera or simulation RNG.
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

/** @param width Plane width in tiles. @param height Plane height in tiles. @param count Number of stable body identities whose locations are disposable. */
void SpaceSpatialIndex::Reset(uint32_t width, uint32_t height, uint32_t count)
{
	this->columns = (width + 15) / 16; this->rows = (height + 15) / 16;
	this->buckets.assign(this->columns * this->rows, {});
	this->locations.assign(count, UINT32_MAX);
	this->slots.assign(count, 0);
}

/** @param id Existing monotonic body identity. @param p Current authoritative plane point; this lookup update creates no body. */
void SpaceSpatialIndex::Update(OrbitalBodyID id, SpacePoint p)
{
	RecordSpaceWork(SpaceWorkKind::IndexUpdates);
	const uint32_t bucket = (p.y / SPACE_Q16 / 16) * this->columns + p.x / SPACE_Q16 / 16;
	auto &old = this->locations.at(id.base());
	if (old == bucket) return;
	if (old != UINT32_MAX) {
		RecordSpaceWork(SpaceWorkKind::IndexUnlinks);
		auto &previous = this->buckets[old];
		const auto moved = previous.back();
		previous[this->slots[id.base()]] = moved;
		this->slots[moved.base()] = this->slots[id.base()];
		previous.pop_back();
	}
	this->slots[id.base()] = static_cast<uint32_t>(this->buckets.at(bucket).size());
	this->buckets[bucket].push_back(id); old = bucket;
	RecordSpaceWork(SpaceWorkKind::IndexLinks);
}

/** @param min Minimum plane query point. @param max Maximum plane query point. @return Stable IDs from intersecting buckets, without precise-distance or ownership admission. */
std::vector<OrbitalBodyID> SpaceSpatialIndex::Query(SpacePoint min, SpacePoint max) const
{
	RecordSpaceWork(SpaceWorkKind::QueryCalls);
	std::vector<OrbitalBodyID> result;
	// An empty index or impossible query cannot authorize lookup outside the generated field.
	if (this->columns == 0 || max.x < 0 || max.y < 0 || min.x > max.x || min.y > max.y) return result;
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
