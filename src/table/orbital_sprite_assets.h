/** @file orbital_sprite_assets.h Generated orbital native ranges.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K
 */
#ifndef ORBITAL_SPRITE_ASSETS_H
#define ORBITAL_SPRITE_ASSETS_H

#include <array>
#include <cstdint>

/** Appended source families only; no saved gameplay identity or capability. */
enum class OrbitalSpriteAsset : uint8_t {
	VehicleModel21, ///< vehicle.model-21
	VehicleModel16, ///< vehicle.model-16
	VehicleModel17, ///< vehicle.model-17
	ModuleNucleus, ///< module.nucleus
	ModuleDock, ///< module.dock
	ModuleStorage, ///< module.storage
	ModulePower, ///< module.power
	ModuleFuelStore, ///< module.fuel-store
	ModuleWorkshop, ///< module.workshop
	ModuleProcessor, ///< module.processor
	End, ///< Number of appended orbital source families.
};

/** Native range with state/frame/direction/world-tile/part ordering. */
struct OrbitalSpriteRange {
	uint16_t offset; ///< Relative native Map Layer block offset.
	uint16_t count; ///< Complete logical part count, including honest absence/aliases.
	uint8_t states; ///< Published physical/functional states.
	uint8_t frames; ///< Authored phases and explicit still aliases.
	uint8_t directions; ///< Eight craft views or one fixed site view.
	uint8_t tiles; ///< Real native world cells; one for craft.
	uint8_t parts; ///< Source body or back/front parts.
};

static constexpr uint16_t ORBITAL_SPRITE_START = 5167; ///< Every prior native sprite precedes this append.
static constexpr uint16_t ORBITAL_SPRITE_END = 6143; ///< Exclusive final orbital native sprite bound.
static constexpr std::array<OrbitalSpriteRange, 10> _orbital_sprite_ranges = {{
	{5167, 128, 8, 2, 8, 1, 1}, // vehicle.model-21
	{5295, 80, 5, 2, 8, 1, 1}, // vehicle.model-16
	{5375, 160, 10, 2, 8, 1, 1}, // vehicle.model-17
	{5535, 144, 4, 2, 1, 9, 2}, // module.nucleus
	{5679, 64, 4, 2, 1, 4, 2}, // module.dock
	{5743, 64, 4, 2, 1, 4, 2}, // module.storage
	{5807, 64, 4, 2, 1, 4, 2}, // module.power
	{5871, 64, 4, 2, 1, 4, 2}, // module.fuel-store
	{5935, 64, 4, 2, 1, 4, 2}, // module.workshop
	{5999, 144, 4, 2, 1, 9, 2}, // module.processor
}};

#endif /* ORBITAL_SPRITE_ASSETS_H */
