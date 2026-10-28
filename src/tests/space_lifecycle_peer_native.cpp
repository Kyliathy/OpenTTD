/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_lifecycle_peer_native.cpp Actual joined peers and local replay of native exit/renewal ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group32.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group32B; parameterized native source identity/holding preserves genuine high-ID packet history.
 * @see docs/zz-reach2/architecture/desync/archi-desync.md Invariants.
 * Extends the socket/startup/checkpoint pattern in probe_replay_native.cpp.
 * Geometry and original paid packets come from the earlier real phase save;
 * only registered company-access and mining-policy commands change its setup.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../company_base.h"
#include "../company_cmd.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../console_func.h"
#include "../core/backup_type.hpp"
#include "../core/string_consumer.hpp"
#include "../launch_center.h"
#include "../launch_operations.h"
#include "../map_layer_gui.h"
#include "../network/network.h"
#include "../network/network_internal.h"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "../space_manifest.h"
#include "../space_mining_continuation.h"
#include "../space_pod.h"
#include "../space_structure.h"
#include "../space_transition.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../timer/timer.h"
#include "../timer/timer_game_realtime.h"
#include "../timer/timer_game_tick.h"
#include "../window_func.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *lifecycle_peer_start = nullptr; ///< Invocation-owned startup observer, never a simulation authority.

/** @param args Exact registered startup command tokens. @return Whether the actual loaded-world observer ran. */
static bool StartLifecyclePeer(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(lifecycle_peer_start); (*lifecycle_peer_start)(); return true;
}

/** @return Original local company's paid heavy miner, or nullptr while a joined map is still being reconstructed. */
static SpaceVehicle *LifecyclePeerMiner()
{
	// Resolve an actual original role rather than inventing another hull record
	// or relying on dense asteroid indices during native join reconstruction.
	for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 21) return craft;
	return nullptr;
}

/** @param owner Original paid hull's company. @return Fifteen synchronized clock, field, mission, cargo, work, endpoint, global RNG and money observations; the field digest includes both complete private streams. */
static std::array<uint64_t, 15> ObserveLifecyclePeer(CompanyID owner)
{
	return {TimerGameTick::counter, ComputeAsteroidFieldDigest(GetAsteroidField()), ComputeSpaceVehicleDigest(), ComputeSpaceOrdersDigest(),
		ComputeSpaceStructureDigest(), ComputeSpaceManifestDigest(), ComputeSpacePodDigest(), ComputeSpaceDescentDigest(), ComputeLaunchCenterDigest(),
		ComputeSpaceTransitionDigest(), ComputeLaunchProductionDigest(), ComputeSurfaceHandlingDigest(),
		_random.state[0], _random.state[1], static_cast<uint64_t>(Company::Get(owner)->money)};
}

/** @param output Existing private invocation directory. @param name Actual reached native phase. Save/check original owners while retaining the actual world and ordinary clock. */
static void SaveLifecyclePeer(const std::filesystem::path &output, std::string_view name)
{
	AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
	const auto saved = (output / fmt::format("{}.sav", name)).string();
	REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
}

/** Original checked client commands and native join transport must preserve the complete lifecycle, beyond the global RNG sync words. */
TEST_CASE("Native lifecycle converges across an owning client late join and local replay", "[.space-lifecycle-peers]")
{
	const char *source = std::getenv("OTTD_LIFECYCLE_SOURCE"), *config = std::getenv("OTTD_LIFECYCLE_CONFIG");
	const char *selected = std::getenv("OTTD_LIFECYCLE_MODE"), *directory = std::getenv("OTTD_LIFECYCLE_OUTPUT");
	const char *epoch = std::getenv("OTTD_LIFECYCLE_EPOCH"), *issuance = std::getenv("OTTD_LIFECYCLE_ISSUANCE");
	const char *body_id = std::getenv("OTTD_LIFECYCLE_BODY"), *quantity = std::getenv("OTTD_LIFECYCLE_HOLDING");
	REQUIRE(source); REQUIRE(config); REQUIRE(selected); REQUIRE(directory); REQUIRE(epoch); REQUIRE(issuance);
	REQUIRE(body_id); REQUIRE(quantity);
	const std::string_view mode{selected};
	// A scope typo must fail before loading, binding a listener or changing the
	// private cloned world's access policy. Only these actual peer/local modes exist.
	REQUIRE((mode == "server" || mode == "client" || mode == "observer" || mode == "local" || mode == "local-reversed"));
	const bool local = mode.starts_with("local"); const bool server = mode == "server";
	const auto parsed_epoch = ParseInteger<uint64_t>(epoch); const auto parsed_issuance = ParseInteger<uint32_t>(issuance);
	REQUIRE(parsed_epoch); REQUIRE(parsed_issuance); const auto base_elapsed = *parsed_epoch; const auto initial_issued = *parsed_issuance;
	const auto parsed_body = ParseInteger<uint32_t>(body_id); const auto parsed_holding = ParseInteger<uint16_t>(quantity);
	REQUIRE(parsed_body); REQUIRE(parsed_holding); const OrbitalBodyID source_body{*parsed_body}; const auto initial_holding = *parsed_holding;
	const std::filesystem::path output{directory};
	std::ofstream trace(output / "opportunities.tsv"), checkpoints(output / "checkpoints.tsv"); REQUIRE(trace.good()); REQUIRE(checkpoints.good());
	bool ready = false, submitted = false, unpaused = false, observed = false, view_selected = false;
	bool departed = false, credited = false, joined = false, expiring = false;
	uint64_t completed = 0; std::exception_ptr failure;
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(95);
	/** @param phase Exact physical boundary shared by named elapsed/frame witnesses. */
	auto checkpoint = [&](std::string_view phase) {
		auto *craft = LifecyclePeerMiner(); REQUIRE(craft); const auto before = ObserveLifecyclePeer(craft->owner);
		SaveLifecyclePeer(output, phase); CHECK(ObserveLifecyclePeer(craft->owner) == before);
		checkpoints << phase << '\t' << _frame_counter << '\t' << GetAsteroidField().elapsed_ticks;
		for (const auto value : before) checkpoints << '\t' << value;
		checkpoints << '\n'; checkpoints.flush(); REQUIRE(checkpoints.good());
		fmt::print("LIFECYCLE_PEER checkpoint={} frame={} tick={} elapsed={} countdown={} continuation={} waiting={} mode={}\n", phase,
			_frame_counter, TimerGameTick::counter, GetAsteroidField().elapsed_ticks, GetAsteroidField().renewal.ticks_remaining,
			to_underlying(craft->mission.continuation.phase), to_underlying(craft->mission.continuation.waiting), mode); std::fflush(stdout);
	};
	/** @param elapsed Actual completed timer callbacks, never installed as mining/renewal time. */
	const IntervalTimer<TimerGameTick> ticks({TimerGameTick::Priority::None, 1}, [&](uint elapsed) {
		// Title startup and partial join reconstruction are not a usable gameplay
		// world. A failed observer must not run again during orderly shutdown.
		if (!ready || failure) return;
		try {
			REQUIRE(elapsed == 1); REQUIRE(std::chrono::steady_clock::now() < deadline);
			auto *craft = LifecyclePeerMiner(); REQUIRE(craft); const auto &field = GetAsteroidField();
			if (!local) { REQUIRE(_networking); REQUIRE(_network_server == server); }
			const auto state = ObserveLifecyclePeer(craft->owner);
			trace << field.elapsed_ticks << '\t' << _frame_counter;
			for (const auto value : state) trace << '\t' << value;
			trace << '\n';
			if (field.elapsed_ticks == base_elapsed) {
				REQUIRE(craft->mission.phase == SpaceMissionPhase::Mining); REQUIRE(craft->mission.target == source_body); REQUIRE(craft->cargo.TotalCount() == initial_holding);
				REQUIRE(craft->space_orders.entries[craft->space_orders.cursor].mining_policy == SpaceMiningPolicy::FullLoadNearest);
				checkpoint("before-detachment");
			}
			// Observe exactly the first ordinary decision after safety release;
			// a later travelling state cannot stand in for this boundary.
			if (!departed && field.elapsed_ticks == base_elapsed + 1) { REQUIRE(craft->mission.phase != SpaceMissionPhase::Mining); checkpoint("after-departure"); departed = true; }
			// The terminal centre crossing must be real before the host permits a
			// second client to join this already armed, genuinely retargeting world.
			if (!credited && GetMutableAsteroidBody(source_body) == nullptr) {
				REQUIRE(field.renewal.credits >= 5); REQUIRE(field.renewal.ticks_remaining != 0);
				REQUIRE(craft->mission.continuation.phase != MiningContinuationPhase::None);
				checkpoint("credit"); credited = true;
			}
			// A fixed shared opportunity after join compares native reconstructed
			// owners while renewal remains armed, independent of render pacing.
			if (!joined && field.elapsed_ticks == base_elapsed + 400) { REQUIRE(field.renewal.ticks_remaining != 0); checkpoint("joined-armed"); joined = true; }
			// The actual last remaining opportunity witnesses the decision before
			// expiry, then only a new issued identity can qualify successful birth.
			if (!expiring && field.renewal.ticks_remaining == 1) { checkpoint("before-expiry"); expiring = true; }
			if (!observed && field.next_body_id == initial_issued + 1) {
				REQUIRE(credited); REQUIRE(joined); REQUIRE(expiring); checkpoint("after-birth"); observed = true; completed = TimerGameTick::counter;
			}
			// Keep the host alive beyond both clients' final acknowledgements;
			// disconnect latency cannot invalidate another child's reached checkpoint.
			if (observed && TimerGameTick::counter >= completed + (server ? 64 : 32)) _exit_game = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	});
	/** @param elapsed Real-time callbacks only poll actual membership/command results; they grant no gameplay opportunity. */
	const IntervalTimer<TimerGameRealtime> polling({std::chrono::milliseconds(50), TimerGameRealtime::Trigger::Always}, [&](auto elapsed) {
		if (local || failure || elapsed == 0) return;
		try {
			REQUIRE(std::chrono::steady_clock::now() < deadline);
			if (!_networking) return;
			auto *craft = LifecyclePeerMiner(); if (craft == nullptr) return;
			// Owning client commands require completed membership; a spectator
			// cannot impersonate the hull's company during native map reconstruction.
			if (!server && _local_company != craft->owner) return;
			if (!view_selected) {
				auto *main = GetMainWindow(); if (!server) REQUIRE(main);
				if (main) SetViewportLayer(main, mode == "client" ? MapLayer::Space : MapLayer::Surface);
				view_selected = true;
			}
			if (mode == "observer" && !ready) {
				REQUIRE(GetAsteroidField().elapsed_ticks > base_elapsed); REQUIRE(GetAsteroidField().elapsed_ticks < base_elapsed + 400);
				REQUIRE(GetAsteroidField().renewal.ticks_remaining != 0); REQUIRE(craft->mission.continuation.phase != MiningContinuationPhase::None);
				fmt::print("LIFECYCLE_PEER late_join elapsed={} countdown={} native_company=true\n", GetAsteroidField().elapsed_ticks, GetAsteroidField().renewal.ticks_remaining); std::fflush(stdout);
			}
			ready = true;
			// Only the already joined owning client submits this checked edit,
			// exactly once. The host observes the later real command result.
			if (mode == "client" && !submitted) {
				REQUIRE(craft->space_orders.entries[craft->space_orders.cursor].mining_policy == SpaceMiningPolicy::LegacyReturn);
				AutoRestoreBackup actor(_current_company, craft->owner);
				REQUIRE(Command<Commands::SetSpaceMiningPolicy>::Post(craft->index, craft->generation, craft->space_orders.cursor, SpaceMiningPolicy::FullLoadNearest));
				submitted = true; fmt::print("LIFECYCLE_PEER client_policy_post native_wire=true\n"); std::fflush(stdout);
			}
			// Host unpauses only after observing the actual checked wire edit.
			// No miner, field timer or motion can advance before that admission.
			if (server && !unpaused && craft->space_orders.entries[craft->space_orders.cursor].mining_policy == SpaceMiningPolicy::FullLoadNearest) {
				IConsoleCmdExec("unpause"); unpaused = true;
			}
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	});
	/** Configure only this cloned host's native access/policy or perform the exact local command before ordinary replay. */
	std::function<void()> startup = [&] {
		try {
			auto *craft = LifecyclePeerMiner(); REQUIRE(craft); AutoRestoreBackup actor(_current_company, craft->owner);
			if (server) {
				REQUIRE(_network_server); REQUIRE(GetAsteroidField().elapsed_ticks == base_elapsed);
				REQUIRE(Command<Commands::CompanyAllowListControl>::Do(DoCommandFlag::Execute, CompanyAllowListCtrlAction::AllowAny, std::string{}).Succeeded());
				REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft->index, craft->generation, craft->space_orders.cursor, SpaceMiningPolicy::LegacyReturn).Succeeded());
				SaveLifecyclePeer(output, "peer-input"); ready = true; fmt::print("LIFECYCLE_PEER ready server=true\n"); std::fflush(stdout);
			} else if (local) {
				if (mode == "local-reversed") {
					auto field = GetAsteroidField(); const auto digest = ComputeAsteroidFieldDigest(field);
					std::ranges::reverse(field.bodies); REQUIRE(RebuildAsteroidBodyLookup(field)); REQUIRE(InstallAsteroidField(std::move(field)));
					CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == digest);
				}
				REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft->index, craft->generation, craft->space_orders.cursor, SpaceMiningPolicy::FullLoadNearest).Succeeded());
				ready = true; AutoRestoreBackup running(_pause_mode, PauseModes{}); const auto initial = GetAsteroidField().elapsed_ticks;
				// Replaying only full ordinary loops retains actual body/vehicle/
				// cargo ordering. Genuine CargoDist pauses earn no hidden interval.
				while (!_exit_game) {
					REQUIRE(GetAsteroidField().elapsed_ticks < initial + 5000); REQUIRE(std::chrono::steady_clock::now() < deadline);
					const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	AutoRestoreBackup hook(lifecycle_peer_start, &startup); IConsole::CmdRegister("space_lifecycle_peer", StartLifecyclePeer);
	std::vector<std::string_view> args{"openttd_test.exe", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config};
	std::string binding;
	if (server) {
		const char *address = std::getenv("OTTD_LIFECYCLE_ADDRESS"); REQUIRE(address); binding = fmt::format("-D{}", address);
		args.insert(args.end(), {binding, "-g", source});
	} else if (!local) {
		const char *address = std::getenv("OTTD_LIFECYCLE_ADDRESS"); REQUIRE(address);
		args.insert(args.end(), {"-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-n", address});
	} else { args.insert(args.end(), {"-v", "null:ticks=2", "-b", "null", "-g", source}); }
	args.insert(args.end(), {"-d", "net=4,desync=2"}); CHECK(openttd_main(args) == 0);
	trace.flush(); checkpoints.flush(); REQUIRE(trace.good()); REQUIRE(checkpoints.good());
	if (failure) std::rethrow_exception(failure);
	CHECK(observed); fmt::print("LIFECYCLE_PEER complete mode={} original_stock_permits=true\n", mode);
}
