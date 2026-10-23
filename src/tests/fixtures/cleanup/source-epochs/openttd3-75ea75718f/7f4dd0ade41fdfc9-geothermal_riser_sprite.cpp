/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_riser_sprite.cpp Pure original paid riser construction/connection/flow presentation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups60C/84BB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "stdafx.h"
#include "geothermal_sprite.h"
#include "geothermal_riser.h"
#include "industry.h"
#include "industry_map.h"
#include "map_func.h"
#include "multilayer_sprite.h"
#include "multilayer_state.h"
#include "utility_power.h"

/** @param tile Candidate actual native machinery cell. @return Borrowed original fixed2x2 riser industry, or nullptr; retained unavailable machinery remains drawable as blocked without reviving its portal. */
static const Industry *GetGeothermalRiserArtIndustry(TileIndex tile)
{
	// Only the selected profile and actual native Industry cell can own these
	// pixels. A camera offset, foreign definition or coincident native ID never
	// grants the fixed original rock-side machinery footprint.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || !IsValidTile(tile) ||
		!IsTileType(tile, TileType::Industry)) return nullptr;
	const auto *industry = Industry::GetByTile(tile); const auto *native = GetGeothermalRiserState(industry->index);
	if (native == nullptr || !IsGeothermalRiserDefinition(industry->type)) return nullptr; // Native construction association and exact package definition must both own this model.
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser);
	// Access and physical port never rotate the authored2x2 campus. Geometry
	// stays with the original paid machinery even after its portal is retired.
	if (industry->location.tile != native->site.tile || industry->location.w != terms->width || industry->location.h != terms->height ||
		(GetTileLayer(tile) != MapLayer::Underground && GetTileLayer(tile) != MapLayer::DeepUnderground)) return nullptr;
	const uint x = TileX(tile) - TileX(native->site.tile), y = TilePlaneY(tile) - TilePlaneY(native->site.tile);
	if (x >= terms->width || y >= terms->height) return nullptr; // Unsigned underflow and overrun cannot appropriate a neighbouring industry's cell.
	return industry;
}

/** @param native Original complete paid machinery and portal association. @return Ready1,working2 or blocked3 from actual construction/portal and shared electrical owners; no Heat state, link registration or energy copy is introduced. */
static uint8_t GetGeothermalRiserArtState(const GeothermalRiserState &native)
{
	if (!IsGeothermalRiserOperational(native.industry)) return 3;
	// Inspect only the existing bounded shared link list, never a map scan or
	// copied whole ledger. Retired history cannot animate a rebuilt machine;
	// registration admits at most one active direction per original sender.
	for (const auto &link : GetUtilityPowerLinks()) {
		if (!link.active || link.kind != UtilityLinkKind::ElectricalRiser || link.riser != native.industry) continue; // Only this original live electrical direction can report flow for the paid machine.
		const auto *source = GetUtilityPowerConsumer(link.source), *destination = GetUtilityPowerConsumer(link.destination);
		// Both real reservations and the original commissioned incarnation must
		// still agree. A reused identity, foreign account or Heat meter cannot
		// lend the equipment another route's working lamps.
		if (link.source_endpoint != native.site.portal || link.destination_endpoint != native.destination || link.portal_revision != native.portal_revision ||
			source == nullptr || destination == nullptr || !source->active || !destination->active || source->owner != native.site.owner || destination->owner != native.site.owner ||
			source->meter.domain != UtilityEnergyDomain::Electricity || destination->meter.domain != UtilityEnergyDomain::Electricity ||
			source->meter.layer != GetTileLayer(native.site.portal) || destination->meter.layer != GetTileLayer(native.destination)) return 3;
		// Shared interval rotation clears link flow. Require both real account
		// epochs too, so previous service can never imply current transfer.
		return link.flow_j != 0 && source->meter.accounting_interval == GetUtilityPowerInterval() && destination->meter.accounting_interval == GetUtilityPowerInterval() ? 2 : 1;
	}
	return 1; // Complete available machinery awaiting an explicit export remains ready; drawing never registers that export.
}

/** @copydoc GetGeothermalRiserSprite */
std::optional<SpriteID> GetGeothermalRiserSprite(TileIndex tile, bool front, uint64_t visual_ticks)
{
	const auto *industry = GetGeothermalRiserArtIndustry(tile); if (industry == nullptr) return std::nullopt;
	const uint x = TileX(tile) - TileX(industry->location.tile), y = TilePlaneY(tile) - TilePlaneY(industry->location.tile);
	const uint8_t state = IsIndustryCompleted(tile) ? GetGeothermalRiserArtState(*GetGeothermalRiserState(industry->index)) : 0;
	const uint8_t frame = state == 0 ? GetIndustryConstructionStage(tile) : GetMultiLayerVisualFrame(MultiLayerSpriteAsset::FacilityUtilityRiser, visual_ticks, false);
	return GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityUtilityRiser, {state, frame, 0, static_cast<uint16_t>(y * industry->location.w + x), static_cast<uint8_t>(front)});
}

/** @copydoc GetGeothermalIndustryBackBounds */
std::optional<SpriteBounds> GetGeothermalIndustryBackBounds(TileIndex tile)
{
	if (GetGeothermalRiserArtIndustry(tile) == nullptr) return std::nullopt;
	// Approved riser art puts the entire solid machine in its back and leaves
	// the front empty. Sort that original back as physical machinery rather
	// than painting a tall body as terrain underneath adjacent native objects.
	return SpriteBounds{{0, 0, 0}, {TILE_SIZE, TILE_SIZE, 64}, {}};
}
