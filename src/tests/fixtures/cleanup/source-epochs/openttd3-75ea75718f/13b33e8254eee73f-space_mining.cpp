/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining.cpp Native fifth-fleet identity and actual finite extraction acceptance. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../engine_base.h"
#include "../asteroid.h"
#include "../vehiclelist.h"
#include "../vehicle_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../settings_type.h"
#include "../saveload/saveload_func.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../window_gui.h"
#include "layer_save_fixture.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include <set>
#include <thread>
#include "../safeguards.h"

TEST_CASE("Space company indexing excludes effects and preserves all old fleet window numbers", "[space-native]")
{
	STATIC_REQUIRE(to_underlying(VehicleType::Space) == 6);
	STATIC_REQUIRE(to_underlying(VehicleType::Effect) == 4);
	STATIC_REQUIRE(to_underlying(VehicleType::Disaster) == 5);
	CHECK_FALSE(IsCompanyVehicleType(VehicleType::Effect));
	CHECK_FALSE(IsCompanyVehicleType(VehicleType::Disaster));
	CHECK_FALSE(IsCompanyVehicleType(VehicleType::Invalid));
	VehicleTypeIndexArray<uint32_t> values{11, 22, 33, 44, 55};
	std::set<WindowNumber> windows;
	for (const auto type : COMPANY_VEHICLE_TYPES) {
		CHECK(values[type] == (CompanyVehicleIndex(type) + 1) * 11);
		for (const auto kind : EnumRange(VehicleListType::End)) for (const CompanyID company : {CompanyID{0}, CompanyID{7}, OWNER_NONE}) for (uint32_t index : {0U, 1U, (1U << 20) - 1}) {
			const WindowNumber number = VehicleListIdentifier(kind, type, company, index).ToWindowNumber();
			CHECK(windows.insert(number).second);
			const uint32_t owner = company == OWNER_NONE ? 15 : company.base();
			if (type != VehicleType::Space) CHECK(number == (owner << 28 | to_underlying(kind) << 23 | to_underlying(type) << 26 | index));
		}
	}
}

static std::function<void()> *_mining_observer = nullptr;
static bool ObserveMining(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(_mining_observer != nullptr); (*_mining_observer)(); return true;
}

TEST_CASE("Native probes mine finite resources and retain conserved aged cargo in ordinary saves", "[.space-mining]")
{
	const char *action = std::getenv("OTTD_SPACE_MINING_ACTION"); REQUIRE(action != nullptr);
	const bool prepare = std::string_view{action} == "prepare";
	const char *input = std::getenv("OTTD_SPACE_MINING_SOURCE");
	const auto source = prepare ? FindTestRepositoryRoot() / "src/tests/fixtures/demos/space/space-survey-approach-1990-404.sav" : std::filesystem::path{input == nullptr ? "" : input};
	const auto paths = GetTestCaseArtifactPaths("space", "mining");
	const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			if (prepare) {
				const Money money = Company::Get(_local_company)->money;
				REQUIRE(PrepareNativeSpaceMiningDemo());
				CHECK(Company::Get(_local_company)->money == money - 120000);
				CHECK(GetAsteroidField().surveys.empty());
			}
			REQUIRE(ValidateSpaceVehicles());
			REQUIRE(Company::Get(_local_company)->group_all[VehicleType::Space].num_vehicle == 3);
			std::vector<SpaceVehicle *> probes;
			for (auto *v : SpaceVehicle::Iterate()) probes.push_back(v);
			REQUIRE(probes.size() == 3);
			uint16_t secondary_capacity = 123;
			CHECK(probes.front()->GetEngine()->DetermineCapacity(probes.front(), &secondary_capacity) == 20);
			CHECK(secondary_capacity == 0);
			// Reload must insert each native viewport entry once. A duplicate insertion
			// forms a self/cyclic chain and otherwise hangs the real renderer below.
			for (const auto *probe : probes) {
				std::set<VehicleID> visited;
				for (const Vehicle *entry = probe; entry != nullptr; entry = entry->hash_viewport_next) REQUIRE(visited.insert(entry->index).second);
			}
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			const TileIndex centre = TileXYL(GetAsteroidField().width / 2, GetAsteroidField().height / 2 - 2, MapLayer::Space);
			SetViewportLayer(GetMainWindow(), MapLayer::Space); ScrollMainWindowToTile(centre, true);
			auto save = [&](std::string_view scene) {
				AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("space-mining-{}-405.sav", scene)), false) == SaveLoadResult::Ok);
			};
			if (prepare) save("approach");
			if (prepare) {
				// Test simultaneous arrival after saving the normal playable three-target start.
				probes[1]->position = probes[0]->position; probes[1]->UpdateSpacePosition();
				REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, probes[1]->index, 0).Succeeded());
			}
			const auto before = GetAsteroidField();
			uint32_t initial_hold = 0; for (auto *v : probes) initial_hold += v->cargo.TotalCount();
			std::array<uint32_t, 3> mining_ticks{}, expected_mining_ticks{};
			for (uint j = 0; j < 3; ++j) {
				const auto *v = probes[j];
				expected_mining_ticks[j] = (20 - v->cargo.TotalCount()) / 2 * 74;
				if (v->mission.phase == SpaceMissionPhase::Mining) expected_mining_ticks[j] -= v->mission.work_ticks;
			}
			{
				AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
				for (uint i = 0; i < 10; ++i) StateGameLoop();
				CHECK(GetAsteroidField() == before);
				uint32_t held = 0; for (auto *v : probes) held += v->cargo.TotalCount(); CHECK(held == initial_hold);
			}
			// Invalid and estimate commands must not mutate a mission, slot, cargo or money.
			const auto target = probes.front()->mission.target;
			const auto phase = probes.front()->mission.phase;
			CHECK(CmdSetSpaceMission({}, probes.front()->index, UINT32_MAX - 1).Failed());
			CHECK(CmdSetSpaceMission({}, probes.front()->index, UINT32_MAX).Succeeded());
			CHECK(probes.front()->mission.target == target); CHECK(probes.front()->mission.phase == phase);
			{
				AutoRestoreBackup foreign(_current_company, OWNER_NONE);
				CHECK(CmdSetSpaceMission(DoCommandFlag::Execute, probes.front()->index, UINT32_MAX).Failed());
			}
			AutoRestoreBackup running(_pause_mode, PauseModes{});
			bool mining_saved = false;
			SetViewportLayer(GetMainWindow(), MapLayer::Surface);
			uint attempts = 0;
			for (uint i = 0; i < 2200;) {
				REQUIRE(++attempts < 100000);
				if (i == 1100) SetViewportLayer(GetMainWindow(), MapLayer::Space);
				std::array<bool, 3> was_mining;
				std::array<Money, 3> expected_profit;
				std::array<Money, 3> paused_profit;
				for (uint j = 0; j < 3; ++j) {
					const auto *v = probes[j];
					paused_profit[j] = v->profit_this_year;
					was_mining[j] = v->mission.phase == SpaceMissionPhase::Mining;
					const uint rate = was_mining[j] ? 500 : 100;
					expected_profit[j] = v->profit_this_year - Money{(v->mission.cost_remainder + rate) / 74} * 256;
				}
				const uint64_t tick = GetAsteroidField().elapsed_ticks;
				StateGameLoop();
				if (GetAsteroidField().elapsed_ticks == tick) {
					// LinkGraph jobs can legitimately pause a reloaded world until their
					// worker completes. Count simulation ticks and allow that worker to run.
					for (uint j = 0; j < 3; ++j) REQUIRE(probes[j]->profit_this_year == paused_profit[j]);
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
				REQUIRE(GetAsteroidField().elapsed_ticks == tick + 1);
				++i;
				for (uint j = 0; j < 3; ++j) if (was_mining[j]) ++mining_ticks[j];
				if (probes[1]->mission.target == OrbitalBodyID{0} && probes[0]->mission.phase == SpaceMissionPhase::Mining) {
					CHECK(GetAsteroidField().bodies[0].mining_slot == probes[0]->index);
					CHECK(probes[1]->mission.phase == SpaceMissionPhase::Approach);
					CHECK(probes[1]->cargo.TotalCount() == 0);
					REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, probes[1]->index, 1).Succeeded());
				}
				REQUIRE(ValidateSpaceVehicles());
				for (uint j = 0; j < 3; ++j) REQUIRE(probes[j]->profit_this_year == expected_profit[j]);
				uint32_t extracted = 0, held = 0;
				for (const auto &body : GetAsteroidField().bodies) extracted += before.bodies[body.id.base()].remaining_reserve - body.remaining_reserve;
				for (auto *v : probes) held += v->cargo.TotalCount();
				REQUIRE(extracted + initial_hold == held);
				if (!mining_saved && probes.front()->mission.phase == SpaceMissionPhase::Mining && probes.front()->cargo.TotalCount() >= 2 && probes.front()->mission.work_ticks == 37) {
					mining_saved = true;
					if (prepare) {
						SetViewportLayer(GetMainWindow(), MapLayer::Space); save("working"); SetViewportLayer(GetMainWindow(), MapLayer::Surface);
						// Recall during partial extraction must release the lease without losing packets.
						auto *probe = probes.front();
						expected_mining_ticks[0] += probe->mission.work_ticks; // Recalled partial work is paid but does not extract.
						const auto hold = probe->cargo.TotalCount();
						const auto reserves = GetAsteroidField().bodies[0].remaining_reserve;
						REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, probe->index, UINT32_MAX).Succeeded());
						CHECK(probe->mission.phase == SpaceMissionPhase::Return);
						CHECK(GetAsteroidField().bodies[0].mining_slot == VehicleID::Invalid());
						CHECK(probe->cargo.TotalCount() == hold);
						CHECK(GetAsteroidField().bodies[0].remaining_reserve == reserves);
						CHECK(CmdSetSpaceMission(DoCommandFlag::Execute, probe->index, 1).Failed());
						CHECK(probe->mission.phase == SpaceMissionPhase::Return);
						CHECK(probe->cargo.TotalCount() == hold);
						REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, probe->index, 0).Succeeded());
						CHECK(probe->mission.phase == SpaceMissionPhase::Approach);
						REQUIRE(ValidateSpaceVehicles());
					}
				}
			}
			if (prepare) REQUIRE(mining_saved);
			for (uint j = 0; j < 3; ++j) CHECK(mining_ticks[j] == expected_mining_ticks[j]);
			for (auto *v : probes) {
				CHECK(v->cargo.TotalCount() == 20); CHECK(v->mission.phase == SpaceMissionPhase::Holding); CHECK(v->position == v->home);
				const auto unchanged = ComputeAsteroidFieldDigest(GetAsteroidField());
				CHECK(CmdSetSpaceMission(DoCommandFlag::Execute, v->index, v->mission.target.base()).Failed());
				CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == unchanged);
				for (const auto *packet : *v->cargo.Packets()) { CHECK(packet->GetSource().type == SourceType::Asteroid); CHECK(packet->GetPeriodsInTransit() > 0); CHECK(GetTileLayer(packet->GetSourceTile()) == MapLayer::Space); }
			}
			fmt::print("SPACE_MINING simulation_complete ticks=2200 cargo=60\n"); std::fflush(stdout);
			CaptureNativeArtViewport(centre, ZoomLevel::Normal, paths.images / "space-mining-holds.ppm");
			save(prepare ? "holds" : "continued");
			fmt::print("SPACE_MINING action={} ticks=2200 conserved=true holds=60 native=true field_digest={}\n", action, ComputeAsteroidFieldDigest(GetAsteroidField()));
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_mining_observer, &observe);
	IConsole::CmdRegister("space_mining_test", ObserveMining);
	const std::string source_path = source.string(), config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
