/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_release_admission.cpp Current owned release quotes during actual orbital movement; no artificial hull positions or cargo.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group12.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_orders.h"
#include "../space_transition.h"
#include "../asteroid.h"
#include "../station_base.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include <filesystem>
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *admission_observer = nullptr; ///< Process-local observer; not a movement, packet or reservation owner.

/** @param args Native console invocation without parameters. @return Whether the installed loaded-world observer ran. */
static bool ObserveReleaseAdmission(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(admission_observer != nullptr); (*admission_observer)(); return true;
}

/** Quote the complete release owner at real in-circle/outside positions while ordinary ordered navigation moves the original paid miner. */
TEST_CASE("Owned release admission follows actual current orbital position and reserved recovery", "[.orbital-release-admission]")
{
	const char *input = std::getenv("OTTD_RELEASE_ADMISSION_SOURCE"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "release-admission"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Run only against the ordinary loaded paid predecessor and preserve assertion failure through native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			SpaceVehicle *miner = nullptr;
			// The retained paid414 descent world supplies the original orbital miner, ten real units still aboard and one original paid carried shell.
			for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 15) { REQUIRE(miner == nullptr); miner = craft; }
			REQUIRE(miner != nullptr); REQUIRE(miner->manifest != nullptr); REQUIRE(miner->cargo.TotalCount() == 10); REQUIRE(miner->manifest->pods.size() == 1);
			const auto reference = GetSpaceCraftReference(*miner);
			const auto center_ref = miner->construction.home;
			auto *center = ResolveLaunchCenter(center_ref); REQUIRE(center != nullptr); const auto point = *GetLaunchCenterPlaneReference(center->launch_center->tile);
			const auto shell = miner->manifest->pods.front(); REQUIRE(miner->position == point); REQUIRE(miner->manifest->pending_returns == 1);
			const auto incoming = GetIncomingSpaceDescent(center_ref, miner->cargo_type); CHECK(incoming == 10);
			CHECK(GetLaunchStockHeadroom(*center, miner->cargo_type) == GetLaunchStockCapacity(*center, miner->cargo_type) - center->goods[miner->cargo_type].TotalCount() - incoming);
			/** @param expected Whether this actual physical position should currently admit the complete paid shell/packet owner. */
			auto quote = [&](bool expected) {
				const auto vehicles = ComputeSpaceVehicleDigest(), descents = ComputeSpaceDescentDigest(), manifests = ComputeSpaceManifestDigest();
				const auto money = Company::Get(_current_company)->money; const auto profit = miner->profit_this_year; const auto random = _random;
				CHECK(CanReleaseOrbitalCargo(reference, center_ref, shell, 10) == expected);
				CHECK(CmdReleaseOrbitalCargo({}, miner->index, miner->generation, center_ref.station, center_ref.generation, shell.pod, shell.generation, 10).Succeeded() == expected);
				CHECK_FALSE(CanReleaseOrbitalCargo(reference, {center_ref.station, center_ref.generation + 100}, shell, 10));
				CHECK_FALSE(CanReleaseOrbitalCargo(reference, center_ref, {shell.pod, shell.generation + 100}, 10));
				CHECK_FALSE(CanReleaseOrbitalCargo(reference, center_ref, shell, 0)); CHECK_FALSE(CanReleaseOrbitalCargo(reference, center_ref, shell, 41));
				CHECK(ComputeSpaceVehicleDigest() == vehicles); CHECK(ComputeSpaceDescentDigest() == descents); CHECK(ComputeSpaceManifestDigest() == manifests);
				CHECK(Company::Get(_current_company)->money == money); CHECK(miner->profit_this_year == profit); CHECK(miner->cargo.TotalCount() == 10);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			};
			quote(true); bool overlap = false;
			// The genuine predecessor already contains a separately paid foreign center overlapping the original drop circle; geometry alone cannot authorize another company's recovery.
			for (const auto other_ref : GetLaunchCenterReferences()) {
				const auto *other = ResolveLaunchCenter(other_ref);
				if (other->owner == miner->owner || !IsInsideLaunchCenterDropZone(miner->position, *GetLaunchCenterPlaneReference(other->launch_center->tile))) continue;
				const auto before = ComputeSpaceVehicleDigest(); const auto cash = Company::Get(_current_company)->money;
				CHECK_FALSE(CanReleaseOrbitalCargo(reference, other_ref, shell, 10)); CHECK(ComputeSpaceVehicleDigest() == before); CHECK(Company::Get(_current_company)->money == cash); overlap = true;
			}
			REQUIRE(overlap);
			/** @param ticks Actual synchronized native opportunities; rendering/yields cannot earn movement or recovery work. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Count authoritative native ticks rather than loop iterations, retaining a bounded stalled-controller deadline.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			const AsteroidBody *target = nullptr;
			// Choose a real unused natural destination safely beyond the original drop zone; the test grants neither a new body nor a private navigation coordinate.
			for (const auto &body : GetAsteroidField().bodies) {
				if (body.mining_slot != VehicleID::Invalid() || body.survey_slot != INVALID_SPACE_SURVEY) continue;
				if (SpaceDistanceSquared(body.motion.position, point) > uint64_t{24 * SPACE_Q16} * (24 * SPACE_Q16)) { target = &body; break; }
			}
			REQUIRE(target != nullptr);
			REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
			REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, target->id.base(), uint64_t{0}));
			bool off_center = false, outside = false;
			// Every quote follows the actual controller's current Q16 point; normal Visit motion crosses the circle without releasing or fabricating any cargo.
			for (uint elapsed = 0; !outside; ++elapsed) {
				REQUIRE(elapsed < 4000); advance(1); const bool inside = IsInsideLaunchCenterDropZone(miner->position, point);
				// One actual off-center sample verifies complete admission; further travel earns no extra test coverage until the true geometric boundary is crossed.
				if (!off_center && inside && miner->position != point && SpaceDistanceSquared(miner->position, point) >= uint64_t{2 * SPACE_Q16} * (2 * SPACE_Q16)) { quote(true); off_center = true; }
				if (!inside) { quote(false); outside = true; }
			}
			CHECK(off_center); CHECK(outside); CHECK(miner->cargo.TotalCount() == 10); CHECK(ResolveSpacePod(shell)->state.craft == reference);
			REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / "release-admission-423.sav"), false) == SaveLoadResult::Ok);
			fmt::print("RELEASE_ADMISSION native=true actual_navigation=true off_center=true outside_refused=true foreign_overlap=true pure_quotes=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(admission_observer, &observe); IConsole::CmdRegister("release_admission_test", ObserveReleaseAdmission);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
