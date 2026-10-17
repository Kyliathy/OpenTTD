/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_return_native.cpp Original paid retained cargo follows a shared clear departure and enters only its authorized circle.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group11
 * Exact paid full-hold input alone supplies hull, packets, body history, shells and allowance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_departure.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_structure.h"
#include "../asteroid.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../command_func.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../station_base.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <thread>
#include "../safeguards.h"

static std::function<void()> *return_observer = nullptr; ///< Exact process observer without independent movement or material ownership.

/** @param args Parameter-free native console arguments. @return Whether this exact loaded-world observer ran. */
static bool ObserveProbeReturn(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(return_observer); (*return_observer)(); return true; }

/** Offer one actual synchronized body/vehicle opportunity, preserving cargo while only native travel earns movement. */
static void ReturnOpportunity()
{
	const auto before = GetAsteroidField().elapsed_ticks; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10); AutoRestoreBackup pause(_pause_mode, PauseModes{});
	// Presentation pacing cannot move the hull, renew allowance, relinquish packets or count as return progress.
	while (GetAsteroidField().elapsed_ticks == before) { REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	REQUIRE(GetAsteroidField().elapsed_ticks == before + 1);
}

/** A paid full hold must leave through actual clear geometry and stop at its original circle without handing off freight or earning service. */
TEST_CASE("Original paid probe retains mined cargo through safe departure and authorized circle entry", "[.probe-return-native]")
{
	const char *source = std::getenv("OTTD_PROBE_RETURN_SOURCE"), *selected = std::getenv("OTTD_PROBE_RETURN_PHASE"); REQUIRE(source); REQUIRE(selected);
	const std::string_view phase{selected}; const bool resave = std::getenv("OTTD_PROBE_RETURN_RESAVE") != nullptr;
	const bool boundaries = std::getenv("OTTD_PROBE_RETURN_BOUNDARIES") != nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "probe-return"); const auto runtime = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Observe only the original ordinary paid input and retain assertion failures through orderly native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); auto *miner = SpaceVehicle::GetIfValid(VehicleID{16}); REQUIRE(miner); REQUIRE(miner->generation == 2); REQUIRE(GetSpaceVehicleModel(*miner) == 15);
			const auto *body = GetMutableAsteroidBody(OrbitalBodyID{0}); REQUIRE(body); const auto home = miner->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center);
			const auto anchor = *GetLaunchCenterPlaneReference(center->launch_center->tile); const auto before = ComputeSpaceVehicleDigest(); const auto fleet = GetSpaceVehicleCount(); REQUIRE(miner->manifest);
			REQUIRE(miner->cargo.TotalCount() == 20); REQUIRE(miner->extracted_units == 20); REQUIRE(body->remaining_reserve == 1740); REQUIRE(body->mining_slot == VehicleID::Invalid()); REQUIRE(CaptureSpaceDescents().empty());
			if (!resave) {
				REQUIRE(miner->mission.phase == SpaceMissionPhase::Return);
				if (phase == "exit") {
					const auto departure = GetSpaceMiningDepartureDestination(*miner, anchor); REQUIRE(departure); REQUIRE(*departure != miner->position);
					CHECK(ComputeSpaceVehicleDigest() == before); const auto position = miner->position; ReturnOpportunity();
					CHECK(miner->position != position); CHECK(SpaceDistanceSquared(miner->position, position) < uint64_t{SPACE_Q16} * SPACE_Q16);
				} else if (phase == "outbound") {
					// Actual travel must clear the original moving attachment envelope; no position assignment or reset can supply departure.
					for (uint elapsed = 0; elapsed < 74; ++elapsed) ReturnOpportunity();
					CHECK(GetSpaceMiningDepartureDestination(*miner, anchor) == miner->position);
				} else {
					REQUIRE(phase == "region");
					// The deterministic native route stops on its first actual step inside the original inclusive circle, retaining the complete same hold and actual idle upkeep.
					for (uint elapsed = 0; miner->mission.phase == SpaceMissionPhase::Return; ++elapsed) {
						REQUIRE(elapsed < 2000); const auto profit = miner->profit_this_year; const uint accrual = miner->mission.cost_remainder + 100;
						ReturnOpportunity(); CHECK(miner->profit_this_year == profit - Money{accrual / 74} * 256); CHECK(miner->mission.cost_remainder == accrual % 74); CHECK(miner->cargo.TotalCount() == 20);
					}
				}
			}
			CHECK(miner->cargo.TotalCount() == 20); CHECK(miner->extracted_units == 20); CHECK(body->remaining_reserve == 1740); CHECK(miner->mission.work_ticks == 0); CHECK(body->mining_slot == VehicleID::Invalid());
			CHECK(miner->manifest->returns_remaining == 2); CHECK(miner->manifest->pending_returns == 0); CHECK(miner->manifest->pods.size() == 2); CHECK(CaptureSpaceDescents().empty()); CHECK(GetSpaceVehicleCount() == fleet);
			if (phase == "region") {
				REQUIRE(miner->mission.phase == SpaceMissionPhase::Holding); CHECK(IsInsideLaunchCenterDropZone(miner->position, anchor)); CHECK(miner->position != anchor); CHECK_FALSE(IsAtLaunchServiceApproach(miner->position, *center));
				const auto pod = miner->manifest->pods.front(); const auto digest = ComputeSpaceVehicleDigest(); const auto money = Company::Get(_current_company)->money;
				CHECK(CmdReleaseOrbitalCargo({}, miner->index, miner->generation, home.station, home.generation, pod.pod, pod.generation, uint16_t{20}).Succeeded());
				CHECK(CmdReleaseOrbitalCargo({}, miner->index, miner->generation, home.station, home.generation + 1, pod.pod, pod.generation, uint16_t{20}).Failed());
				CHECK(ComputeSpaceVehicleDigest() == digest); CHECK(Company::Get(_current_company)->money == money); CHECK(CaptureSpaceDescents().empty());
			} else { CHECK(miner->mission.phase == SpaceMissionPhase::Return); CHECK_FALSE(IsInsideLaunchCenterDropZone(miner->position, anchor)); }
			if (resave) CHECK(ComputeSpaceVehicleDigest() == before);
			if (boundaries) {
				REQUIRE(phase == "region"); REQUIRE(resave); const auto digest = ComputeSpaceVehicleDigest(); const auto pod = miner->manifest->pods.front();
				// Explicit isolated foreign-owner input: the valid temporary company receives no funds, craft, stock or saved target. Restoring the field precedes identity retirement; this quote moves no ownership.
				REQUIRE(Company::CanAllocateItem()); auto *foreign = Company::Create(); SetCompanyManagerFaceStyle(foreign->face, 0);
				{ AutoRestoreBackup owner(center->owner, Owner{foreign->index}); CHECK_FALSE(CanReleaseOrbitalCargo(GetSpaceCraftReference(*miner), home, pod, uint16_t{20})); }
				delete foreign;
				// Controlled capacity input is a separate component fixture, never a paid travel checkpoint. Fill only this candidate's remaining headroom; no file containing this supplied stock is emitted.
				const auto headroom = GetLaunchStockHeadroom(*center, miner->cargo_type); REQUIRE(headroom >= 20); REQUIRE(CargoPacket::CanAllocateItem());
				center->goods[miner->cargo_type].GetOrCreateData().cargo.Append(CargoPacket::Create(center->index, static_cast<uint16_t>(headroom), Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
				CHECK_FALSE(CanReleaseOrbitalCargo(GetSpaceCraftReference(*miner), home, pod, uint16_t{20})); CHECK(GetLaunchStockHeadroom(*center, miner->cargo_type) == 0);
				CHECK(ComputeSpaceVehicleDigest() == digest); CHECK(miner->cargo.TotalCount() == 20); CHECK(miner->manifest->pending_returns == 0); CHECK(CaptureSpaceDescents().empty());
				REQUIRE(ValidateSpaceVehicles()); fmt::print("PROBE_RETURN boundaries=true foreign=true full=true supplied_fixture_units={} no_save=true\n", headroom);
			} else {
				REQUIRE(ValidateSpaceVehicles()); { AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
					REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("return-{}-438.sav", phase)), false) == SaveLoadResult::Ok); }
				fmt::print("PROBE_RETURN native=true phase={} resave={} added_stock=false\n", phase, resave);
			}
			observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(return_observer, &observe); IConsole::CmdRegister("probe_return_test", ObserveProbeReturn); const std::string config = runtime.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
