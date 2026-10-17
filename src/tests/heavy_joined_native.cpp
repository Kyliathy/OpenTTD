/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_joined_native.cpp Actual retained heavy worlds exercise independent copied admission and source-linked inspectors.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CD
 * No fixture hull, inventory, work, utility account or historical receipt is created. Inputs are exact dated paid producer saves.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../orbital_service.h"
#include "../space_pod.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "assembly_native_fixture.h"
#include "native_art_render_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_joined_observer = nullptr; ///< Invocation callback only; never a cargo or service owner.

/** @param args Parameter-free callback in the actual loaded world. @return Whether the installed observer ran. */
static bool ObserveHeavyJoined(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_joined_observer != nullptr); (*heavy_joined_observer)(); return true;
}

/** @return Original physical, utility and mission authority for copied-preview and inspector purity. */
static auto HeavyJoinedAuthority()
{
	return std::array<uint64_t, 11>{ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest(), ComputeUtilityPowerDigest(), ComputeSpaceManifestDigest(),
		ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(), ComputeSpaceOrdersDigest(), ComputeAsteroidFieldDigest(GetAsteroidField()),
		ComputeLaunchProductionDigest(), ComputeLaunchCenterDigest(), ComputeSpaceDescentDigest()};
}

/** Current copied joins and ordinary upgrades inspect actual retained heavy, legacy and sender-lost worlds without advancing their owners. */
TEST_CASE("Joined heavy previews and inspectors preserve original paid physical authority", "[.heavy-joined-native]")
{
	const char *source = std::getenv("OTTD_HEAVY_JOINED_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "heavy-joined-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Keep actual loaded-world assertions alive through ordinary application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			const auto authority = HeavyJoinedAuthority(); const auto money = Company::Get(_current_company)->money; const auto random = _random;
			REQUIRE(ValidateSpaceDescents()); REQUIRE(ValidateOrbitalServices());
			REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			if (const char *corrupt = std::getenv("OTTD_HEAVY_JOINED_CORRUPT")) {
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK_FALSE(_load_check_data.error_msg.empty()); fmt::print("HEAVY_JOINED preview_error={}\n", _load_check_data.error_msg);
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				fmt::print("HEAVY_JOINED preview_reject=true authority_unchanged=true\n");
			} else {
				const auto saved = (paths.saves / "heavy-joined-437.sav").string();
				{
					AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
					REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok);
				}
				REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				REQUIRE(_load_check_data.native_cargo_dimensions_present);
				fmt::print("HEAVY_JOINED station_refs={} ordinary_refs={} configured_cargos={}\n", _load_check_data.native_station_cargo_refs.size(), _load_check_data.native_ordinary_vehicle_cargo_refs.size(), _load_check_data.native_cargo_dimension_rows.size());
				if (std::getenv("OTTD_HEAVY_JOINED_CAPTURE") != nullptr) {
					REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
					const auto *craft = ResolveSpaceCraft({VehicleID{22}, 8});
					if (craft == nullptr) craft = ResolveSpaceCraft({VehicleID{18}, 4});
					REQUIRE(craft != nullptr); ShowSpaceSurveyWindow(GetSpaceCraftReference(*craft));
					auto *inspector = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(inspector != nullptr); CaptureNativeArtWindow(*inspector, paths.images / "heavy-inspector.ppm");
					inspector->Close(); const auto *dock = ResolveSpaceStructure({SpaceStructureID{4}, 5});
					const auto *core = ResolveSpaceStructure({SpaceStructureID{0}, 1}); REQUIRE(core != nullptr);
					ShowSpaceConstructionWindow(dock == nullptr ? GetSpaceStructureReference(*core) : GetSpaceStructureReference(*dock));
					auto *service = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(service != nullptr);
					if (std::getenv("OTTD_HEAVY_JOINED_SECOND_BERTH") != nullptr) service->OnClick({}, 29, 1);
					CaptureNativeArtWindow(*service, paths.images / "heavy-service.ppm");
					service->Close();
				}
			}
			CHECK(HeavyJoinedAuthority() == authority); CHECK(Company::Get(_current_company)->money == money);
			CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			fmt::print("HEAVY_JOINED native=true saved={}\n", std::getenv("OTTD_HEAVY_JOINED_CORRUPT") == nullptr); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(heavy_joined_observer, &observe); IConsole::CmdRegister("heavy_joined_test", ObserveHeavyJoined); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
