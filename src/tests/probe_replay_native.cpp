/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_replay_native.cpp Original paid mining under actual view selection, queued commands and connected peers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group20
 * Observers own no cargo, hull, work, reserve, simulation clock or monetary allowance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../blitter/factory.hpp"
#include "../company_base.h"
#include "../company_cmd.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../fontcache.h"
#include "../network/network.h"
#include "../network/network_internal.h"
#include "../space_pod.h"
#include "../spritecache.h"
#include "../station_base.h"
#include "../timer/timer.h"
#include "../timer/timer_game_tick.h"
#include "assembly_native_fixture.h"
#include "native_art_render_fixture.h"
#include "../safeguards.h"

static std::function<void()> *probe_replay_observer = nullptr; ///< Invocation-local startup observer, without simulation ownership.

static_assert(std::is_same_v<decltype(&CmdSetSpaceMission), CommandCost (*)(DoCommandFlags, VehicleID, uint32_t)>); // Historical wire layout remains unchanged.
static_assert(std::is_same_v<decltype(&CmdSetSpaceMissionChecked), CommandCost (*)(DoCommandFlags, VehicleID, uint64_t, uint32_t)>); // New wire carries the complete native incarnation.

/** @param args Parameter-free actual game-start invocation. @return Whether the observer was invoked. */
static bool ObserveProbeReplay(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(probe_replay_observer); (*probe_replay_observer)(); return true;
}

/** @return Native mission, finite reserve, packet custody, work and endpoint digests for immediate command/render purity checks. */
static auto ProbeReplayAuthority()
{
	return std::array<uint64_t, 11>{ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest(), ComputeSpaceManifestDigest(), ComputeSpacePodDigest(),
		ComputeSpaceTransitionDigest(), ComputeSpaceOrdersDigest(), ComputeAsteroidFieldDigest(GetAsteroidField()), ComputeLaunchProductionDigest(),
		ComputeLaunchCenterDigest(), ComputeSpaceDescentDigest(), ComputeSurfaceHandlingDigest()};
}

/** Dispatch only through registered native admission; synchronized queue frames grant no simulation work.
 * @tparam Tcommand Original typed command whose serializer owns its wire.
 * @tparam Targs Original complete native argument types.
 * @param queued Whether actual server SendNet/distribution/PostFromNet replaces immediate Post.
 * @param company Original command actor.
 * @param args Original arguments, including all applicable incarnation references.
 */
template <Commands Tcommand, typename... Targs>
static void DispatchProbeReplay(bool queued, CompanyID company, Targs... args)
{
	const auto before = ProbeReplayAuthority(); const auto elapsed = GetAsteroidField().elapsed_ticks;
	const auto money = Company::Get(company)->money; const auto random = _random;
	AutoRestoreBackup server(_network_server, queued); AutoRestoreBackup networking(_networking, queued);
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); AutoRestoreBackup frame(_frame_counter); AutoRestoreBackup maximum(_frame_counter_max, _frame_counter);
	if (queued) {
		Command<Tcommand>::SendNet({}, company, args...); NetworkDistributeCommands(); NetworkExecuteLocalCommandQueue();
		CHECK(ProbeReplayAuthority() == before); CHECK(Company::Get(company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
		++_frame_counter; NetworkExecuteLocalCommandQueue(); const auto committed = ProbeReplayAuthority(); const auto paid = Company::Get(company)->money;
		NetworkExecuteLocalCommandQueue(); CHECK(ProbeReplayAuthority() == committed); CHECK(Company::Get(company)->money == paid); NetworkFreeLocalCommandQueue();
	} else {
		REQUIRE(Command<Tcommand>::Post(args...));
	}
	CHECK(GetAsteroidField().elapsed_ticks == elapsed);
}

/** @param paths Actual invocation save/capture owner. @param stage Physically reached checkpoint. @param visible Whether the actual viewport selects Space.
 * @param render Whether this local replay also captures the native viewport. Connected peers retain their ordinary video loops.
 */
static void SaveProbeReplay(const TestArtifactPaths &paths, std::string_view stage, bool visible, bool render)
{
	REQUIRE(ValidateSpaceVehicles()); const auto owner = SpaceVehicle::Get(VehicleID{16})->owner;
	const auto before = ProbeReplayAuthority(); const auto random = _random; const auto money = Company::Get(owner)->money;
	if (render) {
		auto *main = GetMainWindow(); REQUIRE(main); const auto layer = visible ? MapLayer::Space : MapLayer::Surface; SetViewportLayer(main, layer);
		const auto *craft = SpaceVehicle::Get(VehicleID{16}); const auto x = craft->position.x / SPACE_Q16, y = craft->position.y / SPACE_Q16;
		CaptureNativeArtViewport(TileXYL(x, y, layer), ZoomLevel::Min, paths.images / fmt::format("replay-{}.ppm", stage));
	}
	CHECK(ProbeReplayAuthority() == before); CHECK(Company::Get(owner)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("replay-{}-438.sav", stage)), false) == SaveLoadResult::Ok);
	fmt::print("PROBE_REPLAY checkpoint={} frame={} tick={} elapsed={}\n", stage, _frame_counter, TimerGameTick::counter, GetAsteroidField().elapsed_ticks); std::fflush(stdout);
}

/** Offer one complete native opportunity without installing an elapsed interval or calling individual mining/descent owners. */
static void ProbeReplayOpportunity()
{
	const auto initial = GetAsteroidField().elapsed_ticks; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10); AutoRestoreBackup pause(_pause_mode, PauseModes{});
	// Presentation pacing cannot finance movement or work; wait only until the complete actual synchronized game loop advances one field opportunity.
	while (GetAsteroidField().elapsed_ticks == initial) { REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop(); if (GetAsteroidField().elapsed_ticks == initial) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	REQUIRE(GetAsteroidField().elapsed_ticks == initial + 1);
}

/** Actual native view/queued twins and independent connected peers consume the same original paid deployment. */
TEST_CASE("Original paid probe mission converges across views queues and connected peers", "[.probe-replay-native]")
{
	const char *source = std::getenv("OTTD_PROBE_REPLAY_SOURCE"), *selected = std::getenv("OTTD_PROBE_REPLAY_MODE"), *view = std::getenv("OTTD_PROBE_REPLAY_VIEW"); REQUIRE(source); REQUIRE(selected); REQUIRE(view);
	const std::string_view mode{selected}, policy{view};
	// Each invocation claims one exact replay scope; a typo must refuse before loading a world instead of silently selecting another mission or presentation policy.
	REQUIRE((mode == "local" || mode == "queue" || mode == "recall-queue" || mode == "socket-server" || mode == "socket-client"));
	REQUIRE((policy == "visible" || policy == "hidden"));
	const bool visible = policy == "visible", connected = mode.starts_with("socket-");
	const auto paths = GetTestCaseArtifactPaths("space", "probe-replay"); const auto runtime = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	uint8_t stage = 0; uint64_t completed = 0; bool submitted = false, released = false, view_selected = false; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(95);
	/** Observe one connected peer at the same pre-field-update tick boundary; only the real client submits player commands through its socket.
	 * @param elapsed Actual timer opportunities; never installed as mining or descent work.
	 */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&](uint elapsed) {
		// The timer exists before loading/title startup too. Only an actual connected game may read the paid input, and a failed observer must not execute again during orderly shutdown.
		if (!connected || !_networking || failure) return;
		try {
			REQUIRE(elapsed == 1); REQUIRE(std::chrono::steady_clock::now() < deadline);
			REQUIRE(_network_server == (mode == "socket-server")); // A mode label alone cannot qualify an actual native socket endpoint.
			auto *miner = ResolveSpaceCraft({VehicleID{16}, 2});
			if (miner == nullptr) return;
			// Join reconstruction can temporarily expose the map before company registration finishes. Native player commands must wait for actual membership, rather than impersonating an owner from a spectator connection.
			if (mode == "socket-client" && _local_company != miner->owner) return;
			if (!view_selected) {
				auto *main = GetMainWindow();
				// The joined owning client must actually select its requested layer. A dedicated host has no viewport and remains an ordinary headless simulation peer.
				if (mode == "socket-client") REQUIRE(main);
				if (main != nullptr) SetViewportLayer(main, visible ? MapLayer::Space : MapLayer::Surface);
				view_selected = true;
			}
			REQUIRE(miner->manifest); auto *center = ResolveLaunchCenter(miner->construction.home); REQUIRE(center); const auto cargo = GetCargoTypeByLabel(CargoLabel{"NORE"});
			AutoRestoreBackup actor(_current_company, miner->owner);
			// The joined client supplies the real wire command; a server-side imitation would not exercise client-to-server serialization or authorization.
			if (!submitted && mode == "socket-client") { REQUIRE(Command<Commands::SetSpaceMissionChecked>::Post(miner->index, miner->generation, uint32_t{0})); submitted = true; }
			// Retain the first agreed assignment boundary; an independently chosen later frame cannot serve as an exact peer comparison.
			if (stage == 0 && miner->mission.phase == SpaceMissionPhase::Approach) { SaveProbeReplay(paths, "assigned", visible, false); ++stage; }
			// Partial work must be actual synchronized work under the original exclusive lease, before any full-hold return can hide disagreement.
			if (stage == 1 && miner->mission.phase == SpaceMissionPhase::Mining && miner->mission.work_ticks == 37) { SaveProbeReplay(paths, "partial", visible, false); ++stage; }
			// Observe the first actual finite full-hold transaction on both peers, preserving the same packet production frame and source debit.
			if (stage == 2 && miner->mission.phase == SpaceMissionPhase::Return && miner->cargo.TotalCount() == 20) { SaveProbeReplay(paths, "loaded", visible, false); ++stage; }
			// Only native arrival with the original cargo may authorize a later client shell release; presentation cannot supply return travel.
			if (stage == 3 && miner->mission.phase == SpaceMissionPhase::Holding) { REQUIRE(miner->cargo.TotalCount() == 20); SaveProbeReplay(paths, "region", visible, false); ++stage; }
			// Both peers first retain the same real region state. Only the client then submits the original generation-bound shell release; transit latency earns ordinary work on both worlds.
			if (stage == 4 && !released && mode == "socket-client") {
				const auto home = miner->construction.home; const auto shell = miner->manifest->pods.front();
				REQUIRE(Command<Commands::ReleaseOrbitalCargo>::Post(miner->index, miner->generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{20})); released = true;
			}
			// The real release command must transfer custody to exactly one original descent before either peer can claim a recovered delivery.
			if (stage == 4 && miner->manifest->pending_returns == 1) { REQUIRE(CaptureSpaceDescents().size() == 1); SaveProbeReplay(paths, "pending", visible, false); ++stage; }
			// Recovery must settle the actual packets and one allowance together; a cleared job alone would miss lost cargo or repeated shell use.
			if (stage == 5 && miner->manifest->pending_returns == 0 && center->goods[cargo].TotalCount() == 20) {
				REQUIRE(miner->manifest->returns_remaining == 1); REQUIRE(miner->extracted_units == 20); REQUIRE(GetMutableAsteroidBody(OrbitalBodyID{0})->remaining_reserve == 1740);
				SaveProbeReplay(paths, "recovered", visible, false); completed = TimerGameTick::counter; observed = true; ++stage;
			}
			// Keep the server alive long enough for the independently paced client to retain its final checkpoint and send actual acknowledgements before either child exits.
			if (observed && TimerGameTick::counter >= completed + (mode == "socket-server" ? 64 : 32)) _exit_game = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	});
	/** Exercise actual complete native loops with only presentation selection and command transport varying. */
	std::function<void()> observe = [&] {
		if (connected) {
			try {
				if (mode == "socket-server") {
					REQUIRE(_network_server); auto *miner = ResolveSpaceCraft({VehicleID{16}, 2}); REQUIRE(miner);
					const auto before = ProbeReplayAuthority(); const auto money = Company::Get(miner->owner)->money; const auto random = _random;
					AutoRestoreBackup actor(_current_company, miner->owner);
					// The original save has a private company allow-list. Before the first join, configure access on this cloned loopback host through the native company command; this endpoint setup grants no money, stock, work, hull or simulation interval.
					REQUIRE(Command<Commands::CompanyAllowListControl>::Do(DoCommandFlag::Execute, CompanyAllowListCtrlAction::AllowAny, std::string{}).Succeeded());
					REQUIRE(Company::Get(miner->owner)->allow_any); CHECK(ProbeReplayAuthority() == before); CHECK(Company::Get(miner->owner)->money == money);
					CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
					fmt::print("PROBE_REPLAY access native=true resource_grant=false\n");
				}
				fmt::print("PROBE_REPLAY ready mode={}\n", mode); std::fflush(stdout);
			} catch (...) { failure = std::current_exception(); _exit_game = true; }
			return;
		}
		try {
			AutoRestoreBackup actor(_current_company, _local_company); auto *miner = ResolveSpaceCraft({VehicleID{16}, 2}); REQUIRE(miner); REQUIRE(miner->manifest);
			if (mode == "recall-queue") {
				REQUIRE(miner->mission.phase == SpaceMissionPhase::Mining); REQUIRE(miner->mission.work_ticks == 37); REQUIRE(miner->cargo.TotalCount() == 2); REQUIRE(miner->extracted_units == 2);
				REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple")); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED); SetViewportLayer(GetMainWindow(), visible ? MapLayer::Space : MapLayer::Surface);
				DispatchProbeReplay<Commands::SetSpaceMissionChecked>(true, miner->owner, miner->index, miner->generation, UINT32_MAX);
				REQUIRE(miner->mission.phase == SpaceMissionPhase::Return); REQUIRE(miner->mission.work_ticks == 0); REQUIRE(miner->cargo.TotalCount() == 2); SaveProbeReplay(paths, "recalled", visible, true);
				// Actual Recall must earn its native departure and circle arrival with both original packets intact, without replaying the discarded partial extraction interval.
				for (uint ticks = 0; miner->mission.phase != SpaceMissionPhase::Holding; ++ticks) { REQUIRE(ticks < 3000); ProbeReplayOpportunity(); }
				SaveProbeReplay(paths, "region", visible, true); const auto home = miner->construction.home; const auto shell = miner->manifest->pods.front();
				DispatchProbeReplay<Commands::ReleaseOrbitalCargo>(true, miner->owner, miner->index, miner->generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{2});
				REQUIRE(CaptureSpaceDescents().size() == 1); const auto descent = CaptureSpaceDescents().front().remaining_ticks; SaveProbeReplay(paths, "pending", visible, true);
				// Only the independently owned native descent may finish this original two-unit cancellation return and expend its real shell once.
				for (uint ticks = 0; ticks < descent; ++ticks) ProbeReplayOpportunity();
				auto *center = ResolveLaunchCenter(home); REQUIRE(center); REQUIRE(center->goods[GetCargoTypeByLabel(CargoLabel{"NORE"})].TotalCount() == 2); REQUIRE(miner->extracted_units == 2);
				REQUIRE(miner->manifest->returns_remaining == 1); REQUIRE(CaptureSpaceDescents().empty()); SaveProbeReplay(paths, "recovered", visible, true); SaveProbeReplay(paths, "own", visible, false);
				observed = true; _exit_game = true; return;
			}
			REQUIRE(miner->cargo.TotalCount() == 0); REQUIRE(miner->extracted_units == 0); REQUIRE(miner->construction.capital_paid == 100000); REQUIRE(miner->manifest->returns_remaining == 2);
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple")); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED); SetViewportLayer(GetMainWindow(), visible ? MapLayer::Space : MapLayer::Surface);
			const bool queued = mode == "queue"; auto *body = GetMutableAsteroidBody(OrbitalBodyID{0}); REQUIRE(body); const auto initial = body->remaining_reserve;
			// A delayed wire with the wrong original generation must refuse at authoritative execution without clearing the actual paid hull's queue or debiting its source.
			if (queued) { const auto before = ProbeReplayAuthority(); DispatchProbeReplay<Commands::SetSpaceMissionChecked>(true, miner->owner, miner->index, miner->generation + 1, UINT32_MAX); CHECK(ProbeReplayAuthority() == before); }
			DispatchProbeReplay<Commands::SetSpaceMissionChecked>(queued, miner->owner, miner->index, miner->generation, uint32_t{0}); SaveProbeReplay(paths, "assigned", visible, true);
			// Every pursuit, attachment and partial-work opportunity is earned through the full game loop under the selected actual viewport.
			for (uint ticks = 0; miner->mission.phase != SpaceMissionPhase::Mining || miner->mission.work_ticks != 37; ++ticks) { REQUIRE(ticks < 3000); ProbeReplayOpportunity(); }
			SaveProbeReplay(paths, "partial", visible, true);
			// The native transaction alone fills the hold and debits the finite original source, with no ready-work shortcut.
			for (uint ticks = 0; miner->mission.phase != SpaceMissionPhase::Return; ++ticks) { REQUIRE(ticks < 742); ProbeReplayOpportunity(); }
			REQUIRE(miner->cargo.TotalCount() == 20); REQUIRE(body->remaining_reserve == initial - 20); SaveProbeReplay(paths, "loaded", visible, true);
			// Real departure and return movement must enter the inclusive original home's drop region before a shell can be released.
			for (uint ticks = 0; miner->mission.phase != SpaceMissionPhase::Holding; ++ticks) { REQUIRE(ticks < 3000); ProbeReplayOpportunity(); }
			SaveProbeReplay(paths, "region", visible, true); const auto home = miner->construction.home; const auto shell = miner->manifest->pods.front();
			DispatchProbeReplay<Commands::ReleaseOrbitalCargo>(queued, miner->owner, miner->index, miner->generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{20});
			REQUIRE(CaptureSpaceDescents().size() == 1); const auto descent = CaptureSpaceDescents().front().remaining_ticks; SaveProbeReplay(paths, "pending", visible, true);
			// The original independently saved descent owns the physical packets until all actual synchronized recovery work has elapsed.
			for (uint ticks = 0; ticks < descent; ++ticks) ProbeReplayOpportunity();
			REQUIRE(CaptureSpaceDescents().empty()); REQUIRE(miner->manifest->returns_remaining == 1); REQUIRE(miner->manifest->pending_returns == 0); REQUIRE(miner->manifest->services_completed == 1);
			REQUIRE(ResolveSpacePod(shell) == nullptr); auto *center = ResolveLaunchCenter(home); REQUIRE(center); REQUIRE(center->goods[GetCargoTypeByLabel(CargoLabel{"NORE"})].TotalCount() == 20);
			SaveProbeReplay(paths, "recovered", visible, true); SaveProbeReplay(paths, "own", visible, false); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(probe_replay_observer, &observe); IConsole::CmdRegister("probe_replay_test", ObserveProbeReplay);
	const std::string config = runtime.string(); std::string bind_option;
	std::vector<std::string_view> args{"openttd_test.exe", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config};
	if (mode == "socket-server") {
		const char *address = std::getenv("OTTD_PROBE_REPLAY_ADDRESS"); REQUIRE(address); const std::string_view connection{address};
		bind_option = fmt::format("-D{}", connection.substr(0, connection.find('#'))); args.insert(args.end(), {bind_option, "-g", source});
	}
	else if (mode == "socket-client") {
		const char *address = std::getenv("OTTD_PROBE_REPLAY_ADDRESS"); REQUIRE(address); args.insert(args.end(), {"-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-n", address});
	} else { args.insert(args.end(), {"-v", "null:ticks=2", "-b", "null", "-g", source}); }
	args.insert(args.end(), {"-d", "net=4,desync=2"}); CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
	fmt::print("PROBE_REPLAY complete mode={} visible={} connected={} added_stock=false\n", mode, visible, connected);
}
