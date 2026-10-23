/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_definitions.cpp Actual five-model native purchase, geometry, energy and packed-art boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group10.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Runtime art binding and normal new-world controls retain their dispatched integration gates.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_fixture.h"
#include "../transport_tube_model.h"
#include "../multilayer_bootstrap.h"
#include "../multilayer_sprite.h"
#include <set>
#include "../safeguards.h"

/** Five actual purchased originals borrow exact canonical cargo/date/geometry and distinct native tuning. */
TEST_CASE("Five native capsule models retain separate actual engines capacity traction and energy", "[transport-tube-definitions]")
{
	TubePrototypeRig world;
	AutoRestoreBackup date{TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2050}, 0, 1)};
	AutoRestoreBackup year{TimerGameCalendar::year, TimerGameCalendar::Year{2050}};
	AutoRestoreBackup<Money> electric_price{_price[Price::RunningTrainElectric], Money{1000}}; ///< Explicit nonzero ordinary price baseline; isolated fixtures start uninitialized prices at zero.
	std::set<EngineID> identities;
	const std::array<uint8_t, 5> capacities{60, 40, 48, 24, 80};
	const std::array<uint16_t, 5> speeds{100, 130, 90, 90, 70};
	const auto energy = GetMultiLayerTubeEnergyModels();
	uint models = 0;
	// Each immutable catalogue row is exercised as an actual command purchase;
	// no definition alias or five refits of one pool object can satisfy this proof.
	for (const auto &catalogue : GetMultiLayerVehicleCapabilities()) {
		if (catalogue.transport != MultiLayerVehicleTransport::Tube) continue;
		const EngineID engine = GetTubeEngine(catalogue.model_id);
		REQUIRE(engine != EngineID::Invalid());
		CHECK(identities.insert(engine).second);
		const auto *tuning = GetTubeVehicleDefinition(catalogue.model_id);
		REQUIRE(tuning != nullptr);
		Engine::Get(engine)->company_avail.Set(_current_company);
		const auto [result, id, capacity, mail, cargo_capacities] = CmdBuildVehicle(DoCommandFlag::Execute,
			world.depot, engine, false, INVALID_CARGO, ClientID::Invalid);
		REQUIRE(result.Succeeded());
		Train *capsule = Train::Get(id);
		CHECK(capsule->engine_type == engine);
		CHECK(capsule->Next() == nullptr);
		CHECK(capsule->gcache.cached_veh_length == VEHICLE_LENGTH);
		CHECK(capsule->cargo_cap == capacities.at(models));
		CHECK(capsule->GetEngine()->VehInfo<RailVehicleInfo>().max_speed == speeds.at(models));
		CHECK(capsule->GetEngine()->grf_prop.local_id == 0x8000 + catalogue.model_id);
		CHECK(capsule->GetEngine()->name == catalogue.name);
		CHECK(capsule->GetEngine()->intro_date == TimerGameCalendar::ConvertYMDToDate(catalogue.introduction_year, 0, 1));
		CHECK(capsule->GetEngine()->VehInfo<RailVehicleInfo>().cost_factor == tuning->purchase_factor);
		CHECK(capsule->GetEngine()->VehInfo<RailVehicleInfo>().weight == tuning->empty_weight_t);
		CHECK(capsule->GetEngine()->VehInfo<RailVehicleInfo>().power == tuning->traction_hp);
		CHECK(capsule->GetRunningCost() > 0);
		const auto capability = GetTubeVehicleCapability(engine);
		REQUIRE(capability.has_value());
		CHECK(capability->catalogue == &catalogue);
		CHECK(capability->guideway == world.tube);
		// Every label is checked against the single logical catalogue; broad cargo
		// classes must not turn liquids/sealed minerals into bulk or packaged goods.
		for (CargoType cargo = static_cast<CargoType>(0); cargo < NUM_CARGO; ++cargo) {
			const auto label = CargoSpec::Get(cargo)->label;
			const bool admitted = std::ranges::find(catalogue.cargo_labels, label) != catalogue.cargo_labels.end();
			CHECK(capsule->GetEngine()->info.refit_mask.Test(cargo) == admitted);
		}
		const auto model_energy = std::ranges::find(energy, catalogue.model_id, &MultiLayerTubeEnergyModel::model_id);
		REQUIRE(model_energy != energy.end());
		const auto empty = ComputeTubePowerDemand(*capsule, TubePowerPhase::Motion);
		REQUIRE(empty.has_value());
		CHECK(empty->rate_w == model_energy->empty_motion_w);
		// Real packet-backed full load changes demand through its normal native
		// capacity; source animation and profile traction never create power grants.
		REQUIRE(CargoPacket::CanAllocateItem());
		capsule->cargo.Append(CargoPacket::Create(StationID::Invalid(), capsule->cargo_cap,
			Source{Source::Invalid, SourceType::Industry}));
		const auto full = ComputeTubePowerDemand(*capsule, TubePowerPhase::Motion);
		REQUIRE(full.has_value());
		CHECK(full->rate_w == model_energy->full_motion_w);
		++models;
	}
	CHECK(models == 5);
	CHECK(identities.size() == 5);
	const auto count = Engine::GetNumItems();
	CHECK(InitialiseTubeEngineDefinitions(world.ActivatedContent()));
	CHECK(Engine::GetNumItems() == count);
	CHECK_FALSE(GetTubeVehicleDefinition(8));
	CHECK_FALSE(GetTubeVehicleDefinition(14));
}

/** Late-model refusal preserves every earlier engine's identity and configuration before publication. */
TEST_CASE("Capsule family preflight refuses late missing cargo or colliding EIDS before earlier writes", "[transport-tube-definition-family-refusal]")
{
	TubePrototypeRig world;
	Engine *original = Engine::Get(GetTubeEngine(9));
	original->name = "preflight-sentinel";
	const auto engines = Engine::GetNumItems();
	SECTION("missing Forge robotics input") {
		CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"RBPT"}))->label = CargoLabel{"MISS"};
		BuildCargoLabelMap();
	}
	SECTION("foreign engine in Forge mapping") {
		const auto &package = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package;
		_engine_mngr.SetID(VehicleType::Train, 0x800D, package.grfid, 0, GetUndergroundEngine(4));
	}
	SECTION("two models share an absent persisted slot") {
		const auto &package = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package;
		const EngineID absent{500};
		REQUIRE_FALSE(Engine::IsValidID(absent));
		_engine_mngr.SetID(VehicleType::Train, 0x8009, package.grfid, 0, absent);
		_engine_mngr.SetID(VehicleType::Train, 0x800D, package.grfid, 0, absent);
	}
	CHECK_FALSE(InitialiseTubeEngineDefinitions(world.ActivatedContent()));
	CHECK(original->name == "preflight-sentinel");
	CHECK(Engine::GetNumItems() == engines);
}

/** A new capsule cannot take the first free engine slot when that slot already belongs to a later saved model. */
TEST_CASE("Capsule reconstruction reserves persisted late model holes before new engines", "[transport-tube-definition-family-refusal]")
{
	TBMEngineFixture world;
	REQUIRE(InitialiseTubeRailType(world.package) != INVALID_RAILTYPE);
	uint16_t slot = 0;
	// Find the native allocator's first vacant slot, then give that same exact
	// identity to a later persisted model before any new capsule is installed.
	while (Engine::IsValidID(EngineID{slot})) ++slot;
	const EngineID saved{slot};
	const auto &package = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package;
	_engine_mngr.SetID(VehicleType::Train, 0x800A, package.grfid, 0, saved);
	REQUIRE(InitialiseTubeEngineDefinitions(world.package));
	CHECK(GetTubeEngine(10) == saved);
	std::set<EngineID> identities;
	// Every actual model must survive reconstruction as its own EIDS identity,
	// including the earlier unmapped Mole and the explicitly reserved Crate slot.
	for (uint16_t model = 9; model <= 13; ++model) {
		REQUIRE(GetTubeEngine(model) != EngineID::Invalid());
		CHECK(identities.insert(GetTubeEngine(model)).second);
	}
	CHECK(identities.size() == 5);
}

/** Every original owns all eight load/status views and a distinct two-state purchase range in the loaded native payload. */
TEST_CASE("Five native capsule definitions have complete distinct packed model art", "[transport-tube-model-art]")
{
	TubePrototypeRig world;
	const std::array bodies{MultiLayerSpriteAsset::VehicleModel09, MultiLayerSpriteAsset::VehicleModel10,
		MultiLayerSpriteAsset::VehicleModel11, MultiLayerSpriteAsset::VehicleModel12, MultiLayerSpriteAsset::VehicleModel13};
	const std::array purchases{MultiLayerSpriteAsset::PurchaseModel09, MultiLayerSpriteAsset::PurchaseModel10,
		MultiLayerSpriteAsset::PurchaseModel11, MultiLayerSpriteAsset::PurchaseModel12, MultiLayerSpriteAsset::PurchaseModel13};
	std::set<SpriteID> sprites;
	// All nine actual quantity/service/blocked combinations retain two original
	// indicator phases and eight physical views, rather than sharing one recolored hull.
	for (uint model = 0; model < bodies.size(); ++model) {
		REQUIRE(GetTubeEngine(9 + model) != EngineID::Invalid());
		for (uint8_t state = 0; state < 9; ++state) {
			for (uint8_t frame = 0; frame < 2; ++frame) {
				for (uint8_t direction = 0; direction < 8; ++direction) {
					const auto sprite = GetMultiLayerSprite(bodies[model], {state, frame, direction, 0, 0});
					REQUIRE(sprite.has_value());
					CHECK(sprites.insert(*sprite).second);
				}
			}
		}
		CHECK_FALSE(GetMultiLayerSprite(bodies[model], {9}).has_value());
		// Available/locked purchase views belong to this same distinct logical
		// model; a date policy is still enforced by the command, not these pixels.
		for (uint8_t state : {0, 1}) {
			const auto sprite = GetMultiLayerSprite(purchases[model], {state});
			REQUIRE(sprite.has_value());
			CHECK(sprites.insert(*sprite).second);
		}
	}
	CHECK(sprites.size() == 730);
}
