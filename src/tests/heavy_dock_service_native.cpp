/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_dock_service_native.cpp Original paid rocket and builders construct a real Dock before heavy service.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC
 * All orbital owners and material come from the exact recovered heavy world. A separately saved Surface fuel fixture, if needed, claims no procurement acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../map_func.h"
#include "../orbital_service.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_dock_observer = nullptr; ///< Sole invocation-owned observer; native objects retain all gameplay authority.

/** @param args Parameter-free callback in the actually loaded world. @return Whether the installed observer ran. */
static bool ObserveHeavyDockService(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_dock_observer != nullptr); (*heavy_dock_observer)(); return true;
}

/** @return Complete native physical/work authority used to prove estimate and detached-preview purity. */
static auto HeavyDockAuthority()
{
	return std::array<uint64_t, 11>{ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest(), ComputeUtilityPowerDigest(), ComputeSpaceManifestDigest(),
		ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(), ComputeSpaceOrdersDigest(), ComputeAsteroidFieldDigest(GetAsteroidField()),
		ComputeLaunchProductionDigest(), ComputeLaunchCenterDigest(), ComputeSpaceDescentDigest()};
}

/** Original material flights, actual construction and explicit activation/connection must precede any new Dock service rights. */
TEST_CASE("Original recovered heavy world builds a paid Dock from actual Surface deliveries", "[.heavy-dock-service-native]")
{
	const char *source = std::getenv("OTTD_HEAVY_DOCK_SOURCE"), *selected = std::getenv("OTTD_HEAVY_DOCK_PHASE"); REQUIRE(source != nullptr); REQUIRE(selected != nullptr);
	const std::string_view phase{selected}; const auto paths = GetTestCaseArtifactPaths("space", "heavy-dock-service-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve actual loaded-world assertions through normal application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			auto *heavy = ResolveSpaceCraft({VehicleID{22}, 8}), *builder = ResolveSpaceCraft({VehicleID{18}, 4}); REQUIRE(heavy != nullptr); REQUIRE(builder != nullptr);
			REQUIRE(GetSpaceVehicleModel(*heavy) == 21); REQUIRE(heavy->manifest != nullptr); REQUIRE(builder->manifest != nullptr);
			const auto fleet = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration(); const auto producers = CaptureUtilityPowerState().producers.size();
			auto *center = ResolveLaunchCenter(world.rocket->construction.home); REQUIRE(center != nullptr); REQUIRE(world.rocket->manifest != nullptr);
			const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}), propellant = GetSpaceSupplyCargo(SpaceSupply::Propellant);
			REQUIRE(IsValidCargoType(steel)); REQUIRE(IsValidCargoType(goods)); REQUIRE(IsValidCargoType(propellant));
			SpaceStructure *dock = nullptr;
			// The sole newly paid Dock must be selected from the actual native catalogue; an invented frame or copied pool cannot supply its construction receipt.
			for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Dock) { REQUIRE(dock == nullptr); dock = site; }
			if (const char *corrupt = std::getenv("OTTD_HEAVY_DOCK_CORRUPT")) {
				const auto authority = HeavyDockAuthority(); const auto money = Company::Get(_current_company)->money; const auto random = _random;
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(HeavyDockAuthority() == authority); CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("HEAVY_DOCK preview_reject=true authority_unchanged=true\n");
			} else if (phase == "inspected") {
				REQUIRE(dock == nullptr); REQUIRE(world.rocket->construction.placement == SpacePlacement::SurfaceStored);
				fmt::print("HEAVY_DOCK stock steel={} goods={} surface_fuel={} rocket_fuel={}\n", center->goods[steel].AvailableCount(),
					center->goods[goods].AvailableCount(), center->goods[propellant].AvailableCount(), world.rocket->manifest->supplies[0].TotalCount());
			} else if (phase == "starting-fuel") {
				REQUIRE(dock == nullptr); REQUIRE(world.rocket->manifest->supplies[0].TotalCount() == 0);
				auto &stock = center->goods[propellant].GetOrCreateData().cargo; const auto &budget = GetMultiLayerSpaceBudget();
				const uint required = 2 * (uint{budget.outbound_fuel} + budget.recovery_fuel), available = stock.AvailableCount(); const auto authority = HeavyDockAuthority();
				const auto money = Company::Get(_current_company)->money; uint missing = 0;
				// Only the shortfall for two genuine Surface material flights is a declared starting-input fixture; no orbital material, work, account or hull is granted.
				if (available < required) { missing = required - available; REQUIRE(CargoPacket::CanAllocateItem()); stock.Append(CargoPacket::Create(center->index, missing, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid()); }
				CHECK(stock.AvailableCount() >= required); CHECK(HeavyDockAuthority() == authority); CHECK(Company::Get(_current_company)->money == money);
				fmt::print("HEAVY_DOCK starting_fuel_fixture=true added={} retained={} procurement_acceptance=false\n", missing, available);
			} else if (phase == "blueprint") {
				REQUIRE(dock == nullptr); auto *unused = ResolveSpaceStructure({SpaceStructureID{2}, 3}); REQUIRE(unused != nullptr);
				REQUIRE(unused->job.work_ticks == 0); REQUIRE(unused->job.inputs[0].TotalCount() == 0); REQUIRE(unused->job.inputs[1].TotalCount() == 0);
				const auto tile = TileXYL(2, 4, MapLayer::Space); CHECK(CmdBuildSpaceBlueprint({}, tile, SpaceBuildKind::Dock).Failed());
				world.Order(*builder, *world.core, SpaceOrderAction::Visit);
				/** @return Whether the actual original builder has left the retained blueprint footprint and reached the Core. */
				world.Await([&] { return GetSpaceStructureBerth(*builder, *world.core).has_value(); });
				const auto capital = unused->state.capital_paid; const auto cash = Company::Get(_current_company)->money;
				REQUIRE(Command<Commands::CancelSpaceBlueprint>::Post(unused->index, unused->generation)); CHECK(unused->state.capital_paid == capital); CHECK(Company::Get(_current_company)->money == cash);
				const auto authority = HeavyDockAuthority(); REQUIRE(CmdBuildSpaceBlueprint({}, tile, SpaceBuildKind::Dock).Succeeded()); CHECK(HeavyDockAuthority() == authority);
				REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(tile, SpaceBuildKind::Dock)); dock = ResolveSpaceStructure({SpaceStructureID{4}, 5}); REQUIRE(dock != nullptr);
				CHECK(Company::Get(_current_company)->money == cash - GetSpaceConstructionDefinition(SpaceBuildKind::Dock)->capital);
				CHECK(dock->state.phase == SpaceStructurePhase::Blueprint); CHECK(dock->module == nullptr); CHECK(dock->job.work_ticks == 0);
			} else if (phase == "steel") {
				REQUIRE(dock != nullptr); REQUIRE(dock->job.work_ticks == 0); const auto missions = world.rocket->transition->missions_completed;
				world.Import(*builder, steel, 8); CHECK(world.rocket->transition->missions_completed == missions + 1); world.Order(*builder, *dock, SpaceOrderAction::Construct);
				/** @return Whether the original builder has physically delivered the complete Steel input to the paid job. */
				world.Await([&] { return dock->job.inputs[0].TotalCount() == 8; });
				CHECK(dock->job.inputs[1].TotalCount() == 0); CHECK(dock->job.work_ticks == 0); CHECK(dock->module == nullptr);
			} else if (phase == "goods") {
				REQUIRE(dock != nullptr); REQUIRE(dock->job.inputs[0].TotalCount() == 8); const auto missions = world.rocket->transition->missions_completed;
				world.Import(*builder, goods, 12); CHECK(world.rocket->transition->missions_completed == missions + 1); world.Order(*builder, *dock, SpaceOrderAction::Construct);
				/** @return Whether actual common allocation has earned the chosen halfway construction checkpoint. */
				world.Await([&] { return dock->job.work_ticks == 222; });
				CHECK(dock->job.inputs[0].TotalCount() == 8); CHECK(dock->job.inputs[1].TotalCount() == 12); CHECK(dock->module == nullptr);
			} else if (phase == "frame") {
				REQUIRE(dock != nullptr); REQUIRE(dock->job.work_ticks == 222);
				/** @return Whether this original job consumed its complete recipe through normal shared construction work. */
				world.Await([&] { return dock->state.phase == SpaceStructurePhase::FrameComplete; });
				CHECK(dock->job.work_ticks == 444); CHECK((dock->job.consumed == std::array<uint16_t, 2>{8, 12})); CHECK(dock->module == nullptr);
				CHECK(dock->job.inputs[0].TotalCount() == 0); CHECK(dock->job.inputs[1].TotalCount() == 0);
			} else if (phase == "activating") {
				REQUIRE(dock != nullptr); REQUIRE(dock->module == nullptr); const auto authority = HeavyDockAuthority();
				REQUIRE(CmdActivateOrbitalModule({}, dock->index, dock->generation).Succeeded()); CHECK(HeavyDockAuthority() == authority);
				REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(dock->index, dock->generation)); world.Advance(37); REQUIRE(dock->module != nullptr); CHECK(dock->module->activation_ticks == 37);
				CHECK_FALSE(IsOrbitalModuleOperational(*dock)); CHECK(dock->module->producer == UtilityProducerID{UINT32_MAX});
			} else if (phase == "ready") {
				REQUIRE(dock != nullptr); REQUIRE(dock->module != nullptr); REQUIRE(dock->module->activation_ticks == 37); world.Advance(37); REQUIRE(IsOrbitalModuleOperational(*dock));
				const auto authority = HeavyDockAuthority(); REQUIRE(CmdConnectOrbitalServices({}, dock->index, dock->generation, world.core->index, world.core->generation).Succeeded()); CHECK(HeavyDockAuthority() == authority);
				REQUIRE(Command<Commands::ConnectOrbitalServices>::Post(dock->index, dock->generation, world.core->index, world.core->generation));
				CHECK(GetOrbitalServicePowerSite(*dock) == world.core); CHECK(GetSpaceStructureApproachCapacity(*dock) == 2); CHECK(dock->module->producer == UtilityProducerID{UINT32_MAX});
				const auto *consumer = GetUtilityPowerConsumer(dock->module->consumer); REQUIRE(consumer != nullptr); CHECK(consumer->capacity_j == 1500);
				CHECK(consumer->linked_producers == std::vector{world.core->module->producer});
			} else {
				REQUIRE(phase == "resave"); // Ordinary reconstruction cannot create infrastructure, input or service work.
			}
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(CaptureUtilityPowerState().producers.size() == producers);
			CHECK(heavy->manifest->returns_remaining == 0); CHECK(heavy->manifest->services_completed == 0); CHECK(heavy->manifest->supplies[1].TotalCount() == 2); CHECK(heavy->manifest->supplies[2].TotalCount() == 2);
			if (std::getenv("OTTD_HEAVY_DOCK_CORRUPT") == nullptr) world.Save(fmt::format("heavy-dock-{}-436.sav", phase));
			fmt::print("HEAVY_DOCK native=true phase={} dock={} procurement_acceptance=false\n", phase, dock == nullptr ? UINT16_MAX : dock->index.base()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(heavy_dock_observer, &observe); IConsole::CmdRegister("heavy_dock_test", ObserveHeavyDockService); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
