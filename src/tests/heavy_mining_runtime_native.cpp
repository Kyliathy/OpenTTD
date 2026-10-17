/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_mining_runtime_native.cpp Actual paid heavy hull, ordinary mining saves and explicit orbital return.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9C
 * Consumes the immutable9B paid world; grants no cargo, shell, money, pose, work, allowance or utility account.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_mining_tuning.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_mining_runtime_observer = nullptr; ///< Process-local callback only; original native owners retain every physical asset.

/** @param args Parameter-free actual loaded-world console callback. @return Whether the installed observer executed. */
static bool ObserveHeavyMiningRuntime(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_mining_runtime_observer != nullptr); (*heavy_mining_runtime_observer)(); return true;
}

/** Execute one bounded actual mission milestone or independent copied-preview check over the original paid hull. */
TEST_CASE("Paid heavy hull mines through original native owners and ordinary saved missions", "[.heavy-mining-runtime-native]")
{
	const char *source = std::getenv("OTTD_HEAVY_MINING_SOURCE"), *phase_value = std::getenv("OTTD_HEAVY_MINING_PHASE"); REQUIRE(source != nullptr); REQUIRE(phase_value != nullptr);
	const std::string_view phase{phase_value}; const auto paths = GetTestCaseArtifactPaths("space", "heavy-mining-runtime-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe actual source owners and preserve assertion failures through the application's ordinary shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths); SpaceVehicle *craft = nullptr;
			// Select the original paid native role, then require its exact immutable9B incarnation; no newly allocated stand-in can satisfy this observer.
			for (auto *candidate : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*candidate) == SPACE_HEAVY_MINER_MODEL) { REQUIRE(craft == nullptr); craft = candidate; }
			REQUIRE(craft != nullptr); REQUIRE(craft->index == VehicleID{22}); REQUIRE(craft->generation == 8); REQUIRE(craft->manifest != nullptr);
			REQUIRE(craft->construction.content_version == 3); REQUIRE(craft->construction.placement == SpacePlacement::Orbital);
			const auto original = GetSpaceCraftReference(*craft); const auto fleet = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration();
			if (const char *corrupt = std::getenv("OTTD_HEAVY_MINING_CORRUPT")) {
				const auto vehicles = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
				const auto manifests = ComputeSpaceManifestDigest(), field = ComputeAsteroidFieldDigest(GetAsteroidField());
				const auto money = Company::Get(_current_company)->money; const auto random = _random;
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceVehicleDigest() == vehicles); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == power);
				CHECK(ComputeSpaceManifestDigest() == manifests); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field);
				CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("HEAVY_MINING preview_reject=true authority_unchanged=true\n");
			} else if (phase == "approach") {
				REQUIRE(craft->space_orders.entries.empty()); REQUIRE(craft->extracted_units == 0); REQUIRE(craft->manifest->returns_remaining == 2);
				const AsteroidBody *target = nullptr;
				// The closest actual metallic body supplies the mission identity; resource labels and current coordinates come from its sole finite owner.
				for (const auto &body : GetAsteroidField().bodies) {
					if (body.resource_class != AsteroidClass::Metallic) continue;
					if (target == nullptr || SpaceDistanceSquared(craft->position, body.motion.position) < SpaceDistanceSquared(craft->position, target->motion.position)) target = &body;
				}
				REQUIRE(target != nullptr);
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, target->id.base(), uint64_t{0}));
				CHECK(GetSpaceMiningBlock(*craft) == SpaceOrderBlock::PodsMissing);
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Insert, uint8_t{1}, SpaceOrderAction::Unload, SpaceOrderTargetKind::Structure, uint32_t{world.core->index.base()}, world.core->generation));
				CHECK(GetSpaceMiningBlock(*craft) == SpaceOrderBlock::None); world.Advance(1); CHECK(craft->mission.phase == SpaceMissionPhase::Approach);
			} else if (phase == "working") {
				/** @return Whether the actual original hull has extracted one heavy batch and earned a partial second interval. */
				world.Await([&] { return craft->mission.phase == SpaceMissionPhase::Mining && craft->cargo.TotalCount() == 4 && craft->mission.work_ticks == 37; }, 12000);
				CHECK(craft->extracted_units == 4); CHECK(craft->cargo_cap == 40); CHECK(craft->manifest->returns_remaining == 2);
				const auto *body = GetMutableAsteroidBody(craft->mission.target); REQUIRE(body != nullptr); CHECK(body->mining_slot == craft->index);
			} else if (phase == "blocked") {
				/** @return Whether the inherited physical Core occupant blocks the real return while the original whole40t hold remains aboard. */
				world.Await([&] { return craft->cargo.TotalCount() == 40 && craft->space_orders.cursor == 1 && craft->space_orders.blocked == SpaceOrderBlock::ApproachBusy; }, 12000);
				CHECK(craft->extracted_units == 40); CHECK(craft->manifest->returns_remaining == 2); CHECK(craft->mission.work_ticks == 0);
				CHECK(GetMutableAsteroidBody(craft->mission.target)->mining_slot == VehicleID::Invalid());
			} else if (phase == "returned") {
				REQUIRE(craft->space_orders.entries.size() == 2); const auto target = OrbitalBodyID{craft->space_orders.entries[0].target.id};
				const auto reserve = GetMutableAsteroidBody(target)->remaining_reserve; const auto extracted = craft->extracted_units;
				const auto cargo = GetCargoTypeByLabel(CargoLabel{"NORE"}); const auto stock = world.core->module->stock[cargo].TotalCount();
				uint departures = 0;
				// The immutable source's original builder physically occupies the Core. Move that actual owned hull by normal typed Visit, preserving its freight and identity; a pose edit or berth override would bypass the producer under test.
				for (auto *occupant : SpaceVehicle::Iterate()) {
					if (occupant == craft || occupant->owner != craft->owner || occupant->construction.placement != SpacePlacement::Orbital || occupant->position != GetSpaceStructureApproach(world.core->state)) continue;
					const uint32_t destination = (target.base() + 1) % GetAsteroidField().bodies.size();
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(occupant->index, occupant->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(occupant->index, occupant->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, destination, uint64_t{0}));
					++departures;
				}
				REQUIRE(departures != 0);
				/** @return Whether the real typed Unload transferred the whole heavy hold through its actual admitted module berth. */
				world.Await([&] { return craft->space_orders.cursor == craft->space_orders.entries.size(); }, 12000);
				CHECK(craft->cargo.TotalCount() == 0); CHECK(craft->extracted_units == 40); CHECK(craft->manifest->returns_remaining == 1);
				CHECK(world.core->module->stock[cargo].TotalCount() == stock + 40);
				CHECK(GetMutableAsteroidBody(target)->remaining_reserve == reserve - (40 - extracted)); CHECK(GetMutableAsteroidBody(target)->mining_slot == VehicleID::Invalid());
			} else {
				REQUIRE(phase == "resave"); // Ordinary reconstruction preserves exact original native owners without advancing work or issuing a command.
			}
			CHECK(GetSpaceCraftReference(*craft) == original); CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == generation);
			CHECK(craft->manifest->services_completed == 0); CHECK(craft->manifest->pending_returns == 0); CHECK(craft->manifest->pods.empty());
			CHECK(craft->manifest->supplies[1].TotalCount() == 0); CHECK(craft->manifest->supplies[2].TotalCount() == 0);
			if (std::getenv("OTTD_HEAVY_MINING_CORRUPT") == nullptr) world.Save(fmt::format("heavy-mining-{}-432.sav", phase));
			fmt::print("HEAVY_MINING native=true phase={} vehicle={} generation={} extracted={} held={} returns={} work={}\n", phase,
				craft->index.base(), craft->generation, craft->extracted_units, craft->cargo.TotalCount(), craft->manifest->returns_remaining, craft->mission.work_ticks);
			observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(heavy_mining_runtime_observer, &observe); IConsole::CmdRegister("heavy_mining_runtime_test", ObserveHeavyMiningRuntime);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
