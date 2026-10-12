/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_sprite.h Pure native M1 presentation selectors; gameplay remains with the existing feature owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups81B/82/83/79C
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#ifndef MULTILAYER_SPRITE_H
#define MULTILAYER_SPRITE_H

#include "table/sprites.h"
#include "tile_type.h"
#include "direction_type.h"
#include "engine_type.h"
#include <optional>
#include <array>

/** Existing actual train owner; presentation never constructs a substitute vehicle. */
struct Train;
/** Existing native industry owner; its inventory is never copied into a graphics store. */
struct Industry;
/** Existing renderer-owned spatial sorting box. */
struct SpriteBounds;
/** Existing cargo definition owner, including the NewGRF that supplies its meaning. */
struct CargoSpec;

/** One transient visual pose in the published state/frame/direction/tile/part order; never saved simulation state. */
struct MultiLayerSpritePose {
	uint8_t state = 0; ///< Visual state derived by the caller from its authoritative feature query.
	uint8_t frame = 0; ///< Authored visual phase or stable terrain texture variant.
	uint8_t direction = 0; ///< Eight train views, four portal/depot approaches, or zero for fixed ground/UI/receivers.
	uint16_t tile = 0; ///< Physical receiver cell or independent portal endpoint.
	uint8_t part = 0; ///< Body/support or aligned back/front part.
};

/**
 * Resolve one source-authored pose without querying or changing simulation state.
 * @param asset Selected published family; End and unknown identities refuse.
 * @param pose Transient visual indices supplied by the actual consumer's query.
 * @return Exact native sprite identity, or no identity for an undeclared pose.
 */
inline std::optional<SpriteID> GetMultiLayerSprite(MultiLayerSpriteAsset asset, const MultiLayerSpritePose &pose)
{
	const size_t index = static_cast<size_t>(asset);
	if (index >= _multilayer_m1_sprite_ranges.size()) return std::nullopt;
	const auto &range = _multilayer_m1_sprite_ranges[index];
	// Refuse an undeclared view rather than wrapping into another state, physical
	// cell or family. These boundaries protect portal openings and wagon load cues.
	if (pose.state >= range.states || pose.frame >= range.frames || pose.direction >= range.directions || pose.tile >= range.tiles || pose.part >= range.parts) {
		return std::nullopt;
	}
	const uint offset = ((((pose.state * range.frames + pose.frame) * range.directions + pose.direction) * range.tiles + pose.tile) * range.parts + pose.part);
	return SPR_MAP_LAYER_BASE + range.offset + offset;
}

/**
 * Select an authored visual phase without advancing a work or cargo owner.
 * @param asset Selected animated family; static/UI/portal families have one phase.
 * @param visual_ticks Read-only presentation clock; changing it only chooses pixels.
 * @param moving Actual movement of the wagon/vehicle; stopped hopper wheels stay still.
 * @return Authored frame index; static ground must use GetMultiLayerTerrainTexture instead.
 */
inline uint8_t GetMultiLayerVisualFrame(MultiLayerSpriteAsset asset, uint64_t visual_ticks, bool moving)
{
	const size_t index = static_cast<size_t>(asset);
	if (index >= _multilayer_m1_sprite_ranges.size()) return 0;
	// The three base-ground frames are stable texture variants. Drawing them must
	// never turn the floor into an animated terrain or imply additional paid work.
	if (asset == MultiLayerSpriteAsset::TerrainUnderground || asset == MultiLayerSpriteAsset::TerrainDeep) return 0;
	// Load level comes from actual packets even when stopped. Only wheel motion
	// stops with the train; stationary cargo must not cycle through empty/full art.
	if ((asset == MultiLayerSpriteAsset::VehicleModel02 || asset == MultiLayerSpriteAsset::VehicleModel03) && !moving) return 0;
	return static_cast<uint8_t>((visual_ticks / 8) % _multilayer_m1_sprite_ranges[index].frames);
}

/**
 * Choose stable floor texture without simulation RNG, time or camera input.
 * @param x Layer-local tile x coordinate from the existing map geometry owner.
 * @param y Layer-local tile y coordinate from the existing map geometry owner.
 * @return One of the three authored texture variants, identical across depths/views/reloads.
 */
inline uint8_t GetMultiLayerTerrainTexture(uint32_t x, uint32_t y)
{
	return static_cast<uint8_t>((x * 17U + y * 31U) % 3U);
}

/**
 * Query authored solid/work/cavity ground and neighbor rims through live geology.
 * @param tile Explicit world tile; no camera layer or local copied excavation state.
 * @return One authored ground sprite, or no override for legacy/non-rock/invalid ground.
 */
std::optional<SpriteID> GetMultiLayerTerrainSprite(TileIndex tile);

/**
 * Read the four exposed walls surrounding paid open floor from adjacent rock.
 * @param tile Actual geological floor tile on its explicit layer.
 * @return Optional upper-left, upper-right, lower-right and lower-left overlays; open joins have no wall.
 */
std::array<std::optional<SpriteID>, 4> GetMultiLayerTerrainEdgeSprites(TileIndex tile);

/**
 * Query a rail portal's actual saved phase, door and independent endpoint.
 * @param tile Existing reciprocal rail portal housing in the selected native profile.
 * @param front True for the authored aligned front/mouth, false for the back.
 * @return Exact portal part, or no override for Classic, road or invalid geometry.
 */
std::optional<SpriteID> GetMultiLayerPortalSprite(TileIndex tile, bool front);

/**
 * Query a real native cutter/support, hopper or hauler pose.
 * @param train Actual physical part; role and cargo remain with the saved train/controller owners.
 * @param direction Body view supplied by the existing train renderer, including its flip policy.
 * @param visual_ticks Read-only phase clock, or zero for static GUI previews.
 * @return Authored native pose, or no override for ordinary/foreign/invalid vehicles.
 */
std::optional<SpriteID> GetMultiLayerTrainSprite(const Train &train, Direction direction, uint64_t visual_ticks);

/**
 * Invalidate only derived native art caches on the existing front-engine tick.
 * @param head Actual front engine whose ordinary tick already owns its chain traversal.
 * @param visual_ticks Read-only phase clock; refresh only at authored eight-tick boundaries.
 * @return None; never advances work, changes packets, charges money or consumes RNG.
 */
void RefreshMultiLayerTrainArt(Train &head, uint64_t visual_ticks);

/**
 * Query an actual Underground/Deep rail service depot or designated exchange bay.
 * @param tile Existing native depot; Surface and unrelated tile families retain their existing art.
 * @param front True for the authored aligned foreground, false for its back/floor.
 * @param visual_ticks Read-only lamp phase clock, independent of exchange transactions.
 * @return Exact native facility pose or no override outside the selected scope.
 */
std::optional<SpriteID> GetMultiLayerRailDepotSprite(TileIndex tile, bool front, uint64_t visual_ticks);

/**
 * Query the selected actual underground road service depot.
 * @param tile Existing native road depot; foreign/Surface/non-depot scope receives no override.
 * @param front True for its aligned foreground part.
 * @param visual_ticks Read-only lamp clock, never vehicle servicing time.
 * @return Native service pose, or no selected-family override.
 */
std::optional<SpriteID> GetMultiLayerRoadDepotSprite(TileIndex tile, bool front, uint64_t visual_ticks);

/**
 * Query a native Earth/Rock receiver's exact physical cell and stock state.
 * @param tile Existing industry cell; its native kind and inventory supply authority.
 * @param front True for the authored foreground, false for the back/floor.
 * @param visual_ticks Read-only lamp clock; it never processes a receiver interval.
 * @return Selected native receiver part, or no override for another industry.
 */
std::optional<SpriteID> GetMultiLayerReceiverSprite(TileIndex tile, bool front, uint64_t visual_ticks);

/**
 * Query an actual paid collector's physical cell, construction stage and current Heat state.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79C
 * @param tile Existing native collector industry cell, independent of camera selection.
 * @param front True for its aligned foreground, false for the source back/floor.
 * @param visual_ticks Read-only equipment phase clock; paid construction uses its native stage.
 * @return Exact packed collector pose, or no override for absent/foreign geometry.
 */
std::optional<SpriteID> GetMultiLayerCollectorSprite(TileIndex tile, bool front, uint64_t visual_ticks);

/**
 * Resolve an actual selected industry through its existing receiver or collector owner.
 * @param tile Native industry cell; other industry families keep their usual resolver.
 * @param front True for its aligned foreground part, false for its back/floor.
 * @param visual_ticks Read-only equipment clock shared by both parts of one draw.
 * @return Selected native industry pose or no override for another family/profile.
 */
std::optional<SpriteID> GetMultiLayerIndustrySprite(TileIndex tile, bool front, uint64_t visual_ticks);

/**
 * Return the shared native depot mouth/roof sorting geometry.
 * @param door Valid existing NE/SE/SW/NW door orientation.
 * @return Far full-roof or near door-edge bounds; sprite geometry grants no gameplay footprint.
 */
const SpriteBounds &GetMultiLayerDepotFrontBounds(DiagDirection door);

/**
 * Select an authored ground-control state using the actual widget's availability.
 * @param asset One of the seven selected ground-control families.
 * @param disabled Existing UI/command availability, never permission conferred by this selector.
 * @param pressed Actual cursor/button selection; a disabled control always keeps its disabled cue.
 * @return Native control icon, or no override for Classic or another family.
 */
std::optional<SpriteID> GetMultiLayerControlSprite(MultiLayerSpriteAsset asset, bool disabled, bool pressed);

/**
 * Select a static purchase preview without changing the model's calendar or capability gates.
 * @param model Catalogue model zero through four: two cutters, two spoil hoppers and the actual hauler.
 * @param available Availability supplied by the existing purchase/commissioning owner.
 * @return Available/locked UI sprite, or no selected-family override.
 */
std::optional<SpriteID> GetMultiLayerPurchaseSprite(uint16_t model, bool available);

/**
 * Resolve a real engine's selected purchase family through the persisted native mappings.
 * @param engine Actual engine identity, never a fabricated catalogue/GRF-local identity.
 * @param available Existing engine/company purchase query result.
 * @return Native available/locked preview, or no override for foreign/unselected engines.
 */
std::optional<SpriteID> GetMultiLayerEnginePurchaseSprite(EngineID engine, bool available);

/**
 * Resolve a first-party spoil cargo's presentation through its real definition owner.
 * @param cargo Actual cargo definition; foreign definitions with the same label keep their own graphics.
 * @return Native Earth/Rock icon, or no override for another definition/profile.
 */
std::optional<SpriteID> GetMultiLayerCargoSprite(const CargoSpec &cargo);

/**
 * Query the actual seam at an explicit world tile without surveying it or consuming reserve.
 * @param tile Existing geological world tile, independent of camera position.
 * @param surveyed Information visibility chosen by the current survey owner; Native's public/free survey uses true.
 * @return Resource marker in unknown/surveyed/depleted state, or no override outside a selected seam.
 */
std::optional<SpriteID> GetMultiLayerSurveySprite(TileIndex tile, bool surveyed);

#endif /* MULTILAYER_SPRITE_H */
