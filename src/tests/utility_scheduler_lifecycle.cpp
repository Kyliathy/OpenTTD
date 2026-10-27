/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */
/**
 * @file utility_scheduler_lifecycle.cpp Integrated original provider loss, repaired paths, scoped rights and canonical transport.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md.
 * The mixed fixture discloses its geometry/material/frame inputs. Native
 * commands, original accounts and actual consumers retain all runtime authority.
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 10 (cases own their language pack)
 */
#include "utility_scheduler_mixed_fixture.h"
#include "layer_window_fixture.h"
#include "transport_tube_power_fixture.h"
#include "../economy_func.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** Original accepted row order is canonical while every saved fraction and physical provider reference remains observable. */
TEST_CASE("Native provider binding captures and hashes ignore transport row order", "[utility-scheduler-provider-canonical]")
{
	// Own the language pack before the rig names stations (Station::UpdateVirtCoord formats real strings).
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); UtilitySchedulerMixedRig rig{GeothermalGeneratorModel::MagmaLoop};
	auto *first = rig.Buy(), *second = rig.Buy(true); rig.Opportunity(std::array{first, second});
	const auto original = CaptureTubePowerVehicles(); const auto digest = ComputeTubePowerVehicleDigest();
	const auto utility = ComputeUtilityPowerDigest(), source = ComputeTubeGeothermalPowerDigest(), generator = ComputeGeothermalGeneratorDigest();
	auto reversed = original; std::ranges::reverse(reversed);
	StageLoadedTubePowerVehicles(std::move(reversed), GetTubePowerPreparedTick()); REQUIRE(AcceptLoadedTubePowerVehicles());
	CHECK(ComputeTubePowerVehicleDigest() == digest);
	const auto canonical = CaptureTubePowerVehicles(); REQUIRE(canonical.size() == 2);
	CHECK(canonical[0].consumer < canonical[1].consumer);
	auto changed = canonical; changed[0].remainder_mj = 553;
	StageLoadedTubePowerVehicles(std::move(changed), GetTubePowerPreparedTick()); REQUIRE(AcceptLoadedTubePowerVehicles());
	CHECK(ComputeTubePowerVehicleDigest() != digest);
	StageLoadedTubePowerVehicles(original, GetTubePowerPreparedTick()); REQUIRE(AcceptLoadedTubePowerVehicles());
	CHECK(ComputeTubePowerVehicleDigest() == digest); CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(ComputeTubeGeothermalPowerDigest() == source); CHECK(ComputeGeothermalGeneratorDigest() == generator);
}

/** Reusing actual commissioned hardware after closing a paid lease creates a fresh source; row permutation cannot reorder retained invoices. */
TEST_CASE("External provider captures preserve canonical paid source identity", "[utility-scheduler-external-canonical]")
{
	TubePowerRig rig; rig.Connect(); const auto old = rig.source;
	REQUIRE(CloseTubeExternalPower(old) == TubePowerError::None); rig.Connect(); REQUIRE(rig.source != old);
	const auto original = CaptureTubePowerConnections(); REQUIRE(original.size() == 2);
	const auto digest = ComputeTubePowerDigest(), utility = ComputeUtilityPowerDigest();
	auto reversed = original; std::ranges::reverse(reversed);
	StageLoadedTubePowerConnections(std::move(reversed)); REQUIRE(AcceptLoadedTubePowerConnections());
	CHECK(ComputeTubePowerDigest() == digest); CHECK(ComputeUtilityPowerDigest() == utility);
	const auto canonical = CaptureTubePowerConnections(); CHECK(canonical[0].producer < canonical[1].producer);
	CHECK_FALSE(canonical[0].active); CHECK(canonical[1].active);
}

/** Removing one bought branch releases its unused descendants and stops only its capsules; repair uses new cell identity with original provider/carry. */
TEST_CASE("Native cable loss and repair preserve unrelated town and orbital work", "[utility-scheduler-path-reconnect]")
{
	// Own the language pack before the rig names stations (Station::UpdateVirtCoord formats real strings).
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); UtilitySchedulerMixedRig rig{GeothermalGeneratorModel::MagmaLoop};
	auto *capsule = rig.Buy(); rig.Opportunity(std::array{capsule});
	const auto binding = *GetTubePowerVehicle(capsule->index); const auto output = GetTubeGeothermalSource(rig.source)->producer;
	const auto used = GetUtilityPowerProducer(output)->consumed_lifetime_j;
	const auto branch = TileXYL(33, 24, MapLayer::DeepUnderground);
	const auto old_cell = GetGeothermalUtilityLink(branch, UtilityEnergyDomain::Electricity); REQUIRE(old_cell.has_value());
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, branch,
		std::make_tuple(branch, UtilityEnergyDomain::Electricity)).Succeeded());
	CHECK(GetUtilityPowerProducer(output)->consumed_lifetime_j == used); CHECK(GetTubeGeothermalSource(rig.source)->active);
	const auto tile = capsule->tile; const auto x = capsule->x_pos, y = capsule->y_pos;
	rig.Opportunity(std::array{capsule});
	const auto waiting = *GetTubePowerVehicle(capsule->index);
	CHECK(waiting.consumer == binding.consumer); CHECK(waiting.local_feed == binding.local_feed);
	CHECK(waiting.wait == TubePowerWait::Topology); CHECK(waiting.remainder_mj == binding.remainder_mj);
	CHECK(waiting.consumed_tick == binding.consumed_tick); CHECK_FALSE(capsule->vehstatus.Test(VehState::Stopped));
	CHECK(capsule->tile == tile); CHECK(capsule->x_pos == x); CHECK(capsule->y_pos == y);
	CHECK(GetUtilityPowerConsumer(rig.town_consumer)->town_bonus_percent == 25);
	CHECK(rig.assembly->module->manufacture->work_ticks == 1); CHECK(rig.processor->module->processing->paid_lifetime_j == 120);
	rig.Build(branch, UtilityEnergyDomain::Electricity);
	const auto fresh = GetGeothermalUtilityLink(branch, UtilityEnergyDomain::Electricity); REQUIRE(fresh.has_value()); CHECK(fresh->id != old_cell->id);
	rig.Opportunity(std::array{capsule}); const auto resumed = *GetTubePowerVehicle(capsule->index);
	CHECK(resumed.consumer == binding.consumer); CHECK(resumed.local_feed == binding.local_feed); CHECK(resumed.geothermal_source == binding.geothermal_source);
	CHECK(resumed.wait == TubePowerWait::None); CHECK(resumed.consumed_tick == TimerGameTick::counter);
	CHECK_FALSE(capsule->vehstatus.Test(VehState::Stopped));
	CHECK(GetUtilityPowerProducer(output)->consumed_lifetime_j == used + 135 + 270 + 135);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** A real unrelated distribution-pair seal cannot erase a capsule's already prepared original native grant. */
TEST_CASE("Unrelated portal loss preserves an imported native capsule reservation", "[utility-scheduler-scoped-portal-loss]")
{
	// Own the language pack before the rig names stations (Station::UpdateVirtCoord formats real strings).
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); UtilitySchedulerMixedRig rig{GeothermalGeneratorModel::MagmaLoop}; auto *capsule = rig.Buy();
	++TimerGameTick::counter; PrepareTubePowerTick();
	const auto binding = *GetTubePowerVehicle(capsule->index); const auto source = *GetTubeGeothermalSource(rig.source);
	REQUIRE(GetUtilityAllocation(binding.consumer)->granted_energy_j == 270);
	const auto used = GetUtilityPowerProducer(source.producer)->consumed_lifetime_j;
	const auto portal = GetGeothermalRiserState(rig.service.first_riser)->site.portal;
	REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, portal, false).Succeeded());
	CHECK(GetUtilityPowerProducer(source.producer)->consumed_lifetime_j == used);
	CHECK(GetUtilityAllocation(binding.consumer)->granted_energy_j == 270);
	REQUIRE(capsule->Tick()); CHECK(GetTubePowerVehicle(capsule->index)->consumed_tick == TimerGameTick::counter);
	CHECK(GetUtilityPowerProducer(source.producer)->consumed_lifetime_j == used + 270);
	CHECK_FALSE(capsule->vehstatus.Test(VehState::Stopped));
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Actual portal sealing or generator demolition affects its own physical descendants while independent native Space processing continues. */
TEST_CASE("Integrated source and riser loss preserve original histories and independent work", "[utility-scheduler-source-loss]")
{
	// Own the language pack before the rig names stations (Station::UpdateVirtCoord formats real strings).
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); UtilitySchedulerMixedRig rig{GeothermalGeneratorModel::MagmaLoop};
	auto *capsule = rig.Buy(); rig.Opportunity(std::array{capsule});
	const auto native = *GetGeothermalGeneratorState(rig.generator); const auto original = *GetTubeGeothermalSource(rig.source);
	const auto used = GetUtilityPowerProducer(native.electrical_producer)->consumed_lifetime_j;
	bool generator_lost = false;
	SECTION("seal actual distribution pair") {
		const auto portal = GetGeothermalRiserState(rig.service.first_riser)->site.portal;
		REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, portal, false).Succeeded());
		CHECK(GetTubeGeothermalSource(rig.source)->active);
	}
	SECTION("remove actual generator") {
		generator_lost = true;
		REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, rig.plant_port,
			std::make_tuple(rig.plant_port)).Succeeded());
		CHECK_FALSE(GetTubeGeothermalSource(rig.source)->active); CHECK_FALSE(GetUtilityPowerConsumer(original.supply)->active);
		CHECK_FALSE(GetUtilityPowerProducer(native.electrical_producer)->active);
	}
	CHECK(GetUtilityPowerProducer(native.electrical_producer)->consumed_lifetime_j == used);
	rig.Opportunity(std::array{capsule});
	CHECK(GetUtilityPowerConsumer(rig.town_consumer)->town_bonus_percent == 0);
	CHECK_FALSE(GetUtilityPowerConsumer(rig.town_consumer)->town_effect_applied);
	CHECK(rig.assembly->module->manufacture->work_ticks == 1); CHECK(rig.processor->module->processing->paid_lifetime_j == 120);
	CHECK(GetUtilityPowerProducer(native.electrical_producer)->consumed_lifetime_j == used + (generator_lost ? 0 : 270));
	CHECK_FALSE(capsule->vehstatus.Test(VehState::Stopped));
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Renewable native sources offer one bounded interval; exhausting the actual panel cannot buy another work opportunity before rotation. */
TEST_CASE("Original interval depletion never republishes energy during consumer retries", "[utility-scheduler-interval-depletion]")
{
	// Own the language pack before the rig names stations (Station::UpdateVirtCoord formats real strings).
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); UtilitySchedulerMixedRig rig{GeothermalGeneratorModel::VentTap};
	rig.Opportunity(); const auto panel = rig.core->module->producer;
	const auto before = GetUtilityPowerProducer(panel)->consumed_lifetime_j; rig.Opportunity();
	CHECK(GetUtilityPowerProducer(panel)->consumed_lifetime_j == before + 810);
	const auto digest = ComputeUtilityPowerDigest(), physical = ComputeSpaceStructureDigest();
	const auto generation = GetUtilityPowerProducer(panel)->generated_lifetime_j;
	// Both original consumers have used every offered panel joule. Repeating
	// their callbacks or requesting preparation at this same native tick cannot
	// turn an exhausted interval into another renewable-source publication.
	for (uint retry = 0; retry < 3; ++retry) {
		PrepareTubePowerTick(); TickOrbitalManufacturingWork(); TickOrbitalRecipes();
		CHECK(ComputeUtilityPowerDigest() == digest); CHECK(ComputeSpaceStructureDigest() == physical);
		CHECK(GetUtilityPowerProducer(panel)->generated_lifetime_j == generation);
	}
	rig.Opportunity(); CHECK(GetUtilityPowerProducer(panel)->generated_lifetime_j == generation + 810);
	CHECK(GetUtilityPowerProducer(panel)->consumed_lifetime_j == before + 1620);
}

/** Actual takeover and later company-slot reuse retain original generator/Space history while retiring every old Tube incarnation. */
TEST_CASE("Integrated company transitions cannot revive old native provider rights", "[utility-scheduler-company-reuse]")
{
	// Own the language pack before the rig names stations (Station::UpdateVirtCoord formats real strings).
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); UtilitySchedulerMixedRig rig{GeothermalGeneratorModel::MagmaLoop};
	auto *capsule = rig.Buy(true); rig.Opportunity(std::array{capsule});
	const auto original = *GetTubeGeothermalSource(rig.source); const auto old_owner = _current_company;
	const auto used = GetUtilityPowerProducer(original.producer)->consumed_lifetime_j;
	AutoRestoreBackup spectator(_local_company, COMPANY_SPECTATOR);
	REQUIRE(Company::CanAllocateItem()); std::unique_ptr<Company> successor{Company::Create()}; successor->money = 1000000;
	const auto successor_id = successor->index;
	ChangeOwnershipOfCompanyItems(old_owner, successor_id);
	CHECK_FALSE(GetTubeGeothermalSource(rig.source)->active); CHECK(GetTubeGeothermalSource(rig.source)->owner == successor_id);
	const auto *retained = GetTubePowerVehicle(capsule->index); REQUIRE(retained != nullptr);
	CHECK(retained->geothermal_source == original.id);
	{
		AutoRestoreBackup buyer(_current_company, successor_id);
		const auto grant = GetUtilityAllocation(retained->consumer); REQUIRE(grant.has_value()); CHECK(grant->granted_energy_j == 0);
	}
	CHECK(GetGeothermalGeneratorState(rig.generator)->owner == successor_id);
	CHECK(rig.core->state.owner == successor_id); CHECK(GetUtilityPowerProducer(original.producer)->owner == successor_id);
	CHECK(GetUtilityPowerProducer(original.producer)->consumed_lifetime_j == used);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	ChangeOwnershipOfCompanyItems(successor_id, INVALID_OWNER); successor.reset();
	const auto retired = ComputeUtilityPowerDigest(), references = ComputeTubeGeothermalPowerDigest();
	REQUIRE(Company::CanAllocateItem()); std::unique_ptr<Company> replacement{Company::Create()}; REQUIRE(replacement->index == successor_id);
	AutoRestoreBackup actor(_current_company, replacement->index);
	CHECK_FALSE(GetUtilityAllocation(original.supply).has_value());
	CHECK_FALSE(GetUtilityPowerProducer(original.producer)->active);
	CHECK(CloseTubeGeothermalPower(original.id) != TubePowerError::None);
	ChangeOwnershipOfCompanyItems(replacement->index, INVALID_OWNER);
	CHECK(ComputeUtilityPowerDigest() == retired); CHECK(ComputeTubeGeothermalPowerDigest() == references);
	CHECK(GetUtilityPowerProducer(original.producer)->consumed_lifetime_j == used);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}
