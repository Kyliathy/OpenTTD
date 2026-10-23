/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_content.cpp Revised construction, mass, processing and admission contracts; Plan1 Group12B. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_space_content.h"
#include "../multilayer_bootstrap.h"
#include "../cargotype.h"
#include "../safeguards.h"

/** Deliberately remapped public cargo registry; runtime native tests additionally check the shipped weights. */
struct SpaceMassRegistry {
	std::array<CargoSpec, NUM_CARGO> original;
	CargoTypes mask = _cargo_mask;
	SpaceMassRegistry()
	{
		for (uint i = 0; i < NUM_CARGO; ++i) { original[i] = *CargoSpec::Get(i); *CargoSpec::Get(i) = CargoSpec{}; }
		_cargo_mask = {};
		uint8_t i = 40;
		for (const auto label : {CargoLabel{"STEL"}, CargoLabel{"GOOD"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"RBPT"}, CargoLabel{"NORE"}, CargoLabel{"RARE"}}) {
			auto *spec = CargoSpec::Get(i); spec->label = label; spec->bitnum = i; spec->weight = label == CargoLabel{"GOOD"} ? 8 : 16; _cargo_mask.Set(CargoType{i++});
		}
		BuildCargoLabelMap();
	}
	~SpaceMassRegistry() { for (uint i = 0; i < NUM_CARGO; ++i) *CargoSpec::Get(i) = original[i]; _cargo_mask = mask; BuildCargoLabelMap(); }
};

TEST_CASE("Revised Space construction admits paid facilities and conserves crate mass", "[space-content]")
{
	SpaceMassRegistry registry;
	const auto definitions = GetSpaceConstructionDefinitions();
	REQUIRE(definitions.size() == to_underlying(SpaceBuildKind::End));
	std::set<SpaceBuildKind> kinds;
	for (const auto &d : definitions) {
		CHECK(kinds.insert(d.kind).second);
		CHECK(std::ranges::find(d.inputs, CargoLabel{"STEL"}, &MultiLayerRecipeCargo::label) != d.inputs.end());
		CHECK(std::ranges::find(d.inputs, CargoLabel{"GOOD"}, &MultiLayerRecipeCargo::label) != d.inputs.end());
		REQUIRE(GetMultiLayerPayloadMass(d.inputs)); CHECK(*GetMultiLayerPayloadMass(d.inputs) == d.dry_mass_sixteenths);
		CHECK(d.capital > 0); CHECK(d.work_ticks > 0);
	}
	const MultiLayerRecipeCargo first[] = {{CargoLabel{"STEL"}, 6}, {CargoLabel{"GOOD"}, 12}, {CargoLabel{"RFMT"}, 1}, {CargoLabel{"ELEC"}, 1}};
	CHECK(GetMultiLayerPayloadMass(first) == 14 * 16);
	const auto body = GetSpaceVehicleConstruction(20)->dry_mass_sixteenths;
	const auto service = *GetMultiLayerPayloadMass(GetSpaceProbeServiceInputs());
	CHECK(body + service + 16 * 16 == 26 * 16);
	CHECK(body + service + 24 * 8 == 22 * 16);
	CHECK(16 * 16 + 24 * 8 == GetSpaceConstructionDefinition(SpaceBuildKind::Core)->dry_mass_sixteenths);
	const MultiLayerRecipeCargo wide[] = {{CargoLabel{"GOOD"}, UINT32_MAX}};
	CHECK(GetMultiLayerPayloadMass(wide) == uint64_t{UINT32_MAX} * 8);
	const MultiLayerRecipeCargo missing[] = {{CargoLabel{"ZZZZ"}, 1}};
	CHECK_FALSE(GetMultiLayerPayloadMass(missing));
	for (uint16_t model : {15, 19, 20}) {
		CHECK(CanCommissionSpaceVehicle(model, MultiLayerCommissioningFacility::RocketPayload, true));
		CHECK(CanCommissionSpaceVehicle(model, MultiLayerCommissioningFacility::OrbitalAssembly, true));
		CHECK_FALSE(CanCommissionSpaceVehicle(model, MultiLayerCommissioningFacility::OrbitalAssembly, false));
		CHECK_FALSE(CanCommissionSpaceVehicle(model, MultiLayerCommissioningFacility::RailDepot, true));
	}
	CHECK_FALSE(CanCommissionSpaceVehicle(16, MultiLayerCommissioningFacility::RocketPayload, true));
	CHECK_FALSE(CanCommissionSpaceVehicle(18, MultiLayerCommissioningFacility::RocketPayload, true));
	const auto signature = GetMultiLayerSpaceContentSignature(); CHECK(signature != 0);
	CHECK(IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID::MultilayerIndustry, 2, signature));
	CHECK_FALSE(IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID::MultilayerIndustry, 1, signature));
	CHECK_FALSE(IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID::MultilayerIndustry, 2, signature ^ 1));
	CHECK(IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID::Classic, 0, 0));
	CHECK_FALSE(IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID::Classic, 2, signature));
	CHECK(GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->version == 1);
}

TEST_CASE("Orbital Steel and Goods require all material energy and output headroom", "[space-content]")
{
	SpaceMassRegistry registry;
	for (const auto id : {MultiLayerRecipeID{"OSTL"}, MultiLayerRecipeID{"OGDS"}}) {
		const auto recipes = GetMultiLayerManufacturingRecipes();
		const auto it = std::ranges::find_if(recipes, [&](const auto &d) { return d.recipe.id == id; });
		REQUIRE(it != recipes.end()); const auto &r = it->recipe;
		std::vector<MultiLayerRecipeStock> in, out;
		for (const auto &c : r.inputs) in.push_back({c.label, c.quantity, 1024});
		for (const auto &c : r.outputs) out.push_back({c.label, 1024 - c.quantity + 1, 1024});
		CHECK(PlanMultiLayerRecipeBatches(r, in, out, 74, 0, 0, 55500, 0).limiting == MultiLayerRecipeLimit::Output);
		for (size_t i = 0; i < in.size(); ++i) CHECK(in[i].amount == r.inputs[i].quantity);
		out[0].amount = 0;
		CHECK(PlanMultiLayerRecipeBatches(r, in, out, 74, 0, 0, 55499, 0).limiting == MultiLayerRecipeLimit::Power);
		CHECK(PlanMultiLayerRecipeBatches(r, in, out, 74, 0, 0, 55500, 0).batches == 1);
		CHECK(*GetMultiLayerPayloadMass(r.inputs) == *GetMultiLayerPayloadMass(r.outputs) + it->residue_mass_sixteenths);
		CHECK(r.reserve_per_batch == 0); // The processor cannot debit an asteroid a second time.
		for (auto &c : in) { const auto keep = c.amount; c.amount = 0; CHECK(PlanMultiLayerRecipeBatches(r, in, out, 74, 0, 0, 55500, 0).batches == 0); c.amount = keep; }
	}
	CHECK(4 * 16 + 1 * 16 == 4 * 8 + (32 + 16)); // NORE/RARE -> four Goods crates plus recorded residue.
	const auto milestones = GetMultiLayerBootstrapMilestones();
	uint64_t ground = 0;
	for (const auto &m : milestones.first(6)) ground += m.capital_cost;
	CHECK(ground == 1680000);
	const auto core = std::ranges::find(milestones, MultiLayerBootstrapStage::Station, &MultiLayerBootstrapMilestone::stage);
	REQUIRE(core != milestones.end());
	CHECK(std::ranges::find(core->prerequisites, MultiLayerBootstrapStage::ConstructionProbes) != core->prerequisites.end());
	const auto heavy = std::ranges::find(milestones, MultiLayerBootstrapStage::HeavyRobot, &MultiLayerBootstrapMilestone::stage);
	REQUIRE(heavy != milestones.end());
	CHECK(std::ranges::find(heavy->prerequisites, MultiLayerBootstrapStage::Assembly) != heavy->prerequisites.end());
}
