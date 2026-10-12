/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file space.cpp Shared integer Space geometry; no camera or simulation RNG.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
 */
#include "stdafx.h"
#include "space.h"
#include <algorithm>
#include "safeguards.h"

bool SpaceTravelBand::Contains(SpacePoint p) const
{
	return p.x >= this->min.x && p.y >= this->min.y && p.x <= this->max.x && p.y <= this->max.y;
}

SpaceTravelBand GetSpaceTravelBand(uint32_t width, uint32_t height)
{
	if (width < 64 || height < 64 || width > 4096 || height > 4096) return {{0, 0}, {-1, -1}};
	return {{2 * SPACE_Q16, static_cast<int32_t>(height / 4 + 2) * SPACE_Q16},
		{static_cast<int32_t>(width - 3) * SPACE_Q16, static_cast<int32_t>(3 * height / 4 - 3) * SPACE_Q16}};
}

/** Preserve unsigned fractional travel when an axis reverses, including exact arrival. */
static void AdvanceAxis(int32_t &position, int32_t &velocity, uint8_t &remainder, int32_t min, int32_t max)
{
	const uint32_t numerator = static_cast<uint32_t>(std::abs(velocity)) + remainder;
	const int32_t step = numerator / 74;
	remainder = numerator % 74;
	position += velocity < 0 ? -step : step;
	if (position <= min && velocity < 0) { position = min + (min - position); velocity = -velocity; }
	if (position >= max && velocity > 0) { position = max - (position - max); velocity = -velocity; }
}

void AdvanceSpaceMotion(SpaceMotion &motion, const SpaceTravelBand &band)
{
	AdvanceAxis(motion.position.x, motion.velocity_x, motion.remainder_x, band.min.x, band.max.x);
	AdvanceAxis(motion.position.y, motion.velocity_y, motion.remainder_y, band.min.y, band.max.y);
}

uint64_t SpaceDistanceSquared(SpacePoint a, SpacePoint b)
{
	const int64_t dx = static_cast<int64_t>(a.x) - b.x, dy = static_cast<int64_t>(a.y) - b.y;
	return static_cast<uint64_t>(dx * dx + dy * dy);
}

/** Exact floor square root; bounded integer work on every supported platform. */
static uint64_t IntegerSquareRoot(uint64_t value)
{
	uint64_t result = 0, bit = uint64_t{1} << 62;
	while (bit > value) bit >>= 2;
	while (bit != 0) {
		if (value >= result + bit) { value -= result + bit; result = (result >> 1) + bit; }
		else result >>= 1;
		bit >>= 2;
	}
	return result;
}

bool MoveSpaceSurveyTowards(SpacePoint &position, SpacePoint target, uint8_t &remainder)
{
	const uint32_t numerator = 4 * SPACE_Q16 + remainder;
	const uint32_t step = numerator / 74;
	remainder = numerator % 74;
	const uint64_t distance = IntegerSquareRoot(SpaceDistanceSquared(position, target));
	if (distance <= step) { position = target; return true; }
	position.x += static_cast<int32_t>((static_cast<int64_t>(target.x) - position.x) * step / static_cast<int64_t>(distance));
	position.y += static_cast<int32_t>((static_cast<int64_t>(target.y) - position.y) * step / static_cast<int64_t>(distance));
	return false;
}

void SpaceSpatialIndex::Reset(uint32_t width, uint32_t height, uint32_t count)
{
	this->columns = (width + 15) / 16; this->rows = (height + 15) / 16;
	this->buckets.assign(this->columns * this->rows, {});
	this->locations.assign(count, UINT32_MAX);
	this->slots.assign(count, 0);
}

void SpaceSpatialIndex::Update(OrbitalBodyID id, SpacePoint p)
{
	const uint32_t bucket = (p.y / SPACE_Q16 / 16) * this->columns + p.x / SPACE_Q16 / 16;
	auto &old = this->locations.at(id.base());
	if (old == bucket) return;
	if (old != UINT32_MAX) {
		auto &previous = this->buckets[old];
		const auto moved = previous.back();
		previous[this->slots[id.base()]] = moved;
		this->slots[moved.base()] = this->slots[id.base()];
		previous.pop_back();
	}
	this->slots[id.base()] = static_cast<uint32_t>(this->buckets.at(bucket).size());
	this->buckets[bucket].push_back(id); old = bucket;
}

std::vector<OrbitalBodyID> SpaceSpatialIndex::Query(SpacePoint min, SpacePoint max) const
{
	std::vector<OrbitalBodyID> result;
	if (this->columns == 0 || max.x < 0 || max.y < 0 || min.x > max.x || min.y > max.y) return result;
	const int x0 = std::clamp(min.x / SPACE_Q16 / 16, 0, static_cast<int>(this->columns) - 1);
	const int x1 = std::clamp(max.x / SPACE_Q16 / 16, 0, static_cast<int>(this->columns) - 1);
	const int y0 = std::clamp(min.y / SPACE_Q16 / 16, 0, static_cast<int>(this->rows) - 1);
	const int y1 = std::clamp(max.y / SPACE_Q16 / 16, 0, static_cast<int>(this->rows) - 1);
	for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) {
		const auto &bucket = this->buckets[y * this->columns + x];
		result.insert(result.end(), bucket.begin(), bucket.end());
	}
	std::ranges::sort(result);
	return result;
}
