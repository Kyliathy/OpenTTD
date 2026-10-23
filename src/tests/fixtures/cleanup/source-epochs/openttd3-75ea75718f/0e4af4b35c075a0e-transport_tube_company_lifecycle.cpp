/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_company_lifecycle.cpp Loaded native capsules retain recoverable custody across canonical company changes.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group14D.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor/materials and loaded stock remain boundary inputs; actual commands and native ticks earn travel and power use.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "geothermal_tube_switch_fixture.h"
#include "../transport_tube_cmd.h"
#include "../order_base.h"
#include "../rail.h"
#include "../safeguards.h"

/** @param capsule Actual original front. @return Native physical/order/packet state, excluding legitimate ownership, service-default and livery-cache changes during takeover. */
static auto TubeCompanyCustody(const Train &capsule)
{
	REQUIRE(capsule.orders != nullptr);
	return std::tuple{capsule.index, capsule.tile, capsule.x_pos, capsule.y_pos, capsule.z_pos, capsule.direction, capsule.track,
		capsule.cur_speed, capsule.subspeed, capsule.progress, capsule.orders->index, capsule.cur_real_order_index,
		capsule.current_order.GetType(), capsule.current_order.GetDestination(), capsule.cargo_type, capsule.cargo.StoredCount()};
}

/** A genuinely travelling original capsule survives native takeover, brakes without old rights and resumes through explicit fresh service with its original packet/carry. */
TEST_CASE("Tube loaded takeover keeps recoverable original capsule custody", "[tube-company-loaded-takeover]")
{
	const bool native = GENERATE(false, true);
	const bool closed = GENERATE(false, true);
	std::unique_ptr<Company> successor;
	std::unique_ptr<Company> previous_successor;
	{
		MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); ResetRailTypes(); GeothermalTubeSwitchRig rig{true};
		AutoRestoreBackup freight{_settings_game.vehicle.freight_trains, uint8_t{1}};
		const auto coal = GetCargoTypeByLabel(CT_COAL); REQUIRE(coal != INVALID_CARGO);
		CargoSpec::Get(coal)->weight = 16; CargoSpec::Get(coal)->is_freight = true;
		auto *capsule = rig.Buy(); const auto packet = rig.Load(*capsule);
		const auto consumer = native ? BindTubeGeothermalPowerVehicle(capsule->index, rig.geothermal.source) : BindTubePowerVehicle(capsule->index, rig.external);
		REQUIRE(consumer.has_value()); rig.Start(*capsule, rig.inlets[1]);
		uint ticks = 0;
		// Only real normal movement can earn this on-guideway takeover point;
		// no physical relocation, direction edit or fake consumed work is used.
		while (capsule->IsInDepot() && ticks++ < 1000) rig.Run(std::array{capsule});
		REQUIRE(ticks < 1000); REQUIRE_FALSE(capsule->IsInDepot());
		if (closed) {
			if (native) REQUIRE(Command<Commands::CloseTubeGeothermalPower>::Do(DoCommandFlag::Execute, rig.geothermal.source).Succeeded());
			else REQUIRE(Command<Commands::CloseTubePower>::Do(DoCommandFlag::Execute, rig.external).Succeeded());
			// Real later intervals expire the old current shares. A powerless
			// live front must still couple its original source/billing history
			// during ownership changes after those transient grants disappear.
			for (uint wait = 0; wait < 8; ++wait) rig.Run(std::array{capsule});
		}
		const auto original = *GetTubePowerVehicle(capsule->index);
		const auto physical = TubeCompanyCustody(*capsule);
		const auto consumed = GetUtilityPowerProducer(original.source)->consumed_lifetime_j;
		REQUIRE(consumed > 0);
		const auto external = *GetTubePowerConnection(rig.external);
		REQUIRE(Company::CanAllocateItem()); successor.reset(Company::Create());
		successor->money = 1000000; successor->clear_limit = 64U << 16;
		AutoRestoreBackup local{_local_company, CompanyID::Invalid()};
		ChangeOwnershipOfCompanyItems(_current_company, successor->index);
		REQUIRE(capsule->owner == successor->index); CHECK(TubeCompanyCustody(*capsule) == physical);
		REQUIRE(capsule->cargo.Packets()->size() == 1); CHECK(capsule->cargo.Packets()->front()->index == packet);
		CHECK_FALSE(GetTubePowerConnection(rig.external)->active); CHECK_FALSE(GetTubeGeothermalSource(rig.geothermal.source)->active);
		CHECK(GetUtilityPowerProducer(original.source)->consumed_lifetime_j == consumed);
		CHECK(GetUtilityPowerProducer(original.source)->owner == successor->index);
		// The original physical association must remain as a powerless recovery
		// handle for this same surviving front, while every old source permission
		// retires. Requiring a depot-only fresh bind would strand it on the track.
		const auto *waiting = GetTubePowerVehicle(capsule->index); REQUIRE(waiting != nullptr);
		CHECK(waiting->consumer == original.consumer); CHECK(waiting->remainder_mj == original.remainder_mj);
		CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
		AutoRestoreBackup actor{_current_company, successor->index};
		const auto x = capsule->x_pos, y = capsule->y_pos;
		rig.Run(std::array{capsule}); CHECK(capsule->x_pos == x); CHECK(capsule->y_pos == y); CHECK(capsule->cur_speed == 0);
		CHECK_FALSE(capsule->vehstatus.Test(VehState::Stopped));
		CHECK(GetUtilityPowerProducer(original.source)->consumed_lifetime_j == consumed);
		// A second canonical buyer receives the same powerless front after its
		// prior source is already retired. No old allocation can anchor this
		// history now; the actual live association must retain the lineage.
		REQUIRE(Company::CanAllocateItem()); std::unique_ptr<Company> next{Company::Create()};
		next->money = 1000000; next->clear_limit = 64U << 16;
		const auto held = TubeCompanyCustody(*capsule);
		ChangeOwnershipOfCompanyItems(successor->index, next->index);
		previous_successor = std::move(successor); successor = std::move(next);
		AutoRestoreBackup second_actor{_current_company, successor->index};
		CHECK(capsule->owner == successor->index); CHECK(TubeCompanyCustody(*capsule) == held);
		CHECK(GetUtilityPowerProducer(original.source)->owner == successor->index);
		CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded());
		if (native) {
			const auto replacement = rig.Connect(); REQUIRE(replacement.source != rig.geothermal.source);
			REQUIRE(Command<Commands::SetTubeVehicleGeothermalPower>::Do(DoCommandFlag::Execute, capsule->index, replacement.source, true).Succeeded());
		} else {
			const auto replacement = ConnectTubeExternalPower(external.portal, external.interchange, external.utility_path);
			REQUIRE(replacement.error == TubePowerError::None); REQUIRE(replacement.producer != rig.external);
			REQUIRE(Command<Commands::SetTubeVehiclePower>::Do(DoCommandFlag::Execute, capsule->index, replacement.producer, true).Succeeded());
		}
		const auto *rebound = GetTubePowerVehicle(capsule->index); REQUIRE(rebound != nullptr);
		CHECK(rebound->consumer != original.consumer); CHECK(rebound->remainder_mj == original.remainder_mj);
		const auto fresh_source = rebound->source;
		const auto fresh_used = GetUtilityPowerProducer(rebound->source)->consumed_lifetime_j;
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded());
		ticks = 0;
		// Fresh original-owner admission must earn the remaining real trip with
		// the same orders and packet; no old grant can be reused after takeover.
		while (!capsule->IsInDepot() && ticks++ < 1000) rig.Run(std::array{capsule});
		REQUIRE(ticks < 1000); REQUIRE(capsule->tile == rig.inlets[1]); REQUIRE(capsule->vehstatus.Test(VehState::Stopped));
		CHECK(capsule->cargo.StoredCount() == 17); CHECK(capsule->cargo.Packets()->front()->index == packet);
		CHECK(GetUtilityPowerProducer(fresh_source)->consumed_lifetime_j > fresh_used);
		CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
		CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	} // The transferred native front and facilities retire while the successor still exists.
}

/** Canonical terminal liquidation destroys its actual vehicle/packet, preserves spent history and cannot revive old permissions when company/vehicle pool IDs are reused. */
TEST_CASE("Tube terminal company removal preserves history without inherited rights", "[tube-company-terminal-removal]")
{
	const bool native = GENERATE(false, true);
	std::unique_ptr<Company> successor;
	{
		MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); ResetRailTypes(); GeothermalTubeSwitchRig rig{true};
		auto *capsule = rig.Buy(); const auto packet = rig.Load(*capsule); const auto vehicle = capsule->index;
		const auto initial = native ? BindTubeGeothermalPowerVehicle(vehicle, rig.geothermal.source) : BindTubePowerVehicle(vehicle, rig.external);
		REQUIRE(initial.has_value()); capsule->reliability = 100; rig.Run(std::array{capsule});
		REQUIRE(Company::CanAllocateItem()); successor.reset(Company::Create());
		successor->money = 1000000; successor->clear_limit = 64U << 16;
		const auto company = successor->index; AutoRestoreBackup local{_local_company, CompanyID::Invalid()};
		ChangeOwnershipOfCompanyItems(_current_company, company);
		auto retired_native = rig.geothermal.source; auto retired_external = rig.external;
		UtilityConsumerID retired_consumer{UINT32_MAX}; UtilityProducerID retired_output{UINT32_MAX};
		uint64_t generated = 0, consumed = 0;
		{
			AutoRestoreBackup actor{_current_company, company};
			// Surviving ownership first obtains a real fresh source. Liquidation
			// then exercises live selected permissions, actual paid service history
			// and an unused next repair grant, rather than only an idle tombstone.
			if (native) {
				const auto fresh = rig.Connect(); retired_native = fresh.source;
				REQUIRE(Command<Commands::SetTubeVehicleGeothermalPower>::Do(DoCommandFlag::Execute, vehicle, fresh.source, true).Succeeded());
			} else {
				const auto *old = GetTubePowerConnection(rig.external); REQUIRE(old != nullptr);
				const auto fresh = ConnectTubeExternalPower(old->portal, old->interchange, old->utility_path);
				REQUIRE(fresh.error == TubePowerError::None); retired_external = fresh.producer;
				REQUIRE(Command<Commands::SetTubeVehiclePower>::Do(DoCommandFlag::Execute, vehicle, fresh.producer, true).Succeeded());
			}
			capsule->reliability = 100; rig.Run(std::array{capsule});
			const auto original = *GetTubePowerVehicle(vehicle); retired_consumer = original.consumer; retired_output = original.source;
			REQUIRE(GetUtilityPowerProducer(retired_output)->consumed_lifetime_j > 0);
			capsule->reliability = 100; ++TimerGameTick::counter; PrepareTubePowerTick();
			REQUIRE(GetUtilityAllocation(retired_consumer).has_value()); REQUIRE(GetUtilityAllocation(retired_consumer)->granted_energy_j > 0);
			generated = GetUtilityPowerProducer(retired_output)->generated_lifetime_j;
			consumed = GetUtilityPowerProducer(retired_output)->consumed_lifetime_j;
			ChangeOwnershipOfCompanyItems(company, INVALID_OWNER);
		}
		// Native liquidation owns terminal packet destruction. No duplicate or
		// ownerless recovery stock is created, and spent electrical history is
		// retained instead of being refunded when physical property disappears.
		CHECK(Train::GetIfValid(vehicle) == nullptr); CHECK_FALSE(CargoPacket::IsValidID(packet)); CHECK(GetTubePowerVehicle(vehicle) == nullptr);
		REQUIRE(GetUtilityPowerConsumer(retired_consumer) != nullptr); CHECK_FALSE(GetUtilityPowerConsumer(retired_consumer)->active);
		const auto grant = GetUtilityAllocation(retired_consumer);
		if (grant.has_value()) CHECK(grant->granted_energy_j == 0);
		CHECK_FALSE(GetUtilityPowerProducer(retired_output)->active);
		CHECK(GetUtilityPowerProducer(retired_output)->generated_lifetime_j == generated);
		CHECK(GetUtilityPowerProducer(retired_output)->consumed_lifetime_j == consumed);
		CHECK_FALSE(GetTubeGeothermalSource(retired_native)->active); CHECK_FALSE(GetTubePowerConnection(retired_external)->active);
		successor.reset(); REQUIRE(Company::CanAllocateItem()); successor.reset(Company::Create());
		REQUIRE(successor->index == company); successor->money = 1000000; successor->clear_limit = 64U << 16;
		AutoRestoreBackup actor{_current_company, successor->index};
		// Native company and vehicle pools may reuse their numeric IDs. Explicit
		// ordinary purchase permissions and a new paid depot are fixture inputs;
		// neither can reactivate the former company's original source contracts.
		successor->avail_railtypes.Set(GetTubeRailType()); Engine::Get(GetTubeEngine(9))->company_avail.Set(successor->index);
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, rig.inlets[0], GetTubeRailType(), DiagDirection::SE).Succeeded());
		auto *replacement = rig.Buy(); REQUIRE(replacement->index == vehicle); CHECK(GetTubePowerVehicle(replacement->index) == nullptr);
		const auto ledger = ComputeUtilityPowerDigest();
		CHECK_FALSE(BindTubePowerVehicle(replacement->index, retired_external, false).has_value());
		CHECK_FALSE(BindTubeGeothermalPowerVehicle(replacement->index, retired_native, false).has_value());
		CHECK(ComputeUtilityPowerDigest() == ledger);
		CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
		CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	}
}
