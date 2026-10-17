/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_inventory.cpp Whole native extraction, finite reserve and packet provenance commit.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group10
 * No new material, utility account, cargo registry or saved inventory is introduced.
 */
#include "stdafx.h"
#include "space_mining_inventory.h"
#include "space_vehicle.h"
#include "space_mining_tuning.h"
#include "asteroid.h"
#include "multilayer_asteroid_catalogue.h"
#include "map_func.h"
#include "cargotype.h"
#include "safeguards.h"

/** @copydoc CommitSpaceMiningExtraction */
bool CommitSpaceMiningExtraction(SpaceVehicle &craft, AsteroidBody &body, bool allocation_admitted)
{
	const auto *miner = GetSpaceMiningTuning(GetSpaceVehicleModel(craft));
	// Only the actual ready attached hull may consume this body's reserve. A stale lease, unrelated role, incompatible hold or invented work interval cannot create cargo.
	if (miner == nullptr || craft.mission.phase != SpaceMissionPhase::Mining || craft.mission.target != body.id || body.mining_slot != craft.index ||
		craft.mission.work_ticks != miner->extraction_ticks || craft.cargo.TotalCount() >= craft.cargo_cap ||
		craft.cargo_type != GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(body.resource_class)].cargo)) return false;
	const uint16_t amount = std::min<uint32_t>({miner->extraction_units, body.remaining_reserve, craft.cargo_cap - craft.cargo.TotalCount()});
	// Whole output allocation precedes reserve/hold/work mutation. The optional participant veto can refuse admission, but can never override actual native pool exhaustion.
	if (amount == 0 || !allocation_admitted || !CargoPacket::CanAllocateItem()) return false;
	auto *packet = CargoPacket::Create(StationID::Invalid(), amount, Source{static_cast<SourceID>(body.id.base()), SourceType::Asteroid});
	const TileIndex tile = TileXYL((body.motion.position.x + SPACE_Q16 / 2) / SPACE_Q16, (body.motion.position.y + SPACE_Q16 / 2) / SPACE_Q16, MapLayer::Space);
	packet->UpdateLoadingTile(tile); // Initialize actual extraction-plane provenance before Append may merge/delete this new packet.
	craft.cargo.Append(packet);
	body.remaining_reserve -= amount; craft.extracted_units += amount; craft.mission.work_ticks = 0;
	// Final real extraction releases exactly this lease when reserve or hold is exhausted; future return ticks cannot consume the same finite units again.
	if (body.remaining_reserve == 0 || craft.cargo.TotalCount() == craft.cargo_cap) { ReleaseSpaceMiningSlot(craft); craft.mission.phase = SpaceMissionPhase::Return; }
	return true;
}
