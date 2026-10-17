/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_dock_work_native.cpp Original heavy and construction probes share both berths of their newly paid Dock.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC
 * Actual paid predecessors supply every hull, part, panel and allowance. Native commands, shared approach admission and utility mutation alone perform work.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../orbital_service.h"
#include "../space_pod.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_dock_work_observer = nullptr; ///< Sole invocation callback; no copied owner or private service registry exists.

/** @param args Parameter-free callback in the actual loaded world. @return Whether the installed work observer ran. */
static bool ObserveHeavyDockWork(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_dock_work_observer != nullptr); (*heavy_dock_work_observer)(); return true;
}

/** @return Original complete physical/work authority for mixed-query and copied-preview purity. */
static auto HeavyDockWorkAuthority()
{
	return std::array<uint64_t, 11>{ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest(), ComputeUtilityPowerDigest(), ComputeSpaceManifestDigest(),
		ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(), ComputeSpaceOrdersDigest(), ComputeAsteroidFieldDigest(GetAsteroidField()),
		ComputeLaunchProductionDigest(), ComputeLaunchCenterDigest(), ComputeSpaceDescentDigest()};
}

/** Both physical role orderings earn bounded shared work, retain original inputs through interruptions and complete each immutable bill once. */
TEST_CASE("Actual paid Dock services original heavy and legacy roles in both shared berth orderings", "[.heavy-dock-work-native]")
{
	const char *source = std::getenv("OTTD_HEAVY_DOCK_SOURCE"), *selected = std::getenv("OTTD_HEAVY_DOCK_PHASE"); REQUIRE(source != nullptr); REQUIRE(selected != nullptr);
	const std::string_view phase{selected}; const auto paths = GetTestCaseArtifactPaths("space", "heavy-dock-work-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Retain actual-world assertions through normal application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			auto *heavy = ResolveSpaceCraft({VehicleID{22}, 8}), *builder = ResolveSpaceCraft({VehicleID{18}, 4}); REQUIRE(heavy != nullptr); REQUIRE(builder != nullptr);
			auto *dock = ResolveSpaceStructure({SpaceStructureID{4}, 5}); REQUIRE(dock != nullptr); REQUIRE(IsOrbitalModuleOperational(*dock)); REQUIRE(GetOrbitalServicePowerSite(*dock) == world.core);
			REQUIRE(heavy->manifest != nullptr); REQUIRE(builder->manifest != nullptr); REQUIRE(GetSpaceVehicleModel(*heavy) == 21); REQUIRE(GetSpaceVehicleModel(*builder) == 20);
			const auto fleet = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration(); const auto producers = CaptureUtilityPowerState().producers.size();
			bool heavy_first = phase.starts_with("heavy-"); const auto dash = phase.find('-'); const auto action = dash == std::string_view::npos ? phase : phase.substr(dash + 1);
			if (phase == "corrupt") { const auto berth = GetSpaceStructureBerth(*heavy, *dock); REQUIRE(berth); heavy_first = *berth == 0; }
			auto *first_hull = heavy_first ? heavy : builder, *second_hull = heavy_first ? builder : heavy;
			auto &first = dock->module->service; auto &second = dock->module->dock_service;
			const auto first_tuning = *GetSpaceServiceTuning(GetSpaceVehicleModel(*first_hull)), second_tuning = *GetSpaceServiceTuning(GetSpaceVehicleModel(*second_hull));
			/** @return Actual original Core source settlement, never a copied power balance. */
			auto used = [&] { return GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j; };
			/** @param job Original contained job. @param control Explicit operation on that exact retained hull incarnation. */
			auto control = [&](const OrbitalServiceJob &job, OrbitalServiceControl control) {
				REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(dock->index, dock->generation, job.craft.vehicle, job.craft.generation, control));
			};
			if (const char *corrupt = std::getenv("OTTD_HEAVY_DOCK_CORRUPT")) {
				const auto authority = HeavyDockWorkAuthority(); const auto money = Company::Get(_current_company)->money; const auto random = _random;
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(HeavyDockWorkAuthority() == authority); CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("HEAVY_DOCK preview_reject=true authority_unchanged=true\n");
			} else if (action == "prepared") {
				REQUIRE(first.remaining_ticks == 0); REQUIRE(second.remaining_ticks == 0); REQUIRE(heavy->manifest->returns_remaining == 0); REQUIRE(heavy->manifest->services_completed == 0); REQUIRE(builder->manifest->services_completed == 0);
				world.Order(*builder, *world.core, SpaceOrderAction::Visit);
				/** @return Whether the original builder actually reached its original Core before loading its separate existing one-part bill. */
				world.Await([&] { return GetSpaceStructureBerth(*builder, *world.core).has_value(); });
				// Only the Core's original one-part pair is loaded; embodied construction materials and the heavy's loose bill cannot finance this second job.
				for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveOrbitalSupply>::Post(world.core->index, world.core->generation, builder->index, builder->generation, supply, uint16_t{1}, true));
				std::array occupants{first_hull, second_hull};
				// Each role must earn its own precise physical slot and unload its actual complete bill before either job starts; travel itself grants no service progress.
				for (uint8_t slot = 0; slot < occupants.size(); ++slot) {
					auto &craft = *occupants[slot]; const auto tuning = *GetSpaceServiceTuning(GetSpaceVehicleModel(craft)); world.Order(craft, *dock, SpaceOrderAction::Visit);
					/** @return Whether this original hull reached the exact next shared Dock berth through native arbitration. */
					world.Await([&] { return GetSpaceStructureBerth(craft, *dock) == std::optional<uint8_t>{slot}; });
					for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveOrbitalSupply>::Post(dock->index, dock->generation, craft.index, craft.generation, supply, uint16_t{tuning.inputs[supply - 1]}, false));
				}
				CHECK(dock->module->stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount() == 3); CHECK(dock->module->stock[GetSpaceSupplyCargo(SpaceSupply::Electronics)].TotalCount() == 3);
				const auto authority = HeavyDockWorkAuthority(); const auto money = Company::Get(_current_company)->money;
				// Successful mixed queries may price both real complete jobs but cannot reserve either bill or earn an allocation interval.
				for (auto *craft : occupants) CHECK(CmdServiceOrbitalProbe({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, dock->index, dock->generation, craft->index, craft->generation, OrbitalServiceControl::Start).Succeeded());
				CHECK(HeavyDockWorkAuthority() == authority); CHECK(Company::Get(_current_company)->money == money);
				for (auto *craft : occupants) REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(dock->index, dock->generation, craft->index, craft->generation, OrbitalServiceControl::Start));
				CHECK(first.remaining_ticks == first_tuning.ticks); CHECK(second.remaining_ticks == second_tuning.ticks); CHECK(first.last_work_interval == 0); CHECK(second.last_work_interval == 0);
				CHECK(first.craft == GetSpaceCraftReference(*first_hull)); CHECK(second.craft == GetSpaceCraftReference(*second_hull));
				CHECK(CmdDisconnectOrbitalServices({}, dock->index, dock->generation).Failed());
				CHECK(CmdEditSpaceOrder({}, heavy->index, heavy->generation, SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Failed());
			} else if (action == "partial") {
				REQUIRE(first.remaining_ticks == first_tuning.ticks); REQUIRE(second.remaining_ticks == second_tuning.ticks); const auto before = used(); world.Advance(37);
				CHECK(first.remaining_ticks == first_tuning.ticks - 37); CHECK(second.remaining_ticks == second_tuning.ticks); CHECK(used() == before + 27750);
			} else if (action == "paused") {
				REQUIRE(first.remaining_ticks == first_tuning.ticks - 37); REQUIRE(second.remaining_ticks == second_tuning.ticks); control(first, OrbitalServiceControl::Pause); const auto before = used(); world.Advance(37);
				CHECK(first.remaining_ticks == first_tuning.ticks - 37); CHECK(second.remaining_ticks == second_tuning.ticks - 37); CHECK(first.paused); CHECK(used() == before + 27750);
			} else if (action == "outage") {
				REQUIRE(first.paused); control(first, OrbitalServiceControl::Resume); REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(world.core->index, world.core->generation, true)); const auto before = used(); world.Advance(9);
				CHECK(first.remaining_ticks == first_tuning.ticks - 37); CHECK(second.remaining_ticks == second_tuning.ticks - 37); CHECK(used() == before); CHECK(GetSpaceStructureApproachCapacity(*dock) == 2);
			} else if (action == "restored") {
				REQUIRE_FALSE(world.core->module->power_enabled); REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(world.core->index, world.core->generation, false)); const auto before = used(); world.Advance(17);
				CHECK(first.remaining_ticks == first_tuning.ticks - 54); CHECK(second.remaining_ticks == second_tuning.ticks - 37); CHECK(used() == before + 12750);
			} else if (action == "first-last") {
				const uint remaining = first.remaining_ticks; REQUIRE(remaining > 1); const auto before = used(); const auto other = second.remaining_ticks; world.Advance(remaining - 1);
				CHECK(first.remaining_ticks == 1); CHECK(second.remaining_ticks == other); CHECK(used() == before + uint64_t{remaining - 1} * 750);
			} else if (action == "first-complete") {
				REQUIRE(first.remaining_ticks == 1); const auto before = used(); const auto other = second.remaining_ticks; world.Advance(1);
				CHECK(first.remaining_ticks == 0); CHECK(second.remaining_ticks == other); CHECK(first_hull->manifest->services_completed == 1); CHECK(first_hull->manifest->returns_remaining == 2); CHECK(dock->module->services_completed == 1);
				world.Advance(9); CHECK(first_hull->manifest->services_completed == 1); CHECK(second.remaining_ticks == other - 9); CHECK(used() == before + 7500);
			} else if (action == "second-last") {
				REQUIRE(first.remaining_ticks == 0); const uint remaining = second.remaining_ticks; REQUIRE(remaining > 1); const auto before = used(); world.Advance(remaining - 1);
				CHECK(second.remaining_ticks == 1); CHECK(first_hull->manifest->services_completed == 1); CHECK(used() == before + uint64_t{remaining - 1} * 750);
			} else if (action == "complete") {
				REQUIRE(first.remaining_ticks == 0); REQUIRE(second.remaining_ticks == 1); const auto before = used(); world.Advance(1);
				CHECK(second.remaining_ticks == 0); CHECK(used() == before + 750); CHECK(dock->module->services_completed == 2);
				world.Advance(10); CHECK(used() == before + 750); CHECK(heavy->manifest->services_completed == 1); CHECK(builder->manifest->services_completed == 1);
				CHECK(heavy->manifest->returns_remaining == 2); CHECK(builder->manifest->returns_remaining == 2); CHECK(dock->module->services_completed == 2);
			} else if (action == "cancelled") {
				REQUIRE(first.paused); REQUIRE(first.remaining_ticks == first_tuning.ticks - 37); REQUIRE(second.remaining_ticks == second_tuning.ticks - 37); const auto before = used();
				control(first, OrbitalServiceControl::Cancel); control(second, OrbitalServiceControl::Cancel); world.Advance(9); CHECK(used() == before);
				CHECK(first.remaining_ticks == 0); CHECK(second.remaining_ticks == 0); CHECK(dock->module->services_completed == 0);
				CHECK(dock->module->stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount() == 3); CHECK(dock->module->stock[GetSpaceSupplyCargo(SpaceSupply::Electronics)].TotalCount() == 3);
				CHECK(heavy->manifest->returns_remaining == 0); CHECK(heavy->manifest->services_completed == 0); CHECK(builder->manifest->services_completed == 0);
			} else {
				REQUIRE(phase == "corrupt");
			}
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(CaptureUtilityPowerState().producers.size() == producers);
			CHECK(GetSpaceStructureBerth(*heavy, *dock) == std::optional<uint8_t>{heavy_first ? uint8_t{0} : uint8_t{1}});
			CHECK(GetSpaceStructureBerth(*builder, *dock) == std::optional<uint8_t>{heavy_first ? uint8_t{1} : uint8_t{0}});
			CHECK(heavy->manifest->pending_returns == 0); CHECK(heavy->manifest->service_ticks == 0); CHECK(builder->manifest->service_ticks == 0);
			// Every still-active berth owns its complete original physical bill; completion/cancellation leaves no reusable role, timer or hidden reserved part.
			for (const auto *job : dock->module->GetServiceJobs()) {
				const auto tuning = GetOrbitalServiceJobTuning(*job); REQUIRE(tuning);
				for (uint i = 0; i < 2; ++i) CHECK(job->inputs[i].TotalCount() == (job->remaining_ticks == 0 ? 0 : tuning->inputs[i]));
				if (job->remaining_ticks == 0) CHECK(job->model == UINT16_MAX);
			}
			// Handling moved only the original two bills; neither an active job nor cancellation may refill either hull or the original Core.
			for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) {
				CHECK(heavy->manifest->supplies[supply].TotalCount() == 0); CHECK(builder->manifest->supplies[supply].TotalCount() == 0);
				CHECK(world.core->module->stock[GetSpaceSupplyCargo(static_cast<SpaceSupply>(supply))].TotalCount() == 0);
			}
			if (std::getenv("OTTD_HEAVY_DOCK_CORRUPT") == nullptr) world.Save(fmt::format("heavy-dock-{}-436.sav", phase));
			fmt::print("HEAVY_DOCK native=true phase={} first_role={} first_work={} second_work={} source_use={}\n", phase, GetSpaceVehicleModel(*first_hull), first.remaining_ticks, second.remaining_ticks, used()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(heavy_dock_work_observer, &observe); IConsole::CmdRegister("heavy_dock_test", ObserveHeavyDockWork); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
