/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_utility_load.cpp Actual392 paid native path files, genuine391 migration and independent malformed preview; no operating generator is inferred.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group3B
 */

#include "../stdafx.h"
#include <cstdlib>
#include <filesystem>
#include "../3rdparty/catch2/catch.hpp"
#include "layer_save_fixture.h"
#include "../geothermal_collector.h"
#include "../geothermal_utility.h"
#include "../geothermal_utility_cmd.h"
#include "../command_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

static std::function<void()> *_utility_path_observer = nullptr; ///< Test-only accepted-world callback; ordinary loading retains every actual company/source/floor/stock owner.

/** @param arguments Startup callback with no extra arguments. @return Whether the actual accepted-world observer ran. */
static bool ObserveUtilityPath(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_utility_path_observer != nullptr); (*_utility_path_observer)(); return true;
}

/** @param tile Actual fully validated native cell. @param remove Select removal rather than build. @param domain Exact installed network. @return Ordinary synchronized kernel result, including its full capital/removal settlement. */
static CommandCost SavedUtilityCellCommand(TileIndex tile, bool remove = false, UtilityEnergyDomain domain = UtilityEnergyDomain::Heat)
{
	if (remove) return Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile, domain));
	return Command<Commands::BuildGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile, domain));
}

/** Genuine old absence yields no path; actual paid source-port/cell geometry, split and repaired identity survive ordinary native saves without renewing energy. */
TEST_CASE("Paid native Deep utility paths survive actual files and malformed previews", "[.geothermal-utility-file]")
{
	const auto root = std::filesystem::current_path().parent_path(), directory = root / "src/tests/fixtures/geothermal";
	const char *input_text = std::getenv("OTTD_UTILITY_PATH_INPUT"), *mode_text = std::getenv("OTTD_UTILITY_PATH_CASE");
	REQUIRE(input_text != nullptr); REQUIRE(mode_text != nullptr); const std::filesystem::path input{input_text}; const std::string_view mode{mode_text};
	REQUIRE(std::filesystem::exists(input)); bool observed = false; std::exception_ptr failure;
	/** Inspect only accepted actual owners, retaining the predecessor's explicitly supplied test floor/materials and original metered heat history. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			const auto collectors = CaptureGeothermalCollectors(); REQUIRE(collectors.size() == 1); REQUIRE(IsGeothermalCollectorOperational(collectors[0].industry));
			const auto *claim = GetGeothermalSiteClaim(collectors[0].site); REQUIRE(claim != nullptr);
			const auto source = claim->site.endpoint, middle = TileAddWrap(source, -1, 0), end = TileAddWrap(source, -1, 2);
			const auto natural = ComputeGeologyStateDigest(GetGeologyWorldState()), cold = ComputeGeothermalSiteDigest(), collector = ComputeGeothermalCollectorDigest();
			const auto before = CaptureUtilityPowerState(); const auto cash = Company::Get(_current_company)->money;
			if (mode == "legacy-read") {
				CHECK_FALSE(HasUtilityLocalLinks()); CHECK_FALSE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, source, end).has_value());
			} else if (mode == "fixture-build") {
				REQUIRE_FALSE(HasUtilityLocalLinks());
				// Install each real paid native cell through its command, rather
				// than restoring a numerical path fixture or writing raw metadata.
				for (const auto tile : {source, middle, TileAddWrap(source, -1, 1), end}) REQUIRE(SavedUtilityCellCommand(tile).Succeeded());
				REQUIRE(SavedUtilityCellCommand(source, false, UtilityEnergyDomain::Electricity).Succeeded());
				CHECK(Company::Get(_current_company)->money == cash - Money{5000}); CHECK(GetUtilityLocalLinkRevision() == 5);
			} else {
				REQUIRE(HasUtilityLocalLinks());
				if (mode == "fixture-split") {
					REQUIRE(SavedUtilityCellCommand(middle, true).Succeeded()); CHECK(Company::Get(_current_company)->money == cash - Money{250});
				} else if (mode == "fixture-repair") {
					REQUIRE_FALSE(GetGeothermalUtilityLink(middle, UtilityEnergyDomain::Heat).has_value());
					REQUIRE(SavedUtilityCellCommand(middle).Succeeded()); CHECK(Company::Get(_current_company)->money == cash - Money{1000});
				} else if (mode == "preview") {
					const auto *corrupt = std::getenv("OTTD_UTILITY_PATH_CORRUPT"); REQUIRE(corrupt != nullptr); const auto digest = ComputeUtilityPowerDigest();
					CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(ComputeUtilityPowerDigest() == digest);
					REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(_load_check_data.utility_power_state.local_link_revision == GetUtilityLocalLinkRevision()); CHECK(ComputeUtilityPowerDigest() == digest);
				}
			}
			if (mode != "legacy-read") {
				const auto path = FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, source, end);
				if (mode == "fixture-split" || mode == "split-read") CHECK_FALSE(path.has_value()); // The missing paid bridge supplies neither proximity connectivity nor free residual capacity.
				else { REQUIRE(path.has_value()); CHECK(path->cells.size() == 4); CHECK(path->capacity_w == 1000000); }
				CHECK_FALSE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Electricity, source, end).has_value());
			}
			const auto after = CaptureUtilityPowerState();
			CHECK(after.interval == before.interval); CHECK(after.next_producer == before.next_producer); CHECK(after.next_consumer == before.next_consumer);
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural); CHECK(ComputeGeothermalSiteDigest() == cold); CHECK(ComputeGeothermalCollectorDigest() == collector);
			REQUIRE(ValidateGeothermalUtilityLinks()); REQUIRE(ValidateUtilityPowerState(after, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
			if (mode != "preview") {
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / fmt::format("fixture392-utility-path-{}.sav", mode)), false) == SaveLoadResult::Ok);
			}
			fmt::print("UTILITY_PATH_FILE case={} shared={:016X} revision={} paid_native_commands=true physical_heat_allocation=false\n", mode, ComputeUtilityPowerDigest(), GetUtilityLocalLinkRevision());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_utility_path_observer, &inspect); IConsole::CmdRegister("utility_path_check", ObserveUtilityPath);
	const std::string config = (root / "build/native-art.cfg").string(), input_path = input.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", input_path};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
