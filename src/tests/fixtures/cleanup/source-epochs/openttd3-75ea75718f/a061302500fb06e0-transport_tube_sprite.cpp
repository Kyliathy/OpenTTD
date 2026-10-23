/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_sprite.cpp Native tube geometry selects existing packed artwork without simulation mutations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups13B/13CC/13DB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "stdafx.h"
#include "transport_tube_sprite.h"
#include "transport_tube.h"
#include "transport_tube_geothermal.h"
#include "multilayer_sprite.h"
#include "multilayer_content.h"
#include "excavation.h"
#include "excavation_state.h"
#include "rail_map.h"
#include "landscape.h"
#include "track_func.h"
#include "safeguards.h"

/**
 * Query actual current generation rather than inventing a visual power allowance.
 * @param producer Exact original external/native output identity.
 * @return Whether an active original output published positive whole energy in the current shared interval.
 */
static bool HasTubeVisualPublication(UtilityProducerID producer)
{
	const auto *source = GetUtilityPowerProducer(producer);
	// A retired, expired or zero-output source cannot illuminate a disconnected
	// network. Already spent energy remains history; this indicator grants no use.
	return source != nullptr && source->active && source->meter.accounting_interval == GetUtilityPowerInterval() && source->meter.generation_energy_j != 0;
}

/** @copydoc IsTubeTileEnergized */
bool IsTubeTileEnergized(TileIndex tile, TrackBits tracks)
{
	// A numeric rail label cannot illuminate foreign/Classic, unexcavated or
	// nonrock scenery. This checks existing identity, not today's build date.
	if (!IsValidTile(tile) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		!IsTubeRailType(GetTileRailType(tile))) return false;
	const auto layer = GetTileLayer(tile);
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return false; // Only the two real rock layers may carry tube service.
	if (GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated) return false;
	const Owner owner = GetTileOwner(tile);
	// Only bounded installed contracts are inspected, never map planes or
	// vehicles. Original path validation and cached native geometry retain all
	// source identities; these reads cannot renew a lease or publish a grant.
	for (const auto &connection : GetTubePowerConnections()) {
		if (!connection.active || connection.owner.base() != owner || !HasTubeVisualPublication(connection.producer)) continue; // Only the actual owner's live published source may light its track.
		if (ValidateTubePowerConnections(std::span{&connection, size_t{1}}, GetCurrentMultiLayerProfileSelection()) != TubePowerError::None) continue;
		if (IsTubePowerConnectedTile(connection.producer, tile, tracks)) return true;
	}
	// Native inlets retain separate identities even when they share one output.
	// Their existing path query checks the actual purchased cable and native plant.
	for (const auto &source : GetTubeGeothermalSources()) {
		if (!source.active || source.owner.base() != owner || !HasTubeVisualPublication(source.producer)) continue; // Original source ownership/publication precedes physical native inlet reach.
		if (IsTubeGeothermalPowerConnectedTile(source.id, tile, tracks)) return true;
	}
	return false;
}

/**
 * Append one already declared packed aligned casing pair.
 * @param art Bounded complete tile presentation under construction.
 * @param asset Actual packed tube shell family.
 * @param state Powered/unpowered state chosen from the original source owner.
 * @param direction Native source view, independent of capsule facing.
 * @param variant Exact increasing-mask junction variant, otherwise zero.
 */
static void AppendTubeGuidewayPart(TubeGuidewayArt &art, MultiLayerSpriteAsset asset, uint8_t state, uint8_t direction = 0, uint16_t variant = 0)
{
	assert(art.count < art.parts.size());
	const auto back = GetMultiLayerSprite(asset, {state, 0, direction, variant, 0});
	const auto front = GetMultiLayerSprite(asset, {state, 0, direction, variant, 1});
	assert(back.has_value());
	assert(front.has_value());
	SpriteBounds bounds = GetTubeGuidewayFrontBounds();
	if (asset == MultiLayerSpriteAsset::TubeBend45) {
		// Each native corner occupies its actual quarter of the tile. Assigning
		// every curve to the southeast corner falsely moves its far rim in front
		// of a north-facing body. The physical quarter and low original height
		// let the native sorter distinguish that rim from the capsule above it.
		// Cancelling origin/offset retains every original packed pixel/anchor.
		static constexpr std::array curve_bounds{
			SpriteBounds{{0, 0, 0}, {8, 8, 4}, {0, 0, 0}}, // Upper: actual NE/NW mouths.
			SpriteBounds{{8, 8, 0}, {8, 8, 4}, {-8, -8, 0}}, // Lower: actual SE/SW mouths.
			SpriteBounds{{8, 0, 0}, {8, 8, 4}, {-8, 0, 0}}, // Left: actual SW/NW mouths.
			SpriteBounds{{0, 8, 0}, {8, 8, 4}, {0, -8, 0}}, // Right: actual NE/SE mouths.
		};
		assert(direction < curve_bounds.size()); bounds = curve_bounds[direction];
	}
	art.parts[art.count++] = {*back, *front, bounds};
}

/** @copydoc GetTubeGuidewayArt */
std::optional<TubeGuidewayArt> GetTubeGuidewayArt(TileIndex tile)
{
	// Native tube artwork cannot replace foreign/Classic rail or a sloping
	// tile. Today's construction date cannot erase the identity of retained
	// machinery; original power owners still independently enforce availability.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || !IsValidTile(tile) ||
		!IsPlainRailTile(tile) || !IsTubeRailType(GetRailType(tile)) || GetTileSlope(tile) != SLOPE_FLAT) return std::nullopt;
	const auto layer = GetTileLayer(tile);
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return std::nullopt; // A canonical label cannot paint an illegal Surface/Space tube into existence.
	const auto floor = GetGeologyWorldState().terrain.Get(tile);
	if (!floor || *floor != ExcavationState::Excavated) return std::nullopt; // Only the actual geology owner's already excavated floor can carry a casing.
	const auto tracks = GetTrackBits(tile);
	const uint mask = tracks.base();
	if (mask == 0 || mask >= 64) return std::nullopt; // Packed shells describe exactly the six native plain-track bits.
	TubeGuidewayArt art;
	art.energized = IsTubeTileEnergized(tile, tracks);
	const uint8_t state = art.energized ? 1 : 2;
	if (mask == 3) {
		AppendTubeGuidewayPart(art, MultiLayerSpriteAsset::TubeCrossing, state);
	} else if (std::popcount(mask) > 1 && mask != 12 && mask != 48) { // Shared-edge multi-piece layouts need a junction; the two independent curve pairs keep separate casings.
		// Authored junctions enumerate the54 shared-edge masks in ascending
		// native bit order. The crossing and two independent curve pairs retain
		// their own cases, so no absent arm becomes a painted connection.
		uint16_t variant = 0;
		for (uint candidate = 1; candidate < mask; ++candidate) {
			if (std::popcount(candidate) > 1 && candidate != 3 && candidate != 12 && candidate != 48) ++variant; // Match only the authored shared-edge source variants, excluding independent pieces.
		}
		AppendTubeGuidewayPart(art, MultiLayerSpriteAsset::TubeJunction, state, 0, variant);
	} else {
		// Single pieces and the two nonconnecting parallel pairs use the exact
		// original axis/corner views; assembling them cannot join their routes.
		for (Track track : tracks) {
			const uint index = static_cast<uint>(track);
			AppendTubeGuidewayPart(art, index < 2 ? MultiLayerSpriteAsset::TubeStraight : MultiLayerSpriteAsset::TubeBend45,
				state, static_cast<uint8_t>(index < 2 ? index : index - 2));
		}
	}
	const auto ends = GetTubeGuidewayEnds(tile);
	// End caps follow only real native neighbour mouths, including actual depot
	// door direction. Signals, grants and camera selection cannot create an end.
	for (uint8_t port = 0; port < 4; ++port) {
		if (ends[static_cast<DiagDirection>(port)]) AppendTubeGuidewayPart(art, MultiLayerSpriteAsset::TubeEnd, state, port);
	}
	return art;
}

/** @copydoc GetTubeGuidewayFrontBounds */
const SpriteBounds &GetTubeGuidewayFrontBounds()
{
	// The complete casing is ground. Only its low near-rim subset sorts after
	// capsules at the near corner; cancelling that origin preserves the exact
	// packed ground anchor instead of shifting the foreground onto a neighbour.
	static constexpr SpriteBounds bounds{{15, 15, 0}, {1, 1, 4}, {-15, -15, 0}};
	return bounds;
}
