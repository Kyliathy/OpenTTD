/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal_work_loss.cpp Native geothermal shortage, physical loss and manual-stop recovery without renewed rights or lost cargo.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BCD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_tube_work_fixture.h"
#include "../safeguards.h"

/** Native force-proceed cannot bypass absent original generation or physical connectivity; recovery preserves existing cargo, stop intent and demand fractions. */
TEST_CASE("Geothermal native shortages and lost paths hold loaded capsules", "[tube-geothermal-work-loss]")
{
	MockEnvironment::Instance(); GeothermalTubeWorkRig rig; rig.JoinDepots(); auto *capsule = rig.Buy();
	const auto packet = rig.Load(*capsule); const auto source = rig.Connect(1);
	const auto consumer = BindTubeGeothermalPowerVehicle(capsule->index, source.source); REQUIRE(consumer.has_value());
	capsule->reliability = 100; rig.Run(std::array{capsule}); REQUIRE(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == 540);
	REQUIRE(capsule->reliability == capsule->GetEngine()->reliability);
	const auto model = GENERATE(AccelerationModel::Original, AccelerationModel::Realistic);
	AutoRestoreBackup acceleration(_settings_game.vehicle.train_acceleration_model, model);
	AutoRestoreBackup money(Company::Get(_current_company)->money); const auto cash = Company::Get(_current_company)->money;
	const auto cable = TileXYL(35, 24, MapLayer::DeepUnderground), track = TileXYL(32, 25, MapLayer::DeepUnderground);
	bool cable_lost = false, track_lost = false, heat_lost = false, source_lost = false;
	SECTION("native generator cannot fund its service") { Company::Get(_current_company)->money = Money{0}; }
	SECTION("original heat route lost") {
		heat_lost = true;
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, rig.pipe[1], UtilityEnergyDomain::Heat).Succeeded());
	}
	SECTION("purchased electrical cable lost") {
		cable_lost = true;
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, cable, UtilityEnergyDomain::Electricity).Succeeded());
	}
	SECTION("owned guideway disconnected") { track_lost = true; REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, track, Track::Y).Succeeded()); }
	SECTION("original source explicitly closed") { source_lost = true; REQUIRE(CloseTubeGeothermalPower(source.source) == TubePowerError::None); }
	rig.Start(*capsule, rig.inlets[1]); capsule->force_proceed = TFP_SIGNAL;
	const auto x = capsule->x_pos, y = capsule->y_pos; const auto original = *GetTubePowerVehicle(capsule->index);
	// Repeated native opportunities must hold both locomotive substeps, even
	// under force-proceed and after a previous genuinely consumed service tick.
	for (uint tick = 0; tick < 3; ++tick) {
		rig.Run(std::array{capsule}); CHECK(capsule->x_pos == x); CHECK(capsule->y_pos == y); CHECK(capsule->cur_speed == 0);
		CHECK_FALSE(capsule->vehstatus.Test(VehState::Stopped)); CHECK(capsule->cargo.StoredCount() == 17); CHECK(CargoPacket::IsValidID(packet));
		const auto binding = *GetTubePowerVehicle(capsule->index);
		CHECK(binding.consumer == *consumer); CHECK(binding.remainder_mj == original.remainder_mj); CHECK(binding.consumed_tick == original.consumed_tick);
		CHECK(binding.wait == (cable_lost || track_lost || source_lost ? TubePowerWait::Topology : TubePowerWait::Grant));
	}
	CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == 540);
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded());
	Company::Get(_current_company)->money = cash;
	if (cable_lost) rig.Build(cable, UtilityEnergyDomain::Electricity);
	if (heat_lost) rig.Build(rig.pipe[1], UtilityEnergyDomain::Heat);
	if (track_lost) REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, track, GetTubeRailType(), Track::Y, false).Succeeded());
	rig.Run(std::array{capsule}); CHECK(capsule->vehstatus.Test(VehState::Stopped)); CHECK(capsule->x_pos == x); CHECK(capsule->y_pos == y);
	CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == 540);
	// Repaired original cable/heat/guideway resumes only after explicit Start.
	// Retired source identity requires later explicit switching and stays dead.
	if (!source_lost) {
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded());
		rig.Run(std::array{capsule}); CHECK(GetTubePowerVehicle(capsule->index)->consumed_tick == TimerGameTick::counter);
		CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j > 540);
	}
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
}

/** A whole grant imported before physical loss cannot authorize later movement or refund the already-used original service. */
TEST_CASE("Geothermal cable loss between preparation and native work revokes unused slices", "[tube-geothermal-work-late-loss]")
{
	MockEnvironment::Instance(); GeothermalTubeWorkRig rig; rig.JoinDepots(); auto *capsule = rig.Buy(); const auto source = rig.Connect();
	const auto packet = rig.Load(*capsule); REQUIRE(BindTubeGeothermalPowerVehicle(capsule->index, source.source).has_value());
	rig.Start(*capsule, rig.inlets[1]); ++TimerGameTick::counter; PrepareTubePowerTick();
	const auto binding = *GetTubePowerVehicle(capsule->index); const auto demand = ComputeTubePowerDemand(*capsule, TubePowerPhase::Motion); REQUIRE(demand.has_value());
	REQUIRE(GetUtilityPowerLink(binding.local_feed)->flow_j == demand->energy_j);
	const auto tile = TileXYL(35, 24, MapLayer::DeepUnderground);
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, tile, UtilityEnergyDomain::Electricity).Succeeded());
	const auto ledger = ComputeUtilityPowerDigest(); const auto x = capsule->x_pos, y = capsule->y_pos;
	capsule->force_proceed = TFP_SIGNAL; REQUIRE(capsule->Tick());
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(capsule->x_pos == x); CHECK(capsule->y_pos == y);
	CHECK(GetTubePowerVehicle(capsule->index)->consumed_tick == UINT64_MAX); CHECK(GetTubePowerVehicle(capsule->index)->remainder_mj == 0);
	CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == 0); CHECK(capsule->cargo.StoredCount() == 17); CHECK(CargoPacket::IsValidID(packet));
	CHECK(GetUtilityPowerConsumer(binding.consumer)->shares.front().released_j == demand->energy_j);
	CHECK(GetUtilityPowerLink(binding.local_feed)->flow_j == demand->energy_j);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}
