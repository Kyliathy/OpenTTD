/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_orders.cpp Native typed order role/refusal/save execution; Plan8 Group6. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../asteroid.h"
#include "../cargotype.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

TEST_CASE("Typed Space targets enforce role generation and bounded transport", "[space-order-contract]")
{
	const SpaceOrder mine{SpaceOrderAction::Mine, {SpaceOrderTargetKind::Body, 90000, 0}};
	CHECK(IsSpaceOrderAllowed(15, mine)); CHECK_FALSE(IsSpaceOrderAllowed(19, mine)); CHECK_FALSE(IsSpaceOrderAllowed(20, mine));
	auto survey = mine; survey.action = SpaceOrderAction::Survey;
	CHECK(IsSpaceOrderAllowed(19, survey)); CHECK_FALSE(IsSpaceOrderAllowed(15, survey));
	const SpaceOrder build{SpaceOrderAction::Construct, {SpaceOrderTargetKind::Blueprint, 1, 7}};
	CHECK(IsSpaceOrderAllowed(20, build)); CHECK_FALSE(IsSpaceOrderAllowed(15, build)); CHECK_FALSE(IsSpaceOrderAllowed(19, build));
	auto stale = build; stale.target.generation = 0; CHECK_FALSE(IsSpaceOrderAllowed(20, stale));
	SpaceOrderRecord record{VehicleID{2}, 7, {{mine}}};
	CHECK(ValidateSpaceOrderRecords(std::array{record}, 8));
	CHECK_FALSE(ValidateSpaceOrderRecords(std::array{record}, 7));
	CHECK_FALSE(ValidateSpaceOrderRecords(std::array{record, record}, 8));
	for (auto invalid : {SpaceOrderAction::End, static_cast<SpaceOrderAction>(255)}) { auto bad = record; bad.queue.entries[0].action = invalid; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8)); }
	for (auto invalid : {SpaceOrderTargetKind::End, static_cast<SpaceOrderTargetKind>(255)}) { auto bad = record; bad.queue.entries[0].target.kind = invalid; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8)); }
	auto bad = record; bad.queue.cursor = 2; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	bad = record; bad.queue.entries.resize(33, mine); CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	bad = record; bad.queue.entries[0].target.generation = 8; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	bad = record; bad.queue.entries[0].target.id = UINT32_MAX; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	bad = record; bad.queue.work_ticks = 1; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	bad = record; bad.queue.started = true; bad.queue.timer_ticks = UINT32_MAX; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
	bad = record; bad.queue.blocked = SpaceOrderBlock::Completed; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{bad}, 8));
}

static std::function<void()> *_order_observer = nullptr;
static bool ObserveOrders(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(_order_observer != nullptr); (*_order_observer)(); return true; }

TEST_CASE("Native Space visits and mines distinct typed targets through an interrupted save", "[.space-orders]")
{
	const char *action = std::getenv("OTTD_SPACE_ORDERS_ACTION"); REQUIRE(action != nullptr);
	const bool prepare = std::string_view{action} == "prepare";
	const char *input = std::getenv("OTTD_SPACE_ORDERS_SOURCE");
	const auto source = prepare ? FindTestRepositoryRoot() / "src/tests/fixtures/demos/space/space-mining-approach-1990-405.sav" : std::filesystem::path{input == nullptr ? "" : input};
	const auto paths = GetTestCaseArtifactPaths("space", "orders"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			REQUIRE(ValidateSpaceVehicles()); auto *probe = *SpaceVehicle::Iterate().begin();
			REQUIRE(probe->generation != 0);
			{
				const auto before = ComputeSpaceOrdersDigest();
				const auto records = CaptureSpaceOrders();
				const auto *airplane = *Engine::IterateType(VehicleType::Aircraft).begin();
				AutoRestoreBackup wrong_engine(probe->engine_type, airplane->index);
				CHECK_FALSE(ValidateNativeSpaceOrders());
				CHECK_FALSE(InstallLoadedSpaceOrders(records, GetNextSpaceVehicleGeneration()));
				CHECK(ComputeSpaceOrdersDigest() == before);
			}
			CHECK(CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"GOOD"}))->weight == 8);
			CHECK(CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"STEL"}))->weight == 16);
			auto edit = [&](DoCommandFlags flags, uint64_t generation, SpaceOrderAction verb, uint32_t body) { return CmdEditSpaceOrder(flags, probe->index, generation, SpaceOrderEdit::Insert, static_cast<uint8_t>(probe->space_orders.entries.size()), verb, SpaceOrderTargetKind::Body, body, 0); };
			if (prepare) {
				const auto before = ComputeSpaceVehicleDigest();
				CHECK(edit({}, probe->generation, SpaceOrderAction::Visit, 1).Succeeded());
				CHECK(edit(DoCommandFlag::Execute, probe->generation + 999, SpaceOrderAction::Visit, 1).Failed());
				CHECK(edit(DoCommandFlag::Execute, probe->generation, SpaceOrderAction::Survey, 1).Failed());
				CHECK(edit(DoCommandFlag::Execute, probe->generation, SpaceOrderAction::Visit, 90000).Failed());
				{ AutoRestoreBackup foreign(_current_company, OWNER_NONE); CHECK(edit(DoCommandFlag::Execute, probe->generation, SpaceOrderAction::Visit, 1).Failed()); }
				CHECK(ComputeSpaceVehicleDigest() == before);
				REQUIRE(edit(DoCommandFlag::Execute, probe->generation, SpaceOrderAction::Visit, 1).Succeeded());
				REQUIRE(edit(DoCommandFlag::Execute, probe->generation, SpaceOrderAction::Mine, 0).Succeeded());
				const auto original_queue = probe->space_orders; const auto digest = ComputeSpaceVehicleDigest();
				CHECK(CmdEditSpaceOrder({}, probe->index, probe->generation, SpaceOrderEdit::MoveUp, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Failed());
				CHECK(CmdEditSpaceOrder({}, probe->index, probe->generation, SpaceOrderEdit::MoveDown, 1, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Failed());
				CHECK(CmdEditSpaceOrder({}, probe->index, probe->generation, SpaceOrderEdit::MoveDown, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Succeeded()); CHECK(ComputeSpaceVehicleDigest() == digest);
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(probe->index, probe->generation, SpaceOrderEdit::MoveDown, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0})); CHECK(probe->space_orders.entries[0].action == SpaceOrderAction::Mine);
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(probe->index, probe->generation, SpaceOrderEdit::MoveUp, uint8_t{1}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0})); CHECK(probe->space_orders == original_queue);
			}
			auto save = [&](std::string_view name) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{});
				const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
				for (uint i = 0; i < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline);
					const auto before = GetAsteroidField().elapsed_ticks;
					const auto queue = probe->space_orders;
					const auto profit = probe->profit_this_year;
					StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) {
						// CargoDist workers may pause the real loop; count synchronized ticks, not wall-clock calls.
						CHECK(probe->space_orders == queue); CHECK(probe->profit_this_year == profit);
						std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue;
					}
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++i;
				}
			};
			if (prepare) {
				advance(7);
				REQUIRE(probe->space_orders.started); REQUIRE(probe->space_orders.cursor == 0); save("orders-partial-414.sav");
			}
			if (const char *corrupt = std::getenv("OTTD_SPACE_ORDERS_CORRUPT")) {
				const auto live = ComputeSpaceVehicleDigest(), field = ComputeAsteroidFieldDigest(GetAsteroidField());
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(ComputeSpaceVehicleDigest() == live); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field);
				fmt::print("SPACE_ORDERS preview_reject=true authority_unchanged=true\n");
			}
			const auto held = probe->cargo.TotalCount(); const auto reserve = GetAsteroidField().bodies[0].remaining_reserve;
			advance(2200);
			if (std::string_view{action} == "blocked") {
				CHECK(probe->space_orders.cursor == 0); CHECK(probe->space_orders.blocked == SpaceOrderBlock::TargetRemoved);
				CHECK(probe->cargo.TotalCount() == held); CHECK(GetAsteroidField().bodies[0].remaining_reserve == reserve);
			} else {
				CHECK(probe->space_orders.cursor == 2); CHECK(probe->space_orders.blocked == SpaceOrderBlock::Completed);
				CHECK(probe->cargo.TotalCount() == 20); CHECK(GetAsteroidField().bodies[0].remaining_reserve + 20 - held == reserve);
			}
			REQUIRE(ValidateSpaceVehicles()); save("orders-complete-414.sav");
			fmt::print("SPACE_ORDERS native=true action={} conserved=true generation={} digest={}\n", action, probe->generation, ComputeSpaceVehicleDigest());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_order_observer, &observe); IConsole::CmdRegister("space_orders_test", ObserveOrders);
	const std::string source_path = source.string(), config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
