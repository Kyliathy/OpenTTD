/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_replacement_delivery.cpp Actual paid replacement carriage and native labour in the same original paid first-plant world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CCCC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Only ordinary complete game frames and actual departure/depot commands earn movement, delivery and work. No source/stock/pose/clock/account repair.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "plant_rebuild_delivery_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../misc_cmd.h"
#include "../vehicle_cmd.h"
#include "../openttd.h"
#include "../video/null_v.h"
#include "../gfx_func.h"
#include "../window_func.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static std::function<void()> *_replacement_start = nullptr; ///< Invocation-only ordinary afterload callback; never a saved transport/construction owner.
static std::function<void()> *_replacement_frame = nullptr; ///< Observation only after the complete ordinary game/input/window sequence.

/** Explicit native null driver with a bounded completed-frame observation point; it changes no timer or simulation scheduling authority. */
class PlantReplacementDriver final : public VideoDriver_Null {
public:
	/** Run complete normal frames until success/refusal or the bounded actual route budget, without assigning any tick, vehicle pose or production quantity. */
	void MainLoop() override
	{
		// Source stock, native train orders, physical crossings and Industry
		// construction progress advance through the same ordinary world frame.
		for (uint frame = 0; frame < 12005; ++frame) {
			if (_exit_game) break;
			::GameLoop(); ::InputLoop(); ::UpdateWindows();
			if (_exit_game) break;
			if (_replacement_frame != nullptr) (*_replacement_frame)();
		}
	}
	/** @return Explicit-only scenario driver name; production null selection stays unchanged. */
	std::string_view GetName() const override { return "plant-replacement-delivery"; }
};

/** Zero-priority explicit-only registration for the native completed-frame observer, never an automatic production video override. */
class PlantReplacementFactory final : public DriverFactoryBase {
public:
	/** Register the actual native null driver with a completed-world observation callback. */
	PlantReplacementFactory() : DriverFactoryBase(Driver::Type::Video, 0, "plant-replacement-delivery", "Original plant replacement delivery observer") {}
	/** @return Ordinary native null driver with the bounded observer callback. */
	std::unique_ptr<Driver> CreateInstance() const override { return std::make_unique<PlantReplacementDriver>(); }
};
static PlantReplacementFactory _plant_replacement_factory; ///< Test binary only; no scenario driver is linked into the production game.

/** @param arguments Native startup command. @return Success after observing the actual ordinary-loaded paid stock world. */
static bool StartPlantReplacement(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_replacement_start != nullptr); (*_replacement_start)(); return true;
}

/** @param paths Invocation-owned actual files. @param world Original actual carrier/machinery. @param scene Observation label(-1 untouched input;0 ready;1..3 loaded layers;4..6 each unfinished work;7 all built;8 whole empty return). @param step Original0 or untouched complete next1. @param invoices Actual saved paid replacement purchase charge. Retain pure repeated native files with canonical observations and independent unsaved client RNG. */
static void SavePlantReplacement(const TestArtifactPaths &paths, const PlantRebuildCargoWorld &world, int scene, uint step, Money invoices)
{
	const auto snapshot = CapturePlantRebuildDelivery(world, invoices); const std::array random{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	const auto path = paths.Output("saves", fmt::format("plant-replacement-{}-{}.sav", scene, step)); const auto repeat = paths.Output("saves", fmt::format("plant-replacement-{}-{}-repeat.sav", scene, step));
	// Repeated pure saves preserve every complete native chunk independently
	// of the canonical relevant-owner snapshot, including genuine saved effects.
	for (const auto &file : {path, repeat}) {
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup format{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(file), false) == SaveLoadResult::Ok);
	}
	CHECK(CapturePlantRebuildDelivery(world, invoices) == snapshot); CHECK((std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random));
	fmt::print("PLANT_DELIVERY_WORLD scene={} step={} snapshot={} path={} repeat={}\n", scene, step, snapshot, path.string(), repeat.string());
}

/** Actual conventional carriage earns every paid replacement bill/work and whole empty return; complete next boundaries retain the same uninterrupted native opportunity. */
TEST_CASE("Original paid replacement stock earns native delivery construction and whole empty return", "[.geothermal-first-plant-replacement-delivery]")
{
	const char *input = std::getenv("OTTD_PLANT_REPLACEMENT_INPUT"); REQUIRE(input != nullptr); const auto paths = GetTestCaseArtifactPaths("geothermal", "replacement-delivery");
	PlantRebuildCargoWorld world; bool started = false, completed = false, withdrawing = false; std::exception_ptr failure; uint frames = 0;
	std::array<bool, 9> scenes{}; int pending = -1; uint64_t pending_tick = 0, original_tick = 0, geology = 0; Money invoices = 0, original_income = 0;
	std::vector<UtilityPowerProducer> retired;
	/** @param scene Actual newly earned observation; the label never changes gameplay. Retain original0 now, with its full next opportunity due before any further observer command. */
	const auto capture = [&](uint scene) {
		REQUIRE(pending == -1); REQUIRE_FALSE(scenes[scene]); SavePlantReplacement(paths, world, static_cast<int>(scene), 0, invoices);
		scenes[scene] = true; pending = static_cast<int>(scene); pending_tick = TimerGameTick::counter;
	};
	/** Resolve the exact original paid stock/participants, use actual departure/unpause commands and retain their ready world; no purchase or other builder runs here. */
	std::function<void()> begin = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error)); world = LoadPlantRebuildCargoWorld();
			REQUIRE(PlantRebuildWholeEmptyHome(world)); REQUIRE(CargoPacket::GetNumItems() == 4);
			const auto history = GetSurfaceSupplyOrderHistory(); REQUIRE(history.size() >= 4);
			// Only the last four actual paid orders are this new replacement bill.
			// Saved native prices retain its historical charge through future days.
			for (size_t index = history.size() - 4; index < history.size(); ++index) {
				const auto &order = history[index]; REQUIRE(order.station == world.origin); REQUIRE(order.quantity == PlantRebuildWholeBill(order.label)); REQUIRE(order.dispatched == 0);
				invoices += order.unit_price * order.quantity;
			}
			REQUIRE(invoices > Money{0}); retired = CaptureUtilityPowerState().producers; REQUIRE(retired.size() == 3);
			original_tick = TimerGameTick::counter; geology = ComputeGeologyStateDigest(GetGeologyWorldState()); original_income = PlantRebuildGrossIncome(world.carrier->owner);
			SavePlantReplacement(paths, world, -1, 0, invoices);
			REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, world.carrier->index, true).Succeeded());
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
			capture(0); started = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	/** Observe only a complete ordinary frame; retain pending next worlds before any further capture/depot command, allowing normal native source/packet/effect work. */
	std::function<void()> frame = [&] {
		if (!started) return;
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; ++frames;
			if (pending != -1) {
				// Ordinary graph/loading pauses may consume a presentation frame
				// without a simulation opportunity. Never fake the missing tick.
				if (TimerGameTick::counter == pending_tick) return;
				REQUIRE(TimerGameTick::counter == pending_tick + 1); const int previous = pending; pending = -1;
				SavePlantReplacement(paths, world, previous, 1, invoices);
				if (previous == 8) { completed = true; started = false; _exit_game = true; return; }
			}
			REQUIRE_FALSE(world.carrier->vehstatus.Test(VehState::Crashed)); CHECK(PlantRebuildGrossIncome(world.carrier->owner) == original_income);
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			for (const auto label : PLANT_REBUILD_LABELS) CheckPlantRebuildMaterial(world, label);
			// All old consumed generation/use survives native epoch rotation.
			// New construction must never revive a retired original source.
			for (const auto &old : retired) {
				const auto *current = GetUtilityPowerProducer(old.meter.producer_id); REQUIRE(current != nullptr); CHECK_FALSE(current->active);
				CHECK(current->reserved_j == 0); CHECK(current->generated_lifetime_j == old.generated_lifetime_j); CHECK(current->consumed_lifetime_j == old.consumed_lifetime_j);
			}
			const uint aboard = PlantRebuildAboard(world); std::array<uint16_t, 3> work{};
			work[0] = GetGeothermalCollectorState(world.machines[0])->work_ticks;
			work[1] = GetGeothermalGeneratorState(world.machines[1])->work_ticks; work[2] = GetGeothermalGeneratorState(world.machines[2])->work_ticks;
			if (frames % 1000 == 0) fmt::print("PLANT_DELIVERY_PROGRESS frames={} tick={} work={},{},{} aboard={} tile={} layer={} order={}\n", frames, TimerGameTick::counter, work[0], work[1], work[2], aboard, world.carrier->tile.base(), to_underlying(GetTileLayer(world.carrier->tile)), to_underlying(world.carrier->current_order.GetType()));
			// Keep genuine visible positive-speed loaded movement on all three
			// planes, using actual wagon cargo even during a physical crossing.
			const bool moving = aboard > 0 && world.carrier->cur_speed > 0 && !world.carrier->vehstatus.Test(VehState::Hidden) && !world.carrier->current_order.IsType(OT_LOADING);
			const std::array layers{MapLayer::Surface, MapLayer::Underground, MapLayer::DeepUnderground};
			for (uint index = 0; index < layers.size(); ++index) {
				if (!scenes[index + 1] && moving && GetTileLayer(world.carrier->tile) == layers[index] && PlantRebuildAboard(world, layers[index]) > 0) { capture(index + 1); return; }
			}
			// Each actual model's unfinished paid labour is observed separately.
			// Labels cannot force simultaneous or differently ordered native work.
			for (uint index = 0; index < work.size(); ++index) {
				const auto ticks = GetMultiLayerGeothermalConstruction(PLANT_REBUILD_KINDS[index])->work_ticks;
				if (!scenes[index + 4] && work[index] > 0 && work[index] < ticks) { capture(index + 4); return; }
			}
			const bool built = PlantRebuildAllConstructed(world);
			if (!scenes[7] && built) { capture(7); return; } // A whole native construction receipt precedes any observer withdrawal command.
			// An actual empty Surface return past the supplier precedes depot
			// withdrawal; no vehicle position/order cursor is repaired or assigned.
			if (!withdrawing && built && aboard == 0 && GetTileLayer(world.carrier->tile) == MapLayer::Surface && world.carrier->last_station_visited == world.origin && !world.carrier->current_order.IsType(OT_LOADING)) {
				REQUIRE(Command<Commands::SendVehicleToDepot>::Do(DoCommandFlag::Execute, world.carrier->index, DepotCommandFlags{}, VehicleListIdentifier{}).Succeeded()); withdrawing = true; return;
			}
			if (withdrawing && PlantRebuildWholeEmptyHome(world) && !scenes[8]) {
				for (uint scene = 0; scene < 8; ++scene) REQUIRE(scenes[scene]);
				for (const auto label : PLANT_REBUILD_LABELS) {
					const auto ledger = CheckPlantRebuildMaterial(world, label); CHECK(ledger.physical == 0); CHECK(ledger.source == 0); CHECK(ledger.dispatched == ledger.bought);
				}
				CHECK(world.carrier->cargo_payment == nullptr); capture(8);
				fmt::print("PLANT_DELIVERY_COMPLETE original={} final={} frames={} whole_empty=true models=3 original_units=92 no_freight_income=true\n", original_tick, TimerGameTick::counter, frames);
			}
		} catch (...) { failure = std::current_exception(); started = false; _exit_game = true; }
	};
	AutoRestoreBackup startup{_replacement_start, &begin}; AutoRestoreBackup tick{_replacement_frame, &frame}; IConsole::CmdRegister("plant_replacement_delivery", StartPlantReplacement);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "plant-replacement-delivery:ticks=12005", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(completed);
}
