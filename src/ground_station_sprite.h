/**
 * @file ground_station_sprite.h Selected conventional station images retain native layout and status ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FG.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef GROUND_STATION_SPRITE_H
#define GROUND_STATION_SPRITE_H

#include "ground_sprite.h"
#include "sprite.h"
#include "station_type.h"

/** Owned image substitution over the actual default sequence; copied/moved values never borrow another value's storage. */
struct GroundStationArtLayout {
	PalSpriteID ground{}; ///< Original ground identity; track and road renderers retain its existing ownership.
	std::array<DrawTileSeqStruct, 4> sequence{}; ///< Exact native parent/child order, with qualified source images only.
	uint8_t count = 0; ///< Actual native sequence length; absent parents are never invented.

	/**
	 * Borrow this value's layout for one immediate renderer call.
	 * @return Native drawing span; its lifetime is bounded by this owning value.
	 */
	DrawTileSpriteSpan View() const { return {this->ground, std::span(this->sequence.data(), this->count)}; }
};

/**
 * Substitute source images into qualified immutable native station geometry without querying a world.
 * @param type Actual default Rail or Truck station family; other native owners refuse.
 * @param graphics Native rail role/axis0..7 or truck entrance/through layout0..5.
 * @param state Accepted source state0..3; construction/blocked are preview-only for atomic builds.
 * @param frame Exact accepted source phase0..3, without wrapping invalid axes.
 * @return Owned native sequence with unchanged parents/child flags, or absence outside its source contract.
 */
std::optional<GroundStationArtLayout> GetGroundStationArtLayout(StationType type, uint8_t graphics, uint8_t state, uint8_t frame);

/**
 * Resolve an installed native station/terminal's real tile-local loading presentation.
 * @param tile Actual eligible default station cell; custom/Surface/Classic/tube/foreign owners refuse.
 * @param visual_ticks Existing synchronized phase clock, used only while an actual loading part occupies this tile.
 * @return Ready/working source sequence; no installed construction or persistent blocked state is fabricated.
 */
std::optional<GroundStationArtLayout> GetGroundStationTileLayout(TileIndex tile, uint64_t visual_ticks);

#endif /* GROUND_STATION_SPRITE_H */
