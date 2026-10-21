/**
 * @file ground_train_sprite.cpp Packet-backed conventional rail art and static purchase previews.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FE.
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
 */

#include "stdafx.h"
#include "ground_train_sprite.h"
#include "train.h"
#include "underground_vehicle.h"
#include "map_func.h"
#include "rail_map.h"

#include "safeguards.h"

/**
 * Find the registered model5 source cell for an already capability-checked cargo.
 * @param label Shared native cargo meaning, never a cargo slot or permission.
 * @return Source-only cargo variant, or absence when no authored view exists.
 */
static std::optional<uint16_t> GetMineralWagonSourceVariant(CargoLabel label)
{
	// This is the accepted source-manifest's seventeen-cell art registration,
	// not a refit registry. The immutable native capability remains authoritative
	// and is checked before selecting any of these pixel variants.
	static constexpr std::array labels{CargoLabel{"COAL"}, CargoLabel{"IORE"}, CargoLabel{"CORE"}, CargoLabel{"BAUX"},
		CargoLabel{"SALT"}, CargoLabel{"LITH"}, CargoLabel{"NORE"}, CargoLabel{"COBT"}, CargoLabel{"RARE"},
		CargoLabel{"RFMT"}, CargoLabel{"STEL"}, CargoLabel{"SCMP"}, CargoLabel{"RBPT"}, CargoLabel{"ELEC"},
		CargoLabel{"BATY"}, CargoLabel{"SMIK"}, CargoLabel{"GOOD"}};
	const auto found = std::ranges::find(labels, label);
	if (found == labels.end()) return std::nullopt;
	return static_cast<uint16_t>(found - labels.begin());
}

/** @copydoc GetGroundTrainSprite */
std::optional<SpriteID> GetGroundTrainSprite(const Train &train, Direction direction, uint64_t visual_ticks)
{
	const auto *capability = GetUndergroundEngineCapability(train.engine_type);
	// Foreign definitions, road designs and malformed locations/cargo cannot
	// inherit rail art merely by sharing an image number or a cargo label.
	if (capability == nullptr || (capability->model_id != 5 && capability->model_id != 6) ||
		direction >= Direction::End || !IsValidTile(train.tile) || !IsValidCargoType(train.cargo_type)) return std::nullopt;
	if (CheckUndergroundVehicleCapability(train.engine_type, GetTileLayer(train.tile), train.cargo_type).Failed()) return std::nullopt;
	uint16_t variant = 0;
	if (capability->model_id == 5) {
		const auto registered = GetMineralWagonSourceVariant(CargoSpec::Get(train.cargo_type)->label);
		if (!registered) return std::nullopt;
		variant = *registered;
	}
	const Train *head = train.First();
	const uint count = train.cargo.StoredCount();
	uint8_t state = 0;
	if (count != 0) {
		// Real packets have priority over maintenance/hold artwork. A stopped or
		// serviced wagon must keep its visible cargo, including overfull custody.
		state = train.cargo_cap > 0 && count >= train.cargo_cap ? 2 : 1;
	} else if (train.IsInDepot() && IsRailDepotTile(train.tile)) {
		// Portal magazines also use Track::Depot; only a real service facility
		// can show the source's empty maintenance cue.
		state = 3;
	} else if (head->flags.Test(VehicleRailFlag::Stuck)) {
		state = 4; // The existing path reservation owner alone supplies this hold.
	}
	// Actual head speed/stopping owns wheel motion. Empty service/hold phases
	// animate only their authored lamps, whose source gears remain static.
	const bool moving = head->cur_speed > 0 && !head->vehstatus.Test(VehState::Stopped);
	const uint8_t frame = moving || state >= 3 ? static_cast<uint8_t>((visual_ticks / 8) % 2) : 0;
	return GetGroundSprite(capability->model_id == 5 ? GroundSpriteFamily::VehicleModel05 : GroundSpriteFamily::VehicleModel06,
		{state, frame, static_cast<uint8_t>(direction), variant, 0});
}

/** @copydoc GetGroundRailPurchaseSprite */
std::optional<SpriteID> GetGroundRailPurchaseSprite(uint16_t model, bool available)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	// This group delivers only the two actual rail wagons. Road/tube art keeps
	// its later qualified consumer, even though its source is already packed.
	if (model != 5 && model != 6) return std::nullopt;
	return GetGroundSprite(model == 5 ? GroundSpriteFamily::PurchaseModel05 : GroundSpriteFamily::PurchaseModel06,
		{static_cast<uint8_t>(available ? 0 : 1)});
}
