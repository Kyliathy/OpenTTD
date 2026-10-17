/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_reservation_native.cpp Original paid Assembly receives actual Surface flights from declared test stock and retains one grant-qualified native recipe batch.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group7
 * Starting Surface stock is explicit fixture content, as in paid-checkpoints.json; this proves no procurement or factory-to-launch supply chain.
 */
#include "../stdafx.h"
#include "../fios.h"
#include "assembly_native_fixture.h"
#include "../orbital_manufacturing.h"
#include "../company_base.h"
#include "../station_base.h"
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

static std::function<void()> *manufacturing_observer = nullptr; ///< One invocation's actual loaded-world observer.
/** @param args Native console tokens. @return Whether this invocation's actual-world observer ran. */
static bool ObserveManufacturingReservation(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(manufacturing_observer != nullptr); (*manufacturing_observer)(); return true;
}

/** Exercise one bounded Surface delivery, paid request, exact reservation, outage or lifecycle boundary without fixture stock/hulls. */
TEST_CASE("Paid native Assembly reservations retain actual Surface inputs and original grants", "[.manufacturing-reservation-native]")
{
	const char *source = std::getenv("OTTD_MANUFACTURE_SOURCE"), *mode = std::getenv("OTTD_MANUFACTURE_ACTION");
	REQUIRE(source != nullptr); REQUIRE(mode != nullptr); const std::string_view action{mode};
	const char *selected = std::getenv("OTTD_MANUFACTURE_MODEL"); const auto parsed_model = ParseInteger<uint16_t>(selected == nullptr ? "15" : selected);
	REQUIRE(parsed_model.has_value()); const uint16_t model = *parsed_model;
	const auto paths = GetTestCaseArtifactPaths("space", "manufacturing-reservation-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe only after actual native load and preserve assertion failures through normal application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			auto &core = *world.core; SpaceStructure *assembly = nullptr;
			// Resolve the real paid native incarnation; a definition, copied frame or invented local ID cannot supply a manufacturing facility.
			for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Assembly) { REQUIRE(assembly == nullptr); assembly = site; }
			REQUIRE(assembly != nullptr); REQUIRE(IsOrbitalModuleOperational(*assembly));
			const auto hulls = GetSpaceVehicleCount(); const auto *definition = GetSpaceVehicleConstruction(model); REQUIRE(definition != nullptr); const auto &recipe = *definition;
			const auto capital = Company::Get(_current_company)->money; const auto producers = CaptureUtilityPowerState().producers.size();
			/** @param name Exact stage basename. Save the actual paused owner state without replaying any reservation or production. */
			auto save = [&](std::string_view name) { world.Save(fmt::format("manufacture-{}-{}-426.sav", name, model)); };
			if (const char *corrupt = std::getenv("OTTD_MANUFACTURE_CORRUPT")) {
				const auto before = ComputeSpaceVehicleDigest(), utility = ComputeUtilityPowerDigest(); const auto random = _random;
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceVehicleDigest() == before); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(Company::Get(_current_company)->money == capital);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("MANUFACTURE_RESERVATION native=true preview_rejected=true unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "setup") {
				REQUIRE(assembly->module->manufacture == nullptr);
				auto *origin = ResolveLaunchCenter(world.rocket->construction.home); REQUIRE(origin != nullptr);
				const std::array<std::pair<CargoLabel, uint16_t>, 5> surface_inputs{{{CargoLabel{"PROP"}, 60}, {CargoLabel{"STEL"}, 6},
					{CargoLabel{"GOOD"}, 12}, {CargoLabel{"RBPT"}, 4}, {CargoLabel{"ELEC"}, 2}}};
				// Declare only missing initial Surface test content, never orbital inventory or a paid receipt. Every later unit still requires actual packing, fuel, travel and unloading.
				for (const auto &[label, required] : surface_inputs) {
					const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo)); auto &stock = origin->goods[cargo].GetOrCreateData().cargo;
					if (stock.AvailableCount() >= required) continue;
					REQUIRE(CargoPacket::CanAllocateItem()); const auto missing = static_cast<uint16_t>(required - stock.AvailableCount());
					stock.Append(CargoPacket::Create(origin->index, missing, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
					CHECK(stock.AvailableCount() == required);
				}
				SpaceVehicle *departing = nullptr;
				// Storage1 already holds the other paid builder. Select only the actual Assembly occupant; pool order does not grant a free destination berth.
				for (auto *craft : world.builders) if (GetSpaceStructureBerth(*craft, *assembly)) { REQUIRE(departing == nullptr); departing = craft; }
				REQUIRE(departing != nullptr); auto *destination = SpaceStructure::GetIfValid(SpaceStructureID{2}); REQUIRE(destination != nullptr);
				world.Order(*departing, *destination, SpaceOrderAction::Visit);
				// Retain the actual pursuit state on failure so a blocked native admission cannot be mistaken for missing travel time.
				for (uint elapsed = 0; !GetSpaceStructureBerth(*departing, *destination).has_value(); ++elapsed) {
					const auto &craft = *departing; const auto &queue = craft.space_orders;
					INFO(fmt::format("builder={} destination={} position=({}, {}) elapsed={} stopped={} cursor={} block={} approach={} slot={}",
						craft.index.base(), destination->index.base(), craft.position.x, craft.position.y, elapsed, craft.vehstatus.Test(VehState::Stopped),
						queue.cursor, to_underlying(queue.blocked), queue.approach.has_value(), queue.approach ? queue.approach->slot : UINT8_MAX));
					REQUIRE(elapsed < 3000); world.Advance(1);
				}
				REQUIRE(IsSpaceStructureManufactureBerthFree(*assembly)); REQUIRE(IsSpaceApproachSiteAvailable(core.state)); save("ready");
			} else if (action == "steel" || action == "goods" || action == "robot" || action == "electronics") {
				const auto label = action == "steel" ? CargoLabel{"STEL"} : action == "goods" ? CargoLabel{"GOOD"} : action == "robot" ? CargoLabel{"RBPT"} : CargoLabel{"ELEC"};
				const uint16_t quantity = action == "steel" ? 6 : action == "goods" ? 12 : action == "robot" ? 4 : 2;
				const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo)); const auto before = core.module->stock[cargo].TotalCount();
				const auto home = world.rocket->construction.home; const auto missions = world.rocket->transition->missions_completed;
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(home.station, home.generation, world.rocket->index, world.rocket->generation, cargo, quantity, false));
				const auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr); const auto fuel_cargo = GetSpaceSupplyCargo(SpaceSupply::Propellant);
				INFO(fmt::format("phase={} held_fuel={} Surface_fuel={} service_ticks={} completed_flights={}", action, world.rocket->manifest->supplies[0].TotalCount(),
					center->goods[fuel_cargo].HasData() ? center->goods[fuel_cargo].GetData().cargo.AvailableCount() : 0, world.rocket->manifest->service_ticks, missions));
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, world.rocket->index, world.rocket->generation, uint8_t{0}, uint16_t{15}, false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(world.rocket->index, world.rocket->generation, SpaceOrderTargetKind::Structure, uint32_t{core.index.base()}, core.generation));
				world.Await([&] { return world.rocket->transition->missions_completed == missions + 1; });
				CHECK(core.module->stock[cargo].TotalCount() == before + quantity); CHECK(world.rocket->construction.placement == SpacePlacement::SurfaceStored);
				CHECK(assembly->module->manufacture == nullptr); save(action);
			} else if (action == "queued") {
				REQUIRE(assembly->module->manufacture == nullptr); const auto before = ComputeSpaceVehicleDigest(), utility = ComputeUtilityPowerDigest();
				REQUIRE(CmdQueueOrbitalManufacture({}, assembly->index, assembly->generation, core.index, core.generation, model).Succeeded());
				CHECK(CmdQueueOrbitalManufacture({}, assembly->index, assembly->generation, core.index, core.generation + 1, model).Failed());
				CHECK(ComputeSpaceVehicleDigest() == before); CHECK(ComputeUtilityPowerDigest() == utility);
				REQUIRE(Command<Commands::QueueOrbitalManufacture>::Post(assembly->index, assembly->generation, core.index, core.generation, model));
				const auto &job = *assembly->module->manufacture; CHECK(job.phase == OrbitalManufacturePhase::Queued);
				CHECK(job.capital_paid == recipe.capital); CHECK(Company::Get(_current_company)->money == capital - recipe.capital);
				CHECK(CmdQueueOrbitalManufacture({}, assembly->index, assembly->generation, core.index, core.generation, model).Failed());
				for (const auto &slot : job.inputs) CHECK(slot.TotalCount() == 0);
				save("queued");
			} else {
				REQUIRE(assembly->module->manufacture != nullptr); const auto serial = assembly->module->manufacture->serial;
				REQUIRE(assembly->module->manufacture->model == model);
				if (action == "reserved" || action == "outage" || action == "restore") {
					REQUIRE(assembly->module->manufacture->phase == OrbitalManufacturePhase::Queued);
					const auto spent = GetUtilityPowerProducer(core.module->producer)->consumed_lifetime_j;
					if (action == "outage") {
						REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core.index, core.generation, true)); world.Advance(10);
						CHECK(assembly->module->manufacture->phase == OrbitalManufacturePhase::Queued);
						save("outage");
					} else {
						if (action == "restore") REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core.index, core.generation, false));
						world.Advance(1); REQUIRE(assembly->module->manufacture->phase == OrbitalManufacturePhase::Reserved);
						// Every declared Surface-delivered label enters one actual job together; the grant is observed without spending energy, work or capital again.
						for (size_t input = 0; input < recipe.inputs.size(); ++input) CHECK(assembly->module->manufacture->inputs[input].TotalCount() == recipe.inputs[input].quantity);
						save(action);
					}
					CHECK(GetUtilityPowerProducer(core.module->producer)->consumed_lifetime_j == spent);
					CHECK(assembly->module->manufacture->serial == serial);
					CHECK(assembly->module->manufacture->capital_paid == recipe.capital); // Actual opportunities may charge ordinary native upkeep; job capital is not charged again.
				} else if (action == "cancel") {
					REQUIRE(assembly->module->manufacture->phase == OrbitalManufacturePhase::Reserved);
					std::array<uint, MAX_ORBITAL_MANUFACTURE_INPUTS> expected{};
					// The cancellation destination must retain its original stock plus exactly this batch, with no material creation or capital refund.
					for (size_t input = 0; input < recipe.inputs.size(); ++input) expected[input] = core.module->stock[GetCargoTypeByLabel(recipe.inputs[input].label)].TotalCount() + recipe.inputs[input].quantity;
					const auto before = ComputeSpaceVehicleDigest();
					REQUIRE(CmdCancelOrbitalManufacture({}, assembly->index, assembly->generation, core.index, core.generation).Succeeded()); CHECK(ComputeSpaceVehicleDigest() == before);
					REQUIRE(Command<Commands::CancelOrbitalManufacture>::Post(assembly->index, assembly->generation, core.index, core.generation));
					CHECK(assembly->module->manufacture == nullptr); CHECK(assembly->module->manufacture_serial == serial);
					CHECK(Company::Get(_current_company)->money == capital); // No simulation tick or capital refund occurs during explicit packet return.
					for (size_t input = 0; input < recipe.inputs.size(); ++input) CHECK(core.module->stock[GetCargoTypeByLabel(recipe.inputs[input].label)].TotalCount() == expected[input]);
					save("cancel");
				} else {
					REQUIRE(action == "company"); REQUIRE(assembly->module->manufacture->phase == OrbitalManufacturePhase::Reserved);
					const auto ref = GetSpaceStructureReference(*assembly);
					/** @return Exact original facility's per-label packet identities, independent of native pool holes or index reuse. */
					auto capture_inputs = [&] {
						const auto rows = CaptureSpaceStructures();
						/** @param row Captured native owner. @return Whether it is this exact original Assembly incarnation. */
						const auto found = std::ranges::find_if(rows, [&](const auto &row) { return row.ref == ref; });
						REQUIRE(found != rows.end()); REQUIRE(found->module->manufacture.has_value()); return found->module->manufacture->inputs;
					};
					const auto inputs = capture_inputs();
					REQUIRE(Company::CanAllocateItem()); auto *company = Company::Create(); SetCompanyManagerFaceStyle(company->face, 0); company->money = 10000000;
					const auto owner = company->index; ChangeOwnershipOfCompanyItems(_current_company, owner); REQUIRE(ValidateSpaceVehicles());
					CHECK(assembly->state.owner == owner); CHECK(assembly->module->manufacture->serial == serial); CHECK(GetSpaceStructureReference(*assembly) == ref);
					CHECK(capture_inputs() == inputs);
					CHECK(CmdCancelOrbitalManufacture({}, assembly->index, assembly->generation, core.index, core.generation).Failed());
					ChangeOwnershipOfCompanyItems(owner, INVALID_OWNER); delete Company::Get(owner);
					REQUIRE(Company::CanAllocateItem()); auto *reused = Company::Create(); SetCompanyManagerFaceStyle(reused->face, 0); reused->money = 10000000; CHECK(reused->index == owner);
					{
						AutoRestoreBackup other(_current_company, reused->index);
						CHECK(CmdCancelOrbitalManufacture({}, assembly->index, assembly->generation, core.index, core.generation).Failed());
						CHECK(CmdQueueOrbitalManufacture({}, assembly->index, assembly->generation, core.index, core.generation, model).Failed());
					}
					CHECK(assembly->state.owner == OWNER_NONE); CHECK(capture_inputs() == inputs);
					save("company"); fmt::print("MANUFACTURE_RESERVATION native=true phase=company model={} company_id_reuse_refused=true\n", model);
					observed = true; _exit_game = true; return; // Actual company deletion owns hull destruction; non-owning world pointers must not be reused.
				}
			}
			CHECK(GetSpaceVehicleCount() == hulls); CHECK(CaptureUtilityPowerState().producers.size() == producers); REQUIRE(ValidateSpaceVehicles());
			// Inspect the actual native controls at both sides of the reservation boundary; no mock UI can substitute for synchronized command eligibility.
			if (action == "queued" || action == "reserved") {
				REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				ShowSpaceConstructionWindow(GetSpaceStructureReference(*assembly)); auto *window = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(window != nullptr);
				CaptureNativeArtWindow(*window, paths.images / fmt::format("manufacture-{}-{}.ppm", action, model));
			}
			fmt::print("MANUFACTURE_RESERVATION native=true phase={} model={} no_free_vehicle=true actual_inputs=true Surface_fixture_stock=true procurement=false\n", action, model);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(manufacturing_observer, &observe); IConsole::CmdRegister("manufacturing_reservation_test", ObserveManufacturingReservation);
	const std::string input{source}, configuration = config.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", input};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
