/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_conduit_sprite.cpp Derived ground ports for original paid external tube paths.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "stdafx.h"
#include "transport_tube_sprite.h"
#include "transport_tube.h"
#include "multilayer_sprite.h"
#include "multilayer_content.h"
#include "excavation_state.h"
#include "landscape.h"
#include "viewport_func.h"
#include <map>
#include "safeguards.h"

/** One derived original-source edge reference; never an installation or electrical account. */
struct TubeConduitPortReference {
	UtilityProducerID producer; ///< Original retained service identity, revalidated at observation.
	DiagDirection port; ///< Actual adjacent cell's cardinal edge, never a capsule heading.
};

static std::map<TileIndex, std::vector<TubeConduitPortReference>> _tube_conduit_ports; ///< Derived bounded path index, invalidated by existing topology hooks.
static bool _tube_conduit_index_ready = false; ///< No map plane or per-tick traversal is needed to rebuild this index.

/** @copydoc ResetTubeConduitArtCache */
void ResetTubeConduitArtCache()
{
	_tube_conduit_ports.clear();
	_tube_conduit_index_ready = false;
}

/** Rebuild edge references from bounded canonical active contracts only; source quantities and canonical paths stay borrowed. */
static void EnsureTubeConduitPortIndex()
{
	if (_tube_conduit_index_ready) return;
	// Each paid saved corridor contributes only its consecutive edges. Nearby
	// parallel paths never gain a cross-link from geometric proximity alone.
	for (const auto &connection : GetTubePowerConnections()) {
		if (!connection.active) continue;
		for (size_t index = 1; index < connection.utility_path.size(); ++index) {
			const auto a = connection.utility_path[index - 1], b = connection.utility_path[index];
			// Canonical admission owns complete path validity. Defensively refuse
			// invalid/cross-plane addresses before native adjacency operations.
			if (!IsValidTile(a) || !IsValidTile(b) || GetTileLayer(a) != GetTileLayer(b)) continue;
			for (const auto port : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
				if (TileAddByDiagDir(a, port) != b) continue;
				_tube_conduit_ports[a].push_back({connection.producer, port});
				_tube_conduit_ports[b].push_back({connection.producer, ReverseDiagDir(port)});
				break;
			}
		}
	}
	_tube_conduit_index_ready = true;
}

/** @copydoc GetTubeConduitArt */
std::optional<TubeConduitArt> GetTubeConduitArt(TileIndex tile)
{
	// A retained contract cannot paint foreign-profile, solid, sloping or other
	// plane scenery. Today's construction date does not erase installed wires.
	if (!IsValidTile(tile) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		GetTileSlope(tile) != SLOPE_FLAT) return std::nullopt;
	const auto layer = GetTileLayer(tile);
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return std::nullopt; // External services are explicit rock-layer corridors, never cross-layer visual shortcuts.
	if (GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated) return std::nullopt;
	EnsureTubeConduitPortIndex();
	const auto found = _tube_conduit_ports.find(tile);
	if (found == _tube_conduit_ports.end()) return std::nullopt;
	std::array<uint8_t, 4> states{};
	// Overlapping paid corridors may share an authored branch. Positive current
	// publication wins for that edge only; merging pixels creates no route/grant.
	for (const auto &reference : found->second) {
		const auto *connection = GetTubePowerConnection(reference.producer);
		const auto *source = GetUtilityPowerProducer(reference.producer);
		// Retired history and reused/foreign source identities are never an
		// installation. Native lifecycle/admission retains physical authority.
		if (connection == nullptr || !connection->active || source == nullptr || !source->active || source->owner != connection->owner ||
			source->kind != UtilityGenerationKind::External || source->meter.layer != layer) continue;
		const bool energized = source->meter.accounting_interval == GetUtilityPowerInterval() && source->meter.generation_energy_j != 0; // A current original publication lights the wire but promises no selected capsule work.
		auto &state = states[to_underlying(reference.port)];
		if (state != 1) state = energized ? 1 : 2;
	}
	TubeConduitArt art;
	bool installed = false;
	for (uint8_t port = 0; port < states.size(); ++port) {
		if (states[port] == 0) continue;
		art.ports[port] = GetMultiLayerSprite(MultiLayerSpriteAsset::TubeConduit, {states[port], 0, port});
		installed = true;
	}
	return installed ? std::optional{art} : std::nullopt;
}

/** @copydoc DrawTubeConduitGround */
void DrawTubeConduitGround(TileIndex tile)
{
	const auto art = GetTubeConduitArt(tile);
	if (!art.has_value()) return;
	// The four independent source branches compose on existing floor. Native
	// vehicles and sortable buildings remain above this ground-only body.
	for (const auto &sprite : art->ports) {
		if (sprite.has_value()) DrawGroundSprite(*sprite, PAL_NONE);
	}
}
