/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_inventory.cpp Whole native extraction, finite reserve and packet provenance commit.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group15; modern source adapter remains gated on the CAPA reader.
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
#include "map_layer.h"
#include "tile_map.h"
#include "cargotype.h"
#include "safeguards.h"

/** @copydoc PrepareAsteroidMiningSource */
std::optional<Source> PrepareAsteroidMiningSource(const AsteroidBody &body)
{
	const auto &definitions = GetMultiLayerAsteroidDefinitions();
	// A missing identity or unknown class cannot supply a meaningful packet
	// origin, even when another valid body happens to occupy the same dense slot.
	if (body.id == INVALID_ORBITAL_BODY || to_underlying(body.resource_class) >= definitions.size()) return std::nullopt;
	const auto source = Source::FromAsteroid(body.id.base(), definitions[to_underlying(body.resource_class)].cargo);
	if (!source.IsValid()) return std::nullopt;
	return source;
}

/** @copydoc GetSpaceMiningProductionTile */
std::optional<TileIndex> GetSpaceMiningProductionTile(const AsteroidBody &body)
{
	const auto bounds = GetSpacePlaneBounds(Map::SizeX(), Map::SizeY());
	// Validate before rounding or TileXYL: negative/overflowing coordinates must
	// never wrap into another layer and then debit real reserve for that packet.
	if (!bounds || !bounds->Contains(body.motion.position) || Map::LayerCount() <= to_underlying(MapLayer::Space)) return std::nullopt;
	const auto tile = TileXYL((body.motion.position.x + SPACE_Q16 / 2) / SPACE_Q16,
		(body.motion.position.y + SPACE_Q16 / 2) / SPACE_Q16, MapLayer::Space);
	if (!IsValidTile(tile) || GetTileLayer(tile) != MapLayer::Space) return std::nullopt;
	return tile;
}

/** @copydoc CommitSpaceMiningExtraction */
bool CommitSpaceMiningExtraction(SpaceVehicle &craft, AsteroidBody &body, bool allocation_admitted)
{
	const auto *miner = GetSpaceMiningTuning(GetSpaceVehicleModel(craft));
	const auto source = PrepareAsteroidMiningSource(body);
	const auto tile = GetSpaceMiningProductionTile(body);
	// Both native and copied CAPA readers now preserve the complete natural
	// origin. The original packet constructor receives it without truncating
	// fresh identities onto the legacy 16-bit body domain.
	if (!source || !tile) return false;
	// Only the actual field-owned body and ready attached hull may consume
	// reserve. A copied/unregistered body cannot create cargo even with a full
	// typed ID, matching numeric lease or invented extraction work interval.
	if (miner == nullptr || GetMutableAsteroidBody(body.id) != &body || craft.mission.phase != SpaceMissionPhase::Mining || craft.mission.target != body.id || body.mining_slot != craft.index ||
		craft.mission.work_ticks != miner->extraction_ticks || craft.cargo.TotalCount() >= craft.cargo_cap ||
		craft.cargo_type != GetCargoTypeByLabel(source->GetAsteroidOrigin()->mineral)) return false;
	const uint16_t amount = std::min<uint32_t>({miner->extraction_units, body.remaining_reserve, craft.cargo_cap - craft.cargo.TotalCount()});
	// Whole output allocation precedes reserve/hold/work mutation. The optional participant veto can refuse admission, but can never override actual native pool exhaustion.
	if (amount == 0 || !allocation_admitted || !CargoPacket::CanAllocateItem()) return false;
	auto *packet = CargoPacket::Create(StationID::Invalid(), amount, *source);
	packet->UpdateLoadingTile(*tile); // Initialize actual extraction-plane provenance before Append may merge/delete this new packet.
	craft.cargo.Append(packet);
	body.remaining_reserve -= amount; craft.extracted_units += amount; craft.mission.work_ticks = 0;
	// Final real extraction releases exactly this lease when reserve or hold is exhausted; future return ticks cannot consume the same finite units again.
	if (body.remaining_reserve == 0 || craft.cargo.TotalCount() == craft.cargo_cap) { ReleaseSpaceMiningSlot(craft); craft.mission.phase = SpaceMissionPhase::Return; }
	return true;
}
