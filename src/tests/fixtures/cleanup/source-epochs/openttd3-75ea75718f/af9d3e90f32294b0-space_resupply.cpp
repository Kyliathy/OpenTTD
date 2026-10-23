/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_resupply.cpp Paid same-probe repeat, native resupply and interrupted service. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../space_pod.h"
#include "../space_vehicle_cmd.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../asteroid.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../station_base.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

TEST_CASE("Resupply uses a separate receipt and generation-checked craft target", "[space-resupply-contract]")
{
	SpaceTransitionRecord r; r.craft = {VehicleID{2}, 7}; auto &f = r.state;
	f.phase = SpaceFlightPhase::OrbitalDelivery; f.layer = MapLayer::Space; f.source = f.recovery = {StationID{1}, 5};
	f.destination = {SpaceOrderTargetKind::Craft, 3, 8}; f.outbound_burned = true; f.missions_started = 1;
	CHECK(ValidateSpaceTransitionRecords(std::array{r}));
	for (auto block : {SpaceFlightBlock::ReceiverBusy, SpaceFlightBlock::ReceiverFull, SpaceFlightBlock::Approach}) { f.blocked = block; CHECK(ValidateSpaceTransitionRecords(std::array{r})); }
	f.blocked = SpaceFlightBlock::None; f.resupplied = true; CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{r}));
	f.phase = SpaceFlightPhase::ReturnRecovery; f.recovery_burned = true; f.remaining_ticks = 222; CHECK(ValidateSpaceTransitionRecords(std::array{r}));
	f.deployed = true; CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{r})); f.deployed = false;
	f.destination.generation = 0; CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{r})); f.destination.generation = 8;
	f.destination.id = 2; CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{r})); f.destination = {SpaceOrderTargetKind::Body, 0, 0};
	CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{r}));
	SpaceOrderRecord q{VehicleID{2}, 7, {{SpaceOrder{SpaceOrderAction::Visit, {SpaceOrderTargetKind::Craft, 3, 8}}}}};
	CHECK(ValidateSpaceOrderRecords(std::array{q}, 9)); CHECK_FALSE(ValidateSpaceOrderRecords(std::array{q}, 8));
	CHECK(IsSpaceOrderAllowed(19, q.queue.entries[0])); q.queue.entries[0].action = SpaceOrderAction::Survey; CHECK_FALSE(IsSpaceOrderAllowed(19, q.queue.entries[0]));
}
TEST_CASE("Only an unstarted Mine may wait for paid service or pods", "[space-resupply-contract]")
{
	SpaceOrderRecord r{VehicleID{2}, 7, {{SpaceOrder{SpaceOrderAction::Mine, {SpaceOrderTargetKind::Body, 0, 0}}}}};
	for (auto block : {SpaceOrderBlock::ReturnsPending, SpaceOrderBlock::ServiceRequired, SpaceOrderBlock::PodsMissing}) {
		r.queue.blocked = block; CHECK(IsSpaceMiningServiceWait(r.queue)); CHECK(ValidateSpaceOrderRecords(std::array{r}, 8));
		auto bad = r; bad.queue.started = true; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
		bad = r; bad.queue.entries[0].action = SpaceOrderAction::Visit; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	}
	LoadCheckData preview; preview.next_space_vehicle_generation = 10; preview.native_space_vehicle_rows.push_back({VehicleID{2}, VehicleType::Space, Owner{0}, {}});
	preview.Clear(); CHECK(preview.next_space_vehicle_generation == 1); CHECK(preview.native_space_vehicle_rows.empty());
}

static std::function<void()> *resupply_observer = nullptr;
static bool ObserveResupply(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(resupply_observer != nullptr); (*resupply_observer)(); return true; }
TEST_CASE("Three mined collections use one paid probe and a cargo-only second rocket flight", "[.space-resupply]")
{
	const char *mode = std::getenv("OTTD_SPACE_RESUPPLY_ACTION"), *input = std::getenv("OTTD_SPACE_RESUPPLY_SOURCE"); REQUIRE(mode != nullptr); REQUIRE(input != nullptr);
	const std::string_view action{mode}; const std::filesystem::path source{input}; const auto paths = GetTestCaseArtifactPaths("space", "resupply"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto find = [&](uint16_t model) -> SpaceVehicle * { for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == model) return v; return nullptr; };
			auto *rocket = find(14), *miner = find(15); REQUIRE((rocket && miner)); REQUIRE((rocket->manifest && miner->manifest));
			const auto rocket_ref = GetSpaceCraftReference(*rocket), miner_ref = GetSpaceCraftReference(*miner); const auto center_ref = rocket->construction.home;
			auto *center = ResolveLaunchCenter(center_ref); REQUIRE(center != nullptr); REQUIRE(GetSpaceVehicleCount() == 4);
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
				for (uint i = 0; i < ticks;) { REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks;
					StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++i;
				}
			};
			auto save = [&](std::string_view name) { REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok); };
			auto edit = [&](SpaceOrderEdit operation, SpaceOrderAction verb, SpaceOrderTarget target) { REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, operation,
				operation == SpaceOrderEdit::Insert ? static_cast<uint8_t>(miner->space_orders.entries.size()) : uint8_t{0}, verb, target.kind, target.id, target.generation)); };
			auto finish_orders = [&] { uint ticks = 0; while (miner->space_orders.cursor < miner->space_orders.entries.size()) { REQUIRE(++ticks < 20000); advance(1); } };
			auto release = [&] { REQUIRE(miner->cargo.TotalCount() == 20); REQUIRE_FALSE(miner->manifest->pods.empty()); const auto pod = miner->manifest->pods.front();
				REQUIRE(Command<Commands::ReleaseOrbitalCargo>::Post(miner->index, miner->generation, center_ref.station, center_ref.generation, pod.pod, pod.generation, uint16_t{20})); advance(148); };
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			auto capture = [&](std::string_view name) { ShowSpaceSurveyWindow(miner_ref); auto *w = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(w != nullptr); CaptureNativeArtWindow(*w, paths.images / std::string{name}); };
			if (action == "freight-prepare" || action == "freight-continue") {
				auto *builder = find(20); REQUIRE(builder != nullptr); const auto ref = GetSpaceCraftReference(*builder); const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
				if (action == "freight-prepare") {
					REQUIRE(Command<Commands::MoveLaunchFreight>::Post(center_ref.station, center_ref.generation, builder->index, builder->generation, goods, uint16_t{40}, true));
					REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, builder->index, builder->generation, false));
					REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::LaunchCenter, uint32_t{center_ref.station.base()}, center_ref.generation)); advance(667);
					REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
					REQUIRE(Command<Commands::MoveLaunchFreight>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, goods, uint16_t{41}, false));
					const auto digest = ComputeSpaceVehicleDigest(); CHECK(CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, builder->index.base(), builder->generation).Failed()); CHECK(ComputeSpaceVehicleDigest() == digest);
					REQUIRE(Command<Commands::MoveLaunchFreight>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, goods, uint16_t{1}, true));
					for (uint8_t i : {1, 2}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, i, uint16_t{1}, false));
					CHECK(GetSpacePayloadMass(*rocket) == 22 * 16);
					REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, builder->index.base(), builder->generation)); advance(185); save("resupply-freight-ascent-414.sav");
				}
				REQUIRE(rocket->transition->remaining_ticks == 185); advance(186);
				CHECK(rocket->transition->resupplied); CHECK_FALSE(rocket->transition->deployed); CHECK(rocket->cargo.TotalCount() == 0); CHECK(builder->cargo.TotalCount() == 40); CHECK(builder->cargo_type == goods); CHECK(builder->cargo_cap == 40);
				CHECK(builder->manifest->supplies[1].TotalCount() == 1); CHECK(builder->manifest->supplies[2].TotalCount() == 1); CHECK(GetSpaceCraftReference(*builder) == ref);
				advance(296); CHECK(rocket->transition->missions_completed == 2); CHECK(rocket->manifest->fuel_burned == 30); CHECK(GetSpaceVehicleCount() == 4);
				save("resupply-freight-complete-414.sav"); fmt::print("SPACE_RESUPPLY native=true freight=true goods_crates=40 same_builder=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "prepare") {
				for (uint8_t i : {1, 2}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center_ref.station, center_ref.generation, miner->index, miner->generation, i, uint16_t{1}, true));
				REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, miner->index, miner->generation, false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::LaunchCenter, uint32_t{center_ref.station.base()}, center_ref.generation)); advance(667);
				const auto &field = GetAsteroidField(); REQUIRE_FALSE(field.bodies.empty());
				const auto body = std::min_element(field.bodies.begin(), field.bodies.end(), [&](const auto &a, const auto &b) { return SpaceDistanceSquared(a.motion.position, miner->position) < SpaceDistanceSquared(b.motion.position, miner->position); })->id;
				for (uint batch = 0; batch < 2; ++batch) {
					edit(SpaceOrderEdit::Clear, SpaceOrderAction::Visit, {}); edit(SpaceOrderEdit::Insert, SpaceOrderAction::Mine, {SpaceOrderTargetKind::Body, body.base(), 0});
					edit(SpaceOrderEdit::Insert, SpaceOrderAction::Visit, {SpaceOrderTargetKind::LaunchCenter, center_ref.station.base(), center_ref.generation}); finish_orders(); release();
				}
				CHECK(miner->manifest->returns_remaining == 0); CHECK(miner->manifest->pods.empty()); CHECK(miner->extracted_units == 40);
				edit(SpaceOrderEdit::Clear, SpaceOrderAction::Visit, {}); edit(SpaceOrderEdit::Insert, SpaceOrderAction::Mine, {SpaceOrderTargetKind::Body, body.base(), 0}); advance(1);
				REQUIRE(miner->space_orders.blocked == SpaceOrderBlock::ServiceRequired); const auto profit = miner->profit_this_year; const auto remainder = miner->mission.cost_remainder;
				advance(222); CHECK(miner->extracted_units == 40); CHECK(miner->cargo.TotalCount() == 0); CHECK(miner->profit_this_year == profit - Money{300} * 256); CHECK(miner->mission.cost_remainder == remainder);
				CHECK(CmdServiceLaunchProbe({}, miner->index, miner->generation, false).Failed()); CHECK(CmdSetSpaceMission({}, miner->index, body.base()).Failed()); capture("resupply-exhausted.ppm");
				for (uint8_t i : {1, 2}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, i, uint16_t{1}, false));
				for (uint i = 0; i < 3; ++i) {
					SpacePod *pod = nullptr; for (auto *p : SpacePod::Iterate()) if (p->state.custody == SpacePodCustody::CenterStored && p->state.home == center_ref) { pod = p; break; } REQUIRE(pod != nullptr);
					REQUIRE(Command<Commands::MoveLaunchPod>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, pod->index, pod->generation, false));
				}
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
				const auto digest = ComputeSpaceVehicleDigest(); CHECK(CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, miner->index.base(), miner->generation).Failed()); CHECK(ComputeSpaceVehicleDigest() == digest);
				const auto extra = rocket->manifest->pods.back(); REQUIRE(Command<Commands::MoveLaunchPod>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, extra.pod, extra.generation, true));
				CHECK(GetSpaceResupplyBlock(*rocket, miner_ref) == SpaceFlightBlock::None);
				CHECK(CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, miner->index.base(), miner->generation + 100).Failed());
				{ AutoRestoreBackup foreign(_current_company, OWNER_NONE); CHECK(CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, miner->index.base(), miner->generation).Failed()); }
				ShowLaunchAssemblyWindow(center_ref); auto *workshop = FindWindowById(WindowClass::LaunchAssembly, center_ref.station.base()); REQUIRE(workshop != nullptr);
				CaptureNativeArtWindow(*workshop, paths.images / "resupply-workshop.ppm"); const auto panel = workshop->GetWidget<NWidgetBase>(0)->GetCurrentRect(); const int line = GetCharacterHeight(FontSize::Normal) + 3;
				workshop->OnClick({panel.left + 8, panel.top + 8 + 7 * line}, 0, 1); CaptureNativeArtWindow(*workshop, paths.images / "resupply-receiver-selected.ppm");
				workshop->OnClick({}, 28, 1); CaptureNativeArtWindow(*workshop, paths.images / "resupply-target-marked.ppm");
				workshop->OnClick({panel.left + 8, panel.top + 8 + 9 * line}, 0, 1); CaptureNativeArtWindow(*workshop, paths.images / "resupply-ready.ppm");
				const auto money = Company::Get(_current_company)->money; workshop->OnClick({}, 29, 1); REQUIRE(rocket->transition->phase == SpaceFlightPhase::Preparing);
				CHECK(Company::Get(_current_company)->money == money - Money{5000}); advance(185); save("resupply-ascent-414.sav");
			}
			if (const char *corrupt = std::getenv("OTTD_SPACE_RESUPPLY_CORRUPT")) {
				const auto digest = ComputeSpaceVehicleDigest(); REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok); CHECK(ComputeSpaceVehicleDigest() == digest);
				fmt::print("SPACE_RESUPPLY preview_reject=true authority_unchanged=true\n");
			}
			if (action == "blocked-prepare" || action == "blocked-resume") {
				if (action == "blocked-prepare") {
					const auto &field = GetAsteroidField(); const auto far = std::max_element(field.bodies.begin(), field.bodies.end(), [&](const auto &a, const auto &b) { return SpaceDistanceSquared(a.motion.position, miner->position) < SpaceDistanceSquared(b.motion.position, miner->position); })->id;
					edit(SpaceOrderEdit::Clear, SpaceOrderAction::Visit, {}); edit(SpaceOrderEdit::Insert, SpaceOrderAction::Visit, {SpaceOrderTargetKind::Body, far.base(), 0}); advance(186);
					REQUIRE(rocket->transition->blocked == SpaceFlightBlock::ReceiverBusy); save("resupply-blocked-414.sav");
				}
				CHECK(rocket->manifest->pods.size() == 2); CHECK(rocket->manifest->supplies[1].TotalCount() == 1); CHECK(miner->manifest->pods.empty()); CHECK(miner->manifest->returns_remaining == 0); CHECK_FALSE(rocket->transition->resupplied);
				edit(SpaceOrderEdit::Clear, SpaceOrderAction::Visit, {}); edit(SpaceOrderEdit::Insert, SpaceOrderAction::Mine, {SpaceOrderTargetKind::Body, GetAsteroidField().bodies.front().id.base(), 0});
				uint ticks = 0; while (!rocket->transition->resupplied) { REQUIRE(++ticks < 20000); advance(1); }
				CHECK(miner->manifest->pods.size() == 2); CHECK(miner->manifest->returns_remaining == 0); CHECK(miner->extracted_units == 40); advance(296);
				save("resupply-blocked-complete-414.sav"); fmt::print("SPACE_RESUPPLY native=true blocked_receiver=true retained_payload=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "lost-prepare" || action == "lost-resume") {
				if (action == "lost-prepare") {
					// Exercise the real native destructor and paid allocator, without copying pool state.
					REQUIRE(miner->cargo.TotalCount() == 0); REQUIRE(miner->manifest->pods.empty()); delete miner; miner = nullptr; REQUIRE(ValidateSpaceVehicles());
					const auto money = Company::Get(_current_company)->money; REQUIRE(Command<Commands::AssembleLaunchCraft>::Post(center_ref.station, center_ref.generation, uint16_t{15}));
					auto *replacement = ResolveSpaceCraft(center->launch_operations->workshop); REQUIRE(replacement != nullptr); CHECK(replacement->index == miner_ref.vehicle); CHECK(replacement->generation != miner_ref.generation);
					CHECK(Company::Get(_current_company)->money == money - Money{100000}); advance(186); REQUIRE(rocket->transition->blocked == SpaceFlightBlock::TargetLost); save("resupply-lost-414.sav");
				}
				CHECK(ResolveSpaceCraft({VehicleID{rocket->transition->destination.id}, rocket->transition->destination.generation}) == nullptr);
				CHECK(rocket->manifest->pods.size() == 2); CHECK_FALSE(rocket->transition->resupplied); advance(10); CHECK(rocket->manifest->pods.size() == 2);
				REQUIRE(Command<Commands::CancelLaunchFlight>::Post(rocket->index, rocket->generation)); advance(296);
				CHECK(rocket->manifest->pods.size() == 2); CHECK(rocket->manifest->supplies[1].TotalCount() == 1); CHECK(rocket->manifest->supplies[2].TotalCount() == 1);
				CHECK(rocket->manifest->fuel_burned == 30); CHECK(rocket->transition->missions_completed == 1); CHECK(rocket->transition->missions_canceled == 1);
				save("resupply-lost-complete-414.sav"); fmt::print("SPACE_RESUPPLY native=true lost_receiver=true incarnation_reuse=true retained_payload=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "prepare" || action == "continue") {
				REQUIRE(rocket->transition->phase == SpaceFlightPhase::Ascent); REQUIRE(rocket->transition->remaining_ticks == 185); advance(186);
				CHECK(rocket->transition->resupplied); CHECK_FALSE(rocket->transition->deployed); CHECK(rocket->manifest->pods.empty()); CHECK(miner->manifest->pods.size() == 2); CHECK(miner->manifest->returns_remaining == 0);
				CHECK(GetSpaceCraftReference(*miner) == miner_ref); CHECK(miner->construction.deployed); save("resupply-delivered-414.sav");
			}
			if (action != "service" && action != "paused-service") {
				capture("resupply-parts-arrived.ppm"); ShowSpaceSurveyWindow(miner_ref); auto *w = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(w != nullptr); w->OnClick({}, 10, 1);
				REQUIRE(miner->manifest->service_ticks == 148); const auto profit = miner->profit_this_year; advance(37); CHECK(miner->profit_this_year == profit - Money{250} * 256); save("resupply-service-414.sav");
			}
			REQUIRE(miner->manifest->service_ticks == 111);
			if (action != "paused-service") {
				REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(miner->index, miner->generation, true)); const auto profit = miner->profit_this_year;
				advance(74); CHECK(miner->manifest->service_ticks == 111); CHECK(miner->profit_this_year == profit - Money{100} * 256); capture("resupply-service-paused.ppm"); save("resupply-service-paused-414.sav");
			}
			REQUIRE(miner->manifest->service_paused); REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(miner->index, miner->generation, false));
			const auto profit = miner->profit_this_year; const auto remainder = miner->mission.cost_remainder;
			advance(111); CHECK(miner->profit_this_year == profit - Money{750} * 256); CHECK(miner->mission.cost_remainder == remainder);
			CHECK(miner->manifest->services_completed == 1); CHECK(miner->manifest->returns_remaining == 2); CHECK(miner->manifest->supplies[1].TotalCount() == 0); CHECK(miner->manifest->supplies[2].TotalCount() == 0);
			finish_orders(); REQUIRE(miner->cargo.TotalCount() == 20); edit(SpaceOrderEdit::Clear, SpaceOrderAction::Visit, {}); edit(SpaceOrderEdit::Insert, SpaceOrderAction::Visit, {SpaceOrderTargetKind::LaunchCenter, center_ref.station.base(), center_ref.generation}); finish_orders(); release();
			CHECK(miner->extracted_units == 60); CHECK(center->goods[miner->cargo_type].TotalCount() == 60); CHECK(miner->manifest->returns_remaining == 1); CHECK(miner->manifest->pending_returns == 0); CHECK(miner->manifest->pods.size() == 1);
			CHECK(rocket->transition->missions_completed == 2); CHECK(rocket->manifest->fuel_burned == 30); CHECK(rocket->construction.placement == SpacePlacement::SurfaceStored);
			CHECK(GetSpaceVehicleCount() == 4); CHECK(GetSpaceCraftReference(*rocket) == rocket_ref); CHECK(GetSpaceCraftReference(*miner) == miner_ref);
			uint64_t extracted = 0, owned = 0; for (const auto &body : GetAsteroidField().bodies) extracted += body.initial_reserve - body.remaining_reserve;
			for (const auto *p : CargoPacket::Iterate()) if (p->GetSource().type == SourceType::Asteroid) owned += p->Count(); CHECK(extracted == 60); CHECK(owned == 60);
			capture("resupply-third-return.ppm"); save("resupply-complete-414.sav"); fmt::print("SPACE_RESUPPLY native=true same_probe=true paid_third_return=true service_once=true conservation=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(resupply_observer, &observe); IConsole::CmdRegister("space_resupply_test", ObserveResupply);
	const std::string source_path = source.string(), config_path = config.string(); std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
