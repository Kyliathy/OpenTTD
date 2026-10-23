/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_flight.cpp Native rocket phase, identity, fuel and exact interruption fixtures. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_transition.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../launch_operations.h"
#include "../launch_center_cmd.h"
#include "../asteroid.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../station_base.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

TEST_CASE("Native rocket phases require one round trip and consistent burn receipts", "[space-logistics-contract]")
{
	const LaunchCenterRef center{StationID{1}, 4}; const SpaceCraftRef rocket{VehicleID{2}, 7};
	SpaceTransitionRecord r; r.craft = rocket; auto &f = r.state;
	f.phase = SpaceFlightPhase::Preparing; f.source = f.recovery = center; f.destination = {SpaceOrderTargetKind::Body, 0, 0}; f.remaining_ticks = 148; f.missions_started = 1;
	SpaceProductionRecord p{rocket, {SpacePlacement::InTransit, center, center, TileIndex{20}, 0, 2, 14, 400000, false}};
	LaunchOperationsRecord op{center, {}}; op.slots.pad_active = rocket;
	CHECK(ValidateSpaceTransitionRecords(std::array{r})); CHECK(ValidateSpaceTransitionLinks(std::array{r}, std::array{p}, std::array{op}));
	for (uint i = 0; i < 10; ++i) {
		auto bad = r;
		switch (i) {
			case 0: bad.craft.generation = 0; break;
			case 1: bad.state.remaining_ticks = 149; break;
			case 2: bad.state.outbound_burned = true; break;
			case 3: bad.state.pad_cost_remainder = 74; break;
			case 4: bad.state.missions_completed = 1; break;
			case 5: bad.state.destination.generation = 1; break;
			case 6: bad.state.layer = MapLayer::Space; break;
			case 7: bad.state.source.generation = 0; break;
			case 8: bad.state.deployed = true; break;
			case 9: bad.state.missions_canceled = 2; break;
		}
		CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{bad}));
	}
	CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{r, r}));
	f.phase = SpaceFlightPhase::Ascent; f.remaining_ticks = 222; f.outbound_burned = true; CHECK(ValidateSpaceTransitionRecords(std::array{r}));
	f.phase = SpaceFlightPhase::OrbitalDelivery; f.remaining_ticks = 0; f.layer = MapLayer::Space; CHECK(ValidateSpaceTransitionRecords(std::array{r}));
	f.phase = SpaceFlightPhase::ReturnRecovery; f.remaining_ticks = 222; CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{r})); f.recovery_burned = true; CHECK(ValidateSpaceTransitionRecords(std::array{r}));
	f.phase = SpaceFlightPhase::Turnaround; f.remaining_ticks = 74; f.layer = MapLayer::Surface; f.deployed = true; CHECK(ValidateSpaceTransitionRecords(std::array{r}));
	f.phase = SpaceFlightPhase::Holding; f.blocked = SpaceFlightBlock::HomeLost; CHECK(ValidateSpaceTransitionRecords(std::array{r}));
	f.layer = MapLayer::Space; CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{r})); f.remaining_ticks = 0; CHECK(ValidateSpaceTransitionRecords(std::array{r}));
	op.slots.pad_active.generation++; CHECK_FALSE(ValidateSpaceTransitionLinks(std::array{r}, std::array{p}, std::array{op}));
	f = {}; f.missions_started = 2; f.missions_completed = 1; f.missions_canceled = 1; CHECK(ValidateSpaceTransitionRecords(std::array{r}));
	f.deployed = true; CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{r}));
	LoadCheckData preview; preview.space_transitions_present = true; preview.space_transition_rows.push_back(r); preview.Clear();
	CHECK_FALSE(preview.space_transitions_present); CHECK(preview.space_transition_rows.empty());
}

static std::function<void()> *flight_observer = nullptr;
static bool ObserveFlight(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(flight_observer != nullptr); (*flight_observer)(); return true; }
TEST_CASE("The same native rocket deploys paid probes once and survives every interrupted flight phase", "[.launch-flight]")
{
	const char *action_env = std::getenv("OTTD_LAUNCH_FLIGHT_ACTION"); const char *input = std::getenv("OTTD_LAUNCH_FLIGHT_SOURCE");
	REQUIRE(action_env != nullptr); REQUIRE(input != nullptr); const std::string_view action{action_env}; const std::filesystem::path source{input};
	const char *phase_env = std::getenv("OTTD_LAUNCH_FLIGHT_PHASE"); const std::string_view phase{phase_env == nullptr ? "preparing" : phase_env};
	const auto paths = GetTestCaseArtifactPaths("space", "flight"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); REQUIRE(GetLaunchCenterReferences().size() == 1);
			const auto center_ref = GetLaunchCenterReferences().front(); auto *center = ResolveLaunchCenter(center_ref); auto *company = Company::Get(_current_company);
			auto find = [&](uint16_t model) -> SpaceVehicle * { for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == model) return v; return nullptr; };
			auto *rocket = find(14); auto *miner = find(15); auto *survey = find(19); auto *builder = find(20); REQUIRE((rocket && miner && survey && builder));
			const auto rocket_ref = GetSpaceCraftReference(*rocket), miner_ref = GetSpaceCraftReference(*miner);
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
				for (uint i = 0; i < ticks;) { REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks;
					StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++i;
				}
			};
			auto save = [&](std::string_view name) { REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok); };
			if (action == "loss") {
				SpaceVehicle *active = nullptr; for (auto *v : SpaceVehicle::Iterate()) if (IsSpaceFlightActive(*v)) { REQUIRE(active == nullptr); active = v; } REQUIRE(active != nullptr);
				const auto identity = GetSpaceCraftReference(*active); const auto burned = active->manifest->fuel_burned;
				if (active->transition->phase == SpaceFlightPhase::Ascent) { REQUIRE(active->transition->remaining_ticks == 185); advance(185 + 1 + 222); }
				REQUIRE(active->transition->phase == SpaceFlightPhase::Holding);
				const uint recovery = active->transition->layer == MapLayer::Space ? 222 + 74 : active->transition->remaining_ticks;
				REQUIRE(Command<Commands::DivertLaunchRecovery>::Post(active->index, active->generation, center_ref.station, center_ref.generation)); advance(recovery);
				CHECK(GetSpaceCraftReference(*active) == identity); CHECK(active->construction.placement == SpacePlacement::SurfaceStored);
				CHECK(active->manifest->fuel_burned == burned + (burned == 10 ? 5 : 0)); save("flight-loss-complete-414.sav");
				fmt::print("LAUNCH_FLIGHT native=true loss_reload=true lost_home=true phase={} digest={}\n", phase, ComputeSpaceVehicleDigest()); observed = true; _exit_game = true; return;
			}
			if (action == "prepare") {
				REQUIRE(rocket->construction.placement == SpacePlacement::SurfaceStored); CHECK(CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::Body, 0, 0).Failed());
				REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, miner->index, miner->generation, false));
				const auto money = company->money; const auto digest = ComputeSpaceVehicleDigest();
				const auto quote = CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::Body, 0, 0); REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == 5000);
				CHECK(company->money == money); CHECK(ComputeSpaceVehicleDigest() == digest);
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0})); CHECK(company->money == money - 5000);
				advance(37); CHECK(rocket->manifest->fuel_burned == 0); CHECK(rocket->manifest->supplies[0].TotalCount() == 15);
				REQUIRE(Command<Commands::CancelLaunchFlight>::Post(rocket->index, rocket->generation)); CHECK(rocket->transition->missions_canceled == 1); CHECK(rocket->transition->pad_cost_remainder == 0);
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				const uint ticks = phase == "preparing" ? 37 : phase == "ascent" ? 185 : phase == "delivery" ? 370 : phase == "return" ? 408 : 630;
				advance(ticks); save("flight-partial-414.sav");
			}
			if (const char *corrupt = std::getenv("OTTD_LAUNCH_FLIGHT_CORRUPT")) {
				const auto digest = ComputeSpaceVehicleDigest();
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(ComputeSpaceVehicleDigest() == digest); fmt::print("LAUNCH_FLIGHT preview_reject=true authority_unchanged=true\n");
			}
			REQUIRE(IsSpaceFlightActive(*rocket)); const auto &f = *rocket->transition;
			const uint elapsed = f.phase == SpaceFlightPhase::Preparing ? 148 - f.remaining_ticks : f.phase == SpaceFlightPhase::Ascent ? 370 - f.remaining_ticks :
				f.phase == SpaceFlightPhase::OrbitalDelivery ? 370 : f.phase == SpaceFlightPhase::ReturnRecovery ? 593 - f.remaining_ticks : 667 - f.remaining_ticks;
			REQUIRE(elapsed < 1000); advance(1000 - elapsed); REQUIRE(ValidateSpaceVehicles());
			CHECK(GetSpaceCraftReference(*rocket) == rocket_ref); CHECK(GetSpaceCraftReference(*miner) == miner_ref); CHECK(miner->construction.deployed); CHECK(miner->construction.placement == SpacePlacement::Orbital);
			CHECK(rocket->construction.placement == SpacePlacement::SurfaceStored); CHECK(rocket->manifest->passenger == SpaceCraftRef{}); CHECK(rocket->manifest->fuel_burned == 15); CHECK(rocket->manifest->supplies[0].TotalCount() == 0);
			CHECK(rocket->transition->missions_started == 2); CHECK(rocket->transition->missions_completed == 1); CHECK(rocket->transition->missions_canceled == 1); CHECK(SpacePod::GetNumItems() == 10);
			REQUIRE(Command<Commands::AssembleLaunchCraft>::Post(center_ref.station, center_ref.generation, uint16_t{14})); auto *competitor = ResolveSpaceCraft(center->launch_operations->workshop); REQUIRE(competitor != nullptr); advance(148);
			REQUIRE(Command<Commands::CarryLaunchProbe>::Post(competitor->index, competitor->generation, builder->index, builder->generation, false));
			REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center_ref.station, center_ref.generation, competitor->index, competitor->generation, uint8_t{0}, uint16_t{15}, false));
			REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, survey->index, survey->generation, false));
			REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
			REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Body, uint32_t{1}, uint64_t{0}));
			const auto competing = ComputeSpaceVehicleDigest(); CHECK(CmdBeginLaunchFlight({}, competitor->index, competitor->generation, SpaceOrderTargetKind::Body, 2, 0).Failed()); CHECK(ComputeSpaceVehicleDigest() == competing);
			advance(667); REQUIRE(ValidateSpaceVehicles()); CHECK(rocket->transition->missions_completed == 2); CHECK(rocket->manifest->fuel_burned == 30); CHECK(survey->construction.deployed);
			CHECK(GetSpaceCraftReference(*rocket) == rocket_ref); CHECK(GetSpaceVehicleCount() == 5); CHECK(builder->cargo.TotalCount() == 40); CHECK(builder->construction.placement == SpacePlacement::Carried);
			save("flight-complete-414.sav");
			// Forced facility loss during ascent keeps the same rocket/passenger in their current layer.
			REQUIRE(Command<Commands::BeginLaunchFlight>::Post(competitor->index, competitor->generation, SpaceOrderTargetKind::Body, uint32_t{2}, uint64_t{0})); advance(185);
			CHECK(competitor->transition->phase == SpaceFlightPhase::Ascent); const auto lost = GetLaunchCenterReference(*center); const auto anchor = center->launch_center->tile; const auto rotation = center->launch_center->rotation;
			ForgetLaunchCenter(*center); CHECK(competitor->construction.placement == SpacePlacement::InTransit); CHECK(builder->construction.placement == SpacePlacement::Carried); REQUIRE(ValidateSpaceVehicles());
			InstallNewLaunchCenter(*center, anchor, rotation); auto replacement = GetLaunchCenterReference(*center); CHECK(replacement != lost);
			for (auto *v : SpaceVehicle::Iterate()) if (v->construction.placement == SpacePlacement::Unhomed) REQUIRE(Command<Commands::RecoverLaunchCraft>::Post(replacement.station, replacement.generation, v->index, v->generation));
			for (auto *pod : SpacePod::Iterate()) if (pod->state.custody == SpacePodCustody::Unhomed) REQUIRE(Command<Commands::RecoverLaunchPod>::Post(replacement.station, replacement.generation, pod->index, pod->generation));
			save("flight-lost-ascent-414.sav");
			advance(185 + 1 + 222); CHECK(competitor->transition->phase == SpaceFlightPhase::Holding); CHECK(GetSpaceVehicleLayer(*competitor) == MapLayer::Space); CHECK(competitor->construction.home == LaunchCenterRef{});
			save("flight-lost-orbit-414.sav");
			CHECK(builder->construction.deployed); CHECK(builder->cargo.TotalCount() == 40); CHECK(CmdDivertLaunchRecovery({}, competitor->index, competitor->generation, lost.station, lost.generation).Failed());
			REQUIRE(Command<Commands::DivertLaunchRecovery>::Post(competitor->index, competitor->generation, replacement.station, replacement.generation)); advance(222 + 74);
			CHECK(competitor->construction.placement == SpacePlacement::SurfaceStored); CHECK(competitor->manifest->fuel_burned == 15); REQUIRE(ValidateSpaceVehicles());
			save("flight-recovered-orbit-414.sav");
			// A recovered hull interrupted during turnaround retains its remaining physical work at the same site.
			REQUIRE(Command<Commands::AssembleLaunchCraft>::Post(replacement.station, replacement.generation, uint16_t{19})); auto *fresh = ResolveSpaceCraft(center->launch_operations->workshop); REQUIRE(fresh != nullptr); advance(148);
			REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, fresh->index, fresh->generation, false));
			REQUIRE(Command<Commands::MoveSpaceSupply>::Post(replacement.station, replacement.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
			REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::LaunchCenter, uint32_t{replacement.station.base()}, replacement.generation)); advance(630);
			REQUIRE(rocket->transition->phase == SpaceFlightPhase::Turnaround); REQUIRE(rocket->transition->remaining_ticks == 37);
			ForgetLaunchCenter(*center); CHECK(rocket->transition->phase == SpaceFlightPhase::Holding); CHECK(rocket->transition->remaining_ticks == 37); CHECK(IsSpaceCraftAwaitingSurfaceRecovery(*rocket)); REQUIRE(ValidateSpaceVehicles());
			InstallNewLaunchCenter(*center, anchor, rotation); const auto rebuilt = GetLaunchCenterReference(*center);
			for (auto *v : SpaceVehicle::Iterate()) if (v->construction.placement == SpacePlacement::Unhomed) REQUIRE(Command<Commands::RecoverLaunchCraft>::Post(rebuilt.station, rebuilt.generation, v->index, v->generation));
			for (auto *pod : SpacePod::Iterate()) if (pod->state.custody == SpacePodCustody::Unhomed) REQUIRE(Command<Commands::RecoverLaunchPod>::Post(rebuilt.station, rebuilt.generation, pod->index, pod->generation));
			save("flight-lost-turnaround-414.sav");
			REQUIRE(Command<Commands::DivertLaunchRecovery>::Post(rocket->index, rocket->generation, rebuilt.station, rebuilt.generation)); CHECK(rocket->transition->remaining_ticks == 37); advance(37);
			CHECK(rocket->construction.placement == SpacePlacement::SurfaceStored); CHECK(rocket->manifest->fuel_burned == 45); CHECK(GetSpaceCraftReference(*rocket) == rocket_ref); REQUIRE(ValidateSpaceVehicles());
			save("flight-recovered-turnaround-414.sav");
			fmt::print("LAUNCH_FLIGHT native=true deploy_once=true fuel_once=true reusable=true lost_home=true turnaround_recovery=true phase={} digest={}\n", phase, ComputeSpaceVehicleDigest());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(flight_observer, &observe); IConsole::CmdRegister("launch_flight_test", ObserveFlight);
	const std::string source_path = source.string(), config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
