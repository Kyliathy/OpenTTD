/**
 * @file multilayer_sprite_assets.h Generated M1 native graphics ranges; do not edit.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/81B
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

static constexpr uint16_t MULTILAYER_M1_SPRITE_COUNT = 2436; ///< Exclusive selected native block bound.
static constexpr std::array<MultiLayerSpriteRange, 42> _multilayer_m1_sprite_ranges = {{
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
}};

#endif /* MULTILAYER_SPRITE_ASSETS_H */
