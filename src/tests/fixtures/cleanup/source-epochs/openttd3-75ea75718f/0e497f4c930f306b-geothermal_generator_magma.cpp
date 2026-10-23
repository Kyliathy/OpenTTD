/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_magma.cpp Actual dated Magma machinery, paid work/service and two-model native contention.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8E/14B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md Group19.
 * Prepared floor and explicit receiver-boundary materials leave the complete paid-TBM/carrier bootstrap to8F.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_generator_operating_fixture.h"
#include "../industry_map.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** The actual2000 cold claim, command, capability and native stock/work share one canonical dated model. */
TEST_CASE("Magma native construction preserves its exact2000 gate and higher paid bill", "[geothermal-magma-construction]")
{
	GeothermalGeneratorRig rig{false, GeothermalGeneratorModel::MagmaLoop};
	const auto type = GetGeothermalGeneratorIndustryType(GeothermalGeneratorModel::MagmaLoop);
	const auto *terms = GetMultiLayerGeothermalConstruction(rig.request.kind); REQUIRE(terms != nullptr);
	CHECK(terms->kind == MultiLayerGeothermalConstructionKind::MagmaPowerhouse);
	CHECK(terms->capital_cost == 360000); CHECK(terms->work_ticks == 888);
	CHECK(GetGeothermalGeneratorIndustryCapability(type)->asset_id == "facility.magma-powerhouse");
	const auto cash = Company::Get(_current_company)->money;
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 11, 31);
	CHECK(PrepareGeothermalSite(rig.request).error == GeothermalError::Unavailable);
	CHECK(ClaimGeothermalSite(rig.request).error == GeothermalError::Unavailable);
	CHECK(CheckLayerIndustryConstruction(rig.request.tile, type).Failed()); CHECK(rig.Fund().Failed());
	CHECK(GetGeothermalGeneratorIndustryType(GeothermalGeneratorModel::MagmaLoop) == type);
	CHECK(Industry::GetNumItems() == 0); CHECK(Company::Get(_current_company)->money == cash);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2000}, 0, 1);
	CHECK(CheckLayerIndustryConstruction(rig.request.tile, type).Succeeded());
	CHECK(CheckLayerIndustryConstruction(MultiLayerMap::ProjectToSurface(rig.request.tile), type).Failed());
	CHECK(CmdBuildIndustry({}, rig.request.tile, type, 0, true, 123).Failed());
	REQUIRE(rig.Fund(true).Succeeded()); CHECK(Company::Get(_current_company)->money == cash);
	Company::Get(_current_company)->money = 359999; CHECK(rig.Fund().Failed()); CHECK(Industry::GetNumItems() == 0);
	Company::Get(_current_company)->money = cash; REQUIRE(rig.Fund().Succeeded());
	CHECK(Company::Get(_current_company)->money == cash - Money{360000});
	CHECK(rig.Generator().type == type); CHECK(rig.Generator().location.w == 4); CHECK(rig.Generator().location.h == 4);
	CHECK(rig.Deliver(CargoLabel{"STEL"}, 25) == 24); CHECK(rig.Deliver(CargoLabel{"RFMT"}, 16) == 16);
	CHECK(rig.Deliver(CargoLabel{"ELEC"}, 8) == 8); rig.Tick(100);
	CHECK(GetGeothermalGeneratorState(rig.Generator().index)->work_ticks == 0);
	CHECK(rig.Deliver(CargoLabel{"BATY"}, 4) == 4); rig.Tick(887);
	CHECK_FALSE(IsGeothermalGeneratorConstructionComplete(rig.Generator().index)); rig.Tick(1);
	CHECK(IsGeothermalGeneratorConstructionComplete(rig.Generator().index));
	CHECK(GetGeothermalGeneratorState(rig.Generator().index)->model == GeothermalGeneratorModel::MagmaLoop);
	CHECK(Company::Get(_current_company)->money == cash - Money{360000}); CHECK(CaptureUtilityPowerState().producers.empty());
}

/** Calendar/profile locks suspend actual arrived materials and prepaid labour without rewriting retained machinery. */
TEST_CASE("Magma construction pauses under older calendar or incompatible profile", "[geothermal-magma-locks]")
{
	GeothermalGeneratorRig rig{false, GeothermalGeneratorModel::MagmaLoop}; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill();
	const auto original = ComputeGeothermalGeneratorDigest(); const auto cash = Company::Get(_current_company)->money;
	const auto profile = GetCurrentMultiLayerProfileSelection();
	SECTION("1999 calendar") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 11, 31); }
	SECTION("profile deactivated") { ResetMultiLayerProfileSelection(); }
	rig.Tick(3); CHECK(ComputeGeothermalGeneratorDigest() == original); CHECK(Company::Get(_current_company)->money == cash);
	const auto type = GetGeothermalGeneratorIndustryType(GeothermalGeneratorModel::MagmaLoop);
	CHECK(CheckLayerIndustryConstruction(rig.request.tile, type).Failed());
	CHECK(CheckLayerIndustryConstruction(MultiLayerMap::ProjectToSurface(rig.request.tile), type).Failed());
	CHECK(rig.Deliver(CargoLabel{"STEL"}, 1) == 0); CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
	REQUIRE(RestoreMultiLayerProfileSelection(profile, LandscapeType::Temperate, rig.ActivatedContent()) == MultiLayerProfileError::None);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2000}, 0, 1);
	rig.Tick(888); CHECK(IsGeothermalGeneratorConstructionComplete(rig.Generator().index)); CHECK(Company::Get(_current_company)->money == cash);
}

/** One actual native common opportunity consumes one original Heat budget and charges the advanced model's exact active service. */
TEST_CASE("Magma common driver converts35percent and charges35 per74 active ticks", "[geothermal-magma-service]")
{
	GeothermalGeneratorOperatingRig rig{false, true, 1000000, GeothermalGeneratorModel::MagmaLoop};
	const auto cash = Company::Get(_current_company)->money; rig.Operate(74);
	const auto state = *GetGeothermalGeneratorState(rig.generator); const auto *output = GetUtilityPowerProducer(state.electrical_producer);
	REQUIRE(output != nullptr); CHECK(state.work_ticks == 888); CHECK(state.active_service_ticks == 74); CHECK(state.service_remainder == 0);
	CHECK(Company::Get(_current_company)->money == cash - Money{35}); CHECK(output->generated_lifetime_j == 74 * 9450);
	CHECK(output->conversion_efficiency == 35); CHECK(output->conversion_capacity_w == 350000); CHECK(output->conversion_remainder == 0);
	REQUIRE(output->conversion_origins.size() == 1); CHECK(output->conversion_origins.front().heat_j == 74 * 27000);
	const auto digest = ComputeUtilityPowerDigest(); PrepareTubePowerTick(); CHECK(ComputeUtilityPowerDigest() == digest);
	const auto original = *GetGeothermalGeneratorState(rig.generator);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 11, 31);
	rig.Operate(3); CHECK(*GetGeothermalGeneratorState(rig.generator) == original); CHECK(GetUtilityPowerProducer(state.electrical_producer)->generated_lifetime_j == 74 * 9450);
	CHECK(Company::Get(_current_company)->money == cash - Money{35}); CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2000}, 0, 1); rig.Operate();
	CHECK(GetGeothermalGeneratorState(rig.generator)->active_service_ticks == 75);
}

/** Fractional positive Heat earns the advanced numerator/service once; zero Heat and missing complete service funds earn neither. */
TEST_CASE("Magma retains exact fractional conversion and refuses unfunded service", "[geothermal-magma-fractions]")
{
	GeothermalGeneratorOperatingRig rig{false, true, 1, GeothermalGeneratorModel::MagmaLoop};
	const auto cash = Company::Get(_current_company)->money; rig.Operate(38);
	const auto state = *GetGeothermalGeneratorState(rig.generator); REQUIRE(state.active_service_ticks == 1);
	CHECK(state.service_remainder == 35); const auto *output = GetUtilityPowerProducer(state.electrical_producer); REQUIRE(output != nullptr);
	CHECK(output->conversion_remainder == 35); CHECK(output->generated_lifetime_j == 0); CHECK(Company::Get(_current_company)->money == cash);
	Company::Get(_current_company)->money = 0; rig.Operate(74);
	CHECK(*GetGeothermalGeneratorState(rig.generator) == state);
	output = GetUtilityPowerProducer(state.electrical_producer); CHECK(output->conversion_remainder == 35); CHECK(output->generated_lifetime_j == 0);
	Company::Get(_current_company)->money = cash; rig.Operate(74);
	CHECK(GetGeothermalGeneratorState(rig.generator)->active_service_ticks == 3);
	CHECK(GetGeothermalGeneratorState(rig.generator)->service_remainder == 31);
	output = GetUtilityPowerProducer(state.electrical_producer); CHECK(output->generated_lifetime_j == 1); CHECK(output->conversion_remainder == 5);
	CHECK(Company::Get(_current_company)->money == cash - Money{1});
}

/** Actual takeover and demolition preserve the advanced model's original shared quantity/carry and revoke its old typed rights on removal. */
TEST_CASE("Magma lifecycle preserves identity and retires the original conversion", "[geothermal-magma-lifecycle]")
{
	GeothermalGeneratorOperatingRig rig{false, true, 100, GeothermalGeneratorModel::MagmaLoop}; rig.Operate(3);
	const auto original = *GetGeothermalGeneratorState(rig.generator);
	CHECK(original.model == GeothermalGeneratorModel::MagmaLoop); CHECK(original.service_remainder == 31);
	SECTION("takeover") {
		REQUIRE(Company::CanAllocateItem()); auto *successor = Company::Create(); successor->money = 1000000;
		ChangeOwnershipOfCompanyItems(original.owner, successor->index); rig.Operate();
		const auto *state = GetGeothermalGeneratorState(rig.generator); REQUIRE(state != nullptr);
		CHECK(state->owner == successor->index); CHECK(state->model == original.model); CHECK(state->heat_consumer == original.heat_consumer);
		CHECK(state->electrical_producer == original.electrical_producer); CHECK(state->service_remainder == 66);
		CHECK(GetUtilityPowerProducer(state->electrical_producer)->conversion_remainder == 50);
	}
	SECTION("demolition and native ID reuse") {
		REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, rig.plant_port, std::make_tuple(rig.plant_port)).Succeeded());
		CHECK(GetGeothermalGeneratorState(rig.generator) == nullptr); CHECK_FALSE(GetUtilityPowerProducer(original.electrical_producer)->active);
		CHECK_FALSE(GetUtilityPowerConsumer(original.heat_consumer)->active); CHECK(GetUtilityPowerProducer(original.electrical_producer)->conversion_remainder == 80);
		REQUIRE(Industry::CanAllocateItem()); auto *replacement = Industry::Create(); CHECK(replacement->index == rig.generator);
		CHECK(GetGeothermalGeneratorState(replacement->index) == nullptr);
	}
	CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** @param rig Actual first generator/collector/path world. @param model The other canonical dated model. @return A second separately paid native machine on explicit prepared floor, with a purchased branch from the sole original source. */
static IndustryID BuildOtherGenerator(GeothermalGeneratorOperatingRig &rig, GeothermalGeneratorModel model)
{
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2000}, 0, 1);
	const auto *operating = GetMultiLayerGeothermalGeneratorTerms(model); REQUIRE(operating != nullptr);
	const auto *terms = GetMultiLayerGeothermalConstruction(operating->construction);
	GeothermalSiteRequest request = rig.request; request.kind = operating->construction; request.tile = request.endpoint = TileXYL(44, 24, MapLayer::DeepUnderground);
	std::vector<TileIndex> floor;
	// The second original native building gets its own full explicit floor/rim;
	// the shared collector and first machinery are neither replaced nor copied.
	for (int y = -1; y <= terms->height; ++y) {
		for (int x = -1; x <= terms->width; ++x) floor.push_back(TileAddWrap(request.tile, x, y));
	}
	rig.Open(floor); const auto claim = ClaimGeothermalSite(request); REQUIRE(claim.error == GeothermalError::None);
	const auto cash = Company::Get(_current_company)->money;
	REQUIRE(Command<Commands::BuildGeothermalGenerator>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, request.tile,
		std::make_tuple(request.tile, claim.id, uint32_t{123})).Succeeded());
	CHECK(Company::Get(_current_company)->money == cash - Money{terms->capital_cost});
	IndustryID result = IndustryID::Invalid();
	for (const auto &state : CaptureGeothermalGenerators()) { if (state.site == claim.id) result = state.industry; } // Resolve only the newly consumed original blueprint, never an assumed pool slot.
	REQUIRE(Industry::IsValidID(result));
	for (const auto &input : terms->inputs) { // Each independent native accepted slot owns this second bill, without sharing the first building's receipt.
		REQUIRE(ReceiveGeothermalGeneratorCargo(*Industry::Get(result), GetCargoTypeByLabel(input.label), input.quantity) == input.quantity);
	}
	rig.Tick(static_cast<uint16_t>(terms->work_ticks)); REQUIRE(IsGeothermalGeneratorConstructionComplete(result));
	std::vector<TileIndex> branch;
	// Follow the already open outside row to the second original port. Existing
	// paid cells are reused, while every new cell pays its own canonical invoice.
	for (uint x = TileX(rig.plant_port); x <= TileX(request.tile); ++x) branch.push_back(TileXYL(x, 23, MapLayer::DeepUnderground));
	branch.push_back(request.endpoint); rig.Open(branch);
	for (const auto tile : branch) { if (!GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Heat).has_value()) rig.Build(tile, UtilityEnergyDomain::Heat); }
	rig.Build(request.endpoint, UtilityEnergyDomain::Electricity); return result;
}

/** Both actual dated generators compete on the same paid1-MW physical source; the later consumer cannot receive another whole vent. */
TEST_CASE("Both native generator models share one actual original vent budget", "[geothermal-magma-contention]")
{
	const auto first_model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	GeothermalGeneratorOperatingRig rig{false, true, 1000000, first_model};
	const auto other_model = first_model == GeothermalGeneratorModel::VentTap ? GeothermalGeneratorModel::MagmaLoop : GeothermalGeneratorModel::VentTap;
	const auto other = BuildOtherGenerator(rig, other_model); const auto cash = Company::Get(_current_company)->money; rig.Operate();
	const auto first = *GetGeothermalGeneratorState(rig.generator), second = *GetGeothermalGeneratorState(other);
	REQUIRE(first.heat_consumer < second.heat_consumer); CHECK(first.active_service_ticks == 1); CHECK(second.active_service_ticks == 0);
	const auto *output = GetUtilityPowerProducer(first.electrical_producer), *refused = GetUtilityPowerProducer(second.electrical_producer);
	REQUIRE(output != nullptr); REQUIRE(refused != nullptr);
	CHECK(output->generated_lifetime_j == (first_model == GeothermalGeneratorModel::VentTap ? 4050 : 9450)); CHECK(refused->generated_lifetime_j == 0);
	CHECK(refused->conversion_remainder == 0); CHECK(second.service_remainder == 0); CHECK(Company::Get(_current_company)->money == cash);
	const auto *source = GetUtilityPowerProducer(GetGeothermalCollectorState(rig.collector)->heat_producer); REQUIRE(source != nullptr);
	CHECK(source->generated_lifetime_j == 27000); CHECK(source->consumed_lifetime_j == 27000);
	CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Demolishing the earlier claimant lets the surviving model use fresh original-source intervals; retired typed references keep only their settled history. */
TEST_CASE("Removed native generator cannot retain a shared vent allocation", "[geothermal-magma-contention-loss]")
{
	const auto first_model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	GeothermalGeneratorOperatingRig rig{false, true, 1000000, first_model};
	const auto other_model = first_model == GeothermalGeneratorModel::VentTap ? GeothermalGeneratorModel::MagmaLoop : GeothermalGeneratorModel::VentTap;
	const auto other = BuildOtherGenerator(rig, other_model); rig.Operate();
	const auto removed = *GetGeothermalGeneratorState(rig.generator), survivor = *GetGeothermalGeneratorState(other);
	REQUIRE(removed.active_service_ticks == 1); REQUIRE(survivor.active_service_ticks == 0);
	const auto source_id = GetGeothermalCollectorState(rig.collector)->heat_producer;
	const auto generated = GetUtilityPowerProducer(removed.electrical_producer)->generated_lifetime_j;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false,
		rig.plant_port, std::make_tuple(rig.plant_port)).Succeeded());
	CHECK(GetGeothermalGeneratorState(rig.generator) == nullptr);
	CHECK_FALSE(GetUtilityPowerConsumer(removed.heat_consumer)->active);
	CHECK_FALSE(GetUtilityPowerProducer(removed.electrical_producer)->active);
	rig.Operate(2);
	const auto resumed = *GetGeothermalGeneratorState(other);
	CHECK(resumed.heat_consumer == survivor.heat_consumer); CHECK(resumed.electrical_producer == survivor.electrical_producer);
	CHECK(resumed.active_service_ticks == 2);
	const auto *output = GetUtilityPowerProducer(resumed.electrical_producer); REQUIRE(output != nullptr);
	CHECK(output->generated_lifetime_j == 2 * (other_model == GeothermalGeneratorModel::VentTap ? 4050 : 9450));
	REQUIRE(output->conversion_origins.size() == 1); CHECK(output->conversion_origins.front().heat_j == 54000);
	CHECK(GetUtilityPowerProducer(source_id)->generated_lifetime_j == 81000);
	CHECK(GetUtilityPowerProducer(source_id)->consumed_lifetime_j == 81000);
	CHECK(GetUtilityPowerProducer(removed.electrical_producer)->generated_lifetime_j == generated);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}
