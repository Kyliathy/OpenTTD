/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal_binding.cpp Actual source/provider binding, native guideway admission and scoped recipient lifecycle; powered work remains the next producer.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BCC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "geothermal_tube_capsule_fixture.h"
#include "../multilayer_state.h"
#include "../safeguards.h"

/** A real paid native capsule adopts only the original inlet/output/consumer/feed tuple; estimates and duplicate refusal alter no identity, money, quantity or RNG. */
TEST_CASE("Native capsule binds an original geothermal provider without generation", "[tube-geothermal-binding]")
{
	GeothermalTubeCapsuleRig rig; auto *capsule = rig.Buy(); const auto source = rig.Connect();
	const auto original = CaptureUtilityPowerState(); const auto ledger = ComputeUtilityPowerDigest(), bindings = ComputeTubePowerVehicleDigest();
	const auto metadata = ComputeTubeGeothermalPowerDigest(), native = ComputeGeothermalGeneratorDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	const auto quote = BindTubeGeothermalPowerVehicle(capsule->index, source.source, false); REQUIRE(quote.has_value());
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerVehicleDigest() == bindings);
	const auto actual = BindTubeGeothermalPowerVehicle(capsule->index, source.source); REQUIRE(actual == quote);
	const auto *binding = GetTubePowerVehicle(capsule->index); REQUIRE(binding != nullptr);
	CHECK(binding->consumer == *actual); CHECK(binding->source == rig.output); CHECK(binding->geothermal_source == source.source);
	CHECK(binding->local_feed.base() == original.next_link); CHECK(binding->wait == TubePowerWait::Unavailable);
	const auto *feed = GetUtilityPowerLink(binding->local_feed); REQUIRE(feed != nullptr);
	CHECK(feed->source == source.supply); CHECK(feed->destination == *actual); CHECK(feed->generator == rig.generator);
	CHECK(GetUtilityPowerConsumer(*actual)->linked_producers.empty()); CHECK(GetUtilityPowerConsumer(*actual)->shares.empty());
	CHECK(GetTubePowerConnection(rig.output) == nullptr); CHECK(CaptureUtilityPowerState().next_producer == original.next_producer);
	CHECK(ComputeTubeGeothermalPowerDigest() == metadata); CHECK(ComputeGeothermalGeneratorDigest() == native);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
	const auto current = ComputeUtilityPowerDigest(); CHECK_FALSE(BindTubeGeothermalPowerVehicle(capsule->index, source.source).has_value());
	CHECK(ComputeUtilityPowerDigest() == current);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	const auto schema = CaptureMultiLayerStateSchema();
	CHECK((schema.feature_flags & MULTILAYER_FEATURE_TUBE_VEHICLES) != 0); CHECK((schema.feature_flags & MULTILAYER_FEATURE_TUBE_GEOTHERMAL) != 0);
	CHECK((schema.feature_flags & MULTILAYER_FEATURE_TUBE_POWER) == 0);
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::None);
}

/** Actual facing depots/track make one component; electrical cable adjacency alone cannot connect the other purchase depot. */
TEST_CASE("Geothermal provider binding follows actual native guideways", "[tube-geothermal-binding-topology]")
{
	GeothermalTubeCapsuleRig rig;
	const bool joined = GENERATE(false, true);
	if (joined) rig.JoinDepots();
	auto *capsule = rig.Buy(1); const auto source = rig.Connect();
	const auto ledger = ComputeUtilityPowerDigest(), bindings = ComputeTubePowerVehicleDigest();
	CHECK(IsTubeGeothermalPowerConnected(source.source, *capsule) == joined);
	CHECK(BindTubeGeothermalPowerVehicle(capsule->index, source.source, false).has_value() == joined);
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerVehicleDigest() == bindings);
	if (joined) {
		REQUIRE(BindTubeGeothermalPowerVehicle(capsule->index, source.source).has_value());
		CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	}
}

/** Actual capsule, company, source, calendar and purchased-path admission precede both IDs; quote/execution refusal changes no original owner, money or RNG. */
TEST_CASE("Geothermal capsule binding refuses incomplete native admission unchanged", "[tube-geothermal-binding-refusal]")
{
	MockEnvironment::Instance(); std::unique_ptr<Company> foreign;
	GeothermalTubeCapsuleRig rig; auto *capsule = rig.Buy(); auto source = rig.Connect().source; auto vehicle = capsule->index;
	AutoRestoreBackup actor(_current_company); AutoRestoreBackup date(TimerGameCalendar::date);
	SECTION("unknown original source") { source = TubeGeothermalSourceID{999}; }
	SECTION("unknown actual vehicle") { vehicle = VehicleID::Invalid(); }
	SECTION("foreign actual company") { REQUIRE(Company::CanAllocateItem()); foreign.reset(Company::Create()); _current_company = foreign->index; }
	SECTION("locked actual plant model") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 0, 1); }
	SECTION("retired original source") { REQUIRE(CloseTubeGeothermalPower(source) == TubePowerError::None); }
	SECTION("crashed actual capsule") { capsule->vehstatus.Set(VehState::Crashed); }
	SECTION("explicitly started capsule") { REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, vehicle, true).Succeeded()); }
	SECTION("missing purchased cable") {
		const auto tile = TileXYL(35, 24, MapLayer::DeepUnderground);
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
			std::make_tuple(tile, UtilityEnergyDomain::Electricity)).Succeeded());
	}
	const auto utility = ComputeUtilityPowerDigest(), bindings = ComputeTubePowerVehicleDigest();
	const auto metadata = ComputeTubeGeothermalPowerDigest(), native = ComputeGeothermalGeneratorDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	CHECK_FALSE(BindTubeGeothermalPowerVehicle(vehicle, source, false).has_value()); CHECK_FALSE(BindTubeGeothermalPowerVehicle(vehicle, source).has_value());
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubePowerVehicleDigest() == bindings);
	CHECK(ComputeTubeGeothermalPowerDigest() == metadata); CHECK(ComputeGeothermalGeneratorDigest() == native);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
}

/** Real vehicle sale closes only its original recipient/feed, preserves a sibling/source and cannot overflow terminal shared receipt history. */
TEST_CASE("Native geothermal capsule retirement is scoped and nonreused", "[tube-geothermal-binding-retirement]")
{
	GeothermalTubeCapsuleRig rig; auto *first = rig.Buy(); auto *second = rig.Buy(); const auto source = rig.Connect();
	REQUIRE(BindTubeGeothermalPowerVehicle(first->index, source.source).has_value()); REQUIRE(BindTubeGeothermalPowerVehicle(second->index, source.source).has_value());
	const auto retired = *GetTubePowerVehicle(first->index), sibling = *GetTubePowerVehicle(second->index);
	const bool terminal = GENERATE(false, true);
	if (terminal) {
		auto state = CaptureUtilityPowerState(); state.operation_sequence = UINT64_MAX - 1;
		StageLoadedUtilityPowerState(std::move(state)); REQUIRE(AcceptLoadedUtilityPowerState());
	}
	REQUIRE(Command<Commands::SellVehicle>::Do(DoCommandFlag::Execute, first->index, true, false, ClientID::Invalid).Succeeded());
	CHECK_FALSE(GetUtilityPowerConsumer(retired.consumer)->active); CHECK_FALSE(GetUtilityPowerLink(retired.local_feed)->active);
	CHECK(GetUtilityPowerConsumer(sibling.consumer)->active); CHECK(GetUtilityPowerLink(sibling.local_feed)->active);
	CHECK(GetTubeGeothermalSource(source.source)->active); CHECK(GetUtilityPowerConsumer(source.supply)->active); CHECK(GetUtilityPowerProducer(rig.output)->active);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	if (terminal) {
		CHECK(CaptureUtilityPowerState().operation_sequence == UINT64_MAX - 1);
	} else {
		auto *replacement = rig.Buy(); CHECK(GetTubePowerVehicle(replacement->index) == nullptr);
		const auto fresh = BindTubeGeothermalPowerVehicle(replacement->index, source.source); REQUIRE(fresh.has_value());
		CHECK(*fresh != retired.consumer); CHECK(GetTubePowerVehicle(replacement->index)->local_feed != retired.local_feed);
		CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	}
}

/** An idle manual stop requests no geothermal grant and cannot be mistaken for permission to depart or accrue demand carry. */
TEST_CASE("Saved geothermal binding preserves a manually stopped idle capsule", "[tube-geothermal-binding-wait]")
{
	MockEnvironment::Instance();
	GeothermalTubeCapsuleRig rig; auto *capsule = rig.Buy(); const auto source = rig.Connect();
	REQUIRE(BindTubeGeothermalPowerVehicle(capsule->index, source.source).has_value());
	AutoRestoreBackup pause(_pause_mode, PauseModes{});
	const auto tile = capsule->tile; ++TimerGameTick::counter; PrepareTubePowerTick(); REQUIRE(capsule->Tick());
	const auto *binding = GetTubePowerVehicle(capsule->index); REQUIRE(binding != nullptr);
	CHECK(binding->wait == TubePowerWait::None); CHECK(binding->consumed_tick == UINT64_MAX); CHECK(binding->remainder_mj == 0);
	CHECK(capsule->vehstatus.Test(VehState::Stopped));
	CHECK(GetUtilityPowerLink(binding->local_feed)->flow_j == 0); CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == 0);
	CHECK(capsule->tile == tile); CHECK(capsule->cur_speed == 0);
}
