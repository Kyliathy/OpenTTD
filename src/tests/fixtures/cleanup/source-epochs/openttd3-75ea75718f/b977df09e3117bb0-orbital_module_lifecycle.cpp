/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_module_lifecycle.cpp Paid blueprint staging, dependency-local removal and actual company lifecycle.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group5C.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_approach.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../launch_center_cmd.h"
#include "../orbital_service.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
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

static std::function<void()> *lifecycle_observer = nullptr;
/** @param args Native console invocation. @return Whether the exact installed loaded-world observer ran. */
static bool ObserveModuleLifecycle(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(lifecycle_observer != nullptr); (*lifecycle_observer)(); return true;
}

/** Real commands and delivered inventory supply every module/hull; no fixture-created consumed receipt or private deletion is allowed. */
TEST_CASE("Paid module lifecycle keeps Core removal local and prevents company ID reuse acquiring held parts", "[.orbital-module-lifecycle]")
{
	const char *mode = std::getenv("OTTD_ORBITAL_LIFECYCLE_ACTION"), *input = std::getenv("OTTD_ORBITAL_LIFECYCLE_SOURCE");
	REQUIRE(mode != nullptr); REQUIRE(input != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "module-lifecycle"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Drive actual paid owners while retaining any assertion failure until ordinary native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto &core = *SpaceStructure::Get(SpaceStructureID{0}); auto &storage = *SpaceStructure::Get(SpaceStructureID{1});
			SpaceStructure *dock = nullptr, *replacement = nullptr; SpaceVehicle *rocket = nullptr; std::vector<SpaceVehicle *> builders;
			// Resolve only actual paid native identities; the original first Core is distinct from a newly constructed replacement.
			for (auto *site : SpaceStructure::Iterate()) {
				if (site->state.kind == SpaceBuildKind::Dock) dock = site;
				if (site->state.kind == SpaceBuildKind::Core && site != &core) replacement = site;
			}
			for (auto *craft : SpaceVehicle::Iterate()) {
				if (GetSpaceVehicleModel(*craft) == 14) rocket = craft;
				if (GetSpaceVehicleModel(*craft) == 20) builders.push_back(craft);
			}
			REQUIRE(dock != nullptr); REQUIRE(rocket != nullptr); REQUIRE(builders.size() == 2); REQUIRE(IsOrbitalModuleOperational(core));
			/** @param name Actual native checkpoint basename inside the current invocation's evidence owner. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			if (action == "ownership") {
				REQUIRE(dock->module->service.remaining_ticks != 0); REQUIRE(dock->module->dock_service.remaining_ticks != 0);
				const auto original = core.state.owner; const auto before = CaptureSpaceStructures(); const auto producer = core.module->producer; const auto consumer = dock->module->consumer;
				const auto spent = GetUtilityPowerProducer(producer)->consumed_lifetime_j; const auto history = GetSpaceStructureRetirementHistory();
				REQUIRE(Company::CanAllocateItem()); auto *company = Company::Create(); SetCompanyManagerFaceStyle(company->face, 0); company->money = 10000000; const auto owner = company->index;
				// This explicit company lifecycle fixture creates an actor only; every transferred physical packet, hull and receipt was already paid in the immutable input.
				{ AutoRestoreBackup foreign(_current_company, owner); CHECK(CmdRemoveOrbitalModule({}, dock->index, dock->generation).Failed()); }
				ChangeOwnershipOfCompanyItems(original, owner); REQUIRE(ValidateSpaceVehicles());
				CHECK(core.state.owner == owner); CHECK(dock->state.owner == owner); CHECK(GetUtilityPowerProducer(producer)->owner == owner); CHECK(GetUtilityPowerConsumer(consumer)->owner == owner);
				const auto transferred = CaptureSpaceStructures(); REQUIRE(transferred.size() == before.size());
				// Native takeover changes authority together, preserving generation, actual stock, both contained jobs and every original source reference.
				for (size_t i = 0; i < before.size(); ++i) {
					CHECK(transferred[i].ref == before[i].ref); CHECK(transferred[i].inputs == before[i].inputs); CHECK(transferred[i].consumed == before[i].consumed);
					if (!before[i].module) continue;
					CHECK(transferred[i].module->stock == before[i].module->stock); CHECK(transferred[i].module->utility_source == before[i].module->utility_source);
					const auto old_jobs = before[i].module->GetServiceJobs(), new_jobs = transferred[i].module->GetServiceJobs();
					for (size_t berth = 0; berth < old_jobs.size(); ++berth) { CHECK(new_jobs[berth]->inputs == old_jobs[berth]->inputs); CHECK(new_jobs[berth]->craft == old_jobs[berth]->craft); CHECK(new_jobs[berth]->remaining_ticks == old_jobs[berth]->remaining_ticks); }
				}
				{ AutoRestoreBackup new_actor(_current_company, owner); CHECK(ClassifyOrbitalModuleRemoval(*dock, owner) == OrbitalRemovalBlock::Service); CHECK(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, dock->index, dock->generation).Failed()); }
				ChangeOwnershipOfCompanyItems(owner, INVALID_OWNER); delete Company::Get(owner);
				CHECK(core.state.owner == OWNER_NONE); CHECK(dock->state.owner == OWNER_NONE); CHECK_FALSE(GetUtilityPowerProducer(producer)->active); CHECK_FALSE(GetUtilityPowerConsumer(consumer)->active);
				CHECK(dock->module->service.inputs[0].TotalCount() == 1); CHECK(dock->module->service.inputs[1].TotalCount() == 1);
				CHECK(dock->module->dock_service.inputs[0].TotalCount() == 1); CHECK(dock->module->dock_service.inputs[1].TotalCount() == 1);
				CHECK(ResolveSpaceCraft(dock->module->service.craft) == nullptr); CHECK(ResolveSpaceCraft(dock->module->dock_service.craft) == nullptr);
				REQUIRE(Company::CanAllocateItem()); auto *reused = Company::Create(); SetCompanyManagerFaceStyle(reused->face, 0); reused->money = 10000000; CHECK(reused->index == owner);
				// A reused numeric company cannot acquire OWNER_NONE parts, renew an old source or inherit either retired service grant.
				{ AutoRestoreBackup new_actor(_current_company, reused->index); CHECK(CmdRemoveOrbitalModule({}, dock->index, dock->generation).Failed()); CHECK_FALSE(GetUtilityAllocation(consumer)); CHECK(CmdServiceOrbitalProbe({}, dock->index, dock->generation, dock->module->dock_service.craft.vehicle, dock->module->dock_service.craft.generation, OrbitalServiceControl::Resume).Failed()); }
				CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == spent); CHECK(GetSpaceStructureRetirementHistory() == history);
				save("module-lifecycle-unowned-422.sav"); fmt::print("ORBITAL_LIFECYCLE native=true takeover=true both_held_jobs_retained=true company_id_reuse_refused=true\n");
				observed = true; _exit_game = true; return;
			}
			/** @param ticks Actual synchronized opportunities; no independent delivery/construction clock is advanced. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Count the real controller only; rendering and fast-forward yields cannot earn construction work or rocket recovery.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			/** @param craft Actual hull. @param site Exact owned site. @param action Visit or Construct through its real typed target domain. */
			auto order = [&](SpaceVehicle &craft, const SpaceStructure &site, SpaceOrderAction action) {
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, action, action == SpaceOrderAction::Construct ? SpaceOrderTargetKind::Blueprint : SpaceOrderTargetKind::Structure, uint32_t{site.index.base()}, site.generation));
			};
			if (action == "dock-remove") {
				REQUIRE(dock->module->services_completed == 2);
				const auto before = ComputeSpaceStructureDigest();
				CHECK(ClassifyOrbitalModuleRemoval(*dock, _current_company) == OrbitalRemovalBlock::Vehicle);
				CHECK(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, dock->index, dock->generation).Failed());
				CHECK(ComputeSpaceStructureDigest() == before);
				order(*builders[0], core, SpaceOrderAction::Visit); order(*builders[1], storage, SpaceOrderAction::Visit);
				// Removal becomes possible only after both actual paid hulls depart the last Dock through native admission and movement.
				for (uint elapsed = 0; !GetSpaceStructureBerth(*builders[0], core) || !GetSpaceStructureBerth(*builders[1], storage); ++elapsed) { REQUIRE(elapsed < 2000); advance(1); }
				const auto reference = GetSpaceStructureReference(*dock); const auto consumer = dock->module->consumer;
				const auto producer = core.module->producer; const auto spent = GetUtilityPowerProducer(producer)->consumed_lifetime_j;
				const auto cash = Company::Get(_current_company)->money;
				REQUIRE(ClassifyOrbitalModuleRemoval(*dock, _current_company) == OrbitalRemovalBlock::None);
				REQUIRE(Command<Commands::RemoveOrbitalModule>::Post(dock->index, dock->generation));
				CHECK(ResolveSpaceStructure(reference) == nullptr); CHECK_FALSE(GetUtilityPowerConsumer(consumer)->active);
				CHECK(GetUtilityPowerProducer(producer)->active); CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == spent);
				CHECK(Company::Get(_current_company)->money == cash);
				CHECK(GetSpaceStructureRetirementHistory().structures_removed == 1);
				CHECK((GetSpaceStructureRetirementHistory().consumed == std::array<uint64_t, 2>{8, 12}));
				CHECK(GetSpaceStructureRetirementHistory().services_completed == 2);
				save("module-dock-removed-422.sav");
				fmt::print("ORBITAL_LIFECYCLE native=true occupied_last_dock_refused=true actual_departure=true completed_services_preserved=2\n");
				observed = true; _exit_game = true; return;
			}
			REQUIRE(action == "dependencies"); REQUIRE(replacement == nullptr);
			REQUIRE(storage.module == nullptr); REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(storage.index, storage.generation)); advance(74);
			REQUIRE(IsOrbitalModuleOperational(storage));
			order(*builders[0], core, SpaceOrderAction::Visit); order(*builders[1], storage, SpaceOrderAction::Visit);
			// Existing paid hulls physically vacate Dock before rescuing Core stock; no new flight, synthetic fuel or replacement facility is required.
			for (uint elapsed = 0; !GetSpaceStructureBerth(*builders[0], core) || !GetSpaceStructureBerth(*builders[1], storage); ++elapsed) { REQUIRE(elapsed < 2000); advance(1); }
			const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"});
			REQUIRE(core.module->stock[steel].TotalCount() == 2);
			REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(core.index, core.generation, builders[0]->index, builders[0]->generation, steel, uint16_t{2}, true));
			// Each actual spare part enters its declared native service compartment; general freight cannot impersonate a service buffer.
			for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveOrbitalSupply>::Post(core.index, core.generation, builders[0]->index, builders[0]->generation, supply, uint16_t{1}, true));
			const auto tile = TileXYL(dock->state.x + GetSpaceConstructionDefinition(SpaceBuildKind::Dock)->width, dock->state.y, MapLayer::Space);
			REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(tile, SpaceBuildKind::Core));
			// The separate paid blueprint supplies native approach geometry only; it must remain without delivered recipe, stock, power or operational Core functions.
			for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Core && site != &core) replacement = site;
			REQUIRE(replacement != nullptr); REQUIRE(replacement->module == nullptr);
			order(*builders[0], *replacement, SpaceOrderAction::Visit); order(*builders[1], *replacement, SpaceOrderAction::Visit);
			// Both actual hulls must leave all original dependents: the second keeps its deterministic safe waiting point at the independent blueprint's single approach.
			for (uint elapsed = 0; !GetSpaceStructureBerth(*builders[0], *replacement) || ClassifyOrbitalModuleRemoval(storage, _current_company) != OrbitalRemovalBlock::None || ClassifyOrbitalModuleRemoval(*dock, _current_company) != OrbitalRemovalBlock::None; ++elapsed) { REQUIRE(elapsed < 2000); advance(1); }
			CHECK(ClassifyOrbitalModuleRemoval(core, _current_company) == OrbitalRemovalBlock::Connection);
			const auto old_consumer = dock->module->consumer;
			REQUIRE(Command<Commands::DisconnectOrbitalServices>::Post(dock->index, dock->generation));
			CHECK_FALSE(GetUtilityPowerConsumer(old_consumer)->active);
			const auto source_ref = GetSpaceStructureReference(core); const auto producer = core.module->producer;
			const auto consumed = core.job.consumed; const auto services = core.module->services_completed; const auto spent = GetUtilityPowerProducer(producer)->consumed_lifetime_j;
			const auto independent_state = replacement->state, storage_state = storage.state, dock_state = dock->state;
			const auto queues = ComputeSpaceOrdersDigest(), manifests = ComputeSpaceManifestDigest(); const auto cash = Company::Get(_current_company)->money;
			REQUIRE(Command<Commands::RemoveOrbitalModule>::Post(core.index, core.generation));
			CHECK(ResolveSpaceStructure(source_ref) == nullptr); CHECK_FALSE(IsOrbitalModuleOperational(storage)); CHECK_FALSE(IsOrbitalModuleOperational(*dock));
			CHECK(GetOrbitalModuleCargoCapacity(storage, steel) == 0); CHECK(GetOrbitalModuleCargoCapacity(*dock, steel) == 0);
			CHECK(replacement->state == independent_state); CHECK(storage.state == storage_state); CHECK(dock->state == dock_state);
			CHECK(replacement->module == nullptr); CHECK(replacement->job.inputs[0].TotalCount() == 0); CHECK(replacement->job.inputs[1].TotalCount() == 0); CHECK(replacement->job.work_ticks == 0);
			CHECK(ComputeSpaceOrdersDigest() == queues); CHECK(ComputeSpaceManifestDigest() == manifests); CHECK(Company::Get(_current_company)->money == cash);
			CHECK(builders[0]->cargo.TotalCount() == 2); CHECK(builders[0]->manifest->supplies[1].TotalCount() == 1); CHECK(builders[0]->manifest->supplies[2].TotalCount() == 1);
			CHECK(GetSpaceStructureRetirementHistory().structures_removed == 1); CHECK((GetSpaceStructureRetirementHistory().consumed == std::array<uint64_t, 2>{consumed[0], consumed[1]}));
			CHECK(GetSpaceStructureRetirementHistory().services_completed == services); CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == spent); CHECK_FALSE(GetUtilityPowerProducer(producer)->active);
			save("module-dependencies-422.sav");
			fmt::print("ORBITAL_LIFECYCLE native=true phase={} actual_cargo_rescued=true only_dependents_disabled=true independent_blueprint_unchanged=true original_source_history=true\n", action);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(lifecycle_observer, &observe); IConsole::CmdRegister("orbital_lifecycle_test", ObserveModuleLifecycle);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
