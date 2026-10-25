/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_town_recovery.cpp Retained native service repair, exact unused-share settlement and explicit source replacement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15BBC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor/receiver-boundary stock remain disclosed fixture inputs; actual paid-world files provide separate acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "geothermal_town_service_fixture.h"
#include "../geothermal_electrical.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** Both native route shapes and dated models restore the original town without allocating a second account or renewing a settled interval. */
TEST_CASE("Rebuilt native terminal reconnects its original geothermal town", "[geothermal-service-terminal-repair]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	const auto direct = GENERATE(false, true);
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	GeothermalTownServiceRig rig{direct, model};
	const auto original = ConnectGeothermalTownService(rig.service, false); REQUIRE(original.error == UtilityPowerError::None);
	REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded()); rig.Operate();
	const auto output = GetGeothermalGeneratorState(rig.generator)->electrical_producer;
	REQUIRE(GetUtilityPowerProducer(output)->consumed_lifetime_j == 135);
	REQUIRE(Command<Commands::RemoveFromRailStation>::Do(DoCommandFlag::Execute, rig.platform, rig.platform, false).Succeeded());
	rig.Operate(); // An empty native station sign survives removal; the normal coordinator detects its missing facilities and expires references before any rebuild.
	REQUIRE(GetUtilityPowerConsumer(original.destination)->town_terminal == StationID::Invalid());
	rig.service.terminal = rig.BuildTerminal();
	const auto before = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest();
	const auto native = ComputeGeothermalGeneratorDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	const auto proposed = ConnectGeothermalTownService(rig.service, false);
	CHECK(ComputeUtilityPowerDigest() == digest); // Preparing recovery must never expire promises in the live original owner, including late refusal.
	REQUIRE(proposed.error == UtilityPowerError::None);
	CHECK(proposed.source == original.source); CHECK(proposed.relay == original.relay); CHECK(proposed.destination == original.destination);
	CHECK(proposed.links == original.links); REQUIRE(rig.Connect().Succeeded()); CHECK(ComputeUtilityPowerDigest() == digest);
	REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded());
	const auto after = CaptureUtilityPowerState(); CHECK(after.operation_sequence == before.operation_sequence + 1);
	CHECK(after.next_consumer == before.next_consumer); CHECK(after.next_link == before.next_link); CHECK(after.next_share == before.next_share);
	CHECK(GetUtilityPowerProducer(output)->consumed_lifetime_j == 135);
	CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
	const auto repaired = ComputeUtilityPowerDigest(); CHECK(rig.Connect().Failed()); CHECK(rig.Connect(DoCommandFlag::Execute).Failed());
	CHECK(ComputeUtilityPowerDigest() == repaired); RunUtilityTownServices(); CHECK(ComputeUtilityPowerDigest() == repaired);
	rig.Operate(); CHECK(GetUtilityPowerProducer(output)->consumed_lifetime_j == 270);
	CHECK(GetUtilityPowerConsumer(original.destination)->town == rig.service.town); CHECK(GetUtilityTownPowerBonus(rig.service.town) == 25);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Recommissioning the actual paid pair issues only a fresh invalidated edge and retains every original spent joule until ordinary epoch expiry. */
TEST_CASE("Recommissioned native riser replaces only its retired town edge", "[geothermal-service-riser-repair]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTownServiceRig rig;
	const auto original = ConnectGeothermalTownService(rig.service, false); REQUIRE(original.error == UtilityPowerError::None);
	REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded()); rig.Operate();
	const auto portal = GetGeothermalRiserState(rig.service.second_riser)->site.portal;
	REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, portal, false).Succeeded());
	const auto closed = ComputeUtilityPowerDigest(); CHECK(rig.Connect().Failed()); CHECK(rig.Connect(DoCommandFlag::Execute).Failed());
	CHECK(ComputeUtilityPowerDigest() == closed);
	REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, portal, true).Succeeded());
	REQUIRE(IsGeothermalRiserOperational(rig.service.second_riser));
	const auto before = CaptureUtilityPowerState(); const auto proposed = ConnectGeothermalTownService(rig.service, false);
	REQUIRE(proposed.error == UtilityPowerError::None); CHECK(proposed.source == original.source); CHECK(proposed.destination == original.destination);
	CHECK(proposed.links[0] == original.links[0]); CHECK(proposed.links[1] != original.links[1]);
	REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded());
	CHECK(CaptureUtilityPowerState().next_consumer == before.next_consumer); CHECK(CaptureUtilityPowerState().next_link == before.next_link + 1);
	CHECK_FALSE(GetUtilityPowerLink(original.links[1])->active); CHECK(GetUtilityPowerLink(original.links[1])->flow_j == 135);
	CHECK(GetUtilityPowerLink(original.links[0])->active); CHECK(GetUtilityPowerLink(original.links[0])->flow_j == 135);
	CHECK(GetUtilityPowerLink(proposed.links[1])->flow_j == 0); CHECK(GetUtilityPowerConsumer(original.destination)->town_effect_applied);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	rig.Operate(); CHECK(GetUtilityPowerProducer(GetGeothermalGeneratorState(rig.generator)->electrical_producer)->consumed_lifetime_j == 270);
	CHECK(GetUtilityPowerLink(proposed.links[1])->flow_j == 135);
}

/** Actual partial export leaves both root and arriving unused rights; replacement releases those exact promises without refunding the already spent physical50J. */
TEST_CASE("Selected replacement settles original native service grants atomically", "[geothermal-service-source-replacement]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	/** Each attempt starts with the same real outstanding physical promises; refusal must preserve those promises exactly. */
	enum class ReplacementAttempt { Commit, FullLedger, LostAfterQuote };
	const auto attempt = GENERATE(ReplacementAttempt::Commit, ReplacementAttempt::FullLedger, ReplacementAttempt::LostAfterQuote);
	// One direct riser leaves enough of the fixture's ORIGINAL million to buy
	// the second actual model; no mid-test cash or material subsidy is added.
	GeothermalTownServiceRig rig{true, GeothermalGeneratorModel::MagmaLoop};
	const auto original = ConnectGeothermalTownService(rig.service, false); REQUIRE(original.error == UtilityPowerError::None);
	REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded()); rig.Operate();
	const auto replacement = rig.BuildReplacementGenerator(GeothermalGeneratorModel::VentTap); rig.Operate();
	const auto old_output = GetGeothermalGeneratorState(rig.generator)->electrical_producer;
	const auto new_output = GetGeothermalGeneratorState(replacement)->electrical_producer; REQUIRE(new_output.base() != UINT32_MAX);
	// Pause between the REAL coordinator phases to inspect an outstanding
	// physical promise. Native source publication/conversion still owns every
	// joule, bill and clock; no fake output or copied ledger is installed.
	++TimerGameTick::counter;
	{
		AutoRestoreBackup system{_current_company, OWNER_DEITY}; REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	PublishGeothermalHeatSources(GetUtilityPowerInterval(), 1); RunGeothermalGenerators(GetUtilityPowerInterval(), 1);
	std::vector<UtilityPowerRequest> requests; AppendUtilityTownServiceRequests(requests);
	{
		AutoRestoreBackup system{_current_company, OWNER_DEITY}; REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), requests) == UtilityPowerError::None);
	}
	const auto export_part = PrepareGeothermalPowerExport({{original.source, GetUtilityPowerInterval()}, original.destination, rig.plant_port, 50});
	REQUIRE(export_part.error == UtilityPowerError::None); REQUIRE(ExportGeothermalPower(export_part).error == UtilityPowerError::None);
	REQUIRE(GetUtilityAllocation(original.source)->granted_energy_j == 85); REQUIRE(GetUtilityAllocation(original.destination)->granted_energy_j == 50);
	REQUIRE(GetUtilityPowerLink(original.links[0])->flow_j == 50);
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, rig.plant_port, UtilityEnergyDomain::Heat).Succeeded());
	rig.service.generator = replacement;
	if (attempt == ReplacementAttempt::FullLedger) {
		const auto count = CaptureUtilityPowerState().consumers.size();
		// Fill the actual bounded issuer with real zero-demand Surface work
		// accounts. A refused late replacement must not release its original
		// root85J or town50J merely because its new identity cannot fit.
		for (size_t slot = count; slot < MAX_UTILITY_INTERVAL_ACCOUNTS; ++slot) {
			REQUIRE(RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::AssemblyWork, 0, {}).has_value());
		}
		const auto full = ComputeUtilityPowerDigest(); const auto native = ComputeGeothermalGeneratorDigest(); const auto random = _random;
		REQUIRE(ConnectGeothermalTownService(rig.service, false).error == UtilityPowerError::Limit);
		CHECK(rig.Connect().Failed()); CHECK(rig.Connect(DoCommandFlag::Execute).Failed());
		CHECK(ComputeUtilityPowerDigest() == full); CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(std::ranges::equal(random.state, _random.state));
		CHECK(GetUtilityAllocation(original.source)->granted_energy_j == 85); CHECK(GetUtilityAllocation(original.destination)->granted_energy_j == 50);
		CHECK(GetUtilityPowerLink(original.links[0])->flow_j == 50); CHECK(GetUtilityPowerConsumer(original.source)->active);
		return;
	}
	const auto before = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest();
	const auto native = ComputeGeothermalGeneratorDigest(); const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	const auto proposed = ConnectGeothermalTownService(rig.service, false);
	CHECK(ComputeUtilityPowerDigest() == digest); // Preparing recovery must never expire promises in the live original owner, including late refusal.
	REQUIRE(proposed.error == UtilityPowerError::None);
	CHECK(proposed.source != original.source); CHECK(proposed.destination == original.destination); CHECK(proposed.links[0] != original.links[0]);
	REQUIRE(rig.Connect().Succeeded()); CHECK(ComputeUtilityPowerDigest() == digest);
	if (attempt == ReplacementAttempt::LostAfterQuote) {
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute,
			GetGeothermalGeneratorPort(replacement), UtilityEnergyDomain::Electricity).Succeeded());
		const auto lost = ComputeUtilityPowerDigest(); const auto lost_native = ComputeGeothermalGeneratorDigest();
		const auto lost_cash = Company::Get(_current_company)->money; const auto lost_random = _random;
		// Execution revalidates the original hardware after a successful pure
		// quote; the lost new port cannot settle or steal the old live promises.
		CHECK(rig.Connect(DoCommandFlag::Execute).Failed()); CHECK(rig.Connect().Failed());
		CHECK(ComputeUtilityPowerDigest() == lost); CHECK(ComputeGeothermalGeneratorDigest() == lost_native);
		CHECK(Company::Get(_current_company)->money == lost_cash); CHECK(std::ranges::equal(lost_random.state, _random.state));
		CHECK(GetUtilityAllocation(original.source)->granted_energy_j == 85); CHECK(GetUtilityAllocation(original.destination)->granted_energy_j == 50);
		CHECK(GetUtilityPowerLink(original.links[0])->flow_j == 50); CHECK(GetUtilityPowerConsumer(original.source)->active);
		return;
	}
	REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded()); const auto after = CaptureUtilityPowerState();
	CHECK(after.next_consumer == before.next_consumer + 1); CHECK(after.next_link == before.next_link + 1);
	CHECK(after.next_share == before.next_share); CHECK(after.next_producer == before.next_producer); CHECK(after.operation_sequence == before.operation_sequence + 1);
	CHECK_FALSE(GetUtilityPowerConsumer(original.source)->active); CHECK(GetUtilityPowerConsumer(original.source)->linked_producers.empty());
	CHECK(GetUtilityPowerConsumer(original.source)->shares.front().released_j == 85);
	CHECK(GetUtilityPowerConsumer(original.destination)->shares.front().released_j == 50);
	CHECK(GetUtilityPowerProducer(old_output)->reserved_j == 0); CHECK(GetUtilityPowerProducer(old_output)->consumed_lifetime_j == 270);
	CHECK(GetUtilityPowerProducer(new_output)->consumed_lifetime_j == 0); CHECK(GetUtilityPowerLink(original.links[0])->flow_j == 50);
	CHECK_FALSE(GetUtilityPowerLink(original.links[0])->active); CHECK(GetUtilityPowerLink(proposed.links[0])->flow_j == 0);
	CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
	RunUtilityTownServices(); CHECK(GetUtilityPowerProducer(old_output)->consumed_lifetime_j == 270); CHECK(GetUtilityPowerProducer(new_output)->consumed_lifetime_j == 0);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	rig.Operate(); CHECK(GetUtilityPowerProducer(old_output)->consumed_lifetime_j == 270); CHECK(GetUtilityPowerProducer(new_output)->consumed_lifetime_j == 135);
	CHECK(GetUtilityPowerConsumer(original.destination)->town_effect_applied); CHECK(GetUtilityTownPowerBonus(rig.service.town) == 25);
	const auto complete = ComputeUtilityPowerDigest(); CHECK(rig.Connect().Failed()); CHECK(rig.Connect(DoCommandFlag::Execute).Failed()); CHECK(ComputeUtilityPowerDigest() == complete);
}

/** A real bound town without native paid ancestry and foreign/lost recovery requests cannot seize accounts, consume IDs or change settled history. */
TEST_CASE("Native geothermal recovery refuses unowned or unrelated retained rights", "[geothermal-service-recovery-refusal]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTownServiceRig rig;
	AutoRestoreBackup actor{_current_company};
	SECTION("ordinary effect-only town is not a former geothermal route") {
		const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::TownGrowth, 135, {}); REQUIRE(consumer.has_value());
		REQUIRE(BindUtilityTownConsumer(*consumer, rig.service.town) == UtilityPowerError::None);
	}
	SECTION("foreign actor cannot recover an owned service") {
		REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded()); rig.Operate(); _current_company = COMPANY_SPECTATOR;
	}
	SECTION("lost actual terminal cannot authorize recovery") {
		REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded()); rig.Operate(); rig.service.terminal = StationID::Invalid();
	}
	SECTION("demolished actual source cannot authorize recovery") {
		REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded()); rig.Operate();
		REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.plant_port).Succeeded());
	}
	const auto utility = ComputeUtilityPowerDigest(), native = ComputeGeothermalGeneratorDigest(); const auto random = _random;
	// Both synchronized phases must preserve the entire original state even
	// when refusal is discovered after the old town association was found.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(rig.Connect(flags).Failed()); CHECK(ComputeUtilityPowerDigest() == utility);
		CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(std::ranges::equal(random.state, _random.state));
	}
}
