/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_survey.cpp Native company disclosure, moving scans and saved partial-work fixtures. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_vehicle_cmd.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../economy_func.h"
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

TEST_CASE("Sparse survey receipts refuse duplicate bodies unknown companies and invalid field bounds", "[space-survey-contract]")
{
	AsteroidKnowledgeRecord r{OrbitalBodyID{1}, uint64_t{1}};
	CHECK(ValidateAsteroidKnowledgeRecords(std::array{r}, 10));
	CHECK_FALSE(ValidateAsteroidKnowledgeRecords(std::array{r, r}, 10));
	CHECK_FALSE(ValidateAsteroidKnowledgeRecords(std::array{r}, 1));
	CHECK_FALSE(ValidateAsteroidKnowledgeRecords(std::array{r}, UINT32_MAX));
	auto bad = r; bad.surveyed_companies = 0; CHECK_FALSE(ValidateAsteroidKnowledgeRecords(std::array{bad}, 10));
	bad = r; bad.surveyed_companies = uint64_t{1} << MAX_COMPANIES; CHECK_FALSE(ValidateAsteroidKnowledgeRecords(std::array{bad}, 10));
	bad = r; bad.body = OrbitalBodyID{0}; CHECK_FALSE(ValidateAsteroidKnowledgeRecords(std::array{r, bad}, 10));
	CHECK(ValidateAsteroidKnowledgeRecords(std::array{bad, r}, 10));
	LoadCheckData preview; preview.asteroid_knowledge_present = true; preview.asteroid_knowledge_rows.push_back(r); preview.asteroid_body_count = 10;
	preview.Clear(); CHECK_FALSE(preview.asteroid_knowledge_present); CHECK(preview.asteroid_knowledge_rows.empty()); CHECK(preview.asteroid_body_count == 0);
}
TEST_CASE("Only an active survey order may own bounded partial scan work", "[space-survey-contract]")
{
	SpaceOrderRecord r{VehicleID{2}, 7, {{SpaceOrder{SpaceOrderAction::Survey, {SpaceOrderTargetKind::Body, 0, 0}}}}};
	r.queue.started = true; r.queue.timer_ticks = 1000; r.queue.work_ticks = 37;
	CHECK(ValidateSpaceOrderRecords(std::array{r}, 8));
	auto bad = r; bad.queue.work_ticks = 148; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	bad = r; bad.queue.entries[0].action = SpaceOrderAction::Mine; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	bad = r; bad.queue.started = false; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	bad = r; bad.queue.cursor = 1; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	bad = r; bad.queue.timer_ticks = 0; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	bad.queue.blocked = SpaceOrderBlock::Timeout; CHECK(ValidateSpaceOrderRecords(std::array{bad}, 8));
}

static std::function<void()> *survey_observer = nullptr;
static bool ObserveSurvey(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(survey_observer != nullptr); (*survey_observer)(); return true; }
TEST_CASE("Native paid survey follows an actively mined body and discloses only after completed work", "[.space-survey]")
{
	const char *action_env = std::getenv("OTTD_SPACE_SURVEY_ACTION"), *input = std::getenv("OTTD_SPACE_SURVEY_SOURCE"); REQUIRE(action_env != nullptr); REQUIRE(input != nullptr);
	const std::string_view action{action_env}; const std::filesystem::path source{input};
	const auto paths = GetTestCaseArtifactPaths("space", "survey"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto find = [&](uint16_t model) -> SpaceVehicle * { for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == model) return v; return nullptr; };
			auto *rocket = find(14), *miner = find(15), *survey = find(19); REQUIRE((rocket && miner && survey));
			const auto survey_ref = GetSpaceCraftReference(*survey); const OrbitalBodyID body{0}; const auto owner = survey->owner;
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
				for (uint i = 0; i < ticks;) { REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks;
					StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++i;
				}
			};
			auto save = [&](std::string_view name) { REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok); };
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			auto capture = [&](std::string_view name) { ShowSpaceSurveyWindow(survey_ref); auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window != nullptr); CaptureNativeArtWindow(*window, paths.images / std::string{name}); };
			if (action == "prepare") {
				CHECK_FALSE(GetAsteroidQuantityDisclosure(owner, body)); // Old ownerless previews have run, but grant no company knowledge.
				const auto center = rocket->construction.home;
				REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, survey->index, survey->generation, false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Body, body.base(), uint64_t{0})); advance(667);
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center.station, center.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
				REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, miner->index, miner->generation, false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Body, body.base(), uint64_t{0})); advance(667);
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, body.base(), uint64_t{0}));
				uint travel = 0; while (miner->mission.phase != SpaceMissionPhase::Mining) { REQUIRE(++travel < 3000); advance(1); }
				REQUIRE(Company::CanAllocateItem()); auto *other = Company::Create(); SetCompanyManagerFaceStyle(other->face, 0); other->money = 10000000; // Explicit fixture company; a valid face avoids load-time presentation randomization.
				capture("survey-unknown.ppm");
				uint starts = 0;
				auto start = [&] {
					ShowSpaceSurveyWindow(survey_ref); auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window != nullptr);
					CaptureNativeArtWindow(*window, paths.images / (starts++ == 0 ? "survey-start.ppm" : "survey-restart.ppm"));
					window->OnClick({}, 3, 1); // Actual role-aware UI posts the synchronized typed Survey order.
					REQUIRE(survey->space_orders.entries.size() == 1); CHECK(survey->space_orders.entries[0].action == SpaceOrderAction::Survey);
					uint approach = 0; while (survey->space_orders.work_ticks == 0) { REQUIRE(++approach < 3000); advance(1); }
					advance(36); REQUIRE(survey->space_orders.work_ticks == 37);
				};
				start(); CHECK_FALSE(GetAsteroidQuantityDisclosure(owner, body));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(survey->index, survey->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				CHECK(survey->space_orders.work_ticks == 0); CHECK_FALSE(GetAsteroidQuantityDisclosure(owner, body)); advance(3);
				start(); capture("survey-partial.ppm"); save("survey-partial-414.sav");
			}
			if (const char *corrupt = std::getenv("OTTD_SPACE_SURVEY_CORRUPT")) {
				const auto digest = ComputeSpaceVehicleDigest(), field = ComputeAsteroidFieldDigest(GetAsteroidField());
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(ComputeSpaceVehicleDigest() == digest); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field); fmt::print("SPACE_SURVEY preview_reject=true authority_unchanged=true\n");
			}
			if (action == "known") {
				REQUIRE(GetAsteroidQuantityDisclosure(owner, body)); CHECK(survey->space_orders.cursor == survey->space_orders.entries.size());
				save("survey-known-reloaded-414.sav"); fmt::print("SPACE_SURVEY native=true known_reload=true\n"); observed = true; _exit_game = true; return;
			}
			REQUIRE(survey->space_orders.work_ticks == 37); REQUIRE(GetMutableAsteroidBody(body)->mining_slot == miner->index); CHECK_FALSE(GetAsteroidQuantityDisclosure(owner, body));
			const auto profit = survey->profit_this_year; const uint16_t cost_remainder = survey->mission.cost_remainder;
			advance(111); CHECK(survey->space_orders.cursor == 1); CHECK(survey->space_orders.work_ticks == 0);
			CHECK(survey->profit_this_year == profit - Money{(cost_remainder + 111 * 500) / 74} * 256); CHECK(survey->mission.cost_remainder == (cost_remainder + 111 * 500) % 74);
			const auto quantity = GetAsteroidQuantityDisclosure(owner, body); REQUIRE(quantity); CHECK(quantity->remaining == GetMutableAsteroidBody(body)->remaining_reserve); CHECK(quantity->initial == GetMutableAsteroidBody(body)->initial_reserve);
			CHECK(GetMutableAsteroidBody(body)->mining_slot == miner->index); CHECK(miner->cargo.TotalCount() > 0);
			CHECK(GetMutableAsteroidBody(body)->initial_reserve - GetMutableAsteroidBody(body)->remaining_reserve == miner->cargo.TotalCount());
			CHECK_FALSE(GetAsteroidQuantityDisclosure(COMPANY_SPECTATOR, body));
			CompanyID other_id = CompanyID::Invalid(); for (const auto *c : Company::Iterate()) if (c->index != owner) { REQUIRE(other_id == CompanyID::Invalid()); other_id = c->index; } REQUIRE(other_id != CompanyID::Invalid());
			CHECK_FALSE(GetAsteroidQuantityDisclosure(other_id, body)); capture("survey-known.ppm");
			{ AutoRestoreBackup local_actor(_current_company, _local_company); SetLocalCompany(other_id); capture("survey-foreign-unknown.ppm"); SetLocalCompany(owner); }
			save("survey-complete-414.sav");
			// Actual takeover and Company pool reuse: the acquired company retains knowledge; the reused ID does not.
			ChangeOwnershipOfCompanyItems(owner, other_id); CHECK(GetAsteroidQuantityDisclosure(other_id, body)); CHECK_FALSE(GetAsteroidQuantityDisclosure(owner, body));
			{ AutoRestoreBackup local_actor(_current_company, _local_company); delete Company::Get(owner); REQUIRE(Company::CanAllocateItem()); auto *reused = Company::Create(); CHECK(reused->index == owner); CHECK_FALSE(GetAsteroidQuantityDisclosure(reused->index, body)); SetLocalCompany(owner); }
			REQUIRE(ValidateSpaceVehicles()); fmt::print("SPACE_SURVEY native=true paid_launch=true working_ticks=148 company_private=true cancel_no_knowledge=true mining_lease_independent=true company_reuse=true digest={}\n", ComputeAsteroidFieldDigest(GetAsteroidField()));
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(survey_observer, &observe); IConsole::CmdRegister("space_survey_test", ObserveSurvey);
	const std::string source_path = source.string(), config_path = config.string(); std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
