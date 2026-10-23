/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_sprite.cpp Existing packed generator art selected solely from original native construction and shared utility owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8/1CB-1CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "stdafx.h"
#include "geothermal_sprite.h"
#include "geothermal.h"
#include "geothermal_generator.h"
#include "industry.h"
#include "industry_map.h"
#include "map_func.h"
#include "multilayer_sprite.h"
#include "multilayer_state.h"
#include "utility_power.h"

/** @param native Actual completed native generator and sole original service/reference owner. @return Authored ready1/working2/blocked3/heat-starved4 status, without requesting Heat, issuing identities or paying service from presentation. */
static uint8_t GetGeothermalGeneratorArtState(const GeothermalGeneratorState &native)
{
	// Calendar/company/construction permission and both explicitly installed
	// port domains are real operation prerequisites; an attractive source image
	// cannot authorize missing paid infrastructure or reopen unavailable machinery.
	if (!IsGeothermalGeneratorConstructionComplete(native.industry) || !HasGeothermalGeneratorInstalledPorts(native.industry)) return 3;
	const auto *input = GetUtilityPowerConsumer(native.heat_consumer); const auto *output = GetUtilityPowerProducer(native.electrical_producer);
	// A newly completed/connected machine may await its first ordinary driver
	// opportunity. Drawing keeps those identities unissued and still reports
	// an actual unpaid service obstruction before advertising ready machinery.
	if (native.heat_consumer.base() == UINT32_MAX && native.electrical_producer.base() == UINT32_MAX) return HasGeothermalGeneratorServiceFunds(native.industry, 1) ? 1 : 3;
	// A reused native pool slot or a numerically similar shared meter never
	// supplies this actual model's original typed conversion pair.
	if (input == nullptr || output == nullptr || !input->active || !output->active || input->owner != native.owner || output->owner != native.owner ||
		input->meter.domain != UtilityEnergyDomain::Heat || input->effect != UtilityConsumerEffect::HeatGenerator ||
		output->meter.domain != UtilityEnergyDomain::Electricity || output->kind != UtilityGenerationKind::HeatConversion || output->conversion_input != native.heat_consumer) return 3;
	// Positive original conversion in the current shared interval is actual
	// working machinery, even if that paid opportunity exhausted next-tick cash.
	if (output->meter.accounting_interval == GetUtilityPowerInterval() && output->meter.generation_energy_j != 0) return 2;
	if (!HasGeothermalGeneratorServiceFunds(native.industry, 1)) return 3;
	// Once the real allocator closed Heat, an otherwise eligible native model
	// with no conversion is genuinely starved. This includes physical source/
	// path loss and earlier consumers winning the one shared vent budget. No
	// copied availability sum or presentation-owned reservation is introduced.
	if (IsUtilityHeatAllocated()) return 4;
	return 1;
}

/** @copydoc GetGeothermalGeneratorSprite */
std::optional<SpriteID> GetGeothermalGeneratorSprite(TileIndex tile, bool front, uint64_t visual_ticks)
{
	// Only the actual selected native profile/Deep Industry cell can consume
	// this first-party art. Foreign NewGRF and conventional industries retain
	// their own existing resolver, rather than inheriting a nearby vent's model.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || !IsValidTile(tile) ||
		GetTileLayer(tile) != MapLayer::DeepUnderground || !IsTileType(tile, TileType::Industry)) return std::nullopt;
	const auto *industry = Industry::GetByTile(tile); const auto *native = GetGeothermalGeneratorState(industry->index);
	if (native == nullptr) return std::nullopt;
	if (!IsGeothermalGeneratorDefinition(industry->type)) return std::nullopt;
	const auto *terms = GetMultiLayerGeothermalGeneratorTerms(native->model); if (terms == nullptr) return std::nullopt;
	const auto *bill = GetMultiLayerGeothermalConstruction(terms->construction); const auto *claim = GetGeothermalSiteClaim(native->site);
	// Original consumed geometry and canonical4x4 layout select the cell and
	// authored view. Camera layer, a cold blueprint or a coincident rebuilt
	// IndustryID cannot appropriate another incarnation's construction art.
	if (claim == nullptr || claim->active || claim->site.kind != terms->construction || claim->site.owner != native->owner ||
		claim->site.tile != industry->location.tile || industry->location.w != bill->width || industry->location.h != bill->height) return std::nullopt;
	const uint x = TileX(tile) - TileX(industry->location.tile), y = TilePlaneY(tile) - TilePlaneY(industry->location.tile);
	const uint8_t view = static_cast<uint8_t>(claim->site.access);
	if (x >= bill->width || y >= bill->height || view >= 4) return std::nullopt; // Exact row-major world cells/view, including underflow refusal, prevent a source canvas extending native ownership.
	const auto asset = native->model == GeothermalGeneratorModel::VentTap ? MultiLayerSpriteAsset::FacilityVentPowerhouse : MultiLayerSpriteAsset::FacilityMagmaPowerhouse;
	const uint8_t state = IsIndustryCompleted(tile) ? GetGeothermalGeneratorArtState(*native) : 0;
	const uint8_t frame = state == 0 ? GetIndustryConstructionStage(tile) : GetMultiLayerVisualFrame(asset, visual_ticks, false);
	return GetMultiLayerSprite(asset, {state, frame, view, static_cast<uint16_t>(y * bill->width + x), static_cast<uint8_t>(front)});
}

/** @copydoc GetGeothermalIndustryFrontBounds */
std::optional<SpriteBounds> GetGeothermalIndustryFrontBounds(TileIndex tile)
{
	if (!GetGeothermalGeneratorSprite(tile, true, 0).has_value()) return std::nullopt;
	// The approved machinery has64-pixel cell canvases, taller than the older
	// generic20-high collector/receiver sorting box. Cover the full source
	// height while retaining exactly one native world cell per sprite parent.
	return SpriteBounds{{0, 0, 0}, {TILE_SIZE, TILE_SIZE, 64}, {}};
}
