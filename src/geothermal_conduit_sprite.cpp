/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_conduit_sprite.cpp Original branch/cap art derived from the canonical installed utility topology.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups60B/84BC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups3B-5.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "stdafx.h"
#include "geothermal_conduit_sprite.h"
#include "geothermal_utility.h"
#include "company_base.h"
#include "map_func.h"
#include "ground_sprite.h"
#include "multilayer_state.h"
#include "viewport_func.h"

/** @copydoc GetGeothermalConduitArt */
std::optional<GeothermalConduitArt> GetGeothermalConduitArt(TileIndex tile)
{
	// Only the selected native profile and a real rock-plane world cell may
	// consume this family. Camera coordinates and other profile/layer scenery
	// never stand in for an explicitly paid physical installation.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || !IsValidTile(tile)) return std::nullopt;
	const auto layer = GetTileLayer(tile);
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return std::nullopt; // Surface/orbital wiring retains its own renderer and admission; these original branches are rock-layer art only.
	GeothermalConduitArt art; bool drawable = false;
	// Source variants deliberately use Heat0/Electricity1, unlike the shared
	// domain enum. The canonical cached cell lookup supplies paid topology;
	// eight bounded neighbour queries copy no ledger, route or resource owner.
	for (uint8_t variant = 0; variant < art.domains.size(); ++variant) {
		const auto domain = variant == 0 ? UtilityEnergyDomain::Heat : UtilityEnergyDomain::Electricity;
		if (domain == UtilityEnergyDomain::Heat && layer != MapLayer::DeepUnderground) continue; // Heat has no inter-plane or Underground branch in this authored/native contract.
		const auto installed = GetGeothermalUtilityLink(tile, domain);
		if (!installed.has_value()) continue;
		if (!Company::IsValidID(installed->owner)) continue;
		// Each installed domain owns four genuine source branches. Missing,
		// removed or foreign neighbours show their approved cap, never a fake
		// connection. Publication/flow/date changes are not physical topology.
		for (uint8_t port = 0; port < art.domains[variant].size(); ++port) {
			const auto offset = TileIndexDiffCByDiagDir(static_cast<DiagDirection>(port)); const auto neighbour = TileAddWrap(tile, offset.x, offset.y);
			uint8_t state = 2;
			if (IsValidTile(neighbour) && GetTileLayer(neighbour) == layer) { // Canonical layer-safe adjacency prevents a map edge or stacked plane from supplying a cross-layer wire.
				const auto adjacent = GetGeothermalUtilityLink(neighbour, domain);
				if (adjacent.has_value() && adjacent->owner == installed->owner) state = 1; // Only an actually installed same-company/domain neighbour joins this physical branch; pixels grant no shared energy.
			}
			art.domains[variant][port] = GetGroundSprite(GroundSpriteFamily::GeothermalConduit, {state, 0, port, variant});
			drawable |= art.domains[variant][port].has_value();
		}
	}
	return drawable ? std::optional{art} : std::nullopt;
}

/** @copydoc DrawGeothermalConduitGround */
void DrawGeothermalConduitGround(TileIndex tile)
{
	const auto art = GetGeothermalConduitArt(tile); if (!art.has_value()) return;
	// Coexisting source lanes have fixed separate contacts and canonical
	// Heat-before-Electricity composition. Native bodies sort above both;
	// neither visible cap nor connection creates an electrical/Heat grant.
	for (const auto &domain : art->domains) {
		for (const auto &sprite : domain) {
			if (sprite.has_value()) DrawGroundSprite(*sprite, PAL_NONE);
		}
	}
}

/** @copydoc MarkGeothermalConduitArtDirty */
void MarkGeothermalConduitArtDirty(TileIndex tile)
{
	if (!IsValidTile(tile)) return;
	const auto layer = GetTileLayer(tile);
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return; // Only the two actual rock planes own this artwork; other infrastructure retains its own repaint contract.
	MarkTileDirtyByTile(tile);
	// A physical edit changes both endpoint appearances. Five bounded dirty
	// cells update native UI immediately, without a map scan, clock, saved
	// animation state, route repair or renewed resource reservation.
	for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
		const auto offset = TileIndexDiffCByDiagDir(direction); const auto neighbour = TileAddWrap(tile, offset.x, offset.y);
		if (IsValidTile(neighbour)) MarkTileDirtyByTile(neighbour);
	}
}
