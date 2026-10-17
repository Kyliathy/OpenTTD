/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_inspection_native.cpp Actual paid/scan inspector captures retain original resources, company knowledge and work.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14
 * These native widget captures qualify functional disclosure; final sprite-family acceptance remains Group14B.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid_inspection.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_structure.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../core/string_consumer.hpp"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "native_art_render_fixture.h"
#include "../safeguards.h"

static std::function<void()> *inspection_observer = nullptr; ///< Process-local native presentation observer; owns no simulation or asset identity.

/** @param args Parameter-free console invocation. @return Whether the exact loaded-world observer ran. */
static bool ObserveAsteroidInspection(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(inspection_observer); (*inspection_observer)(); return true; }

/** Save original current native owners without granting simulation work.
 * @param path Exact repository-owned ordinary438 checkpoint path.
 */
static void SaveInspectionWorld(const std::filesystem::path &path)
{
	REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** The actual native inspector must reveal only authorized quantities and must not change paid cargo, work, service, knowledge or money. */
TEST_CASE("Actual asteroid inspector captures preserve company knowledge and paid mission owners", "[.asteroid-inspection-native]")
{
	const char *source = std::getenv("OTTD_ASTEROID_INSPECT_SOURCE"), *selected = std::getenv("OTTD_ASTEROID_INSPECT_PHASE"); REQUIRE(source); REQUIRE(selected);
	const char *id_text = std::getenv("OTTD_ASTEROID_INSPECT_VEHICLE"), *generation_text = std::getenv("OTTD_ASTEROID_INSPECT_GENERATION"); REQUIRE(id_text); REQUIRE(generation_text);
	const auto id = ParseInteger<uint32_t>(id_text); const auto generation = ParseInteger<uint64_t>(generation_text); REQUIRE(id.has_value()); REQUIRE(generation.has_value());
	const std::string_view phase{selected}; const SpaceCraftRef selected_craft{VehicleID{*id}, *generation};
	const auto paths = GetTestCaseArtifactPaths("space", "asteroid-inspection"); const auto runtime = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Inspect only exact original paid/scan inputs; native rendering may disclose state but cannot supply work or material. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); const auto *craft = ResolveSpaceCraft(selected_craft); REQUIRE(craft); REQUIRE(craft->manifest);
			REQUIRE(craft->construction.placement == SpacePlacement::Orbital); const auto owner = craft->owner; REQUIRE(owner == _local_company);
			CompanyID viewer = owner;
			if (phase == "foreign") {
				// This exact completed-scan producer already contains the other real identity. Selection grants no company, assets, recipe or survey receipt.
				for (const auto *company : Company::Iterate()) if (company->index != owner) { viewer = company->index; break; }
				REQUIRE(viewer != owner);
			}
			const auto disclosure = GetAsteroidInspectionDisclosure(viewer, OrbitalBodyID{0}); REQUIRE(disclosure); CHECK(disclosure->quantities.has_value() == (phase == "known"));
			if (phase == "scan") { REQUIRE(craft->space_orders.work_ticks == 37); }
			if (phase == "mining") { REQUIRE(craft->mission.phase == SpaceMissionPhase::Mining); REQUIRE(craft->mission.work_ticks == 37); }
			if (phase == "pending") { REQUIRE(craft->manifest->pending_returns == 1); REQUIRE(CaptureSpaceDescents().size() == 1); }
			if (phase == "service") { REQUIRE(craft->manifest->service_ticks == 74); }
			if (phase == "construction") {
				REQUIRE(craft->space_orders.cursor < craft->space_orders.entries.size()); const auto &order = craft->space_orders.entries[craft->space_orders.cursor]; REQUIRE(order.action == SpaceOrderAction::Construct);
				const auto *site = ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(order.target.id)}, order.target.generation}); REQUIRE(site); REQUIRE(site->job.work_ticks > 0);
			}
			SaveInspectionWorld(paths.saves / "inspection-before-438.sav"); const auto fleet = ComputeSpaceVehicleDigest(), field = ComputeAsteroidFieldDigest(GetAsteroidField());
			const auto manifests = ComputeSpaceManifestDigest(), structures = ComputeSpaceStructureDigest(), pods = ComputeSpacePodDigest(); const auto random = _random;
			const auto money = Company::Get(owner)->money; const auto elapsed = GetAsteroidField().elapsed_ticks;
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple")); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			SetLocalCompany(viewer); ShowSpaceSurveyWindow(selected_craft); auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window);
			CaptureNativeArtWindow(*window, paths.images / fmt::format("inspector-{}.ppm", phase)); SetLocalCompany(owner);
			CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field); CHECK(ComputeSpaceManifestDigest() == manifests); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeSpacePodDigest() == pods);
			CHECK(Company::Get(owner)->money == money); CHECK(GetAsteroidField().elapsed_ticks == elapsed); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			SaveInspectionWorld(paths.saves / "inspection-after-438.sav"); fmt::print("ASTEROID_INSPECTION native=true phase={} known={} observer={} added_stock=false final_art=false\n", phase, disclosure->quantities.has_value(), viewer.base()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(inspection_observer, &observe); IConsole::CmdRegister("asteroid_inspection_test", ObserveAsteroidInspection); const std::string config = runtime.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
