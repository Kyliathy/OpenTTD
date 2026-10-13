/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_logistics.cpp Native supply custody/service contracts; Plan9 connected logistics batch. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../launch_operations.h"
#include "../space_vehicle.h"
#include "../multilayer_space_content.h"
#include "../fios.h"
#include "../launch_center_cmd.h"
#include "../asteroid.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../window_gui.h"
#include "../window_func.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../station_base.h"
#include "../hub_cargo.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

TEST_CASE("Space supply compartments retain separate canonical capacities", "[space-logistics-contract]")
{
	CHECK(GetSpaceSupplyCapacity(14, SpaceSupply::Propellant) == 40);
	CHECK(GetSpaceSupplyCapacity(14, SpaceSupply::RefinedMetal) == 64);
	CHECK(GetSpaceSupplyCapacity(14, SpaceSupply::Electronics) == 32);
	for (uint16_t model : {15, 19, 20}) {
		CHECK(GetSpaceSupplyCapacity(model, SpaceSupply::Propellant) == 0);
		CHECK(GetSpaceSupplyCapacity(model, SpaceSupply::RefinedMetal) == 8);
		CHECK(GetSpaceSupplyCapacity(model, SpaceSupply::Electronics) == 4);
	}
	CHECK(GetSpaceSupplyCapacity(14, SpaceSupply::End) == 0);
	CHECK(GetSpaceSupplyCapacity(18, SpaceSupply::Propellant) == 0);
	CHECK(GetSpaceSupplyCapacity(16, SpaceSupply::RefinedMetal) == 0);
}
TEST_CASE("Space manifest transport refuses duplicate custody and unearned work", "[space-logistics-contract]")
{
	SpaceManifestRecord r; r.craft = {VehicleID{2}, 5};
	r.supplies[0].push_back({CargoPacketID{3}, StationID::Invalid()});
	CHECK(ValidateSpaceManifestRecords(std::array{r}));
	CHECK_FALSE(ValidateSpaceManifestRecords(std::array{r, r}));
	for (uint test = 0; test < 13; ++test) {
		auto bad = r;
		switch (test) {
			case 0: bad.craft.generation = 0; break;
			case 1: bad.passenger = r.craft; break;
			case 2: bad.carrier = r.craft; break;
			case 3: bad.passenger = {VehicleID{3}, 0}; break;
			case 4: bad.passenger = {VehicleID{3}, 6}; bad.carrier = {VehicleID{4}, 7}; break;
			case 5: bad.age_ticks = Ticks::CARGO_AGING_TICKS; break;
			case 6: bad.service_ticks = 149; break;
			case 7: bad.service_paused = true; break;
			case 8: bad.returns_remaining = 3; break;
			case 9: bad.service_ticks = 1; bad.services_completed = UINT64_MAX; break;
			case 10: bad.pending_returns = 1; break;
			case 11: bad.service_location = SpaceServiceLocation::End; break;
			case 12: bad.service_location = SpaceServiceLocation::Onboard; break;
		}
		CHECK_FALSE(ValidateSpaceManifestRecords(std::array{bad}));
	}
	auto duplicate = r; duplicate.craft = {VehicleID{3}, 6};
	CHECK_FALSE(ValidateSpaceManifestRecords(std::array{r, duplicate}));
	duplicate.supplies[0][0].first = CargoPacketID{4}; CHECK(ValidateSpaceManifestRecords(std::array{r, duplicate}));
	duplicate.supplies[1].push_back(duplicate.supplies[0][0]); CHECK_FALSE(ValidateSpaceManifestRecords(std::array{duplicate}));
	auto bad = r; bad.supplies[0][0].first = CargoPacketID::Invalid(); CHECK_FALSE(ValidateSpaceManifestRecords(std::array{bad}));
	bad = r; bad.supplies[0].resize(129, {CargoPacketID{3}, StationID::Invalid()}); CHECK_FALSE(ValidateSpaceManifestRecords(std::array{bad}));
	LoadCheckData preview; preview.space_manifests_present = true; preview.space_manifest_rows.push_back(r);
	preview.Clear(); CHECK_FALSE(preview.space_manifests_present); CHECK(preview.space_manifest_rows.empty());
}

TEST_CASE("Native Keep freight moves preserve age feeder share and routing through partial transfers", "[space-logistics-contract]")
{
	MockEnvironment::Instance(); ExcavationFixture world;
	StationCargoList stock{}, returned{}; VehicleCargoList hold{};
	const auto tile = TileXYL(20, 20, MapLayer::Surface);
	REQUIRE(CargoPacket::CanAllocateItem()); auto *packet = CargoPacket::Create(uint16_t{23}, uint16_t{7}, StationID{3}, tile, Money{230});
	stock.Append(packet, StationID{4}); const auto seeds = _random;
	CHECK_FALSE(HubCargoTransfer::CanMove(hold, 1)); REQUIRE(HubCargoTransfer::Move(stock, hold, 9, tile));
	CHECK(stock.TotalCount() == 14); CHECK(hold.TotalCount() == 9); CHECK(hold.GetFeederShare() == 90); CHECK(hold.ReservedCount() == 0); CHECK(hold.UnloadCount() == 0);
	const auto *held = hold.Packets()->front(); CHECK(held->GetPeriodsInTransit() == 7); CHECK(held->GetSourceTile() == tile); CHECK(held->GetFirstStation() == StationID{3}); CHECK(held->GetNextHop() == StationID{4});
	hold.AgeCargo(); REQUIRE(HubCargoTransfer::Move(hold, returned, 4, tile));
	CHECK(hold.TotalCount() == 5); CHECK(hold.GetFeederShare() == 50); CHECK(returned.TotalCount() == 4); CHECK(returned.PeriodsInTransit() == 8);
	const auto *back = returned.Packets()->begin()->second.front(); CHECK(back->GetFeederShare() == 40); CHECK(back->GetSourceTile() == tile); CHECK(back->GetFirstStation() == StationID{3}); CHECK(returned.Packets()->begin()->first == StationID{4});
	GoodsEntry destination_goods{};
	REQUIRE(hold.Stage(true, StationID{5}, {}, OrderUnloadType::Unload, &destination_goods, CargoType{0}, nullptr, tile));
	CHECK(hold.ActionCount(VehicleCargoList::MoveToAction::Deliver) == 5);
	CHECK_FALSE(HubCargoTransfer::CanMove(hold, 1)); CHECK_FALSE(HubCargoTransfer::Move(hold, returned, 1, tile)); CHECK(hold.TotalCount() == 5); CHECK(returned.TotalCount() == 4);
	hold.KeepAll(); REQUIRE(HubCargoTransfer::Move(hold, returned, 5, tile)); CHECK(hold.TotalCount() == 0); CHECK(hold.GetFeederShare() == 0); CHECK(returned.TotalCount() == 9);
	CHECK(_random.state[0] == seeds.state[0]); CHECK(_random.state[1] == seeds.state[1]);
}

TEST_CASE("Passive pod receipts and custody refuse duplicate packing and workshop owners", "[space-logistics-contract]")
{
	const LaunchCenterRef center{StationID{1}, 4};
	SpacePodRecord r; r.ref = {SpacePodID{3}, 9}; auto &s = r.state;
	s.owner = Owner{0}; s.custody = SpacePodCustody::Assembling; s.home = s.origin = center;
	s.surface_tile = TileIndex{20}; s.position = {20 * SPACE_Q16, 20 * SPACE_Q16}; s.remaining_ticks = 74; s.content_version = 2; s.capital_paid = 2000;
	LaunchOperationsRecord op{center, {}}; op.slots.pod_workshop = r.ref;
	CHECK(ValidateSpacePodRecords(std::array{r}, 10, 64, 64));
	CHECK(ValidateSpacePodLinks(std::array{r}, {}, std::array{op}, {}));
	CHECK_FALSE(ValidateSpacePodRecords(std::array{r, r}, 10, 64, 64));
	CHECK_FALSE(ValidateSpacePodRecords(std::array{r}, 9, 64, 64));
	CHECK_FALSE(ValidateSpacePodRecords(std::array{r}, 0, 64, 64));
	for (uint i = 0; i < 11; ++i) {
		auto bad = r;
		switch (i) {
			case 0: bad.ref.generation = 0; break;
			case 1: bad.ref.pod = SpacePodID::End(); break;
			case 2: bad.state.capital_paid = 0; break;
			case 3: bad.state.content_version = 1; break;
			case 4: bad.state.remaining_ticks = 75; break;
			case 5: bad.state.remaining_ticks = 0; break;
			case 6: bad.state.home.generation = 0; break;
			case 7: bad.state.origin.generation = 0; break;
			case 8: bad.state.surface_tile = TileIndex{4096}; break;
			case 9: bad.state.owner = OWNER_NONE; break;
			case 10: bad.state.craft = {VehicleID{2}, 8}; break;
		}
		CHECK_FALSE(ValidateSpacePodRecords(std::array{bad}, 10, 64, 64));
	}
	auto wrong = op; ++wrong.slots.pod_workshop.generation; CHECK_FALSE(ValidateSpacePodLinks(std::array{r}, {}, std::array{wrong}, {}));
	wrong = op; wrong.slots.workshop = {VehicleID{2}, 8}; CHECK_FALSE(ValidateLaunchOperationRecords(std::array{wrong}));
	wrong = op; wrong.slots.service = {VehicleID{2}, 8}; CHECK_FALSE(ValidateLaunchOperationRecords(std::array{wrong}));
	wrong = op; wrong.slots.pod_workshop = {}; CHECK_FALSE(ValidateSpacePodLinks(std::array{r}, {}, std::array{wrong}, {}));
	SpaceProductionRecord craft; craft.craft = {VehicleID{2}, 8}; craft.state.model = 15;
	SpaceManifestRecord manifest; manifest.craft = craft.craft; manifest.pods.push_back(r.ref);
	r.state.custody = SpacePodCustody::Craft; r.state.craft = craft.craft; r.state.home = {}; r.state.remaining_ticks = 0; op.slots.pod_workshop = {};
	CHECK(ValidateSpacePodRecords(std::array{r}, 10, 64, 64));
	CHECK(ValidateSpacePodLinks(std::array{r}, std::array{craft}, std::array{op}, std::array{manifest}));
	manifest.pods.push_back(r.ref); CHECK_FALSE(ValidateSpacePodLinks(std::array{r}, std::array{craft}, std::array{op}, std::array{manifest}));
	CHECK_FALSE(ValidateSpaceManifestRecords(std::array{manifest}));
	manifest.pods.resize(1); ++manifest.pods[0].generation; CHECK_FALSE(ValidateSpacePodLinks(std::array{r}, std::array{craft}, std::array{op}, std::array{manifest}));
	r.state.custody = SpacePodCustody::Unhomed; r.state.craft = {}; r.state.paused = true;
	CHECK(ValidateSpacePodRecords(std::array{r}, 10, 64, 64));
	LoadCheckData preview; preview.space_pods_present = true; preview.next_space_pod_generation = 10; preview.space_pod_rows.push_back(r); preview.Clear();
	CHECK_FALSE(preview.space_pods_present); CHECK(preview.next_space_pod_generation == 1); CHECK(preview.space_pod_rows.empty());
}

static std::function<void()> *logistics_observer = nullptr;
static bool ObserveLogistics(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(logistics_observer != nullptr); (*logistics_observer)(); return true;
}
TEST_CASE("Native paid pods supplies and freight retain single custody across interrupted service", "[.launch-logistics]")
{
	const char *action_env = std::getenv("OTTD_LAUNCH_LOGISTICS_ACTION"); REQUIRE(action_env != nullptr); const std::string_view action{action_env};
	const char *input = std::getenv("OTTD_LAUNCH_LOGISTICS_SOURCE");
	const auto source = action == "prepare" ? FindTestRepositoryRoot() / "src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav" : std::filesystem::path{input == nullptr ? "" : input};
	const auto paths = GetTestCaseArtifactPaths("space", "logistics"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			REQUIRE(ValidateSpaceVehicles()); REQUIRE(GetLaunchCenterReferences().size() == 1);
			auto center_ref = GetLaunchCenterReferences().front(); auto *center = ResolveLaunchCenter(center_ref); REQUIRE(center != nullptr);
			auto *company = Company::Get(_current_company);
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
				for (uint i = 0; i < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks;
					StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++i;
				}
			};
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			auto find = [&](uint16_t model) -> SpaceVehicle * { for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == model) return v; return nullptr; };
			if (action == "prepare") {
				REQUIRE(GetSpaceVehicleCount() == 0); REQUIRE(SpacePod::GetNumItems() == 0);
				for (const auto &[label, count] : std::array<std::pair<CargoLabel, uint16_t>, 5>{{{CargoLabel{"STEL"}, 100}, {CargoLabel{"GOOD"}, 200}, {CargoLabel{"RFMT"}, 16}, {CargoLabel{"ELEC"}, 16}, {CargoLabel{"PROP"}, 128}}}) {
					const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo)); REQUIRE(center->goods[cargo].TotalCount() == 0);
					REQUIRE(CargoPacket::CanAllocateItem()); auto *cp = CargoPacket::Create(count, uint16_t{9}, center->index, center->launch_center->tile, Money{count * 10});
					center->goods[cargo].GetOrCreateData().cargo.Append(cp, StationID::Invalid());
				}
				for (uint16_t model : {14, 15, 19, 20}) {
					REQUIRE(Command<Commands::AssembleLaunchCraft>::Post(center_ref.station, center_ref.generation, model));
					CHECK(CmdAssembleLaunchPod({}, center_ref.station, center_ref.generation).Failed()); advance(148);
				}
				auto *rocket = find(14); auto *miner = find(15); auto *survey = find(19); auto *builder = find(20); REQUIRE((rocket && miner && survey && builder));
				for (uint i = 0; i < 8; ++i) {
					const auto money = company->money; const auto digest = ComputeSpacePodDigest(); const auto seeds = _random; const auto generation = GetNextSpacePodGeneration();
					const auto quote = CmdAssembleLaunchPod({}, center_ref.station, center_ref.generation); REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == 2000);
					CHECK(company->money == money); CHECK(ComputeSpacePodDigest() == digest); CHECK(GetNextSpacePodGeneration() == generation); CHECK(_random.state[0] == seeds.state[0]); CHECK(_random.state[1] == seeds.state[1]);
					REQUIRE(Command<Commands::AssembleLaunchPod>::Post(center_ref.station, center_ref.generation)); CHECK(company->money == money - 2000);
					auto *pod = ResolveSpacePod(center->launch_operations->pod_workshop); REQUIRE(pod != nullptr);
					CHECK(CmdAssembleLaunchCraft({}, center_ref.station, center_ref.generation, 15).Failed()); CHECK(CmdRemoveLaunchCenter({}, center->launch_center->tile).Failed());
					if (i == 0) {
						REQUIRE(Command<Commands::PauseLaunchPod>::Post(pod->index, pod->generation, true)); advance(3); CHECK(pod->state.remaining_ticks == 74);
						REQUIRE(Command<Commands::PauseLaunchPod>::Post(pod->index, pod->generation, false));
					}
					advance(74); CHECK(pod->state.custody == SpacePodCustody::CenterStored);
				}
				CHECK(CountLaunchPods(center_ref) == 8); CHECK(CmdAssembleLaunchPod({}, center_ref.station, center_ref.generation).Failed());
				uint packed = 0;
				for (auto *pod : SpacePod::Iterate()) if (packed < 2) {
					REQUIRE(Command<Commands::MoveLaunchPod>::Post(center_ref.station, center_ref.generation, miner->index, miner->generation, pod->index, pod->generation, false));
					CHECK(CmdMoveLaunchPod({}, center_ref.station, center_ref.generation, miner->index, miner->generation, pod->index, pod->generation, false).Failed()); ++packed;
				}
				for (uint i = 0; i < 2; ++i) { REQUIRE(Command<Commands::AssembleLaunchPod>::Post(center_ref.station, center_ref.generation)); advance(74); }
				CHECK(SpacePod::GetNumItems() == 10); CHECK(CountLaunchPods(center_ref) == 8);
				CHECK(CmdMoveLaunchPod({}, center_ref.station, center_ref.generation, miner->index, miner->generation, miner->manifest->pods.front().pod, miner->manifest->pods.front().generation, true).Failed());
				for (uint8_t index : {1, 2}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center_ref.station, center_ref.generation, miner->index, miner->generation, index, uint16_t{1}, false));
				CHECK(GetSpacePayloadMass(*miner) == 6 * 16); // Own shells/service only; dry body is counted by its carrier.
				REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				ShowLaunchAssemblyWindow(center_ref); auto *workshop = FindWindowById(WindowClass::LaunchAssembly, center_ref.station.base()); REQUIRE(workshop != nullptr);
				CaptureNativeArtWindow(*workshop, paths.images / "workshop-pods.ppm"); const auto panel = workshop->GetWidget<NWidgetBase>(0)->GetCurrentRect(); const int row_height = GetCharacterHeight(FontSize::Normal) + 3;
				workshop->OnClick({panel.left + 8, panel.top + 8 + 6 * row_height}, 0, 1); CaptureNativeArtWindow(*workshop, paths.images / "workshop-rocket-selected.ppm");
				workshop->OnClick({}, 11, 1); // Actual UI posts the same synchronized generation-checked fuel transfer.
				CHECK(rocket->manifest->supplies[0].TotalCount() == 15); CHECK(rocket->manifest->supplies[0].Packets()->begin()->second.front()->GetFeederShare() == 150);
				const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}); CHECK(GetSpaceFreightCapacity(20, goods) == 40);
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(center_ref.station, center_ref.generation, builder->index, builder->generation, goods, uint16_t{40}, false));
				CHECK(builder->cargo.TotalCount() == 40); CHECK(builder->cargo_cap == 40); CHECK(GetSpacePayloadMass(*builder) == 20 * 16);
				CHECK(CmdMoveLaunchFreight({}, center_ref.station, center_ref.generation, builder->index, builder->generation, goods, 1, false).Failed());
				for (auto *probe : {miner, survey, builder}) {
					const auto money = company->money; const auto ref = GetSpaceCraftReference(*probe);
					REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, probe->index, probe->generation, false));
					CHECK(probe->construction.placement == SpacePlacement::Carried); CHECK(rocket->manifest->passenger == ref); CHECK(company->money == money); REQUIRE(ValidateSpaceVehicles());
					CHECK(CmdCarryLaunchProbe({}, rocket->index, rocket->generation, probe->index, probe->generation, false).Failed());
					REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, probe->index, probe->generation, true));
					CHECK(GetSpaceCraftReference(*probe) == ref); CHECK(company->money == money);
				}
				REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, builder->index, builder->generation, false));
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, uint8_t{1}, uint16_t{2}, false)); CHECK(GetSpacePayloadMass(*rocket) == 30 * 16);
				CaptureNativeArtWindow(*workshop, paths.images / "workshop-payload-limit.ppm");
				CHECK(CmdMoveSpaceSupply({}, center_ref.station, center_ref.generation, rocket->index, rocket->generation, 2, 1, false).Failed());
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, uint8_t{1}, uint16_t{2}, true));
				REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, builder->index, builder->generation, true));
				save("logistics-bootstrap-414.sav"); // Same paid mining body, two shells and the unconsumed service preload, ready for its first real launch.
				workshop->OnClick({panel.left + 8, panel.top + 8 + 10 * row_height}, 0, 1); CaptureNativeArtWindow(*workshop, paths.images / "workshop-probe-service.ppm");
				workshop->OnClick({}, 10, 1); advance(37);
				CaptureNativeArtWindow(*workshop, paths.images / "workshop-probe-service-active.ppm");
				CHECK(miner->manifest->service_ticks == 111); CHECK(miner->manifest->supplies[1].TotalCount() == 1); CHECK(miner->manifest->supplies[2].TotalCount() == 1);
				CHECK(CmdAssembleLaunchPod({}, center_ref.station, center_ref.generation).Failed()); CHECK(CmdMoveSpaceSupply({}, center_ref.station, center_ref.generation, miner->index, miner->generation, 1, 1, true).Failed());
				REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(miner->index, miner->generation, true)); CHECK(miner->manifest->supplies[1].TotalCount() == 1); CHECK(miner->manifest->services_completed == 0);
				advance(9); CHECK(miner->manifest->service_ticks == 111); CHECK(miner->manifest->service_paused); REQUIRE(ValidateSpaceVehicles());
				REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(miner->index, miner->generation, false)); CHECK(miner->manifest->service_ticks == 111);
				const auto old_home = center_ref; const auto anchor = center->launch_center->tile; const auto rotation = center->launch_center->rotation;
				ForgetLaunchCenter(*center); CHECK(miner->manifest->service_ticks == 111); CHECK(miner->manifest->service_paused); CHECK(CountLaunchPods(old_home) == 0);
				InstallNewLaunchCenter(*center, anchor, rotation); center_ref = GetLaunchCenterReference(*center); CHECK(center_ref != old_home);
				for (auto *v : SpaceVehicle::Iterate()) REQUIRE(Command<Commands::RecoverLaunchCraft>::Post(center_ref.station, center_ref.generation, v->index, v->generation));
				for (auto *pod : SpacePod::Iterate()) if (pod->state.custody == SpacePodCustody::Unhomed) {
					CHECK(CmdRecoverLaunchPod({}, old_home.station, old_home.generation, pod->index, pod->generation).Failed());
					REQUIRE(Command<Commands::RecoverLaunchPod>::Post(center_ref.station, center_ref.generation, pod->index, pod->generation));
				}
				save("logistics-partial-414.sav");
			}
			if (const char *corrupt = std::getenv("OTTD_LAUNCH_LOGISTICS_CORRUPT")) {
				const auto fleet = ComputeSpaceVehicleDigest(), pods = ComputeSpacePodDigest();
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeSpacePodDigest() == pods); fmt::print("LAUNCH_LOGISTICS preview_reject=true authority_unchanged=true\n");
			}
			auto *miner = find(15); REQUIRE(miner != nullptr); REQUIRE(miner->manifest); REQUIRE(miner->manifest->service_ticks == 111);
			advance(111); CHECK(miner->manifest->services_completed == 1); CHECK(miner->manifest->returns_remaining == 2); CHECK(miner->manifest->supplies[1].TotalCount() == 0); CHECK(miner->manifest->supplies[2].TotalCount() == 0);
			CHECK(miner->manifest->pods.size() == 2); CHECK(SpacePod::GetNumItems() == 10); advance(500); save("logistics-complete-414.sav");
			fmt::print("LAUNCH_LOGISTICS native=true paid_pods=true freight_mass=true service_once=true digest={} pods={}\n", ComputeSpaceVehicleDigest(), ComputeSpacePodDigest());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(logistics_observer, &observe); IConsole::CmdRegister("launch_logistics_test", ObserveLogistics);
	const std::string source_path = source.string(), config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
