/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_shell_native.cpp Actual paid carried-hull admission, bounded work and packet conservation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md; consumes its executed paid launch handoff.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../multilayer_space_content.h"
#include "../engine_base.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../settings_type.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *shell_observer = nullptr; ///< Only the current native application's observer.

/** @param args Registered console command tokens. @return Whether the invocation's observer ran. */
static bool ObserveProbeShell(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(shell_observer != nullptr); (*shell_observer)(); return true;
}

/** Consume an executed paid deployment, preserve the original hull and verify its model15 economic work. */
TEST_CASE("Paid small probe shell mines through one native owned identity", "[.probe-shell-native]")
{
	const char *input = std::getenv("OTTD_PROBE_SHELL_SOURCE"); REQUIRE(input != nullptr);
	const bool resume = std::getenv("OTTD_PROBE_SHELL_RESUME") != nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "probe-shell-native");
	const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Run inside the real loaded application and retain assertion failures until native teardown completes. */
	std::function<void()> observe = [&] {
		try {
			REQUIRE(ValidateSpaceVehicles());
			SpaceVehicle *miner = nullptr;
			// Only a deployed materially commissioned model15 can supply this boundary; no fixture-created hull or cargo is admitted.
			for (auto *vehicle : SpaceVehicle::Iterate()) {
				if (GetSpaceVehicleModel(*vehicle) == SPACE_PROBE_MODEL) { REQUIRE(miner == nullptr); miner = vehicle; }
			}
			REQUIRE(miner != nullptr); REQUIRE(miner->construction.deployed);
			REQUIRE(miner->construction.content_version == MULTILAYER_SPACE_CONTENT_VERSION);
			REQUIRE(miner->construction.placement == SpacePlacement::Orbital);
			REQUIRE(miner->manifest != nullptr);
			const auto craft = GetSpaceCraftReference(*miner); const auto engine = miner->engine_type;
			const auto home = miner->construction.home; REQUIRE(ResolveLaunchCenter(home) != nullptr);
			CHECK(engine == GetSpaceProbeEngine()); CHECK(engine == GetSpaceEngine(SPACE_PROBE_MODEL));
			CHECK(miner->cargo_cap == 20); CHECK(miner->manifest->returns_remaining == 2); CHECK(miner->manifest->pods.size() == 2);
			const auto count = GetSpaceVehicleCount();
			AutoRestoreBackup actor(_current_company, miner->owner);
			/** @param name Unique checkpoint basename in the invocation. Save real owners while paused, without inventing a transport record. */
			auto save = [&](std::string_view name) {
				AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("{}.sav", name)), false) == SaveLoadResult::Ok);
			};
			/** Advance exactly one native field/vehicle opportunity, bounded independently of GUI pacing. */
			auto tick = [&] {
				const auto before = GetAsteroidField().elapsed_ticks;
				const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
				// Application pacing may skip a call; only an actual synchronized opportunity may count as paid mission work.
				while (GetAsteroidField().elapsed_ticks == before) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				REQUIRE(GetAsteroidField().elapsed_ticks == before + 1);
			};
			AutoRestoreBackup running(_pause_mode, PauseModes{});
			if (!resume) {
				CHECK(miner->mission.phase == SpaceMissionPhase::Idle); CHECK(miner->mission.target == INVALID_ORBITAL_BODY);
				CHECK(miner->cargo.TotalCount() == 0); CHECK(miner->extracted_units == 0);
				const auto profit = miner->profit_this_year; const auto remainder = miner->mission.cost_remainder;
				// A newly deployed unassigned shell remains idle, retaining all supplied pods and paying exactly one day of idle upkeep.
				for (uint tick_index = 0; tick_index < 74; ++tick_index) tick();
				CHECK(miner->profit_this_year == profit - Money{100} * 256); CHECK(miner->mission.cost_remainder == remainder);
				CHECK(miner->mission.phase == SpaceMissionPhase::Idle); CHECK(miner->mission.target == INVALID_ORBITAL_BODY);
				const auto digest = ComputeSpaceVehicleDigest(); const auto money = Company::Get(miner->owner)->money;
				CHECK(CmdSetSpaceMission({}, miner->index, static_cast<uint32_t>(GetAsteroidField().bodies.size())).Failed());
				// Every starter class may be selected without prior survey; estimation must preserve cargo labels, work, packets and money.
				for (uint32_t body = 0; body < 3; ++body) CHECK(CmdSetSpaceMission({}, miner->index, body).Succeeded());
				CHECK(CmdSetSpaceMission({}, miner->index, UINT32_MAX).Succeeded());
				CHECK(ComputeSpaceVehicleDigest() == digest); CHECK(Company::Get(miner->owner)->money == money);
				REQUIRE(Command<Commands::SetSpaceMission>::Post(miner->index, uint32_t{0}));
				const auto *body = GetMutableAsteroidBody(OrbitalBodyID{0}); REQUIRE(body != nullptr);
				const SpacePoint approach{body->motion.position.x + miner->mission.attachment_offset.x, body->motion.position.y + miner->mission.attachment_offset.y};
				CHECK(miner->mission.approach_ticks_left == GetSpaceMiningApproachTicks(miner->position, approach));
				CHECK(miner->construction.home == home); CHECK(miner->engine_type == engine); CHECK(GetSpaceCraftReference(*miner) == craft);
			} else {
				REQUIRE(miner->mission.phase == SpaceMissionPhase::Mining); REQUIRE(miner->mission.work_ticks == 37); REQUIRE(miner->cargo.TotalCount() == 2);
			}
			const auto target = miner->mission.target; REQUIRE(target == OrbitalBodyID{0});
			const auto initial_total = GetMutableAsteroidBody(target)->remaining_reserve + miner->cargo.TotalCount();
			bool partial_saved = resume; uint32_t working = 0;
			// Observe a bounded physical collection; compare every tick's active/idle accrual and finite reserve against the same native packet owner.
			for (uint32_t elapsed = 0; elapsed < 4000; ++elapsed) {
				if (miner->mission.phase == SpaceMissionPhase::Holding) break;
				const bool active = IsSpaceMiningWorkTick(*miner); working += active;
				const auto before_profit = miner->profit_this_year;
				const uint16_t accrual = miner->mission.cost_remainder + (active ? 500 : 100);
				tick();
				CHECK(miner->profit_this_year == before_profit - Money{accrual / 74} * 256);
				CHECK(miner->mission.cost_remainder == accrual % 74);
				CHECK(GetMutableAsteroidBody(target)->remaining_reserve + miner->cargo.TotalCount() == initial_total);
				CHECK(miner->cargo.TotalCount() <= 20); CHECK(GetSpaceCraftReference(*miner) == craft);
				// Retain a real partially completed extraction boundary so reload must preserve both paid work and packet custody.
				if (!partial_saved && miner->cargo.TotalCount() == 2 && miner->mission.work_ticks == 37) {
					REQUIRE(ValidateSpaceVehicles()); save("probe-shell-partial-424"); partial_saved = true;
				}
			}
			REQUIRE(partial_saved); REQUIRE(miner->mission.phase == SpaceMissionPhase::Holding);
			CHECK(miner->cargo.TotalCount() == 20); CHECK(miner->extracted_units == 20);
			CHECK(working == (resume ? 629 : 740)); CHECK(GetMutableAsteroidBody(target)->mining_slot == VehicleID::Invalid());
			CHECK(GetSpaceVehicleCount() == count); CHECK(miner->engine_type == engine); CHECK(miner->construction.home == home);
			CHECK(miner->manifest->returns_remaining == 2); CHECK(miner->manifest->pods.size() == 2);
			const auto digest = ComputeSpaceVehicleDigest();
			CHECK(CmdSetSpaceMission({}, miner->index, 1).Failed()); CHECK(ComputeSpaceVehicleDigest() == digest);
			REQUIRE(ValidateSpaceVehicles()); save("probe-shell-complete-424");
			fmt::print("PROBE_SHELL native=true model=15 same_hull=true tonnes=20 work_ticks={} resumed={}\n", working, resume);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(shell_observer, &observe); IConsole::CmdRegister("probe_shell_test", ObserveProbeShell);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
