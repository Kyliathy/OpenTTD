/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_removal_native.cpp Genuine paid module removal, retained receipts and generation-qualified navigation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups5/5C.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_approach.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../orbital_service.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../asteroid.h"
#include "../map_func.h"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *removal_observer = nullptr;
/** @param args Native console invocation. @return Whether the installed actual-world observer executed. */
static bool ObserveRemoval(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(removal_observer != nullptr); (*removal_observer)(); return true;
}

/** A genuine paid predecessor supplies every craft, consumed receipt and packet; native commands alone remove/reuse an identity. */
TEST_CASE("Safe native removal retains spent module receipts and cannot retarget an old queue", "[.orbital-removal]")
{
	const char *mode = std::getenv("OTTD_ORBITAL_REMOVAL_ACTION"), *input = std::getenv("OTTD_ORBITAL_REMOVAL_SOURCE");
	REQUIRE(mode != nullptr); REQUIRE(input != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "orbital-removal"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Drive only the ordinary loaded owners, preserving failures across normal native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto &core = *SpaceStructure::Get(SpaceStructureID{0}); auto &storage = *SpaceStructure::Get(SpaceStructureID{1}); SpaceStructure *dock = nullptr;
			SpaceVehicle *builder = nullptr;
			// Reuse actual paid facilities and one real deployed builder; no substitute frame, material or hull position is created.
			for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Dock) dock = site;
			for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 20) { builder = craft; break; }
			REQUIRE(dock != nullptr); REQUIRE(builder != nullptr); REQUIRE(IsOrbitalModuleOperational(core)); REQUIRE(IsOrbitalModuleOperational(*dock));
			if (const char *corrupt = std::getenv("OTTD_ORBITAL_REMOVAL_CORRUPT")) {
				const auto structures = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest(), orders = ComputeSpaceOrdersDigest();
				const auto cash = Company::Get(_current_company)->money; const auto random = _random;
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == power); CHECK(ComputeSpaceOrdersDigest() == orders);
				CHECK(Company::Get(_current_company)->money == cash); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("ORBITAL_REMOVAL native=true preview_rejected=true authority_unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			REQUIRE(action == "remove"); REQUIRE(storage.state.phase == SpaceStructurePhase::FrameComplete); REQUIRE(storage.module == nullptr);
			REQUIRE(GetSpaceStructureRetirementHistory() == SpaceStructureRetirementHistory{});
			const auto old = GetSpaceStructureReference(storage); const auto geometry = storage.state;
			const auto consumed = storage.job.consumed; const auto source = core.module->producer;
			const auto before = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest(), orders = ComputeSpaceOrdersDigest();
			const auto cash = Company::Get(_current_company)->money; const auto random = _random;
			CHECK(ClassifyOrbitalModuleRemoval(core, _current_company) == OrbitalRemovalBlock::Inventory);
			CHECK(ClassifyOrbitalModuleRemoval(*dock, _current_company) == OrbitalRemovalBlock::Vehicle);
			CHECK(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, core.index, core.generation).Failed());
			CHECK(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, dock->index, dock->generation).Failed());
			CHECK(CmdRemoveOrbitalModule({}, old.structure, old.generation).Succeeded()); CHECK(CmdRemoveOrbitalModule({}, old.structure, old.generation + 1).Failed());
			CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeUtilityPowerDigest() == power); CHECK(ComputeSpaceOrdersDigest() == orders);
			CHECK(Company::Get(_current_company)->money == cash); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			// Queue the exact old target while the actual hull remains at Dock. Removal must stop this action without changing that physical pose.
			REQUIRE(Command<Commands::EditSpaceOrder>::Post(builder->index, builder->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
			REQUIRE(Command<Commands::EditSpaceOrder>::Post(builder->index, builder->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure, uint32_t{old.structure.base()}, old.generation));
			const auto position = builder->position; const auto queued = builder->space_orders.entries;
			const auto used = GetUtilityPowerProducer(source)->consumed_lifetime_j;
			REQUIRE(Command<Commands::RemoveOrbitalModule>::Post(old.structure, old.generation));
			CHECK(ResolveSpaceStructure(old) == nullptr); CHECK(GetSpaceStructureRetirementHistory().structures_removed == 1);
			CHECK((GetSpaceStructureRetirementHistory().consumed == std::array<uint64_t, 2>{consumed[0], consumed[1]}));
			CHECK(GetSpaceStructureRetirementHistory().services_completed == 0); CHECK(GetUtilityPowerProducer(source)->consumed_lifetime_j == used);
			CHECK(builder->position == position); CHECK(builder->space_orders.entries == queued); CHECK(builder->space_orders.blocked == SpaceOrderBlock::TargetRemoved); CHECK_FALSE(builder->space_orders.approach);
			CHECK(Company::Get(_current_company)->money == cash); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(TileXYL(geometry.x, geometry.y, MapLayer::Space), geometry.kind));
			const auto *replacement = SpaceStructure::Get(old.structure); REQUIRE(replacement != nullptr);
			CHECK(replacement->generation > old.generation); CHECK(replacement->module == nullptr); CHECK(replacement->job.work_ticks == 0);
			CHECK(replacement->job.inputs[0].TotalCount() == 0); CHECK(replacement->job.inputs[1].TotalCount() == 0);
			CHECK(Company::Get(_current_company)->money == cash - Money{GetSpaceConstructionDefinition(geometry.kind)->capital});
			CHECK(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, old.structure, old.generation).Failed());
			{
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto start = GetAsteroidField().elapsed_ticks;
				const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Observe the real next opportunity: the retained old generation must remain removed after a numeric slot is reused.
				while (GetAsteroidField().elapsed_ticks == start) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop(); std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
			CHECK(builder->space_orders.blocked == SpaceOrderBlock::TargetRemoved); CHECK_FALSE(builder->space_orders.approach); CHECK(builder->position == position);
			CHECK(replacement->job.work_ticks == 0); CHECK(IsOrbitalModuleOperational(*dock)); CHECK(GetUtilityPowerProducer(source)->consumed_lifetime_j == used);
			REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
			AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / "module-removed-422.sav"), false) == SaveLoadResult::Ok);
			fmt::print("ORBITAL_REMOVAL native=true actual_paid_receipts=true empty_only=true generation_safe=true no_refund=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(removal_observer, &observe); IConsole::CmdRegister("orbital_removal_test", ObserveRemoval);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
