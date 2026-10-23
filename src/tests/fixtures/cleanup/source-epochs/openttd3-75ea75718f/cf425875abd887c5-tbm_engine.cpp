/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_engine.cpp Native definitions through the real engine pool and persisted EIDS owner. @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 2 */

#include "stdafx.h"
#include "tbm.h"
#include "engine_base.h"
#include "table/strings.h"

#include "safeguards.h"

static constexpr uint16_t TBM_ENGINE_LOCAL_BASE = 0x8000; ///< Reserved native range inside the pinned package's existing EIDS scope.

/**
 * Resolve the existing first-party mapping without introducing a feature-local engine registry.
 * @param model_id Logical native design.
 * @return Live matching engine or Invalid.
 */
EngineID GetTBMEngine(uint16_t model_id)
{
	if (model_id > 3) return EngineID::Invalid();
	const auto *profile = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto id = _engine_mngr.GetID(VehicleType::Train, TBM_ENGINE_LOCAL_BASE + model_id, profile->required_content->package.grfid);
	const auto *engine = Engine::GetIfValid(id);
	// Only the actual native definition may inherit the capability; a missing or foreign pool record refuses.
	if (engine == nullptr || engine->type != VehicleType::Train || engine->grf_prop.local_id != TBM_ENGINE_LOCAL_BASE + model_id) return EngineID::Invalid();
	return id;
}

/**
 * Classify against the sole persisted native engine mapping.
 * @param engine Actual pool identity.
 * @return Logical cutter/hopper design or UINT16_MAX.
 */
uint16_t GetTBMEngineModel(EngineID engine)
{
	if (engine == EngineID::Invalid()) return UINT16_MAX;
	const auto *definition = Engine::GetIfValid(engine);
	// Hot articulated/cost paths discard ordinary engines before consulting native mappings.
	if (definition == nullptr || definition->type != VehicleType::Train || definition->grf_prop.local_id < TBM_ENGINE_LOCAL_BASE || definition->grf_prop.local_id > TBM_ENGINE_LOCAL_BASE + 3) return UINT16_MAX;
	const auto model = static_cast<uint16_t>(definition->grf_prop.local_id - TBM_ENGINE_LOCAL_BASE);
	return GetTBMEngine(model) == engine ? model : UINT16_MAX;
}

/**
 * Identify machinery requiring validated portal-only construction.
 * @param engine Actual pool identity.
 * @return True for either native cutter model.
 */
bool IsTBMCutterEngine(EngineID engine)
{
	return GetTBMEngineModel(engine) < 2;
}

/**
 * Reconstruct definitions after loader activation while retaining original and saved EIDS identities.
 * @param activated Actual pinned loader configuration; cargo meanings remain in the shared registry.
 * @return Complete availability or refusal before allocation for invalid content/capacity.
 */
bool InitialiseTBMEngineDefinitions(const GRFConfigList &activated)
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	if (CheckMultiLayerContentIdentity(required, activated) != MultiLayerContentIdentityError::None) return false;
	uint missing = 0;
	// Validate every existing mapping before changing any definition; malformed saved/foreign records cannot seize a native identity.
	for (uint16_t model = 0; model < 4; ++model) {
		const auto local = static_cast<uint16_t>(TBM_ENGINE_LOCAL_BASE + model);
		const auto id = _engine_mngr.GetID(VehicleType::Train, local, required.package.grfid);
		const auto *engine = Engine::GetIfValid(id);
		if (engine != nullptr && (engine->type != VehicleType::Train || engine->grf_prop.local_id != local)) return false; // A colliding mapping is refused before any original or native definition is overwritten.
		if (engine == nullptr) ++missing;
	}
	if (!Engine::CanAllocateItem(missing)) return false;
	const auto capabilities = GetMultiLayerVehicleCapabilities();
	BuildCargoLabelMap(); // Rebuild only the shared registry's derived lookup after activation.
	// EIDS owns reload identity; a fresh world appends definitions rather than replacing original engine tables.
	for (uint16_t model = 0; model < 4; ++model) {
		const auto local = static_cast<uint16_t>(TBM_ENGINE_LOCAL_BASE + model);
		auto id = _engine_mngr.GetID(VehicleType::Train, local, required.package.grfid);
		Engine *engine;
		if (id == EngineID::Invalid()) {
			engine = Engine::Create(VehicleType::Train, local);
			id = engine->index;
			_engine_mngr.SetID(VehicleType::Train, local, required.package.grfid, model < 2 ? 0 : 20, id);
		} else {
			engine = Engine::GetIfValid(id);
			if (engine == nullptr) engine = Engine::CreateAtIndex(id, VehicleType::Train, local);
		}
		const auto capability = std::ranges::find(capabilities, model, &LayerVehicleCapability::model_id);
		assert(capability != capabilities.end());
		engine->info.base_intro = TimerGameCalendar::ConvertYMDToDate(capability->introduction_year, 0, 1);
		engine->info.lifelength = TimerGameCalendar::Year{30};
		engine->info.base_life = TimerGameCalendar::Year{0xFF};
		engine->info.climates = {LandscapeType::Temperate};
		engine->info.cargo_type = GetCargoTypeByLabel(model == 3 ? CargoLabel{"ROCK"} : CargoLabel{"ERTH"});
		engine->info.cargo_label = model == 3 ? CargoLabel{"ROCK"} : CargoLabel{"ERTH"};
		engine->info.refit_mask = {};
		engine->info.load_amount = 8;
		engine->info.extra_flags.Set({ExtraEngineFlag::NoNews, ExtraEngineFlag::NoPreview});
		engine->info.string_id = STR_JUST_RAW_STRING;
		engine->name = std::string{capability->name};
		engine->intro_date = engine->info.base_intro;
		engine->reliability = 0xFFFF;
		engine->reliability_spd_dec = 16;
		auto &rail = engine->VehInfo<RailVehicleInfo>();
		rail.railveh_type = model < 2 ? RailVehicleType::Singlehead : RailVehicleType::Wagon;
		rail.railtypes = {RAILTYPE_RAIL}; rail.intended_railtypes = rail.railtypes;
		rail.engclass = EngineClass::Diesel;
		rail.max_speed = model < 2 ? GetTBMCutterModel(model)->travel_speed : 80;
		rail.power = model < 2 ? GetTBMCutterModel(model)->power_hp : 0;
		rail.weight = model < 2 ? 120 : 24;
		rail.capacity = model == 2 ? 64 : model == 3 ? 128 : 0;
		rail.tractive_effort = 0x80; rail.air_drag = 8;
		rail.running_cost = 0; // Controller state-duration accounting owns recurring costs; geology owns paid cut/support fees.
		rail.cost_factor = model < 2 ? 200 : 100;
		rail.image_index = model < 2 ? 0 : 13; // Existing base art supports development; final TBM assets remain gated by Plan 13.
		engine->original_image_index = rail.image_index;
	}
	return true;
}
