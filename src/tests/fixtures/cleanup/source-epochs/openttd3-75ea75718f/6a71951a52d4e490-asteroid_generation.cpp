/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_generation.cpp Exact local starter-grid selection and real Surface candidate search.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
 */
#include "stdafx.h"
#include "asteroid_generation.h"
#include "launch_site.h"
#include "command_type.h"
#include "map_layer.h"
#include "multilayer_content.h"
#include "multilayer_asteroid_catalogue.h"
#include "space_vehicle.h"
#include "safeguards.h"

/** One disposable grid point and its evolving index during the generator's three leading swaps. */
struct StarterGridCandidate {
	SpacePoint point; ///< Original admissible body centre.
	uint32_t index; ///< Current row-major vector index, preserving the generator's exact distance-tie order.
	bool selected = false; ///< Earlier starter classes cannot select this point again.
};

/**
 * Reproduce nearest-point selection with at most25 local points and the original swap tie break.
 * @param width Supported Surface-plane width.
 * @param height Supported Surface-plane height.
 * @param preference Projected generator preference inside the plane.
 * @param candidates_examined Optional exact local-point count, reset before calculation.
 * @return Exact first three centres and maximum canonical approach distance, or unsupported geometry.
 */
std::optional<AsteroidStarterTriple> GetAsteroidStarterTriple(uint32_t width, uint32_t height, SpacePoint preference, uint32_t *candidates_examined)
{
	if (candidates_examined != nullptr) *candidates_examined = 0;
	// Unsupported dimensions or an out-of-plane preference cannot define an initial approach or valid grid index.
	if (GetMultiLayerAsteroidPopulation(width, height, 0) == 0 || preference.x < 0 || preference.y < 0 ||
		preference.x >= static_cast<int64_t>(width) * SPACE_Q16 || preference.y >= static_cast<int64_t>(height) * SPACE_Q16) return std::nullopt;
	const auto band = GetSpaceTravelBand(width, height);
	const int32_t spacing = GetMultiLayerAsteroidPlacementDefinition(MultiLayerGameplayProfileID::MultilayerIndustry)->grid_spacing_tiles * SPACE_Q16;
	const int32_t columns = (band.max.x - band.min.x) / spacing + 1;
	const int32_t rows = (band.max.y - band.min.y) / spacing + 1;
	if (static_cast<int64_t>(columns) * rows < 3) return std::nullopt;
	const int32_t near_x = std::clamp((preference.x - band.min.x) / spacing, 0, columns - 1);
	const int32_t near_y = std::clamp((preference.y - band.min.y) / spacing, 0, rows - 1);
	std::vector<StarterGridCandidate> candidates;
	candidates.reserve(25);
	// Two neighboring indices each way cover three points along one edge when the preference lies far outside the band.
	for (int32_t y = std::max(0, near_y - 2); y <= std::min(rows - 1, near_y + 2); ++y) {
		for (int32_t x = std::max(0, near_x - 2); x <= std::min(columns - 1, near_x + 2); ++x) {
			candidates.push_back({{band.min.x + x * spacing, band.min.y + y * spacing}, static_cast<uint32_t>(y * columns + x)});
		}
	}
	if (candidates_examined != nullptr) *candidates_examined = static_cast<uint32_t>(candidates.size());
	AsteroidStarterTriple result;
	const auto offset = SpaceMission{}.attachment_offset;
	// Preserve the complete grid vector's distance and evolving-index tie break without scanning its distant cells.
	for (uint32_t starter = 0; starter < 3; ++starter) {
		StarterGridCandidate *best = nullptr;
		for (auto &candidate : candidates) {
			if (candidate.selected) continue;
			const auto distance = SpaceDistanceSquared(candidate.point, preference);
			// Equal-distance candidates must follow the same current vector order as the production generator's leading swaps.
			if (best == nullptr || distance < SpaceDistanceSquared(best->point, preference) ||
				(distance == SpaceDistanceSquared(best->point, preference) && candidate.index < best->index)) best = &candidate;
		}
		assert(best != nullptr);
		result.positions[starter] = best->point;
		const SpacePoint approach{best->point.x + offset.x, best->point.y + offset.y};
		result.approach_distance_squared = std::max(result.approach_distance_squared, SpaceDistanceSquared(approach, preference));
		const auto selected_index = best->index;
		best->selected = true;
		// A displaced leading cell may participate in a later distance tie; update its current index exactly as iter_swap does.
		for (auto &candidate : candidates) {
			if (candidate.index == starter) candidate.index = selected_index;
			else if (candidate.index == selected_index) candidate.index = starter;
		}
	}
	return result;
}

/**
 * Convert saved squared Q16 distance into a conservative integer Q16 bound.
 * @param input Frozen initial input; Legacy explicitly has no reconstructible historical bound.
 * @return Upward-rounded maximum approach distance, or no value for a legacy field.
 */
std::optional<uint32_t> GetAsteroidInitialApproachDistance(const AsteroidStarterInput &input)
{
	if (input.kind == AsteroidStarterKind::Legacy) return std::nullopt;
	uint64_t low = 0, high = uint64_t{8192} * SPACE_Q16;
	// Unknown input provenance or an out-of-contract magnitude cannot produce an understated access bound.
	if (to_underlying(input.kind) > to_underlying(AsteroidStarterKind::Site) || input.initial_approach_distance_squared > high * high) return std::nullopt;
	// Integer lower-bound search keeps a boundary one Q16 unit outside the radius from rounding into eligibility.
	while (low < high) {
		const uint64_t middle = low + (high - low) / 2;
		if (middle * middle >= input.initial_approach_distance_squared) high = middle;
		else low = middle + 1;
	}
	return static_cast<uint32_t>(low);
}

/**
 * Select a real geometric launch candidate in stable Surface tile/rotation order without RNG draws.
 * @param width Actual supported plane width.
 * @param height Actual supported plane height.
 * @param work Optional resettable work counters for acceptance.
 * @return Initial site/preference inputs, or unsupported dimension failure; NoSite is explicit rather than invented eligibility.
 */
std::optional<AsteroidStarterInput> FindWorldAsteroidStarterInput(uint32_t width, uint32_t height, AsteroidStarterSearchWork *work)
{
	if (work != nullptr) *work = {};
	// A query cannot borrow another world's geometry or silently convert an unsupported requested population.
	if (GetMultiLayerAsteroidPopulation(width, height, 0) == 0 || Map::SizeX() != width || Map::SizeY() != height) return std::nullopt;
	AsteroidStarterInput result;
	result.kind = AsteroidStarterKind::NoSite;
	result.preference = {static_cast<int32_t>(width / 2) * SPACE_Q16, static_cast<int32_t>(height / 2) * SPACE_Q16};
	result.initial_approach_distance_squared = GetAsteroidStarterTriple(width, height, result.preference)->approach_distance_squared;
	const uint64_t radius = GetMultiLayerAsteroidPlacementDefinition(MultiLayerGameplayProfileID::MultilayerIndustry)->starter_radius_tiles * SPACE_Q16;
	const uint64_t preferred_bound = radius * radius;
	// Visit at most one plane in stable order; the two unique footprint orientations cover all four native rotations.
	for (uint32_t y = 0; y < height; ++y) for (uint32_t x = 0; x < width; ++x) {
		if (work != nullptr) ++work->anchors;
		const auto anchor = TileXYL(x, y, MapLayer::Surface);
		for (uint8_t rotation = 0; rotation < 2; ++rotation) {
			if (work != nullptr) ++work->geometry_queries;
			if (GetLaunchCenterSiteGeometryBlock(anchor, rotation).Failed()) continue;
			const SpacePoint preference{static_cast<int32_t>(x) * SPACE_Q16, static_cast<int32_t>(y) * SPACE_Q16};
			uint32_t examined = 0;
			const auto triple = GetAsteroidStarterTriple(width, height, preference, &examined);
			assert(triple);
			if (work != nullptr) work->grid_candidates += examined;
			// Outside the preference radius retain the closest actual triple; equal bounds retain stable tile/rotation priority.
			if (result.kind == AsteroidStarterKind::NoSite || triple->approach_distance_squared < result.initial_approach_distance_squared) {
				result = {AsteroidStarterKind::Site, x, y, rotation, preference, triple->approach_distance_squared};
			}
			if (triple->approach_distance_squared <= preferred_bound) return result;
		}
	}
	return result;
}
