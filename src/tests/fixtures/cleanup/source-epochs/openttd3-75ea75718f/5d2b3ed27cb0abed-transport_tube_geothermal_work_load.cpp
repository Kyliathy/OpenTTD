/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal_work_load.cpp Ordinary434 files of actual capsule work, source switching and uninterrupted next-step baselines.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups6BDDB/6BDDC/14B/14EC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Paid machinery/cable/capsules use their actual owners; prepared floor/stock/portal inputs remain separate from normal bootstrap.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "geothermal_tube_switch_fixture.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include "../console_internal.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../safeguards.h"

/** @param capsule Actual native front. @param label Observation boundary. Emit only rebuildable physics diagnostics to the focused native log; no cache or saved owner is changed. */
static void LogGeothermalWorkPhysics(const Train &capsule, std::string_view label)
{
	fmt::print("WORK_PHYS {} tick={} speed={}:{} progress={} model={} power={} weight={} max_te={} drag={} accel={} actual_accel={} freight={}\n",
		label, TimerGameTick::counter, capsule.cur_speed, capsule.subspeed, capsule.progress, to_underlying(_settings_game.vehicle.train_acceleration_model),
		capsule.gcache.cached_power, capsule.gcache.cached_weight, capsule.gcache.cached_max_te, capsule.gcache.cached_air_drag, capsule.acceleration,
		capsule.GetAcceleration(), _settings_game.vehicle.freight_trains);
}

/** @param capsule Actual original purchased front. @param packet Original loaded native packet. Check physical/quantity owners without advancing work or repairing references. */
static void CheckGeothermalWorkFileOwners(const Train &capsule, CargoPacketID packet)
{
	REQUIRE(capsule.cargo.StoredCount() == 17); REQUIRE(capsule.cargo.Packets()->size() == 1);
	CHECK(capsule.cargo.Packets()->front()->index == packet); CHECK(IsTrainOnItsTiles(&capsule));
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	CHECK(ValidateTubeGeothermalPowerState(CaptureTubeGeothermalPowerState(), CaptureUtilityPowerState()));
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** @param capsule Actual native front. Advance the same complete common preparation and train opportunity used by the uninterrupted writer, with no synthetic allocation or relocation. */
static void ContinueGeothermalWorkFile(Train &capsule)
{
	UpdateSignalsInBuffer(); ++TimerGameTick::counter; PrepareTubePowerTick();
	REQUIRE(capsule.Tick()); REQUIRE_FALSE(capsule.vehstatus.Test(VehState::Crashed)); CHECK(IsTrainOnItsTiles(&capsule));
}

/** Actual work/source checkpoints and one uninterrupted next opportunity per checkpoint retain all original native accounting and packets. */
TEST_CASE("Native geothermal work writes ordinary switched capsule worlds", "[tube-geothermal-work-save-file]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTubeSwitchRig rig{true};
	// A zero-initialized unit-test multiplier is outside the native setting's
	// minimum1. Ordinary loading clamps it and rebuilds weight/acceleration;
	// use the real legal setting before purchase/loading to compare like worlds.
	AutoRestoreBackup<uint8_t> freight{_settings_game.vehicle.freight_trains, uint8_t{1}};
	// The lightweight construction fixture installs labels, not the original
	// climate cargo properties. Native Temperate Coal is one tonne per unit;
	// declare that real mass before loading so afterload rebuilds the same physics.
	const auto coal = GetCargoTypeByLabel(CT_COAL); REQUIRE(coal != INVALID_CARGO);
	CargoSpec::Get(coal)->weight = 16; CargoSpec::Get(coal)->is_freight = true;
	LayerSaveContent content{rig.ActivatedContent(), true}; AutoRestoreBackup local{_local_company, _current_company};
	(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
	auto *capsule = rig.Buy(); const auto packet = rig.Load(*capsule);
	REQUIRE(BindTubeGeothermalPowerVehicle(capsule->index, rig.geothermal.source).has_value()); rig.Start(*capsule, rig.inlets[1]);
	const auto paths = GetTestCaseArtifactPaths("tubes", "geothermal-work-files");
	/** @param phase Actual motion/switch/wait/service checkpoint. @param step Zero is the original; one is its uninterrupted next native opportunity. Save without changing simulation, money or packet owners. */
	const auto save = [&](unsigned phase, unsigned step) {
		LogGeothermalWorkPhysics(*capsule, fmt::format("writer-{}-{}", phase, step));
		CheckGeothermalWorkFileOwners(*capsule, packet);
		const auto utility = ComputeUtilityPowerDigest(), source = ComputeTubeGeothermalPowerDigest(), native = ComputeGeothermalGeneratorDigest();
		const auto binding = ComputeTubePowerVehicleDigest(), invoice = ComputeTubePowerDigest();
		const auto *row = GetTubePowerVehicle(capsule->index); REQUIRE(row != nullptr);
		SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
		AutoRestoreBackup menu{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		const auto path = paths.Output("saves", fmt::format("geothermal-work-{}-{}.sav", phase, step));
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
		CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubeGeothermalPowerDigest() == source); CHECK(ComputeGeothermalGeneratorDigest() == native);
		CHECK(ComputeTubePowerVehicleDigest() == binding); CHECK(ComputeTubePowerDigest() == invoice);
		fmt::print("GEOTHERMAL_WORK_SAVE phase={} step={} source={} binding={} utility={} native={} invoice={} packet={} vehicle={} tick={} carry={} stopped={} path={}\n",
			phase, step, source, binding, utility, native, invoice, packet.base(), capsule->index.base(), TimerGameTick::counter, row->remainder_mj,
			capsule->vehstatus.Test(VehState::Stopped) ? 1 : 0, path.string());
	};
	/** @param phase Current actual native checkpoint. Record an ordinary file and its uninterrupted next complete opportunity, without restoring or substituting private state. */
	const auto checkpoint = [&](unsigned phase) { save(phase, 0); rig.Run(std::array{capsule}); save(phase, 1); };
	unsigned ticks = 0;
	// A real departure precedes the partially used file; neither position nor
	// energy is supplied by the observer, save writer or fixture relocation.
	while (capsule->IsInDepot() && ticks++ < 1000) rig.Run(std::array{capsule});
	REQUIRE(ticks < 1000); REQUIRE(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j > 0); checkpoint(0);
	rig.StopOnRoute(*capsule); REQUIRE(rig.Switch(*capsule, false).has_value());
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded()); rig.Run(std::array{capsule}); checkpoint(1);
	rig.StopOnRoute(*capsule); REQUIRE(rig.Switch(*capsule, true).has_value());
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded()); rig.Run(std::array{capsule}); checkpoint(2);
	const auto wire = TileXYL(34, 24, MapLayer::DeepUnderground);
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, wire, UtilityEnergyDomain::Electricity).Succeeded());
	rig.Run(std::array{capsule}); CHECK(GetTubePowerVehicle(capsule->index)->wait == TubePowerWait::Topology);
	CHECK_FALSE(capsule->vehstatus.Test(VehState::Stopped)); checkpoint(3);
	rig.Build(wire, UtilityEnergyDomain::Electricity); ticks = 0;
	// Recovery resumes the same orders and actual loaded capsule; the native
	// depot order owns arrival/halting before damaged service can be requested.
	while ((!capsule->IsInDepot() || capsule->tile != rig.inlets[1]) && ticks++ < 1500) rig.Run(std::array{capsule});
	REQUIRE(ticks < 1500); REQUIRE(capsule->vehstatus.Test(VehState::Stopped));
	capsule->reliability = 100; rig.Run(std::array{capsule}); REQUIRE(capsule->reliability == capsule->GetEngine()->reliability); checkpoint(4);
	REQUIRE(rig.Switch(*capsule, false).has_value()); capsule->reliability = 100; rig.Run(std::array{capsule});
	REQUIRE(capsule->reliability == capsule->GetEngine()->reliability); checkpoint(5);
}

static std::function<void()> *_geothermal_work_file_observer = nullptr; ///< Process-local afterload observation only; it never becomes a simulation or resource owner.

/** @param arguments Actual ordinary startup command. @return Success after the registered observer runs against native loaded owners. */
static bool ObserveGeothermalWorkFile(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_geothermal_work_file_observer != nullptr); (*_geothermal_work_file_observer)(); return true;
}

/** @param name Explicit independent runner expectation. @return Bounded unsigned scalar; malformed/missing environment fails before loading or mutating a world. */
static uint64_t GeothermalWorkFileExpected(const char *name)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr);
	uint64_t value = 0; const auto parsed = std::from_chars(text, text + strlen(text), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text + strlen(text)); return value;
}

/**
 * Verify every original queue participant after ordinary loading without changing any owner.
 * @param primary Original reservation winner, already checked against the writer expectation.
 * @return Both genuine front engines in native pool order, retaining distinct original packets.
 */
static std::vector<Train *> GetLoadedTubeQueueFronts(const Train &primary)
{
	const auto vehicle = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_SECONDARY_VEHICLE");
	const auto packet = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_SECONDARY_PACKET");
	REQUIRE(vehicle < UINT32_MAX); REQUIRE(packet < UINT32_MAX);
	auto *secondary = Train::GetIfValid(VehicleID{static_cast<uint32_t>(vehicle)});
	REQUIRE(secondary != nullptr); REQUIRE(secondary != &primary);
	CheckGeothermalWorkFileOwners(*secondary, CargoPacketID{static_cast<uint32_t>(packet)});
	REQUIRE(secondary->cargo.Packets()->front()->index != primary.cargo.Packets()->front()->index);
	std::vector<Train *> fronts;
	// The actual native pool owns participant identity and iteration order.
	// Inspect all bound fronts so a missing competitor or unexpected extra
	// participant cannot be hidden by a primary-only observation or tick.
	for (auto *front : Train::Iterate()) {
		if (!front->IsFrontEngine()) continue;
		if (GetTubePowerVehicle(front->index) == nullptr) continue;
		REQUIRE((front == &primary || front == secondary));
		if (!fronts.empty()) REQUIRE(fronts.back()->index < front->index);
		fronts.push_back(front);
	}
	REQUIRE(fronts.size() == 2);
	fmt::print("TUBE_QUEUE_LOADED primary={} secondary={} primary_packet={} secondary_packet={} fronts=2\n",
		primary.index.base(), secondary->index.base(), primary.cargo.Packets()->front()->index.base(), packet);
	return fronts;
}

/**
 * Continue all original queue fronts through one shared native opportunity.
 * @param fronts Actual loaded native front engines in their verified stable pool order.
 */
static void ContinueLoadedTubeQueue(std::span<Train *const> fronts)
{
	UpdateSignalsInBuffer(); ++TimerGameTick::counter; PrepareTubePowerTick();
	// Gather all demand once before either actual front moves. Native Tick
	// alone owns reservation arbitration, braking and position; preserve each
	// original packet across the same common opportunity as the producer.
	for (auto *front : fronts) {
		const auto packet = front->cargo.Packets()->front()->index;
		REQUIRE(front->Tick()); REQUIRE_FALSE(front->vehstatus.Test(VehState::Crashed));
		CheckGeothermalWorkFileOwners(*front, packet);
	}
	fmt::print("TUBE_QUEUE_STEP fronts={} tick={}\n", fronts.size(), TimerGameTick::counter);
}

/**
 * Rebuild only derived loaded connectivity between two ordinary world saves.
 * @param capsule Original accepted native front, with its actual occupied track and layer.
 * @param before New before-save path within this invocation's generated review directory.
 */
static void RebuildLoadedTubeWorkCaches(const Train &capsule, const std::filesystem::path &before)
{
	AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(before), false) == SaveLoadResult::Ok);
	const auto external = GetTubePowerConnections();
	const auto geothermal = GetTubeGeothermalSources();
	REQUIRE(external.size() == 1); REQUIRE(geothermal.size() == 1);
	const auto occupied = capsule.track == Track::Depot ? TrackBits{} : capsule.track;
	/** @return Both actual source families' whole-consist and occupied-tile reach, without allocating work or advancing native time. */
	const auto observe = [&] {
		std::vector<bool> result;
		// Existing original source identities retain their physical admission;
		// queries cannot substitute the selected consumer's grant for topology.
		for (const auto &source : external) {
			result.push_back(IsTubePowerConnected(source.producer, capsule));
			result.push_back(IsTubePowerConnectedTile(source.producer, capsule.tile, occupied));
		}
		for (const auto &source : geothermal) {
			result.push_back(IsTubeGeothermalPowerConnected(source.id, capsule));
			result.push_back(IsTubeGeothermalPowerConnectedTile(source.id, capsule.tile, occupied));
		}
		return result;
	};
	const auto expected = observe();
	REQUIRE(expected.size() == 4);
	// These retained worlds have one original live external inlet and one
	// original geothermal inlet. The real cable-loss scene must retain external
	// reach while refusing native reach; an always-false lookup cannot pass.
	REQUIRE(expected[0]); REQUIRE(expected[1]);
	const bool native_connected = GetTubeGeothermalSourcePath(geothermal.front().id).has_value();
	CHECK(expected[2] == native_connected); CHECK(expected[3] == native_connected);
	// Each cold rebuild and immediate warm lookup must derive the same real
	// component. Clearing a cache never revokes a grant, moves cargo or pays a
	// fee; the full before/after transport proves every owner stayed unchanged.
	for (uint repeat = 0; repeat < 3; ++repeat) {
		ResetTubePowerTopology();
		CHECK(observe() == expected); CHECK(observe() == expected);
	}
	fmt::print("GEOTHERMAL_WORK_CACHE external={} native={} rebuilds=3 external_connected=true native_connected={} before={}\n",
		external.size(), geothermal.size(), native_connected, before.string());
}

/** Ordinary executable startup preserves complete original state, then optionally continues the exact next actual native opportunity. */
TEST_CASE("Native geothermal switched work continues ordinary files", "[.tube-geothermal-work-native-file]")
{
	const char *input = std::getenv("OTTD_GEOTHERMAL_WORK_INPUT"), *action_text = std::getenv("OTTD_GEOTHERMAL_WORK_ACTION");
	REQUIRE(input != nullptr); REQUIRE(action_text != nullptr); const std::string_view action{action_text};
	REQUIRE((action == "inspect" || action == "step" || action == "step-all" || action == "cache" || action == "preview-refusal" || action == "full-refusal"));
	const auto source = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_SOURCE"), binding = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_BINDING");
	const auto utility = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_UTILITY"), native = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_NATIVE");
	const auto invoice = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_INVOICE"), packet = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_PACKET");
	const auto vehicle = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_VEHICLE"), tick = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_TICK");
	const auto carry = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_CARRY"), stopped = GeothermalWorkFileExpected("OTTD_GEOTHERMAL_WORK_STOPPED");
	REQUIRE(vehicle < UINT32_MAX); REQUIRE(packet < UINT32_MAX); REQUIRE(carry < 1000); REQUIRE(stopped <= 1);
	const auto paths = GetTestCaseArtifactPaths("tubes", "geothermal-work-files"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect accepted originals before simulation; the optional continuation matches the writer's real native driver rather than repairing file state. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			/** @return Reacquired original capsule only after every expected owner, packet, carry, clock and player-stop check; full refusal/retry cannot retain a borrowed old pool pointer. */
			const auto check_loaded = [&]() -> Train * {
				REQUIRE(ComputeTubeGeothermalPowerDigest() == source); REQUIRE(ComputeTubePowerVehicleDigest() == binding);
				REQUIRE(ComputeUtilityPowerDigest() == utility); REQUIRE(ComputeGeothermalGeneratorDigest() == native); REQUIRE(ComputeTubePowerDigest() == invoice);
				auto *loaded = Train::GetIfValid(VehicleID{static_cast<uint32_t>(vehicle)}); REQUIRE(loaded != nullptr);
				CheckGeothermalWorkFileOwners(*loaded, CargoPacketID{static_cast<uint32_t>(packet)});
				REQUIRE(TimerGameTick::counter == tick); REQUIRE(GetTubePowerVehicle(loaded->index)->remainder_mj == carry);
				REQUIRE(loaded->vehstatus.Test(VehState::Stopped) == (stopped != 0));
				if (std::getenv("OTTD_GEOTHERMAL_WORK_SECONDARY_VEHICLE") != nullptr) GetLoadedTubeQueueFronts(*loaded);
				return loaded;
			};
			auto *capsule = check_loaded();
			LogGeothermalWorkPhysics(*capsule, "loaded");
			const auto cash = Company::Get(_current_company)->money; const auto random = std::array{_random.state[0], _random.state[1]};
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			// Independent preview must leave the running world intact; an actual
			// failed full load destroys/reinitializes it, then the original ordinary
			// file must restore every owner before another work opportunity exists.
			if (action == "preview-refusal" || action == "full-refusal") {
				const char *bad = std::getenv("OTTD_GEOTHERMAL_WORK_CORRUPT"); REQUIRE(bad != nullptr);
				const char *expected_error = std::getenv("OTTD_GEOTHERMAL_WORK_ERROR"); REQUIRE(expected_error != nullptr);
				if (action == "preview-refusal") {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(_load_check_data.error_msg.find(expected_error) != std::string::npos);
					fmt::print("GEOTHERMAL_WORK_REFUSAL action={} reason={}\n", action, _load_check_data.error_msg);
				} else {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
					const auto reason = GetSaveLoadErrorMessage().GetDecodedString();
					CHECK(reason.find(expected_error) != std::string::npos);
					fmt::print("GEOTHERMAL_WORK_REFUSAL action={} reason={}\n", action, reason);
					REQUIRE(SaveOrLoad(input, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				}
				capsule = check_loaded();
				// A refused copied preview must also release its temporary family
				// presence; selecting the original again is an independent valid check.
				REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				capsule = check_loaded();
			}
			CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubePowerVehicleDigest() == binding); CHECK(ComputeTubeGeothermalPowerDigest() == source);
			CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(ComputeTubePowerDigest() == invoice); CHECK(Company::Get(_current_company)->money == cash);
			CHECK(TimerGameTick::counter == tick); CHECK(std::array{_random.state[0], _random.state[1]} == random);
			if (action == "cache") {
				RebuildLoadedTubeWorkCaches(*capsule, paths.Output("saves", "tube-cache-before.sav"));
				capsule = check_loaded();
				CHECK(Company::Get(_current_company)->money == cash);
				CHECK(std::array{_random.state[0], _random.state[1]} == random);
			}
			if (action == "step") {
				AutoRestoreBackup pause{_pause_mode, PauseModes{}}; PathfinderDefaults pathfinder;
				ContinueGeothermalWorkFile(*capsule);
				LogGeothermalWorkPhysics(*capsule, "continued");
				CheckGeothermalWorkFileOwners(*capsule, CargoPacketID{static_cast<uint32_t>(packet)});
			}
			if (action == "step-all") {
				AutoRestoreBackup pause{_pause_mode, PauseModes{}}; PathfinderDefaults pathfinder;
				ContinueLoadedTubeQueue(GetLoadedTubeQueueFronts(*capsule));
				LogGeothermalWorkPhysics(*capsule, "continued-all");
			}
			AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
			const auto saved = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action));
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok);
			fmt::print("GEOTHERMAL_WORK_READ action={} path={}\n", action, saved.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_geothermal_work_file_observer, &inspect}; IConsole::CmdRegister("geothermal_work_file_check", ObserveGeothermalWorkFile);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
