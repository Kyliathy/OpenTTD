/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_module_activation.cpp Focused role bounds, activation purity and genuine paid-frame continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md prerequisite checklist.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_approach.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../orbital_service.h"
#include "../console_internal.h"
#include "../command_func.h"
#include "../core/backup_type.hpp"
#include "../asteroid.h"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "excavation_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

/** Build detached boundary-test data; this helper never claims a transported or manufactured gameplay predecessor.
 * @param kind Canonical static recipe.
 * @param index Native test identity; generation is its next positive integer.
 * @param x Explicit test geometry sharing an edge with the separate Core fixture.
 * @return Fully consumed inert frame without stock, panel or functionality.
 */
static SpaceStructureRecord ActivationFrame(SpaceBuildKind kind, uint16_t index, uint16_t x)
{
	const auto &definition = *GetSpaceConstructionDefinition(kind);
	SpaceStructureRecord row;
	row.ref = {SpaceStructureID{index}, uint64_t{index} + 1}; row.state.owner = _current_company;
	row.state.kind = kind; row.state.x = x; row.state.y = 4; row.state.capital_paid = definition.capital;
	row.state.phase = SpaceStructurePhase::FrameComplete; row.work_ticks = definition.work_ticks;
	row.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)};
	return row;
}

/** Real profile/company fixture for command and shared-ledger boundaries; native paid-world acceptance is a separate hidden case. */
class OrbitalActivationRig {
	ExcavationFixture world;
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{1995}};
public:
	/** Install isolated consumed test receipts through the public native restore boundary, without fabricating any cargo or energy. */
	OrbitalActivationRig()
	{
		ResetUtilityPowerState(); ResetSpaceStructures();
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1995}, 0, 1);
		const std::array rows{ActivationFrame(SpaceBuildKind::Core, 0, 4), ActivationFrame(SpaceBuildKind::Storage, 1, 7), ActivationFrame(SpaceBuildKind::Power, 2, 2)};
		REQUIRE(InstallLoadedSpaceStructures(rows, 4));
	}
	/** Clear only the isolated owners before the fixture restores its real company and content registry. */
	~OrbitalActivationRig()
	{
		_space_structure_pool.CleanPool(); ResetSpaceStructures(); ResetUtilityPowerState();
	}
};

/** Check commands against real native identities, refusing estimates, stale incarnations and incomplete dependencies without side effects. */
TEST_CASE("Module activation prepares once-only native facilities and preserves estimates", "[orbital-activation-contract]")
{
	OrbitalActivationRig rig;
	auto &core = *SpaceStructure::Get(SpaceStructureID{0});
	auto &storage = *SpaceStructure::Get(SpaceStructureID{1});
	auto &power = *SpaceStructure::Get(SpaceStructureID{2});
	const auto before = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest();
	CHECK(CmdActivateOrbitalModule({}, storage.index, storage.generation).Failed());
	CHECK(CmdActivateOrbitalModule({}, core.index, core.generation).Succeeded());
	CHECK(CmdActivateOrbitalModule({}, core.index, core.generation + 1).Failed());
	CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeUtilityPowerDigest() == utility);
	const auto money = Company::Get(_current_company)->money;
	REQUIRE(CmdActivateOrbitalCore({DoCommandFlag::Execute}, core.index, core.generation).Succeeded());
	CHECK_FALSE(IsOrbitalModuleOperational(core)); CHECK(GetOrbitalModulePanelRate(core) == 0);
	CHECK(CmdActivateOrbitalModule({DoCommandFlag::Execute}, core.index, core.generation).Failed());
	CHECK(CmdActivateOrbitalModule({DoCommandFlag::Execute}, storage.index, storage.generation).Failed());
	// Count actual inventory ticks against the saved countdown; no UI query may shorten commissioning.
	for (uint tick = 0; tick < GetMultiLayerSpaceBudget().core_activation_ticks; ++tick) TickOrbitalConstructionInventory();
	REQUIRE(IsOrbitalModuleOperational(core)); CHECK(GetOrbitalModulePanelRate(core) == 30000);
	REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, storage.index, storage.generation).Succeeded());
	REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, power.index, power.generation).Succeeded());
	CHECK(storage.module->producer == UtilityProducerID{UINT32_MAX});
	CHECK(power.module->producer != core.module->producer); CHECK(CaptureUtilityPowerState().producers.size() == 2);
	CHECK(GetOrbitalModuleCargoCapacity(storage, GetCargoTypeByLabel(CargoLabel{"STEL"})) == 0);
	CHECK(GetOrbitalModulePanelRate(power) == 0);
	// Each extension completes its own saved activation; connections never accelerate or multiply another facility's countdown.
	for (uint tick = 0; tick < GetMultiLayerSpaceBudget().core_activation_ticks; ++tick) TickOrbitalConstructionInventory();
	CHECK(GetOrbitalModuleCargoCapacity(storage, GetCargoTypeByLabel(CargoLabel{"STEL"})) == 1024);
	CHECK(GetOrbitalModuleCargoCapacity(core, GetCargoTypeByLabel(CargoLabel{"STEL"})) == 1024);
	CHECK(GetOrbitalModuleCargoCapacity(power, GetCargoTypeByLabel(CargoLabel{"STEL"})) == 0);
	CHECK(GetOrbitalModulePanelRate(power) == 60000);
	CHECK(CmdCommissionOrbitalServices({}, storage.index, storage.generation).Failed());
	CHECK(CmdCommissionOrbitalServices({}, power.index, power.generation).Failed());
	CHECK(Company::Get(_current_company)->money == money);
	REQUIRE(ValidateSpaceStructures());
	PublishOrbitalUtilitySources(GetUtilityPowerInterval(), 1);
	CHECK(GetUtilityPowerProducer(core.module->producer)->meter.generation_energy_j == 810);
	CHECK(GetUtilityPowerProducer(power.module->producer)->meter.generation_energy_j == 1620);
	CHECK(GetUtilityPowerProducer(power.module->producer)->consumed_lifetime_j == 0);
	const std::array sources{power.module->producer};
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, 750, sources);
	REQUIRE(consumer.has_value());
	{
		AutoRestoreBackup authority(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{*consumer, 750}}) == UtilityPowerError::None);
	}
	const auto grant = GetUtilityAllocation(*consumer); REQUIRE(grant.has_value()); CHECK(grant->granted_energy_j == 750);
	// An explicit bounded consumer can reserve only the real Power panel's published output; releasing unused work creates no source debit or new energy.
	const auto release = PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {*consumer, GetUtilityPowerInterval()}, 750);
	CHECK(CommitUtilityMutation(release).error == UtilityPowerError::None);
	CHECK(GetUtilityPowerProducer(power.module->producer)->consumed_lifetime_j == 0);
	REQUIRE(CmdSetOrbitalPanelPaused({DoCommandFlag::Execute}, power.index, power.generation, true).Succeeded());
	CHECK(GetOrbitalModulePanelRate(power) == 0); CHECK(GetOrbitalModulePanelRate(core) == 30000);
}

/** Reject copied role escalation and impossible stock without resolving or mutating a live cargo pool. */
TEST_CASE("Copied module records preserve bounded role-specific storage and panel ownership", "[orbital-activation-contract]")
{
	auto storage = ActivationFrame(SpaceBuildKind::Storage, 1, 7); storage.state.owner = Owner{0}; storage.module.emplace();
	CHECK(ValidateSpaceStructureRecords(std::array{storage}, 4, 64, 64));
	storage.module->quantities[0] = 1024; storage.module->stock[0] = {{CargoPacketID{1}, StationID::Invalid()}};
	CHECK(ValidateSpaceStructureRecords(std::array{storage}, 4, 64, 64));
	auto bad = storage; ++bad.module->quantities[0]; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{bad}, 4, 64, 64));
	bad = storage; bad.module->producer = UtilityProducerID{0}; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{bad}, 4, 64, 64));
	bad = storage; bad.module->activation_ticks = 1; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{bad}, 4, 64, 64));
	bad = storage; bad.module->assembly_consumer = UtilityConsumerID{0}; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{bad}, 4, 64, 64));
	auto power = ActivationFrame(SpaceBuildKind::Power, 2, 2); power.state.owner = Owner{0}; power.module.emplace();
	CHECK_FALSE(ValidateSpaceStructureRecords(std::array{power}, 4, 64, 64));
	power.module->producer = UtilityProducerID{1}; CHECK(ValidateSpaceStructureRecords(std::array{power}, 4, 64, 64));
	power.module->quantities[0] = 1; power.module->stock[0] = {{CargoPacketID{2}, StationID::Invalid()}};
	CHECK_FALSE(ValidateSpaceStructureRecords(std::array{power}, 4, 64, 64));
}

static std::function<void()> *activation_observer = nullptr; ///< Process-local console hook; never a simulation owner.

/** Invoke the loaded-world acceptance observer.
 * @param args Console command without additional parameters.
 * @return Whether the installed native observer ran.
 */
static bool ObserveActivation(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(activation_observer != nullptr); (*activation_observer)(); return true;
}

/** Activate and exchange freight only from an immutable genuine Earth-paid module frame, preserving interrupted native saves. */
TEST_CASE("A genuine paid Storage frame activates and owns transported packets once", "[.orbital-module-activation]")
{
	const char *input = std::getenv("OTTD_ORBITAL_ACTIVATION_SOURCE"), *mode = std::getenv("OTTD_ORBITAL_ACTIVATION_ACTION");
	REQUIRE(input != nullptr); REQUIRE(mode != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "module-activation"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Execute through the ordinary loaded game and preserve assertion failures across native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			auto &storage = *SpaceStructure::Get(SpaceStructureID{1});
			REQUIRE(storage.state.kind == SpaceBuildKind::Storage); REQUIRE(ValidateSpaceVehicles());
			const auto money = Company::Get(_current_company)->money;
			if (const char *corrupt = std::getenv("OTTD_ORBITAL_ACTIVATION_CORRUPT")) {
				const auto state = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest(); const auto random = _random;
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceStructureDigest() == state); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(Company::Get(_current_company)->money == money);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("ORBITAL_ACTIVATION native=true preview_rejected=true authority_unchanged=true\n");
				observed = true; _exit_game = true; return;
			}
			/** Advance the real synchronized game, granting no independent activation clock.
			 * @param ticks Actual simulation opportunities to observe.
			 */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Rendering/yields are excluded; only actual field/controller ticks count toward the saved countdown.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			/** Write actual native packet and facility owners through the ordinary writer.
			 * @param name Checkpoint basename inside this invocation's authorized save directory.
			 */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			if (action == "freight" || action == "stored") {
				REQUIRE(IsOrbitalModuleOperational(storage));
				const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}); auto &core = *SpaceStructure::Get(SpaceStructureID{0});
				SpaceVehicle *builder = nullptr;
				// Select the actual paid builder already standing at this completed Storage approach, rather than injecting another craft or position.
				for (auto *craft : SpaceVehicle::Iterate()) {
					if (GetSpaceVehicleModel(*craft) == 20 && craft->position == GetSpaceStructureApproach(storage.state)) { builder = craft; break; }
				}
				REQUIRE(builder != nullptr); REQUIRE(builder->cargo.TotalCount() == 0);
				if (action == "freight") {
					const auto original_stock = core.module->stock[steel].TotalCount(); REQUIRE(original_stock >= 1);
					auto &staging = *SpaceStructure::Get(SpaceStructureID{2}); REQUIRE(staging.state.phase == SpaceStructurePhase::Blueprint);
					SpaceVehicle *other = nullptr;
					// The genuine pre-admission418 seed has two paid hulls at Storage. Move its other builder to existing paid blueprint geometry before returning freight here.
					for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 20 && craft != builder) other = craft;
					REQUIRE(other != nullptr);
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(other->index, other->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(other->index, other->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure, uint32_t{staging.index.base()}, staging.generation));
					// Native movement frees the occupied Storage approach without deleting its original hull, manufacturing another berth or granting the staging blueprint facilities.
					for (uint elapsed = 0; !GetSpaceStructureBerth(*other, staging); ++elapsed) { REQUIRE(elapsed < 1500); advance(1); }
					/** Travel through actual typed queues to a native site's precise approach.
					 * @param destination Exact owned operational module whose approach the native controller must reach.
					 */
					auto visit = [&](const SpaceStructure &destination) {
						REQUIRE(Command<Commands::EditSpaceOrder>::Post(builder->index, builder->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
						REQUIRE(Command<Commands::EditSpaceOrder>::Post(builder->index, builder->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure, uint32_t{destination.index.base()}, destination.generation));
						// A retained Visit owns its admitted berth rather than advancing the queue cursor; real physical admission authorizes this freight exchange.
						for (uint elapsed = 0; !GetSpaceStructureBerth(*builder, destination); ++elapsed) {
							REQUIRE(elapsed < 1500); advance(1);
						}
					};
					visit(core);
					REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(core.index, core.generation, builder->index, builder->generation, steel, uint16_t{1}, true));
					CHECK(core.module->stock[steel].TotalCount() == original_stock - 1);
					visit(storage);
					const auto cash = Company::Get(_current_company)->money; const auto profit = builder->profit_this_year;
					REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(storage.index, storage.generation, builder->index, builder->generation, steel, uint16_t{1}, false));
					CHECK(Company::Get(_current_company)->money == cash); CHECK(builder->profit_this_year == profit);
					save("module-stocked-422.sav");
				}
				REQUIRE(storage.module->stock[steel].TotalCount() == 1);
				const auto state = ComputeSpaceStructureDigest(); const auto cash = Company::Get(_current_company)->money;
				CHECK(CmdMoveOrbitalFreight({}, storage.index, storage.generation + 1, builder->index, builder->generation, steel, 1, true).Failed());
				CHECK(CmdMoveOrbitalFreight({}, storage.index, storage.generation, builder->index, builder->generation, steel, 1025, true).Failed());
				CHECK(CmdActivateOrbitalModule({DoCommandFlag::Execute}, storage.index, storage.generation).Failed());
				CHECK(ComputeSpaceStructureDigest() == state); CHECK(Company::Get(_current_company)->money == cash);
				REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(storage.index, storage.generation, builder->index, builder->generation, steel, uint16_t{1}, true));
				REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(storage.index, storage.generation, builder->index, builder->generation, steel, uint16_t{1}, false));
				CHECK(ComputeSpaceStructureDigest() == state); CHECK(builder->cargo.TotalCount() == 0);
				save("module-freight-422.sav");
				fmt::print("ORBITAL_ACTIVATION native=true actual_storage_freight=true once_only=true no_delivery_payment=true\n");
				observed = true; _exit_game = true; return;
			}
			if (action == "prepare") {
				REQUIRE(storage.module == nullptr); const auto before = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
				CHECK(CmdActivateOrbitalModule({}, storage.index, storage.generation).Succeeded());
				CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeUtilityPowerDigest() == power);
				REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(storage.index, storage.generation));
				CHECK(Company::Get(_current_company)->money == money); CHECK_FALSE(IsOrbitalModuleOperational(storage));
				advance(37); REQUIRE(storage.module->activation_ticks == 37); save("module-activating-422.sav");
			} else {
				REQUIRE(action == "continue"); REQUIRE(storage.module != nullptr); REQUIRE(storage.module->activation_ticks == 37);
			}
			advance(37); REQUIRE(IsOrbitalModuleOperational(storage));
			CHECK(storage.module->producer == UtilityProducerID{UINT32_MAX}); CHECK((storage.job.consumed == std::array<uint16_t, 2>{8, 8}));
			CHECK(CmdActivateOrbitalModule({DoCommandFlag::Execute}, storage.index, storage.generation).Failed());
			const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}); CHECK(GetOrbitalModuleCargoCapacity(storage, steel) == 1024);
			CHECK(ValidateSpaceStructures()); save("module-activated-422.sav");
			fmt::print("ORBITAL_ACTIVATION native=true paid_storage=true saved_countdown=true once_only=true no_new_source=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(activation_observer, &observe); IConsole::CmdRegister("orbital_activation_test", ObserveActivation);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
