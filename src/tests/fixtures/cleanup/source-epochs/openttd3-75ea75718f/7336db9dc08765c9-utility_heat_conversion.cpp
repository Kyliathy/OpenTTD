/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_heat_conversion.cpp Exact shared Heat/electrical transaction and real company expense boundary; no physical powerhouse claim.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8DB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "utility_heat_conversion_fixture.h"
#include "../multilayer_state.h"

/** Exact electrical sources have one original typed input and cannot be created or supplied through ordinary rate APIs. */
TEST_CASE("Shared conversion registers one exact output without granting energy", "[utility-conversion-definition]")
{
	UtilityConversionRig rig; const auto before = ComputeUtilityPowerDigest();
	CHECK_FALSE(RegisterUtilityHeatConversionProducer(_current_company, rig.input, 15, 150000).has_value());
	CHECK_FALSE(RegisterUtilityPowerProducer(_current_company, MapLayer::DeepUnderground, UtilityGenerationKind::HeatConversion).has_value());
	CHECK(PublishUtilityGeneration(rig.output, GetUtilityPowerInterval(), 150000, 1) == UtilityPowerError::Refused);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(GetUtilityPowerProducer(rig.output)->generated_lifetime_j == 0);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	const auto schema = CaptureMultiLayerStateSchema(); CHECK((schema.feature_flags & MULTILAYER_FEATURE_HEAT_CONVERSIONS) != 0);
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::None);
	// The metadata marker cannot retain output/provenance while omitting
	// either of the original shared quantity domains it necessarily depends on.
	for (uint32_t dependency : {MULTILAYER_FEATURE_UTILITY_POWER, MULTILAYER_FEATURE_UTILITY_HEAT}) {
		auto missing = schema; missing.feature_flags &= ~dependency;
		CHECK(ValidateMultiLayerStateReferences(missing) == MultiLayerStateError::UnsupportedFeatures);
	}
}

/** Pure preparation and complete success settle original Heat, actual cash, exact electrical output and historical source attribution together. */
TEST_CASE("Shared conversion commits whole actual input and exact electrical publication once", "[utility-conversion-atomic]")
{
	UtilityConversionRig rig; rig.Allocate(27000); UtilityConversionBenefit benefit{_current_company};
	const auto before = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	const auto plan = rig.Prepare(27000); REQUIRE(plan.error == UtilityPowerError::None);
	CHECK(plan.electric_j == 4050); CHECK(plan.loss_j == 22950); CHECK(plan.remainder_after == 0);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(Company::Get(_current_company)->money == cash);
	const auto done = CommitUtilityHeatConversion(plan, benefit); REQUIRE(done.error == UtilityPowerError::None);
	CHECK(done.heat_consumed_j == 27000); CHECK(done.electricity_generated_j == 4050); CHECK(done.loss_j == 22950);
	CHECK(Company::Get(_current_company)->money == cash - Money{5}); CHECK(benefit.completed_heat == 27000);
	const auto *source = GetUtilityPowerProducer(rig.output); REQUIRE(source != nullptr);
	CHECK(source->meter.generation_energy_j == 4050); CHECK(source->conversion_origins == std::vector{UtilityHeatConversionOrigin{rig.heat, 27000}});
	CHECK(GetUtilityPowerProducer(rig.heat)->reserved_j == 0); CHECK(GetUtilityPowerProducer(rig.heat)->consumed_lifetime_j == 27000);
	const auto after = ComputeUtilityPowerDigest(); CHECK(CommitUtilityHeatConversion(plan, benefit).error == UtilityPowerError::StaleMutation);
	CHECK(ComputeUtilityPowerDigest() == after); CHECK(benefit.completed_heat == 27000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** One-joule thermal intervals exercise exact output that integer watts cannot publish; only the fractional numerator crosses ordinary interval rotation. */
TEST_CASE("Shared conversion retains fractional percent across exact tiny intervals", "[utility-conversion-fraction]")
{
	UtilityConversionRig rig; UtilityConversionBenefit benefit{_current_company}; const auto cash = Company::Get(_current_company)->money;
	// Repeated real owner rotations preserve spent original Heat and ratio
	// history, while every previous whole output offer expires independently.
	for (uint tick = 0; tick < 7; ++tick) {
		rig.Allocate(1); const auto plan = rig.Prepare(1); REQUIRE(plan.error == UtilityPowerError::None);
		REQUIRE(CommitUtilityHeatConversion(plan, benefit).error == UtilityPowerError::None);
		CHECK(plan.electric_j == (tick == 6 ? 1 : 0));
		CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
		if (tick < 6) rig.Rotate();
	}
	CHECK(GetUtilityPowerProducer(rig.output)->generated_lifetime_j == 1); CHECK(GetUtilityPowerProducer(rig.output)->conversion_remainder == 5);
	CHECK(GetUtilityPowerProducer(rig.heat)->consumed_lifetime_j == 7); CHECK(Company::Get(_current_company)->money == cash - Money{35});
}

/** Complete invoice refusal at validation or commit leaves both quantity domains, fractional carry and actual company money untouched. */
TEST_CASE("Shared conversion refuses partial benefit and invalid retry tickets unchanged", "[utility-conversion-refusal]")
{
	UtilityConversionRig rig; rig.Allocate(7); UtilityConversionBenefit benefit{_current_company}; auto plan = rig.Prepare(7);
	REQUIRE(plan.error == UtilityPowerError::None);
	SECTION("validation refuses") { benefit.admit = false; }
	SECTION("commit refuses unchanged") { benefit.accept = false; }
	SECTION("actual invoice cannot be paid") { Company::Get(_current_company)->money = 4; }
	SECTION("modified result") { ++plan.electric_j; }
	SECTION("modified duration") { ++plan.request.economy_ticks; }
	SECTION("modified input") { ++plan.request.heat_j; }
	SECTION("unavailable remaining grant") { plan = rig.Prepare(8); }
	SECTION("zero original heat") { plan = rig.Prepare(0); }
	SECTION("wrong actual benefit role") { benefit.role = UtilityHeatUse::DirectRecipe; }
	SECTION("zero committed service duration") { auto request = plan.request; request.economy_ticks = 0; plan = PrepareUtilityHeatConversion(request); }
	SECTION("electrical phase already closed") {
		{ AutoRestoreBackup system{_current_company, OWNER_DEITY}; REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), {}) == UtilityPowerError::None); }
		plan = rig.Prepare(7); CHECK(plan.error == UtilityPowerError::AlreadyAllocated);
	}
	const auto before = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	CHECK(CommitUtilityHeatConversion(plan, benefit).error != UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(Company::Get(_current_company)->money == cash); CHECK(benefit.completed_heat == 0);
}

/** The later35-percent numerical model uses the same shared transaction; physical2000 availability remains the later native model owner's contract. */
TEST_CASE("Shared conversion applies the advanced ratio without another quantity owner", "[utility-conversion-advanced]")
{
	UtilityConversionRig rig{35}; rig.Allocate(27000); UtilityConversionBenefit benefit{_current_company};
	const auto plan = rig.Prepare(27000); REQUIRE(plan.error == UtilityPowerError::None);
	CHECK(plan.electric_j == 9450); CHECK(plan.loss_j == 17550); CHECK(plan.remainder_after == 0);
	REQUIRE(CommitUtilityHeatConversion(plan, benefit).error == UtilityPowerError::None);
	CHECK(GetUtilityPowerProducer(rig.output)->conversion_efficiency == 35);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Exact partial use retains every original thermal source's actual historical contribution, independently of the output's electrical source identity. */
TEST_CASE("Shared conversion attributes a mixed original thermal grant exactly", "[utility-conversion-origins]")
{
	ExcavationFixture world; UtilityPowerReset ledger;
	const auto first = RegisterUtilityHeatProducer(_current_company), second = RegisterUtilityHeatProducer(_current_company);
	REQUIRE(first.has_value()); REQUIRE(second.has_value());
	const auto input = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::ElectricalGenerator, 27000, std::array{*first, *second});
	REQUIRE(input.has_value()); const auto output = RegisterUtilityHeatConversionProducer(_current_company, *input, 15, 150000); REQUIRE(output.has_value());
	// These actual bounded rate publications yield3 and4 whole thermal joules;
	// their separate millijoule carries remain with their original Heat sources.
	REQUIRE(PublishUtilityHeatGeneration(*first, GetUtilityPowerInterval(), 112, 1) == UtilityPowerError::None);
	REQUIRE(PublishUtilityHeatGeneration(*second, GetUtilityPowerInterval(), 149, 1) == UtilityPowerError::None);
	{ AutoRestoreBackup system{_current_company, OWNER_DEITY}; REQUIRE(AllocateUtilityHeat(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{*input, 7}}) == UtilityPowerError::None); }
	const auto plan = PrepareUtilityHeatConversion({{*input, GetUtilityPowerInterval()}, *output, 7, 1}); REQUIRE(plan.error == UtilityPowerError::None);
	UtilityConversionBenefit benefit{_current_company}; REQUIRE(CommitUtilityHeatConversion(plan, benefit).error == UtilityPowerError::None);
	const auto *source = GetUtilityPowerProducer(*output); REQUIRE(source != nullptr);
	CHECK(source->conversion_origins == std::vector{UtilityHeatConversionOrigin{*first, 3}, UtilityHeatConversionOrigin{*second, 4}});
	CHECK(source->generated_lifetime_j == 1); CHECK(source->conversion_remainder == 5); CHECK(source->meter.generation_rate_w == 38);
	CHECK(GetUtilityPowerProducer(*first)->consumed_lifetime_j == 3); CHECK(GetUtilityPowerProducer(*second)->consumed_lifetime_j == 4);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Detached carry/history/reference corruption cannot become a published source or repair any actual original use, output, invoice or participant state. */
TEST_CASE("Shared conversion staged history refuses malformed coupled quantities", "[utility-conversion-stage]")
{
	UtilityConversionRig rig; rig.Allocate(7); UtilityConversionBenefit benefit{_current_company};
	REQUIRE(CommitUtilityHeatConversion(rig.Prepare(7), benefit).error == UtilityPowerError::None);
	const auto valid = CaptureUtilityPowerState(); const auto before = ComputeUtilityPowerDigest();
	// Each candidate attacks a distinct original reference, conversion ratio or
	// retained quantity. A failed complete validation must preserve the live owner.
	for (uint fault = 0; fault < 10; ++fault) {
		auto bad = valid; auto &source = bad.producers[rig.output.base()];
		switch (fault) {
			case 0: source.conversion_input = UtilityConsumerID{UINT32_MAX}; break;
			case 1: source.conversion_efficiency = 35; break;
			case 2: source.conversion_remainder = 100; break;
			case 3: source.conversion_capacity_w = 1; break;
			case 4: source.conversion_origins.front().heat_j = 8; break;
			case 5: source.conversion_origins.push_back(source.conversion_origins.front()); break;
			case 6: source.conversion_origins.front().producer = rig.output; break;
			case 7: ++source.generated_lifetime_j; break;
			case 8: bad.producers[rig.heat.base()].conversion_remainder = 1; break;
			case 9: source.meter.generation_rate_w = 37; break;
		}
		CHECK(ValidateUtilityPowerState(bad, GetCurrentMultiLayerProfileSelection()) != UtilityPowerError::None);
		StageLoadedUtilityPowerState(std::move(bad)); CHECK_FALSE(AcceptLoadedUtilityPowerState()); CHECK(ComputeUtilityPowerDigest() == before);
	}
	StageLoadedUtilityPowerState(valid); REQUIRE(AcceptLoadedUtilityPowerState()); CHECK(ComputeUtilityPowerDigest() == before);
}

/** Zero actual Heat supply cannot settle an invoice, advance a numerator or create an electrical offer merely because an input requested work. */
TEST_CASE("Shared conversion waits unchanged for actual zero Heat supply", "[utility-conversion-zero]")
{
	UtilityConversionRig rig; rig.Allocate(1, 0); UtilityConversionBenefit benefit{_current_company};
	const auto before = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	const auto plan = rig.Prepare(1); CHECK(plan.error == UtilityPowerError::InsufficientGrant);
	CHECK(CommitUtilityHeatConversion(plan, benefit).error != UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(Company::Get(_current_company)->money == cash); CHECK(benefit.completed_heat == 0);
	CHECK(GetUtilityPowerProducer(rig.output)->conversion_remainder == 0); CHECK(GetUtilityPowerProducer(rig.output)->generated_lifetime_j == 0);
}

/** A real registered output ceiling bounds whole conversion; refusing an oversized run cannot consume the smaller feasible grant or its saved carry. */
TEST_CASE("Shared conversion admits only the exact feasible output capacity", "[utility-conversion-capacity]")
{
	UtilityConversionRig rig{15, false, 100}; rig.Allocate(20); UtilityConversionBenefit benefit{_current_company};
	const auto before = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	const auto too_large = rig.Prepare(20); CHECK(too_large.error == UtilityPowerError::InvalidQuantity);
	CHECK(CommitUtilityHeatConversion(too_large, benefit).error != UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(Company::Get(_current_company)->money == cash);
	const auto partial = rig.Prepare(13); REQUIRE(partial.error == UtilityPowerError::None); CHECK(partial.electric_j == 1); CHECK(partial.remainder_after == 95);
	REQUIRE(CommitUtilityHeatConversion(partial, benefit).error == UtilityPowerError::None);
	CHECK(GetUtilityPowerProducer(rig.heat)->consumed_lifetime_j == 13); CHECK(Company::Get(_current_company)->money == cash - Money{5});
	CHECK(rig.Prepare(1).error == UtilityPowerError::DuplicatePublication);
	rig.Rotate(); rig.Allocate(14); const auto blocked = ComputeUtilityPowerDigest();
	CHECK(rig.Prepare(14).error == UtilityPowerError::InvalidQuantity); CHECK(ComputeUtilityPowerDigest() == blocked);
	CHECK(GetUtilityPowerProducer(rig.output)->conversion_remainder == 95); CHECK(GetUtilityPowerProducer(rig.output)->meter.generation_energy_j == 0);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Detached synthetic uint64 lifetime history checks arithmetic bounds; it is not a claim that a native source produced that history during this fixture. */
TEST_CASE("Shared conversion validates large lifetime provenance without multiplying unbounded input", "[utility-conversion-lifetime]")
{
	UtilityConversionRig rig; rig.Allocate(7); UtilityConversionBenefit benefit{_current_company};
	REQUIRE(CommitUtilityHeatConversion(rig.Prepare(7), benefit).error == UtilityPowerError::None);
	const auto before = ComputeUtilityPowerDigest(); auto candidate = CaptureUtilityPowerState();
	auto &heat = candidate.producers[rig.heat.base()]; auto &output = candidate.producers[rig.output.base()];
	heat.generated_lifetime_j = UINT64_MAX; heat.consumed_lifetime_j = UINT64_MAX - 27000;
	output.conversion_origins.front().heat_j = heat.consumed_lifetime_j;
	const uint64_t whole = (heat.consumed_lifetime_j / 100) * 15, numerator = (heat.consumed_lifetime_j % 100) * 15;
	output.generated_lifetime_j = whole + numerator / 100; output.conversion_remainder = static_cast<uint8_t>(numerator % 100);
	CHECK(ValidateUtilityPowerState(candidate, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	++output.generated_lifetime_j; CHECK(ValidateUtilityPowerState(candidate, GetCurrentMultiLayerProfileSelection()) != UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == before);
}

/** A bound conversion input cannot bypass its exact output/history transaction through the earlier standalone Heat-consume API. */
TEST_CASE("Shared conversion cannot consume its bound input through an unrelated benefit", "[utility-conversion-bypass]")
{
	UtilityConversionRig rig; rig.Allocate(7); UtilityConversionBenefit benefit{_current_company};
	const auto plan = PrepareUtilityHeatMutation(UtilityMutationKind::Consume, {rig.input, GetUtilityPowerInterval()}, 7);
	REQUIRE(plan.error == UtilityPowerError::None);
	const auto before = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	CHECK(CommitUtilityMutation(plan, &benefit).error == UtilityPowerError::Refused);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(Company::Get(_current_company)->money == cash); CHECK(benefit.completed_heat == 0);
}

/** Existing publication's owner guard prevents recursive rate writes while the complete conversion draft waits to publish. */
TEST_CASE("Shared conversion rejects recursive publication during actual invoice commit", "[utility-conversion-reentry]")
{
	UtilityConversionRig rig; const auto external = RegisterUtilityPowerProducer(_current_company, MapLayer::DeepUnderground, UtilityGenerationKind::External);
	REQUIRE(external.has_value()); rig.Allocate(7); UtilityConversionBenefit benefit{_current_company}; benefit.recursive_source = *external;
	const auto plan = rig.Prepare(7); REQUIRE(plan.error == UtilityPowerError::None);
	REQUIRE(CommitUtilityHeatConversion(plan, benefit).error == UtilityPowerError::None);
	CHECK(benefit.recursive_result == UtilityPowerError::Refused); CHECK(GetUtilityPowerProducer(*external)->meter.accounting_interval == 0);
	CHECK(GetUtilityPowerProducer(rig.output)->meter.generation_energy_j == 1);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}
