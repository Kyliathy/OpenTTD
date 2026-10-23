/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_vehicle_sprite.cpp Actual capsule packets, source waits and native service select original packed body/purchase poses.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "stdafx.h"
#include "transport_tube_sprite.h"
#include "transport_tube.h"
#include "multilayer_sprite.h"
#include "multilayer_content.h"
#include "train.h"
#include "rail_map.h"
#include "safeguards.h"

static constexpr std::array _tube_body_assets{MultiLayerSpriteAsset::VehicleModel09, MultiLayerSpriteAsset::VehicleModel10,
	MultiLayerSpriteAsset::VehicleModel11, MultiLayerSpriteAsset::VehicleModel12, MultiLayerSpriteAsset::VehicleModel13}; ///< Authored source order for the immutable five original models, independent of runtime engine slots.
static constexpr std::array _tube_purchase_assets{MultiLayerSpriteAsset::PurchaseModel09, MultiLayerSpriteAsset::PurchaseModel10,
	MultiLayerSpriteAsset::PurchaseModel11, MultiLayerSpriteAsset::PurchaseModel12, MultiLayerSpriteAsset::PurchaseModel13}; ///< Separate available/locked previews; a picture never supplies build permission.

/**
 * Resolve artwork through actual admitted native engine identity, independently of today's purchase date.
 * @param engine Existing native engine; labels, names and copied local numbers confer no source identity.
 * @return Source-family index0..4, or absence for foreign/ordinary/future/unselected content.
 */
static std::optional<uint8_t> GetTubeSpriteModel(EngineID engine)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	const auto capability = GetTubeVehicleCapability(engine);
	if (!capability.has_value()) return std::nullopt;
	const uint16_t model = capability->catalogue->model_id;
	// Only original models9..13 have this packed source matrix. Retained native
	// identity survives unavailable dates, while foreign/future definitions must
	// not wrap into a different hull or acquire purchase authority from artwork.
	if (model < 9 || model > 13) return std::nullopt;
	return static_cast<uint8_t>(model - 9);
}

/** @copydoc GetTubeTrainSprite */
std::optional<SpriteID> GetTubeTrainSprite(const Train &train, Direction direction, uint64_t visual_ticks)
{
	if (direction >= Direction::End) return std::nullopt;
	const auto model = GetTubeSpriteModel(train.engine_type);
	if (!model.has_value()) return std::nullopt;
	const Train *head = train.First();
	const auto *binding = GetTubePowerVehicle(head->index);
	const uint count = train.cargo.StoredCount();
	// Load visibility follows this part's actual packets through every wait and
	// service state, including retained over-capacity history. No lamp/animation
	// may make real cargo appear empty or alter its native capacity/identity.
	const uint8_t load = count == 0 ? 0 : count >= train.cargo_cap ? 2 : 1;
	// A native power/path refusal or actual crashed/stuck controller has priority
	// over the requested maintenance cue. Manual Stop remains independent: a
	// healthy parked, supplied capsule is ready, without animation moving it.
	const bool blocked = binding == nullptr || binding->wait != TubePowerWait::None ||
		head->flags.Test(VehicleRailFlag::Stuck) || head->vehstatus.Test(VehState::Crashed);
	uint8_t mode = blocked ? 6 : 0;
	// Amber means the actual damaged capsule requests service in its native tube
	// depot. Merely parking a healthy vehicle cannot claim a repair or invoice.
	if (!blocked && IsRailDepotTile(head->tile) && IsTubeRailType(GetRailType(head->tile)) && GetTubePowerPhase(*head) == TubePowerPhase::DepotService) mode = 3;
	const auto asset = _tube_body_assets[*model];
	// The two authored phases change only a status-lamp pixel. They continue at
	// rest and never imply wheel rotation, physical movement or paid service.
	return GetMultiLayerSprite(asset, {static_cast<uint8_t>(mode + load), GetMultiLayerVisualFrame(asset, visual_ticks, head->cur_speed != 0),
		static_cast<uint8_t>(direction), 0, 0});
}

/** @copydoc GetTubeEnginePurchaseSprite */
std::optional<SpriteID> GetTubeEnginePurchaseSprite(EngineID engine, bool available)
{
	const auto model = GetTubeSpriteModel(engine);
	if (!model.has_value()) return std::nullopt;
	return GetMultiLayerSprite(_tube_purchase_assets[*model], {static_cast<uint8_t>(available ? 0 : 1)});
}
