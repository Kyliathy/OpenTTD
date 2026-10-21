/**
 * @file ground_mine_sprite.cpp Derive exact seven-mine source poses without changing finite production.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "stdafx.h"
#include "ground_mine_sprite.h"
#include "industry.h"
#include "industry_map.h"
#include "map_func.h"
#include "multilayer_industry.h"
#include "multilayer_content.h"
#include <array>
#include <string_view>
#include <utility>

#include "safeguards.h"

/** Approved art-name associations only; resource meanings, dates, stocks and industry IDs stay native. */
static constexpr std::array<std::pair<std::string_view, GroundSpriteFamily>, 7> _selected_ground_mines = {{
	{"mine.COAL", GroundSpriteFamily::MineCOAL},
	{"mine.IORE", GroundSpriteFamily::MineIORE},
	{"mine.CORE", GroundSpriteFamily::MineCORE},
	{"mine.SALT", GroundSpriteFamily::MineSALT},
	{"mine.LITH", GroundSpriteFamily::MineLITH},
	{"mine.COBT", GroundSpriteFamily::MineCOBT},
	{"mine.RARE", GroundSpriteFamily::MineRARE},
}};

/**
 * Match the native capability's explicit art identity to its qualified source family.
 * @param asset_id Exact existing native capability art name, never a translated name or cargo-slot cast.
 * @return Selected published source family; unselected native minerals keep their existing renderer.
 */
static std::optional<GroundSpriteFamily> FindSelectedGroundMine(std::string_view asset_id)
{
	// These rows associate delivered artwork with the sole capability owner;
	// they deliberately contain no copied recipe, date or production registry.
	for (const auto &entry : _selected_ground_mines) {
		if (entry.first == asset_id) return entry.second;
	}
	return std::nullopt;
}

/** @copydoc GetGroundMineSprite */
std::optional<SpriteID> GetGroundMineSprite(TileIndex tile, bool front, uint64_t visual_ticks)
{
	// Only a real native-profile machinery tile can supply industry/capability
	// authority. Classic, invalid and unrelated ground must keep its own art.
	if (!IsValidTile(tile) || !IsTileType(tile, TileType::Industry) ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	const Industry *industry = Industry::GetIfValid(GetIndustryIndex(tile));
	if (industry == nullptr) return std::nullopt;
	const auto layer = GetTileLayer(tile);
	const auto *capability = GetLayerIndustryCapability(industry->type);
	// Exact native layer/definition ownership prevents a numeric tile or art
	// name from granting source authority to a Surface/foreign mineral layout.
	if (capability == nullptr || !capability->layers.Test(layer) ||
		GetTileLayer(industry->location.tile) != layer) return std::nullopt;
	const auto family = FindSelectedGroundMine(capability->asset_id);
	if (!family) return std::nullopt;
	// Source artwork is qualified only for the actual nine-cell machinery
	// rectangle. Loading floor is ordinary floor and never acquires mine parts.
	if (industry->location.w != 3 || industry->location.h != 3) return std::nullopt;
	const uint x = TileX(tile) - TileX(industry->location.tile);
	const uint y = TilePlaneY(tile) - TilePlaneY(industry->location.tile);
	if (x >= 3 || y >= 3) return std::nullopt; // Reject a detached/stray owner tile instead of wrapping its source cell.
	const auto status = GetLayerMineOperationalStatus(*industry);
	if (status == LayerMineOperationalStatus::MissingState) return std::nullopt;
	MultiLayerSpritePose pose;
	pose.tile = static_cast<uint16_t>(y * 3 + x);
	pose.part = static_cast<uint8_t>(front);
	// A completed cell keeps the completed construction image while another
	// machinery cell holds the whole-mine gate. No part animates extraction
	// before the native finite producer reports actual operating availability.
	if (!IsIndustryCompleted(tile) || status == LayerMineOperationalStatus::Construction) {
		pose.state = 0;
		pose.frame = GetIndustryConstructionStage(tile);
	} else {
		switch (status) {
			case LayerMineOperationalStatus::Operating:
				pose.state = 1;
				pose.frame = static_cast<uint8_t>((visual_ticks / 8) % 4);
				break;
			case LayerMineOperationalStatus::Depleted:
				pose.state = 3;
				break;
			case LayerMineOperationalStatus::Unavailable:
			case LayerMineOperationalStatus::Abandoned:
			case LayerMineOperationalStatus::Paused:
			case LayerMineOperationalStatus::Stopped:
			case LayerMineOperationalStatus::OutputFull:
			case LayerMineOperationalStatus::AwaitPickup:
				pose.state = 2;
				break;
			default: return std::nullopt; // A new/invalid native status needs an explicit source mapping before it may display operation.
		}
	}
	return GetGroundSprite(*family, pose);
}
