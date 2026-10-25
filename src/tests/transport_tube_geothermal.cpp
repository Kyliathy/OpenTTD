/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal.cpp Original native geothermal tube source ownership, pure connection and scoped supply lifecycle; no capsule work is simulated.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BBB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_tube_power_fixture.h"
#include "../safeguards.h"

/** Detached source transport preserves physical identity and retired history, refuses corrupt references without publishing, and separates readiness from identity. */
TEST_CASE("Geothermal tube source transport validates original references atomically", "[tube-geothermal-source-state]")
{
	GeothermalTubePowerRig rig; const auto first = rig.Connect(0), second = rig.Connect(1);
	const auto original = CaptureTubeGeothermalPowerState(); const auto utility = CaptureUtilityPowerState();
	const auto digest = ComputeTubeGeothermalPowerDigest(), ledger = ComputeUtilityPowerDigest(); REQUIRE(ValidateTubeGeothermalPowerState(original, utility));
	SECTION("accepted exact references") {
		ResetTubeGeothermalPowerState(); StageLoadedTubeGeothermalPowerState(original); REQUIRE(AcceptLoadedTubeGeothermalPowerState());
		CHECK(ComputeTubeGeothermalPowerDigest() == digest);
	}
	SECTION("wrong original metadata refuses unchanged") {
		auto bad = original;
		const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7);
		switch (fault) {
			case 0: ++bad.next_source; break;
			case 1: bad.sequence = UINT64_MAX; break;
			case 2: bad.sources[0].producer = UtilityProducerID{UINT32_MAX}; break;
			case 3: bad.sources[0].supply = second.supply; break;
			case 4: bad.sources[0].active = false; break;
			case 5: bad.sources[0].port = rig.inlets[0]; break;
			case 6: bad.sources[0].depot = DepotID::Invalid(); break;
			case 7: bad.sources[0].inlet = rig.inlets[1]; break;
		}
		StageLoadedTubeGeothermalPowerState(bad); CHECK_FALSE(AcceptLoadedTubeGeothermalPowerState());
		CHECK(ComputeTubeGeothermalPowerDigest() == digest); CHECK(ComputeUtilityPowerDigest() == ledger);
		StageLoadedTubeGeothermalPowerState(original); REQUIRE(AcceptLoadedTubeGeothermalPowerState());
	}
	SECTION("temporary date lock retains original identity") {
		AutoRestoreBackup date(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 0, 1));
		CHECK_FALSE(GetTubeGeothermalSourcePath(first.source).has_value()); CHECK(ValidateTubeGeothermalPowerState(original, utility));
	}
	SECTION("retired history retains original references") {
		REQUIRE(CloseTubeGeothermalPower(first.source) == TubePowerError::None);
		const auto retired = CaptureTubeGeothermalPowerState(); const auto closed = CaptureUtilityPowerState();
		CHECK(ValidateTubeGeothermalPowerState(retired, closed));
		auto bad = retired; bad.sources[0].active = true; CHECK_FALSE(ValidateTubeGeothermalPowerState(bad, closed));
	}
}

/** Both actual dated plants acquire an independent original inlet identity and one shared root, without adding power, spending a second tariff or changing RNG. */
TEST_CASE("Geothermal tube source connection owns references without duplicate generation", "[tube-geothermal-source-owner]")
{
	GeothermalTubePowerRig rig{GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop)};
	const auto utility = CaptureUtilityPowerState(); const auto physical = CaptureTubeGeothermalPowerState();
	const auto native = ComputeGeothermalGeneratorDigest(), metadata = ComputeTubeGeothermalPowerDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	const auto quote = ConnectTubeGeothermalPower(rig.generator, rig.inlets[0], false); REQUIRE(quote.error == TubePowerError::None);
	CHECK(ComputeTubeGeothermalPowerDigest() == metadata); CHECK(CaptureUtilityPowerState().next_consumer == utility.next_consumer);
	const auto result = rig.Connect(); CHECK(result.source == quote.source); CHECK(result.supply == quote.supply);
	const auto after = CaptureUtilityPowerState(); const auto state = CaptureTubeGeothermalPowerState();
	CHECK(after.next_producer == utility.next_producer); CHECK(after.next_consumer == utility.next_consumer + 1);
	CHECK(after.next_link == utility.next_link); CHECK(after.next_share == utility.next_share);
	CHECK(state.next_source == physical.next_source + 1); CHECK(state.sequence == physical.sequence + 1);
	REQUIRE(state.sources.size() == 1); const auto &source = state.sources.front();
	CHECK(source.generator == rig.generator); CHECK(source.producer == rig.output); CHECK(source.supply == result.supply);
	CHECK(GetUtilityPowerConsumer(source.supply)->linked_producers == std::vector{rig.output});
	CHECK(GetUtilityPowerConsumer(source.supply)->shares.empty()); CHECK(GetTubeGeothermalSourcePath(source.id).has_value());
	CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(std::ranges::equal(random.state, _random.state));
	const auto connected = ComputeTubeGeothermalPowerDigest(), ledger = ComputeUtilityPowerDigest();
	CHECK(ConnectTubeGeothermalPower(rig.generator, rig.inlets[0]).error == TubePowerError::Refused);
	CHECK(ComputeTubeGeothermalPowerDigest() == connected); CHECK(ComputeUtilityPowerDigest() == ledger);
}

/** Closing one actual source retires only its root/feed promises while an independently connected recipient retains the same native producer and its own grant. */
TEST_CASE("Geothermal tube source closure preserves original output and sibling users", "[tube-geothermal-source-supply]")
{
	GeothermalTubePowerRig rig; const auto first = rig.Connect(0), second = rig.Connect(1);
	const auto recipient = RegisterUtilityPowerConsumer(_current_company, MapLayer::DeepUnderground, UtilityConsumerEffect::TubeWork, 10000, {});
	REQUIRE(recipient.has_value()); const auto feed = RegisterGeothermalLocalFeed(rig.generator, first.supply, *recipient, rig.inlets[0]); REQUIRE(feed.has_value());
	rig.Allocate(std::array{UtilityPowerRequest{first.supply, 3000}, UtilityPowerRequest{second.supply, 3000}, UtilityPowerRequest{*recipient, 3000}});
	const auto path = GetTubeGeothermalSourcePath(first.source); REQUIRE(path.has_value());
	UtilityPathTransferRequest request{{first.supply, GetUtilityPowerInterval()}, *recipient, rig.plant_port, 2000}; request.legs.push_back({*feed, path->cells});
	REQUIRE(CommitUtilityPathTransfer(PrepareUtilityPathTransfer(request)).error == UtilityPowerError::None);
	const auto native = ComputeGeothermalGeneratorDigest(); const auto generated = GetUtilityPowerProducer(rig.output)->generated_lifetime_j;
	const auto before = ComputeUtilityPowerDigest(), references = ComputeTubeGeothermalPowerDigest();
	REQUIRE(CloseTubeGeothermalPower(first.source, false) == TubePowerError::None);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(ComputeTubeGeothermalPowerDigest() == references);
	REQUIRE(CloseTubeGeothermalPower(first.source) == TubePowerError::None);
	CHECK_FALSE(GetTubeGeothermalSource(first.source)->active); CHECK_FALSE(GetUtilityPowerConsumer(first.supply)->active);
	CHECK_FALSE(GetUtilityPowerLink(*feed)->active); CHECK(GetUtilityPowerLink(*feed)->flow_j == 2000);
	CHECK(GetUtilityPowerConsumer(*recipient)->shares.front().released_j == 2000);
	CHECK(GetUtilityPowerProducer(rig.output)->active); CHECK(GetUtilityPowerProducer(rig.output)->reserved_j == 3000);
	CHECK(GetUtilityPowerProducer(rig.output)->generated_lifetime_j == generated); CHECK(ComputeGeothermalGeneratorDigest() == native);
	CHECK(GetTubeGeothermalSource(second.source)->active); CHECK(GetTubeGeothermalSourcePath(second.source).has_value());
	CHECK(GetUtilityPowerConsumer(second.supply)->meter.granted_energy_j == 3000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Wrong actual actor, model date, recipient or purchased path refuses before any source/root issuer, quantity, cash or RNG changes. */
TEST_CASE("Geothermal tube source admission refuses unchanged", "[tube-geothermal-source-refusal]")
{
	std::unique_ptr<Company> foreign;
	GeothermalTubePowerRig rig; AutoRestoreBackup actor(_current_company); AutoRestoreBackup date(TimerGameCalendar::date);
	auto inlet = rig.inlets[0];
	SECTION("foreign actual company") { REQUIRE(Company::CanAllocateItem()); foreign.reset(Company::Create()); _current_company = foreign->index; }
	SECTION("locked actual model") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 0, 1); }
	SECTION("not a native inlet") { inlet = TileXYL(34, 24, MapLayer::DeepUnderground); }
	SECTION("missing actual cable") {
		const auto tile = TileXYL(35, 24, MapLayer::DeepUnderground);
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
			std::make_tuple(tile, UtilityEnergyDomain::Electricity)).Succeeded());
	}
	const auto before = ComputeUtilityPowerDigest(), physical = ComputeTubeGeothermalPowerDigest(); const auto random = _random;
	CHECK(ConnectTubeGeothermalPower(rig.generator, inlet, false).error != TubePowerError::None);
	CHECK(ConnectTubeGeothermalPower(rig.generator, inlet).error != TubePowerError::None);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(ComputeTubeGeothermalPowerDigest() == physical);
	CHECK(std::ranges::equal(random.state, _random.state));
}

/** Actual depot replacement retires the original physical source and root before another native facility on the same tile can acquire a new incarnation. */
TEST_CASE("Geothermal tube inlet replacement cannot reuse source identity", "[tube-geothermal-source-lifecycle]")
{
	GeothermalTubePowerRig rig; const auto original = rig.Connect(); const auto tile = rig.inlets[0];
	REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile)).Succeeded());
	CHECK_FALSE(GetTubeGeothermalSource(original.source)->active); CHECK_FALSE(GetUtilityPowerConsumer(original.supply)->active);
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
		std::make_tuple(tile, UtilityEnergyDomain::Electricity)).Succeeded());
	rig.BuyDepot(tile); rig.Build(tile, UtilityEnergyDomain::Electricity); const auto replacement = rig.Connect();
	CHECK(replacement.source != original.source); CHECK(replacement.supply != original.supply);
	CHECK_FALSE(GetTubeGeothermalSourcePath(original.source).has_value()); CHECK(GetTubeGeothermalSourcePath(replacement.source).has_value());
}

/** Original native output retirement closes all of its physical source incarnations without deleting consumed generation/service history. */
TEST_CASE("Geothermal tube source follows original generator loss", "[tube-geothermal-source-lifecycle]")
{
	GeothermalTubePowerRig rig; const auto first = rig.Connect(0), second = rig.Connect(1);
	const auto generated = GetUtilityPowerProducer(rig.output)->generated_lifetime_j;
	REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, rig.plant_port, std::make_tuple(rig.plant_port)).Succeeded());
	CHECK_FALSE(GetTubeGeothermalSource(first.source)->active); CHECK_FALSE(GetTubeGeothermalSource(second.source)->active);
	CHECK_FALSE(GetUtilityPowerConsumer(first.supply)->active); CHECK_FALSE(GetUtilityPowerConsumer(second.supply)->active);
	CHECK(GetUtilityPowerProducer(rig.output)->generated_lifetime_j == generated);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** A real producer loss closes several original forwarding roots even when their shared operation receipt has only one valid increment left. */
TEST_CASE("Geothermal tube source loss cannot overflow the shared terminal receipt", "[tube-geothermal-source-lifecycle]")
{
	GeothermalTubePowerRig rig; const auto first = rig.Connect(0), second = rig.Connect(1);
	rig.Allocate(std::array{UtilityPowerRequest{first.supply, 2000}, UtilityPowerRequest{second.supply, 2000}});
	auto state = CaptureUtilityPowerState(); state.operation_sequence = UINT64_MAX - 2;
	StageLoadedUtilityPowerState(std::move(state)); REQUIRE(AcceptLoadedUtilityPowerState());
	REQUIRE(RemoveUtilityPowerProducer(rig.output) == UtilityPowerError::None);
	CHECK(CaptureUtilityPowerState().operation_sequence == UINT64_MAX - 1);
	CHECK_FALSE(GetTubeGeothermalSource(first.source)->active); CHECK_FALSE(GetTubeGeothermalSource(second.source)->active);
	CHECK_FALSE(GetUtilityPowerConsumer(first.supply)->active); CHECK_FALSE(GetUtilityPowerConsumer(second.supply)->active);
	CHECK(GetUtilityPowerProducer(rig.output)->reserved_j == 0);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Actual company takeover retires old permissions while original utility ownership/history follows the real successor, requiring explicit new inlet association. */
TEST_CASE("Geothermal tube source takeover retains history and requires new permission", "[tube-geothermal-source-lifecycle]")
{
	std::unique_ptr<Company> successor;
	GeothermalTubePowerRig rig; const auto original = rig.Connect();
	SECTION("live original source") {}
	SECTION("closed source with current original history") {
		rig.Allocate(std::array{UtilityPowerRequest{original.supply, 2000}});
		REQUIRE(CloseTubeGeothermalPower(original.source) == TubePowerError::None);
	}
	REQUIRE(Company::CanAllocateItem()); successor.reset(Company::Create()); successor->money = 1000000; successor->clear_limit = 64U << 16;
	ChangeOwnershipOfCompanyItems(_current_company, successor->index);
	CHECK_FALSE(GetTubeGeothermalSource(original.source)->active); CHECK(GetTubeGeothermalSource(original.source)->owner == successor->index);
	CHECK_FALSE(GetUtilityPowerConsumer(original.supply)->active); CHECK(GetUtilityPowerProducer(rig.output)->owner == successor->index);
	CHECK_FALSE(GetTubeGeothermalSourcePath(original.source).has_value());
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	AutoRestoreBackup actor(_current_company, successor->index); const auto replacement = rig.Connect();
	CHECK(replacement.source != original.source); CHECK(replacement.supply != original.supply);
}
