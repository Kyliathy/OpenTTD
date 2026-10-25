/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_engine.cpp Native capsule definitions, complete fleet preflight and exclusive movement domain.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group1B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md canonical catalogue
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group10B physical source geometry
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group10 original fleet definitions
 */

#include "stdafx.h"
#include "transport_tube.h"
#include "transport_tube_model.h"
#include "multilayer_bootstrap.h"
#include "engine_base.h"
#include "rail.h"
#include "train.h"
#include "layer_portal_map.h"
#include "table/strings.h"

#include "safeguards.h"

static constexpr uint16_t TUBE_ENGINE_LOCAL_BASE = 0x8000; ///< Existing append-only native EIDS namespace shared with canonical ground model identities.

/** First-party native tuning; model9 stays byte-compatible with the delivered prototype. Energy remains in multilayer_bootstrap.cpp. */
static constexpr TubeVehicleDefinition _tube_definitions[] = {
	{9, 100, 1000, 20, 60, 120, 100},
	{10, 130, 1100, 18, 40, 150, 110},
	{11, 90, 1000, 28, 48, 180, 140},
	{12, 90, 950, 45, 24, 220, 160},
	{13, 70, 1500, 48, 80, 240, 220},
};

/** @copydoc GetTubeVehicleDefinition */
const TubeVehicleDefinition *GetTubeVehicleDefinition(uint16_t model_id)
{
	const auto found = std::ranges::find(_tube_definitions, model_id, &TubeVehicleDefinition::model_id);
	return found == std::end(_tube_definitions) ? nullptr : &*found;
}

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

/** @copydoc GetTubeEngine */
EngineID GetTubeEngine(uint16_t model_id)
{
	if (!GetTubeVehicleGeometry(model_id).has_value()) return EngineID::Invalid();
	const uint16_t local = TUBE_ENGINE_LOCAL_BASE + model_id;
	const auto &package = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package;
	const auto id = _engine_mngr.GetID(VehicleType::Train, local, package.grfid);
	const auto *engine = Engine::GetIfValid(id);
	// Only the actual native EIDS row can identify a capsule. Foreign definitions
	// and stale mappings cannot acquire this domain from broad railtype masks.
	if (engine == nullptr || engine->type != VehicleType::Train || engine->grf_prop.local_id != local) return EngineID::Invalid();
	return id;
}

/** @copydoc GetTubeVehicleCapability */
std::optional<TubeVehicleCapability> GetTubeVehicleCapability(EngineID engine)
{
	const auto *definition = Engine::GetIfValid(engine);
	// Ordinary engines avoid a mapping lookup on the movement hot path. A copied
	// package-local number alone still cannot impersonate the native definition.
	if (definition == nullptr || definition->type != VehicleType::Train || definition->grf_prop.local_id < TUBE_ENGINE_LOCAL_BASE) return std::nullopt;
	const uint16_t model = definition->grf_prop.local_id - TUBE_ENGINE_LOCAL_BASE;
	const auto geometry = GetTubeVehicleGeometry(model);
	// An ordinary native model in the same namespace still has no tube movement
	// domain. Resolve the actual package mapping before borrowing its capability.
	if (!geometry.has_value() || GetTubeEngine(model) != engine) return std::nullopt;
	return TubeVehicleCapability{engine, geometry->catalogue, GetTubeRailType()};
}

/** @copydoc RestoreTubeEngineIntroduction */
void RestoreTubeEngineIntroduction(Engine &engine)
{
	const auto capability = GetTubeVehicleCapability(engine.index);
	if (!capability) return;
	engine.intro_date = TimerGameCalendar::ConvertYMDToDate(capability->catalogue->introduction_year, 0, 1);
	// Older saves can retain an ordinary randomized capsule date and a pending
	// publication flag. Canonical native originals have no exclusive preview:
	// once their actual launch date has arrived, publish that existing definition
	// for all companies without recreating vehicles, aging or world quantities.
	if (TimerGameCalendar::date >= engine.intro_date && !engine.flags.Test(EngineFlag::Available)) {
		engine.flags.Reset(EngineFlag::ExclusivePreview).Set(EngineFlag::Available);
		engine.company_avail.Set();
	}
}

/**
 * Apply already admitted native coefficients to the actual EIDS definition.
 * @param engine Existing or newly allocated real definition, exclusively owned by this model's native mapping.
 * @param geometry Borrowed canonical capability and native physical-part contract.
 * @param tuning Selected delivered per-model coefficients; labels/dates/energy retain their original owners.
 * @param tube Previously admitted dedicated native railtype.
 */
static void ApplyTubeDefinition(Engine &engine, const TubeVehicleGeometry &geometry, const TubeVehicleDefinition &tuning, RailType tube)
{
	const auto &capability = *geometry.catalogue;
	engine.info.base_intro = TimerGameCalendar::ConvertYMDToDate(capability.introduction_year, 0, 1);
	engine.info.lifelength = TimerGameCalendar::Year{30};
	engine.info.base_life = TimerGameCalendar::Year{0xFF};
	engine.info.climates = {LandscapeType::Temperate};
	engine.info.cargo_type = GetCargoTypeByLabel(capability.cargo_labels.front());
	engine.info.cargo_label = capability.cargo_labels.front();
	engine.info.refit_mask = {};
	// Exact cargo labels retain the mandatory physical TBM spoil leg; broad
	// classes are not an alternate commissioning or disposal route.
	for (const auto label : capability.cargo_labels) engine.info.refit_mask.Set(GetCargoTypeByLabel(label));
	engine.info.load_amount = 8;
	engine.info.refit_cost = 10;
	engine.info.misc_flags.Set(EngineMiscFlag::NoDefaultCargoMultiplier);
	engine.info.extra_flags.Set({ExtraEngineFlag::NoNews, ExtraEngineFlag::NoPreview});
	engine.info.string_id = STR_JUST_RAW_STRING;
	engine.name = std::string{capability.name};
	engine.intro_date = engine.info.base_intro;
	engine.reliability = 0xFFFF;
	engine.reliability_spd_dec = 16;
	auto &rail = engine.VehInfo<RailVehicleInfo>();
	// Native spacing comes from the same physical source contract used by
	// authoring. All five original hulls are one full native self-propelled part.
	assert(geometry.native_parts == 1);
	rail.railveh_type = RailVehicleType::Singlehead;
	rail.shorten_factor = VEHICLE_LENGTH - geometry.vehicle_length_units;
	rail.railtypes = {tube};
	rail.intended_railtypes = rail.railtypes;
	rail.engclass = EngineClass::Maglev;
	// Distinct payload/speed/mass/handling coefficients use native acceleration
	// and economy. Electrical work still requires its independent paid grant.
	rail.max_speed = tuning.max_speed;
	rail.power = tuning.traction_hp;
	rail.weight = tuning.empty_weight_t;
	rail.capacity = static_cast<uint8_t>(tuning.capacity);
	rail.tractive_effort = 0x80;
	rail.air_drag = 8;
	rail.running_cost = tuning.running_factor;
	rail.running_cost_class = Price::RunningTrainElectric;
	rail.cost_factor = tuning.purchase_factor;
	rail.image_index = 0;
	engine.original_image_index = rail.image_index;
}

/** A completely preflighted native model; this temporary plan owns no saved identity, cargo or utility quantity. */
struct TubeDefinitionPlan {
	TubeVehicleGeometry geometry; ///< Borrowed canonical model and physical-part contract.
	const TubeVehicleDefinition *tuning; ///< Borrowed immutable native coefficients.
	EngineID id; ///< Existing persisted EIDS slot, or Invalid until the ordinary engine pool allocates one.
	Engine *engine; ///< Actual existing/new definition, never a private replacement pool.
};

/**
 * Preflight the entire requested native definition set before its first publication.
 * @param activated Exact current native content/cargo package.
 * @param models Complete canonical five-design catalogue in stable model order.
 * @return Complete installation or unchanged prerequisite/collision/capacity refusal.
 */
static bool InitialiseTubeModels(const GRFConfigList &activated, std::span<const uint16_t> models)
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	const auto tube = GetTubeRailType();
	// Neither a foreign package nor an unbounded/empty request can publish a
	// partial native fleet or create its own semantic guideway identity.
	if (tube == INVALID_RAILTYPE || models.empty() || models.size() > 5 ||
		CheckMultiLayerContentIdentity(required, activated) != MultiLayerContentIdentityError::None) return false;
	std::vector<TubeDefinitionPlan> plans;
	std::set<EngineID> fixed_ids;
	size_t missing = 0;
	// Validate the complete future fleet first. A missing final heavy/liquid
	// cargo or foreign persisted slot must not leave earlier models installed.
	for (const uint16_t model : models) {
		const auto geometry = GetTubeVehicleGeometry(model);
		const auto *tuning = GetTubeVehicleDefinition(model);
		if (!geometry.has_value() || tuning == nullptr || tuning->capacity > UINT8_MAX) return false; // Native part capacity and canonical domain must fit before publication.
		const auto energy = GetMultiLayerTubeEnergyModels();
		if (std::ranges::find(energy, model, &MultiLayerTubeEnergyModel::model_id) == energy.end()) return false;
		// The canonical model owns every exact physical cargo form. Resolving all
		// labels here prevents a late model from broadening a previously installed refit.
		for (const CargoLabel label : geometry->catalogue->cargo_labels) {
			if (!IsValidCargoType(GetCargoTypeByLabel(label))) return false;
		}
		const uint16_t local = TUBE_ENGINE_LOCAL_BASE + model;
		const auto id = _engine_mngr.GetID(VehicleType::Train, local, required.package.grfid);
		// Persisted references must be bounded and uniquely assigned before any
		// missing slot is reconstructed; two models cannot own the same engine.
		if (id != EngineID::Invalid() && (id >= EngineID::End() || !fixed_ids.insert(id).second)) return false;
		Engine *engine = Engine::GetIfValid(id);
		if (engine != nullptr && (engine->type != VehicleType::Train || engine->grf_prop.local_id != local)) return false; // Never overwrite an original/foreign definition through a forged mapping.
		if (engine == nullptr) ++missing;
		plans.push_back({*geometry, tuning, id, engine});
	}
	if (missing > 0 && !Engine::CanAllocateItem(missing)) return false; // The whole missing set must fit; an exhausted pool cannot publish only its earlier models.
	// Restore fixed persisted slots before allocating fresh ones. Otherwise the
	// ordinary first-free allocator could claim a later model's reserved EIDS slot.
	for (auto &plan : plans) {
		if (plan.engine != nullptr || plan.id == EngineID::Invalid()) continue; // Existing definitions and genuinely fresh mappings need no fixed-slot reconstruction.
		plan.engine = Engine::CreateAtIndex(plan.id, VehicleType::Train, TUBE_ENGINE_LOCAL_BASE + plan.geometry.catalogue->model_id);
	}
	// Every fallible semantic/capacity check has completed. Materialize remaining
	// fresh definitions, then apply their original coefficients in catalogue order.
	for (auto &plan : plans) {
		const uint16_t local = TUBE_ENGINE_LOCAL_BASE + plan.geometry.catalogue->model_id;
		if (plan.engine == nullptr) {
			plan.engine = Engine::Create(VehicleType::Train, local);
			_engine_mngr.SetID(VehicleType::Train, local, required.package.grfid, 0, plan.engine->index);
		}
		ApplyTubeDefinition(*plan.engine, plan.geometry, *plan.tuning, tube);
	}
	return true;
}

/** @copydoc InitialiseTubeEngineDefinitions */
bool InitialiseTubeEngineDefinitions(const GRFConfigList &activated)
{
	std::array<uint16_t, std::size(_tube_definitions)> models;
	// The sole frozen definition table determines family size and ordering;
	// every requested identity is still checked against canonical geometry,
	// cargo and energy before either allocation pass can publish an engine.
	std::ranges::transform(_tube_definitions, models.begin(), &TubeVehicleDefinition::model_id);
	return InitialiseTubeModels(activated, models);
}

/** @copydoc InitialiseTubeContentDefinitions */
bool InitialiseTubeContentDefinitions(const GRFConfigList &activated)
{
	if (InitialiseTubeRailType(activated) == INVALID_RAILTYPE) return false;
	return InitialiseTubeEngineDefinitions(activated);
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
