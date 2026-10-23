/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal_switch.cpp Actual native external/geothermal switching, conserved billing/cargo/fractions and unchanged admission refusals.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BC task6BC.2.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Connected portal geometry is an explicit boundary input; source service, plant and capsules use their actual owners.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_tube_switch_fixture.h"
#include "../safeguards.h"

/** Round-trip switching releases only unused old grants; actual native movement/service retain packets, original tariff obligations and one active demand fraction. */
TEST_CASE("Native capsules switch external and geothermal sources without renewing history", "[tube-geothermal-switch]")
{
	MockEnvironment::Instance(); GeothermalTubeSwitchRig rig; auto *capsule = rig.Buy(); const auto packet = rig.Load(*capsule);
	const bool initially_native = GENERATE(false, true);
	const auto model = GENERATE(AccelerationModel::Original, AccelerationModel::Realistic);
	AutoRestoreBackup acceleration(_settings_game.vehicle.train_acceleration_model, model);
	const auto bound = initially_native ? BindTubeGeothermalPowerVehicle(capsule->index, rig.geothermal.source) : BindTubePowerVehicle(capsule->index, rig.external);
	REQUIRE(bound.has_value()); rig.Start(*capsule, rig.inlets[1]); rig.StopOnRoute(*capsule);
	const auto old = *GetTubePowerVehicle(capsule->index); REQUIRE(old.remainder_mj != 0);
	// Prepare a positive unused original offer at the held physical location.
	// Stopping again changes only intent; switching must expire this exact offer.
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded());
	++TimerGameTick::counter; PrepareTubePowerTick();
	REQUIRE(GetUtilityAllocation(old.consumer)->granted_energy_j > 0);
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded());
	const auto ledger = ComputeUtilityPowerDigest(), bindings = ComputeTubePowerVehicleDigest(), native = ComputeGeothermalGeneratorDigest();
	const auto invoice = ComputeTubePowerDigest(); const auto cash = Company::Get(_current_company)->money;
	const auto geo_used = GetUtilityPowerProducer(rig.output)->consumed_lifetime_j, external_used = GetUtilityPowerProducer(rig.external)->consumed_lifetime_j;
	const auto quote = rig.Switch(*capsule, !initially_native, false); REQUIRE(quote.has_value());
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerVehicleDigest() == bindings); CHECK(Company::Get(_current_company)->money == cash);
	REQUIRE(rig.Switch(*capsule, !initially_native) == quote);
	const auto current = *GetTubePowerVehicle(capsule->index); CHECK(current.consumer != old.consumer); CHECK(current.remainder_mj == old.remainder_mj);
	CHECK(current.prepared_tick == UINT64_MAX); CHECK(current.consumed_tick == UINT64_MAX);
	const auto expired = GetUtilityAllocation(old.consumer);
	CHECK((!expired.has_value() || expired->granted_energy_j == 0)); // Retired local recipients expose no allocation; retained external records expose no usable original slice.
	CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == geo_used); CHECK(GetUtilityPowerProducer(rig.external)->consumed_lifetime_j == external_used);
	CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(ComputeTubePowerDigest() == invoice); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(capsule->vehstatus.Test(VehState::Stopped)); CHECK(capsule->cargo.Packets()->front()->index == packet);
	const auto rows = CaptureTubePowerVehicles(); REQUIRE(rows.size() == 2); CHECK_FALSE(rows.front().active); CHECK(rows.front().remainder_mj == 0);
	if (initially_native) CHECK_FALSE(GetUtilityPowerLink(old.local_feed)->active);
	const auto untouched = ComputeUtilityPowerDigest(); PrepareTubePowerTick(); REQUIRE(capsule->Tick()); CHECK(ComputeUtilityPowerDigest() == untouched);
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded());
	uint ticks = 0;
	// The replacement source must perform actual native travel to the existing
	// halt depot; a changed binding alone is not successful source integration.
	while (!capsule->IsStoppedInDepot() && ticks++ < 1000) rig.Run(std::array{capsule});
	REQUIRE(ticks < 1000); CHECK(capsule->tile == rig.inlets[1]); CHECK(capsule->cargo.Packets()->front()->index == packet);
	const auto carry = GetTubePowerVehicle(capsule->index)->remainder_mj;
	REQUIRE(rig.Switch(*capsule, initially_native).has_value()); CHECK(GetTubePowerVehicle(capsule->index)->remainder_mj == carry);
	capsule->reliability = 100; rig.Run(std::array{capsule}); CHECK(capsule->reliability == capsule->GetEngine()->reliability);
	CHECK(GetTubePowerVehicle(capsule->index)->remainder_mj == carry); CHECK(capsule->vehstatus.Test(VehState::Stopped));
	const auto old_invoice = *GetTubePowerConnection(rig.external); const auto closing_cash = Company::Get(_current_company)->money;
	REQUIRE(CloseTubeExternalPower(rig.external) == TubePowerError::None);
	CHECK(GetTubePowerConnection(rig.external)->invoiced_money == old_invoice.invoiced_money + (old_invoice.usage_remainder_j != 0));
	CHECK(Company::Get(_current_company)->money == closing_cash - Money{old_invoice.usage_remainder_j != 0 ? 1 : 0});
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Different original inlets may legitimately use the same real generator; their nonreused source/feed identities remain separate on recovery. */
TEST_CASE("Geothermal switching distinguishes original inlets sharing one output", "[tube-geothermal-switch-inlet]")
{
	MockEnvironment::Instance(); GeothermalTubeWorkRig rig; rig.JoinDepots(); auto *capsule = rig.Buy(); const auto first = rig.Connect(), second = rig.Connect(1);
	REQUIRE(BindTubeGeothermalPowerVehicle(capsule->index, first.source).has_value()); const auto old = *GetTubePowerVehicle(capsule->index);
	REQUIRE(CloseTubeGeothermalPower(first.source) == TubePowerError::None);
	const auto next = ReconnectTubeGeothermalPowerVehicle(capsule->index, second.source); REQUIRE(next.has_value());
	const auto current = *GetTubePowerVehicle(capsule->index); CHECK(current.source == old.source); CHECK(current.geothermal_source != old.geothermal_source);
	CHECK(current.local_feed != old.local_feed); CHECK(current.consumer != old.consumer); CHECK_FALSE(GetUtilityPowerLink(old.local_feed)->active);
	capsule->reliability = 100; rig.Run(std::array{capsule}); CHECK(capsule->reliability == capsule->GetEngine()->reliability);
	CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == 540); CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
}

/** Foreign, duplicate, moving, unavailable or sequence-exhausted choices refuse before publishing either recipient identity or erasing existing obligations. */
TEST_CASE("Geothermal switching admission refuses atomically", "[tube-geothermal-switch-refusal]")
{
	MockEnvironment::Instance(); GeothermalTubeSwitchRig rig; auto *capsule = rig.Buy(); REQUIRE(BindTubePowerVehicle(capsule->index, rig.external).has_value());
	auto source = rig.geothermal.source;
	AutoRestoreBackup actor(_current_company); AutoRestoreBackup speed(capsule->cur_speed);
	SECTION("unknown original inlet") { source = TubeGeothermalSourceID{999}; }
	SECTION("foreign caller") { _current_company = OWNER_DEITY; }
	SECTION("moving capsule") { capsule->cur_speed = 1; }
	SECTION("closed original inlet") { REQUIRE(CloseTubeGeothermalPower(source) == TubePowerError::None); }
	SECTION("crashed actual capsule") { capsule->vehstatus.Set(VehState::Crashed); }
	SECTION("same original provider") { REQUIRE(rig.Switch(*capsule, true).has_value()); }
	SECTION("no receipt remains for releasing old unused grant") {
		rig.Start(*capsule, rig.inlets[1]); ++TimerGameTick::counter; PrepareTubePowerTick();
		REQUIRE(GetUtilityAllocation(GetTubePowerVehicle(capsule->index)->consumer)->granted_energy_j > 0);
		auto state = CaptureUtilityPowerState(); state.operation_sequence = UINT64_MAX - 2;
		REQUIRE(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
		StageLoadedUtilityPowerState(std::move(state)); REQUIRE(AcceptLoadedUtilityPowerState());
	}
	const auto ledger = ComputeUtilityPowerDigest(), binding = ComputeTubePowerVehicleDigest(), invoice = ComputeTubePowerDigest();
	const auto cash = Company::Get(GetTubeGeothermalSource(rig.geothermal.source)->owner)->money;
	CHECK_FALSE(ReconnectTubeGeothermalPowerVehicle(capsule->index, source, false).has_value());
	CHECK_FALSE(ReconnectTubeGeothermalPowerVehicle(capsule->index, source).has_value());
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerVehicleDigest() == binding); CHECK(ComputeTubePowerDigest() == invoice);
	CHECK(Company::Get(GetTubeGeothermalSource(rig.geothermal.source)->owner)->money == cash);
}
