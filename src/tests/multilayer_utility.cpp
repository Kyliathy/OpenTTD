/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_utility.cpp Conservation, interval subdivision, linked layers and shared heat fixtures. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_utility.h"
#include "../multilayer_bootstrap.h"

#include "../safeguards.h"

/**
 * Assert every account and the whole proposal conserve generated electrical energy.
 * @param plan Valid pure proposal to check; imports/exports cancel globally.
 */
static void CheckUtilityConservation(const UtilityAllocationPlan &plan)
{
	REQUIRE(plan.valid);
	uint64_t generated = 0;
	uint64_t consumed = 0;
	uint64_t unused = 0;
	// Each source owns its own debit/credit identity; global export circulation must create no extra energy.
	for (const auto &producer : plan.producers) {
		CHECK(producer.generated_j + producer.imported_j == producer.exported_j + producer.consumed_j + producer.unused_j);
		generated += producer.generated_j;
		consumed += producer.consumed_j;
		unused += producer.unused_j;
	}
	CHECK(generated == consumed + unused);
}

/** Verify utility rate integration retains small fractions and rejects unsafe bounds. */
TEST_CASE("Utility rate integration retains small fractions and rejects unsafe bounds", "[multilayer-utility]")
{
	const auto whole = IntegrateUtilityGeneration(17, 37, 0);
	REQUIRE(whole);
	CHECK(whole->energy_j == 16);
	CHECK(whole->remainder_mj == 983);
	uint64_t accumulated = 0;
	uint16_t remainder = 0;
	// Rendering may subdivide observations, but fixed simulation intervals plus carried fractions give the same generation.
	for (uint32_t tick = 0; tick < 37; ++tick) {
		const auto step = IntegrateUtilityGeneration(17, 1, remainder);
		REQUIRE(step);
		accumulated += step->energy_j;
		remainder = step->remainder_mj;
	}
	CHECK(accumulated == whole->energy_j);
	CHECK(remainder == whole->remainder_mj);
	const auto maximum = IntegrateUtilityGeneration(MAX_UTILITY_RATE_W, MAX_UTILITY_INTERVAL_TICKS, 999);
	REQUIRE(maximum);
	CHECK(maximum->energy_j == MAX_UTILITY_ACCOUNT_ENERGY_J);
	CHECK(maximum->remainder_mj == 999);
	CHECK_FALSE(IntegrateUtilityGeneration(MAX_UTILITY_RATE_W + 1, 1, 0));
	CHECK_FALSE(IntegrateUtilityGeneration(1, MAX_UTILITY_INTERVAL_TICKS + 1, 0));
	CHECK_FALSE(IntegrateUtilityGeneration(1, 1, 1000));
}

/** Verify stable electricity allocation handles shortage zero supply and explicit exports. */
TEST_CASE("Stable electricity allocation handles shortage zero supply and explicit exports", "[multilayer-utility]")
{
	const UtilityProducerID surface_source[] = {UtilityProducerID{2}};
	const UtilityProducerID deep_source[] = {UtilityProducerID{1}};
	const UtilityProducerSnapshot sources[] = {{UtilityProducerID{2}, MapLayer::Surface, 0}, {UtilityProducerID{1}, MapLayer::DeepUnderground, 100}};
	const UtilityConsumerDemand demands[] = {{UtilityConsumerID{2}, MapLayer::DeepUnderground, deep_source, 80}, {UtilityConsumerID{1}, MapLayer::Surface, surface_source, 70}};
	const UtilityElectricalExport links[] = {{UtilityLinkID{7}, UtilityProducerID{1}, UtilityProducerID{2}, 40}};
	const auto plan = PlanUtilityAllocation(1, sources, demands, links);
	CheckUtilityConservation(plan);
	REQUIRE(plan.consumers.size() == 2);
	CHECK(plan.consumers[0].id == UtilityConsumerID{1});
	CHECK(plan.consumers[0].granted_j == 40);
	CHECK(plan.consumers[1].granted_j == 60);
	CHECK(plan.links[0].energy_j == 40);
	CHECK(plan.producers[0].exported_j == plan.producers[1].imported_j);
	const auto unlinked = PlanUtilityAllocation(2, sources, demands, {});
	CheckUtilityConservation(unlinked);
	CHECK(unlinked.consumers[0].granted_j == 0);
	CHECK(unlinked.consumers[1].granted_j == 80);
	CHECK(unlinked.producers[0].unused_j == 20);
	const UtilityProducerSnapshot one_source[] = {{UtilityProducerID{2}, MapLayer::Surface, 100}};
	const UtilityConsumerDemand two_consumers[] = {{UtilityConsumerID{8}, MapLayer::Surface, surface_source, 70}, {UtilityConsumerID{3}, MapLayer::Surface, surface_source, 70}};
	const auto shortage = PlanUtilityAllocation(3, one_source, two_consumers, {});
	CheckUtilityConservation(shortage);
	CHECK(shortage.consumers[0].granted_j == 70);
	CHECK(shortage.consumers[1].granted_j == 30);
	const UtilityProducerSnapshot zero[] = {{UtilityProducerID{2}, MapLayer::Surface, 0}};
	const auto starved = PlanUtilityAllocation(4, zero, two_consumers, {});
	CheckUtilityConservation(starved);
	CHECK(starved.consumers[0].granted_j == 0);
	CHECK(starved.consumers[1].granted_j == 0);
	const UtilityConsumerDemand reordered[] = {two_consumers[1], two_consumers[0]};
	const auto same = PlanUtilityAllocation(3, one_source, reordered, {});
	CHECK(same.consumers[0].granted_j == shortage.consumers[0].granted_j);
	CHECK(same.consumers[1].granted_j == shortage.consumers[1].granted_j);
	const UtilityConsumerDemand wrong_layer[] = {{UtilityConsumerID{1}, MapLayer::Surface, deep_source, 70}};
	CHECK_FALSE(PlanUtilityAllocation(5, sources, wrong_layer, {}).valid);
	const UtilityProducerSnapshot duplicate[] = {sources[0], sources[0]};
	CHECK_FALSE(PlanUtilityAllocation(5, duplicate, {}, {}).valid);
	CHECK_FALSE(PlanUtilityAllocation(0, sources, demands, links).valid);
	CHECK_FALSE(PlanUtilityAllocation(UINT64_MAX, sources, demands, links).valid);
	CHECK(sources[1].generated_j == 100);
}

/** Verify vent heat is shared before conversion with its own remainder. */
TEST_CASE("Vent heat is shared before conversion with its own remainder", "[multilayer-utility]")
{
	const UtilityHeatDemand demands[] = {{UtilityConsumerID{2}, UtilityHeatUse::ElectricalGenerator, 600}, {UtilityConsumerID{1}, UtilityHeatUse::DirectRecipe, 600}};
	const auto shares = PlanVentHeatShares(1000, demands);
	REQUIRE(shares);
	CHECK((*shares)[0].use == UtilityHeatUse::DirectRecipe);
	CHECK((*shares)[0].granted_j == 600);
	CHECK((*shares)[1].granted_j == 400);
	const auto electric = ConvertUtilityHeatToElectricity((*shares)[1].granted_j, 35, 0);
	REQUIRE(electric);
	CHECK(electric->electric_j == 140);
	const auto first = ConvertUtilityHeatToElectricity(20, 15, 0);
	REQUIRE(first);
	const auto second = ConvertUtilityHeatToElectricity(23, 15, first->percent_remainder);
	const auto whole = ConvertUtilityHeatToElectricity(43, 15, 0);
	REQUIRE(second);
	REQUIRE(whole);
	CHECK(first->electric_j + second->electric_j == whole->electric_j);
	CHECK(second->percent_remainder == whole->percent_remainder);
	CHECK_FALSE(ConvertUtilityHeatToElectricity(MAX_UTILITY_ACCOUNT_ENERGY_J + 1, 35, 0));
	CHECK_FALSE(ConvertUtilityHeatToElectricity(100, 101, 0));
	CHECK_FALSE(ConvertUtilityHeatToElectricity(100, 35, 100));
	const UtilityHeatDemand repeated[] = {demands[0], demands[0]};
	CHECK_FALSE(PlanVentHeatShares(1000, repeated));
}

/** Verify native useful power has one town authority and only Surface solar is mirror eligible. */
TEST_CASE("Native useful power has one town authority and only Surface solar is mirror eligible", "[multilayer-power-effects]")
{
	const auto *profile = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(profile != nullptr);
	CHECK(profile->town_power_authority == MultiLayerTownPowerAuthority::Native);
	CHECK(IsUtilityMirrorEligible(UtilityGenerationKind::SolarFarm, MapLayer::Surface));
	CHECK_FALSE(IsUtilityMirrorEligible(UtilityGenerationKind::SolarFarm, MapLayer::Space));
	CHECK_FALSE(IsUtilityMirrorEligible(UtilityGenerationKind::Geothermal, MapLayer::Surface));
	CHECK_FALSE(IsUtilityMirrorEligible(UtilityGenerationKind::OrbitalPanel, MapLayer::Space));
	const UtilityProducerID grid[] = {UtilityProducerID{1}, UtilityProducerID{2}};
	// Already-resolved solar output and unmodified geothermal electrical output enter the same allocator once.
	const UtilityProducerSnapshot sources[] = {{UtilityProducerID{1}, MapLayer::Surface, 400}, {UtilityProducerID{2}, MapLayer::Surface, 140}};
	const UtilityConsumerDemand demand[] = {{UtilityConsumerID{1}, MapLayer::Surface, grid, 540}};
	const auto plan = PlanUtilityAllocation(1, sources, demand, {});
	CheckUtilityConservation(plan);
	CHECK(plan.consumers[0].granted_j == 540);
	CHECK(ComputeNativeTownPowerBonus(MultiLayerTownPowerAuthority::Native, 540, 540) == 25);
	CHECK(ComputeNativeTownPowerBonus(MultiLayerTownPowerAuthority::Native, 27, 54) == 12);
	CHECK(ComputeNativeTownPowerBonus(MultiLayerTownPowerAuthority::Native, 0, 54) == 0);
	CHECK(ComputeNativeTownPowerBonus(MultiLayerTownPowerAuthority::Native, 0, 0) == 0);
	CHECK_FALSE(ComputeNativeTownPowerBonus(MultiLayerTownPowerAuthority::ETIGameScript, 540, 540));
	CHECK_FALSE(ComputeNativeTownPowerBonus(MultiLayerTownPowerAuthority::Invalid, 540, 540));
	CHECK_FALSE(ComputeNativeTownPowerBonus(MultiLayerTownPowerAuthority::Native, 541, 540));
	CHECK(GetMultiLayerCargoContract(CargoLabel{"POWR"}) == nullptr);
	const auto effects = GetUtilityConsumerEffects();
	CHECK(effects[0].maximum_bonus_percent == 25);
	CHECK_FALSE(effects[0].required_for_work);
	CHECK(effects[1].required_for_work);
}

/** Verify geothermal bootstrap uses Surface parts and direct recipes share a finite heat budget. */
TEST_CASE("Geothermal bootstrap uses Surface parts and direct recipes share a finite heat budget", "[multilayer-geothermal-bootstrap]")
{
	const auto supplies = GetMultiLayerSurfaceSupplies();
	const auto milestones = GetMultiLayerBootstrapMilestones();
	const auto tube = std::ranges::find(milestones, MultiLayerBootstrapStage::Tube, &MultiLayerBootstrapMilestone::stage);
	const auto geo = std::ranges::find(milestones, MultiLayerBootstrapStage::Geothermal, &MultiLayerBootstrapMilestone::stage);
	REQUIRE(tube != milestones.end());
	REQUIRE(geo != milestones.end());
	CHECK(tube->introduction_year == TimerGameCalendar::Year{1970});
	CHECK(geo->introduction_year == TimerGameCalendar::Year{1980});
	// Every early utility material is bought on Surface by its date; no heat factory supplies its own first vent/riser.
	for (const auto &purchase : geo->purchases) {
		const auto source = std::ranges::find(supplies, purchase.label, &MultiLayerSurfaceSupply::label);
		REQUIRE(source != supplies.end());
		CHECK(source->introduction_year <= geo->introduction_year);
	}
	const auto heat = IntegrateUtilityGeneration(1000000, 74, 0);
	REQUIRE(heat);
	CHECK(heat->energy_j == 1998000);
	const UtilityHeatDemand requests[] = {{UtilityConsumerID{1}, UtilityHeatUse::DirectRecipe, 1000000}, {UtilityConsumerID{2}, UtilityHeatUse::ElectricalGenerator, 1998000}};
	const auto shares = PlanVentHeatShares(heat->energy_j, requests);
	REQUIRE(shares);
	CHECK((*shares)[0].granted_j + (*shares)[1].granted_j == heat->energy_j);
	CHECK((*shares)[1].granted_j == 998000);
	const auto electricity = ConvertUtilityHeatToElectricity((*shares)[1].granted_j, 35, 0);
	REQUIRE(electricity);
	CHECK(electricity->electric_j == 349300);
	const auto recipes = GetMultiLayerGeothermalRecipes();
	REQUIRE(recipes.size() == 6);
	// Each later factory uses the existing atomic planner; zero heat pauses complete production without consuming cargo.
	for (const auto &definition : recipes) {
		CHECK(definition.layer == MapLayer::DeepUnderground);
		CHECK(definition.introduction_year >= TimerGameCalendar::Year{2010});
		CHECK(ValidateMultiLayerRecipe(definition.recipe) == MultiLayerRecipeLimit::None);
		std::vector<MultiLayerRecipeStock> inputs;
		std::vector<MultiLayerRecipeStock> outputs;
		for (const auto &input : definition.recipe.inputs) inputs.push_back({input.label, input.quantity * 2, definition.input_capacity});
		for (const auto &output : definition.recipe.outputs) outputs.push_back({output.label, 0, definition.output_capacity});
		const auto ready = PlanMultiLayerRecipeBatches(definition.recipe, inputs, outputs, 148, 0, 1600000, 30000, 0);
		CHECK(ready.batches == 2);
		CHECK(PlanMultiLayerRecipeBatches(definition.recipe, inputs, outputs, 148, 0, 0, 30000, 0).limiting == MultiLayerRecipeLimit::Heat);
		CHECK(outputs[0].amount == 0);
	}
}
