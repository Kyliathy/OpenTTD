/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_sprite.h Pure native guideway, capsule packet/status and actual purchase presentation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups13B-13CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TRANSPORT_TUBE_SPRITE_H
#define TRANSPORT_TUBE_SPRITE_H

#include "tile_type.h"
#include "transport_tube_facility_sprite.h"
#include "sprite.h"
#include "direction_type.h"
#include "engine_type.h"
#include "track_type.h"
#include <array>
#include <optional>

/** Actual native physical train part; its packets, physics and source binding remain vehicle-owned. */
struct Train;

/** Derived original external-service branches, with no saved path, energy or routing authority. */
struct TubeConduitArt {
	std::array<std::optional<SpriteID>, 4> ports{}; ///< Only actual previous/next cardinal path cells, in native DiagDirection order.
};

/**
 * Select ground branches from active original external service corridors.
 * @param tile Explicit excavated rock-layer world cell, independent of the selected camera.
 * @return Actual installed ports with current original publication state, or no physical installation.
 * Native purchased geothermal cables retain their separate geothermal.conduit family; this query never paints inferred wires.
 */
std::optional<TubeConduitArt> GetTubeConduitArt(TileIndex tile);

/** Clear only the derived branch index after native source/path topology reset; no canonical owner changes. */
void ResetTubeConduitArtCache();

/** @param tile Actual viewport ground cell. Draw only original conduit branches beneath vehicles and facility buildings. */
void DrawTubeConduitGround(TileIndex tile);

/** Aligned authored casing layers; the complete back lies beneath vehicles and the aperture retains the near rim. */
struct TubeGuidewaySpritePart {
	SpriteID back = 0; ///< Complete original shell at its packed ground anchor.
	SpriteID front = 0; ///< Exact near-rim subset at the identical anchor.
	SpriteBounds front_bounds{}; ///< Actual guideway piece's spatial sorter box; facility parts retain their separate native facility bounds.
};

/** Transient bounded tile presentation, never a saved network, allocation or capsule owner. */
struct TubeGuidewayArt {
	std::array<TubeGuidewaySpritePart, 6> parts{}; ///< At most two independent native pieces and four physical ends.
	uint8_t count = 0; ///< Complete admitted prefix of aligned casing pairs.
	bool energized = false; ///< Original current publication and physical reach, never a promise of a capsule grant.
};

/** Pure actual facility parts with one aligned native sorting owner; no station/depot or packet state is copied. */
struct TubeFacilityArt {
	TubeGuidewaySpritePart sprites; ///< Whole original back and exact low near-ledge/front subset.
	SpriteBounds front_bounds; ///< Native sorting box whose origin/offset retain the packed building anchor.
};

/**
 * Read current original publication/path reach for actual native tube track or a facility.
 * @param tile Existing excavated rock-layer native tube cell.
 * @param tracks Complete installed plain-track pieces, or empty for the native facility's single through path.
 * @return Whether current positive original output physically reaches this cell, never a capsule work grant.
 */
bool IsTubeTileEnergized(TileIndex tile, TrackBits tracks = {});

/**
 * Select actual depot/terminal/interchange parts from original native geometry, packets, service and source queries.
 * @param tile Actual built native facility cell; custom station graphics retain their own renderer.
 * @param visual_ticks Existing read-only lamp clock, never a loading or repair opportunity.
 * @return Packed aligned parts and native foreground bounds, or no override outside the selected geometry.
 */
std::optional<TubeFacilityArt> GetTubeFacilityArt(TileIndex tile, uint64_t visual_ticks);



/**
 * Select only declared packed shells for one actual flat native tube tile.
 * @param tile Explicit native world address; Classic, facilities, ordinary track and nonexcavated/invalid geometry refuse.
 * @return Complete bounded back/front parts and pure original supply indication, or no override.
 * Queries may rebuild derived connectivity but never publish power, alter cash/packets or acquire work rights.
 */
std::optional<TubeGuidewayArt> GetTubeGuidewayArt(TileIndex tile);

/**
 * Return default near-corner bounds for straight, shared and end foreground pieces.
 * @return Original shared box retaining the ground anchor; curves use their actual quarter through each part's front_bounds.
 */
const SpriteBounds &GetTubeGuidewayFrontBounds();

/**
 * Read one actual capsule's packet level, native service demand and power/path wait.
 * @param train Actual physical capsule part; its front owns shared work/status while this part owns its load.
 * @param direction Existing train renderer's body facing, after its ordinary flip policy.
 * @param visual_ticks Read-only status-lamp clock; its two phases never move the hull or earn work.
 * @return Packed native model/load/status pose, or no override for foreign/ordinary/invalid scope.
 */
std::optional<SpriteID> GetTubeTrainSprite(const Train &train, Direction direction, uint64_t visual_ticks);

/**
 * Select the actual native capsule engine's purchase identity without granting availability.
 * @param engine Existing original native engine, resolved through the canonical capability/mapping owner.
 * @param available Existing company/date purchase owner's result; false selects the authored locked preview.
 * @return Exact original model's available/locked preview, or no override outside the five native families.
 */
std::optional<SpriteID> GetTubeEnginePurchaseSprite(EngineID engine, bool available);

#endif /* TRANSPORT_TUBE_SPRITE_H */
