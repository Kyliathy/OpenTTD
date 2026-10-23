/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_lifecycle.cpp Native retirement of the original operating paid first-plant machinery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CB/operating1.3B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Ordinary afterload supplies the original world; no fixture builder, material, floor, clock or energy restoration participates.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_first_plant_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../landscape_cmd.h"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static std::function<void()> *_plant_lifecycle_observer = nullptr; ///< Invocation-only ordinary startup observer; no saved participant or alternate energy owner.

/** @param arguments Ordinary native startup command. @return Success after the original loaded world was observed through the bounded test callback. */
static bool ObservePlantLifecycle(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_plant_lifecycle_observer != nullptr); (*_plant_lifecycle_observer)(); return true;
}

/** @param original Original operating producer before physical removal. Require immutable consumed/generated/conversion history and terminal rejection of fresh publication, without assigning an old balance to replacement machinery. */
static void CheckRetiredPlantProducer(const UtilityPowerProducer &original)
{
	const auto *retired = GetUtilityPowerProducer(original.meter.producer_id); REQUIRE(retired != nullptr);
	CHECK_FALSE(retired->active); CHECK(retired->reserved_j == 0);
	CHECK(retired->owner == original.owner); CHECK(retired->kind == original.kind);
	CHECK(retired->generated_lifetime_j == original.generated_lifetime_j); CHECK(retired->consumed_lifetime_j == original.consumed_lifetime_j);
	CHECK(retired->conversion_input == original.conversion_input); CHECK(retired->conversion_efficiency == original.conversion_efficiency);
	CHECK(retired->conversion_capacity_w == original.conversion_capacity_w); CHECK(retired->conversion_remainder == original.conversion_remainder);
	const auto digest = ComputeUtilityPowerDigest();
	// A retired Heat source must refuse through its actual typed publisher;
	// an Electricity-domain mismatch alone would not prove thermal retirement.
	const auto publication = original.meter.domain == UtilityEnergyDomain::Heat ?
		PublishUtilityHeatGeneration(original.meter.producer_id, GetUtilityPowerInterval(), 1000, 1, false) :
		PublishUtilityGeneration(original.meter.producer_id, GetUtilityPowerInterval(), 1000, 1, false);
	CHECK(publication != UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == digest);
}

/** @param tile Actual original machinery footprint. @return Ordinary native demolition invoice, after independently pure estimate and exact command-kernel cash settlement. */
static Money ClearOriginalPlantMachinery(TileIndex tile)
{
	const auto *company = Company::Get(_current_company); const auto cash = company->money;
	const auto utility = ComputeUtilityPowerDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const std::array random{_random.state[0], _random.state[1]};
	const auto quote = Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(tile));
	REQUIRE(quote.Succeeded()); CHECK(company->money == cash); CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK((std::array{_random.state[0], _random.state[1]} == random));
	const auto cost = Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, tile); REQUIRE(cost.Succeeded());
	CHECK(cost.GetCost() == quote.GetCost()); CHECK(company->money == cash - cost.GetCost()); return cost.GetCost();
}

/** @param paths Invocation-owned actual native outputs. @param stage Original0 or successive actual demolition1..3. @param generators Original model incarnations, retained as observations only. @param collector Original collector incarnation. @param tiles Actual original machinery origins. @param invoices Native demolition debits only. Retain a pure native world and explicit original/natural/account/retired-owner observations for dependent reconstruction and file acceptance. */
static void SavePlantLifecycle(const TestArtifactPaths &paths, uint stage, const std::vector<GeothermalGeneratorState> &generators,
	const GeothermalCollectorState &collector, const std::array<TileIndex, 3> &tiles, Money invoices)
{
	const auto path = paths.Output("saves", fmt::format("plant-lifecycle-retired-{}.sav", stage));
	const auto &vent = GetGeologyWorldState().vents.front(); const auto utility = ComputeUtilityPowerDigest();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()); const auto tick = TimerGameTick::counter;
	const auto cash = Company::Get(_current_company)->money; const std::array random{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup format{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(TimerGameTick::counter == tick); CHECK(Company::Get(_current_company)->money == cash);
	CHECK((std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random));
	fmt::print("PLANT_LIFECYCLE_WORLD stage={} snapshot=tick={},year={},cash={},invoices={},vent={},capacity={},geology={},projects={},exchange={},supply={},risers={},industries={},generators={},collectors={},vehicles={},packets={},collector={},collector_site={},collector_tile={},heat_source={},",
		stage, tick, TimerGameCalendar::year.base(), cash.base(), invoices.base(), vent.id.base(), vent.thermal_capacity_w, geology,
		ComputeTBMProjectDigest(), ComputeTBMExchangeDigest(), ComputeSurfaceSupplyDigest(), ComputeGeothermalRiserDigest(),
		Industry::GetNumItems(), CaptureGeothermalGenerators().size(), CaptureGeothermalCollectors().size(), Vehicle::GetNumItems(), CargoPacket::GetNumItems(),
		collector.industry.base(), collector.site.base(), tiles[2].base(), collector.heat_producer.base());
	// Original paid model/claim/geometry references survive in the receipt,
	// even after native demolition legitimately removes the actual machinery.
	for (uint index = 0; index < generators.size(); ++index) {
		const auto &state = generators[index]; const auto *source = GetUtilityPowerProducer(state.electrical_producer); REQUIRE(source != nullptr);
		fmt::print("generator{}={},site{}={},tile{}={},model{}={},work{}={},service{}={},heat_consumer{}={},electrical_source{}={},generated{}={},consumed{}={},active{}={},",
			index, state.industry.base(), index, state.site.base(), index, tiles[index].base(), index, to_underlying(state.model), index, state.work_ticks,
			index, state.active_service_ticks, index, state.heat_consumer.base(), index, state.electrical_producer.base(),
			index, source->generated_lifetime_j, index, source->consumed_lifetime_j, index, source->active);
	}
	const auto *heat = GetUtilityPowerProducer(collector.heat_producer); REQUIRE(heat != nullptr);
	fmt::print("heat_generated={},heat_consumed={},heat_active={}, path={}\n", heat->generated_lifetime_j, heat->consumed_lifetime_j, heat->active, path.string());
}

/** Actual ordinary-loaded paid machinery retires through native clearing; the natural source, earned floor, spoil/stock/cargo owners and consumed energy remain original. */
TEST_CASE("Original operating first plant retires machinery without changing its natural source", "[.geothermal-first-plant-lifecycle-retire]")
{
	const char *input = std::getenv("OTTD_PLANT_LIFECYCLE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("geothermal", "source-lifecycle"); bool observed = false; std::exception_ptr failure;
	/** Borrow actual ordinary-loaded owners; demolition is native command work and saving never advances a frame. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
			REQUIRE(TimerGameCalendar::year.base() == 2000); REQUIRE(GetGeologyWorldState().vents.size() == 1);
			const auto vent = GetGeologyWorldState().vents.front(); REQUIRE(vent.thermal_capacity_w == 1000000);
			auto generators = CaptureGeothermalGenerators(); REQUIRE(generators.size() == 2);
			std::ranges::sort(generators, {}, &GeothermalGeneratorState::model);
			REQUIRE(generators[0].model == GeothermalGeneratorModel::VentTap); REQUIRE(generators[1].model == GeothermalGeneratorModel::MagmaLoop);
			const auto collectors = CaptureGeothermalCollectors(); REQUIRE(collectors.size() == 1); const auto collector = collectors.front();
			REQUIRE(IsGeothermalCollectorOperational(collector.industry)); REQUIRE(collector.materials_committed);
			std::array<TileIndex, 3> tiles{Industry::Get(generators[0].industry)->location.tile, Industry::Get(generators[1].industry)->location.tile, Industry::Get(collector.industry)->location.tile};
			std::vector<UtilityPowerProducer> producers;
			// Require genuine completed, previously operating construction in both
			// dated models. A cold blueprint/definition cannot satisfy this entry.
			for (const auto &state : generators) {
				REQUIRE(IsGeothermalGeneratorConstructionComplete(state.industry)); REQUIRE(state.materials_committed); REQUIRE(state.active_service_ticks > 0);
				const auto *source = GetUtilityPowerProducer(state.electrical_producer); REQUIRE(source != nullptr); REQUIRE(source->generated_lifetime_j > 0);
				REQUIRE(source->consumed_lifetime_j > 0); producers.push_back(*source);
				REQUIRE_FALSE(GeothermalGeneratorHasStoredCargo(*Industry::Get(state.industry)));
			}
			const auto *heat = GetUtilityPowerProducer(collector.heat_producer); REQUIRE(heat != nullptr); REQUIRE(heat->consumed_lifetime_j > 0); const auto original_heat = *heat;
			REQUIRE_FALSE(GeothermalCollectorHasStoredCargo(*Industry::Get(collector.industry))); REQUIRE(CargoPacket::GetNumItems() == 0);
			const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), projects = ComputeTBMProjectDigest(), exchange = ComputeTBMExchangeDigest();
			const auto supply = ComputeSurfaceSupplyDigest(), risers = ComputeGeothermalRiserDigest(); const auto tick = TimerGameTick::counter;
			const auto income = Company::Get(_current_company)->cur_economy.income; const auto loan = Company::Get(_current_company)->current_loan;
			const auto vehicles = Vehicle::GetNumItems(); Money invoices = 0; SavePlantLifecycle(paths, 0, generators, collector, tiles, invoices);
			// Each native clear retires only one actual incarnation. Test old
			// grants immediately, before a replacement can reuse a native pool ID.
			for (uint index = 0; index < generators.size(); ++index) {
				invoices += ClearOriginalPlantMachinery(tiles[index]); const auto &state = generators[index];
				CHECK_FALSE(Industry::IsValidID(state.industry)); CHECK(GetGeothermalGeneratorState(state.industry) == nullptr);
				const auto *consumer = GetUtilityPowerConsumer(state.heat_consumer); REQUIRE(consumer != nullptr); CHECK_FALSE(consumer->active);
				CHECK(PrepareUtilityMutation(UtilityMutationKind::Consume, {state.heat_consumer, GetUtilityPowerInterval()}, 1).error != UtilityPowerError::None);
				CheckRetiredPlantProducer(producers[index]); SavePlantLifecycle(paths, index + 1, generators, collector, tiles, invoices);
			}
			invoices += ClearOriginalPlantMachinery(tiles[2]); CHECK_FALSE(Industry::IsValidID(collector.industry)); CHECK(GetGeothermalCollectorState(collector.industry) == nullptr);
			CHECK(FindGeothermalCollectorForVent(vent.id) == IndustryID::Invalid()); CheckRetiredPlantProducer(original_heat);
			CHECK(CaptureGeothermalCollectors().empty()); CHECK(CaptureGeothermalGenerators().empty());
			CHECK(GetGeologyWorldState().vents.front().id == vent.id); CHECK(GetGeologyWorldState().vents.front().thermal_capacity_w == vent.thermal_capacity_w);
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeTBMProjectDigest() == projects); CHECK(ComputeTBMExchangeDigest() == exchange);
			CHECK(ComputeSurfaceSupplyDigest() == supply); CHECK(ComputeGeothermalRiserDigest() == risers); CHECK(TimerGameTick::counter == tick);
			CHECK(Company::Get(_current_company)->cur_economy.income == income); CHECK(Company::Get(_current_company)->current_loan == loan);
			CHECK(Vehicle::GetNumItems() == vehicles); CHECK(CargoPacket::GetNumItems() == 0);
			// The complete original native footprints remain excavated after
			// demolition; only their machinery/claim binding may change.
			for (uint index = 0; index < tiles.size(); ++index) {
				const uint size = index == 2 ? 3 : 4;
				for (uint y = 0; y < size; ++y) for (uint x = 0; x < size; ++x) CHECK(GetGeologyWorldState().terrain.Get(TileAddWrap(tiles[index], x, y)) == ExcavationState::Excavated);
			}
			SavePlantLifecycle(paths, 3, generators, collector, tiles, invoices);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_plant_lifecycle_observer, &inspect}; IConsole::CmdRegister("plant_lifecycle_retire", ObservePlantLifecycle);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
