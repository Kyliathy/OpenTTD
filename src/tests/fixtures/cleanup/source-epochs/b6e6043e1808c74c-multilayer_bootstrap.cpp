/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_bootstrap.cpp Calendar, route, cash and acyclic Surface-supply proof. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_bootstrap.h"

#include "../safeguards.h"

/** Verify all first-build milestones expand from paid Surface supplies and earlier routes. */
TEST_CASE("All first-build milestones expand from paid Surface supplies and earlier routes", "[multilayer-bootstrap]")
{
	const auto milestones = GetMultiLayerBootstrapMilestones();
	const auto supplies = GetMultiLayerSurfaceSupplies();
	REQUIRE(milestones.size() == 11);
	std::map<MultiLayerBootstrapStage, TimerGameCalendar::Year> reachable;
	uint64_t cash = 5000000;
	uint64_t paid_supplies = 0;
	// Topological expansion rejects backward/self dependencies and checks the cumulative cash ledger at every milestone.
	for (const auto &milestone : milestones) {
		CHECK_FALSE(reachable.contains(milestone.stage));
		for (const auto prerequisite : milestone.prerequisites) {
			REQUIRE(reachable.contains(prerequisite));
			CHECK(reachable.at(prerequisite) <= milestone.introduction_year);
		}
		uint64_t expense = milestone.capital_cost;
		// Resolve each delivered material through the one paid seed catalogue; no unreachable Deep product is implicit.
		for (const auto &purchase : milestone.purchases) {
			const auto supply = std::ranges::find(supplies, purchase.label, &MultiLayerSurfaceSupply::label);
			REQUIRE(supply != supplies.end());
			CHECK(supply->introduction_year <= milestone.introduction_year);
			CHECK(purchase.quantity <= supply->capacity_per_day);
			REQUIRE(GetMultiLayerCargoContract(purchase.label) != nullptr);
			const uint64_t cost = uint64_t{purchase.quantity} * supply->price_per_unit;
			expense += cost;
			paid_supplies += cost;
		}
		REQUIRE(cash >= expense);
		cash -= expense;
		reachable.emplace(milestone.stage, milestone.introduction_year);
	}
	CHECK(paid_supplies == 76380);
	CHECK(cash == 1219620);
	CHECK(reachable.at(MultiLayerBootstrapStage::Tube) == TimerGameCalendar::Year{1970});
	CHECK(reachable.at(MultiLayerBootstrapStage::Geothermal) == TimerGameCalendar::Year{1980});
	CHECK(reachable.at(MultiLayerBootstrapStage::Probe) == TimerGameCalendar::Year{1990});
	CHECK(reachable.at(MultiLayerBootstrapStage::HeavyRobot) == TimerGameCalendar::Year{1995});
	const auto mines = GetMultiLayerMineRecipes();
	REQUIRE(mines.size() == 10);
	std::set<CargoLabel> resources;
	// Each distinct mine must have a dated conventional receiver before extraction; future heat plants are not bootstrap receivers.
	for (const auto &mine : mines) {
		CHECK(resources.insert(mine.resource).second);
		CHECK(mine.receiver_year <= mine.introduction_year);
		CHECK_FALSE(mine.receiver.empty());
		CHECK(ValidateMultiLayerRecipe(mine.recipe) == MultiLayerRecipeLimit::None);
		CHECK(mine.recipe.reserve_label == mine.resource);
		const auto access = mine.layer == MapLayer::DeepUnderground ? MultiLayerBootstrapStage::DeepTBM : MultiLayerBootstrapStage::TBM;
		CHECK(reachable.at(access) <= mine.introduction_year);
	}
}

/** Verify vehicle catalogue provides conventional resource paths without tube or liquid bypasses. */
TEST_CASE("Vehicle catalogue provides conventional resource paths without tube or liquid bypasses", "[multilayer-vehicle-catalogue]")
{
	const auto vehicles = GetMultiLayerVehicleCapabilities();
	REQUIRE(vehicles.size() == 19);
	std::set<uint16_t> models;
	// Capability invariants apply to every logical design before feature owners map it to an engine pool.
	for (const auto &vehicle : vehicles) {
		CHECK(models.insert(vehicle.model_id).second);
		CHECK(vehicle.commissioning != MultiLayerCommissioningFacility::Invalid);
		CHECK(vehicle.service != MultiLayerServiceFacility::Invalid);
		if (vehicle.transport == MultiLayerVehicleTransport::Tube) {
			CHECK_FALSE(vehicle.conventional_portals);
			CHECK_FALSE(vehicle.supported_layers.Test(MapLayer::Surface));
			CHECK_FALSE(vehicle.supported_layers.Test(MapLayer::Space));
			CHECK(vehicle.commissioning == MultiLayerCommissioningFacility::TubeDepot);
			CHECK(vehicle.service == MultiLayerServiceFacility::TubeService);
		}
		// Every allowed liquid is carried through a liquid refit, never merely because a container class fits.
		for (const auto label : vehicle.cargo_labels) {
			const auto *cargo = GetMultiLayerCargoContract(label);
			REQUIRE(cargo != nullptr);
			if (cargo->form == MultiLayerCargoForm::Liquid) CHECK(vehicle.cargo_classes.Test(CargoClass::Liquid));
			// A later bulk/tube leg cannot bypass the required actual spoil-wagon journey to Surface.
			if (label == CargoLabel{"ERTH"} || label == CargoLabel{"ROCK"}) CHECK(vehicle.mandatory_spoil_wagon_leg);
		}
	}
	// A conventional carrier must exist by each mine's date; Shield-85 is not Uranium's first available route.
	for (const auto &mine : GetMultiLayerMineRecipes()) {
		bool conventional = false;
		for (const auto &vehicle : vehicles) {
			// Only ordinary rail/road designs prove a bootstrap route before special tube/Space facilities exist.
			if (vehicle.transport != MultiLayerVehicleTransport::Rail && vehicle.transport != MultiLayerVehicleTransport::Road) continue;
			if (vehicle.introduction_year > mine.introduction_year) continue;
			if (!vehicle.supported_layers.Test(mine.layer)) continue;
			if (std::ranges::find(vehicle.cargo_labels, mine.resource) != vehicle.cargo_labels.end()) conventional = true;
		}
		CHECK(conventional);
	}
}
