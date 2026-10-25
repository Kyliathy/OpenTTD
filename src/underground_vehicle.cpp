/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_vehicle.cpp Native conventional fleet using canonical catalogue meanings and existing EIDS persistence. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 7 */

#include "stdafx.h"
#include "underground_vehicle.h"
#include "transport_tube.h"
#include "engine_base.h"
#include "tbm.h"
#include "table/strings.h"

#include "safeguards.h"

static constexpr uint16_t CONVENTIONAL_ENGINE_LOCAL_BASE = 0x8000; ///< Extend the first-party native EIDS range after the four delivered TBM definitions; no second mapping owner.

/** @param capability Canonical ground design. @return Its ordinary engine category. */
static VehicleType GetConventionalType(const LayerVehicleCapability &capability)
{
	return capability.transport == MultiLayerVehicleTransport::Rail ? VehicleType::Train : VehicleType::Road;
}

/** @param model_id Immutable catalogue identity. @return Its catalogue row, or nullptr outside the conventional family. */
static const LayerVehicleCapability *GetConventionalModel(uint16_t model_id)
{
	// Portal-only machinery and later tube/orbital designs must never acquire ordinary depot identities here.
	if (model_id < 2 || model_id > 8) return nullptr;
	const auto catalogue = GetMultiLayerVehicleCapabilities();
	const auto found = std::ranges::find(catalogue, model_id, &LayerVehicleCapability::model_id);
	return found == catalogue.end() ? nullptr : &*found;
}

/** @copydoc GetUndergroundEngine */
EngineID GetUndergroundEngine(uint16_t model_id)
{
	const auto *capability = GetConventionalModel(model_id);
	if (capability == nullptr) return EngineID::Invalid();
	if (model_id < 4) return GetTBMEngine(model_id);
	const auto type = GetConventionalType(*capability);
	const auto &package = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package;
	const auto local = static_cast<uint16_t>(CONVENTIONAL_ENGINE_LOCAL_BASE + model_id);
	const auto id = _engine_mngr.GetID(type, local, package.grfid);
	const auto *definition = Engine::GetIfValid(id);
	// Only a live definition in the pinned package's real mapping can inherit its layer/cargo capability.
	if (definition == nullptr || definition->type != type || definition->grf_prop.local_id != local) return EngineID::Invalid();
	return id;
}

/** @copydoc GetUndergroundEngineCapability */
const LayerVehicleCapability *GetUndergroundEngineCapability(EngineID engine)
{
	const auto *definition = Engine::GetIfValid(engine);
	if (definition == nullptr) return nullptr;
	const auto local = definition->grf_prop.local_id;
	// Reject original/foreign definitions cheaply in hot refit and list paths, before consulting the persisted mapping.
	if (local < CONVENTIONAL_ENGINE_LOCAL_BASE + 2 || local > CONVENTIONAL_ENGINE_LOCAL_BASE + 8) return nullptr;
	const auto model = static_cast<uint16_t>(local - CONVENTIONAL_ENGINE_LOCAL_BASE);
	return GetUndergroundEngine(model) == engine ? GetConventionalModel(model) : nullptr;
}

/** @copydoc CheckUndergroundVehicleCapability */
CommandCost CheckUndergroundVehicleCapability(EngineID engine, MapLayer layer, CargoType cargo)
{
	const auto tube = GetTubeVehicleCapability(engine);
	const auto *capability = GetUndergroundEngineCapability(engine);
	// Native capsules share the existing profile/date/layer/cargo policy, so purchase lists and commands cannot disagree.
	if (capability == nullptr && tube.has_value()) capability = tube->catalogue;
	if (capability == nullptr) return CommandCost();
	const auto &selection = GetCurrentMultiLayerProfileSelection();
	const auto *profile = GetMultiLayerContentProfile(selection.id);
	// Native depot designs are unavailable under Classic, malformed or legacy policy, even if a script toggles an engine's company flag.
	if (profile == nullptr || selection.id != MultiLayerGameplayProfileID::MultilayerIndustry || selection.legacy || selection.version != profile->version) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
	if (TimerGameCalendar::date < TimerGameCalendar::ConvertYMDToDate(capability->introduction_year, 0, 1)) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
	if (layer != MapLayer::Invalid) {
		if (!MultiLayerMap::IsValidLayer(layer)) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
		if (!capability->supported_layers.Test(layer)) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
		// Deep construction/refit remains technology-gated independently of each design's own introduction year.
		if (layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(selection, TimerGameCalendar::date)) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
	}
	// Exact labels prevent broad cargo classes from allowing passengers, unsealed minerals or unrelated later fuels.
	if (cargo != INVALID_CARGO && !capability->cargo_labels.empty()) {
		if (cargo >= NUM_CARGO) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
		if (std::ranges::find(capability->cargo_labels, CargoSpec::Get(cargo)->label) == capability->cargo_labels.end()) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
	}
	return CommandCost();
}

/** @copydoc InitialiseUndergroundEngineDefinitions */
bool InitialiseUndergroundEngineDefinitions(const GRFConfigList &activated)
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	if (CheckMultiLayerContentIdentity(required, activated) != MultiLayerContentIdentityError::None) return false;
	BuildCargoLabelMap();
	uint missing = 0;
	// Preflight the complete appended family before allocation, preserving every saved/original definition on collision or missing cargo.
	for (uint16_t model = 4; model <= 8; ++model) {
		const auto &capability = *GetConventionalModel(model);
		for (CargoLabel label : capability.cargo_labels) {
			if (!IsValidCargoType(GetCargoTypeByLabel(label))) return false;
		}
		const auto type = GetConventionalType(capability);
		const auto local = static_cast<uint16_t>(CONVENTIONAL_ENGINE_LOCAL_BASE + model);
		const auto id = _engine_mngr.GetID(type, local, required.package.grfid);
		const auto *engine = Engine::GetIfValid(id);
		// A saved mapping cannot redirect a conventional definition onto another vehicle type or package-local slot.
		if (engine != nullptr && (engine->type != type || engine->grf_prop.local_id != local)) return false;
		if (engine == nullptr) ++missing;
	}
	if (!IsValidCargoType(GetCargoTypeByLabel(CargoLabel{"COAL"}))) return false;
	if (!Engine::CanAllocateItem(missing)) return false;
	// Reconstruct definitions at the existing EIDS IDs; the immutable catalogue supplies identity, dates and exact refit meanings.
	for (uint16_t model = 4; model <= 8; ++model) {
		const auto &capability = *GetConventionalModel(model);
		const auto type = GetConventionalType(capability);
		const auto local = static_cast<uint16_t>(CONVENTIONAL_ENGINE_LOCAL_BASE + model);
		auto id = _engine_mngr.GetID(type, local, required.package.grfid);
		Engine *engine;
		if (id == EngineID::Invalid()) {
			engine = Engine::Create(type, local);
			id = engine->index;
			_engine_mngr.SetID(type, local, required.package.grfid, 0, id);
		} else {
			engine = Engine::GetIfValid(id);
			if (engine == nullptr) engine = Engine::CreateAtIndex(id, type, local);
		}
		const CargoLabel default_label = capability.cargo_labels.empty() ? CargoLabel{"COAL"} : capability.cargo_labels.front();
		engine->info.base_intro = TimerGameCalendar::ConvertYMDToDate(capability.introduction_year, 0, 1);
		engine->info.lifelength = TimerGameCalendar::Year{30};
		engine->info.base_life = TimerGameCalendar::Year{0xFF};
		engine->info.climates = {LandscapeType::Temperate};
		engine->info.cargo_type = GetCargoTypeByLabel(default_label);
		engine->info.cargo_label = default_label;
		engine->info.refit_mask = {};
		// Resolve refit slots from the activated shared registry, never from a hard-coded cargo index table.
		for (CargoLabel label : capability.cargo_labels) engine->info.refit_mask.Set(GetCargoTypeByLabel(label));
		engine->info.load_amount = 8;
		engine->info.refit_cost = 10;
		engine->info.misc_flags.Set(EngineMiscFlag::NoDefaultCargoMultiplier); // Native capacity is measured in the catalogue cargo's own units across every permitted refit.
		engine->info.extra_flags.Set({ExtraEngineFlag::NoNews, ExtraEngineFlag::NoPreview});
		engine->info.string_id = STR_JUST_RAW_STRING;
		engine->name = std::string{capability.name};
		engine->intro_date = engine->info.base_intro;
		engine->reliability = 0xFFFF;
		engine->reliability_spd_dec = 16;
		if (type == VehicleType::Train) {
			auto &rail = engine->VehInfo<RailVehicleInfo>();
			rail.railveh_type = model == 4 ? RailVehicleType::Singlehead : RailVehicleType::Wagon;
			rail.railtypes = {RAILTYPE_RAIL};
			rail.intended_railtypes = rail.railtypes;
			rail.engclass = EngineClass::Diesel;
			rail.max_speed = 80;
			rail.power = model == 4 ? 1000 : 0;
			rail.weight = model == 4 ? 60 : 24;
			rail.capacity = model == 4 ? 0 : model == 5 ? 60 : 40;
			rail.tractive_effort = 0x80;
			rail.air_drag = 8;
			rail.running_cost = model == 4 ? 120 : 0;
			rail.running_cost_class = Price::RunningTrainDiesel;
			rail.cost_factor = model == 4 ? 140 : 80;
			rail.image_index = model == 4 ? 0 : model == 5 ? 34 : 32; // Existing diesel/bulk/covered art is development-only; final family assets belong to Plan 13.
			engine->original_image_index = rail.image_index;
		} else {
			auto &road = engine->VehInfo<RoadVehicleInfo>();
			road.roadtype = ROADTYPE_ROAD;
			road.max_speed = 140;
			road.power = 35;
			road.weight = 80;
			road.capacity = model == 7 ? 32 : 24;
			road.tractive_effort = 0x80;
			road.running_cost = 100;
			road.running_cost_class = Price::RunningRoadveh;
			road.cost_factor = model == 7 ? 130 : 160;
			road.image_index = model == 7 ? 18 : 19; // Ordinary base truck sprites until the matching final assets are delivered.
			engine->original_image_index = road.image_index;
		}
	}
	return true;
}
