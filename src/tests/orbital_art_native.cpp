/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_art_native.cpp Paid module and factory milestones for ordinary presentation review.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13L
 * The retained paid world supplies all stock and hulls. Only public commands and normal game opportunities may change it.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_func.h"
#include "../console_internal.h"
#include "../orbital_manufacturing.h"
#include "../vehicle_cmd.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *art_observer = nullptr; ///< Invocation callback; owns no gameplay state.

/** Invoke the observer only after the original engine has loaded its retained paid world.
 * @param args Parameter-free startup console command. @return Whether the installed observer ran.
 */
static bool ObserveOrbitalArt(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(art_observer); (*art_observer)(); return true;
}

/** Release an actual paid craft's berth through its existing native navigation controller.
 * @param world Loaded native command/tick adapter. @param craft Existing orbital hull, with its original inventory intact.
 */
static void DepartArtBerth(AssemblyNativeWorld &world, SpaceVehicle &craft)
{
	REQUIRE_FALSE(GetAsteroidField().bodies.empty()); const auto before = craft.position;
	if (craft.vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Post(craft.tile, craft.index, true));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, GetAsteroidField().bodies.front().id.base(), uint64_t{0}));
	/** @return Whether normal navigation has physically freed the original point. */
	world.Await([&] { return craft.position != before; });
}

/** Import only the missing canonical inputs into their actual Core stock owner.
 * @param world Retained paid Core, original deployed builders and reusable rocket.
 * @param inputs Original immutable module or vehicle recipe, never an observer-created cargo bill.
 */
static void ImportArtMaterials(AssemblyNativeWorld &world, std::span<const MultiLayerRecipeCargo> inputs)
{
	auto &builder = *world.builders.front();
	// Every missing input must take a paid real rocket flight and leave its
	// actual recipient at the precise Core berth. Existing stock remains finite.
	for (const auto &input : inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label); REQUIRE(IsValidCargoType(cargo));
		const auto held = world.core->module->stock[cargo].TotalCount();
		if (held >= input.quantity) continue;
		REQUIRE(builder.cargo.TotalCount() == 0);
		const auto missing = static_cast<uint16_t>(input.quantity - held);
		world.Import(builder, cargo, missing); world.Order(builder, *world.core, SpaceOrderAction::Visit);
		/** @return Whether normal navigation physically reached the Core's receiving berth. */
		world.Await([&] { return GetSpaceStructureBerth(builder, *world.core).has_value(); });
		REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(world.core->index, world.core->generation, builder.index, builder.generation, cargo, missing, false));
		REQUIRE(builder.cargo.TotalCount() == 0); REQUIRE(world.core->module->stock[cargo].TotalCount() == input.quantity);
	}
}

/** Pay and construct one missing presentation family, retaining original intermediate physical milestones.
 * @param world Actual paid Core, builders and Surface carrier; this helper installs no stock or work.
 * @param kind Original Power, FuelStore or Assembly recipe. @param name Stable checkpoint prefix for this exact paid project.
 */
static void ConstructArtModule(AssemblyNativeWorld &world, SpaceBuildKind kind, std::string_view name)
{
	auto &builder = *world.builders.front(); const auto &definition = *GetSpaceConstructionDefinition(kind);
	const auto money = Company::Get(_current_company)->money; const auto digest = ComputeSpaceStructureDigest();
	TileIndex selected = INVALID_TILE;
	// Quote only the bounded patch adjacent to the actual Core. Its original placement command owns footprint/approach admission; no site or tile is installed by the search.
	for (int dy = -2; dy <= 3 && selected == INVALID_TILE; ++dy) {
		// Stop at the first legal query result without reserving rejected cells, deleting another project or relocating a craft.
		for (int dx = -2; dx <= 3; ++dx) {
			const auto tile = TileXYL(world.core->state.x + dx, world.core->state.y + dy, MapLayer::Space);
			if (CmdBuildSpaceBlueprint({}, tile, kind).Succeeded()) { selected = tile; break; }
		}
	}
	REQUIRE(selected != INVALID_TILE); CHECK(ComputeSpaceStructureDigest() == digest); CHECK(Company::Get(_current_company)->money == money);
	REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(selected, kind));
	SpaceStructure *site = nullptr;
	// Resolve only the one newly issued role, never a locally invented ID or copied facility.
	for (auto *candidate : SpaceStructure::Iterate()) if (candidate->state.kind == kind) { REQUIRE(site == nullptr); site = candidate; }
	REQUIRE(site); CHECK(Company::Get(_current_company)->money == money - definition.capital);
	world.Save(fmt::format("art-{}-blueprint-438.sav", name));
	ImportArtMaterials(world, definition.inputs);
	REQUIRE(Command<Commands::ReserveOrbitalModuleInputs>::Post(site->index, site->generation, world.core->index, world.core->generation));
	CHECK(HasOrbitalConstructionInputs(*site)); CHECK(site->job.work_ticks == 0);
	world.Save(fmt::format("art-{}-reserved-438.sav", name));
	world.Order(builder, *site, SpaceOrderAction::Construct);
	try {
		/** @return Whether real travel and shared paid work have reached a physical half-built milestone. */
		world.Await([&] { return site->job.work_ticks >= definition.work_ticks / 2; });
	} catch (...) {
		// Preserve actual failed admission/travel/work owners for diagnosis;
		// a bounded timeout cannot be converted into synthetic paid progress.
		world.Save(fmt::format("art-{}-wait-438.sav", name));
		fmt::print("ORBITAL_ART_WAIT site={} x={} y={} work={} inputs={} builder={} x={} y={} stopped={} cursor={} orders={} block={} core_w={}\n",
			site->index.base(), site->state.x, site->state.y, site->job.work_ticks, HasOrbitalConstructionInputs(*site), builder.index.base(),
			builder.position.x, builder.position.y, builder.vehstatus.Test(VehState::Stopped), builder.space_orders.cursor,
			builder.space_orders.entries.size(), to_underlying(builder.space_orders.blocked), GetOrbitalModulePanelRate(*world.core));
		throw;
	}
	REQUIRE(site->job.work_ticks < definition.work_ticks); CHECK(site->module == nullptr);
	world.Save(fmt::format("art-{}-partial-438.sav", name));
	/** @return Whether the original construction controller has consumed the delivered bill and completed the frame once. */
	world.Await([&] { return site->state.phase == SpaceStructurePhase::FrameComplete; });
	// Every physical input has exactly one canonical final debit; paid work cannot create a second bill or a free operational module.
	for (size_t i = 0; i < definition.inputs.size(); ++i) { CHECK(site->job.consumed[i] == definition.inputs[i].quantity); CHECK(site->job.inputs[i].TotalCount() == 0); }
	REQUIRE(site->module == nullptr); world.Save(fmt::format("art-{}-frame-438.sav", name));
	// Only these two commissioned roles publish facilities after a complete
	// paid frame. FuelStore retains its explicit unsupported activation boundary.
	if (kind == SpaceBuildKind::Power || kind == SpaceBuildKind::Assembly) {
		REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(site->index, site->generation));
		/** @return Whether original paid activation has published the real module and its single native producer. */
		world.Await([&] { return IsOrbitalModuleOperational(*site); });
		if (kind == SpaceBuildKind::Power) {
			world.Save("art-power-active-438.sav");
			REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(site->index, site->generation, true)); world.Advance(1);
			world.Save("art-power-blocked-438.sav");
			REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(site->index, site->generation, false));
		} else {
			REQUIRE(Command<Commands::ConnectOrbitalServices>::Post(site->index, site->generation, world.core->index, world.core->generation));
			world.Save("art-assembly-active-438.sav");
		}
	} else {
		REQUIRE(kind == SpaceBuildKind::FuelStore);
		CHECK(CmdActivateOrbitalModule({}, site->index, site->generation).Failed()); CHECK(site->module == nullptr);
	}
	DepartArtBerth(world, builder);
}

/** Earn a genuine current paid factory work view instead of relabeling an old completed receipt.
 * @param world Actual retained Core world and its finite Surface stock/carriers; the Assembly must also be paid here.
 */
static void ManufactureArtProduct(AssemblyNativeWorld &world)
{
	const auto &assembly = *GetSpaceConstructionDefinition(SpaceBuildKind::Assembly);
	const auto &recipe = *GetSpaceVehicleConstruction(20);
	REQUIRE(assembly.inputs.size() == 2); REQUIRE(recipe.inputs.size() == 2);
	std::array<MultiLayerRecipeCargo, 2> combined;
	// Ship both original complete bills together while their real packets are
	// available. This avoids an unnecessary later flight without granting fuel,
	// changing either recipe or reserving the same unit for both owners.
	for (size_t i = 0; i < combined.size(); ++i) {
		REQUIRE(assembly.inputs[i].label == recipe.inputs[i].label);
		REQUIRE(assembly.inputs[i].quantity <= UINT16_MAX - recipe.inputs[i].quantity);
		combined[i] = {assembly.inputs[i].label, assembly.inputs[i].quantity + recipe.inputs[i].quantity};
	}
	ImportArtMaterials(world, combined); ConstructArtModule(world, SpaceBuildKind::Assembly, "assembly");
	SpaceStructure *factory = nullptr;
	// Resolve only the original newly paid physical Assembly; lookup cannot
	// publish another facility, product or source/consumer account.
	for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Assembly) { REQUIRE(factory == nullptr); factory = site; }
	REQUIRE(factory); REQUIRE(IsOrbitalModuleOperational(*factory)); REQUIRE(factory->module->manufacture == nullptr);
	const auto hulls = GetSpaceVehicleCount(); const auto money = Company::Get(_current_company)->money;
	REQUIRE(Command<Commands::QueueOrbitalManufacture>::Post(factory->index, factory->generation, world.core->index, world.core->generation, uint16_t{20}));
	CHECK(Company::Get(_current_company)->money == money - recipe.capital);
	REQUIRE(factory->module->manufacture); auto &job = *factory->module->manufacture;
	/** @return Whether the native factory has exclusively reserved every real canonical input. */
	world.Await([&] { return job.phase == OrbitalManufacturePhase::Reserved; });
	CHECK(job.work_ticks == 0); CHECK(GetSpaceVehicleCount() == hulls); world.Save("art-manufacture-reserved-438.sav");
	/** @return Whether normal dedicated shared work reached its physical half-paid milestone. */
	world.Await([&] { return job.work_ticks >= recipe.work_ticks / 2; });
	REQUIRE(job.work_ticks < recipe.work_ticks); CHECK(GetSpaceVehicleCount() == hulls); world.Save("art-manufacture-partial-438.sav");
	/** @return Whether the original final materialization admitted exactly one real paid product. */
	world.Await([&] { return job.phase == OrbitalManufacturePhase::Complete; });
	auto *product = ResolveSpaceCraft(job.product); REQUIRE(product); CHECK(GetSpaceVehicleModel(*product) == 20);
	CHECK(GetSpaceVehicleCount() == hulls + 1); CHECK(job.work_energy_j == uint64_t{recipe.work_ticks} * GetMultiLayerSpaceBudget().work_j_per_tick);
	// Only the original committing tail embodies the complete bill; the observer
	// may not manufacture cargo, consume a prefix or publish another hull.
	for (size_t i = 0; i < recipe.inputs.size(); ++i) { CHECK(job.inputs[i].TotalCount() == 0); CHECK(job.consumed[i] == recipe.inputs[i].quantity); }
	world.Save("art-manufacture-complete-438.sav");
}

/** Construct genuinely paid missing families and save their native physical milestones for the ordinary game. */
TEST_CASE("Paid native orbital art milestones retain their original material owners", "[.orbital-art-native]")
{
	const char *source = std::getenv("OTTD_ORBITAL_ART_SOURCE"); REQUIRE(source);
	const char *selected = std::getenv("OTTD_ORBITAL_ART_MODE"); const bool manufacture = selected != nullptr && std::string_view{selected} == "manufacture";
	const auto paths = GetTestCaseArtifactPaths("orbital-art", "paid-construction"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Retain native assertion failures across normal engine shutdown without manufacturing another gameplay owner. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			REQUIRE(ValidateSpaceStructures()); world.Save("art-initial-438.sav");
			if (world.core->module->producer == UtilityProducerID{UINT32_MAX}) {
				// The accepted inventory-only legacy Core has no commissioned panel.
				// Its original public commissioning command, as in the processor
				// producer, must publish the paid hardware before any shared work.
				REQUIRE(CmdCommissionOrbitalServices({}, world.core->index, world.core->generation).Succeeded());
				REQUIRE(Command<Commands::CommissionOrbitalServices>::Post(world.core->index, world.core->generation));
			}
			REQUIRE(GetOrbitalModulePanelRate(*world.core) == GetMultiLayerSpaceBudget().core_panel_w);
			// Both existing paid builders must actually depart before one may return with transported materials; occupied berths cannot be freed by editing coordinates or service records.
			for (auto *craft : world.builders) DepartArtBerth(world, *craft);
			if (manufacture) {
				ManufactureArtProduct(world);
			} else {
				ConstructArtModule(world, SpaceBuildKind::Power, "power");
				ConstructArtModule(world, SpaceBuildKind::FuelStore, "fuel");
			}
			REQUIRE(ValidateSpaceStructures()); REQUIRE(ValidateSpaceVehicles());
			fmt::print("ORBITAL_ART native=true added_stock=false granted_work=false granted_pose=false {}\n", manufacture ? "paid_products=1" : "paid_modules=2"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(art_observer, &observe); IConsole::CmdRegister("orbital_art_test", ObserveOrbitalArt);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
