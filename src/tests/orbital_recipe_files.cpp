/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_recipe_files.cpp Ordinary native/copy admission and continuation of explicitly isolated processing-file fixtures.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C
 * Source files declare consumed-frame/packet unit inputs; paid native processor procurement remains Group11.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../orbital_recipe.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../company_func.h"
#include "../company_base.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../timer/timer_game_tick.h"
#include "../timer/timer_game_calendar.h"
#include "../timer/timer_game_economy.h"
#include "test_artifact_paths.h"
#include "layer_save_fixture.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *recipe_file_observer = nullptr; ///< Synchronous test callback; it grants no gameplay ownership.

/** Invoke only after the ordinary native loader accepts all physical/profile/utility/processing families.
 * @param args Exact registered console invocation with no gameplay arguments.
 * @return Whether the installed acceptance observer ran.
 */
static bool ObserveRecipeFile(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(recipe_file_observer); (*recipe_file_observer)(); return true;
}

/** Declare isolated consumed construction receipts without modifying the actual loader's cargo registry or creating gameplay procurement evidence.
 * @param kind Original immutable construction definition.
 * @param index Explicit native restore-fixture identity; the complete empty native family is admitted before installation.
 * @param x Test footprint sharing the actual Core fixture's edge.
 * @return Inert frame with its declared unit construction receipts, no activation or packet stock.
 */
static SpaceStructureRecord RecipeFileFrame(SpaceBuildKind kind, uint16_t index, uint16_t x)
{
	const auto &definition = *GetSpaceConstructionDefinition(kind); SpaceStructureRecord row;
	row.ref = {SpaceStructureID{index}, uint64_t{index} + 1}; row.state.owner = _current_company;
	row.state.kind = kind; row.state.phase = SpaceStructurePhase::FrameComplete; row.state.x = x; row.state.y = 4; row.state.capital_paid = definition.capital;
	row.work_ticks = definition.work_ticks; row.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)};
	return row;
}

/** Ordinary native initialization supplies actual package/cargo properties before a declared unit processing-file fixture is assembled. */
TEST_CASE("Orbital recipe metadata exports actual isolated native save checkpoints", "[.orbital-recipe-export]")
{
	const char *input = std::getenv("OTTD_ORBITAL_RECIPE_SOURCE"), *mode = std::getenv("OTTD_ORBITAL_RECIPE_PHASE"); REQUIRE(input); REQUIRE(mode);
	const std::string_view phase{mode}; const auto paths = GetTestCaseArtifactPaths("space", "orbital-recipe-export"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Assemble only explicit unit inputs after the real loader has accepted its original world/content; no private pool or cargo definition is copied. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			REQUIRE(SpaceStructure::GetNumItems() == 0); REQUIRE(GetNextSpaceStructureGeneration() == 1); REQUIRE(GetSpaceStructureRetirementHistory() == SpaceStructureRetirementHistory{});
			// This openly declared unit-file scenario starts at the canonical recipe introduction. It supplies no paid gameplay predecessor or financial proof.
			TimerGameCalendar::SetDate(TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1995}, 0, 1), 0);
			TimerGameEconomy::SetDate(TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{1995}, 0, 1), 0);
			const auto kind = phase == "unpowered" ? SpaceBuildKind::Refinery : SpaceBuildKind::Steelworks;
			std::vector rows{RecipeFileFrame(SpaceBuildKind::Core, 0, 4), RecipeFileFrame(kind, 1, 7)};
			if (phase == "fractional") rows.push_back(RecipeFileFrame(kind, 2, 1));
			REQUIRE(InstallLoadedSpaceStructures(rows, phase == "fractional" ? 4 : 3));
			/** @param ticks Actual normal synchronized ticks, with no fixture electricity or physical progress injected. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Native fast-forward can yield without advancing; only the actual game clock earns commissioning or processing opportunities.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto tick = TimerGameTick::counter; StateGameLoop();
					if (tick == TimerGameTick::counter) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(TimerGameTick::counter == tick + 1); ++done;
				}
			};
			auto &core = *SpaceStructure::Get(SpaceStructureID{0}); auto &first = *SpaceStructure::Get(SpaceStructureID{1});
			REQUIRE(CmdActivateOrbitalCore({DoCommandFlag::Execute}, core.index, core.generation).Succeeded()); advance(GetMultiLayerSpaceBudget().core_activation_ticks); REQUIRE(IsOrbitalModuleOperational(core));
			// The extra idle native consumer is never requested or credited; negative file cases use a real wrong-consumer target without inventing an identity.
			const auto idle = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, 750, std::array{core.module->producer}); REQUIRE(idle);
			// Each declared processor frame completes its own native commissioning before its private work connection or material input is available.
			for (const auto &row : rows) {
				if (row.state.kind == SpaceBuildKind::Core) continue;
				auto &site = *ResolveSpaceStructure(row.ref);
				REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, site.index, site.generation).Succeeded()); advance(GetMultiLayerSpaceBudget().core_activation_ticks); REQUIRE(IsOrbitalModuleOperational(site));
			}
			const auto nore = GetCargoTypeByLabel(CargoLabel{"NORE"}); REQUIRE(IsValidCargoType(nore)); CHECK(CargoSpec::Get(nore)->weight == 16);
			// Explicit unit input packets use the real loader's physical cargo meaning and sole native module stock; they claim no transported/mined/paid procurement history.
			for (const auto &row : rows) {
				if (row.state.kind == SpaceBuildKind::Core) continue;
				auto &site = *ResolveSpaceStructure(row.ref); REQUIRE(CargoPacket::CanAllocateItem());
				site.module->stock[nore].Append(CargoPacket::Create(StationID::Invalid(), uint16_t{4}, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid()); ++site.module->processing->revision;
				if (phase != "unpowered") REQUIRE(CmdConnectOrbitalServices({DoCommandFlag::Execute}, site.index, site.generation, core.index, core.generation).Succeeded());
				REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Start)).Succeeded());
			}
			advance(phase == "complete" || phase == "retired" ? 74 : 13);
			if (phase == "cancelled") REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, first.index, first.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Cancel)).Succeeded());
			if (phase == "retired") {
				const auto ref = GetSpaceStructureReference(first);
				REQUIRE(CmdEvacuateOrbitalModule({DoCommandFlag::Execute}, ref.structure, ref.generation, core.index, core.generation).Succeeded());
				REQUIRE(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, ref.structure, ref.generation).Succeeded());
			}
			REQUIRE(ValidateSpaceStructures()); REQUIRE(ValidateOrbitalRecipes()); const auto before = CaptureOrbitalRecipes();
			{
				AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("orbital-recipe-{}.sav", phase)), false) == SaveLoadResult::Ok);
			}
			CHECK(CaptureOrbitalRecipes() == before);
			fmt::print("ORBITAL_RECIPE_EXPORT declared_unit_inputs=true phase={} actual_cargo_weight={} live_records={}\n", phase, CargoSpec::Get(nore)->weight, before.size()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(recipe_file_observer, &observe); IConsole::CmdRegister("orbital_recipe_file_test", ObserveRecipeFile); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", input};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** Actual ordinary file load, same-file copied admission, own resave and real native continuation retain one paid-work/material owner. */
TEST_CASE("Ordinary orbital recipe files retain exact work and material owners", "[.orbital-recipe-file]")
{
	const char *input = std::getenv("OTTD_ORBITAL_RECIPE_SOURCE"), *mode = std::getenv("OTTD_ORBITAL_RECIPE_PHASE");
	REQUIRE(input); REQUIRE(mode); const std::string_view phase{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "orbital-recipe-file"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Exercise only the ordinary accepted game; failure is retained across normal native shutdown. */
	std::function<void()> observe = [&] {
		try {
			auto *site = SpaceStructure::GetIfValid(SpaceStructureID{static_cast<uint16_t>(phase == "retired" ? 0 : 1)}); REQUIRE(site); REQUIRE(site->module);
			AutoRestoreBackup actor(_current_company, site->state.owner); REQUIRE(Company::IsValidID(_current_company)); REQUIRE(ValidateSpaceStructures()); REQUIRE(ValidateOrbitalRecipes());
			const auto before = CaptureOrbitalRecipes(); const auto power = ComputeUtilityPowerDigest();
			const auto money = Company::Get(_current_company)->money; const auto random = _random;
			const auto source_tick = TimerGameTick::counter;
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(CaptureOrbitalRecipes() == before); CHECK(ComputeUtilityPowerDigest() == power); CHECK(TimerGameTick::counter == source_tick);
			CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			/** @param name Exact run-owned ordinary checkpoint filename; this writes no substitute processing or utility state. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateOrbitalRecipes()); REQUIRE(ValidateSpaceStructures());
				AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			save("orbital-recipe-own.sav");
			if (const char *bad = std::getenv("OTTD_ORBITAL_RECIPE_CORRUPT")) {
				CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK_FALSE(_load_check_data.error_msg.empty()); fmt::print("ORBITAL_RECIPE_FILE preview_error={}\n", _load_check_data.error_msg);
				CHECK(CaptureOrbitalRecipes() == before); CHECK(ComputeUtilityPowerDigest() == power); CHECK(TimerGameTick::counter == source_tick);
				CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				fmt::print("ORBITAL_RECIPE_FILE copied_rejected=true authority_unchanged=true\n");
			} else {
				if (phase == "retired") {
					CHECK(before.empty()); const auto &history = GetSpaceStructureRetirementHistory();
					CHECK(history.processing_batches.at(MultiLayerRecipeID{"OSTL"}) == 1); CHECK(GetOrbitalRecipeRetirementEnergy(history) == std::optional<uint64_t>{55500});
					CHECK(site->module->stock[GetCargoTypeByLabel(CargoLabel{"STEL"})].TotalCount() == 2);
					save("orbital-recipe-continued.sav");
					fmt::print("ORBITAL_RECIPE_FILE native=true declared_unit_inputs=true phase=retired paid=55500 discarded=0 batches=1\n");
					observed = true; _exit_game = true; return;
				}
				REQUIRE(site->module->processing); auto &state = *site->module->processing;
				const auto nore = GetCargoTypeByLabel(CargoLabel{"NORE"});
				if (phase == "complete") {
					CHECK(state.paid_lifetime_j == 55500); CHECK(state.electrical->Capture().paid_j == 0); CHECK(site->module->stock[nore].TotalCount() == 0);
				} else if (phase == "unpowered") {
					CHECK(state.unpowered_work_ticks == 13); CHECK_FALSE(state.electrical); CHECK(state.choice == MultiLayerRecipeID{"OREF"});
				} else {
					REQUIRE(state.electrical); CHECK(state.paid_lifetime_j == 9750); CHECK(state.electrical->Capture().paid_j == (phase == "cancelled" ? 0 : 9750));
					CHECK(state.discarded_paid_j == (phase == "cancelled" ? 9750 : 0)); CHECK(site->module->stock[nore].TotalCount() == 4);
				}
				if (phase == "fractional") {
					auto &second = *SpaceStructure::Get(SpaceStructureID{2}); REQUIRE(second.module); REQUIRE(second.module->processing);
					CHECK(second.module->processing->electrical->Capture().paid_j == 780); CHECK(second.module->processing->electrical->WorkTicks() == 1);
				}
				if (phase == "cancelled") REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site->index, site->generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Start)).Succeeded());
				const uint ticks = phase == "complete" ? 5 : phase == "cancelled" ? 74 : 61;
				{
					AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
					// Count only actual normal-game native timer advancement; renderer/fast-forward polling cannot supply an extra recipe opportunity.
					for (uint done = 0; done < ticks;) {
						REQUIRE(std::chrono::steady_clock::now() < deadline); const auto tick = TimerGameTick::counter; StateGameLoop();
						if (tick == TimerGameTick::counter) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
						REQUIRE(TimerGameTick::counter == tick + 1); ++done;
					}
				}
				CHECK(site->module->stock[nore].TotalCount() == 0); CHECK(state.batches.at(state.choice) == 1);
				const auto output = GetCargoTypeByLabel(phase == "unpowered" ? CargoLabel{"RFMT"} : CargoLabel{"STEL"}); CHECK(site->module->stock[output].TotalCount() == 2);
				CHECK(state.paid_lifetime_j == (phase == "unpowered" ? 0 : phase == "cancelled" ? 65250 : 55500));
				if (phase == "fractional") {
					const auto &second = *SpaceStructure::Get(SpaceStructureID{2}); CHECK(second.module->processing->electrical->Capture().paid_j == 4440);
					CHECK(second.module->stock[nore].TotalCount() == 4); CHECK(second.module->processing->batches.at(MultiLayerRecipeID{"OSTL"}) == 0);
				}
				save("orbital-recipe-continued.sav");
				fmt::print("ORBITAL_RECIPE_FILE native=true declared_unit_inputs=true phase={} paid={} discarded={} batches={}\n", phase, state.paid_lifetime_j, state.discarded_paid_j, state.batches.at(state.choice));
			}
			observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(recipe_file_observer, &observe); IConsole::CmdRegister("orbital_recipe_file_test", ObserveRecipeFile); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", input};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
