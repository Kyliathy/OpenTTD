/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_pursuit_native.cpp Actual Earth-paid deployed probe follows a moving body without a station.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group9
 * Accepted deployment supplies every hull, shell, input, position and service allowance; this observer creates none.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../multilayer_space_content.h"
#include "../space_structure.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../company_func.h"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <thread>
#include "../safeguards.h"

static std::function<void()> *pursuit_observer = nullptr; ///< Exact process observer, without independent gameplay ownership.

/** @param args Parameter-free original console command. @return Whether this exact native observer ran. */
static bool ObservePursuit(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(pursuit_observer); (*pursuit_observer)(); return true;
}

/** Offer one actual bounded normal simulation opportunity; presentation and wall-clock waits earn no pursuit work. */
static void PursuitOpportunity()
{
	const auto elapsed = GetAsteroidField().elapsed_ticks; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	AutoRestoreBackup pause(_pause_mode, PauseModes{});
	// Actual field/native-vehicle progression defines the opportunity; a skipped paced frame cannot consume the saved pursuit budget.
	while (GetAsteroidField().elapsed_ticks == elapsed) {
		REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop();
		if (GetAsteroidField().elapsed_ticks == elapsed) std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	REQUIRE(GetAsteroidField().elapsed_ticks == elapsed + 1);
}

/** A genuine paid deployment, original home and current target alone authorize this first station-free mission. */
TEST_CASE("Original paid Earth probe pursues a current moving approach without a station", "[.probe-pursuit-native]")
{
	const char *source = std::getenv("OTTD_PROBE_PURSUIT_SOURCE"), *selected = std::getenv("OTTD_PROBE_PURSUIT_PHASE");
	REQUIRE(source); REQUIRE(selected); const std::string_view phase{selected}; const bool resave = std::getenv("OTTD_PROBE_PURSUIT_RESAVE") != nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "probe-pursuit"); const auto runtime = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Observe only the original ordinary loaded paid world and preserve assertion failures through orderly engine shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); REQUIRE(SpaceStructure::GetNumItems() == 0);
			SpaceVehicle *miner = nullptr;
			// Only the original Earth-deployed model15 with its actual center/paid receipt may drive this acceptance; no unit legacy hull can substitute it.
			for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 15) { REQUIRE(miner == nullptr); miner = craft; }
			REQUIRE(miner); REQUIRE(miner->construction.deployed); REQUIRE(IsSupportedSpaceVehicleContentVersion(miner->construction.content_version, 15));
			const auto reference = GetSpaceCraftReference(*miner); const auto home = miner->construction.home; REQUIRE(ResolveLaunchCenter(home));
			REQUIRE(miner->manifest); REQUIRE(miner->manifest->returns_remaining == 2); REQUIRE(miner->manifest->pods.size() == 2); REQUIRE(miner->cargo.TotalCount() == 0);
			const auto fleet = GetSpaceVehicleCount(); const auto before = ComputeSpaceVehicleDigest();
			if (resave) {
				REQUIRE(phase == "assigned");
			} else if (phase == "assigned") {
				REQUIRE(miner->mission.phase == SpaceMissionPhase::Idle); const auto money = Company::Get(_current_company)->money;
				CHECK(CmdSetSpaceMission({}, miner->index, static_cast<uint32_t>(GetAsteroidField().bodies.size())).Failed());
				CHECK(ComputeSpaceVehicleDigest() == before); CHECK(Company::Get(_current_company)->money == money);
				REQUIRE(Command<Commands::SetSpaceMission>::Post(miner->index, uint32_t{0}));
				const auto *body = GetMutableAsteroidBody(OrbitalBodyID{0}); REQUIRE(body); const SpacePoint point{body->motion.position.x + miner->mission.attachment_offset.x, body->motion.position.y + miner->mission.attachment_offset.y};
				CHECK(miner->mission.phase == SpaceMissionPhase::Approach); CHECK(miner->mission.approach_ticks_left == GetSpaceMiningApproachTicks(miner->position, point));
			} else {
				REQUIRE(phase == "intercepted"); REQUIRE(miner->mission.phase == SpaceMissionPhase::Approach);
				const auto *body = GetMutableAsteroidBody(miner->mission.target); REQUIRE(body); const auto initial = body->motion.position;
				const auto reserve = body->remaining_reserve; const auto allowance = miner->mission.approach_ticks_left; uint elapsed = 0;
				// Every opportunity follows the same actual monotonic body ID and frozen initial budget until native exclusive attachment succeeds; no old tile or edited hull point supplies arrival.
				while (miner->mission.phase == SpaceMissionPhase::Approach) { REQUIRE(elapsed++ <= allowance + 10); PursuitOpportunity(); CHECK(miner->mission.target == body->id); }
				REQUIRE(miner->mission.phase == SpaceMissionPhase::Mining); CHECK(body->motion.position != initial); CHECK(body->remaining_reserve == reserve);
				CHECK(body->mining_slot == miner->index); CHECK(miner->position == SpacePoint{body->motion.position.x + miner->mission.attachment_offset.x, body->motion.position.y + miner->mission.attachment_offset.y});
				CHECK(miner->mission.approach_ticks_left == 0); CHECK(miner->mission.work_ticks == 0); CHECK(miner->extracted_units == 0);
			}
			CHECK(GetSpaceCraftReference(*miner) == reference); CHECK(miner->construction.home == home); CHECK(GetSpaceVehicleCount() == fleet); REQUIRE(ValidateSpaceVehicles());
			if (resave) CHECK(ComputeSpaceVehicleDigest() == before);
			{ AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("pursuit-{}-438.sav", phase)), false) == SaveLoadResult::Ok); }
			fmt::print("PROBE_PURSUIT native=true phase={} resave={} added_stock=false no_station=true\n", phase, resave); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(pursuit_observer, &observe); IConsole::CmdRegister("probe_pursuit_test", ObservePursuit); const std::string config = runtime.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
