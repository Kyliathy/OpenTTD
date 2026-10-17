/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_materialization_native.cpp Exact paid work and one canonical native product from the original accepted Surface-delivered recipe batch.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8
 * Inputs come from exact Group7 evidence with declared starting Surface test content; this fixture creates no stock, hull, paid work or receipt privately.
 */
#include "../stdafx.h"
#include "../fios.h"
#include "assembly_native_fixture.h"
#include "../orbital_manufacturing.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../core/string_consumer.hpp"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include "../safeguards.h"

static std::function<void()> *materialization_observer = nullptr; ///< One explicit actual loaded-world acceptance observer.
/** @param args Actual native console tokens. @return Whether this invocation's original loaded-world observer executed. */
static bool ObserveManufacturingMaterialization(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(materialization_observer != nullptr); (*materialization_observer)(); return true;
}

/** Observe exactly one bounded paid-work, completion, repeat, recovery or provenance boundary without fixture stock, hull or progress grants. */
TEST_CASE("Original paid orbital manufacture completes one canonical native product", "[.manufacturing-materialization-native]")
{
	const char *source = std::getenv("OTTD_MATERIALIZATION_SOURCE"), *mode = std::getenv("OTTD_MATERIALIZATION_ACTION");
	REQUIRE(source != nullptr); REQUIRE(mode != nullptr); const std::string_view action{mode};
	const char *selected = std::getenv("OTTD_MATERIALIZATION_MODEL"); const auto model_value = ParseInteger<uint16_t>(selected == nullptr ? "15" : selected);
	REQUIRE(model_value.has_value()); const uint16_t model = *model_value;
	const auto paths = GetTestCaseArtifactPaths("space", "manufacturing-materialization-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect only the actual loaded native world; preserve assertion failures through normal native application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			SpaceStructure *assembly = nullptr;
			// Resolve the original real paid facility incarnation; copied definitions or a fabricated factory ID cannot supply a producer.
			for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Assembly) { REQUIRE(assembly == nullptr); assembly = site; }
			REQUIRE(assembly != nullptr); REQUIRE(assembly->module != nullptr); REQUIRE(assembly->module->manufacture != nullptr);
			auto &job = *assembly->module->manufacture; REQUIRE(job.model == model); const auto &recipe = *GetSpaceVehicleConstruction(model);
			const auto serial = job.serial; const auto hulls = GetSpaceVehicleCount(); const auto source_ref = GetSpaceStructureReference(*assembly);
			const auto capital = Company::Get(_current_company)->money;
			/** @param name Explicit native boundary basename. Save only original actual owners after complete physical validation. */
			auto save = [&](std::string_view name) { world.Save(fmt::format("manufacture-{}-{}-427.sav", name, model)); };
			if (const char *corrupt = std::getenv("OTTD_MATERIALIZATION_CORRUPT")) {
				const auto before = ComputeSpaceVehicleDigest(), utility = ComputeUtilityPowerDigest(), structures = ComputeSpaceStructureDigest(); const auto random = _random;
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceVehicleDigest() == before); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeSpaceStructureDigest() == structures);
				CHECK(Company::Get(_current_company)->money == capital); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("MANUFACTURE_MATERIALIZATION native=true preview_rejected=true unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			// These three original-job checkpoints differ only in their paid work target; none may replenish materials or issue another factory serial between saves.
			if (action == "halfway" || action == "before" || action == "complete") {
				const auto target = action == "halfway" ? recipe.work_ticks / 2 : action == "before" ? recipe.work_ticks - 1 : recipe.work_ticks;
				REQUIRE(job.work_ticks < target); const auto initial = job.work_ticks, remaining = target - initial;
				const auto spent = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
				const auto held = CaptureSpaceStructures(); world.Advance(remaining);
				CHECK(job.work_ticks == target); CHECK(job.work_energy_j == uint64_t{target} * 750);
				CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == spent + uint64_t{remaining} * 750);
				if (action == "complete") {
					REQUIRE(job.phase == OrbitalManufacturePhase::Complete); CHECK(GetSpaceVehicleCount() == hulls + 1);
					auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr); CHECK(GetSpaceVehicleModel(*product) == model);
					CHECK(product->construction.assembly == source_ref); CHECK(product->construction.assembly_serial == serial);
					CHECK(product->construction.origin == LaunchCenterRef{}); CHECK(product->construction.home == LaunchCenterRef{}); CHECK(product->construction.surface_tile == INVALID_TILE);
					CHECK(product->construction.capital_paid == recipe.capital); CHECK(GetSpaceStructureBerth(*product, *assembly) == std::optional<uint8_t>{0});
					// The one final prepared native commit consumes every original Surface-delivered label and records exactly this product's embodied recipe.
					for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) {
						CHECK(job.inputs[input].TotalCount() == 0); CHECK(job.consumed[input] == (input < recipe.inputs.size() ? recipe.inputs[input].quantity : 0));
					}
				} else {
					CHECK(job.phase == OrbitalManufacturePhase::Working); CHECK(GetSpaceVehicleCount() == hulls); CHECK(job.product == SpaceCraftRef{});
					const auto after = CaptureSpaceStructures(); REQUIRE(after.size() == held.size());
					// Paid work does not consume/replace reserved material early; every original packet identity and route remains in its actual batch until completion admission.
					for (size_t row = 0; row < held.size(); ++row) if (held[row].ref == source_ref) CHECK(after[row].module->manufacture->inputs == held[row].module->manufacture->inputs);
				}
				save(action);
			} else if (action == "repeat" || action == "depart") {
				// A completed job remains a receipt while its actual product waits or departs; either path must preserve the same hull, embodied bill and factory serial.
				REQUIRE(job.phase == OrbitalManufacturePhase::Complete); auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
				const auto original = job.product; const auto paid = job.work_energy_j; // Separate real receipt and paid energy owners below; neither may be renewed.
				if (action == "depart") {
					world.Order(*product, *world.core, SpaceOrderAction::Visit);
					/** @return Whether this same paid product reached the actual Core berth through its typed Visit order, without fixture movement. */
					world.Await([&] { return GetSpaceStructureBerth(*product, *world.core).has_value(); });
					CHECK(IsSpaceStructureManufactureBerthFree(*assembly));
				} else {
					world.Advance(3); AutoRestoreBackup pause(_pause_mode, PauseModes{}); TickOrbitalManufacturingWork(); TickOrbitalManufacturingWork();
				}
				CHECK(GetSpaceVehicleCount() == hulls); CHECK(job.product == original); CHECK(job.work_energy_j == paid); CHECK(job.serial == serial); save(action);
			} else if (action == "outage" || action == "restore") {
				// Only the original panel's availability changes: restoring it admits one new paid quantum, while outage preserves all earlier work and reserved material.
				REQUIRE(job.phase == OrbitalManufacturePhase::Working); const auto before = job.work_ticks; const auto energy = job.work_energy_j;
				REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(world.core->index, world.core->generation, action == "outage"));
				world.Advance(action == "outage" ? 3 : 1);
				CHECK(job.work_ticks == before + (action == "outage" ? 0 : 1)); CHECK(job.work_energy_j == energy + (action == "outage" ? 0 : 750));
				CHECK(GetSpaceVehicleCount() == hulls); CHECK(job.product == SpaceCraftRef{}); save(action);
			} else if (action == "cancel") {
				REQUIRE(job.phase == OrbitalManufacturePhase::Working); const auto spent = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
				std::array<uint, MAX_ORBITAL_MANUFACTURE_INPUTS> expected{};
				// Cancellation returns the whole still-unconsumed original batch while already paid common work stays spent and no capital is refunded.
				for (size_t input = 0; input < recipe.inputs.size(); ++input) expected[input] = world.core->module->stock[GetCargoTypeByLabel(recipe.inputs[input].label)].TotalCount() + recipe.inputs[input].quantity;
				REQUIRE(Command<Commands::CancelOrbitalManufacture>::Post(assembly->index, assembly->generation, world.core->index, world.core->generation));
				CHECK(assembly->module->manufacture == nullptr); CHECK(assembly->module->manufacture_serial == serial); CHECK(GetSpaceVehicleCount() == hulls);
				CHECK(Company::Get(_current_company)->money == capital); CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == spent);
				for (size_t input = 0; input < recipe.inputs.size(); ++input) CHECK(world.core->module->stock[GetCargoTypeByLabel(recipe.inputs[input].label)].TotalCount() == expected[input]);
				save(action);
			} else if (action == "company") {
				REQUIRE(job.phase == OrbitalManufacturePhase::Complete); const auto original = job.product;
				REQUIRE(Company::CanAllocateItem()); auto *company = Company::Create(); SetCompanyManagerFaceStyle(company->face, 0); company->money = 10000000;
				const auto owner = company->index; ChangeOwnershipOfCompanyItems(_current_company, owner); REQUIRE(ValidateSpaceVehicles());
				REQUIRE(ResolveSpaceCraft(original) != nullptr); CHECK(ResolveSpaceCraft(original)->owner == owner);
				CHECK(ResolveSpaceCraft(original)->construction.assembly == source_ref); CHECK(ResolveSpaceCraft(original)->construction.assembly_serial == serial);
				CHECK(assembly->state.owner == owner); CHECK(job.product == original); CHECK(job.work_energy_j == uint64_t{recipe.work_ticks} * 750);
				ChangeOwnershipOfCompanyItems(owner, INVALID_OWNER); delete Company::Get(owner); CHECK(ResolveSpaceCraft(original) == nullptr);
				REQUIRE(Company::CanAllocateItem()); auto *reused = Company::Create(); SetCompanyManagerFaceStyle(reused->face, 0); reused->money = 10000000; CHECK(reused->index == owner);
				{
					AutoRestoreBackup other(_current_company, reused->index);
					CHECK(CmdCancelOrbitalManufacture({}, assembly->index, assembly->generation, world.core->index, world.core->generation).Failed());
					CHECK(CmdQueueOrbitalManufacture({}, assembly->index, assembly->generation, world.core->index, world.core->generation, model).Failed());
				}
				CHECK(assembly->state.owner == OWNER_NONE); CHECK(job.product == original); save(action);
				fmt::print("MANUFACTURE_MATERIALIZATION native=true phase=company model={} company_id_reuse_refused=true\n", model);
				observed = true; _exit_game = true; return; // Native company deletion owns hull teardown; non-owning world/product pointers must not be read again.
			} else {
				REQUIRE(action == "second"); REQUIRE(model == 19); REQUIRE(job.phase == OrbitalManufacturePhase::Complete); const auto first = job.product;
				REQUIRE(IsSpaceStructureManufactureBerthFree(*assembly));
				REQUIRE(Command<Commands::CancelOrbitalManufacture>::Post(assembly->index, assembly->generation, world.core->index, world.core->generation));
				REQUIRE(Command<Commands::QueueOrbitalManufacture>::Post(assembly->index, assembly->generation, world.core->index, world.core->generation, model));
				CHECK(Company::Get(_current_company)->money == capital - recipe.capital);
				world.Advance(recipe.work_ticks + 1); const auto &next = *assembly->module->manufacture;
				CHECK(next.phase == OrbitalManufacturePhase::Complete); CHECK(next.serial == serial + 1); CHECK(next.product != first); CHECK(ResolveSpaceCraft(first) != nullptr);
				CHECK(GetSpaceVehicleCount() == hulls + 1); save(action);
			}
			REQUIRE(ValidateSpaceVehicles());
			// Capture the actual pre/post milestone controls so their paid progress and original completed product can be reviewed against the native save evidence.
			if (action == "before" || action == "complete") {
				REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				ShowSpaceConstructionWindow(source_ref); auto *window = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(window != nullptr);
				CaptureNativeArtWindow(*window, paths.images / fmt::format("manufacture-{}-{}.ppm", action, model));
			}
			fmt::print("MANUFACTURE_MATERIALIZATION native=true phase={} model={} original_factory=true paid_work=true single_consumption=true\n", action, model);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(materialization_observer, &observe); IConsole::CmdRegister("manufacturing_materialization_test", ObserveManufacturingMaterialization);
	const std::string input{source}, configuration = config.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", input};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
