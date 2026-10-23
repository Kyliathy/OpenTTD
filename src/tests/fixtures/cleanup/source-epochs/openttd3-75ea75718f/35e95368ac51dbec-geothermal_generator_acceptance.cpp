/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_acceptance.cpp Actual common-driver contention and viewed/hidden/headless native operating equivalence.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8DD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_view_fixture.h"
#include "geothermal_generator_operating_fixture.h"
#include "geothermal_heat_participant_fixture.h"
#include "../map_layer_gui.h"
#include "../safeguards.h"

/** A previously issued real physical expense and actual native powerhouse compete by their shared ConsumerID, with one original thermal budget and complete expenses. */
TEST_CASE("Native powerhouse and existing direct Heat expense share the common driver", "[geothermal-generator-contention]")
{
	GeothermalGeneratorOperatingRig rig;
	const auto owner = _current_company;
	const auto direct = RegisterGeothermalHeatConsumer(owner, rig.pipe[1], UtilityHeatUse::DirectRecipe, 27000); REQUIRE(direct.has_value());
	RoutedExpense expense(owner, UtilityHeatUse::DirectRecipe);
	// Acquire the actual native generator's original pair during synchronized
	// simulation with no cash. This earns no Heat/service or output and never
	// fabricates a synthetic generator participant for the later contention.
	const auto cash = Company::Get(owner)->money; Company::Get(owner)->money = 0; rig.Operate(); Company::Get(owner)->money = cash;
	const auto original = *GetGeothermalGeneratorState(rig.generator);
	CHECK(original.active_service_ticks == 0); REQUIRE(*direct < original.heat_consumer);
	// Cash and physical admission are now healthy, the original output has
	// never published and no successful tick exists. Only the closed electrical
	// opportunity blocks a positive demand; queries cannot reserve more Heat.
	REQUIRE(IsUtilityPowerAllocated()); const auto closed = ComputeUtilityPowerDigest();
	CHECK_FALSE(PrepareGeothermalGeneratorDemand(rig.generator, 1).has_value());
	CHECK(ComputeUtilityPowerDigest() == closed); CHECK(*GetGeothermalGeneratorState(rig.generator) == original);
	CHECK(Company::Get(owner)->money == cash);
	const uint64_t requested = GENERATE(6000, 27000);
	const GeothermalHeatRequest request{owner, *direct, rig.pipe[1], UtilityHeatUse::DirectRecipe, requested};
	const std::array demands{GeothermalHeatDemand{request, &expense}};
	++TimerGameTick::counter; PrepareTubePowerTick(demands);
	const auto *output = GetUtilityPowerProducer(original.electrical_producer); REQUIRE(output != nullptr);
	const uint64_t converted = 27000 - requested;
	CHECK(output->generated_lifetime_j == converted * 15 / 100);
	CHECK(GetGeothermalGeneratorState(rig.generator)->active_service_ticks == (converted == 0 ? 0 : 1));
	const auto allocation = GetUtilityHeatAllocation(*direct); REQUIRE(allocation.has_value()); CHECK(allocation->granted_energy_j == requested);
	REQUIRE(ConsumeGeothermalHeat({*direct, GetUtilityPowerInterval()}, requested, expense).error == UtilityPowerError::None);
	CHECK(Company::Get(owner)->money == cash - Money{5});
	const auto *source = GetUtilityPowerProducer(GetGeothermalCollectorState(rig.collector)->heat_producer); REQUIRE(source != nullptr);
	CHECK(source->consumed_lifetime_j == 27000);
	const auto digest = ComputeUtilityPowerDigest(); const auto native = ComputeGeothermalGeneratorDigest();
	PrepareTubePowerTick(demands); CHECK(ComputeUtilityPowerDigest() == digest); CHECK(ComputeGeothermalGeneratorDigest() == native);
	CHECK(ConsumeGeothermalHeat({*direct, GetUtilityPowerInterval()}, 1, expense).error != UtilityPowerError::None);
	CHECK(Company::Get(owner)->money == cash - Money{5});
}

/** Exact native operating comparison; views/caches are deliberately absent from every saved authority compared here. */
struct GeothermalViewResult {
	uint64_t utility; ///< Complete original shared Heat/electrical/source lineage.
	uint64_t generator; ///< Original native construction/service/reference digest.
	uint64_t collector; ///< Original paid thermal machinery digest.
	uint64_t geology; ///< Exact natural source/floor/reserve owner.
	Money cash; ///< Actual company money after committed service.
	std::array<uint32_t, 2> random; ///< Actual gameplay random stream, independent of cosmetic UI state.
	/** @param other Another full physical replay. @return Exact saved native authority equality. */
	bool operator==(const GeothermalViewResult &other) const = default;
};

/** @param mode Zero uses a Deep viewport, one a Surface viewport hiding Deep machinery, two no viewport. @return Exact actual native authority after identical common opportunities and repeated pure queries. */
static GeothermalViewResult RunGeothermalViewed(uint8_t mode)
{
	GeothermalGeneratorOperatingRig rig;
	std::unique_ptr<UndergroundToolbarTestUI> ui;
	Window *view = nullptr;
	if (mode != 2) {
		ui = std::make_unique<UndergroundToolbarTestUI>(_current_company); view = new UndergroundTestViewport();
		SetViewportLayer(view, mode == 0 ? MapLayer::DeepUnderground : MapLayer::Surface);
	}
	// Repeated presentation queries must not allocate identities, generate
	// power, charge service or consume RNG before each actual native tick.
	for (uint16_t tick = 0; tick < 74; ++tick) {
		const auto utility = ComputeUtilityPowerDigest(); const auto native = ComputeGeothermalGeneratorDigest();
		const auto cash = Company::Get(_current_company)->money; const auto random = std::array{_random.state[0], _random.state[1]};
		PrepareGeothermalGeneratorDemand(rig.generator, 1); GetGeothermalGeneratorPort(rig.generator); HasGeothermalGeneratorInstalledPorts(rig.generator);
		CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native);
		CHECK(Company::Get(_current_company)->money == cash); CHECK(std::array{_random.state[0], _random.state[1]} == random);
		rig.Operate();
	}
	return {ComputeUtilityPowerDigest(), ComputeGeothermalGeneratorDigest(), ComputeGeothermalCollectorDigest(),
		ComputeGeologyStateDigest(GetGeologyWorldState()), Company::Get(_current_company)->money, {_random.state[0], _random.state[1]}};
}

/** A real selected Deep viewport, a selected Surface viewport and headless replay produce exactly the same original utility, service, stock, geology, cash and RNG. */
TEST_CASE("Native powerhouse operation is equal when viewed hidden or headless", "[geothermal-generator-view-equivalence]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	const auto visible = RunGeothermalViewed(0); const auto hidden = RunGeothermalViewed(1); const auto headless = RunGeothermalViewed(2);
	CHECK(visible == hidden); CHECK(visible == headless);
}
