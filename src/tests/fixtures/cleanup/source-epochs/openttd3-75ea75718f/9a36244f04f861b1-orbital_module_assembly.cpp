/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_module_assembly.cpp Exact assembly grants, saved milestones and native cancellation from genuine paid stock.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group3.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../launch_center_cmd.h"
#include "../asteroid.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../map_func.h"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

/** Verify exact whole-joule work conservation across every possible partial canonical grant. */
TEST_CASE("Orbital assembly preserves partial grant work and caps useful final energy", "[orbital-assembly-contract]")
{
	const auto unit = GetMultiLayerSpaceBudget().work_j_per_tick; REQUIRE(unit == 750);
	// Every split of one real work unit must rejoin exactly; truncating its fractional remainder would lose paid work.
	for (uint16_t grant = 1; grant < unit; ++grant) {
		const auto partial = PlanOrbitalConstructionProgress({}, 2, grant); REQUIRE(partial.has_value());
		CHECK(GetOrbitalConstructionRemainingEnergy(*partial, 2) == uint64_t{1500} - grant);
		const auto joined = PlanOrbitalConstructionProgress(*partial, 2, static_cast<uint16_t>(unit - grant)); REQUIRE(joined.has_value());
		CHECK(((*joined) == OrbitalConstructionProgress{1, 0, 0}));
	}
	const auto partial = PlanOrbitalConstructionProgress({295, 0, 0}, 296, 720); REQUIRE(partial.has_value());
	CHECK(partial->work_ticks == 295); CHECK(partial->work_fraction == 62914); CHECK(partial->work_remainder == 420);
	CHECK(GetOrbitalConstructionRemainingEnergy(*partial, 296) == 30);
	CHECK_FALSE(PlanOrbitalConstructionProgress(*partial, 296, 31));
	const auto complete = PlanOrbitalConstructionProgress(*partial, 296, 30); REQUIRE(complete.has_value());
	CHECK(((*complete) == OrbitalConstructionProgress{296, 0, 0})); CHECK_FALSE(PlanOrbitalConstructionProgress(*complete, 296, 1));
	CHECK_FALSE(PlanOrbitalConstructionProgress({}, 296, 0)); CHECK_FALSE(PlanOrbitalConstructionProgress({}, 296, 751));
	CHECK_FALSE(GetOrbitalConstructionRemainingEnergy({1, 1, 0}, 296)); CHECK_FALSE(GetOrbitalConstructionRemainingEnergy({1, 0, 750}, 296));
}

/** Verify copied physical work receipts independently of live clocks, cargo pools or a utility account. */
TEST_CASE("Orbital assembly copied milestones bind exact work and the same file clock", "[orbital-assembly-contract]")
{
	SpaceStructureRecord row; row.ref = {SpaceStructureID{1}, 2}; row.state.kind = SpaceBuildKind::Storage; row.state.owner = Owner{0};
	row.state.x = 7; row.state.y = 4; row.state.capital_paid = 120000; row.quantities = {8, 8};
	row.inputs[0] = {{CargoPacketID{1}, StationID::Invalid()}}; row.inputs[1] = {{CargoPacketID{2}, StationID::Invalid()}};
	static_cast<OrbitalConstructionProgress &>(row) = *PlanOrbitalConstructionProgress({}, 296, 60); row.last_work_tick = 100; row.last_work_interval = 7;
	CHECK(ValidateSpaceStructureRecords(std::array{row}, 3, 64, 64, 100));
	CHECK_FALSE(ValidateSpaceStructureRecords(std::array{row}, 3, 64, 64, 99));
	auto bad = row; ++bad.work_fraction; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{bad}, 3, 64, 64, 100));
	bad = row; bad.last_work_tick = 0; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{bad}, 3, 64, 64, 100));
	bad = row; bad.last_work_interval = 0; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{bad}, 3, 64, 64, 100));
	bad = row; bad.quantities[0] = 7; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{bad}, 3, 64, 64, 100));
	UtilityPowerState power; power.interval = 6; CHECK_FALSE(ValidateOrbitalServiceUtilityRecords(std::array{row}, power));
	power.interval = 7; CHECK(ValidateOrbitalServiceUtilityRecords(std::array{row}, power));
	row.state.phase = SpaceStructurePhase::Cancelled; row.quantities = {}; row.inputs = {};
	CHECK(ValidateSpaceStructureRecords(std::array{row}, 3, 64, 64, 100)); // Physical collection retains spent work without recreating cargo or energy.
}

static std::function<void()> *assembly_observer = nullptr;
/** @param args Native console invocation. @return Whether the installed loaded-world assembly observer ran. */
static bool ObserveAssembly(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(assembly_observer != nullptr); (*assembly_observer)(); return true;
}

/** Exercise actual service/assembly competition for810J, exact30J completion and physical return of cancelled inputs. */
TEST_CASE("A delivered module assembles against its actual Core panel exactly once", "[.orbital-module-assembly]")
{
	const char *mode = std::getenv("OTTD_ORBITAL_ASSEMBLY_ACTION"), *input = std::getenv("OTTD_ORBITAL_ASSEMBLY_SOURCE");
	REQUIRE(mode != nullptr); REQUIRE(input != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "module-assembly"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Run only against the loaded actual paid world; preserve observer failures across normal shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto &core = *SpaceStructure::Get(SpaceStructureID{0}); auto &site = *SpaceStructure::Get(SpaceStructureID{1});
			REQUIRE(IsOrbitalModuleOperational(core)); REQUIRE(site.state.kind == SpaceBuildKind::Storage); REQUIRE(site.state.phase == SpaceStructurePhase::Blueprint);
			REQUIRE(site.job.inputs[0].TotalCount() == 8); REQUIRE(site.job.inputs[1].TotalCount() == 8);
			SpaceVehicle *builder = nullptr, *other = nullptr, *rocket = nullptr;
			// Resolve the two real paid builders in stable native order; the second occupies the independent Core service berth.
			for (auto *craft : SpaceVehicle::Iterate()) {
				if (GetSpaceVehicleModel(*craft) == 14) rocket = craft;
				if (GetSpaceVehicleModel(*craft) != 20) continue;
				if (builder == nullptr) builder = craft;
				else other = craft;
			}
			REQUIRE(builder != nullptr); REQUIRE(other != nullptr); REQUIRE(rocket != nullptr); auto *company = Company::Get(_current_company);
			const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
			/** @param ticks Actual synchronized native opportunities; rendering/fast-forward yields earn no work. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Count the real simulation controller, preserving bounded waiting without advancing a second assembly clock.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			/** @param done Actual physical continuation predicate; no simulated position or progress is injected. */
			auto until = [&](auto done) {
				// A bounded real approach/work wait must end through the native controllers themselves.
				for (uint elapsed = 0; !done(); ++elapsed) { REQUIRE(elapsed < 1500); advance(1); }
			};
			/** @param name Isolated native checkpoint name. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			/** @return Original actual panel's lifetime settlement, re-queried after each native mutation. */
			auto used = [&] { return GetUtilityPowerProducer(core.module->producer)->consumed_lifetime_j; };
			/** @param control Explicit operation on the second actual builder's precise service berth. */
			auto service = [&](OrbitalServiceControl control) {
				REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(core.index, core.generation, other->index, other->generation, control));
			};
			/** @param craft Actual builder. @param target Exact owned physical site. @param action Construct or Visit; uses only normal native queue edits. */
			auto order = [&](SpaceVehicle &craft, const SpaceStructure &target, SpaceOrderAction action) {
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, action,
					action == SpaceOrderAction::Construct ? SpaceOrderTargetKind::Blueprint : SpaceOrderTargetKind::Structure, uint32_t{target.index.base()}, target.generation));
			};
			if (const char *corrupt = std::getenv("OTTD_ORBITAL_ASSEMBLY_CORRUPT")) {
				const auto structure = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest(); const auto cash = company->money; const auto random = _random;
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceStructureDigest() == structure); CHECK(ComputeUtilityPowerDigest() == power); CHECK(company->money == cash);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("ORBITAL_ASSEMBLY preview_rejected=true authority_unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "prepare") {
				REQUIRE(site.job.work_ticks == 0); REQUIRE(core.module->assembly_consumer == UtilityConsumerID{UINT32_MAX}); REQUIRE(used() == 111000);
				const auto home = rocket->construction.home; const auto missions = rocket->transition->missions_completed;
				// A further actual paid resupply imports the service parts; this creates the real competing750J berth, not a synthetic utility sink.
				for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, supply, uint16_t{1}, false));
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, uint32_t{other->index.base()}, other->generation));
				advance(667); CHECK(rocket->transition->missions_completed == missions + 1);
				// Move both delivered compartments into actual Core stock for the independent berth's later all-input reservation.
				for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveOrbitalSupply>::Post(core.index, core.generation, other->index, other->generation, supply, uint16_t{1}, false));
				order(*builder, site, SpaceOrderAction::Construct); until([&] { return site.job.work_ticks == 37; });
				CHECK(used() == 138750); CHECK(core.module->assembly_consumer != UtilityConsumerID{UINT32_MAX}); CHECK(core.module->assembly_consumer != core.module->consumer);
				CHECK(site.job.work_fraction == 0); CHECK(site.job.work_remainder == 0); save("module-assembly-work-418.sav");
			}
			// Earlier checkpoints still need actual berth competition; later ones already contain its paid fractional work receipt.
			if (action == "prepare" || action == "work") {
				service(OrbitalServiceControl::Start); const auto profit = builder->profit_this_year; advance(37); service(OrbitalServiceControl::Pause);
				CHECK(builder->profit_this_year == profit - Money{250} * 256); // Positive fractional assembly uses active upkeep for the actual working duration.
				CHECK(core.module->service.remaining_ticks == 111); CHECK(site.job.work_ticks == 39); CHECK(site.job.work_fraction == 62914); CHECK(site.job.work_remainder == 420);
				CHECK(used() == 168720); CHECK(GetUtilityPowerProducer(core.module->producer)->meter.generation_energy_j == 810);
				CHECK(site.job.inputs[0].TotalCount() == 8); CHECK(site.job.inputs[1].TotalCount() == 8); save("module-assembly-partial-418.sav");
			}
			// A zero-grant interval must retain the exact partial milestone and both real material containers.
			if (action == "prepare" || action == "work" || action == "partial") {
				REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core.index, core.generation, true));
				const auto before = static_cast<const OrbitalConstructionProgress &>(site.job); const auto profit = builder->profit_this_year; advance(37);
				CHECK(builder->profit_this_year == profit - Money{50} * 256); // A zero-grant held job pays only idle upkeep.
				CHECK(static_cast<const OrbitalConstructionProgress &>(site.job) == before); CHECK(used() == 168720); save("module-assembly-power-418.sav");
			}
			if (action == "cancel") {
				const auto progress = static_cast<const OrbitalConstructionState &>(site.job); const auto spent = used(); service(OrbitalServiceControl::Cancel);
				REQUIRE(Command<Commands::CancelSpaceBlueprint>::Post(site.index, site.generation));
				// Return each real label through a compatible builder and actual owned Structure approaches; cancelled jobs remain physically collectable.
				for (const auto cargo : {steel, goods}) {
					REQUIRE(Command<Commands::CollectSpaceConstructionInputs>::Post(site.index, site.generation, builder->index, builder->generation, cargo, uint16_t{8}));
					order(*builder, core, SpaceOrderAction::Visit); until([&] { return builder->position == GetSpaceStructureApproach(core.state); });
					const auto cash = company->money, profit = builder->profit_this_year;
					REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(core.index, core.generation, builder->index, builder->generation, cargo, uint16_t{8}, false));
					CHECK(company->money == cash); CHECK(builder->profit_this_year == profit);
					if (cargo == steel) { order(*builder, site, SpaceOrderAction::Visit); until([&] { return builder->position == GetSpaceStructureApproach(site.state); }); }
				}
				CHECK(site.job.work_ticks == progress.work_ticks); CHECK(site.job.work_fraction == progress.work_fraction); CHECK(site.job.work_remainder == progress.work_remainder);
				CHECK(used() == spent); CHECK(core.module->stock[steel].TotalCount() == 10); CHECK(core.module->stock[goods].TotalCount() == 8);
				CHECK((site.job.consumed == std::array<uint16_t, 2>{0, 0})); const auto cash = company->money;
				REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(TileXYL(site.state.x, site.state.y, MapLayer::Space), SpaceBuildKind::Storage));
				CHECK(company->money == cash - Money{120000}); const auto &replacement = *SpaceStructure::Get(SpaceStructureID{3});
				CHECK(replacement.generation > site.generation); CHECK(replacement.job.work_ticks == 0); CHECK(replacement.job.work_fraction == 0);
				save("module-assembly-cancelled-418.sav");
			} else {
				if (action != "last") {
					service(OrbitalServiceControl::Cancel); REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core.index, core.generation, false));
					order(*other, site, SpaceOrderAction::Construct); until([&] { return other->position == GetSpaceStructureApproach(site.state); });
					const auto before = site.job.work_ticks; advance(1); CHECK(site.job.work_ticks == before + 1); // Two actual builders still share one750J slot.
					until([&] { return site.job.work_ticks == 295; }); CHECK(GetOrbitalConstructionRemainingEnergy(site.job, 296) == 30);
					CHECK(used() == 360720); save("module-assembly-last-418.sav");
				}
				const auto structure = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
				{ AutoRestoreBackup pause(_pause_mode, PauseModes{}); CHECK_FALSE(CommitOrbitalConstructionStep(*builder)); CHECK_FALSE(CommitOrbitalConstructionStep(*other)); }
				CHECK(ComputeSpaceStructureDigest() == structure); CHECK(ComputeUtilityPowerDigest() == power);
				advance(1); CHECK(site.state.phase == SpaceStructurePhase::FrameComplete); CHECK(site.module == nullptr); CHECK(site.job.work_ticks == 296);
				CHECK(site.job.work_fraction == 0); CHECK(site.job.work_remainder == 0); CHECK((site.job.consumed == std::array<uint16_t, 2>{8, 8}));
				CHECK(site.job.inputs[0].TotalCount() == 0); CHECK(site.job.inputs[1].TotalCount() == 0); CHECK(used() == 360750);
				advance(74); CHECK(used() == 360750); CHECK_FALSE(IsOrbitalModuleOperational(site)); save("module-assembly-frame-418.sav");
			}
			fmt::print("ORBITAL_ASSEMBLY native=true shared_panel_810=true real_partial_grant_60=true exact_last_use_30=true once_only=true no_energy_refund=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(assembly_observer, &observe); IConsole::CmdRegister("orbital_assembly_test", ObserveAssembly);
	const std::string source_path{input}, config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** Continue genuine415 first-Core work with two paid builders, one saved native receipt and no station utility prerequisite. */
TEST_CASE("Portable first-Core work migrates with one native tick receipt", "[.orbital-bootstrap-receipt]")
{
	const char *input = std::getenv("OTTD_ORBITAL_ASSEMBLY_SOURCE"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "module-bootstrap"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe the actual old delivered job; only native vehicle-loop opportunities may advance its portable construction tooling. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto &site = *SpaceStructure::Get(SpaceStructureID{0}); REQUIRE(site.state.kind == SpaceBuildKind::Core); REQUIRE(site.job.work_ticks == 37);
			REQUIRE(site.job.last_work_tick == 0); REQUIRE(site.module == nullptr); std::vector<SpaceVehicle *> builders;
			// Only the two real paid Earth builders from the immutable construction predecessor can compete for this one job.
			for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 20) builders.push_back(craft);
			REQUIRE(builders.size() == 2); const auto power = ComputeUtilityPowerDigest();
			/** @param ticks Actual synchronized native opportunities, bounded independently of render/fast-forward yields. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Count the actual controller clock; neither a loop iteration nor a duplicate public work call earns progress.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			/** @param name Isolated current-version continuation filename. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			advance(1); CHECK(site.job.work_ticks == 38); CHECK(site.job.last_work_tick != 0); CHECK(site.job.last_work_interval == 0);
			const auto digest = ComputeSpaceStructureDigest();
			{ AutoRestoreBackup pause(_pause_mode, PauseModes{}); CHECK_FALSE(CommitOrbitalConstructionStep(*builders[0])); CHECK_FALSE(CommitOrbitalConstructionStep(*builders[1])); }
			CHECK(ComputeSpaceStructureDigest() == digest); CHECK(ComputeUtilityPowerDigest() == power); save("bootstrap-receipt-418.sav");
			advance(702); CHECK(site.job.work_ticks == 740); CHECK(site.state.phase == SpaceStructurePhase::FrameComplete); CHECK(site.module == nullptr);
			CHECK((site.job.consumed == std::array<uint16_t, 2>{16, 24})); CHECK(site.job.inputs[0].TotalCount() == 0); CHECK(site.job.inputs[1].TotalCount() == 0);
			CHECK(ComputeUtilityPowerDigest() == power); save("bootstrap-frame-418.sav");
			fmt::print("ORBITAL_ASSEMBLY bootstrap=true legacy415_paid_inputs=true once_per_native_tick=true no_station_utility_created=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(assembly_observer, &observe); IConsole::CmdRegister("orbital_assembly_test", ObserveAssembly);
	const std::string source_path{input}, config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
