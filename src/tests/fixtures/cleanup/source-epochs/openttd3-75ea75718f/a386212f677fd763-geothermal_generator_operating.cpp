/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_operating.cpp Real native powerhouse ports, common intervals and complete service/conversion boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8DC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_generator_operating_fixture.h"
#include "../geothermal_generator_wire.h"
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

/** Seventy-four real common-driver ticks charge exactly20 Money once, consume only actual thermal supply and retain original references. */
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
	PrepareTubePowerTick(); CHECK(ComputeUtilityPowerDigest() == digest); CHECK(ComputeGeothermalGeneratorDigest() == native);
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

/** Copied native operating transport rejects forged references/model bounds/billing carry before any candidate can acquire real rights. */
TEST_CASE("Native powerhouse operating references and billing transport are exact", "[geothermal-generator-operating-wire]")
{
	GeothermalGeneratorOperatingRig rig; rig.Operate(3);
	const auto state = *GetGeothermalGeneratorState(rig.generator); const auto utility = CaptureUtilityPowerState();
	const auto wire = EncodeGeothermalGenerator(state); REQUIRE(ValidateGeothermalGeneratorWire(wire)); CHECK(DecodeGeothermalGenerator(wire) == state);
	REQUIRE(ValidateGeothermalGeneratorOperatingReferences(state, utility));
	// Independent mutations exercise raw flags/fractions and actual immutable
	// shared identities, without changing the native world or repairing a copy.
	for (unsigned fault = 0; fault < 8; ++fault) {
		auto bad = wire;
		switch (fault) {
			case 0: bad.has_operating_tick = 2; break;
			case 1: bad.service_remainder = 74; break;
			case 2: ++bad.service_remainder; break;
			case 3: bad.active_service_ticks = 0; break;
			case 4: bad.heat_consumer = UINT32_MAX; break;
			case 5: bad.electrical_producer = UINT32_MAX; break;
			case 6: bad.heat_consumer = UINT32_MAX - 1; break;
			case 7: bad.electrical_producer = GetGeothermalCollectorState(rig.collector)->heat_producer.base(); break;
		}
		CHECK_FALSE((ValidateGeothermalGeneratorWire(bad) && ValidateGeothermalGeneratorOperatingReferences(DecodeGeothermalGenerator(bad), utility)));
	}
}
