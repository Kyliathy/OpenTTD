/** @file ground_sprite_assets.h Generated selected ground artwork ranges.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef GROUND_SPRITE_ASSETS_H
#define GROUND_SPRITE_ASSETS_H

#include <array>
#include <cstdint>
#include "launch_fleet_sprite_assets.h"

/** Stable append-local family identities; all prior native enum values stay unchanged. */
enum class GroundSpriteFamily : uint8_t {
	MineCOAL = 0, ///< mine.COAL.
	MineIORE = 1, ///< mine.IORE.
	MineCORE = 2, ///< mine.CORE.
	MineSALT = 3, ///< mine.SALT.
	MineLITH = 4, ///< mine.LITH.
	MineCOBT = 5, ///< mine.COBT.
	MineRARE = 6, ///< mine.RARE.
	FacilityMetalRefinery = 7, ///< facility.metal-refinery.
	FacilityChemicalWorks = 8, ///< facility.chemical-works.
	FacilityBatteryWorks = 9, ///< facility.battery-works.
	FacilityElectronicsWorks = 10, ///< facility.electronics-works.
	VehicleModel05 = 11, ///< vehicle.model-05.
	PurchaseModel05 = 12, ///< purchase.model-05.
	VehicleModel06 = 13, ///< vehicle.model-06.
	PurchaseModel06 = 14, ///< purchase.model-06.
	VehicleModel07 = 15, ///< vehicle.model-07.
	PurchaseModel07 = 16, ///< purchase.model-07.
	VehicleModel08 = 17, ///< vehicle.model-08.
	PurchaseModel08 = 18, ///< purchase.model-08.
	VehicleModel09 = 19, ///< vehicle.model-09.
	PurchaseModel09 = 20, ///< purchase.model-09.
	VehicleModel10 = 21, ///< vehicle.model-10.
	PurchaseModel10 = 22, ///< purchase.model-10.
	VehicleModel11 = 23, ///< vehicle.model-11.
	PurchaseModel11 = 24, ///< purchase.model-11.
	VehicleModel12 = 25, ///< vehicle.model-12.
	PurchaseModel12 = 26, ///< purchase.model-12.
	VehicleModel13 = 27, ///< vehicle.model-13.
	PurchaseModel13 = 28, ///< purchase.model-13.
	FacilityRailStation = 29, ///< facility.rail-station.
	FacilityRoadTerminal = 30, ///< facility.road-terminal.
	FacilityTubeTerminal = 31, ///< facility.tube-terminal.
	FacilityTubeService = 32, ///< facility.tube-service.
	FacilityTubeInterchange = 33, ///< facility.tube-interchange.
	FacilityVentPowerhouse = 34, ///< facility.vent-powerhouse.
	FacilityMagmaPowerhouse = 35, ///< facility.magma-powerhouse.
	FacilityUtilityRiser = 36, ///< facility.utility-riser.
	GeothermalVent = 37, ///< geothermal.vent.
	GeothermalConduit = 38, ///< geothermal.conduit.
	TubeStraight = 39, ///< tube.straight.
	TubeBend45 = 40, ///< tube.bend45.
	TubeJunction = 41, ///< tube.junction.
	TubeEnd = 42, ///< tube.end.
	TubeCrossing = 43, ///< tube.crossing.
	TubeConduit = 44, ///< tube.conduit.
	ControlTube = 45, ///< control.tube.
	ControlGeothermal = 46, ///< control.geothermal.
	End = 47, ///< Exclusive Ground family bound.
};

/** Canonical state/frame/view/cell/parent dimensions for one native source family. */
struct GroundSpriteRange {
	uint16_t begin; ///< Absolute native sprite offset.
	uint16_t count; ///< Complete Cartesian source pose count.
	uint8_t states; ///< Native state count.
	uint8_t frames; ///< Source phase count; no simulation clock or work authority.
	uint8_t directions; ///< Genuine declared source views.
	uint16_t tiles; ///< Actual native cell/variant count.
	uint8_t parts; ///< Independently parented source component count.
};

static constexpr uint16_t GROUND_SPRITE_START = 7791; ///< Complete preserved prefix precedes this append.
static constexpr uint16_t GROUND_SPRITE_END = 22045; ///< Exclusive Ground sprite bound.
static_assert(GROUND_SPRITE_START == LAUNCH_FLEET_SPRITE_END, "Ground sprites must follow the complete native prefix");
static constexpr std::array<GroundSpriteRange, 47> _ground_sprite_ranges = {{
	{7791, 288, 4, 4, 1, 9, 2}, // mine.COAL
	{8079, 288, 4, 4, 1, 9, 2}, // mine.IORE
	{8367, 288, 4, 4, 1, 9, 2}, // mine.CORE
	{8655, 288, 4, 4, 1, 9, 2}, // mine.SALT
	{8943, 288, 4, 4, 1, 9, 2}, // mine.LITH
	{9231, 288, 4, 4, 1, 9, 2}, // mine.COBT
	{9519, 288, 4, 4, 1, 9, 2}, // mine.RARE
	{9807, 512, 4, 4, 1, 16, 2}, // facility.metal-refinery
	{10319, 512, 4, 4, 1, 16, 2}, // facility.chemical-works
	{10831, 512, 4, 4, 1, 16, 2}, // facility.battery-works
	{11343, 512, 4, 4, 1, 16, 2}, // facility.electronics-works
	{11855, 1360, 5, 2, 8, 17, 1}, // vehicle.model-05
	{13215, 2, 2, 1, 1, 1, 1}, // purchase.model-05
	{13217, 80, 5, 2, 8, 1, 1}, // vehicle.model-06
	{13297, 2, 2, 1, 1, 1, 1}, // purchase.model-06
	{13299, 800, 5, 2, 8, 10, 1}, // vehicle.model-07
	{14099, 2, 2, 1, 1, 1, 1}, // purchase.model-07
	{14101, 80, 5, 2, 8, 1, 1}, // vehicle.model-08
	{14181, 2, 2, 1, 1, 1, 1}, // purchase.model-08
	{14183, 144, 9, 2, 8, 1, 1}, // vehicle.model-09
	{14327, 2, 2, 1, 1, 1, 1}, // purchase.model-09
	{14329, 144, 9, 2, 8, 1, 1}, // vehicle.model-10
	{14473, 2, 2, 1, 1, 1, 1}, // purchase.model-10
	{14475, 144, 9, 2, 8, 1, 1}, // vehicle.model-11
	{14619, 2, 2, 1, 1, 1, 1}, // purchase.model-11
	{14621, 144, 9, 2, 8, 1, 1}, // vehicle.model-12
	{14765, 2, 2, 1, 1, 1, 1}, // purchase.model-12
	{14767, 144, 9, 2, 8, 1, 1}, // vehicle.model-13
	{14911, 2, 2, 1, 1, 1, 1}, // purchase.model-13
	{14913, 512, 4, 4, 2, 4, 4}, // facility.rail-station
	{15425, 288, 4, 4, 6, 1, 3}, // facility.road-terminal
	{15713, 256, 4, 4, 2, 4, 2}, // facility.tube-terminal
	{15969, 128, 4, 4, 4, 1, 2}, // facility.tube-service
	{16097, 256, 4, 4, 2, 4, 2}, // facility.tube-interchange
	{16353, 2560, 5, 4, 4, 16, 2}, // facility.vent-powerhouse
	{18913, 2560, 5, 4, 4, 16, 2}, // facility.magma-powerhouse
	{21473, 128, 4, 4, 1, 4, 2}, // facility.utility-riser
	{21601, 12, 3, 4, 1, 1, 1}, // geothermal.vent
	{21613, 24, 3, 1, 4, 2, 1}, // geothermal.conduit
	{21637, 12, 3, 1, 2, 1, 2}, // tube.straight
	{21649, 24, 3, 1, 4, 1, 2}, // tube.bend45
	{21673, 324, 3, 1, 1, 54, 2}, // tube.junction
	{21997, 24, 3, 1, 4, 1, 2}, // tube.end
	{22021, 6, 3, 1, 1, 1, 2}, // tube.crossing
	{22027, 12, 3, 1, 4, 1, 1}, // tube.conduit
	{22039, 3, 3, 1, 1, 1, 1}, // control.tube
	{22042, 3, 3, 1, 1, 1, 1}, // control.geothermal
}};

#endif /* GROUND_SPRITE_ASSETS_H */
