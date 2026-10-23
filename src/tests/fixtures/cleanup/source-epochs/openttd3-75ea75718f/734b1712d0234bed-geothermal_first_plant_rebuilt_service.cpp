/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_rebuilt_service.cpp Paid physical repair and fresh useful service on the original reconstructed first plant.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CCD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Complete ordinary frames own every new identity, thermal conversion, service bill and useful town effect.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "plant_rebuild_service_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../misc_cmd.h"
#include "../openttd.h"
#include "../video/null_v.h"
#include "../gfx_func.h"
#include "../window_func.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static std::function<void()> *_rebuilt_service_start = nullptr; ///< Invocation-only ordinary afterload observer, never a saved job/account.
static std::function<void()> *_rebuilt_service_frame = nullptr; ///< Observation only after all ordinary simulation/input/window owners.

/** Explicit-only bounded native null driver; it supplies no simulation ticks or gameplay quantities. */
class RebuiltPlantServiceDriver final : public VideoDriver_Null {
public:
	/** Run complete ordinary frames until actual service acceptance or bounded refusal. */
	void MainLoop() override
	{
		// Both rebuilt machines, their physical network, shared ledger and native
		// town must run through one ordinary opportunity before observation.
		for (uint frame = 0; frame < 1005; ++frame) {
			if (_exit_game) break;
			::GameLoop(); ::InputLoop(); ::UpdateWindows();
			if (_exit_game) break;
			if (_rebuilt_service_frame != nullptr) (*_rebuilt_service_frame)();
		}
	}
	/** @return Explicit scenario driver name, leaving automatic production video selection unchanged. */
	std::string_view GetName() const override { return "plant-rebuilt-service"; }
};

/** Zero-priority test registration; production binaries contain no scenario observer. */
class RebuiltPlantServiceFactory final : public DriverFactoryBase {
public:
	/** Register only the explicitly selected completed-frame native observer. */
	RebuiltPlantServiceFactory() : DriverFactoryBase(Driver::Type::Video, 0, "plant-rebuilt-service", "Original rebuilt plant service observer") {}
	/** @return Native null driver with one read/command observation point after each whole frame. */
	std::unique_ptr<Driver> CreateInstance() const override { return std::make_unique<RebuiltPlantServiceDriver>(); }
};
static RebuiltPlantServiceFactory _rebuilt_plant_service_factory; ///< Explicit-only test driver, never a production override.

/** @param arguments Actual startup command. @return Success after inspecting ordinary-loaded original reconstructed machinery. */
static bool StartRebuiltPlantService(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_rebuilt_service_start != nullptr); (*_rebuilt_service_start)(); return true;
}

/** @param paths Invocation-owned original files. @param world Actual reconstructed machinery. @param scene Actual source-1/ports0/bound1/VentTapconnection2/partial3/complete4/isolation5/Magmaconnection6/partial7/complete8. @param step Original0 or its untouched complete next1. @param invoices Actual paid port invoices. Retain pure repeated complete worlds and both independent RNG authorities. */
static void SaveRebuiltPlantService(const TestArtifactPaths &paths, const PlantRebuildCargoWorld &world, int scene, uint step, Money invoices)
{
	const auto snapshot = CapturePlantRebuildService(world, invoices); const auto random = std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	const auto path = paths.Output("saves", fmt::format("plant-service-{}-{}.sav", scene, step)), repeat = paths.Output("saves", fmt::format("plant-service-{}-{}-repeat.sav", scene, step));
	// Every native owner, including genuine saved effects, is retained by both
	// pure writes; the observer does not normalize any world data for equality.
	for (const auto &file : {path, repeat}) {
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup format{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(file), false) == SaveLoadResult::Ok);
	}
	CHECK(CapturePlantRebuildService(world, invoices) == snapshot); CHECK((std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random));
	fmt::print("PLANT_SERVICE_WORLD scene={} step={} snapshot={} path={} repeat={}\n", scene, step, snapshot, path.string(), repeat.string());
}

/** Actual paid port repair, unique old-town recovery and two native models earn useful output without reusing any retired source/grant. */
TEST_CASE("Original reconstructed plant earns fresh useful service with both native models", "[.geothermal-first-plant-rebuilt-service]")
{
	const char *input = std::getenv("OTTD_PLANT_SERVICE_INPUT"); REQUIRE(input != nullptr); const auto paths = GetTestCaseArtifactPaths("geothermal", "rebuilt-service");
	PlantRebuildCargoWorld world; std::exception_ptr failure; bool started = false, completed = false; uint frames = 0;
	std::array<bool, 9> scenes{}; int pending = -1; uint64_t pending_tick = 0, original_tick = 0, geology = 0, original_heat = 0; Money invoices = 0;
	UtilityConsumerID town{UINT32_MAX}; UtilityPowerState original;
	/** @param scene Actual newly earned service boundary. Capture before any later observer command; its full next opportunity is mandatory. */
	const auto capture = [&](uint scene) {
		REQUIRE(pending == -1); REQUIRE_FALSE(scenes[scene]); SaveRebuiltPlantService(paths, world, static_cast<int>(scene), 0, invoices);
		scenes[scene] = true; pending = static_cast<int>(scene); pending_tick = TimerGameTick::counter;
	};
	/** Resolve only the accepted whole-return world; pay for five missing real ports, preserving old retired cells and naturally earned collector history. */
	std::function<void()> begin = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error)); world = LoadPlantRebuildCargoWorld();
			REQUIRE(PlantRebuildWholeEmptyHome(world)); REQUIRE(PlantRebuildAllConstructed(world)); original = CaptureUtilityPowerState(); REQUIRE(original.producers.size() == 4);
			original_tick = TimerGameTick::counter; geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			const auto *collector = GetGeothermalCollectorState(world.machines[0]); REQUIRE(collector != nullptr); REQUIRE(collector->heat_producer.base() == 3);
			original_heat = GetUtilityPowerProducer(collector->heat_producer)->generated_lifetime_j; REQUIRE(original_heat > 0);
			for (uint machine = 1; machine <= 2; ++machine) { // Fresh reconstructed models must not inherit their reused Industry pool slot's old energy/service identity.
				const auto *state = GetGeothermalGeneratorState(world.machines[machine]); REQUIRE(state->active_service_ticks == 0); REQUIRE(state->heat_consumer.base() == UINT32_MAX); REQUIRE(state->electrical_producer.base() == UINT32_MAX);
			}
			SaveRebuiltPlantService(paths, world, -1, 0, invoices);
			const auto *claim = GetGeothermalSiteClaim(collector->site); REQUIRE(claim != nullptr);
			invoices += ChangePlantRebuildPort(claim->site.endpoint, UtilityEnergyDomain::Heat);
			for (uint machine = 1; machine <= 2; ++machine) { // Both distinct port domains require their own paid installation; shared coordinates supply no connection.
				for (const auto domain : {UtilityEnergyDomain::Heat, UtilityEnergyDomain::Electricity}) invoices += ChangePlantRebuildPort(GetGeothermalGeneratorPort(world.machines[machine]), domain);
			}
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded()); capture(0); started = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	/** Observe complete ordinary frames; keep untouched next worlds before commands, every old tombstone and one exact shared original vent budget. */
	std::function<void()> frame = [&] {
		if (!started) return;
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; ++frames;
			if (pending != -1) {
				if (TimerGameTick::counter == pending_tick) return; // Presentation-only pauses cannot be repaired by inventing a simulation opportunity.
				REQUIRE(TimerGameTick::counter == pending_tick + 1); const int previous = pending; pending = -1; SaveRebuiltPlantService(paths, world, previous, 1, invoices);
				if (previous == 8) { completed = true; started = false; _exit_game = true; return; }
			}
			CHECK(PlantRebuildWholeEmptyHome(world)); CHECK(PlantRebuildAllConstructed(world)); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			for (const auto label : PLANT_REBUILD_LABELS) { const auto ledger = CheckPlantRebuildMaterial(world, label); CHECK(ledger.physical == 0); } // Actual paid replacement materials remain consumed, never reconstructed as stock by wiring/service.
			for (const auto &old : original.producers) {
				if (old.active) continue; // Naturally earned collector3 keeps publishing; only historical retired machinery must stay terminal.
				const auto *current = GetUtilityPowerProducer(old.meter.producer_id); REQUIRE(current != nullptr); CHECK_FALSE(current->active); CHECK(current->reserved_j == 0);
				CHECK(current->generated_lifetime_j == old.generated_lifetime_j); CHECK(current->consumed_lifetime_j == old.consumed_lifetime_j); CHECK(current->conversion_origins == old.conversion_origins);
			}
			for (const auto &old : original.consumers) if (!old.active) CHECK_FALSE(GetUtilityPowerConsumer(old.meter.consumer_id)->active); // Old retired input/forwarding grants never regain eligibility through pool-ID reuse.
			for (const auto &old : original.links) if (!old.active) CHECK_FALSE(GetUtilityPowerLink(old.id)->active); // Paid repairs issue new cells/edges and may not reactivate any retained tombstone.
			const auto *first = GetGeothermalGeneratorState(world.machines[1]), *second = GetGeothermalGeneratorState(world.machines[2]);
			const auto *heat = GetUtilityPowerProducer(GetGeothermalCollectorState(world.machines[0])->heat_producer); REQUIRE(heat != nullptr);
			CHECK(heat->generated_lifetime_j == original_heat + (TimerGameTick::counter - original_tick) * 27000);
			CHECK(heat->consumed_lifetime_j == (first->active_service_ticks + second->active_service_ticks) * 27000);
			CHECK(heat->consumed_lifetime_j <= heat->generated_lifetime_j - original_heat);
			if (!scenes[1]) {
				REQUIRE(first->heat_consumer.base() >= original.next_consumer); REQUIRE(second->heat_consumer > first->heat_consumer);
				REQUIRE(first->electrical_producer.base() >= original.next_producer); REQUIRE(second->electrical_producer > first->electrical_producer);
				CHECK(first->active_service_ticks == 1); CHECK(second->active_service_ticks == 0); capture(1); return;
			}
			if (!scenes[2]) { town = ConnectPlantRebuildTown(world, 1); capture(2); return; }
			const auto *first_output = GetUtilityPowerProducer(first->electrical_producer), *second_output = GetUtilityPowerProducer(second->electrical_producer); REQUIRE(first_output != nullptr); REQUIRE(second_output != nullptr);
			CHECK(first_output->generated_lifetime_j == first->active_service_ticks * 4050); CHECK(second_output->generated_lifetime_j == second->active_service_ticks * 9450);
			const auto *destination = GetUtilityPowerConsumer(town); REQUIRE(destination != nullptr);
			if (!scenes[4]) {
				CHECK(second->active_service_ticks == 0); const auto uses = first_output->consumed_lifetime_j / 135; CHECK(first_output->consumed_lifetime_j == uses * 135);
				CHECK(destination->town_effect_applied); CHECK(GetUtilityTownPowerBonus(destination->town) == 25);
				if (!scenes[3] && uses == 37) { capture(3); return; } // Retain a genuine interrupted useful VentTap world before its completed74-use boundary.
				if (uses == 74) { capture(4); return; }
				REQUIRE(uses < 74); return;
			}
			if (!scenes[5]) {
				invoices += ChangePlantRebuildPort(GetGeothermalGeneratorPort(world.machines[1]), UtilityEnergyDomain::Heat, true); capture(5); return;
			}
			if (!scenes[6]) { const auto retained = ConnectPlantRebuildTown(world, 2); REQUIRE(retained == town); capture(6); return; }
			const auto uses = second_output->consumed_lifetime_j / 135; CHECK(second_output->consumed_lifetime_j == uses * 135);
			CHECK(destination->town_effect_applied); CHECK(GetUtilityTownPowerBonus(destination->town) == 25); CHECK(Town::Get(destination->town)->cache.population > 0);
			if (!scenes[7] && uses == 37) { capture(7); return; } // The same native town must receive the second model's actual fresh output, without a duplicate town account.
			if (!scenes[8] && uses == 74) {
				for (uint scene = 0; scene < 8; ++scene) REQUIRE(scenes[scene]);
				capture(8); fmt::print("PLANT_SERVICE_COMPLETE original={} final={} frames={} original_vent=1 models=2 uses=74,74 ports_paid={}\n", original_tick, TimerGameTick::counter, frames, invoices.base()); return;
			}
			REQUIRE(uses < 74);
		} catch (...) { failure = std::current_exception(); started = false; _exit_game = true; }
	};
	AutoRestoreBackup startup{_rebuilt_service_start, &begin}; AutoRestoreBackup observer{_rebuilt_service_frame, &frame}; IConsole::CmdRegister("plant_rebuilt_service", StartRebuiltPlantService);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "plant-rebuilt-service:ticks=1005", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(completed);
}
