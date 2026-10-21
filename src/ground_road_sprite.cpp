/**
 * @file ground_road_sprite.cpp Native packet, road traffic and depot owners select accepted carrier pixels.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FF.
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
 */

#include "stdafx.h"
#include "ground_road_sprite.h"
#include "roadveh.h"
#include "underground_vehicle.h"
#include "map_func.h"
#include "network/network.h"

#include "safeguards.h"

/**
 * Find the accepted model7 cargo source cell after native capability checking.
 * @param label Actual shared cargo meaning; this table supplies pixels, never refit permission.
 * @return Registered source-only variant, or absence for an unauthored cargo view.
 */
static std::optional<uint16_t> GetMineTruckSourceVariant(CargoLabel label)
{
	// Preserve the ten accepted cargo registrations, independently of runtime
	// slots. The native engine capability alone authorizes carrying each cargo.
	static constexpr std::array labels{CargoLabel{"COAL"}, CargoLabel{"IORE"}, CargoLabel{"CORE"}, CargoLabel{"BAUX"},
		CargoLabel{"SALT"}, CargoLabel{"LITH"}, CargoLabel{"NORE"}, CargoLabel{"COBT"}, CargoLabel{"ERTH"}, CargoLabel{"ROCK"}};
	const auto found = std::ranges::find(labels, label);
	if (found == labels.end()) return std::nullopt;
	return static_cast<uint16_t>(found - labels.begin());
}

/** @copydoc GetGroundRoadSprite */
std::optional<SpriteID> GetGroundRoadSprite(const RoadVehicle &vehicle, Direction direction, uint64_t visual_ticks)
{
	const auto *capability = GetUndergroundEngineCapability(vehicle.engine_type);
	// Foreign or rail definitions, malformed locations and invalid cargo slots
	// cannot inherit native carrier art by sharing original image numbers.
	if (capability == nullptr || (capability->model_id != 7 && capability->model_id != 8) ||
		direction >= Direction::End || !IsValidTile(vehicle.tile) || !IsValidCargoType(vehicle.cargo_type)) return std::nullopt;
	if (CheckUndergroundVehicleCapability(vehicle.engine_type, GetTileLayer(vehicle.tile), vehicle.cargo_type).Failed()) return std::nullopt;
	uint16_t variant = 0;
	if (capability->model_id == 7) {
		const auto registered = GetMineTruckSourceVariant(CargoSpec::Get(vehicle.cargo_type)->label);
		if (!registered) return std::nullopt;
		variant = *registered;
	}
	const RoadVehicle *head = vehicle.First();
	const uint count = vehicle.cargo.StoredCount();
	uint8_t state = 0;
	if (count != 0) {
		// Real load takes priority over service/traffic cues. A stopped or
		// serviced carrier cannot visually lose custody of its existing packets.
		state = vehicle.cargo_cap > 0 && count >= vehicle.cargo_cap ? 2 : 1;
	} else if (vehicle.IsInDepot() && IsRoadDepotTile(vehicle.tile)) {
		// Only actual owned depot geometry can supply the empty maintenance cue.
		state = 3;
	} else if (head->blocked_ctr > 0 && head->cur_speed == 0 &&
		!head->vehstatus.Test(VehState::Stopped) && !head->vehstatus.Test(VehState::Crashed)) {
		// The existing traffic wait counter supplies this visible hold only
		// while the live head is blocked, rather than deliberately stopped/crashed.
		state = 4;
	}
	// Actual live head movement owns tyre phases. Authored service/blocked
	// phases change glass only, preserving static tyres at the same clock rate.
	const bool moving = head->cur_speed > 0 && !head->vehstatus.Test(VehState::Stopped) && !head->vehstatus.Test(VehState::Crashed);
	const uint8_t frame = moving || state >= 3 ? static_cast<uint8_t>((visual_ticks / 8) % 2) : 0;
	return GetGroundSprite(capability->model_id == 7 ? GroundSpriteFamily::VehicleModel07 : GroundSpriteFamily::VehicleModel08,
		{state, frame, static_cast<uint8_t>(direction), variant, 0});
}

/** @copydoc GetGroundRoadPurchaseSprite */
std::optional<SpriteID> GetGroundRoadPurchaseSprite(uint16_t model, bool available)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	// Source-packed tube capsules belong to later operating owners; only the
	// two actual native conventional road designs receive these purchase cues.
	if (model != 7 && model != 8) return std::nullopt;
	return GetGroundSprite(model == 7 ? GroundSpriteFamily::PurchaseModel07 : GroundSpriteFamily::PurchaseModel08,
		{static_cast<uint8_t>(available ? 0 : 1)});
}

/** @copydoc RefreshGroundRoadArt */
void RefreshGroundRoadArt(RoadVehicle &head, uint64_t visual_ticks)
{
	// Dedicated/foreign/profile-disabled worlds and non-phase ticks need no
	// drawing work. The ordinary head alone owns traversal of its real chain.
	if (_network_dedicated || !head.IsFrontEngine() || visual_ticks % 8 != 0 ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return;
	// Refresh only derived sprite caches on the existing simulation tick. Do
	// not run a second road controller, alter routes or rebuild acceleration.
	for (RoadVehicle *part = &head; part != nullptr; part = part->Next()) {
		if (!GetGroundRoadSprite(*part, part->direction, visual_ticks)) continue;
		part->sprite_cache.revalidate_before_draw = true;
		part->UpdateViewport(true, false);
	}
}
