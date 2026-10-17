/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file processor_freight_native.cpp Original paid hauler navigation, exclusive packet exchange and ordinary continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12
 * Only exact retained paid factory worlds and original public commands supply hulls, stock, work, travel and berths.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_func.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../station_base.h"
#include "../timer/timer_game_tick.h"
#include "../vehicle_cmd.h"
#include "../orbital_recipe.h"
#include "../orbital_manufacturing.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *freight_observer = nullptr; ///< Invocation callback only; no alternative gameplay or cargo owner.

/** @param args Parameter-free ordinary native console callback. @return Whether this invocation's loaded-world observer ran. */
static bool ObserveProcessorFreight(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(freight_observer); (*freight_observer)(); return true;
}

/** @return The actual completed17 job's original paid product, never an allocated substitute or another older hull sharing its model. */
static SpaceVehicle &PaidFreightHauler()
{
	SpaceVehicle *hauler = nullptr;
	// Earlier materialization also issued a hauler in this retained world. Only the current completed ordinary-purchase job identifies this acceptance's exact product; model equality cannot replace its actual paid factory/serial join.
	for (auto *site : SpaceStructure::Iterate()) {
		if (!site->module || !site->module->manufacture) continue;
		const auto &job = *site->module->manufacture; auto *craft = ResolveSpaceCraft(job.product);
		// Only an actual completed17 product with this same factory job may supply the current paid transport proof; incomplete jobs and other roles grant nothing.
		if (job.phase != OrbitalManufacturePhase::Complete || craft == nullptr || GetSpaceVehicleModel(*craft) != 17) continue;
		REQUIRE(hauler == nullptr); REQUIRE(craft->construction.assembly == GetSpaceStructureReference(*site)); REQUIRE(craft->construction.assembly_serial == job.serial); hauler = craft;
	}
	REQUIRE(hauler); REQUIRE(hauler->construction.assembly != SpaceStructureRef{}); REQUIRE(hauler->construction.assembly_serial != 0);
	return *hauler;
}

/** Vacate only an actual old Core occupant through original typed navigation before the hauler approaches.
 * @param world Loaded actual-world commands and synchronized opportunities.
 * @param craft Original paid craft physically occupying the target approach.
 */
static void DepartFreightApproach(AssemblyNativeWorld &world, SpaceVehicle &craft)
{
	REQUIRE_FALSE(GetAsteroidField().bodies.empty()); const auto before = craft.position;
	if (craft.vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Post(craft.tile, craft.index, true));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, GetAsteroidField().bodies.front().id.base(), uint64_t{0}));
	/** @return Whether native movement actually freed the old point rather than merely changing a request or hidden berth flag. */
	world.Await([&] { return craft.position != before; });
}

/** Actual paid general freight must survive native admission, precise travel, packet movement and interrupted ordinary save/resume. */
TEST_CASE("Original paid hauler conserves explicit freight through native berths and saved next actions", "[.processor-freight-native]")
{
	const char *source = std::getenv("OTTD_PROCESSOR_FREIGHT_SOURCE"), *selected = std::getenv("OTTD_PROCESSOR_FREIGHT_PHASE"), *own = std::getenv("OTTD_PROCESSOR_FREIGHT_RESAVE");
	REQUIRE(source); REQUIRE(selected); const std::string_view phase{selected}; const bool resave = own != nullptr && std::string_view{own} == "1";
	const auto paths = GetTestCaseArtifactPaths("space", "processor-freight-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve native assertion failures across orderly shutdown while examining only actual loaded paid owners. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths); auto &hauler = PaidFreightHauler();
			auto *factory = ResolveSpaceStructure(hauler.construction.assembly); REQUIRE(factory); REQUIRE(IsOrbitalModuleOperational(*factory));
			const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}); REQUIRE(IsValidCargoType(goods)); CHECK(CargoSpec::Get(goods)->weight == 8);
			CHECK(CargoSpec::Get(goods)->classes.Test(CargoClass::Express));
			const auto freight = GetCargoTypeByLabel(CargoLabel{"STEL"}); REQUIRE(IsValidCargoType(freight));
			auto *storage = SpaceStructure::GetIfValid(SpaceStructureID{1}); REQUIRE(storage); REQUIRE(storage->state.kind == SpaceBuildKind::Storage);
			REQUIRE(storage->state.phase == SpaceStructurePhase::FrameComplete); REQUIRE(storage->state.capital_paid == 120000); REQUIRE(storage->job.work_ticks == 296);
			CHECK(storage->job.consumed == std::array<uint16_t, 2>{8, 8});
			const auto craft = GetSpaceCraftReference(hauler); const auto assembly = GetSpaceStructureReference(*factory);
			const auto structures = ComputeSpaceStructureDigest(), vehicles = ComputeSpaceVehicleDigest(), power = ComputeUtilityPowerDigest(), tick = TimerGameTick::counter;
			const auto cash = Company::Get(_current_company)->money; const auto random = _random;
			const auto deliveries = Company::Get(_current_company)->cur_economy.delivered_cargo[freight];
			REQUIRE(ValidateSpaceVehicles()); REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(ComputeSpaceVehicleDigest() == vehicles); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == power);
			CHECK(Company::Get(_current_company)->money == cash); CHECK(TimerGameTick::counter == tick);
			CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			if (const char *corrupt = std::getenv("OTTD_PROCESSOR_FREIGHT_CORRUPT")) {
				REQUIRE(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceVehicleDigest() == vehicles); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == power);
				CHECK(Company::Get(_current_company)->money == cash); CHECK(TimerGameTick::counter == tick);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("PROCESSOR_FREIGHT native=true preview_rejected=true authority_unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			fmt::print("FREIGHT_STOCK core_goods={} assembly_goods={} held={} capacity={} craft={} generation={} pods={}\n", world.core->module->stock[goods].TotalCount(),
				factory->module->stock[goods].TotalCount(), hauler.cargo.TotalCount(), hauler.cargo_cap, hauler.index.base(), hauler.generation, hauler.manifest ? hauler.manifest->pods.size() : 0);
			// Characterize actual retained participants before choosing a native destination; a declared stock type cannot grant an inert blueprint or Assembly an operational cargo buffer.
			for (const auto *site : SpaceStructure::Iterate()) fmt::print("FREIGHT_SITE id={} generation={} kind={} phase={} module={} capacity={} steel={} goods={}\n", site->index.base(), site->generation,
				to_underlying(site->state.kind), to_underlying(site->state.phase), site->module != nullptr, GetOrbitalModuleCargoCapacity(*site, goods),
				site->module ? site->module->stock[GetCargoTypeByLabel(CargoLabel{"STEL"})].TotalCount() : 0, site->module ? site->module->stock[goods].TotalCount() : 0);
			if (resave) {
				// Rewriting this same checkpoint must repeat no controls, travel, refit, work or packet transfer. The independent Python reader compares every original physical family afterward.
				REQUIRE(phase != "inspected");
			} else if (phase == "prepared") {
				// This original paid Storage frame already consumed its complete bill. Only its public native activation earns facilities now; loading or a coincident footprint grants no free buffer or cargo.
				REQUIRE_FALSE(storage->module); REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(storage->index, storage->generation)); world.Advance(74);
				REQUIRE(IsOrbitalModuleOperational(*storage)); CHECK(storage->module->stock[freight].TotalCount() == 0); CHECK(world.core->module->stock[freight].TotalCount() == 2);
			} else if (phase == "loaded") {
				REQUIRE(IsOrbitalModuleOperational(*storage));
				REQUIRE(hauler.cargo.TotalCount() == 0); const auto approach = GetSpaceStructureApproach(world.core->state), destination = GetSpaceStructureApproach(storage->state);
				// Only real occupants of either chosen endpoint must depart. The retained constructor still physically occupies its completed Storage approach; real navigation frees that berth without changing a pose, hidden flag or paid material history.
				for (auto *other : SpaceVehicle::Iterate()) {
					if (other == &hauler || other->construction.placement != SpacePlacement::Orbital || (other->position != approach && other->position != destination)) continue;
					DepartFreightApproach(world, *other);
				}
				if (hauler.vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Post(hauler.tile, hauler.index, true));
				world.Order(hauler, *world.core, SpaceOrderAction::Visit);
				/** @return Whether this same actual hauler reached and acquired its precise original Core berth through native movement. */
				world.Await([&] { return GetSpaceStructureBerth(hauler, *world.core).has_value() && hauler.mission.phase == SpaceMissionPhase::Holding; });
				const auto available = world.core->module->stock[freight].TotalCount(); REQUIRE(available == 2);
				const auto before = ComputeSpaceStructureDigest(); const auto money = Company::Get(_current_company)->money;
				REQUIRE(CmdMoveOrbitalFreight({}, world.core->index, world.core->generation, hauler.index, hauler.generation, freight, 2, true).Succeeded());
				CHECK(ComputeSpaceStructureDigest() == before); CHECK(hauler.cargo.TotalCount() == 0); CHECK(Company::Get(_current_company)->money == money);
				REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(world.core->index, world.core->generation, hauler.index, hauler.generation, freight, uint16_t{2}, true));
				CHECK(hauler.cargo.TotalCount() == 2); CHECK(hauler.cargo_type == freight); CHECK(hauler.cargo_cap == 20);
				CHECK(world.core->module->stock[freight].TotalCount() == available - 2); CHECK(Company::Get(_current_company)->money == money);
				// Actual imported stock keeps its original non-asteroid provenance; hauling cannot reclassify it as a finite mined resource to pass native validation.
				for (const auto *packet : *hauler.cargo.Packets()) CHECK(packet->GetSource().type != SourceType::Asteroid);
			} else if (phase == "travelling") {
				REQUIRE(hauler.cargo.TotalCount() == 2); REQUIRE(hauler.cargo_type == freight); world.Order(hauler, *storage, SpaceOrderAction::Unload); world.Advance(1);
				CHECK(hauler.cargo.TotalCount() == 2); CHECK(hauler.space_orders.cursor < hauler.space_orders.entries.size());
			} else if (phase == "transferred") {
				REQUIRE(hauler.cargo.TotalCount() == 2); const auto before = storage->module->stock[freight].TotalCount();
				/** @return Whether the actual typed Unload moved the complete original holding at its admitted destination. */
				world.Await([&] { return hauler.space_orders.cursor == hauler.space_orders.entries.size(); });
				CHECK(hauler.cargo.TotalCount() == 0); CHECK(storage->module->stock[freight].TotalCount() == before + 2); REQUIRE(GetSpaceStructureBerth(hauler, *storage));
			} else if (phase == "returned") {
				REQUIRE(hauler.cargo.TotalCount() == 0); const auto before = world.core->module->stock[freight].TotalCount(), held = storage->module->stock[freight].TotalCount(); REQUIRE(held == 2);
				REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(storage->index, storage->generation, hauler.index, hauler.generation, freight, uint16_t{2}, true));
				world.Order(hauler, *world.core, SpaceOrderAction::Unload);
				/** @return Whether native explicit return navigation restored only these same two transported Steel units, with no delivery income or second stock owner. */
				world.Await([&] { return hauler.space_orders.cursor == hauler.space_orders.entries.size(); });
				CHECK(hauler.cargo.TotalCount() == 0); CHECK(world.core->module->stock[freight].TotalCount() == before + 2); CHECK(storage->module->stock[freight].TotalCount() == held - 2);
			} else {
				REQUIRE(phase == "inspected"); REQUIRE(hauler.cargo.TotalCount() == 0);
			}
			CHECK(GetSpaceCraftReference(hauler) == craft); CHECK(hauler.construction.assembly == assembly);
			CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[freight] == deliveries); REQUIRE(ValidateSpaceVehicles());
			world.Save(fmt::format("freight-{}-438.sav", phase));
			if (resave) {
				CHECK(ComputeSpaceVehicleDigest() == vehicles); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == power);
				CHECK(Company::Get(_current_company)->money == cash); CHECK(TimerGameTick::counter == tick);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			}
			fmt::print("PROCESSOR_FREIGHT native=true phase={} added_stock=false resave={}\n", phase, resave); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(freight_observer, &observe); IConsole::CmdRegister("processor_freight_test", ObserveProcessorFreight);
	const std::string configuration = config.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
