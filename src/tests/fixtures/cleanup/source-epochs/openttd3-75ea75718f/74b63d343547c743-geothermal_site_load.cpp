/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_site_load.cpp Actual388 cold-site files on explicitly fixture-open floor, preserving original natural and electrical authorities.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.3; operating paid plant bootstrap remains10.6.
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <filesystem>
#include "../3rdparty/catch2/catch.hpp"
#include "layer_save_fixture.h"
#include "../geothermal.h"
#include "../company_func.h"
#include "../tile_map.h"
#include "../excavation_state.h"
#include "../utility_power.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

static std::function<void()> *_geothermal_site_observer = nullptr; ///< Isolated complete accepted-world observer; native loading owns every actual pool and source.

/** @param arguments Exact zero-argument startup fixture command. @return Whether the actual accepted-world observer executed. */
static bool ObserveGeothermalSite(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_geothermal_site_observer != nullptr); (*_geothermal_site_observer)(); return true;
}

/** Native cold planning transport/recovery, with explicitly declared open test floor and no operating collector/generator or delivered construction claim. */
TEST_CASE("Native cold geothermal site identity and cancellation survive actual files", "[.geothermal-site-file]")
{
	const auto root = FindTestRepositoryRoot(), directory = GetTestCaseArtifactPaths("geothermal", "native-reload").saves;
	const char *input_text = std::getenv("OTTD_GEOTHERMAL_SITE_INPUT"), *mode_text = std::getenv("OTTD_GEOTHERMAL_SITE_CASE");
	REQUIRE(input_text != nullptr); REQUIRE(mode_text != nullptr);
	const auto input = ResolveTestInputPath(input_text); const std::string_view mode{mode_text};
	REQUIRE(std::filesystem::exists(input)); REQUIRE(std::filesystem::is_directory(directory));
	bool observed = false; std::exception_ptr failure;
	/** Inspect the actual accepted world; cold fixture floor is declared input for persistence, while later real paid construction retains its independent acceptance. */
	std::function<void()> inspect = [&] {
		try {
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			AutoRestoreBackup actor(_current_company, _local_company);
			const auto electrical = ComputeUtilityPowerDigest();
			if (mode == "fixture-create") {
				REQUIRE(CaptureGeothermalSiteClaims().empty()); REQUIRE_FALSE(GetGeologyWorldState().vents.empty());
				const auto natural = GetGeologyWorldState().vents.front();
				GeothermalSiteRequest request; request.owner = _current_company; request.vent = natural.id;
				request.tile = request.endpoint = TileXYL(natural.x, natural.y, MapLayer::DeepUnderground); request.access = DiagDirection::SE;
				const auto snapshot = CaptureGeologyState(); GeologyWorldState candidate;
				REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
				// Only this labelled cold persistence fixture supplies open floor.
				// It preserves every natural ID/rate, reserve and active work claim;
				// no paid excavation/actual operating plant is accepted from this setup.
				for (int y = -1; y <= 3; ++y) {
					for (int x = -1; x <= 3; ++x) {
						const auto tile = TileAddWrap(request.tile, x, y);
						REQUIRE(IsValidTile(tile)); REQUIRE(IsTileType(tile, TileType::Clear)); REQUIRE_FALSE(candidate.job_claims.contains(tile));
						REQUIRE(candidate.terrain.Set(tile, ExcavationState::Excavated));
					}
				}
				StageLoadedGeologyState(std::move(candidate)); AcceptLoadedGeologyState();
				const auto introduction = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
				if (TimerGameCalendar::date < introduction) TimerGameCalendar::SetDate(introduction, {});
				const auto result = ClaimGeothermalSite(request); REQUIRE(result.error == GeothermalError::None);
				CHECK(GetGeologyWorldState().vents.front().id == natural.id); CHECK(GetGeologyWorldState().vents.front().thermal_capacity_w == natural.thermal_capacity_w);
			} else if (mode == "legacy-read") {
				CHECK(CaptureGeothermalSiteClaims().empty());
			} else {
				const auto claims = CaptureGeothermalSiteClaims(); REQUIRE(claims.size() == 1);
				CHECK(ValidateGeothermalSiteClaims(claims) == GeothermalError::None);
				if (mode == "fixture-cancel") {
					REQUIRE(claims[0].active); REQUIRE(CancelGeothermalSite(claims[0].id) == GeothermalError::None);
				} else if (mode == "preview") {
					const auto *corrupt = std::getenv("OTTD_GEOTHERMAL_SITE_CORRUPT"); REQUIRE(corrupt != nullptr);
					const auto before = ComputeGeothermalSiteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
					CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(ComputeGeothermalSiteDigest() == before); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
					REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(_load_check_data.geothermal_site_rows.size() == 1); CHECK_FALSE(_load_check_data.geology_vent_fields.empty());
					CHECK(ComputeGeothermalSiteDigest() == before); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
				} else {
					CHECK(claims[0].active == (mode != "cancelled-read"));
				}
			}
			CHECK(ComputeUtilityPowerDigest() == electrical);
			if (mode != "preview") {
				const auto before = ComputeGeothermalSiteDigest();
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / fmt::format("fixture388-cold-site-{}.sav", mode)), false) == SaveLoadResult::Ok);
				CHECK(ComputeGeothermalSiteDigest() == before); CHECK(ComputeUtilityPowerDigest() == electrical);
			}
			fmt::print("GEOTHERMAL_SITE_FILE case={} claims={:016X} fixture_open_floor=true completed_construction=false electrical_unchanged=true\n", mode, ComputeGeothermalSiteDigest());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_geothermal_site_observer, &inspect);
	IConsole::CmdRegister("geothermal_site_check", ObserveGeothermalSite);
	const std::string config = (GetTestRuntimeConfig()).string(), input_path = input.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", input_path};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
