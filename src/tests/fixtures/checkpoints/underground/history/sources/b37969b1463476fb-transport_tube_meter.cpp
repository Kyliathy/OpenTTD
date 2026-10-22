/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_meter.cpp Actual paid native ticks, admitted demand carry, complete invoice refusal and unpowered physical holds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups6-7
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.2
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_meter_fixture.h"
#include "../safeguards.h"

/** Checked physical load and phase integrate canonical27-ms units; pure queries never consume or update carry. */
TEST_CASE("Tube demand uses actual load model phase and admitted fractions", "[transport-tube-meter]")
{
	TubePowerRig rig;
	const auto empty = ComputeTubePowerDemand(*rig.capsule, TubePowerPhase::Motion);
	REQUIRE(empty.has_value()); CHECK(empty->rate_w == 10000); CHECK(empty->energy_j == 270);
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
	packet->UpdateLoadingTile(rig.depot); rig.capsule->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
	const auto loaded = ComputeTubePowerDemand(*rig.capsule, TubePowerPhase::Motion);
	REQUIRE(loaded.has_value()); CHECK(loaded->rate_w == 12833); CHECK(loaded->energy_j == 346); CHECK(loaded->remainder_mj == 491);
	const auto next = ComputeTubePowerDemand(*rig.capsule, TubePowerPhase::Motion, 1, loaded->remainder_mj);
	REQUIRE(next.has_value()); CHECK(next->energy_j == 346); CHECK(next->remainder_mj == 982);
	const auto service = ComputeTubePowerDemand(*rig.capsule, TubePowerPhase::DepotService, 74);
	REQUIRE(service.has_value()); CHECK(service->energy_j == 39960);
	CHECK(ComputeTubePowerDemand(*rig.capsule, TubePowerPhase::Idle, 1, 982)->remainder_mj == 982);
	CHECK_FALSE(ComputeTubePowerDemand(*rig.capsule, TubePowerPhase::Motion, 0).has_value());
	CHECK_FALSE(ComputeTubePowerDemand(*rig.capsule, TubePowerPhase::Motion, 1, 1000).has_value());
}

/** A physically compatible and force-proceeding capsule still cannot depart without an actual prepared electrical grant. */
TEST_CASE("Tube native departure refuses missing grants under both acceleration models", "[transport-tube-meter]")
{
	MockEnvironment::Instance();
	TubeMeterRig rig;
	const auto model = GENERATE(AccelerationModel::Original, AccelerationModel::Realistic);
	AutoRestoreBackup acceleration(_settings_game.vehicle.train_acceleration_model, model);
	rig.Start(); rig.capsule->force_proceed = TFP_SIGNAL;
	const auto x = rig.capsule->x_pos, y = rig.capsule->y_pos;
	rig.Run(100);
	CHECK(rig.capsule->tile == rig.depot); CHECK(rig.capsule->x_pos == x); CHECK(rig.capsule->y_pos == y);
	CHECK(rig.capsule->cur_speed == 0); CHECK_FALSE(rig.capsule->vehstatus.Test(VehState::Stopped));
}

/** One actual granted tick covers both native locomotive handlers; cache callbacks are never electricity or invoice owners. */
TEST_CASE("Tube native powered route consumes one actual front opportunity per epoch", "[transport-tube-meter]")
{
	MockEnvironment::Instance();
	TubeMeterRig rig;
	const bool loaded = GENERATE(false, true);
	if (loaded) {
		// A genuine partial hold makes admitted millijoule carry observable
		// during actual movement, with the same original packet retained at arrival.
		REQUIRE(CargoPacket::CanAllocateItem());
		auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
		packet->UpdateLoadingTile(rig.depot); rig.capsule->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
		rig.capsule->CargoChanged();
	}
	rig.Bind(); rig.Start();
	const auto model = GENERATE(AccelerationModel::Original, AccelerationModel::Realistic);
	AutoRestoreBackup acceleration(_settings_game.vehicle.train_acceleration_model, model);
	uint64_t expected = 0;
	uint16_t expected_carry = 0;
	uint ticks = 0;
	// Accumulate only native motoring opportunities while the real train
	// traverses each track cell and enters its ordinary halt depot.
	while (!rig.capsule->IsStoppedInDepot() && ticks < 6000) {
		const auto demand = ComputeTubePowerDemand(*rig.capsule, GetTubePowerPhase(*rig.capsule), 1, expected_carry); REQUIRE(demand.has_value());
		expected += demand->energy_j;
		expected_carry = demand->remainder_mj;
		rig.Run(); ++ticks;
	}
	INFO(DescribeTrain(rig.capsule)); REQUIRE(ticks < 6000);
	CHECK(rig.capsule->tile == rig.destination); CHECK(ticks > 0);
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == expected);
	CHECK(GetTubePowerConnection(rig.source)->invoiced_money == 20 + expected / 1000);
	CHECK(GetTubePowerConnection(rig.source)->usage_remainder_j == expected % 1000);
	CHECK(GetTubePowerVehicle(rig.capsule->index)->remainder_mj == expected_carry);
	CHECK(rig.capsule->cargo.StoredCount() == (loaded ? 17 : 0));
	const auto ledger = ComputeUtilityPowerDigest(), invoice = ComputeTubePowerDigest();
	const auto cash = Company::Get(_current_company)->money;
	// Consist/power/cargo cache refreshes and repeated GUI queries expose the
	// same native metadata without renewing, consuming or charging an offer.
	for (uint redraw = 0; redraw < 10; ++redraw) {
		rig.capsule->CargoChanged(); rig.capsule->ConsistChanged(CCF_LOADUNLOAD);
		GetUtilityAllocation(rig.consumer);
	}
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == invoice);
	CHECK(Company::Get(_current_company)->money == cash);
}

/** Actual damaged depot service settles one mandatory20-kW tick, preserves the manual stop, and cannot replay in the same simulation epoch. */
TEST_CASE("Tube actual depot service requires paid native work and preserves manual stop", "[transport-tube-meter]")
{
	TubeMeterRig rig; rig.Bind();
	rig.capsule->reliability = 100;
	VehicleServiceInDepot(rig.capsule); CHECK(rig.capsule->reliability == 100);
	rig.Run();
	CHECK(rig.capsule->reliability == rig.capsule->GetEngine()->reliability);
	CHECK(rig.capsule->vehstatus.Test(VehState::Stopped));
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == 540);
	const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerVehicleDigest();
	PrepareTubePowerTick(); REQUIRE(rig.capsule->Tick());
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerVehicleDigest() == physical);
}

/** Physical binding estimates and bad saved source/consumer/fraction references cannot allocate or repair a native power owner. */
TEST_CASE("Tube binding admission is pure and saved identities refuse malformed references", "[transport-tube-meter]")
{
	TubePowerRig rig;
	const auto service = ConnectTubeExternalPower(rig.portal, rig.depot, rig.path);
	REQUIRE(service.error == TubePowerError::None);
	const auto ledger = ComputeUtilityPowerDigest(), bodies = ComputeTubePowerVehicleDigest();
	const auto estimate = BindTubePowerVehicle(rig.capsule->index, service.producer, false);
	REQUIRE(estimate.has_value()); CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerVehicleDigest() == bodies);
	REQUIRE(BindTubePowerVehicle(rig.capsule->index, service.producer) == estimate);
	CHECK_FALSE(BindTubePowerVehicle(rig.capsule->index, service.producer).has_value());
	const auto good = CaptureTubePowerVehicles(); auto malformed = good;
	SECTION("orphan consumer") { malformed[0].consumer = UtilityConsumerID{999}; }
	SECTION("orphan source") { malformed[0].source = UtilityProducerID{999}; }
	SECTION("orphan vehicle") { malformed[0].vehicle = VehicleID::Invalid(); }
	SECTION("invalid fractional demand") { malformed[0].remainder_mj = 1000; }
	SECTION("future physical opportunity") { malformed[0].prepared_tick = TimerGameTick::counter + 1; }
	SECTION("duplicate active association") { malformed.push_back(malformed.front()); }
	const auto digest = ComputeTubePowerVehicleDigest();
	StageLoadedTubePowerVehicles(malformed, UINT64_MAX);
	CHECK_FALSE(AcceptLoadedTubePowerVehicles()); CHECK(ComputeTubePowerVehicleDigest() == digest);
	CHECK(ValidateTubePowerVehicles(good) == TubePowerError::None);
}

/** Actual grant availability never substitutes for cash; a refused whole invoice leaves the capsule physically held and demand carry untouched. */
TEST_CASE("Tube native cash refusal preserves actual held footprint and grant", "[transport-tube-meter]")
{
	MockEnvironment::Instance();
	TubeMeterRig rig; rig.Bind(); rig.Start();
	Company::Get(_current_company)->money = Money{0};
	const auto tile = rig.capsule->tile;
	const auto x = rig.capsule->x_pos, y = rig.capsule->y_pos;
	rig.Run(50);
	CHECK(rig.capsule->tile == tile); CHECK(rig.capsule->x_pos == x); CHECK(rig.capsule->y_pos == y);
	CHECK(GetTubePowerVehicle(rig.capsule->index)->wait == TubePowerWait::Funds);
	CHECK(GetTubePowerVehicle(rig.capsule->index)->remainder_mj == 0);
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == 0);
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 270);
	CHECK(GetTubePowerConnection(rig.source)->invoiced_money == 20);
}
