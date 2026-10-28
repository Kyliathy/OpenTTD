/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_operating.cpp Real native powerhouse ports, explicit native component intervals and complete service/conversion boundaries; native component proofs remain separate from merged production scheduler Groups34-35.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8DC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_generator_operating_fixture.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** Unfinished paid machinery and unrelated electricity cannot activate a thermal domain or advance shared utility epochs before actual operating admission. */
TEST_CASE("Native inactive powerhouse does not activate unrelated thermal accounting", "[geothermal-generator-no-thermal-domain]")
{
	GeothermalGeneratorRig rig; ResetUtilityPowerState(); ResetTubePowerVehicles(); REQUIRE(rig.Fund().Succeeded());
	CHECK_FALSE(HasGeothermalGenerators()); const auto before = ComputeUtilityPowerDigest();
	{ AutoRestoreBackup pause{_pause_mode, PauseModes{}}; ++TimerGameTick::counter; PrepareTubePowerTick(); }
	CHECK(ComputeUtilityPowerDigest() == before); CHECK_FALSE(HasUtilityHeatState());
	RunGeothermalGenerators(GetUtilityPowerInterval(), 1); CHECK(ComputeUtilityPowerDigest() == before);
	ResetUtilityPowerState(); ResetTubePowerVehicles();
}

/** Pure native queries cannot register accounts; both paid installed domains are required before actual synchronized generation. */
TEST_CASE("Native powerhouse requires its actual installed boundary ports", "[geothermal-generator-ports]")
{
	GeothermalGeneratorOperatingRig rig{false, false};
	CHECK(GetGeothermalUtilityPortOwner(rig.plant_port) == std::optional<CompanyID>{_current_company});
	CHECK(GetGeothermalUtilityPortOwner(TileAddWrap(rig.plant_port, 1, 0)) == std::nullopt);
	const auto before = ComputeUtilityPowerDigest();
	CHECK_FALSE(PrepareGeothermalGeneratorDemand(rig.generator, 1).has_value());
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(CaptureUtilityPowerState().consumers.empty());
	rig.Operate(); CHECK(GetGeothermalGeneratorState(rig.generator)->heat_consumer.base() == UINT32_MAX);
	CHECK(CaptureUtilityPowerState().consumers.empty());
	rig.Build(rig.plant_port, UtilityEnergyDomain::Electricity); rig.Operate();
	const auto &state = *GetGeothermalGeneratorState(rig.generator);
	REQUIRE(state.electrical_producer.base() != UINT32_MAX); CHECK(state.active_service_ticks == 1);
	CHECK(GetUtilityPowerProducer(state.electrical_producer)->meter.generation_energy_j == 4050);
	CHECK(ValidateGeothermalGeneratorPhysicalReferences()); CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
}

/** Seventy-four explicit native component opportunities charge exactly20 Money once, consume only actual thermal supply and retain original references. */
TEST_CASE("Native powerhouse service and exact electricity share one common opportunity", "[geothermal-generator-service]")
{
	GeothermalGeneratorOperatingRig rig;
	const auto cash = Company::Get(_current_company)->money; rig.Operate(74);
	const auto state = *GetGeothermalGeneratorState(rig.generator);
	CHECK(state.active_service_ticks == 74); CHECK(state.service_remainder == 0);
	CHECK(Company::Get(_current_company)->money == cash - Money{20});
	const auto *output = GetUtilityPowerProducer(state.electrical_producer); REQUIRE(output != nullptr);
	CHECK(output->generated_lifetime_j == 74 * 4050); CHECK(output->conversion_remainder == 0);
	REQUIRE(output->conversion_origins.size() == 1); CHECK(output->conversion_origins.front().heat_j == 74 * 27000);
	const auto digest = ComputeUtilityPowerDigest(); const auto native = ComputeGeothermalGeneratorDigest();
	RunGeothermalGenerators(GetUtilityPowerInterval(), 1); CHECK(ComputeUtilityPowerDigest() == digest); CHECK(ComputeGeothermalGeneratorDigest() == native);
	CHECK(ConvertGeothermalGeneratorHeat(rig.generator, 1, 1).error != UtilityPowerError::None);
	CHECK(Company::Get(_current_company)->money == cash - Money{20}); CHECK(ComputeUtilityPowerDigest() == digest);
	CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Temporary missing cash or path and actual pause/date gates earn no active time, invoice, Heat use or conversion carry; repair resumes original identities. */
TEST_CASE("Native powerhouse interruptions preserve committed service and original energy", "[geothermal-generator-operating-waits]")
{
	GeothermalGeneratorOperatingRig rig; rig.Operate();
	const auto original = *GetGeothermalGeneratorState(rig.generator);
	const auto generated = GetUtilityPowerProducer(original.electrical_producer)->generated_lifetime_j;
	const auto consumed = GetUtilityPowerProducer(GetGeothermalCollectorState(rig.collector)->heat_producer)->consumed_lifetime_j;
	const auto cash = Company::Get(_current_company)->money;
	SECTION("unfunded complete service") { Company::Get(_current_company)->money = 0; rig.Operate(3); Company::Get(_current_company)->money = cash; }
	SECTION("actual pause") { AutoRestoreBackup paused{_pause_mode, PauseModes{PauseMode::Normal}}; rig.Operate(3); }
	SECTION("actual calendar lock") { AutoRestoreBackup date{TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31)}; rig.Operate(3); }
	SECTION("removed actual path") {
		const auto cell = rig.pipe[rig.pipe.size() / 2];
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, cell, std::make_tuple(cell, UtilityEnergyDomain::Heat)).Succeeded());
		rig.Operate(3); rig.Build(cell, UtilityEnergyDomain::Heat);
	}
	CHECK(*GetGeothermalGeneratorState(rig.generator) == original);
	CHECK(GetUtilityPowerProducer(original.electrical_producer)->generated_lifetime_j == generated);
	CHECK(GetUtilityPowerProducer(GetGeothermalCollectorState(rig.collector)->heat_producer)->consumed_lifetime_j == consumed);
	const auto before = Company::Get(_current_company)->money; rig.Operate();
	CHECK(GetGeothermalGeneratorState(rig.generator)->active_service_ticks == 2);
	CHECK(GetGeothermalGeneratorState(rig.generator)->heat_consumer == original.heat_consumer);
	CHECK(GetGeothermalGeneratorState(rig.generator)->electrical_producer == original.electrical_producer);
	CHECK(Company::Get(_current_company)->money == before);
}

/** A physically zero-rate field cannot earn service or conversion fractions; no numerical source substitutes for actual machinery supply. */
TEST_CASE("Native powerhouse zero physical supply accrues no service", "[geothermal-generator-zero-heat]")
{
	GeothermalGeneratorOperatingRig rig{false, true, 1};
	const auto cash = Company::Get(_current_company)->money;
	// At1 W, actual27-ms source integration produces no whole Heat in its first
	// three opportunities. The source millijoule carry alone retains that offer.
	rig.Operate(3); const auto &state = *GetGeothermalGeneratorState(rig.generator);
	CHECK(state.active_service_ticks == 0); CHECK(state.service_remainder == 0); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(GetUtilityPowerProducer(state.electrical_producer)->conversion_remainder == 0);
	// Native source millijoules eventually yield exactly one whole Heat joule;
	// only that positive committed interval earns service and conversion carry.
	rig.Operate(35); CHECK(state.active_service_ticks == 1); CHECK(state.service_remainder == 20);
	CHECK(GetUtilityPowerProducer(state.electrical_producer)->conversion_remainder == 15);
	CHECK(GetUtilityPowerProducer(state.electrical_producer)->generated_lifetime_j == 0);
	CHECK(Company::Get(_current_company)->money == cash);
}

/** Actual demolition retires the original typed pair and preserves paid Heat/output history while a reused native pool ID receives no old rights. */
TEST_CASE("Native operating powerhouse demolition retains retired utility history", "[geothermal-generator-operating-removal]")
{
	GeothermalGeneratorOperatingRig rig; rig.Operate(4);
	const auto state = *GetGeothermalGeneratorState(rig.generator);
	const auto generated = GetUtilityPowerProducer(state.electrical_producer)->generated_lifetime_j;
	const auto cash = Company::Get(_current_company)->money;
	REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, rig.plant_port, std::make_tuple(rig.plant_port)).Succeeded());
	CHECK(GetGeothermalGeneratorState(rig.generator) == nullptr);
	CHECK_FALSE(GetUtilityPowerConsumer(state.heat_consumer)->active); CHECK_FALSE(GetUtilityPowerProducer(state.electrical_producer)->active);
	CHECK(GetUtilityPowerProducer(state.electrical_producer)->generated_lifetime_j == generated);
	CHECK(Company::Get(_current_company)->money <= cash);
	REQUIRE(Industry::CanAllocateItem()); Industry *replacement = Industry::Create(); CHECK(replacement->index == rig.generator);
	CHECK(GetGeothermalGeneratorState(replacement->index) == nullptr); CHECK(GetGeothermalGeneratorPort(replacement->index) == INVALID_TILE);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Actual takeover preserves original service/carry/history while native company loss permanently revokes both utility identities. */
TEST_CASE("Native operating powerhouse follows actual company lifecycle", "[geothermal-generator-operating-company]")
{
	GeothermalGeneratorOperatingRig rig; rig.Operate(3);
	const auto original = *GetGeothermalGeneratorState(rig.generator);
	SECTION("surviving takeover") {
		REQUIRE(Company::CanAllocateItem()); Company *successor = Company::Create(); successor->money = 1000000;
		ChangeOwnershipOfCompanyItems(original.owner, successor->index);
		const auto *state = GetGeothermalGeneratorState(rig.generator); REQUIRE(state != nullptr);
		CHECK(state->owner == successor->index); CHECK(state->heat_consumer == original.heat_consumer);
		CHECK(state->electrical_producer == original.electrical_producer); CHECK(state->service_remainder == 60);
		const auto cash = successor->money; rig.Operate();
		CHECK(GetGeothermalGeneratorState(rig.generator)->active_service_ticks == 4);
		CHECK(GetGeothermalGeneratorState(rig.generator)->service_remainder == 6);
		CHECK(successor->money == cash - Money{1}); CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
	}
	SECTION("actual company abandonment") {
		AutoRestoreBackup spectator{_local_company, COMPANY_SPECTATOR};
		ChangeOwnershipOfCompanyItems(original.owner, INVALID_OWNER);
		const auto state = *GetGeothermalGeneratorState(rig.generator);
		CHECK(state.owner == CompanyID::Invalid()); CHECK(state.service_remainder == 60);
		CHECK_FALSE(GetUtilityPowerProducer(state.electrical_producer)->active); CHECK_FALSE(GetUtilityPowerConsumer(state.heat_consumer)->active);
		const auto history = GetUtilityPowerProducer(state.electrical_producer)->generated_lifetime_j;
		rig.Operate(3); CHECK(*GetGeothermalGeneratorState(rig.generator) == state);
		CHECK(GetUtilityPowerProducer(state.electrical_producer)->generated_lifetime_j == history);
		CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
	}
}


/** A restored partial native binding completes its missing output without acquiring a second original Heat consumer. */
TEST_CASE("Native generator resumes its original registered input before output acquisition", "[geothermal-generator-registration-retry]")
{
	GeothermalGeneratorOperatingRig rig; const auto *terms = GetMultiLayerGeothermalGeneratorTerms(GeothermalGeneratorModel::VentTap);
	const auto capacity = IntegrateUtilityGeneration(terms->thermal_capacity_w, MAX_UTILITY_INTERVAL_TICKS, 0); REQUIRE(capacity.has_value());
	PublishGeothermalHeatSources(GetUtilityPowerInterval(), 1);
	const auto input = RegisterGeothermalHeatConsumer(_current_company, rig.plant_port, UtilityHeatUse::ElectricalGenerator, capacity->energy_j);
	REQUIRE(input.has_value()); auto partial = CaptureGeothermalGenerators(); REQUIRE(partial.size() == 1); partial.front().heat_consumer = *input;
	// This is explicit interrupted-file component input over actual native
	// machinery and a real issued port, not permission to rewrite a live pair.
	ResetGeothermalGenerators(); StageLoadedGeothermalGenerators(partial); REQUIRE(AcceptLoadedGeothermalGenerators());
	const auto next_consumer = CaptureUtilityPowerState().next_consumer; rig.Operate();
	const auto *state = GetGeothermalGeneratorState(rig.generator); REQUIRE(state != nullptr);
	CHECK(state->heat_consumer == *input); CHECK(state->electrical_producer.base() != UINT32_MAX);
	CHECK(CaptureUtilityPowerState().next_consumer == next_consumer); CHECK(state->active_service_ticks == 1);
	CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators())); CHECK(ValidateGeothermalGeneratorPhysicalReferences());
}

/** Exhausted original producer capacity retains its one pending input and refuses an invoice on every later opportunity. */
TEST_CASE("Native generator retains pending input while original output capacity is exhausted", "[geothermal-generator-registration-capacity]")
{
	GeothermalGeneratorOperatingRig rig;
	// These are explicit unused numerical external identities, not supplying
	// energy or replacing the actual native collector. Tombstones are never
	// removed or reused to create room for this test's missing output.
	for (size_t index = CaptureUtilityPowerState().producers.size(); index < MAX_UTILITY_INTERVAL_ACCOUNTS; ++index) {
		REQUIRE(RegisterUtilityPowerProducer(_current_company, MapLayer::DeepUnderground, UtilityGenerationKind::External).has_value());
	}
	const auto cash = Company::Get(_current_company)->money; rig.Operate();
	const auto pending = *GetGeothermalGeneratorState(rig.generator); REQUIRE(pending.heat_consumer.base() != UINT32_MAX);
	CHECK(pending.electrical_producer.base() == UINT32_MAX); CHECK(pending.active_service_ticks == 0);
	const auto next_consumer = CaptureUtilityPowerState().next_consumer; rig.Operate();
	CHECK(*GetGeothermalGeneratorState(rig.generator) == pending); CHECK(CaptureUtilityPowerState().next_consumer == next_consumer);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
}

/** Numerical orbital-only electricity cannot initialize Heat merely by passing the new native generator driver. */
TEST_CASE("Native generator driver leaves unrelated orbital-only utility state unchanged", "[geothermal-generator-orbital-noop]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded());
	const auto orbital = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::OrbitalPanel); REQUIRE(orbital.has_value());
	REQUIRE(PublishUtilityGeneration(*orbital, GetUtilityPowerInterval(), 1000, 1) == UtilityPowerError::None);
	const auto before = ComputeUtilityPowerDigest(); RunGeothermalGenerators(GetUtilityPowerInterval(), 1);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK_FALSE(HasUtilityHeatState()); CHECK_FALSE(IsUtilityHeatAllocated());
}

/** Coupled operating transport resolves original roles/carry without any wire-format interpretation or repair. */
TEST_CASE("Native generator operating references refuse changed billing and shared roles", "[geothermal-generator-operating-references]")
{
	GeothermalGeneratorOperatingRig rig; rig.Operate(3);
	const auto state = *GetGeothermalGeneratorState(rig.generator); const auto utility = CaptureUtilityPowerState();
	REQUIRE(ValidateGeothermalGeneratorOperatingReferences(state, utility)); const auto before = ComputeGeothermalGeneratorDigest();
	// Each independent fault changes one original billing/role relationship;
	// no malformed candidate is installed into the native machine or ledger.
	for (unsigned fault = 0; fault < 7; ++fault) {
		auto bad = state;
		switch (fault) {
			case 0: bad.service_remainder = 74; break;
			case 1: ++bad.service_remainder; break;
			case 2: bad.active_service_ticks = 0; break;
			case 3: bad.heat_consumer = UtilityConsumerID{UINT32_MAX}; break;
			case 4: bad.electrical_producer = UtilityProducerID{UINT32_MAX}; break;
			case 5: bad.heat_consumer = UtilityConsumerID{UINT32_MAX - 1}; break;
			case 6: bad.electrical_producer = GetGeothermalCollectorState(rig.collector)->heat_producer; break;
		}
		CHECK_FALSE(ValidateGeothermalGeneratorOperatingReferences(bad, utility));
	}
	CHECK(ComputeGeothermalGeneratorDigest() == before);
}

/** The dated Magma model settles its own physical35-percent output and35-Money service bill through the same original coupled owners. */
TEST_CASE("Native Magma service preserves its paid ratio invoice and calendar lock", "[geothermal-generator-magma-service]")
{
	GeothermalGeneratorOperatingRig rig{false, true, 1000000, GeothermalGeneratorModel::MagmaLoop};
	const auto cash = Company::Get(_current_company)->money; rig.Operate(74);
	const auto state = *GetGeothermalGeneratorState(rig.generator); const auto *output = GetUtilityPowerProducer(state.electrical_producer); REQUIRE(output != nullptr);
	CHECK(state.work_ticks == 888); CHECK(state.active_service_ticks == 74); CHECK(state.service_remainder == 0);
	CHECK(Company::Get(_current_company)->money == cash - Money{35}); CHECK(output->generated_lifetime_j == 74 * 9450);
	CHECK(output->conversion_efficiency == 35); CHECK(output->conversion_capacity_w == 350000); CHECK(output->conversion_remainder == 0);
	REQUIRE(output->conversion_origins.size() == 1); CHECK(output->conversion_origins.front().heat_j == 74 * 27000);
	{
		AutoRestoreBackup date{TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 11, 31)};
		rig.Operate(3); CHECK(*GetGeothermalGeneratorState(rig.generator) == state);
		CHECK(GetUtilityPowerProducer(state.electrical_producer)->generated_lifetime_j == 74 * 9450);
		CHECK(Company::Get(_current_company)->money == cash - Money{35});
	}
	rig.Operate(); CHECK(GetGeothermalGeneratorState(rig.generator)->active_service_ticks == 75);
	CHECK(GetGeothermalGeneratorState(rig.generator)->service_remainder == 35);
	CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators())); CHECK(ValidateGeothermalGeneratorPhysicalReferences());
}
