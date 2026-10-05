/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_outline.cpp Canonical arbitrary floor/authorization sets with explicit charged extra-work previews. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 12 */

#include "stdafx.h"
#include "excavation.h"
#include "tile_map.h"

#include "safeguards.h"

/**
 * Normalize one bounded connected explicit rock-layer set, without interpreting intended final construction type.
 * @param layer Explicit expected rock layer.
 * @param tiles User working-domain references.
 * @return Canonical proposal or stable empty refusal.
 */
ExcavationOutline NormalizeExcavationOutline(MapLayer layer, std::span<const TileIndex> tiles)
{
	if (tiles.empty()) return {ExcavationError::InvalidRoute};
	if (tiles.size() > MAX_EXCAVATION_JOB_TILES * 4) return {ExcavationError::Limit};
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return {ExcavationError::MixedLayers, tiles.front()}; // Rock outlines cannot acquire Surface/Space terrain rights.
	std::vector<TileIndex> canonical(tiles.begin(), tiles.end());
	std::ranges::sort(canonical);
	canonical.erase(std::unique(canonical.begin(), canonical.end()), canonical.end());
	if (canonical.size() > MAX_EXCAVATION_JOB_TILES) return {ExcavationError::Limit, canonical.front()};
	// Canonical order makes the first invalid/wrong-layer location independent of UI drag/list insertion order.
	for (TileIndex tile : canonical) {
		if (!IsValidTile(tile)) return {ExcavationError::InvalidTile, tile};
		if (GetTileLayer(tile) != layer) return {ExcavationError::MixedLayers, tile};
	}
	if (layer == MapLayer::DeepUnderground) {
		if (!CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return {ExcavationError::LockedDeep, canonical.front()};
	}
	std::vector<bool> reached(canonical.size(), false);
	std::vector<size_t> queue{0};
	queue.reserve(canonical.size());
	reached[0] = true;
	// Eight native advancing headings define the connectivity domain; diagonal clearance is checked by the route/sweep producer.
	for (size_t cursor = 0; cursor < queue.size(); ++cursor) {
		for (uint heading = 0; heading < 8; ++heading) {
			const auto offset = TileIndexDiffCByDir(static_cast<Direction>(heading));
			const auto next = TileAddWrap(canonical[queue[cursor]], offset.x, offset.y);
			const auto it = std::lower_bound(canonical.begin(), canonical.end(), next);
			if (it == canonical.end() || *it != next) continue; // Neighbours outside this requested domain cannot connect separate rooms by implicit extra work.
			const size_t index = it - canonical.begin();
			if (reached[index]) continue;
			reached[index] = true;
			queue.push_back(index);
		}
	}
	// Report the lowest canonical unreachable member rather than whichever disconnected component a UI happened to submit last.
	for (size_t i = 0; i < canonical.size(); ++i) {
		if (!reached[i]) return {ExcavationError::InvalidRoute, canonical[i]};
	}
	ExcavationOutline result;
	result.layer = layer;
	result.requested_tiles = canonical;
	result.authorized_tiles = std::move(canonical);
	return result;
}

/**
 * Validate explicitly accepted working space and quote only its new cells through the actual shared hardness owner.
 * @param outline Requested final floor, never an inferred rail-only corridor.
 * @param accepted_extra Caller-approved cells; omitted approach/turn space remains unauthorized.
 * @param model_id Immutable design for exact support/spoil/time preview.
 * @return Complete canonical proposal or empty stable refusal.
 */
ExcavationOutline AuthorizeExcavationOutline(const ExcavationOutline &outline, std::span<const TileIndex> accepted_extra, uint16_t model_id)
{
	if (outline.error != ExcavationError::None) return outline;
	auto requested = NormalizeExcavationOutline(outline.layer, outline.requested_tiles);
	if (requested.error != ExcavationError::None) return requested;
	if (accepted_extra.size() > MAX_EXCAVATION_JOB_TILES * 4) return {ExcavationError::Limit};
	auto combined = requested.requested_tiles;
	combined.insert(combined.end(), accepted_extra.begin(), accepted_extra.end());
	auto authorized = NormalizeExcavationOutline(outline.layer, combined);
	if (authorized.error != ExcavationError::None) return authorized;
	ExcavationOutline result;
	result.layer = outline.layer;
	result.requested_tiles = std::move(requested.requested_tiles);
	result.authorized_tiles = std::move(authorized.requested_tiles);
	std::set_difference(result.authorized_tiles.begin(), result.authorized_tiles.end(), result.requested_tiles.begin(), result.requested_tiles.end(), std::back_inserter(result.extra_tiles));
	if (!result.extra_tiles.empty()) {
		result.extra_estimate = QuoteExcavationCells(result.extra_tiles, model_id);
		if (result.extra_estimate.error != ExcavationError::None) return {result.extra_estimate.error, result.extra_estimate.failure_tile};
	}
	return result;
}
