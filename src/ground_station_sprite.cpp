/**
 * @file ground_station_sprite.cpp Native station geometry and actual loading chains select accepted source pixels.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FG.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "stdafx.h"
#include "ground_station_sprite.h"
#include "conventional_station_art.h"
#include "conventional_terminal_art.h"
#include "station_func.h"
#include "station_base.h"
#include "station_map.h"
#include "vehicle_base.h"

#include "safeguards.h"

/** @copydoc GetGroundStationArtLayout */
std::optional<GroundStationArtLayout> GetGroundStationArtLayout(StationType type, uint8_t graphics, uint8_t state, uint8_t frame)
{
	const bool rail = type == StationType::Rail;
	// Numeric graphics overlap among native station families; only these two
	// qualified immutable default producers can supply the selected source art.
	if (!rail && type != StationType::Truck) return std::nullopt;
	if (rail) {
		if (!GetConventionalRailStationArtGeometry(graphics)) return std::nullopt;
	} else {
		if (!GetConventionalRoadTerminalArtGeometry(graphics)) return std::nullopt;
	}
	const auto *native = GetStationTileLayout(type, graphics);
	const auto sequence = native->GetSequence();
	GroundStationArtLayout result;
	result.ground = native->ground;
	if (sequence.size() > result.sequence.size()) return std::nullopt;
	result.count = static_cast<uint8_t>(sequence.size());
	const uint8_t view = rail ? graphics & 1 : graphics;
	const uint16_t variant = rail ? graphics / 2 : 0;
	// Each image remains attached to its actual native parent/child. Canopy
	// glass keeps its elevated roof parent and transparency palette; absent
	// roofs/through-stop third parents never acquire a transparent sorting box.
	for (uint8_t part = 0; part < result.count; ++part) {
		const auto sprite = GetGroundSprite(rail ? GroundSpriteFamily::FacilityRailStation : GroundSpriteFamily::FacilityRoadTerminal,
			{state, frame, view, variant, part});
		if (!sprite) return std::nullopt;
		result.sequence[part] = sequence[part];
		result.sequence[part].image.sprite = (sequence[part].image.sprite & ~SPRITE_MASK) | *sprite;
	}
	return result;
}

/** @copydoc GetGroundStationTileLayout */
std::optional<GroundStationArtLayout> GetGroundStationTileLayout(TileIndex tile, uint64_t visual_ticks)
{
	// Public native geometry owners retain all profile/layer/custom/tube/type
	// admission. A nearby station ID or shared graphics number grants no art.
	if (!GetConventionalRailStationTileArtGeometry(tile) && !GetConventionalRoadTerminalTileArtGeometry(tile)) return std::nullopt;
	const auto *station = Station::GetIfValid(GetStationIndex(tile));
	if (station == nullptr) return std::nullopt;
	const VehicleType expected = IsRailStationTile(tile) ? VehicleType::Train : VehicleType::Road;
	bool working = false;
	// The actual station loading list owns activity; inspect each physical part
	// of the matching real chain so a loading train can span several cells.
	// Off-platform parts and loading another cell cannot animate this tile.
	for (const Vehicle *head : station->loading_vehicles) {
		if (head->type != expected) continue;
		for (const Vehicle *part = head; part != nullptr; part = part->Next()) {
			if (part->tile != tile) continue;
			working = true;
			break;
		}
		if (working) break;
	}
	// Native commands build these default cells atomically. Only ready/working
	// are installed states; source construction/refusal artwork stays preview-only.
	const uint8_t state = working ? 2 : 1;
	const uint8_t frame = working ? static_cast<uint8_t>((visual_ticks / 8) % 4) : 0;
	return GetGroundStationArtLayout(GetStationType(tile), GetStationGfx(tile), state, frame);
}
