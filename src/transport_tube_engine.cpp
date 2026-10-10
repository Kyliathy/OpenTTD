/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_engine.cpp Native capsule prototype and exclusive physical movement domain.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Group1B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md canonical catalogue
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Group10B physical source geometry
 */

#include "stdafx.h"
#include "transport_tube.h"
#include "transport_tube_model.h"
#include "engine_base.h"
#include "rail.h"
#include "train.h"
#include "layer_portal_map.h"
#include "table/strings.h"

#include "safeguards.h"

static constexpr uint16_t TUBE_PROTOTYPE_MODEL = 9; ///< First delivered capsule; later models require their own producer groups.
static constexpr uint16_t TUBE_ENGINE_LOCAL = 0x8000 + TUBE_PROTOTYPE_MODEL; ///< Extend the native EIDS namespace after conventional models4..8.

/** @copydoc GetTubeVehicleGeometry */
std::optional<TubeVehicleGeometry> GetTubeVehicleGeometry(uint16_t model_id)
{
	const auto catalogue = GetMultiLayerVehicleCapabilities();
	const auto found = std::ranges::find(catalogue, model_id, &LayerVehicleCapability::model_id);
	// Only the canonical tube movement domain supplies capsule geometry.
	// Ordinary/orbital models cannot acquire tube identity through an art query,
	// and missing future engine definitions are never allocated by this lookup.
	if (found == catalogue.end() || found->transport != MultiLayerVehicleTransport::Tube) return std::nullopt;
	return TubeVehicleGeometry{&*found, VEHICLE_LENGTH, 1};
}

/** @return The immutable first capsule's shared profile capability; no duplicate model/cargo registry is created. */
static const LayerVehicleCapability &GetPrototypeCatalogue()
{
	const auto catalogue = GetMultiLayerVehicleCapabilities();
	const auto found = std::ranges::find(catalogue, TUBE_PROTOTYPE_MODEL, &LayerVehicleCapability::model_id);
	assert(found != catalogue.end());
	return *found;
}

/** @copydoc GetTubeEngine */
EngineID GetTubeEngine(uint16_t model_id)
{
	if (model_id != TUBE_PROTOTYPE_MODEL) return EngineID::Invalid();
	const auto &package = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package;
	const auto id = _engine_mngr.GetID(VehicleType::Train, TUBE_ENGINE_LOCAL, package.grfid);
	const auto *engine = Engine::GetIfValid(id);
	// Only the actual native EIDS row can identify a capsule. Foreign definitions
	// and stale mappings cannot acquire this domain from broad railtype masks.
	if (engine == nullptr || engine->type != VehicleType::Train || engine->grf_prop.local_id != TUBE_ENGINE_LOCAL) return EngineID::Invalid();
	return id;
}

/** @copydoc GetTubeVehicleCapability */
std::optional<TubeVehicleCapability> GetTubeVehicleCapability(EngineID engine)
{
	const auto *definition = Engine::GetIfValid(engine);
	// Ordinary engines avoid a mapping lookup on the movement hot path. A copied
	// package-local number alone still cannot impersonate the native definition.
	if (definition == nullptr || definition->type != VehicleType::Train || definition->grf_prop.local_id != TUBE_ENGINE_LOCAL) return std::nullopt;
	if (GetTubeEngine(TUBE_PROTOTYPE_MODEL) != engine) return std::nullopt;
	return TubeVehicleCapability{engine, &GetPrototypeCatalogue(), GetTubeRailType()};
}

/** @copydoc InitialiseTubeEngineDefinitions */
bool InitialiseTubeEngineDefinitions(const GRFConfigList &activated)
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	const auto tube = GetTubeRailType();
	// Definition installation requires both prior canonical rail admission and
	// exact content. This prototype cannot activate itself through a foreign GRF.
	if (tube == INVALID_RAILTYPE || CheckMultiLayerContentIdentity(required, activated) != MultiLayerContentIdentityError::None) return false;
	const auto geometry = GetTubeVehicleGeometry(TUBE_PROTOTYPE_MODEL);
	if (!geometry) return false;
	const auto &capability = *geometry->catalogue;
	// Resolve every allowed cargo before allocating a definition. Missing inputs
	// refuse unchanged rather than partially publishing a refit policy.
	for (const auto label : capability.cargo_labels) {
		if (!IsValidCargoType(GetCargoTypeByLabel(label))) return false;
	}
	auto id = _engine_mngr.GetID(VehicleType::Train, TUBE_ENGINE_LOCAL, required.package.grfid);
	Engine *engine = Engine::GetIfValid(id);
	// A persisted mapping may reconstruct an absent slot, but cannot overwrite a
	// live original/foreign/type-mismatched definition.
	if (engine != nullptr && (engine->type != VehicleType::Train || engine->grf_prop.local_id != TUBE_ENGINE_LOCAL)) return false;
	if (engine == nullptr) {
		if (!Engine::CanAllocateItem()) return false;
		if (id == EngineID::Invalid()) {
			engine = Engine::Create(VehicleType::Train, TUBE_ENGINE_LOCAL);
			id = engine->index;
			_engine_mngr.SetID(VehicleType::Train, TUBE_ENGINE_LOCAL, required.package.grfid, 0, id);
		} else {
			engine = Engine::CreateAtIndex(id, VehicleType::Train, TUBE_ENGINE_LOCAL);
		}
	}
	engine->info.base_intro = TimerGameCalendar::ConvertYMDToDate(capability.introduction_year, 0, 1);
	engine->info.lifelength = TimerGameCalendar::Year{30};
	engine->info.base_life = TimerGameCalendar::Year{0xFF};
	engine->info.climates = {LandscapeType::Temperate};
	engine->info.cargo_type = GetCargoTypeByLabel(capability.cargo_labels.front());
	engine->info.cargo_label = capability.cargo_labels.front();
	engine->info.refit_mask = {};
	// Exact cargo labels retain the mandatory physical TBM spoil leg; broad
	// classes are not an alternate commissioning or disposal route.
	for (const auto label : capability.cargo_labels) engine->info.refit_mask.Set(GetCargoTypeByLabel(label));
	engine->info.load_amount = 8;
	engine->info.refit_cost = 10;
	engine->info.misc_flags.Set(EngineMiscFlag::NoDefaultCargoMultiplier);
	engine->info.extra_flags.Set({ExtraEngineFlag::NoNews, ExtraEngineFlag::NoPreview});
	engine->info.string_id = STR_JUST_RAW_STRING;
	engine->name = std::string{capability.name};
	engine->intro_date = engine->info.base_intro;
	engine->reliability = 0xFFFF;
	engine->reliability_spd_dec = 16;
	auto &rail = engine->VehInfo<RailVehicleInfo>();
	// Native spacing comes from the same physical source contract used by
	// authoring. The delivered single-headed prototype still has one actual part;
	// publishing geometry for later designs does not install their definitions.
	assert(geometry->native_parts == 1);
	rail.railveh_type = RailVehicleType::Singlehead;
	rail.shorten_factor = VEHICLE_LENGTH - geometry->vehicle_length_units;
	rail.railtypes = {tube};
	rail.intended_railtypes = rail.railtypes;
	rail.engclass = EngineClass::Maglev;
	// Bounded prototype physics reuse the existing train owner. Full five-model
	// balance, metered propulsion and final art remain with their later groups.
	rail.max_speed = 100;
	rail.power = 1000;
	rail.weight = 20;
	rail.capacity = 60;
	rail.tractive_effort = 0x80;
	rail.air_drag = 8;
	rail.running_cost = 100;
	rail.running_cost_class = Price::RunningTrainElectric;
	rail.cost_factor = 120;
	rail.image_index = 0;
	engine->original_image_index = rail.image_index;
	return true;
}

/** @copydoc CanTubeTrainEnterTile */
bool CanTubeTrainEnterTile(const Train &train, TileIndex tile)
{
	if (!IsValidTile(tile)) return false;
	const bool tube_tile = IsTubeRailType(GetTileRailType(tile));
	// Every physical part must belong to the same destination domain. This also
	// catches stale/mixed consists before broad cached masks can authorize entry.
	for (const Train *part = train.First(); part != nullptr; part = part->Next()) {
		const auto capsule = GetTubeVehicleCapability(part->engine_type);
		if (capsule.has_value() != tube_tile) return false;
		if (!capsule) continue;
		const auto layer = GetTileLayer(tile);
		const auto &selection = GetCurrentMultiLayerProfileSelection();
		const auto *profile = GetMultiLayerContentProfile(selection.id);
		// Capsules never cross ordinary shafts or borrow Surface/Space/legacy
		// authority; model and Deep unlocks use the synchronized calendar.
		if (profile == nullptr || selection.id != MultiLayerGameplayProfileID::MultilayerIndustry || selection.legacy ||
			selection.version != profile->version || !capsule->catalogue->supported_layers.Test(layer) ||
			GetTileLayer(part->tile) != layer || IsLayerPortalTile(tile) || IsTileType(tile, TileType::TunnelBridge) ||
			TimerGameCalendar::date < TimerGameCalendar::ConvertYMDToDate(capsule->catalogue->introduction_year, 0, 1)) return false;
		if (layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(selection, TimerGameCalendar::date)) return false;
	}
	return true;
}
