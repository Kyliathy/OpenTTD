/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file space_sprite.h Read-only final Space presentation from original native owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */
#ifndef SPACE_SPRITE_H
#define SPACE_SPRITE_H

#include "space.h"
#include "company_type.h"
#include "direction_type.h"
#include "table/sprites.h"
#include <optional>

/** Actual native fleet owner; presentation cannot create or replace it. */
struct SpaceVehicle;
/** Actual passive paid shell and sole optional descent-job owner. */
struct SpacePod;

/** Transient art indices only; never saved orientation, work, resource or custody. */
struct SpaceArtPose {
	SpaceSpriteAsset asset = SpaceSpriteAsset::VehicleModel15; ///< Existing source family with a fixed native append range.
	uint8_t state = 0; ///< Visual state selected from original native role/work owners.
	uint8_t frame = 0; ///< Actual progress phase or declared static alias.
	Direction direction = Direction::N; ///< Genuine original viewport view, without rotating an image.
	uint8_t size = 0; ///< Existing saved asteroid size; craft and pods use their sole variant.
};

/**
 * Resolve a declared source pose without consulting any gameplay owner.
 * @param pose Explicit existing family/state/frame/view/size presentation indices.
 * @return Exact native sprite, or absence for an undeclared source pose.
 */
inline std::optional<SpriteID> ResolveSpaceSprite(const SpaceArtPose &pose)
{
	const auto index = static_cast<size_t>(pose.asset);
	if (index >= _space_sprite_ranges.size()) return std::nullopt;
	const auto &range = _space_sprite_ranges[index];
	// Undeclared indices must refuse rather than wrap into another family's state or saved size; a believable neighbouring sprite cannot supply missing source coverage.
	if (pose.state >= range.states || pose.frame >= range.frames || to_underlying(pose.direction) >= range.directions || pose.size >= range.sizes) return std::nullopt;
	const uint offset = ((pose.state * range.frames + pose.frame) * range.directions + to_underlying(pose.direction)) * range.sizes + pose.size;
	return SPR_MAP_LAYER_BASE + range.offset + offset;
}

/**
 * Choose an eight-view direction from existing plane geometry without saving a heading.
 * @param from Actual native plane position, in canonical Q16 tile coordinates.
 * @param to Actual resolved navigation/attachment destination in the same geometry.
 * @param fallback Existing native facing used when both positions coincide.
 * @return Viewport-aligned view under the fixed native2:1 projection.
 */
Direction GetSpaceArtDirection(SpacePoint from, SpacePoint to, Direction fallback);

/**
 * Derive craft facing through its current physical target or retained native direction.
 * @param craft Original actual native hull; all target queries are read-only.
 * @return Genuine source view without modifying inherited Vehicle direction or motion.
 */
Direction GetSpaceCraftArtDirection(const SpaceVehicle &craft);

/**
 * Query one of the three delivered probe families using native work and cargo admission.
 * @param craft Original paid or retained legacy hull; no commissioning or stock is granted.
 * @param direction Existing renderer-requested view, already resolved for on-map use.
 * @return Actual source pose for model15/19/20, or absence for another role's later art gate.
 */
std::optional<SpaceArtPose> GetSpaceCraftArtPose(const SpaceVehicle &craft, Direction direction);

/**
 * Query an original body's class, saved size and authorized finite-resource appearance.
 * @param id Monotonic native body identity; no copied body or resource registry.
 * @param observer Actual viewing company; unknown observers retain the normal body.
 * @return Fixed identity-based view and native state, or absence for a missing body.
 */
std::optional<SpaceArtPose> GetSpaceAsteroidArtPose(OrbitalBodyID id, CompanyID observer);

/**
 * Query a visible original passive shell without drawing a stored or carried duplicate.
 * @param pod Actual native pod and its sole optional live descent job.
 * @return Drifting/descent/recovery-ready pose, or absence while internally stored/carried.
 */
std::optional<SpaceArtPose> GetSpacePodArtPose(const SpacePod &pod);

#endif /* SPACE_SPRITE_H */
