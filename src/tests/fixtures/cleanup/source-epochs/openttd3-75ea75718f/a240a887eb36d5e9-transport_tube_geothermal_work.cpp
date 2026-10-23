/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal_work.cpp Actual capsule motion, service, original-source lineage and aggregate purchased-cable limits.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BCD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_tube_work_fixture.h"
#include "../safeguards.h"

/** Both real locomotive substeps traverse the bought guideway while original packets, fractions and native operating invoices remain conserved. */
TEST_CASE("Native geothermal output powers ordered loaded capsules exactly once", "[tube-geothermal-work-motion]")
{
	MockEnvironment::Instance(); GeothermalTubeWorkRig rig; rig.JoinDepots(); auto *capsule = rig.Buy();
	const auto packet = rig.Load(*capsule); const auto source = rig.Connect();
	const auto consumer = BindTubeGeothermalPowerVehicle(capsule->index, source.source); REQUIRE(consumer.has_value());
	const auto acceleration = GENERATE(AccelerationModel::Original, AccelerationModel::Realistic);
	AutoRestoreBackup model(_settings_game.vehicle.train_acceleration_model, acceleration);
	rig.Start(*capsule, rig.inlets[1]); const auto cash = Company::Get(_current_company)->money;
	const auto service = GetGeothermalGeneratorState(rig.generator)->active_service_ticks;
	const auto service_rate = GetMultiLayerGeothermalGeneratorTerms(GeothermalGeneratorModel::MagmaLoop)->service_cost_per_day;
	const auto producers = CaptureUtilityPowerState().next_producer;
	uint64_t expected = 0; uint16_t carry = 0; uint ticks = 0;
	// Follow actual arrival rather than a synthetic position update. Only a
	// successfully admitted native opportunity advances expected usage/carry.
	while (!capsule->IsStoppedInDepot() && ticks < 3000) {
		const auto demand = ComputeTubePowerDemand(*capsule, GetTubePowerPhase(*capsule), 1, carry); REQUIRE(demand.has_value());
		rig.Run(std::array{capsule}); ++ticks;
		const auto binding = *GetTubePowerVehicle(capsule->index);
		if (binding.consumed_tick == TimerGameTick::counter) {
			expected += demand->energy_j; carry = demand->remainder_mj;
			const auto *recipient = GetUtilityPowerConsumer(*consumer); REQUIRE(recipient->shares.size() == 1);
			CHECK(recipient->shares.front().producer == rig.output); CHECK(recipient->shares.front().imported_link == binding.local_feed);
			CHECK(recipient->shares.front().consumed_j == demand->energy_j);
		}
		CHECK(binding.remainder_mj == carry);
	}
	INFO(DescribeTrain(capsule));
	INFO("wait=" << to_underlying(GetTubePowerVehicle(capsule->index)->wait) << " phase=" << to_underlying(GetTubePowerPhase(*capsule)) <<
		" prepared=" << GetTubePowerVehicle(capsule->index)->prepared_tick << " consumed=" << GetTubePowerVehicle(capsule->index)->consumed_tick <<
		" tick=" << TimerGameTick::counter << " in_depot=" << capsule->IsInDepot() << " stopped=" << capsule->vehstatus.Test(VehState::Stopped) <<
		" destination=" << rig.inlets[1].base() << " connected=" << IsTubeGeothermalPowerConnected(source.source, *capsule));
	REQUIRE(ticks < 3000); CHECK(ticks > 1); CHECK(capsule->tile == rig.inlets[1]); CHECK(expected > 0);
	const auto digest = ComputeUtilityPowerDigest(), physical = ComputeTubePowerVehicleDigest();
	PrepareTubePowerTick(); REQUIRE(capsule->Tick());
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(ComputeTubePowerVehicleDigest() == physical);
	CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == expected); CHECK(CaptureUtilityPowerState().next_producer == producers);
	CHECK(CaptureTubePowerConnections().empty()); CHECK(CargoPacket::IsValidID(packet)); CHECK(capsule->cargo.StoredCount() == 17);
	CHECK(Company::Get(_current_company)->money == cash - Money{static_cast<int64_t>(GetGeothermalGeneratorState(rig.generator)->active_service_ticks * service_rate / 74 - service * service_rate / 74)});
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
}

/** Many actual depot jobs share the same purchased cable regardless of one or two original inlets; shortages keep complete work and manual stops intact. */
TEST_CASE("Native geothermal capsule service shares one physical cable allowance", "[tube-geothermal-work-aggregate]")
{
	MockEnvironment::Instance(); GeothermalTubeWorkRig rig; const bool split = GENERATE(false, true);
	Company::Get(_current_company)->money = Money{50000000}; // Explicit fleet-capital fixture input; all twelve native purchases still pay their real invoices.
	const auto first = rig.Connect(); const auto second = split ? rig.Connect(1) : first;
	std::vector<Train *> capsules;
	// Twelve genuine damaged native capsules demand6480J. The shared200kW
	// purchased trunk admits5400J per27ms, even across distinct source roots.
	for (uint index = 0; index < 12; ++index) {
		const bool branch = split && index >= 6;
		auto *capsule = rig.Buy(branch ? 1 : 0); REQUIRE(BindTubeGeothermalPowerVehicle(capsule->index, branch ? second.source : first.source).has_value());
		capsule->reliability = 100; VehicleServiceInDepot(capsule); REQUIRE(capsule->reliability == 100); capsules.push_back(capsule);
	}
	const auto producers = CaptureUtilityPowerState().next_producer; rig.Run(capsules);
	uint serviced = 0; uint64_t imported = 0;
	// The first ten whole native jobs consume the actual shared slices; the
	// last two retain their damage, original binding and unadvanced demand carry.
	for (auto *capsule : capsules) {
		const auto binding = *GetTubePowerVehicle(capsule->index); const bool admitted = binding.consumed_tick == TimerGameTick::counter;
		CHECK(capsule->vehstatus.Test(VehState::Stopped)); CHECK(capsule->cur_speed == 0); CHECK(binding.remainder_mj == 0);
		CHECK(capsule->reliability == (admitted ? capsule->GetEngine()->reliability : 100));
		CHECK(binding.wait == (admitted ? TubePowerWait::None : TubePowerWait::Grant));
		imported += GetUtilityPowerLink(binding.local_feed)->flow_j; serviced += admitted;
	}
	CHECK(serviced == 10); CHECK(imported == 5400); CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == 5400);
	CHECK(GetUtilityPowerConsumer(first.supply)->meter.demand_energy_j == (split ? 3240 : 6480));
	CHECK(CaptureUtilityPowerState().next_producer == producers); CHECK(CaptureTubePowerConnections().empty());
	rig.Run(capsules); CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == 6480);
	for (const auto *capsule : capsules) CHECK(capsule->reliability == capsule->GetEngine()->reliability); // Only the two previously refused jobs need a second service opportunity.
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}
