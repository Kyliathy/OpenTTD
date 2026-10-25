/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */
/**
 * @file utility_scheduler_contention.cpp Actual source shortage and purchased path contention through the sole common driver.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md.
 * The mixed fixture documents supplied floor, consumed frames and received
 * material inputs. All output, native work and money changes use original owners.
 */
#include "utility_scheduler_mixed_fixture.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/** Exact current native/ledger observations from independently built identical worlds; transport ordering grants no priority. */
struct MixedSchedulerObservation {
	Money cash = 0; ///< Original company balance after paid native service and work.
	uint64_t utility = 0, generator = 0, space = 0; ///< Original canonical owner digests.
	std::vector<uint64_t> capsules; ///< Native identities, grants, work receipts, pose and fractional carry in fixed VehicleID order.
	/** @param other Independently executed world. @return Exact observable native work, cash and deterministic accounting equality. */
	bool operator==(const MixedSchedulerObservation &other) const = default;
};

/**
 * Execute original shared requests and native consumers with a declared source/path ceiling.
 * @param model Actual Vent/Magma output, never a synthetic numerical source.
 * @param reverse_rows Reorder only valid binding transport, preserving original native/consumer identities.
 * @return Exact independent-world observations after two real opportunities.
 */
static MixedSchedulerObservation RunMixedContention(GeothermalGeneratorModel model, bool reverse_rows)
{
	UtilitySchedulerMixedRig rig{model};
	const size_t count = model == GeothermalGeneratorModel::VentTap ? 8 : 11;
	std::vector<Train *> capsules;
	// Original native purchase order fixes consumer identity. Reversing later
	// serialized bindings must not change who receives a whole path transfer.
	for (size_t index = 0; index < count; ++index) capsules.push_back(rig.Buy(index != 0));
	if (reverse_rows) {
		auto rows = CaptureTubePowerVehicles(); std::ranges::reverse(rows);
		StageLoadedTubePowerVehicles(std::move(rows), GetTubePowerPreparedTick()); REQUIRE(AcceptLoadedTubePowerVehicles());
	}
	const auto output = GetGeothermalGeneratorState(rig.generator)->electrical_producer;
	const auto before_generation = GetUtilityPowerProducer(output)->generated_lifetime_j;
	const auto before_use = GetUtilityPowerProducer(output)->consumed_lifetime_j;
	const auto before_service = GetGeothermalGeneratorState(rig.generator)->active_service_ticks;
	const auto before_cash = Company::Get(_current_company)->money;
	const auto random = _random;
	MixedSchedulerObservation observation;
	// Both native opportunities repeat the complete preparation once, then
	// invoke actual original consumers. Repeated callbacks below must be pure.
	for (uint tick = 0; tick < 2; ++tick) {
		UpdateSignalsInBuffer(); ++TimerGameTick::counter; PrepareTubePowerTick();
		const auto allocated = ComputeUtilityPowerDigest(), sequence = CaptureUtilityPowerState().operation_sequence;
		const auto cash = Company::Get(_current_company)->money;
		PrepareTubePowerTick(); CHECK(ComputeUtilityPowerDigest() == allocated);
		CHECK(CaptureUtilityPowerState().operation_sequence == sequence); CHECK(Company::Get(_current_company)->money == cash);
		const auto &job = *rig.assembly->module->manufacture;
		const auto assembly_grant = GetUtilityAllocation(rig.assembly->module->consumer); REQUIRE(assembly_grant.has_value());
		const auto recipe_grant = GetUtilityAllocation(rig.processor->module->processing->consumer); REQUIRE(recipe_grant.has_value());
		CHECK(assembly_grant->granted_energy_j == 750); CHECK(recipe_grant->granted_energy_j == 60); // One real 810-J Core panel cannot fund both 750-J requests.
		for (size_t index = 0; index < capsules.size(); ++index) {
			auto *capsule = capsules[index]; const auto *binding = GetTubePowerVehicle(capsule->index); REQUIRE(binding != nullptr);
			const auto grant = GetUtilityAllocation(binding->consumer); REQUIRE(grant.has_value());
			// First interval: one real270-J motion plus540-J depot jobs exceed
			// Vent output or Magma's shared5400-J cable. The final whole repair
			// waits. Second interval: repaired stopped hulls are idle; only the
			// previously refused repair and original moving hull request power.
			const bool admitted = tick == 0 ? index < count - 1 : index == 0 || index == count - 1;
			const uint64_t demand = index == 0 ? 270 : 540;
			CHECK(grant->granted_energy_j == (admitted ? demand : 0));
			const auto tile = capsule->tile; const auto x = capsule->x_pos, y = capsule->y_pos;
			const auto carry = binding->remainder_mj;
			REQUIRE(capsule->Tick()); CHECK_FALSE(capsule->vehstatus.Test(VehState::Crashed));
			binding = GetTubePowerVehicle(capsule->index);
			if (admitted) {
				CHECK(binding->consumed_tick == TimerGameTick::counter); CHECK(binding->wait == TubePowerWait::None);
			} else if (tick == 0) {
				CHECK(binding->consumed_tick != TimerGameTick::counter); CHECK(binding->wait == TubePowerWait::Grant);
				CHECK(capsule->tile == tile); CHECK(capsule->x_pos == x); CHECK(capsule->y_pos == y); CHECK(binding->remainder_mj == carry);
			} else {
				CHECK(binding->consumed_tick != TimerGameTick::counter); CHECK(binding->wait == TubePowerWait::None);
			}
			CHECK(capsule->vehstatus.Test(VehState::Stopped) == (index != 0)); CHECK(capsule->cargo.TotalCount() == 0);
			if (index != 0) CHECK(capsule->reliability == (admitted || tick != 0 ? capsule->GetEngine()->reliability : 100)); // Whole native repair is the observable physical benefit, without clearing the player's stop.
			observation.capsules.insert(observation.capsules.end(), {capsule->index.base(), grant->granted_energy_j,
				binding->consumed_tick, binding->remainder_mj, capsule->tile.base(), static_cast<uint64_t>(capsule->x_pos),
				static_cast<uint64_t>(capsule->y_pos), capsule->cur_speed, to_underlying(binding->wait)});
		}
		TickOrbitalManufacturingReservations(); TickOrbitalManufacturingWork(); TickOrbitalRecipes();
		const auto *town = GetUtilityPowerConsumer(rig.town_consumer); REQUIRE(town != nullptr);
		CHECK(town->town_bonus_percent == 25); CHECK(town->town_effect_applied);
		const auto port = GetGeothermalUtilityLink(rig.plant_port, UtilityEnergyDomain::Electricity); REQUIRE(port.has_value());
		const uint64_t expected_flow = tick == 0 ? 270 + 540 * (count - 2) + 135 : 270 + 540 + 135;
		uint64_t port_flow = 0;
		// Local cells intentionally store no duplicate flow counter. Original
		// child receipts record each newly traversed segment exactly once, so
		// summing this shared port verifies Tube and town used one cable ceiling.
		for (const auto &consumer : CaptureUtilityPowerState().consumers) {
			for (const auto &share : consumer.shares) {
				if (std::ranges::find(share.segment, port->id) != share.segment.end()) port_flow += share.allocated_j;
			}
		}
		CHECK(port_flow == expected_flow);
		const auto settled = ComputeUtilityPowerDigest(), orbital = ComputeSpaceStructureDigest();
		const auto bindings = ComputeTubePowerVehicleDigest();
		const auto work = job.work_ticks;
		const auto energy = job.work_energy_j;
		RunUtilityTownServices(); TickOrbitalManufacturingReservations(); TickOrbitalManufacturingWork(); TickOrbitalRecipes(); PrepareTubePowerTick();
		// A repeated native front callback receives no second paid action. The
		// ordinary unpaid path may maintain orders but cannot repair or move it.
		for (auto *capsule : capsules) {
			const auto tile = capsule->tile; const auto x = capsule->x_pos, y = capsule->y_pos;
			REQUIRE(capsule->Tick()); CHECK(capsule->tile == tile); CHECK(capsule->x_pos == x); CHECK(capsule->y_pos == y);
		}
		CHECK(ComputeUtilityPowerDigest() == settled); CHECK(ComputeSpaceStructureDigest() == orbital);
		CHECK(ComputeTubePowerVehicleDigest() == bindings);
		CHECK(job.work_ticks == work); CHECK(job.work_energy_j == energy);
	}
	CHECK(GetUtilityPowerProducer(output)->generated_lifetime_j - before_generation == (model == GeothermalGeneratorModel::VentTap ? 8100 : 18900));
	CHECK(GetUtilityPowerProducer(output)->consumed_lifetime_j - before_use == 270 * 2 + 540 * (count - 1) + 135 * 2);
	CHECK(GetGeothermalGeneratorState(rig.generator)->active_service_ticks == before_service + 2);
	const auto rate = GetMultiLayerGeothermalGeneratorTerms(model)->service_cost_per_day;
	const auto service_cost = (before_service + 2) * rate / 74 - before_service * rate / 74;
	CHECK(Company::Get(_current_company)->money == before_cash - Money{static_cast<int64_t>(service_cost)});
	CHECK(rig.assembly->module->manufacture->work_ticks == 1); CHECK(rig.assembly->module->manufacture->work_energy_j == 750);
	CHECK(rig.processor->module->processing->paid_lifetime_j == 120);
	CHECK(std::ranges::equal(_random.state, random.state));
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	observation.cash = Company::Get(_current_company)->money; observation.utility = ComputeUtilityPowerDigest();
	observation.generator = ComputeGeothermalGeneratorDigest(); observation.space = ComputeSpaceStructureDigest();
	return observation;
}

/** Native short grants and shared cable residuals preserve exact work, money and counters across valid binding transport permutations. */
TEST_CASE("Common scheduler settles mixed native contention in original destination order", "[utility-scheduler-mixed-contention]")
{
	MockEnvironment::Instance();
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	const auto ordered = RunMixedContention(model, false); const auto reversed = RunMixedContention(model, true);
	CHECK(ordered == reversed);
}
