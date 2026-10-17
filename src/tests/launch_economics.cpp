/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_economics.cpp Revised launch, persistent-probe and first-Core numerical ledgers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group9.
 * @see docs/zz-rhr/upgrades/2026-10/r2mlvs9-a-launch-ledgers.md
 * These content-budget checks complement actual paid native flight/descent fixtures;
 * they neither commission a hull nor prove a completed orbital facility.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_bootstrap.h"
#include "../multilayer_space_content.h"
#include "../launch_center.h"
#include "../safeguards.h"

/**
 * Price physical inputs through the public bounded Surface procurement catalogue.
 * @param inputs Whole-unit canonical cargo quantities consumed or retained by the scenario.
 * @return Uninflated internal Money sum; a missing published seed fails the fixture.
 */
static uint64_t InputPrice(std::span<const MultiLayerRecipeCargo> inputs)
{
	uint64_t total = 0;
	const auto supplies = GetMultiLayerSurfaceSupplies();
	// Price each declared cargo independently so Goods crates retain their own unit price.
	for (const auto &input : inputs) {
		const auto supply = std::ranges::find(supplies, input.label, &MultiLayerSurfaceSupply::label);
		REQUIRE(supply != supplies.end());
		total += uint64_t{input.quantity} * supply->price_per_unit;
	}
	return total;
}

/**
 * Price an explicit fuel purchase independently of the reusable hull and payload.
 * @param quantity Whole kilolitres purchased; unburned fuel stays in the native compartment.
 * @return Published procurement debit for this purchase.
 */
static uint64_t FuelPrice(uint32_t quantity)
{
	const std::array inputs{MultiLayerRecipeCargo{CargoLabel{"PROP"}, quantity}};
	return InputPrice(inputs);
}

/** Verify two flights charge the same paid hull once and retain the first flight's unused fuel. */
TEST_CASE("Reusable rocket ledgers separate capital fuel mission and pad work", "[launch-economics]")
{
	const auto &budget = GetMultiLayerSpaceBudget();
	const auto *rocket = GetSpaceConstructionDefinition(SpaceBuildKind::Rocket);
	REQUIRE(rocket != nullptr);
	const uint32_t round_trip_fuel = uint32_t{budget.outbound_fuel} + budget.recovery_fuel;
	const uint32_t pad_ticks = uint32_t{budget.prepare_ticks} + budget.turnaround_ticks;
	const uint64_t pad_cost_numerator = uint64_t{pad_ticks} * budget.pad_per_day;
	REQUIRE(pad_cost_numerator % 74 == 0);
	const uint64_t pad_cost = pad_cost_numerator / 74;
	CHECK(round_trip_fuel == 15);
	CHECK(pad_ticks == 222);
	CHECK(pad_cost == 1500);
	CHECK(rocket->capital == 400000);
	CHECK(InputPrice(rocket->inputs) == 3120);
	CHECK(rocket->dry_mass_sixteenths == 20 * 16);
	CHECK(budget.rocket_payload_sixteenths == 30 * 16);

	const uint32_t first_purchase = 16;
	REQUIRE(first_purchase >= round_trip_fuel);
	const uint32_t retained_fuel = first_purchase - round_trip_fuel;
	const uint32_t repeat_purchase = round_trip_fuel - retained_fuel;
	const uint64_t first_flight = FuelPrice(first_purchase) + budget.mission_fee + pad_cost;
	const uint64_t second_flight = FuelPrice(repeat_purchase) + budget.mission_fee + pad_cost;
	CHECK(retained_fuel == 1);
	CHECK(repeat_purchase == 14);
	CHECK(first_flight == 10500);
	CHECK(second_flight == 10000);
	CHECK(first_flight + second_flight == 2 * (FuelPrice(round_trip_fuel) + budget.mission_fee + pad_cost));
	CHECK(rocket->capital + InputPrice(rocket->inputs) + first_flight + second_flight == 423620);
}

/** Verify the original 25,900 subtotal and the separately required two-day onboard service upkeep. */
TEST_CASE("Persistent mining probe pays two shells and service without a second probe launch", "[launch-economics]")
{
	const auto &budget = GetMultiLayerSpaceBudget();
	const auto *probe = GetSpaceConstructionDefinition(SpaceBuildKind::MiningProbe);
	const auto *pod = GetSpaceConstructionDefinition(SpaceBuildKind::Pod);
	REQUIRE(probe != nullptr);
	REQUIRE(pod != nullptr);
	const auto service_inputs = GetSpaceProbeServiceInputs();
	CHECK(InputPrice(probe->inputs) == 1360);
	CHECK(InputPrice(pod->inputs) == 340);
	CHECK(InputPrice(service_inputs) == 680);
	CHECK(probe->capital + budget.starting_pods * pod->capital == 104000);
	CHECK(InputPrice(probe->inputs) + budget.starting_pods * InputPrice(pod->inputs) + InputPrice(service_inputs) == 2720);
	CHECK(probe->dry_mass_sixteenths + budget.starting_pods * pod->dry_mass_sixteenths + 2 * 16 == 14 * 16);

	const uint32_t whole_tonnes = 2 * (budget.probe_hold_sixteenths / 16);
	REQUIRE(whole_tonnes % budget.mining_units == 0);
	const uint32_t mining_ticks = whole_tonnes / budget.mining_units * budget.mining_interval;
	REQUIRE(uint64_t{mining_ticks} * budget.active_per_day % 74 == 0);
	const uint64_t mining_work = uint64_t{mining_ticks} * budget.active_per_day / 74;
	const uint64_t pad_work = (uint64_t{budget.prepare_ticks} + budget.turnaround_ticks) * budget.pad_per_day / 74;
	const uint64_t shell_replacement = InputPrice(pod->inputs) + pod->capital;
	const uint64_t subtotal = FuelPrice(16) + budget.starting_pods * shell_replacement + InputPrice(service_inputs) +
		mining_work + budget.mission_fee + pad_work + whole_tonnes;
	REQUIRE(uint64_t{budget.service_ticks} * budget.active_per_day % 74 == 0);
	const uint64_t service_work = uint64_t{budget.service_ticks} * budget.active_per_day / 74;
	CHECK(whole_tonnes == 40);
	CHECK(mining_ticks == 1480);
	CHECK(mining_work == 10000);
	CHECK(shell_replacement == 2340);
	CHECK(subtotal == 25900);
	CHECK(service_work == 1000);
	CHECK(subtotal + service_work == 26900);
	CHECK(budget.returns_per_service == 2);
	CHECK(budget.starting_pods == 2);
}

/** Verify both physical first-Core cargo manifests, capital and sequential work without a prebuilt station. */
TEST_CASE("Two material launches price the first Core without repurchasing the rocket", "[launch-economics]")
{
	const auto &budget = GetMultiLayerSpaceBudget();
	const auto *builder = GetSpaceConstructionDefinition(SpaceBuildKind::ConstructionProbe);
	const auto *core = GetSpaceConstructionDefinition(SpaceBuildKind::Core);
	REQUIRE(builder != nullptr);
	REQUIRE(core != nullptr);
	const uint64_t body_mass = builder->dry_mass_sixteenths;
	// Native mass fixtures check STEL=16 and GOOD=8 sixteenths. Both cargoes
	// remain loose freight here; the Core is assembled only after both deliveries.
	const uint64_t steel_mass = 16 * 16;
	const uint64_t goods_mass = 24 * 8;
	const uint64_t service_mass = 2 * 16;
	CHECK(body_mass + service_mass + steel_mass == 26 * 16);
	CHECK(body_mass + service_mass + goods_mass == 22 * 16);
	CHECK(body_mass + service_mass + steel_mass <= budget.rocket_payload_sixteenths);
	CHECK(body_mass + service_mass + goods_mass <= budget.rocket_payload_sixteenths);
	CHECK(steel_mass <= budget.probe_hold_sixteenths);
	CHECK(goods_mass <= budget.probe_hold_sixteenths);
	CHECK(steel_mass + goods_mass == core->dry_mass_sixteenths);
	CHECK(InputPrice(core->inputs) == 4480);

	const uint32_t fuel = 2 * (uint32_t{budget.outbound_fuel} + budget.recovery_fuel);
	const uint32_t flight_ticks = 2 * (uint32_t{budget.prepare_ticks} + 2 * budget.flight_ticks + budget.turnaround_ticks);
	const uint64_t pad_cost = 2 * (uint64_t{budget.prepare_ticks} + budget.turnaround_ticks) * budget.pad_per_day / 74;
	const uint64_t flight_cost = FuelPrice(fuel) + 2 * budget.mission_fee + pad_cost;
	const uint64_t project_cost = 2 * builder->capital + 2 * InputPrice(builder->inputs) +
		2 * InputPrice(GetSpaceProbeServiceInputs()) + InputPrice(core->inputs) + core->capital + flight_cost;
	CHECK(fuel == 30);
	CHECK(flight_ticks == 1332);
	CHECK(2 * builder->work_ticks + flight_ticks + core->work_ticks + budget.core_activation_ticks == 2442);
	CHECK(pad_cost == 3000);
	CHECK(flight_cost == 20500);
	CHECK(project_cost == 929060);
}
