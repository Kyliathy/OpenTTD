/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file orbital_sprite.h Read-only presentation for the appended orbital source families.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */
#ifndef ORBITAL_SPRITE_H
#define ORBITAL_SPRITE_H

#include "direction_type.h"
#include "table/sprites.h"
#include "company_type.h"
#include "sprite.h"
#include <optional>

/** Original native hull; presentation cannot issue or replace its identity. */
struct SpaceVehicle;
/** Original generation-qualified module and construction inventory owner. */
struct SpaceStructure;

/** Transient source indices only, never saved work, quantity, orientation or capability. */
struct OrbitalArtPose {
	OrbitalSpriteAsset asset = OrbitalSpriteAsset::VehicleModel21; ///< Existing appended source family.
	uint8_t state = 0; ///< Published physical or functional state in that exact family.
	uint8_t frame = 0; ///< Original work/travel phase or explicit canonical still.
	Direction direction = Direction::N; ///< Eight genuine craft views; fixed sites use ordinal zero only.
	uint8_t tile = 0; ///< Actual fixed-site world cell; craft use their sole cell.
	uint8_t part = 0; ///< Body/back is zero; fixed-site foreground is one.
};

/**
 * Resolve a declared orbital source part without consulting or changing gameplay.
 * @param pose Exact existing family/state/frame/view/world-cell/part indices.
 * @return Appended native sprite, or absence for any undeclared source index.
 */
inline std::optional<SpriteID> ResolveOrbitalSprite(const OrbitalArtPose &pose)
{
	const auto index = static_cast<size_t>(pose.asset);
	if (index >= _orbital_sprite_ranges.size()) return std::nullopt;
	const auto &range = _orbital_sprite_ranges[index];
	// Invalid indices cannot wrap into another paid site's tile, another hull's
	// role or a neighbouring foreground; only the published source matrix resolves.
	if (pose.state >= range.states || pose.frame >= range.frames || to_underlying(pose.direction) >= range.directions ||
		pose.tile >= range.tiles || pose.part >= range.parts) return std::nullopt;
	const uint offset = ((((pose.state * range.frames + pose.frame) * range.directions + to_underlying(pose.direction)) * range.tiles + pose.tile) * range.parts) + pose.part;
	return SPR_MAP_LAYER_BASE + range.offset + offset;
}

/**
 * Select actual heavy21, retained16 or hauler17 art from original native owners.
 * @param craft Original native hull, packets, mission and typed queue; no inputs are granted.
 * @param direction Existing renderer view, already resolved from world geometry for map use.
 * @return Authored native pose, or absence for another role or invalid view.
 */
std::optional<OrbitalArtPose> GetOrbitalCraftArtPose(const SpaceVehicle &craft, Direction direction);

/** Select a real module cell without changing stage, inventory, job or actor.
 * @param site Actual original structure and its physical construction/facility owners.
 * @param observer Company requesting presentation; private work queries require its existing owner context.
 * @param tile Row-major original footprint cell, never a repeated whole building.
 * @param part Original back zero or genuine near-jaw foreground one.
 * @return Exact fixed-view pose, or absence outside the source/physical domain.
 */
std::optional<OrbitalArtPose> GetOrbitalModuleArtPose(const SpaceStructure &site, CompanyID observer, uint8_t tile, uint8_t part);

/** Keep native cell anchors while sorting genuine near jaws above visiting hulls.
 * @param site Original fixed structure, whose declared footprint owns the source cuts.
 * @param tile Row-major original world cell.
 * @param part Back zero or near-jaw foreground one.
 * @return Native sorter bounds with inverse drawing compensation, or absence for invalid cells/parts.
 */
std::optional<SpriteBounds> GetOrbitalModuleArtBounds(const SpaceStructure &site, uint8_t tile, uint8_t part);

#endif /* ORBITAL_SPRITE_H */
