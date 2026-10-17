/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_commissioning_native.cpp Actual independently paid21 commissioning from immutable delivered factory/stock checkpoints.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9B
 * The accepted upstream world's declared starting-content/procurement limits remain explicit. This observer adds no stock, money, hull, pose, work, pod or loose service part.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_mining_tuning.h"
#include "../space_purchase.h"
#include "../orbital_manufacturing.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_commission_observer = nullptr; ///< Exact current native process observer; all state remains with original physical owners.

/** @param args Parameter-free native loaded-world console callback. @return Whether the explicitly installed observer ran. */
static bool ObserveHeavyCommissioning(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_commission_observer != nullptr); (*heavy_commission_observer)(); return true;
}

/** Execute one independently bounded original commissioning milestone without preparing new physical source content. */
TEST_CASE("Appended heavy miner uses the original paid factory and preserved legacy receipts", "[.heavy-commissioning-native]")
{
	const char *input = std::getenv("OTTD_HEAVY_COMMISSION_SOURCE"), *phase_env = std::getenv("OTTD_HEAVY_COMMISSION_PHASE"); REQUIRE(input != nullptr); REQUIRE(phase_env != nullptr);
	const std::string_view phase{phase_env}; const auto paths = GetTestCaseArtifactPaths("space", "heavy-commissioning-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe only the actual loaded world and preserve assertions through ordinary native application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths); SpaceStructure *factory = nullptr;
			// The accepted source has one real original Assembly. Its public type/owner supplies the factory identity; a guessed ID or newly allocated proxy cannot substitute another producer.
			for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Assembly) { REQUIRE(factory == nullptr); factory = site; }
			REQUIRE(factory != nullptr); REQUIRE(factory->module != nullptr); REQUIRE(factory->state.owner == _current_company);
			const auto *recipe = GetSpaceVehicleConstruction(SPACE_HEAVY_MINER_MODEL); REQUIRE(recipe != nullptr); REQUIRE(recipe->inputs.size() == 4);
			const auto before_hulls = GetSpaceVehicleCount(); const auto before_generation = GetNextSpaceVehicleGeneration();
			const auto original_receipts = CaptureSpaceProduction();
			if (const char *corrupt = std::getenv("OTTD_HEAVY_COMMISSION_CORRUPT")) {
				const auto fleet = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
				const auto money = Company::Get(_current_company)->money; const auto random = _random;
				REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == power);
				CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("HEAVY_COMMISSION preview_reject=true authority_unchanged=true\n");
			} else if (phase == "upgrade") {
				CHECK(factory->module->manufacture == nullptr); CHECK(GetSpaceEngine(SPACE_HEAVY_MINER_MODEL) == EngineID::Invalid());
				// Genuine428 inputs retain every actual older paid receipt. Loading a new definition is not another commissioning, service or material payment.
				for (const auto &row : original_receipts) CHECK((row.state.content_version == 0 || row.state.content_version == 2));
			} else if (phase == "queued") {
				REQUIRE(factory->module->manufacture == nullptr); const auto money = Company::Get(_current_company)->money; const auto random = _random;
				const auto digest = ComputeSpaceStructureDigest(); const auto engine = GetSpaceEngine(SPACE_HEAVY_MINER_MODEL);
				const auto home = world.rocket->construction.home;
				CHECK(CmdAssembleLaunchCraft({}, home.station, home.generation, SPACE_HEAVY_MINER_MODEL).Failed()); // The real existing Earth workshop cannot borrow the new orbital-only commissioning permission.
				// Real quotes, including mixed Execute/QueryCost, may inspect the complete physical facility without consuming inputs, registering an engine or reserving another product.
				for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) {
					const auto quote = CmdQueueOrbitalManufacture(flags, factory->index, factory->generation, world.core->index, world.core->generation, SPACE_HEAVY_MINER_MODEL);
					REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == Money{recipe->capital});
				}
				CHECK(ComputeSpaceStructureDigest() == digest); CHECK(Company::Get(_current_company)->money == money);
				CHECK(GetSpaceEngine(SPACE_HEAVY_MINER_MODEL) == engine); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				REQUIRE(Command<Commands::QueueOrbitalManufacture>::Post(factory->index, factory->generation, world.core->index, world.core->generation, SPACE_HEAVY_MINER_MODEL));
				REQUIRE(factory->module->manufacture != nullptr); const auto &job = *factory->module->manufacture;
				CHECK(job.model == SPACE_HEAVY_MINER_MODEL); CHECK(job.phase == OrbitalManufacturePhase::Queued); CHECK(job.work_ticks == 0); CHECK(job.product == SpaceCraftRef{});
				CHECK(Company::Get(_current_company)->money == money - Money{recipe->capital});
			} else if (phase == "reserved") {
				REQUIRE(factory->module->manufacture != nullptr);
				/** @return Whether actual complete-bill reservation reached its original zero-work boundary. */
				world.Await([&] { return factory->module->manufacture->phase == OrbitalManufacturePhase::Reserved; });
				CHECK(factory->module->manufacture->work_ticks == 0);
			// Both checkpoints exercise the same earned-work owner; only the requested boundary differs, so neither path consumes the final bill or allocates a product early.
			} else if (phase == "partial" || phase == "before") {
				REQUIRE(factory->module->manufacture != nullptr); const uint32_t target = phase == "partial" ? 37 : recipe->work_ticks - 1;
				/** @return Whether original common-grant work reached the exact requested partial/final opportunity. */
				world.Await([&] { return factory->module->manufacture->work_ticks == target; });
				const auto &job = *factory->module->manufacture; CHECK(job.product == SpaceCraftRef{});
				// Every actual label remains reserved through the final pre-completion opportunity; neither a partial bill nor a second phantom hull may own it.
				for (size_t slot = 0; slot < recipe->inputs.size(); ++slot) { CHECK(job.inputs[slot].TotalCount() == recipe->inputs[slot].quantity); CHECK(job.consumed[slot] == 0); }
			} else if (phase == "complete") {
				REQUIRE(factory->module->manufacture != nullptr); REQUIRE(factory->module->manufacture->work_ticks == recipe->work_ticks - 1);
				/** @return Whether the original remaining paid work consumed the complete original bill and issued its unique native product. */
				world.Await([&] { return factory->module->manufacture->phase == OrbitalManufacturePhase::Complete; });
				const auto &job = *factory->module->manufacture; auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
				CHECK(GetSpaceVehicleModel(*product) == SPACE_HEAVY_MINER_MODEL); CHECK(product->construction.content_version == 3);
				CHECK(product->construction.assembly == GetSpaceStructureReference(*factory)); CHECK(product->construction.assembly_serial == job.serial);
				CHECK(product->cargo.TotalCount() == 0); CHECK(product->cargo_cap == 40); REQUIRE(product->manifest != nullptr);
				CHECK(product->manifest->returns_remaining == 2); CHECK(product->manifest->services_completed == 0); CHECK(product->manifest->pods.empty());
				CHECK(product->manifest->supplies[1].TotalCount() == 0); CHECK(product->manifest->supplies[2].TotalCount() == 0);
				CHECK(GetSpaceVehicleCount() == before_hulls + 1); CHECK(GetNextSpaceVehicleGeneration() == before_generation + 1);
				// Final material consumption belongs to this actual complete receipt exactly once, including every advanced component.
				for (size_t slot = 0; slot < recipe->inputs.size(); ++slot) { CHECK(job.inputs[slot].TotalCount() == 0); CHECK(job.consumed[slot] == recipe->inputs[slot].quantity); }
			} else if (phase == "repeat") {
				REQUIRE(factory->module->manufacture != nullptr); const auto &job = *factory->module->manufacture; REQUIRE(job.phase == OrbitalManufacturePhase::Complete);
				const auto product = job.product; world.Advance(10); CHECK(job.product == product); CHECK(GetSpaceVehicleCount() == before_hulls); CHECK(GetNextSpaceVehicleGeneration() == before_generation);
			} else if (phase == "cancel") {
				REQUIRE(factory->module->manufacture != nullptr); const auto money = Company::Get(_current_company)->money;
				std::array<uint, 4> stock{};
				// Cancellation returns each original unconsumed label to its actual owned source inventory; spent capital/work are not a refund or another output hull.
				for (size_t slot = 0; slot < recipe->inputs.size(); ++slot) stock[slot] = world.core->module->stock[GetCargoTypeByLabel(recipe->inputs[slot].label)].TotalCount();
				REQUIRE(Command<Commands::CancelOrbitalManufacture>::Post(factory->index, factory->generation, world.core->index, world.core->generation));
				CHECK(factory->module->manufacture == nullptr); CHECK(Company::Get(_current_company)->money == money);
				for (size_t slot = 0; slot < recipe->inputs.size(); ++slot) CHECK(world.core->module->stock[GetCargoTypeByLabel(recipe->inputs[slot].label)].TotalCount() == stock[slot] + recipe->inputs[slot].quantity);
			} else FAIL("Unknown explicit heavy commissioning phase");
			// Old paid provenance is immutable even while the new job advances; whole cargo/accounting state may evolve only through the original native world ticks.
			for (const auto &row : original_receipts) { const auto *craft = ResolveSpaceCraft(row.craft); REQUIRE(craft != nullptr); CHECK(craft->construction == row.state); }
			world.Save(fmt::format("heavy-commission-{}-431.sav", phase));
			const auto *job = factory->module->manufacture.get(); const auto product = job == nullptr ? SpaceCraftRef{} : job->product;
			fmt::print("HEAVY_COMMISSION native=true phase={} fleet={} next={} work={} serial={} product={} generation={}\n", phase, GetSpaceVehicleCount(), GetNextSpaceVehicleGeneration(),
				job == nullptr ? 0 : job->work_ticks, factory->module->manufacture_serial, product.vehicle.base(), product.generation);
		} catch (...) { failure = std::current_exception(); }
		_current_company = _local_company; observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(heavy_commission_observer, &observe); IConsole::CmdRegister("heavy_commission_test", ObserveHeavyCommissioning);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
