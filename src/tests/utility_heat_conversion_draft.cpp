/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */
/** @file utility_heat_conversion_draft.cpp Pure conversion admission and detached-history limits over the original shared owner.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group27.
 * Declared restored history is explicit component input, never paid machinery, a current save format or committed conversion proof.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "utility_heat_conversion_fixture.h"

/** Publish one explicitly declared complete historical component through the original validation/staging boundary.
 * @param state Fully declared numerical history; this helper creates no physical plant or paid operating receipt.
 */
static void RestoreConversionComponent(UtilityPowerState state)
{
	REQUIRE(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	StageLoadedUtilityPowerState(std::move(state)); REQUIRE(AcceptLoadedUtilityPowerState());
}

/** Declare an already published seven-joule prefix while retaining seven original granted joules for a duplicate-publication query.
 * @param rig Actual numerical owner whose fourteen-joule grant precedes this explicit history input.
 */
static void DeclareConversionPrefix(UtilityConversionRig &rig)
{
	auto state = CaptureUtilityPowerState(); auto &heat = state.producers[rig.heat.base()];
	auto &output = state.producers[rig.output.base()]; auto &input = state.consumers[rig.input.base()];
	REQUIRE(input.shares.size() == 1); input.shares.front().consumed_j = 7;
	heat.consumed_lifetime_j = 7; heat.reserved_j -= 7;
	output.conversion_origins = {{rig.heat, 7}}; output.generated_lifetime_j = 1; output.conversion_remainder = 5;
	output.meter.accounting_interval = state.interval; output.meter.economy_ticks = 1;
	output.meter.generation_rate_w = 38; output.meter.generation_energy_j = 1;
	RestoreConversionComponent(std::move(state));
}

/** Existing electrical roles and rate APIs cannot acquire or publish a second conversion-only source. */
TEST_CASE("Conversion-only registration grants neither ordinary generation nor duplicate output", "[utility-conversion-draft-definition]")
{
	UtilityConversionRig rig; const auto before = ComputeUtilityPowerDigest();
	CHECK_FALSE(RegisterUtilityHeatConversionProducer(_current_company, rig.input, 15, 150000).has_value());
	CHECK_FALSE(RegisterUtilityPowerProducer(_current_company, MapLayer::DeepUnderground, UtilityGenerationKind::HeatConversion).has_value());
	CHECK(PublishUtilityGeneration(rig.output, GetUtilityPowerInterval(), 150000, 1) == UtilityPowerError::Refused);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(GetUtilityPowerProducer(rig.output)->generated_lifetime_j == 0);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	const auto direct = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::DirectRecipe, 27000, std::array{rig.heat}); REQUIRE(direct.has_value());
	const auto established = ComputeUtilityPowerDigest();
	CHECK_FALSE(RegisterUtilityHeatConversionProducer(_current_company, *direct, 15, 150000).has_value());
	CHECK(ComputeUtilityPowerDigest() == established);
}

/** Both dated ratios prepare exact quantities without spending original Heat, output, cash or fractional carry. */
TEST_CASE("Conversion draft computes exact fifteen and thirty-five percent without settlement", "[utility-conversion-draft-exact]")
{
	const auto efficiency = GENERATE(uint8_t{15}, uint8_t{35}); UtilityConversionRig rig{efficiency}; rig.Allocate(27000);
	const auto before = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	const auto random = std::array{_random.state[0], _random.state[1]};
	const auto plan = rig.Prepare(27000); REQUIRE(plan.error == UtilityPowerError::None);
	CHECK(plan.electric_j == (efficiency == 15 ? 4050 : 9450)); CHECK(plan.loss_j == 27000 - plan.electric_j);
	CHECK(plan.remainder_after == 0); CHECK(plan.seal != 0);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(std::array{_random.state[0], _random.state[1]} == random);
	CHECK(GetUtilityPowerProducer(rig.heat)->reserved_j == 27000); CHECK(GetUtilityPowerProducer(rig.heat)->consumed_lifetime_j == 0);
	CHECK(GetUtilityPowerProducer(rig.output)->meter.generation_energy_j == 0);
}

/** A declared original percent carry produces the next exact joule without rounding an input rate or mutating the saved carry. */
TEST_CASE("Conversion draft preserves original fractional carry until whole settlement", "[utility-conversion-draft-carry]")
{
	UtilityConversionRig rig; auto state = CaptureUtilityPowerState(); state.interval = 2;
	auto &heat = state.producers[rig.heat.base()]; auto &output = state.producers[rig.output.base()];
	heat.meter.accounting_interval = 1; heat.generated_lifetime_j = heat.consumed_lifetime_j = 99;
	output.meter.accounting_interval = 1; output.generated_lifetime_j = 14; output.conversion_remainder = 85;
	output.conversion_origins = {{rig.heat, 99}}; RestoreConversionComponent(std::move(state));
	rig.Allocate(1); const auto before = ComputeUtilityPowerDigest(); const auto plan = rig.Prepare(1);
	REQUIRE(plan.error == UtilityPowerError::None); CHECK(plan.electric_j == 1); CHECK(plan.loss_j == 0); CHECK(plan.remainder_after == 0);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(GetUtilityPowerProducer(rig.output)->conversion_remainder == 85);
}

/** Refused owners, roles, intervals, grants, durations and closed publication phases cannot reserve a result or debit either domain. */
TEST_CASE("Conversion draft refuses invalid original participants unchanged", "[utility-conversion-draft-refusal]")
{
	UtilityConversionRig rig; rig.Allocate(14); UtilityHeatConversionRequest request{{rig.input, GetUtilityPowerInterval()}, rig.output, 7, 1};
	AutoRestoreBackup actor{_current_company}; UtilityPowerError expected = UtilityPowerError::InvalidReference;
	SECTION("another output role") { request.output = rig.heat; }
	SECTION("foreign actor") { REQUIRE(Company::CanAllocateItem()); auto *foreign = Company::Create(); foreign->money = 1000000; _current_company = foreign->index; expected = UtilityPowerError::InvalidOwner; }
	SECTION("stale original epoch") { --request.grant.interval; expected = UtilityPowerError::InvalidInterval; }
	SECTION("insufficient actual grant") { request.heat_j = 15; expected = UtilityPowerError::InsufficientGrant; }
	SECTION("zero heat") { request.heat_j = 0; expected = UtilityPowerError::InvalidQuantity; }
	SECTION("zero duration") { request.economy_ticks = 0; expected = UtilityPowerError::InvalidQuantity; }
	SECTION("unbounded duration") { request.economy_ticks = MAX_UTILITY_INTERVAL_TICKS + 1; expected = UtilityPowerError::InvalidQuantity; }
	SECTION("output already published") { DeclareConversionPrefix(rig); expected = UtilityPowerError::DuplicatePublication; }
	SECTION("electrical allocation already closed") {
		AutoRestoreBackup system{_current_company, OWNER_DEITY}; REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), {}) == UtilityPowerError::None);
		expected = UtilityPowerError::AlreadyAllocated;
	}
	const auto before = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	CHECK(PrepareUtilityHeatConversion(request).error == expected);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(Company::Get(_current_company)->money == cash);
}

/** Actual output rating is a whole duration ceiling; a pure query cannot bank an over-capacity draft. */
TEST_CASE("Conversion draft refuses whole output beyond its original rating", "[utility-conversion-draft-capacity]")
{
	UtilityConversionRig rig{15, false, 100}; rig.Allocate(20); const auto before = ComputeUtilityPowerDigest();
	CHECK(rig.Prepare(20).error == UtilityPowerError::InvalidQuantity);
	const auto feasible = rig.Prepare(19); REQUIRE(feasible.error == UtilityPowerError::None); CHECK(feasible.electric_j == 2);
	CHECK(ComputeUtilityPowerDigest() == before);
}

/** A saturated original output history cannot wrap its lifetime counter when fresh Heat becomes available from another real shared source. */
TEST_CASE("Conversion draft rejects output lifetime overflow without altering new grant", "[utility-conversion-draft-lifetime]")
{
	UtilityConversionRig rig{100}; auto state = CaptureUtilityPowerState(); state.interval = 2;
	auto &heat = state.producers[rig.heat.base()]; auto &output = state.producers[rig.output.base()];
	heat.meter.accounting_interval = 1; heat.generated_lifetime_j = heat.consumed_lifetime_j = UINT64_MAX;
	output.meter.accounting_interval = 1; output.generated_lifetime_j = UINT64_MAX; output.conversion_origins = {{rig.heat, UINT64_MAX}};
	UtilityPowerProducer fresh; fresh.owner = _current_company; fresh.kind = UtilityGenerationKind::Geothermal;
	fresh.meter.producer_id = UtilityProducerID{2}; fresh.meter.layer = MapLayer::DeepUnderground; fresh.meter.domain = UtilityEnergyDomain::Heat;
	state.producers.push_back(fresh); state.next_producer = 3; state.consumers.front().linked_producers.push_back(fresh.meter.producer_id);
	RestoreConversionComponent(std::move(state)); rig.heat = fresh.meter.producer_id; rig.Allocate(1);
	const auto before = ComputeUtilityPowerDigest(); CHECK(rig.Prepare(1).error == UtilityPowerError::InvalidQuantity);
	CHECK(ComputeUtilityPowerDigest() == before);
}

/** Declare a complete bounded original-source history solely for the attribution-cap boundary, without a physical plant or free live energy.
 * @param rig Actual shared owner receiving the explicitly declared component history.
 */
static void FillConversionOriginHistory(UtilityConversionRig &rig)
{
	constexpr uint32_t side = 128; static_assert(side * side == MAX_UTILITY_CONVERSION_ORIGINS);
	UtilityPowerState state; state.interval = 2; state.next_producer = 2 * side + 1; state.next_consumer = side;
	// Each of128 original Heat histories supplied one joule to each of128 exact
	// outputs. Their frozen128-joule totals conserve all16384 attributed joules.
	for (uint32_t index = 0; index <= side; ++index) {
		UtilityPowerProducer source; source.owner = _current_company; source.kind = UtilityGenerationKind::Geothermal;
		source.meter.producer_id = UtilityProducerID{index}; source.meter.layer = MapLayer::DeepUnderground; source.meter.domain = UtilityEnergyDomain::Heat;
		if (index < side) { source.meter.accounting_interval = 1; source.generated_lifetime_j = source.consumed_lifetime_j = side; }
		state.producers.push_back(std::move(source));
	}
	// Every output retains its own original input, ratio,19 whole joules and20
	// percent carry. New source128 is eligible now but absent from all old pairs.
	for (uint32_t index = 0; index < side; ++index) {
		UtilityPowerConsumer input; input.owner = _current_company; input.effect = UtilityConsumerEffect::HeatGenerator; input.capacity_j = 27000;
		input.meter.consumer_id = UtilityConsumerID{index}; input.meter.layer = MapLayer::DeepUnderground; input.meter.domain = UtilityEnergyDomain::Heat;
		input.linked_producers = {UtilityProducerID{side}}; state.consumers.push_back(std::move(input));
		UtilityPowerProducer output; output.owner = _current_company; output.kind = UtilityGenerationKind::HeatConversion;
		output.meter.producer_id = UtilityProducerID{side + 1 + index}; output.meter.layer = MapLayer::DeepUnderground;
		output.meter.accounting_interval = 1; output.conversion_input = UtilityConsumerID{index}; output.conversion_efficiency = 15;
		output.conversion_capacity_w = 150000; output.generated_lifetime_j = 19; output.conversion_remainder = 20;
		// All positive pairs are stored in canonical original-source order; this
		// explicit history is independent of the draft under examination.
		for (uint32_t original = 0; original < side; ++original) output.conversion_origins.push_back({UtilityProducerID{original}, 1});
		state.producers.push_back(std::move(output));
	}
	RestoreConversionComponent(std::move(state)); rig.heat = UtilityProducerID{side}; rig.input = UtilityConsumerID{0}; rig.output = UtilityProducerID{side + 1};
}

/** Full original attribution refuses one more source pair before publishing Heat debit, output, carry or participant rights. */
TEST_CASE("Conversion draft refuses exhausted original-source attribution without pruning history", "[utility-conversion-draft-origin-limit]")
{
	UtilityConversionRig rig; FillConversionOriginHistory(rig); rig.Allocate(1, 1000);
	const auto before = ComputeUtilityPowerDigest(); CHECK(rig.Prepare(1).error == UtilityPowerError::Limit);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(GetUtilityPowerProducer(rig.heat)->reserved_j == 1);
}
