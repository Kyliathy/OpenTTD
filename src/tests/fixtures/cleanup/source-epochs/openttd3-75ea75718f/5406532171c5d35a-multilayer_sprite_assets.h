/**
 * @file multilayer_sprite_assets.h Generated M1 native graphics ranges; do not edit.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/81B/79C
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */
#ifndef MULTILAYER_SPRITE_ASSETS_H
#define MULTILAYER_SPRITE_ASSETS_H

#include <array>
#include <cstdint>

/** Selected M1 graphics identities; saved engine/cargo/industry IDs stay with their existing owners. */
enum class MultiLayerSpriteAsset : uint8_t {
	TerrainUnderground, ///< terrain.underground.
	TerrainDeep, ///< terrain.deep.
	SurveyCOAL, ///< survey.COAL.
	SurveyIORE, ///< survey.IORE.
	SurveyCORE, ///< survey.CORE.
	SurveyBAUX, ///< survey.BAUX.
	SurveySALT, ///< survey.SALT.
	SurveyLITH, ///< survey.LITH.
	SurveyNORE, ///< survey.NORE.
	SurveyCOBT, ///< survey.COBT.
	SurveyRARE, ///< survey.RARE.
	SurveyURAN, ///< survey.URAN.
	VehicleModel00, ///< vehicle.model-00.
	PurchaseModel00, ///< purchase.model-00.
	VehicleModel01, ///< vehicle.model-01.
	PurchaseModel01, ///< purchase.model-01.
	VehicleModel02, ///< vehicle.model-02.
	PurchaseModel02, ///< purchase.model-02.
	VehicleModel03, ///< vehicle.model-03.
	PurchaseModel03, ///< purchase.model-03.
	VehicleModel04, ///< vehicle.model-04.
	PurchaseModel04, ///< purchase.model-04.
	FacilityPortalSealed, ///< facility.portal.sealed.
	FacilityPortalDeepRail, ///< facility.portal.deep-rail.
	FacilityExchange, ///< facility.exchange.
	FacilityRailService, ///< facility.rail-service.
	FacilityRoadService, ///< facility.road-service.
	FacilityEarthReceiver, ///< facility.earth-receiver.
	FacilityRockReceiver, ///< facility.rock-receiver.
	ControlSurvey, ///< control.survey.
	ControlExcavation, ///< control.excavation.
	ControlTbmArea, ///< control.tbm-area.
	ControlTbmTurn45, ///< control.tbm-turn45.
	ControlExchange, ///< control.exchange.
	ControlDeepLock, ///< control.deep-lock.
	ControlFutureLock, ///< control.future-lock.
	CargoERTH, ///< cargo.ERTH.
	CargoROCK, ///< cargo.ROCK.
	TerrainEdgesUnderground, ///< terrain.edges.underground.
	TerrainEdgesDeep, ///< terrain.edges.deep.
	TerrainHardness, ///< terrain.hardness.
	TerrainHardnessEdges, ///< terrain.hardness-edges.
	FacilityVentCollector, ///< facility.vent-collector.
	MineCOAL, ///< mine.COAL.
	MineIORE, ///< mine.IORE.
	MineCORE, ///< mine.CORE.
	MineSALT, ///< mine.SALT.
	MineLITH, ///< mine.LITH.
	MineCOBT, ///< mine.COBT.
	MineRARE, ///< mine.RARE.
	VehicleModel05, ///< vehicle.model-05.
	PurchaseModel05, ///< purchase.model-05.
	VehicleModel06, ///< vehicle.model-06.
	PurchaseModel06, ///< purchase.model-06.
	VehicleModel07, ///< vehicle.model-07.
	PurchaseModel07, ///< purchase.model-07.
	VehicleModel08, ///< vehicle.model-08.
	PurchaseModel08, ///< purchase.model-08.
	VehicleModel09, ///< vehicle.model-09.
	PurchaseModel09, ///< purchase.model-09.
	VehicleModel10, ///< vehicle.model-10.
	PurchaseModel10, ///< purchase.model-10.
	VehicleModel11, ///< vehicle.model-11.
	PurchaseModel11, ///< purchase.model-11.
	VehicleModel12, ///< vehicle.model-12.
	PurchaseModel12, ///< purchase.model-12.
	VehicleModel13, ///< vehicle.model-13.
	PurchaseModel13, ///< purchase.model-13.
	FacilityRailStation, ///< facility.rail-station.
	FacilityRoadTerminal, ///< facility.road-terminal.
	FacilityMetalRefinery, ///< facility.metal-refinery.
	FacilityChemicalWorks, ///< facility.chemical-works.
	FacilityBatteryWorks, ///< facility.battery-works.
	FacilityElectronicsWorks, ///< facility.electronics-works.
	FacilityTubeTerminal, ///< facility.tube-terminal.
	FacilityTubeService, ///< facility.tube-service.
	FacilityTubeInterchange, ///< facility.tube-interchange.
	FacilityVentPowerhouse, ///< facility.vent-powerhouse.
	FacilityMagmaPowerhouse, ///< facility.magma-powerhouse.
	FacilityUtilityRiser, ///< facility.utility-riser.
	TubeStraight, ///< tube.straight.
	TubeBend45, ///< tube.bend45.
	TubeJunction, ///< tube.junction.
	TubeEnd, ///< tube.end.
	TubeCrossing, ///< tube.crossing.
	TubeConduit, ///< tube.conduit.
	GeothermalVent, ///< geothermal.vent.
	GeothermalConduit, ///< geothermal.conduit.
	ControlTube, ///< control.tube.
	ControlGeothermal, ///< control.geothermal.
	End, ///< Number of selected family declarations.
};

/** One complete native family range. Pose order is state/frame/direction/tile/part. */
struct MultiLayerSpriteRange {
	uint16_t offset; ///< Relative native Action5 type1C offset.
	uint16_t count; ///< Exact physical source pose count.
	uint8_t states; ///< Public state count.
	uint8_t frames; ///< Authored visual phase/texture count.
	uint8_t directions; ///< Engine view count.
	uint16_t tiles; ///< Physical tile variants or independent portal endpoints.
	uint8_t parts; ///< Physical back/front or vehicle part count.
};

static constexpr uint16_t MULTILAYER_M1_SPRITE_COUNT = 17847; ///< Exclusive selected native block bound, including preserved gaps.
static constexpr uint16_t MULTILAYER_PRESERVED_EXTERNAL_SPRITE_COUNT = 11; ///< Independently published sprites inside the selected block.
static constexpr uint16_t MULTILAYER_SPACE_PREVIEW_OFFSET = 3588; ///< Stable original Space preview start.
static constexpr std::array<MultiLayerSpriteRange, 90> _multilayer_m1_sprite_ranges = {{
	{74, 12, 4, 3, 1, 1, 1}, // terrain.underground
	{86, 12, 4, 3, 1, 1, 1}, // terrain.deep
	{98, 3, 3, 1, 1, 1, 1}, // survey.COAL
	{101, 3, 3, 1, 1, 1, 1}, // survey.IORE
	{104, 3, 3, 1, 1, 1, 1}, // survey.CORE
	{107, 3, 3, 1, 1, 1, 1}, // survey.BAUX
	{110, 3, 3, 1, 1, 1, 1}, // survey.SALT
	{113, 3, 3, 1, 1, 1, 1}, // survey.LITH
	{116, 3, 3, 1, 1, 1, 1}, // survey.NORE
	{119, 3, 3, 1, 1, 1, 1}, // survey.COBT
	{122, 3, 3, 1, 1, 1, 1}, // survey.RARE
	{125, 3, 3, 1, 1, 1, 1}, // survey.URAN
	{200, 384, 6, 4, 8, 1, 2}, // vehicle.model-00
	{584, 2, 2, 1, 1, 1, 1}, // purchase.model-00
	{586, 384, 6, 4, 8, 1, 2}, // vehicle.model-01
	{970, 2, 2, 1, 1, 1, 1}, // purchase.model-01
	{972, 80, 5, 2, 8, 1, 1}, // vehicle.model-02
	{1052, 2, 2, 1, 1, 1, 1}, // purchase.model-02
	{1054, 80, 5, 2, 8, 1, 1}, // vehicle.model-03
	{1134, 2, 2, 1, 1, 1, 1}, // purchase.model-03
	{1136, 80, 5, 2, 8, 1, 1}, // vehicle.model-04
	{1216, 2, 2, 1, 1, 1, 1}, // purchase.model-04
	{1218, 48, 3, 1, 4, 2, 2}, // facility.portal.sealed
	{1266, 48, 3, 1, 4, 2, 2}, // facility.portal.deep-rail
	{1314, 128, 4, 4, 4, 1, 2}, // facility.exchange
	{1442, 128, 4, 4, 4, 1, 2}, // facility.rail-service
	{1570, 128, 4, 4, 4, 1, 2}, // facility.road-service
	{1698, 192, 4, 4, 1, 6, 2}, // facility.earth-receiver
	{1890, 512, 4, 4, 1, 16, 2}, // facility.rock-receiver
	{128, 3, 3, 1, 1, 1, 1}, // control.survey
	{131, 3, 3, 1, 1, 1, 1}, // control.excavation
	{134, 3, 3, 1, 1, 1, 1}, // control.tbm-area
	{137, 3, 3, 1, 1, 1, 1}, // control.tbm-turn45
	{140, 3, 3, 1, 1, 1, 1}, // control.exchange
	{158, 3, 3, 1, 1, 1, 1}, // control.deep-lock
	{161, 3, 3, 1, 1, 1, 1}, // control.future-lock
	{2402, 1, 1, 1, 1, 1, 1}, // cargo.ERTH
	{2403, 1, 1, 1, 1, 1, 1}, // cargo.ROCK
	{168, 16, 16, 1, 1, 1, 1}, // terrain.edges.underground
	{184, 16, 16, 1, 1, 1, 1}, // terrain.edges.deep
	{2404, 16, 16, 1, 1, 1, 1}, // terrain.hardness
	{2420, 16, 16, 1, 1, 1, 1}, // terrain.hardness-edges
	{2436, 1152, 4, 4, 4, 9, 2}, // facility.vent-collector
	{3599, 288, 4, 4, 1, 9, 2}, // mine.COAL
	{3887, 288, 4, 4, 1, 9, 2}, // mine.IORE
	{4175, 288, 4, 4, 1, 9, 2}, // mine.CORE
	{4463, 288, 4, 4, 1, 9, 2}, // mine.SALT
	{4751, 288, 4, 4, 1, 9, 2}, // mine.LITH
	{5039, 288, 4, 4, 1, 9, 2}, // mine.COBT
	{5327, 288, 4, 4, 1, 9, 2}, // mine.RARE
	{5615, 1360, 5, 2, 8, 17, 1}, // vehicle.model-05
	{6975, 2, 2, 1, 1, 1, 1}, // purchase.model-05
	{6977, 80, 5, 2, 8, 1, 1}, // vehicle.model-06
	{7057, 2, 2, 1, 1, 1, 1}, // purchase.model-06
	{7059, 800, 5, 2, 8, 10, 1}, // vehicle.model-07
	{7859, 2, 2, 1, 1, 1, 1}, // purchase.model-07
	{7861, 80, 5, 2, 8, 1, 1}, // vehicle.model-08
	{7941, 2, 2, 1, 1, 1, 1}, // purchase.model-08
	{7943, 144, 9, 2, 8, 1, 1}, // vehicle.model-09
	{8087, 2, 2, 1, 1, 1, 1}, // purchase.model-09
	{8089, 144, 9, 2, 8, 1, 1}, // vehicle.model-10
	{8233, 2, 2, 1, 1, 1, 1}, // purchase.model-10
	{8235, 144, 9, 2, 8, 1, 1}, // vehicle.model-11
	{8379, 2, 2, 1, 1, 1, 1}, // purchase.model-11
	{8381, 144, 9, 2, 8, 1, 1}, // vehicle.model-12
	{8525, 2, 2, 1, 1, 1, 1}, // purchase.model-12
	{8527, 144, 9, 2, 8, 1, 1}, // vehicle.model-13
	{8671, 2, 2, 1, 1, 1, 1}, // purchase.model-13
	{8673, 512, 4, 4, 2, 4, 4}, // facility.rail-station
	{9185, 288, 4, 4, 6, 1, 3}, // facility.road-terminal
	{9473, 512, 4, 4, 1, 16, 2}, // facility.metal-refinery
	{9985, 512, 4, 4, 1, 16, 2}, // facility.chemical-works
	{10497, 512, 4, 4, 1, 16, 2}, // facility.battery-works
	{11009, 512, 4, 4, 1, 16, 2}, // facility.electronics-works
	{11521, 256, 4, 4, 2, 4, 2}, // facility.tube-terminal
	{11777, 128, 4, 4, 4, 1, 2}, // facility.tube-service
	{11905, 256, 4, 4, 2, 4, 2}, // facility.tube-interchange
	{12161, 2560, 5, 4, 4, 16, 2}, // facility.vent-powerhouse
	{14721, 2560, 5, 4, 4, 16, 2}, // facility.magma-powerhouse
	{17281, 128, 4, 4, 1, 4, 2}, // facility.utility-riser
	{17409, 12, 3, 1, 2, 1, 2}, // tube.straight
	{17421, 24, 3, 1, 4, 1, 2}, // tube.bend45
	{17445, 324, 3, 1, 1, 54, 2}, // tube.junction
	{17769, 24, 3, 1, 4, 1, 2}, // tube.end
	{17793, 6, 3, 1, 1, 1, 2}, // tube.crossing
	{17799, 12, 3, 1, 4, 1, 1}, // tube.conduit
	{17811, 12, 3, 4, 1, 1, 1}, // geothermal.vent
	{17823, 24, 3, 1, 4, 2, 1}, // geothermal.conduit
	{152, 3, 3, 1, 1, 1, 1}, // control.tube
	{155, 3, 3, 1, 1, 1, 1}, // control.geothermal
}};

#endif /* MULTILAYER_SPRITE_ASSETS_H */
