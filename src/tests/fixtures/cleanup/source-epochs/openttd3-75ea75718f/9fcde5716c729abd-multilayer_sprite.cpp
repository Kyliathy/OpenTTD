/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_sprite.cpp Read-only bindings from native terrain/portal owners to packed M1 art.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups82/83/79C
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79C
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups84BB/84BD.
 */

#include "stdafx.h"
#include "multilayer_sprite.h"
#include "transport_tube_sprite.h"
#include "excavation.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "map_func.h"
#include "train.h"
#include "tbm.h"
#include "tbm_exchange.h"
#include "underground_vehicle.h"
#include "rail_map.h"
#include "vehicle_func.h"
#include "road_map.h"
#include "industry.h"
#include "industry_map.h"
#include "spoil_receiver.h"
#include "geothermal.h"
#include "geothermal_collector.h"
#include "geothermal_sprite.h"
#include "utility_power.h"
#include "sprite.h"
#include "newgrf.h"

#include "safeguards.h"

/** @copydoc GetMultiLayerControlSprite */
std::optional<SpriteID> GetMultiLayerControlSprite(MultiLayerSpriteAsset asset, bool disabled, bool pressed)
{
	const bool ground = asset >= MultiLayerSpriteAsset::ControlSurvey && asset <= MultiLayerSpriteAsset::ControlFutureLock;
	// Only the seven original controls and these two delivered native tool
	// families may advertise widget status. Future/unrelated packed families
	// acquire no command permission or implicit runtime binding from this API.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		(!ground && asset != MultiLayerSpriteAsset::ControlTube && asset != MultiLayerSpriteAsset::ControlGeothermal)) return std::nullopt;
	return GetMultiLayerSprite(asset, {static_cast<uint8_t>(disabled ? 1 : pressed ? 2 : 0)});
}

/** @copydoc GetMultiLayerPurchaseSprite */
std::optional<SpriteID> GetMultiLayerPurchaseSprite(uint16_t model, bool available)
{
	static constexpr std::array assets{MultiLayerSpriteAsset::PurchaseModel00, MultiLayerSpriteAsset::PurchaseModel01,
		MultiLayerSpriteAsset::PurchaseModel02, MultiLayerSpriteAsset::PurchaseModel03, MultiLayerSpriteAsset::PurchaseModel04};
	// The published five-model purchase slice does not provide art or permission
	// for future fleet designs, foreign vehicles or Classic games.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || model >= assets.size()) return std::nullopt;
	return GetMultiLayerSprite(assets[model], {static_cast<uint8_t>(available ? 0 : 1)});
}

/** @copydoc GetMultiLayerEnginePurchaseSprite */
std::optional<SpriteID> GetMultiLayerEnginePurchaseSprite(EngineID engine, bool available)
{
	if (const auto tube = GetTubeEnginePurchaseSprite(engine, available)) return tube;
	uint16_t model = GetTBMEngineModel(engine);
	if (model == UINT16_MAX) {
		const auto *capability = GetUndergroundEngineCapability(engine);
		if (capability != nullptr) model = capability->model_id;
	}
	return GetMultiLayerPurchaseSprite(model, available);
}

/** @copydoc GetMultiLayerCargoSprite */
std::optional<SpriteID> GetMultiLayerCargoSprite(const CargoSpec &cargo)
{
	const auto selection = GetCurrentMultiLayerProfileSelection();
	const auto *profile = GetMultiLayerContentProfile(selection.id);
	// A matching label alone never establishes first-party ownership. Preserve
	// foreign cargo graphics and the immutable definition package's actual meaning.
	if (selection.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile == nullptr ||
		profile->required_content == nullptr || cargo.grffile == nullptr || cargo.grffile->grfid != profile->required_content->package.grfid) return std::nullopt;
	if (cargo.label == CargoLabel{"ERTH"}) return GetMultiLayerSprite(MultiLayerSpriteAsset::CargoERTH, {});
	if (cargo.label == CargoLabel{"ROCK"}) return GetMultiLayerSprite(MultiLayerSpriteAsset::CargoROCK, {});
	return std::nullopt;
}

/** @copydoc GetMultiLayerSurveySprite */
std::optional<SpriteID> GetMultiLayerSurveySprite(TileIndex tile, bool surveyed)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	const auto &geology = GetGeologyWorldState();
	if (!geology.terrain.Get(tile)) return std::nullopt;
	const auto *deposit = geology.deposits.Get(geology.deposits.Find(tile));
	if (deposit == nullptr) return std::nullopt;
	// This is a graphics-label map only. The geology/content owners still define
	// resource meaning, visibility and the sole finite reserve balance.
	static constexpr std::array labels{CargoLabel{"COAL"}, CargoLabel{"IORE"}, CargoLabel{"CORE"}, CargoLabel{"BAUX"}, CargoLabel{"SALT"},
		CargoLabel{"LITH"}, CargoLabel{"NORE"}, CargoLabel{"COBT"}, CargoLabel{"RARE"}, CargoLabel{"URAN"}};
	const auto found = std::ranges::find(labels, deposit->resource);
	if (found == labels.end()) return std::nullopt;
	const auto asset = static_cast<MultiLayerSpriteAsset>(static_cast<uint8_t>(MultiLayerSpriteAsset::SurveyCOAL) + std::distance(labels.begin(), found));
	return GetMultiLayerSprite(asset, {static_cast<uint8_t>(!surveyed ? 0 : deposit->remaining_reserve == 0 ? 2 : 1)});
}

/** @copydoc GetMultiLayerTerrainSprite */
std::optional<SpriteID> GetMultiLayerTerrainSprite(TileIndex tile)
{
	const auto &geology = GetGeologyWorldState();
	// Only an explicitly selected geological world changes the ground artwork.
	// Existing Classic/no-geology saves retain their original free-floor contract.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || geology.generator_version == 0) return std::nullopt;
	const auto state = geology.terrain.Get(tile);
	if (!state) return std::nullopt;
	// Source mask order follows the screen diamond: upper-left, upper-right,
	// lower-right, lower-left. TileAddWrap protects actual layer and map borders.
	static constexpr std::array<TileIndexDiffC, 4> neighbors{{{0, -1}, {-1, 0}, {0, 1}, {1, 0}}};
	uint8_t solid_mask = 0;
	for (size_t bit = 0; bit < neighbors.size(); ++bit) {
		const auto delta = neighbors[bit];
		const TileIndex adjacent = TileAddWrap(tile, delta.x, delta.y);
		const auto adjacent_state = geology.terrain.Get(adjacent);
		// Undeclared/outside cells and work-in-progress remain closed boundaries.
		// A renderer cannot present them as paid, buildable open joins.
		if (!adjacent_state || *adjacent_state != ExcavationState::Excavated) solid_mask |= static_cast<uint8_t>(1U << bit);
	}
	// Actual in-progress terrain is separate from a solid face adjoining paid
	// floor. No render frame changes the packed state or the project's progress.
	const uint8_t visual_state = *state == ExcavationState::Excavated ? 3 : *state == ExcavationState::Excavating ? 2 : solid_mask != 15 ? 1 : 0;
	const auto hardness = GetExcavationCellHardness(geology, tile);
	return GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardness,
		{static_cast<uint8_t>(to_underlying(hardness) * 4 + visual_state), 0, 0, 0, 0});
}

/** @copydoc GetMultiLayerTerrainEdgeSprites */
std::array<std::optional<SpriteID>, 4> GetMultiLayerTerrainEdgeSprites(TileIndex tile)
{
	std::array<std::optional<SpriteID>, 4> result{};
	const auto &geology = GetGeologyWorldState();
	// Only actual paid open floor exposes neighboring walls. Profile-disabled
	// worlds and work-in-progress cannot gain a visual claim to open ground.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		geology.generator_version == 0 || geology.terrain.Get(tile) != ExcavationState::Excavated) return result;
	static constexpr std::array<TileIndexDiffC, 4> neighbors{{{0, -1}, {-1, 0}, {0, 1}, {1, 0}}};
	// Each exposed face belongs to the adjacent uncut material. At the map
	// fence only, use the floor's material without querying an invalid tile.
	for (size_t side = 0; side < neighbors.size(); ++side) {
		const auto delta = neighbors[side];
		const TileIndex adjacent = TileAddWrap(tile, delta.x, delta.y);
		const auto state = geology.terrain.Get(adjacent);
		if (state == ExcavationState::Excavated) continue;
		const auto hardness = GetExcavationCellHardness(geology, state ? adjacent : tile);
		result[side] = GetMultiLayerSprite(MultiLayerSpriteAsset::TerrainHardnessEdges,
			{static_cast<uint8_t>(to_underlying(hardness) * 4 + side), 0, 0, 0, 0});
	}
	return result;
}

/** @copydoc GetMultiLayerPortalSprite */
std::optional<SpriteID> GetMultiLayerPortalSprite(TileIndex tile, bool front)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	if (!GetLayerPortalPairKey(tile)) return std::nullopt;
	if (GetLayerPortalTransportType(tile) != TransportType::Rail) return std::nullopt;
	const auto state = GetLayerPortalWorkState(tile);
	if (state == LayerPortalWorkState::Invalid) return std::nullopt;
	const MapLayer layer = GetTileLayer(tile);
	const MapLayer target = GetLayerPortalTargetLayer(tile);
	// The same native phase belongs to both independent world endpoints. A Deep
	// pair uses its own authored housings on both the Underground and Deep ends.
	const bool deep = layer == MapLayer::DeepUnderground || target == MapLayer::DeepUnderground;
	const uint16_t endpoint = layer == MapLayer::Surface || (layer == MapLayer::Underground && deep) ? 0 : 1;
	return GetMultiLayerSprite(deep ? MultiLayerSpriteAsset::FacilityPortalDeepRail : MultiLayerSpriteAsset::FacilityPortalSealed,
		{static_cast<uint8_t>(state), 0, static_cast<uint8_t>(GetLayerPortalDoorDirection(tile)), endpoint, static_cast<uint8_t>(front)});
}

/** @copydoc GetMultiLayerTrainSprite */
std::optional<SpriteID> GetMultiLayerTrainSprite(const Train &train, Direction direction, uint64_t visual_ticks)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	if (direction >= Direction::End) return std::nullopt;
	if (const auto tube = GetTubeTrainSprite(train, direction, visual_ticks)) return tube;
	uint16_t model = GetTBMEngineModel(train.engine_type);
	if (model == UINT16_MAX) {
		const auto *capability = GetUndergroundEngineCapability(train.engine_type);
		// The selected hauler is actual conventional model4. Other delivered or
		// future conventional designs retain their own later art integration.
		if (capability == nullptr || capability->model_id != 4) return std::nullopt;
		model = 4;
	}
	const Train *head = train.First();
	const auto *project = GetTBMProjectForCutter(head->index);
	// Actual head speed/stopping governs each wagon's rolling phase; articulated
	// parts do not own a second speed or animation-based movement authority.
	const bool moving = head->cur_speed > 0 && !head->vehstatus.Test(VehState::Stopped);
	// Commissioning magazines also use Track::Depot. Only a real rail depot
	// shows maintenance artwork; a portal cannot impersonate a service facility.
	const bool service = train.IsInDepot() && IsRailDepotTile(train.tile);
	uint8_t state = 0;
	uint8_t part = 0;
	MultiLayerSpriteAsset asset;
	if (model < 2) {
		asset = model == 0 ? MultiLayerSpriteAsset::VehicleModel00 : MultiLayerSpriteAsset::VehicleModel01;
		if (project != nullptr) {
			const auto role = GetTBMConsistRole(*project, train);
			// Native work assignment supplies the immutable body/support roles.
			// A malformed or unrelated part cannot acquire machinery art by proximity.
			if (role != TBMConsistRole::Cutter && role != TBMConsistRole::PermanentSupport) return std::nullopt;
			part = role == TBMConsistRole::PermanentSupport ? 1 : 0;
		} else {
			// A retired intact native machine retains its real permanently
			// articulated support after the project owner has released assignment.
			part = train.IsArticulatedPart() ? 1 : 0;
		}
		if (service) {
			state = 5;
		} else if (project != nullptr && project->blocked != TBMBlockedReason::None) {
			// Existing resumable holds choose warning art; rendering cannot clear
			// a stopped/full/unsupported/underfunded controller to animate boring.
			state = 3;
		} else if (head->IsDrivingBackwards()) {
			state = 4; // The body's actual direction stays fixed while motion reverses.
		} else if (project != nullptr && project->phase == TBMPhase::Bore) {
			// Only the actual unblocked work phase shows the active cutter/support.
			state = 2;
		} else if (moving) {
			state = 1;
		}
	// The immutable Earth/Rock engine definitions select separate packet-backed
	// hopper families; presentation never recolors one cargo into the other.
	} else if (model == 2 || model == 3) {
		asset = model == 2 ? MultiLayerSpriteAsset::VehicleModel02 : MultiLayerSpriteAsset::VehicleModel03;
		const uint count = train.cargo.StoredCount();
		if (count != 0) {
			// Packet quantity has priority over service/hold cues so a loaded
			// stopped wagon cannot appear empty when coupled to another locomotive.
			state = count >= train.cargo_cap ? 2 : 1;
		} else if (service) {
			state = 3;
		} else if (project != nullptr && project->blocked != TBMBlockedReason::None) {
			// Only empty wagons use the separate blocked cue; loaded stages persist.
			state = 4;
		}
	} else {
		asset = MultiLayerSpriteAsset::VehicleModel04;
		const auto *assignment = GetTBMExchangeAssignmentForHauler(head->index);
		if (service) {
			state = 4;
		} else if (assignment != nullptr && assignment->wait != TBMHaulerWait::None) {
			// Saved logistics holds expose their existing refusal without hiding
			// loaded wagons or changing receiver intake to make the hauler look busy.
			state = 3;
		} else if (assignment != nullptr && assignment->phase == TBMHaulerPhase::DisposeOrTransfer && !moving) {
			// Existing stopped disposal/transfer chooses the equipment cue; the
			// station/receiver owners still exclusively move and process the packets.
			state = 2;
		} else if (moving) {
			state = 1;
		}
	}
	return GetMultiLayerSprite(asset, {state, GetMultiLayerVisualFrame(asset, visual_ticks, moving), static_cast<uint8_t>(direction), 0, part});
}

/** @copydoc RefreshMultiLayerTrainArt */
void RefreshMultiLayerTrainArt(Train &head, uint64_t visual_ticks)
{
	// Dedicated and Classic worlds need no native raster refresh. Sampling only
	// authored phase boundaries adds no second simulation tick or work clock.
	if (_network_dedicated || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || visual_ticks % 8 != 0) return;
	// Traverse only the chain already owned by this ordinary front-engine tick.
	// Native sprite/cache invalidation is independent of cutting, exchange and RNG.
	for (Train *part = &head; part != nullptr; part = part->Next()) {
		if (!GetMultiLayerTrainSprite(*part, part->direction, visual_ticks)) continue;
		part->sprite_cache.revalidate_before_draw = true;
		// Train::MarkDirty also rebuilds consist/acceleration and accepts only
		// front engines or free wagons. Refresh each physical part through the
		// viewport owner instead, leaving every simulation cache and role intact.
		part->UpdateViewport(true, false);
	}
}

/** @copydoc GetMultiLayerRailDepotSprite */
std::optional<SpriteID> GetMultiLayerRailDepotSprite(TileIndex tile, bool front, uint64_t visual_ticks)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	if (!IsValidTile(tile)) return std::nullopt;
	if (!IsRailDepotTile(tile)) return std::nullopt;
	if (const auto tube = GetTubeFacilityArt(tile, visual_ticks)) return front ? tube->sprites.front : tube->sprites.back;
	const MapLayer layer = GetTileLayer(tile);
	// The selected underground facilities are native one-tile depots. Existing
	// Surface depots and orbital/later facilities keep their own art contracts.
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return std::nullopt;
	const auto *assignment = GetTBMExchangeAssignmentAtBay(tile);
	const auto *project = GetTBMProjectForExchangeBay(tile);
	const bool exchange = assignment != nullptr || project != nullptr;
	const auto asset = exchange ? MultiLayerSpriteAsset::FacilityExchange : MultiLayerSpriteAsset::FacilityRailService;
	uint8_t state = 1;
	/** @param vehicle Actual spatially indexed occupant. @return Whether it is physically inside this rail depot. */
	const bool occupied = HasVehicleOnTile(tile, [](const Vehicle *vehicle) {
		// Actual depot occupancy chooses the bounded working/maintenance lamp;
		// an approaching train or a sprite overlapping the roof does not count.
		return vehicle->type == VehicleType::Train && vehicle->IsInDepot();
	});
	if (occupied) state = 2;
	// Saved queue/logistics refusals select a blocked lamp. Reading a refusal
	// never exchanges wagons or clears the saved assignment to make the bay busy.
	if (assignment != nullptr && assignment->wait != TBMHaulerWait::None && assignment->wait != TBMHaulerWait::Stopped) state = 3;
	return GetMultiLayerSprite(asset, {state, GetMultiLayerVisualFrame(asset, visual_ticks, false), static_cast<uint8_t>(GetRailDepotDirection(tile)), 0, static_cast<uint8_t>(front)});
}

/** @copydoc GetMultiLayerRoadDepotSprite */
std::optional<SpriteID> GetMultiLayerRoadDepotSprite(TileIndex tile, bool front, uint64_t visual_ticks)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	if (!IsValidTile(tile)) return std::nullopt;
	if (!IsRoadDepotTile(tile)) return std::nullopt;
	const MapLayer layer = GetTileLayer(tile);
	// The authored road-service family belongs to the two excavated rock layers;
	// ordinary Surface and unselected later road buildings retain their own art.
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return std::nullopt;
	/** @param vehicle Actual indexed occupant. @return Whether it is physically inside this road depot. */
	const bool occupied = HasVehicleOnTile(tile, [](const Vehicle *vehicle) {
		// Actual depot state chooses servicing occupancy, not a sprite overlapping
		// the driveway or a vehicle on an ordinary road outside the building.
		return vehicle->type == VehicleType::Road && vehicle->IsInDepot();
	});
	return GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityRoadService,
		{static_cast<uint8_t>(occupied ? 2 : 1), GetMultiLayerVisualFrame(MultiLayerSpriteAsset::FacilityRoadService, visual_ticks, false),
		static_cast<uint8_t>(GetRoadDepotDirection(tile)), 0, static_cast<uint8_t>(front)});
}

/** @copydoc GetMultiLayerReceiverSprite */
std::optional<SpriteID> GetMultiLayerReceiverSprite(TileIndex tile, bool front, uint64_t visual_ticks)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	if (!IsValidTile(tile)) return std::nullopt;
	if (!IsTileType(tile, TileType::Industry)) return std::nullopt;
	const Industry *industry = Industry::GetByTile(tile);
	const auto *receiver = GetSpoilReceiver(industry->index);
	if (receiver == nullptr) return std::nullopt;
	const auto &recipe = GetSpoilReceiverRecipe(receiver->kind);
	const auto stock = industry->GetCargoAccepted(GetCargoTypeByLabel(recipe.cargo));
	if (stock == industry->accepted.end()) return std::nullopt;
	const auto asset = receiver->kind == SpoilReceiverKind::Earth ? MultiLayerSpriteAsset::FacilityEarthReceiver : MultiLayerSpriteAsset::FacilityRockReceiver;
	const uint x = TileX(tile) - TileX(industry->location.tile);
	const uint y = TilePlaneY(tile) - TilePlaneY(industry->location.tile);
	const uint16_t cell = static_cast<uint16_t>(y * industry->location.w + x);
	uint8_t state = 0;
	if (IsIndustryCompleted(tile)) {
		// The actual crusher cannot process a batch into a full aggregate store.
		// Surplus disposal has no aggregate output and keeps its separate policy.
		const bool output_blocked = receiver->kind == SpoilReceiverKind::Rock && receiver->mode == SpoilReceiverMode::Process &&
			stock->waiting >= recipe.batch_t && receiver->aggregate + recipe.aggregate_per_batch_t > recipe.aggregate_cap_t;
		// Native stock and interval intake own acceptance. A paused-full receiver
		// shows blocked art rather than inventing processing from a visual clock.
		if (GetSpoilReceiverRoom(*industry) == 0 || output_blocked) {
			state = 3;
		} else if (stock->waiting >= recipe.batch_t) {
			state = 2;
		} else {
			state = 1;
		}
	}
	return GetMultiLayerSprite(asset, {state, GetMultiLayerVisualFrame(asset, visual_ticks, false), 0, cell, static_cast<uint8_t>(front)});
}

/** @copydoc GetMultiLayerCollectorSprite */
std::optional<SpriteID> GetMultiLayerCollectorSprite(TileIndex tile, bool front, uint64_t visual_ticks)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	if (!IsValidTile(tile)) return std::nullopt;
	if (!IsTileType(tile, TileType::Industry)) return std::nullopt;
	const Industry *industry = Industry::GetByTile(tile);
	const auto *collector = GetGeothermalCollectorState(industry->index);
	if (collector == nullptr) return std::nullopt;
	if (!IsGeothermalCollectorDefinition(industry->type)) return std::nullopt;
	if (GetTileLayer(tile) != MapLayer::DeepUnderground) return std::nullopt;
	const auto *claim = GetGeothermalSiteClaim(collector->site);
	if (claim == nullptr) return std::nullopt;
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector);
	// Only the actual paid collector footprint can consume its nine-cell source.
	// A coincident source, wrong family or relocated native industry cannot select
	// another site's orientation or expand artwork into neighboring world cells.
	if (claim->active || claim->site.kind != MultiLayerGeothermalConstructionKind::Collector || claim->site.tile != industry->location.tile ||
		industry->location.w != terms->width || industry->location.h != terms->height) return std::nullopt;
	const uint x = TileX(tile) - TileX(industry->location.tile);
	const uint y = TilePlaneY(tile) - TilePlaneY(industry->location.tile);
	const uint8_t view = static_cast<uint8_t>(claim->site.access);
	// All four authored orientations use the native access side. Unsigned local
	// coordinates also refuse cells before the origin, without a camera offset.
	if (x >= industry->location.w || y >= industry->location.h || view >= 4) return std::nullopt;
	const uint16_t cell = static_cast<uint16_t>(y * industry->location.w + x);
	uint8_t state = 0;
	uint8_t frame = GetIndustryConstructionStage(tile);
	if (IsIndustryCompleted(tile)) {
		frame = GetMultiLayerVisualFrame(MultiLayerSpriteAsset::FacilityVentCollector, visual_ticks, false);
		state = IsGeothermalCollectorOperational(industry->index) ? 1 : 3;
		if (state == 1) {
			const auto *source = GetUtilityPowerProducer(collector->heat_producer);
			if (source != nullptr) {
				// A retired, foreign or wrong-domain meter blocks this machinery's
				// real publication. Drawing may report it but never repair/rebind it.
				if (!source->active || source->owner != collector->owner || source->meter.domain != UtilityEnergyDomain::Heat ||
					source->meter.layer != MapLayer::DeepUnderground || source->kind != UtilityGenerationKind::Geothermal) {
					state = 3;
				} else if (source->meter.accounting_interval == GetUtilityPowerInterval() && source->meter.generation_energy_j != 0) {
					// Working lamps read a genuine positive offer in the current
					// shared interval; a visual clock or stale offer produces no Heat.
					state = 2;
				}
			} else if (collector->heat_producer != UtilityProducerID{UINT32_MAX}) {
				// Missing retained meter identity is a real publication refusal;
				// a completed collector with no acquired meter yet remains ready.
				state = 3;
			}
		}
	}
	return GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityVentCollector, {state, frame, view, cell, static_cast<uint8_t>(front)});
}

/** @copydoc GetMultiLayerIndustrySprite */
std::optional<SpriteID> GetMultiLayerIndustrySprite(TileIndex tile, bool front, uint64_t visual_ticks)
{
	const auto receiver = GetMultiLayerReceiverSprite(tile, front, visual_ticks);
	if (receiver.has_value()) return receiver;
	const auto collector = GetMultiLayerCollectorSprite(tile, front, visual_ticks);
	if (collector.has_value()) return collector;
	const auto generator = GetGeothermalGeneratorSprite(tile, front, visual_ticks);
	return generator.has_value() ? generator : GetGeothermalRiserSprite(tile, front, visual_ticks);
}

/** @copydoc GetMultiLayerDepotFrontBounds */
const SpriteBounds &GetMultiLayerDepotFrontBounds(DiagDirection door)
{
	// Far roofs cover the complete depot footprint; near mouths sort at their
	// door edge using the same established geometry as the native layer portal.
	static constexpr DiagDirectionIndexArray<SpriteBounds> bounds{{{
		{{0, 0, 0}, {TILE_SIZE, TILE_SIZE, 20}, {}},
		{{0, 15, 0}, {16, 1, 20}, {0, -15, 0}},
		{{15, 0, 0}, {1, 16, 20}, {-15, 0, 0}},
		{{0, 0, 0}, {TILE_SIZE, TILE_SIZE, 20}, {}},
	}}};
	return bounds[door];
}
