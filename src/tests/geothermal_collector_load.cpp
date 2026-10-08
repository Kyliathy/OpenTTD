/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_collector_load.cpp Actual389 native construction files on explicitly supplied test floor/material deliveries; transported paid bootstrap remains master10.6.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group1B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoints10.3/10.6
 */

#include "../stdafx.h"
#include <cstdlib>
#include <filesystem>
#include "../3rdparty/catch2/catch.hpp"
#include "layer_save_fixture.h"
#include "../geothermal_collector.h"
#include "../industry.h"
#include "../industry_cmd.h"
#include "../command_func.h"
#include "../company_func.h"
#include "../utility_power.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

static std::function<void()> *_geothermal_collector_observer = nullptr; ///< Complete accepted-world test observer; ordinary loading owns all native references and quantities.

/** @param arguments Exact zero-argument startup fixture command. @return Whether the actual accepted-world construction observer executed. */
static bool ObserveGeothermalCollector(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_geothermal_collector_observer != nullptr); (*_geothermal_collector_observer)(); return true;
}

/** Exact real native construction/shortage/completion files and independent corrupt preview retries preserve the original geological/electrical owners. */
TEST_CASE("Native actual geothermal collector construction survives real files", "[.geothermal-collector-file]")
{
	const auto root = std::filesystem::current_path().parent_path(), directory = root / "src/tests/fixtures/geothermal";
	const char *input_text = std::getenv("OTTD_GEOTHERMAL_COLLECTOR_INPUT"), *mode_text = std::getenv("OTTD_GEOTHERMAL_COLLECTOR_CASE");
	REQUIRE(input_text != nullptr); REQUIRE(mode_text != nullptr);
	const std::filesystem::path input{input_text}; const std::string_view mode{mode_text};
	REQUIRE(std::filesystem::exists(input)); REQUIRE(std::filesystem::is_directory(directory));
	bool observed = false; std::exception_ptr failure;
	/** Inspect actual accepted native stocks/work/source identities; explicit boundary delivery is not evidence of a driven Surface bootstrap. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			const auto electrical = ComputeUtilityPowerDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			const auto cold = CaptureGeothermalSiteClaims(); REQUIRE(cold.size() == 1); REQUIRE(cold[0].site.kind == MultiLayerGeothermalConstructionKind::Collector);
			if (mode == "legacy-read") {
				CHECK(CaptureGeothermalCollectors().empty()); CHECK(cold[0].active);
			} else {
				if (mode == "fixture-fund") {
					REQUIRE(CaptureGeothermalCollectors().empty()); REQUIRE(cold[0].active);
					const auto cash = Company::Get(_current_company)->money;
					const auto result = Command<Commands::BuildGeothermalCollector>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, cold[0].site.tile,
						std::make_tuple(cold[0].site.tile, cold[0].id, uint32_t{456}));
					REQUIRE(result.Succeeded()); CHECK(result.GetCost() == Money{180000}); CHECK(Company::Get(_current_company)->money == cash - Money{180000});
				}
				const auto rows = CaptureGeothermalCollectors(); REQUIRE(rows.size() == 1); REQUIRE(ValidateGeothermalCollectors(rows));
				Industry *industry = Industry::GetIfValid(rows[0].industry); REQUIRE(industry != nullptr); CHECK_FALSE(GetGeothermalSiteClaim(rows[0].site)->active);
				if (mode == "fixture-shortage") {
					REQUIRE(rows[0].work_ticks == 0);
					REQUIRE(ReceiveGeothermalCollectorCargo(*industry, GetCargoTypeByLabel(CargoLabel{"STEL"}), 8) == 8);
					REQUIRE(ReceiveGeothermalCollectorCargo(*industry, GetCargoTypeByLabel(CargoLabel{"RFMT"}), 4) == 4);
					AutoRestoreBackup pause(_pause_mode, PauseModes{}); AdvanceGeothermalCollector(*industry);
					CHECK(GetGeothermalCollectorState(industry->index)->work_ticks == 0); CHECK_FALSE(IsGeothermalCollectorOperational(industry->index));
				} else if (mode == "fixture-partial" || mode == "fixture-complete") {
					if (mode == "fixture-partial") {
						REQUIRE(rows[0].work_ticks == 0); REQUIRE(ReceiveGeothermalCollectorCargo(*industry, GetCargoTypeByLabel(CargoLabel{"ELEC"}), 2) == 2);
					} else { REQUIRE(rows[0].work_ticks == 148); }
					AutoRestoreBackup pause(_pause_mode, PauseModes{});
					// Actual saved tick meter owns every labour credit; no fixture
					// writes a receipt/work/native completed flag directly.
					for (uint tick = 0; tick < 148; ++tick) { ++TimerGameTick::counter; AdvanceGeothermalCollector(*industry); }
					CHECK(GetGeothermalCollectorState(industry->index)->work_ticks == (mode == "fixture-partial" ? 148 : 296));
					CHECK(IsGeothermalCollectorOperational(industry->index) == (mode == "fixture-complete"));
				} else if (mode == "preview") {
					const auto *corrupt = std::getenv("OTTD_GEOTHERMAL_COLLECTOR_CORRUPT"); REQUIRE(corrupt != nullptr);
					const auto before = ComputeGeothermalCollectorDigest(), planning = ComputeGeothermalSiteDigest();
					CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(ComputeGeothermalCollectorDigest() == before); CHECK(ComputeGeothermalSiteDigest() == planning);
					REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(_load_check_data.geothermal_collector_rows.size() == 1); CHECK(ComputeGeothermalCollectorDigest() == before);
				} else {
					const uint16_t expected = mode == "partial-read" ? 148 : mode == "complete-read" ? 296 : 0;
					CHECK(rows[0].work_ticks == expected); CHECK(IsGeothermalCollectorOperational(industry->index) == (expected == 296));
					if (mode == "shortage-read") {
						CHECK(industry->GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == 8);
						CHECK(industry->GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"RFMT"}))->waiting == 4);
						CHECK(industry->GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"ELEC"}))->waiting == 0);
					}
				}
			}
			CHECK(ComputeUtilityPowerDigest() == electrical); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			if (mode != "preview") {
				const auto before = ComputeGeothermalCollectorDigest(); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / fmt::format("fixture389-collector-{}.sav", mode)), false) == SaveLoadResult::Ok);
				CHECK(ComputeGeothermalCollectorDigest() == before); CHECK(ComputeUtilityPowerDigest() == electrical);
			}
			fmt::print("GEOTHERMAL_COLLECTOR_FILE case={} construction={:016X} fixture_open_floor=true boundary_material_delivery=true transported_bootstrap=false electrical_unchanged=true\n", mode, ComputeGeothermalCollectorDigest());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_geothermal_collector_observer, &inspect); IConsole::CmdRegister("geothermal_collector_check", ObserveGeothermalCollector);
	const std::string config = (root / "build/native-art.cfg").string(), input_path = input.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", input_path};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
