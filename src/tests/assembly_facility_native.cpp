/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file assembly_facility_native.cpp Imported Steel/Goods build a real Assembly before its explicit electrical connection.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group6B
 */
#include "../stdafx.h"
#include "assembly_native_fixture.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../console_internal.h"
#include "../fios.h"
#include "../core/random_func.hpp"
#include "../map_func.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include "../safeguards.h"

static std::function<void()> *assembly_observer = nullptr; ///< Exact invocation's loaded-world observer.

/** @param args Registered native console tokens. @return Whether the currently installed actual-world observer ran. */
static bool ObserveAssemblyFacility(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(assembly_observer != nullptr); (*assembly_observer)(); return true;
}

/** Drive one bounded construction/activation/connection boundary using the original native paid owners. */
TEST_CASE("Actual imported inputs activate the paid Assembly without manufacturing a free vehicle", "[.assembly-facility-native]")
{
	const char *input = std::getenv("OTTD_ASSEMBLY_FACILITY_SOURCE"), *mode = std::getenv("OTTD_ASSEMBLY_FACILITY_ACTION");
	REQUIRE(input != nullptr); REQUIRE(mode != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "assembly-facility-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Run only after actual native load; retain observer assertions until ordinary application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			AssemblyNativeWorld world(paths); auto &core = *world.core;
			SpaceStructure *assembly = nullptr;
			// Find the actual native module incarnation; no fixture geometry, material packet or consumed receipt is installed.
			for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Assembly) { REQUIRE(assembly == nullptr); assembly = site; }
			const auto hulls = GetSpaceVehicleCount(); const auto sources = CaptureUtilityPowerState().producers.size();
			if (const char *corrupt = std::getenv("OTTD_ASSEMBLY_FACILITY_CORRUPT")) {
				const auto state = ComputeSpaceVehicleDigest(), utility = ComputeUtilityPowerDigest(); const auto random = _random;
				const auto money = Company::Get(_current_company)->money;
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceVehicleDigest() == state); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(Company::Get(_current_company)->money == money);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("ASSEMBLY_FACILITY native=true preview_rejected=true unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "steel") {
				REQUIRE(assembly == nullptr); SpaceVehicle *builder = nullptr;
				// Select an already paid empty receiver and approach the real Core before placing its adjacent extension.
				for (auto *craft : world.builders) if (craft->cargo.TotalCount() == 0) { builder = craft; break; }
				REQUIRE(builder != nullptr); world.Order(*builder, core, SpaceOrderAction::Visit);
				world.Await([&] { return GetSpaceStructureBerth(*builder, core).has_value(); });
				const auto tile = TileXYL(core.state.x + 1, core.state.y + 3, MapLayer::Space);
				REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(tile, SpaceBuildKind::Assembly));
				// Resolve the newly issued native incarnation; the test cannot invent a parallel construction identity for its paid footprint.
				for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Assembly) assembly = site;
				REQUIRE(assembly != nullptr); CHECK(assembly->state.capital_paid == 180000);
				world.Import(*builder, GetCargoTypeByLabel(CargoLabel{"STEL"}), 8);
				world.Order(*builder, *assembly, SpaceOrderAction::Construct);
				world.Await([&] { return assembly->job.inputs[0].TotalCount() == 8; });
				CHECK(assembly->job.inputs[1].TotalCount() == 0); CHECK(assembly->job.work_ticks == 0);
				CHECK(assembly->module == nullptr); CHECK_FALSE(IsOrbitalModuleOperational(*assembly));
				world.Save("assembly-steel-425.sav");
			// Fresh physical construction and saved partial activation converge at the same final37 opportunities, making exact reload comparison meaningful.
			} else if (action == "goods" || action == "activation") {
				REQUIRE(assembly != nullptr);
				if (action == "goods") {
					SpaceVehicle *builder = nullptr;
					// Only the hull that actually arrived at this saved construction berth may receive and deliver the second input.
					for (auto *craft : world.builders) if (GetSpaceStructureBerth(*craft, *assembly)) builder = craft;
					REQUIRE(builder != nullptr); REQUIRE(assembly->job.inputs[0].TotalCount() == 8);
					world.Import(*builder, GetCargoTypeByLabel(CargoLabel{"GOOD"}), 16);
					world.Order(*builder, *assembly, SpaceOrderAction::Construct);
					world.Await([&] { return assembly->state.phase == SpaceStructurePhase::FrameComplete; });
					CHECK(assembly->job.work_ticks == 592); CHECK((assembly->job.consumed == std::array<uint16_t, 2>{8, 16}));
					REQUIRE(assembly->module == nullptr); world.Save("assembly-frame-425.sav");
					const auto state = ComputeSpaceVehicleDigest(), utility = ComputeUtilityPowerDigest();
					REQUIRE(CmdActivateOrbitalModule({}, assembly->index, assembly->generation).Succeeded());
					CHECK(ComputeSpaceVehicleDigest() == state); CHECK(ComputeUtilityPowerDigest() == utility);
					REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(assembly->index, assembly->generation));
					world.Advance(37); REQUIRE(assembly->module->activation_ticks == 37); world.Save("assembly-activating-425.sav");
				} else {
					REQUIRE(assembly->module != nullptr); REQUIRE(assembly->module->activation_ticks == 37);
				}
				world.Advance(37); REQUIRE(IsOrbitalModuleOperational(*assembly));
				CHECK(assembly->module->producer == UtilityProducerID{UINT32_MAX}); CHECK(assembly->module->consumer == UtilityConsumerID{UINT32_MAX});
				CHECK(GetOrbitalModuleStockLimit(SpaceBuildKind::Assembly) == 0); CHECK(GetOrbitalModuleServiceCapacity(SpaceBuildKind::Assembly) == 0);
				world.Save("assembly-active-425.sav");
			} else {
				REQUIRE(assembly != nullptr); REQUIRE(IsOrbitalModuleOperational(*assembly));
				if (action == "connect") {
					const auto state = ComputeSpaceVehicleDigest(), utility = ComputeUtilityPowerDigest();
					REQUIRE(CmdConnectOrbitalServices({}, assembly->index, assembly->generation, core.index, core.generation).Succeeded());
					CHECK(CmdConnectOrbitalServices({}, assembly->index, assembly->generation, core.index, core.generation + 1).Failed());
					CHECK(ComputeSpaceVehicleDigest() == state); CHECK(ComputeUtilityPowerDigest() == utility);
					REQUIRE(Command<Commands::ConnectOrbitalServices>::Post(assembly->index, assembly->generation, core.index, core.generation));
					CHECK(GetOrbitalServicePowerSite(*assembly) == &core);
					const auto *consumer = GetUtilityPowerConsumer(assembly->module->consumer); REQUIRE(consumer != nullptr);
					CHECK(consumer->capacity_j == 750); CHECK(consumer->linked_producers == std::vector{core.module->producer});
					world.Save("assembly-connected-425.sav");
					REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
					ShowSpaceConstructionWindow(GetSpaceStructureReference(*assembly)); auto *window = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(window != nullptr);
					CaptureNativeArtWindow(*window, paths.images / "assembly-connection.ppm");
				} else if (action == "company") {
					const auto original = _current_company; const auto reference = GetSpaceStructureReference(*assembly);
					const auto consumer = assembly->module->consumer; const auto producer = core.module->producer;
					const auto spent = GetUtilityPowerProducer(producer)->consumed_lifetime_j;
					const auto physical = CaptureSpaceStructures();
					REQUIRE(Company::CanAllocateItem()); auto *company = Company::Create(); SetCompanyManagerFaceStyle(company->face, 0);
					company->money = 10000000; const auto owner = company->index;
					ChangeOwnershipOfCompanyItems(original, owner); REQUIRE(ValidateSpaceVehicles());
					CHECK(assembly->state.owner == owner); CHECK(core.state.owner == owner); CHECK(GetSpaceStructureReference(*assembly) == reference);
					CHECK(assembly->module->consumer == consumer); CHECK(GetUtilityPowerConsumer(consumer)->owner == owner);
					CHECK(assembly->module->utility_source == GetSpaceStructureReference(core));
					CHECK(CmdDisconnectOrbitalServices({}, assembly->index, assembly->generation).Failed());
					const auto transferred = CaptureSpaceStructures(); REQUIRE(transferred.size() == physical.size());
					// Takeover moves actual native authority together; no inventory, consumed material or source identity may be reissued by ownership change.
					for (size_t index = 0; index < physical.size(); ++index) {
						CHECK(transferred[index].ref == physical[index].ref); CHECK(transferred[index].consumed == physical[index].consumed);
						CHECK(transferred[index].inputs == physical[index].inputs);
					}
					ChangeOwnershipOfCompanyItems(owner, INVALID_OWNER); delete Company::Get(owner);
					CHECK(assembly->state.owner == OWNER_NONE); CHECK_FALSE(GetUtilityPowerConsumer(consumer)->active); CHECK_FALSE(GetUtilityPowerProducer(producer)->active);
					REQUIRE(Company::CanAllocateItem()); auto *reused = Company::Create(); SetCompanyManagerFaceStyle(reused->face, 0); reused->money = 10000000;
					CHECK(reused->index == owner);
					{
						AutoRestoreBackup other(_current_company, reused->index);
						CHECK(CmdConnectOrbitalServices({}, assembly->index, assembly->generation, core.index, core.generation).Failed());
						CHECK(CmdRemoveOrbitalModule({}, assembly->index, assembly->generation).Failed()); CHECK_FALSE(GetUtilityAllocation(consumer));
					}
					CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == spent); CHECK_FALSE(IsOrbitalModuleOperational(*assembly));
					world.Save("assembly-company-425.sav");
					fmt::print("ASSEMBLY_FACILITY native=true phase=company takeover=true company_id_reuse_refused=true original_source=true\n");
					observed = true; _exit_game = true; return;
				} else if (action == "outage") {
					REQUIRE(GetOrbitalServicePowerSite(*assembly) == &core); const auto consumer = assembly->module->consumer;
					REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core.index, core.generation, true)); world.Advance(74);
					CHECK(GetOrbitalModulePanelRate(core) == 0); CHECK(assembly->module->consumer == consumer);
					const auto allocation = GetUtilityAllocation(consumer);
					CHECK((!allocation || allocation->granted_energy_j == 0));
					world.Save("assembly-unpowered-425.sav");
					REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core.index, core.generation, false)); world.Advance(1);
					CHECK(GetOrbitalModulePanelRate(core) == 30000); CHECK(assembly->module->consumer == consumer);
					world.Save("assembly-restored-425.sav");
				} else {
					REQUIRE(action == "disconnect"); const auto consumer = assembly->module->consumer;
					auto *storage = SpaceStructure::GetIfValid(SpaceStructureID{1}); REQUIRE(storage != nullptr);
					// This genuine Core retains stock, so inventory refusal precedes its connection guard. A dependent Storage may not be used to strand that stock; isolated empty-Core contracts cover the later connection reason.
					if (!storage->module) {
						REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(storage->index, storage->generation)); world.Advance(74);
					}
					REQUIRE(IsOrbitalModuleOperational(*storage));
					CHECK(ClassifyOrbitalModuleRemoval(core, _current_company) == OrbitalRemovalBlock::Inventory);
					const auto retained = ComputeSpaceStructureDigest();
					CHECK(CmdEvacuateOrbitalModule({}, core.index, core.generation, storage->index, storage->generation).Failed());
					CHECK(ComputeSpaceStructureDigest() == retained); CHECK(GetOrbitalServicePowerSite(*assembly) == &core);
					REQUIRE(Command<Commands::DisconnectOrbitalServices>::Post(assembly->index, assembly->generation));
					CHECK_FALSE(GetUtilityPowerConsumer(consumer)->active); CHECK(assembly->module->utility_source == SpaceStructureRef{});
					CHECK(ClassifyOrbitalModuleRemoval(*assembly, _current_company) == OrbitalRemovalBlock::Vehicle);
					auto *staging = SpaceStructure::GetIfValid(SpaceStructureID{2}); REQUIRE(staging != nullptr);
					SpaceVehicle *builder = nullptr;
					// Removal must wait for the actual occupant's departure; cancelling a request alone cannot erase its physical berth occupancy.
					for (auto *craft : world.builders) if (GetSpaceStructureBerth(*craft, *assembly)) builder = craft;
					REQUIRE(builder != nullptr); world.Order(*builder, *staging, SpaceOrderAction::Visit);
					world.Await([&] { return GetSpaceStructureBerth(*builder, *staging).has_value(); });
					const auto removed = GetSpaceStructureReference(*assembly); const auto history = GetSpaceStructureRetirementHistory();
					REQUIRE(Command<Commands::RemoveOrbitalModule>::Post(assembly->index, assembly->generation)); assembly = nullptr;
					CHECK(ResolveSpaceStructure(removed) == nullptr); CHECK(GetSpaceStructureRetirementHistory().consumed[0] == history.consumed[0] + 8);
					CHECK(GetSpaceStructureRetirementHistory().consumed[1] == history.consumed[1] + 16);
					world.Save("assembly-removed-425.sav");
				}
			}
			CHECK(GetSpaceVehicleCount() == hulls); CHECK(CaptureUtilityPowerState().producers.size() == sources);
			REQUIRE(ValidateSpaceVehicles());
			fmt::print("ASSEMBLY_FACILITY native=true phase={} paid_inputs=true no_free_vehicle=true original_source=true\n", action);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(assembly_observer, &observe); IConsole::CmdRegister("assembly_facility_test", ObserveAssemblyFacility);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
