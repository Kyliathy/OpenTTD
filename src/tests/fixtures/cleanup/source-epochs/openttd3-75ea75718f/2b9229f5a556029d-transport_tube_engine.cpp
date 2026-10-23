/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_engine.cpp Five native capsule definitions and exclusive physical movement domain.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group1B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md canonical catalogue
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group10B physical source geometry
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group10 native definitions
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DCBA canonical startup/saved launch dates
 */

#include "stdafx.h"
#include "transport_tube.h"
#include "transport_tube_model.h"
#include "engine_base.h"
#include "rail.h"
#include "train.h"
#include "layer_portal_map.h"
#include "table/strings.h"
#include <set>

#include "safeguards.h"

static constexpr uint16_t TUBE_ENGINE_LOCAL_BASE = 0x8000; ///< Existing append-only EIDS namespace; model9 retains local0x8009.

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
	if (GetTubeVehicleDefinition(model_id) == nullptr) return EngineID::Invalid();
	const auto &package = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package;
	const uint16_t local = TUBE_ENGINE_LOCAL_BASE + model_id;
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
	if (GetTubeEngine(model) != engine) return std::nullopt;
	const auto geometry = GetTubeVehicleGeometry(model);
	if (!geometry) return std::nullopt;
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
 * Configure one preflighted real engine without changing the catalogue or allocation authority.
 * @param engine Actual newly allocated or reconstructed native definition.
 * @param geometry Borrowed canonical capability and unchanged physical source geometry.
 * @param tuning Immutable native model parameters; energy grants remain separate.
 * @param tube Actual admitted canonical railtype, never a hard-coded slot.
 */
static void ConfigureTubeEngine(Engine &engine, const TubeVehicleGeometry &geometry, const TubeVehicleDefinition &tuning, RailType tube)
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
	// authoring. Each original capsule has exactly one full native part, rather
	// than shrinking its physical length to fit newly selected source art.
	assert(geometry.native_parts == 1);
	rail.railveh_type = RailVehicleType::Singlehead;
	rail.shorten_factor = VEHICLE_LENGTH - geometry.vehicle_length_units;
	rail.railtypes = {tube};
	rail.intended_railtypes = rail.railtypes;
	rail.engclass = EngineClass::Maglev;
	// Distinct native tuning affects actual capacity/traction/prices. Electrical
	// demand still uses the canonical per-model load envelope and one paid tick.
	rail.max_speed = tuning.max_speed;
	rail.power = tuning.traction_hp;
	rail.weight = tuning.empty_weight_t;
	rail.capacity = tuning.capacity;
	rail.tractive_effort = 0x80;
	rail.air_drag = 8;
	rail.running_cost = tuning.running_factor;
	rail.running_cost_class = Price::RunningTrainElectric;
	rail.cost_factor = tuning.purchase_factor;
	rail.image_index = 0;
	engine.original_image_index = rail.image_index;
}

/** @copydoc InitialiseTubeEngineDefinitions */
bool InitialiseTubeEngineDefinitions(const GRFConfigList &activated)
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	const RailType tube = GetTubeRailType();
	// Exact native content and the semantic registry precede every allocation;
	// a foreign GRF cannot partially install this family or choose its refits.
	if (tube == INVALID_RAILTYPE || CheckMultiLayerContentIdentity(required, activated) != MultiLayerContentIdentityError::None) return false;
	uint missing = 0;
	std::set<EngineID> persisted;
	// Preflight the complete family before publishing any EIDS row. Missing late
	// model cargo or a persisted mapping collision must also preserve model9.
	for (const auto &tuning : _tube_definitions) {
		const auto geometry = GetTubeVehicleGeometry(tuning.model_id);
		if (!geometry) return false;
		for (CargoLabel label : geometry->catalogue->cargo_labels) {
			if (!IsValidCargoType(GetCargoTypeByLabel(label))) return false;
		}
		const uint16_t local = TUBE_ENGINE_LOCAL_BASE + tuning.model_id;
		const EngineID id = _engine_mngr.GetID(VehicleType::Train, local, required.package.grfid);
		// Even absent pool slots have one persisted owner. Two models naming the
		// same future slot must refuse before either could overwrite the other.
		if (id != EngineID::Invalid() && !persisted.insert(id).second) return false;
		const Engine *engine = Engine::GetIfValid(id);
		// Existing live slots must belong to this precise model; reconstruction
		// may fill an absent slot but must never overwrite another vehicle domain.
		if (engine != nullptr && (engine->type != VehicleType::Train || engine->grf_prop.local_id != local)) return false;
		if (engine == nullptr) ++missing;
	}
	if (!Engine::CanAllocateItem(missing)) return false;
	// Reconstruct all saved holes first. Otherwise a new unmapped model could
	// take a vacant pool index that already belongs to a later persisted model.
	for (const auto &tuning : _tube_definitions) {
		const uint16_t local = TUBE_ENGINE_LOCAL_BASE + tuning.model_id;
		const EngineID id = _engine_mngr.GetID(VehicleType::Train, local, required.package.grfid);
		if (id == EngineID::Invalid()) continue;
		if (Engine::GetIfValid(id) == nullptr) Engine::CreateAtIndex(id, VehicleType::Train, local);
	}
	// Once all fallible admissions pass, reconstruct each append-only native row
	// through the ordinary engine pool and EIDS mapping; no second registry exists.
	for (const auto &tuning : _tube_definitions) {
		const uint16_t local = TUBE_ENGINE_LOCAL_BASE + tuning.model_id;
		const auto id = _engine_mngr.GetID(VehicleType::Train, local, required.package.grfid);
		Engine *engine = Engine::GetIfValid(id);
		if (engine == nullptr) {
			assert(id == EngineID::Invalid());
			engine = Engine::Create(VehicleType::Train, local);
			_engine_mngr.SetID(VehicleType::Train, local, required.package.grfid, 0, engine->index);
		}
		ConfigureTubeEngine(*engine, *GetTubeVehicleGeometry(tuning.model_id), tuning, tube);
	}
	return true;
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
