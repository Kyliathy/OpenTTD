/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_purchase_native.cpp Actual ordinary purchase payment, callback and once-only production from accepted paid worlds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8B
 * Explicit heavy/Earth preparation may declare initial Surface test content, as Group7 did; procurement/factory-chain acceptance is not claimed.
 * Every orbital material movement, grant, work quantum, hull and receipt still belongs to its real public gameplay owner.
 */
#include "../stdafx.h"
#include "../fios.h"
#include "assembly_native_fixture.h"
#include "../space_purchase.h"
#include "../orbital_manufacturing.h"
#include "../vehicle_cmd.h"
#include "../station_base.h"
#include "../launch_operations.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../console_internal.h"
#include "../core/string_consumer.hpp"
#include "../core/random_func.hpp"
#include "../window_func.h"
#include "../window_gui.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include "../safeguards.h"

static std::function<void()> *purchase_observer = nullptr; ///< Exact current loaded-world observer; never a replacement simulation.
/** @param args Actual console tokens. @return Whether the explicit current loaded-world observer ran. */
static bool ObserveManufacturingPurchase(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(purchase_observer != nullptr); (*purchase_observer)(); return true;
}
/** Declare missing initial Surface test content before the new purchase/freight scenario, with no orbital input or paid owner grant.
 * @param origin Actual original Surface station; this fixture does not claim production/procurement of these starting packets.
 * @param label Existing selected-profile cargo identity.
 * @param quantity Required starting available quantity, keeping any existing native stock.
 */
static void PurchaseSurfaceStartingContent(Station &origin, CargoLabel label, uint16_t quantity)
{
	const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo)); auto &stock = origin.goods[cargo].GetOrCreateData().cargo;
	if (stock.AvailableCount() >= quantity) return;
	const auto missing = static_cast<uint16_t>(quantity - stock.AvailableCount()); REQUIRE(CargoPacket::CanAllocateItem());
	stock.Append(CargoPacket::Create(origin.index, missing, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
	fmt::print("PURCHASE_SURFACE_FIXTURE cargo={} added={}\n", cargo, missing);
}
/** Select the actual loaded renderer for purchase callbacks/captures, without granting a gameplay owner or advancing simulation. */
static void PreparePurchaseNativeRenderer()
{
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
}

/** Exercise one bounded actual purchase, packet flight, completion or old Earth path; no orbital stock, hull, energy or receipt is fabricated. */
TEST_CASE("Ordinary paid Space purchases retain real facility and native callback owners", "[.manufacturing-purchase-native]")
{
	const char *source = std::getenv("OTTD_PURCHASE_SOURCE"), *mode = std::getenv("OTTD_PURCHASE_ACTION"); REQUIRE(source != nullptr); REQUIRE(mode != nullptr);
	const char *selected = std::getenv("OTTD_PURCHASE_MODEL"); const auto parsed = ParseInteger<uint16_t>(selected == nullptr ? "15" : selected); REQUIRE(parsed.has_value());
	const auto model = *parsed; const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "manufacturing-purchase-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe the actual loaded paid world and preserve assertion failures through ordinary native application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths); SpaceStructure *factory = nullptr;
			// Reuse the original physical factory incarnation, not a generated test ID or a private module registry.
			for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Assembly) { REQUIRE(factory == nullptr); factory = site; }
			REQUIRE(factory != nullptr); REQUIRE(factory->module != nullptr); const auto factory_ref = GetSpaceStructureReference(*factory);
			const auto tile = TileXYL(factory->state.x, factory->state.y, MapLayer::Space); const auto engine = GetSpaceEngine(model); REQUIRE(IsSpacePurchaseEngine(engine));
			const auto &recipe = *GetSpaceVehicleConstruction(model); const auto hulls = GetSpaceVehicleCount();
			auto *origin = ResolveLaunchCenter(world.rocket->construction.home); REQUIRE(origin != nullptr); const auto home = GetLaunchCenterReference(*origin);
			/** @param name Explicit accepted boundary basename. Save only the actual fully validated native owners. */
			auto save = [&](std::string_view name) { world.Save(fmt::format("purchase-{}-{}-427.sav", name, model)); };
			if (action == "prepared") {
				if (factory->module->manufacture) {
					REQUIRE(factory->module->manufacture->phase == OrbitalManufacturePhase::Complete); const auto original = factory->module->manufacture->product;
					REQUIRE(Command<Commands::CancelOrbitalManufacture>::Post(factory->index, factory->generation, world.core->index, world.core->generation));
					CHECK(ResolveSpaceCraft(original) != nullptr); CHECK(GetSpaceVehicleCount() == hulls);
				}
				// A second heavy product needs a new complete delivered bill. Declare only initial Surface scenario content, then ship it through four actual paid flights below.
				if (model == 16 || model == 17) {
					for (const auto &input : recipe.inputs) PurchaseSurfaceStartingContent(*origin, input.label, static_cast<uint16_t>(input.quantity));
					const auto &budget = GetMultiLayerSpaceBudget();
					PurchaseSurfaceStartingContent(*origin, CargoLabel{"PROP"}, static_cast<uint16_t>((budget.outbound_fuel + budget.recovery_fuel) * recipe.inputs.size()));
				}
				CHECK(factory->module->manufacture == nullptr); CHECK(GetSpaceVehicleCount() == hulls); save(action);
			} else if (action == "steel" || action == "goods" || action == "parts" || action == "electronics") {
				// Only prepared heavy scenarios need extra traffic; source Surface packets, actual fuel, original rocket and precise Core admission remain authoritative.
				REQUIRE((model == 16 || model == 17)); REQUIRE(factory->module->manufacture == nullptr);
				const size_t index = action == "steel" ? 0 : action == "goods" ? 1 : action == "parts" ? 2 : 3; const auto &input = recipe.inputs[index];
				const auto cargo = GetCargoTypeByLabel(input.label); const auto before = world.core->module->stock[cargo].TotalCount(); const auto missions = world.rocket->transition->missions_completed;
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(home.station, home.generation, world.rocket->index, world.rocket->generation, cargo, static_cast<uint16_t>(input.quantity), false));
				const auto &budget = GetMultiLayerSpaceBudget();
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, world.rocket->index, world.rocket->generation, uint8_t{0}, static_cast<uint16_t>(budget.outbound_fuel + budget.recovery_fuel), false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(world.rocket->index, world.rocket->generation, SpaceOrderTargetKind::Structure, uint32_t{world.core->index.base()}, world.core->generation));
				/** @return Whether this actual original rocket completed its real paid delivery and recovery, without an elapsed-time substitute. */
				world.Await([&] { return world.rocket->transition->missions_completed == missions + 1; });
				CHECK(world.core->module->stock[cargo].TotalCount() == before + input.quantity); CHECK(GetSpaceVehicleCount() == hulls); save(action);
			} else if (action == "depart") {
				REQUIRE(factory->module->manufacture == nullptr); SpaceVehicle *original = nullptr;
				// Free the output through the real prior product's typed movement, never a fixture pose or hidden temporary hull.
				for (auto *craft : SpaceVehicle::Iterate()) if (craft->construction.assembly == factory_ref) { REQUIRE(original == nullptr); original = craft; }
				REQUIRE(original != nullptr); const auto original_ref = GetSpaceCraftReference(*original); world.Order(*original, *world.core, SpaceOrderAction::Visit);
				/** @return Whether that same prior paid product reached the real Core berth through native movement. */
				world.Await([&] { return GetSpaceStructureBerth(*original, *world.core).has_value(); });
				CHECK(ResolveSpaceCraft(original_ref) == original); CHECK(IsSpaceStructureManufactureBerthFree(*factory)); CHECK(GetSpaceVehicleCount() == hulls); save(action);
			} else if (action == "queued") {
				REQUIRE(factory->module->manufacture == nullptr); const auto serial = factory->module->manufacture_serial;
				const auto before = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest();
				const auto money = Company::Get(_current_company)->money; const auto random = _random;
				auto [quote, vehicle, capacity, mail, capacities] = CmdBuildVehicle({}, tile, engine, true, INVALID_CARGO, ClientID::Invalid);
				REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == Money{recipe.capital}); CHECK(vehicle == VehicleID::Invalid());
				CHECK(ComputeSpaceVehicleDigest() == before); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == utility);
				CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				PreparePurchaseNativeRenderer();
				REQUIRE(Command<Commands::BuildVehicle>::Post(CcBuildPrimaryVehicle, tile, engine, true, INVALID_CARGO, ClientID::Invalid));
				REQUIRE(factory->module->manufacture != nullptr); const auto &job = *factory->module->manufacture;
				CHECK(job.phase == OrbitalManufacturePhase::Queued); CHECK(job.model == model); CHECK(job.serial == serial + 1); CHECK(job.source == GetSpaceStructureReference(*world.core));
				CHECK(GetSpaceVehicleCount() == hulls); CHECK(Company::Get(_current_company)->money == money - recipe.capital);
				CHECK(FindWindowById(WindowClass::SpaceConstruction, 0) != nullptr); const auto issued = ComputeSpaceStructureDigest();
				CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, tile, engine, true, INVALID_CARGO, ClientID::Invalid)).Failed());
				CHECK(ComputeSpaceStructureDigest() == issued); CHECK(Company::Get(_current_company)->money == money - recipe.capital); CHECK(GetSpaceVehicleCount() == hulls); save(action);
			} else if (action == "earth" || action == "earth-complete") {
				// Preserve the original pre-station producer: it creates a real paid Assembling hull immediately, with actual launch provenance and no orbital factory receipt.
				REQUIRE(model == 15); const auto zone = GetLaunchFacilityZone(origin->launch_center->tile, origin->launch_center->rotation, LaunchFacilityZone::Workshop); REQUIRE(zone.has_value());
				if (action == "earth") {
					for (const auto &input : recipe.inputs) PurchaseSurfaceStartingContent(*origin, input.label, static_cast<uint16_t>(input.quantity));
					const auto before = ComputeSpaceVehicleDigest(); const auto money = Company::Get(_current_company)->money;
					REQUIRE(std::get<0>(CmdBuildVehicle({}, zone->tile, engine, true, INVALID_CARGO, ClientID::Invalid)).Succeeded()); CHECK(ComputeSpaceVehicleDigest() == before);
					PreparePurchaseNativeRenderer(); REQUIRE(Command<Commands::BuildVehicle>::Post(CcBuildPrimaryVehicle, zone->tile, engine, true, INVALID_CARGO, ClientID::Invalid));
					auto *product = ResolveSpaceCraft(origin->launch_operations->workshop); REQUIRE(product != nullptr);
					CHECK(product->construction.placement == SpacePlacement::Assembling); CHECK(product->construction.home == home); CHECK(product->construction.origin == home);
					CHECK(product->construction.assembly == SpaceStructureRef{}); CHECK(product->construction.capital_paid == recipe.capital); CHECK(GetSpaceVehicleCount() == hulls + 1);
					CHECK(Company::Get(_current_company)->money == money - recipe.capital);
				} else {
					auto *product = ResolveSpaceCraft(origin->launch_operations->workshop); REQUIRE(product != nullptr); const auto original = GetSpaceCraftReference(*product);
					world.Advance(product->construction.remaining_ticks); CHECK(ResolveSpaceCraft(original) == product); CHECK(product->construction.placement == SpacePlacement::SurfaceStored);
					CHECK(GetSpaceVehicleCount() == hulls); CHECK(product->construction.capital_paid == recipe.capital); CHECK(origin->launch_operations->workshop == SpaceCraftRef{});
				}
				save(action);
			} else if (action == "guards") {
				REQUIRE(factory->module->manufacture == nullptr); REQUIRE(Company::CanAllocateItem()); auto *other = Company::Create(); SetCompanyManagerFaceStyle(other->face, 0);
				const auto before = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest(); const auto random = _random;
				const auto money = Company::Get(_current_company)->money;
				{
					AutoRestoreBackup foreign(_current_company, other->index);
					CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, tile, engine, true, INVALID_CARGO, ClientID::Invalid)).Failed());
				}
				CHECK(std::get<0>(CmdBuildVehicle({}, INVALID_TILE, engine, true, INVALID_CARGO, ClientID::Invalid)).Failed());
				CHECK(std::get<0>(CmdBuildVehicle({}, TileXYL(world.core->state.x, world.core->state.y, MapLayer::Space), engine, true, INVALID_CARGO, ClientID::Invalid)).Failed());
				CHECK(std::get<0>(CmdBuildVehicle({}, tile, engine, true, GetCargoTypeByLabel(CargoLabel{"GOOD"}), ClientID::Invalid)).Failed());
				CHECK(ComputeSpaceVehicleDigest() == before); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == utility);
				CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]); delete other; save(action);
			} else {
				REQUIRE(factory->module->manufacture != nullptr); auto &job = *factory->module->manufacture; REQUIRE(job.model == model);
				if (action == "reserved") {
					REQUIRE(job.phase == OrbitalManufacturePhase::Queued); world.Advance(1); CHECK(job.phase == OrbitalManufacturePhase::Reserved); CHECK(job.work_ticks == 0); CHECK(GetSpaceVehicleCount() == hulls);
				} else if (action == "before" || action == "complete") {
					// Both checkpoints advance only genuine public work; the final opportunity alone may embody every input in one actual new native product.
					const auto target = action == "before" ? recipe.work_ticks - 1 : recipe.work_ticks; REQUIRE(job.work_ticks < target);
					const auto remaining = target - job.work_ticks; const auto spent = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j; world.Advance(remaining);
					CHECK(job.work_ticks == target); CHECK(job.work_energy_j == uint64_t{target} * 750);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == spent + uint64_t{remaining} * 750);
					if (action == "complete") {
						REQUIRE(job.phase == OrbitalManufacturePhase::Complete); auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
						CHECK(GetSpaceVehicleCount() == hulls + 1); CHECK(GetSpaceVehicleModel(*product) == model); CHECK(product->construction.assembly == factory_ref); CHECK(product->construction.assembly_serial == job.serial);
						// The same complete original batch is consumed once; unused slots remain empty and every consumed receipt belongs to this actual new hull.
						for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) { CHECK(job.inputs[input].TotalCount() == 0); CHECK(job.consumed[input] == (input < recipe.inputs.size() ? recipe.inputs[input].quantity : 0)); }
					} else { CHECK(job.phase == OrbitalManufacturePhase::Working); CHECK(GetSpaceVehicleCount() == hulls); }
				} else if (action == "repeat") {
					REQUIRE(job.phase == OrbitalManufacturePhase::Complete); const auto original = job.product; const auto paid = job.work_energy_j;
					world.Advance(3); { AutoRestoreBackup pause(_pause_mode, PauseModes{}); TickOrbitalManufacturingWork(); TickOrbitalManufacturingWork(); }
					CHECK(job.product == original); CHECK(job.work_energy_j == paid); CHECK(GetSpaceVehicleCount() == hulls);
				} else {
					REQUIRE(action == "cancel"); REQUIRE(job.phase == OrbitalManufacturePhase::Working); std::array<uint, MAX_ORBITAL_MANUFACTURE_INPUTS> expected{};
					// Cancelling a purchased job returns its whole still-unconsumed real bill; original capital and already consumed common energy remain spent.
					for (size_t input = 0; input < recipe.inputs.size(); ++input) expected[input] = world.core->module->stock[GetCargoTypeByLabel(recipe.inputs[input].label)].TotalCount() + job.inputs[input].TotalCount();
					const auto money = Company::Get(_current_company)->money; const auto energy = ComputeUtilityPowerDigest(), serial = job.serial;
					REQUIRE(Command<Commands::CancelOrbitalManufacture>::Post(factory->index, factory->generation, world.core->index, world.core->generation));
					CHECK(factory->module->manufacture == nullptr); CHECK(factory->module->manufacture_serial == serial); CHECK(GetSpaceVehicleCount() == hulls);
					CHECK(Company::Get(_current_company)->money == money); CHECK(ComputeUtilityPowerDigest() == energy);
					for (size_t input = 0; input < recipe.inputs.size(); ++input) CHECK(world.core->module->stock[GetCargoTypeByLabel(recipe.inputs[input].label)].TotalCount() == expected[input]);
				}
				save(action);
			}
			REQUIRE(ValidateSpaceVehicles());
			// These actual queued/pre/post controls show the physical paid job, never an immediate proxy vehicle returned by the purchase callback.
			if (action == "queued" || action == "before" || action == "complete") {
				if (action != "queued") PreparePurchaseNativeRenderer();
				ShowSpaceConstructionWindow(factory_ref); auto *window = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(window != nullptr);
				CaptureNativeArtWindow(*window, paths.images / fmt::format("purchase-{}-{}.ppm", action, model));
			}
			fmt::print("MANUFACTURE_PURCHASE native=true phase={} model={} paid_producer=true original_facility=true procurement=false\n", action, model);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(purchase_observer, &observe); IConsole::CmdRegister("manufacturing_purchase_test", ObserveManufacturingPurchase);
	const std::string input{source}, configuration = config.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", input};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
