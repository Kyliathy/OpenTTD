/**
 * @file ground_processor_sprite.cpp Pure four-processor original art selection from native production status.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "stdafx.h"
#include "ground_processor_sprite.h"
#include "industry.h"
#include "industry_map.h"
#include "map_func.h"
#include "layer_processor_presentation.h"
#include "multilayer_content.h"
#include <array>
#include <string_view>
#include <utility>

#include "safeguards.h"

/** Source-only associations; native definitions retain every primary/alternate recipe and physical stock. */
static constexpr std::array<std::pair<std::string_view, GroundSpriteFamily>, 4> _selected_ground_processors = {{
	{"facility.metal-refinery", GroundSpriteFamily::FacilityMetalRefinery},
	{"facility.chemical-works", GroundSpriteFamily::FacilityChemicalWorks},
	{"facility.battery-works", GroundSpriteFamily::FacilityBatteryWorks},
	{"facility.electronics-works", GroundSpriteFamily::FacilityElectronicsWorks},
}};

/** Match the native capability's source identity without introducing another recipe or cargo registry.
 * @param asset_id Actual existing native capability art name.
 * @return Delivered selected source family, or absence for a deferred/foreign facility.
 */
static std::optional<GroundSpriteFamily> FindSelectedGroundProcessor(std::string_view asset_id)
{
	// Only artwork associations live here. Production still reads the actual
	// selected native recipe, all native input alternatives and sole Industry stocks.
	for (const auto &entry : _selected_ground_processors) {
		if (entry.first == asset_id) return entry.second;
	}
	return std::nullopt;
}

/** @copydoc GetGroundProcessorSprite */
std::optional<SpriteID> GetGroundProcessorSprite(TileIndex tile, bool front, uint64_t visual_ticks)
{
	// A source family requires an actual native-profile Surface industry tile;
	// numeric IDs or a completed-looking image confer no recipe/layer authority.
	if (!IsValidTile(tile) || !IsTileType(tile, TileType::Industry) ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		GetTileLayer(tile) != MapLayer::Surface) return std::nullopt;
	const Industry *industry = Industry::GetIfValid(GetIndustryIndex(tile));
	if (industry == nullptr) return std::nullopt;
	const auto *capability = GetLayerIndustryCapability(industry->type);
	// The fixed four-by-four source cannot substitute for another native
	// footprint or operating layer, including future geothermal factories.
	if (capability == nullptr || !capability->layers.Test(MapLayer::Surface) ||
		GetTileLayer(industry->location.tile) != MapLayer::Surface || industry->location.w != 4 ||
		industry->location.h != 4) return std::nullopt;
	const auto family = FindSelectedGroundProcessor(capability->asset_id);
	if (!family) return std::nullopt;
	const uint x = TileX(tile) - TileX(industry->location.tile);
	const uint y = TilePlaneY(tile) - TilePlaneY(industry->location.tile);
	if (x >= 4 || y >= 4) return std::nullopt; // Detached cells never wrap into a neighboring source parent.
	const auto status = GetLayerProcessorOperationalStatus(*industry);
	if (status == LayerProcessorOperationalStatus::MissingState) return std::nullopt;
	MultiLayerSpritePose pose;
	pose.tile = static_cast<uint16_t>(y * 4 + x);
	pose.part = static_cast<uint8_t>(front);
	// Per-cell paid construction remains static. A finished cell waits with
	// its completed construction image until the whole native machinery gate
	// allows the common inventory/recipe owner to advertise operational state.
	if (!IsIndustryCompleted(tile) || status == LayerProcessorOperationalStatus::Construction) {
		pose.state = 0;
		pose.frame = GetIndustryConstructionStage(tile);
	} else {
		switch (status) {
			case LayerProcessorOperationalStatus::Idle:
				pose.state = 1;
				break;
			case LayerProcessorOperationalStatus::Working:
				pose.state = 2;
				pose.frame = static_cast<uint8_t>((visual_ticks / 8) % 4);
				break;
			case LayerProcessorOperationalStatus::Unavailable:
			case LayerProcessorOperationalStatus::Paused:
			case LayerProcessorOperationalStatus::Stopped:
			case LayerProcessorOperationalStatus::OutputFull:
			case LayerProcessorOperationalStatus::InvalidState:
				pose.state = 3;
				break;
			default: return std::nullopt; // Unknown owner state needs an explicit source contract before it can display operation.
		}
	}
	return GetGroundSprite(*family, pose);
}
