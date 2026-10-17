/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_clone_native.cpp Bounded real paid clone, original asset and immutable request continuation observers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BB
 * Independent scenarios declare initial Surface content before real traffic; no orbital packet, capital, hull, receipt or pose is granted.
 */
#include "../stdafx.h"
#include "assembly_native_fixture.h"
#include "../space_clone.h"
#include "../space_manifest.h"
#include "../orbital_manufacturing.h"
#include "../vehicle_cmd.h"
#include "../vehicle_func.h"
#include "../engine_base.h"
#include "../depot_cmd.h"
#include "../launch_operations.h"
#include "../station_base.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../console_internal.h"
#include "../core/string_consumer.hpp"
#include "../core/random_func.hpp"
#include "../timer/timer_game_economy.h"
#include "../window_gui.h"
#include "../window_func.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include "../safeguards.h"

static std::function<void()> *clone_observer = nullptr; ///< One explicit current loaded-world observer; never a replacement simulation.

/** @param args Actual native console tokens. @return Whether the exact installed loaded-world observer ran. */
static bool ObserveManufacturingClone(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(clone_observer != nullptr); (*clone_observer)(); return true;
}

/** Select the actual native renderer for callback/capture inspection without advancing a gameplay owner or creating artwork. */
static void PrepareCloneNativeRenderer()
{
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
}

/** Declare missing independent initial Surface scenario content before any new freight or clone request.
 * @param origin Actual original Surface launch station.
 * @param label Existing selected-profile cargo identity, not an invented registry entry.
 * @param quantity Required starting availability; existing genuine stock is retained.
 */
static void CloneSurfaceStartingContent(Station &origin, CargoLabel label, uint16_t quantity)
{
	const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo)); auto &stock = origin.goods[cargo].GetOrCreateData().cargo;
	if (stock.AvailableCount() >= quantity) return;
	const auto missing = static_cast<uint16_t>(quantity - stock.AvailableCount()); REQUIRE(CargoPacket::CanAllocateItem());
	stock.Append(CargoPacket::Create(origin.index, missing, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
	fmt::print("CLONE_SURFACE_FIXTURE cargo={} added={}\n", cargo, missing);
}

/** Hash actual source asset identities/quantities while ordinary elapsed cargo aging remains with its normal controller.
 * @param source Actual original hull; neither source name/orders nor naturally advancing transit ages are asset transfers.
 * @return Stable original freight/service ownership digest, including packet provenance and the previously owned return allowance.
 */
static uint64_t CloneSourceAssets(const SpaceVehicle &source)
{
	uint64_t hash = 1469598103934665603ULL;
	/** @param value One actual scalar owner field. */
	auto add = [&](uint64_t value) { hash ^= value; hash *= 1099511628211ULL; };
	/** @param packet Actual accountable original packet; age is deliberately left to normal elapsed-time aging. */
	auto packet = [&](const CargoPacket &packet) {
		add(packet.index.base()); add(packet.Count()); add(static_cast<uint64_t>(static_cast<int64_t>(packet.GetFeederShare())));
		add(packet.GetSourceTile().base()); add(packet.GetFirstStation().base()); add(to_underlying(packet.GetSource().type)); add(packet.GetSource().id);
	};
	add(source.construction.capital_paid); add(source.cargo_type); add(source.cargo.TotalCount());
	// Every existing original freight packet retains its identity/provenance; a clone cannot consume it as a recipe input or recreate it in another hull.
	for (const auto *owned : *source.cargo.Packets()) packet(*owned);
	add(source.manifest != nullptr);
	if (source.manifest) {
		const auto &manifest = *source.manifest; add(manifest.returns_remaining); add(manifest.pending_returns); add(manifest.services_completed);
		// Fuel/parts/electronics remain the original physical service owner even when source company/name/orders later change.
		for (const auto &stock : manifest.supplies) {
			add(stock.TotalCount());
			for (const auto &[hop, packets] : *stock.Packets()) { add(hop.base()); for (const auto *owned : packets) packet(*owned); }
		}
	}
	return hash;
}

/** Give one actual hull a real natural-body Visit so it releases an old Core berth through ordinary navigation.
 * @param craft Actual owned deployed hull, never a geometric fixture allocation.
 * @param body Existing public natural-body identity from the loaded field.
 */
static void CloneVisitBody(SpaceVehicle &craft, OrbitalBodyID body)
{
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, body.base(), uint64_t{0}));
	if (craft.vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Post(craft.tile, craft.index, true));
}

/** Exercise one actual original-world clone boundary; each child is independently bounded by its invocation wrapper. */
TEST_CASE("Ordinary Space clones preserve actual paid source and new physical product owners", "[.manufacturing-clone-native]")
{
	const char *input = std::getenv("OTTD_CLONE_SOURCE"), *mode = std::getenv("OTTD_CLONE_ACTION"); REQUIRE(input != nullptr); REQUIRE(mode != nullptr);
	const char *selected = std::getenv("OTTD_CLONE_MODEL"); const auto parsed = ParseInteger<uint16_t>(selected == nullptr ? "15" : selected); REQUIRE(parsed.has_value());
	const auto model = *parsed; const std::string_view action{mode}; const auto paths = GetTestCaseArtifactPaths("space", "manufacturing-clone-native");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Observe only the genuinely loaded current snapshot; preserve assertion failures through normal native shutdown. */
	std::function<void()> observe = [&] {
		try {
			// Keep the actual takeover's saved ownership while restoring this observer's local/current UI actors together before the outer application input loop resumes.
			AutoRestoreBackup actor(_current_company, _local_company); AutoRestoreBackup local(_local_company, _local_company);
			AssemblyNativeWorld world(paths); SpaceStructure *factory = nullptr;
			// Discover the one actual original factory incarnation from its public owner; numerical fixture IDs or screen coordinates grant no facility.
			for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Assembly) { REQUIRE(factory == nullptr); factory = site; }
			REQUIRE(factory != nullptr); REQUIRE(factory->module != nullptr); const auto factory_ref = GetSpaceStructureReference(*factory);
			// Loaded takeover checkpoints can select another local player. The normal player switch aligns both command and simulation actors before any synchronized work opportunity.
			SetLocalCompany(factory->state.owner);
			const auto tile = TileXYL(factory->state.x, factory->state.y, MapLayer::Space); const auto *definition = GetSpaceVehicleConstruction(model); REQUIRE(definition != nullptr); const auto &recipe = *definition;
			const auto home = world.rocket->construction.home; auto *origin = ResolveLaunchCenter(home); REQUIRE(origin != nullptr); const auto hulls = GetSpaceVehicleCount();
			SpaceCraftRef original, built;
			/** @param phase Exact current operation basename; save only actual native owners/versioned receipts. */
			auto save = [&](std::string_view phase) { world.Save(fmt::format("clone-{}-{}-428.sav", phase, model)); };
			// Both dated-input operations exercise absent clone intent: ordinary reconstruction and one already paid legacy final milestone.
			const bool legacy = action == "upgrade" || action == "legacy-complete";
			if (legacy) {
				REQUIRE(factory->module->manufacture != nullptr); auto &job = *factory->module->manufacture; REQUIRE(job.model == model); CHECK_FALSE(job.clone.has_value());
				if (action == "legacy-complete") { REQUIRE(job.phase == OrbitalManufacturePhase::Working); world.Advance(1); CHECK(job.phase == OrbitalManufacturePhase::Complete); CHECK_FALSE(job.clone.has_value()); }
				save(action);
			} else if (action == "prepared") {
				REQUIRE(factory->module->manufacture != nullptr); REQUIRE(factory->module->manufacture->phase == OrbitalManufacturePhase::Complete);
				original = factory->module->manufacture->product; auto *source = ResolveSpaceCraft(original); REQUIRE(source != nullptr); REQUIRE(GetSpaceVehicleModel(*source) == model);
				// Starting Surface content precedes new traffic. Model15 retains another independent Earth bill; model20 ships real source Goods/RFMT/ELEC assets separately from its fresh clone recipe.
				for (const auto &part : recipe.inputs) {
					const auto quantity = part.quantity * (model == 15 ? 2 : 1) + (model == 20 && part.label == CargoLabel{"GOOD"} ? 1 : 0);
					CloneSurfaceStartingContent(*origin, part.label, static_cast<uint16_t>(quantity));
				}
				if (model == 20) { CloneSurfaceStartingContent(*origin, CargoLabel{"RFMT"}, 1); CloneSurfaceStartingContent(*origin, CargoLabel{"ELEC"}, 1); }
				const auto flights = recipe.inputs.size() + (model == 20 ? 2 : 0);
				CloneSurfaceStartingContent(*origin, CargoLabel{"PROP"}, static_cast<uint16_t>(flights * 15));
				REQUIRE(Command<Commands::CancelOrbitalManufacture>::Post(factory->index, factory->generation, world.core->index, world.core->generation));
				REQUIRE(Command<Commands::RenameVehicle>::Post(source->index, std::string{"Clone input 07"}));
				CHECK(ResolveSpaceCraft(original) == source); CHECK(GetSpaceVehicleCount() == hulls); save(action);
			} else {
				const auto *id = std::getenv("OTTD_CLONE_ORIGINAL_ID"), *generation = std::getenv("OTTD_CLONE_ORIGINAL_GENERATION"); REQUIRE(id != nullptr); REQUIRE(generation != nullptr);
				const auto parsed_id = ParseInteger<uint32_t>(id); const auto parsed_generation = ParseInteger<uint64_t>(generation);
				REQUIRE(parsed_id.has_value()); REQUIRE(parsed_generation.has_value()); original = {VehicleID{*parsed_id}, *parsed_generation};
				auto *source = ResolveSpaceCraft(original); const auto assets = source == nullptr ? 0 : CloneSourceAssets(*source);
				if (action == "clear-core") {
					REQUIRE(!GetAsteroidField().bodies.empty()); const auto body = GetAsteroidField().bodies.front().id;
					std::vector<SpaceCraftRef> departing;
					// Later accepted producer worlds may already have an older product at Core. Move every actual claimant through typed navigation before new freight; never teleport it out of the admission boundary.
					for (auto *craft : SpaceVehicle::Iterate()) if (craft->space_orders.approach && craft->space_orders.approach->target == SpaceOrderTarget{SpaceOrderTargetKind::Structure, world.core->index.base(), world.core->generation}) {
						departing.push_back(GetSpaceCraftReference(*craft)); CloneVisitBody(*craft, body);
					}
					/** @return Whether real movement has cleared the Core's physical arrival neighborhood, not merely removed a reservation record. */
					world.Await([&] {
						for (const auto ref : departing) { const auto *craft = ResolveSpaceCraft(ref); if (craft != nullptr && SpaceDistanceSquared(craft->position, GetSpaceStructureApproach(world.core->state)) <= uint64_t{4} * SPACE_Q16 * SPACE_Q16) return false; }
						return true;
					});
					save(action);
				// Each independent freight operation carries one actual recipe or source-service component through a real fuel-paid round trip before a clone can reserve it.
				} else if (action == "steel" || action == "goods" || action == "parts" || action == "electronics" || action == "service-metal" || action == "service-electronics") {
					CargoLabel label; uint16_t quantity;
					// Model20's source service stores are separate accountable assets, delivered independently of its Steel/Goods clone bill.
					if (action == "service-metal" || action == "service-electronics") {
						REQUIRE(model == 20); label = action == "service-metal" ? CargoLabel{"RFMT"} : CargoLabel{"ELEC"}; quantity = 1;
					} else {
						const size_t index = action == "steel" ? 0 : action == "goods" ? 1 : action == "parts" ? 2 : 3; REQUIRE(index < recipe.inputs.size());
						label = recipe.inputs[index].label; quantity = static_cast<uint16_t>(recipe.inputs[index].quantity + (model == 20 && label == CargoLabel{"GOOD"} ? 1 : 0));
					}
					const auto cargo = GetCargoTypeByLabel(label); const auto before = world.core->module->stock[cargo].TotalCount(); const auto missions = world.rocket->transition->missions_completed;
					REQUIRE(Command<Commands::MoveLaunchFreight>::Post(home.station, home.generation, world.rocket->index, world.rocket->generation, cargo, quantity, false));
					REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, world.rocket->index, world.rocket->generation, uint8_t{0}, uint16_t{15}, false));
					REQUIRE(Command<Commands::BeginLaunchFlight>::Post(world.rocket->index, world.rocket->generation, SpaceOrderTargetKind::Structure, uint32_t{world.core->index.base()}, world.core->generation));
					/** @return Whether this exact original reusable rocket completed actual paid delivery and recovery. */
					world.Await([&] { return world.rocket->transition->missions_completed == missions + 1; });
					CHECK(world.core->module->stock[cargo].TotalCount() == before + quantity); CHECK(GetSpaceVehicleCount() == hulls); save(action);
				} else if (action == "depart") {
					REQUIRE(source != nullptr); world.Order(*source, *world.core, SpaceOrderAction::Visit);
					/** @return Whether this actual original product arrived at the Core through native typed movement. */
					world.Await([&] { return GetSpaceStructureBerth(*source, *world.core).has_value(); });
					if (model == 20) {
						REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(world.core->index, world.core->generation, source->index, source->generation, GetCargoTypeByLabel(CargoLabel{"GOOD"}), uint16_t{1}, true));
						// These are real source-owned service packets; they are loaded before stopping, through the existing idle precise-berth owner rather than fixture compartment writes.
						for (uint8_t supply : {1, 2}) REQUIRE(Command<Commands::MoveOrbitalSupply>::Post(world.core->index, world.core->generation, source->index, source->generation, supply, uint16_t{1}, true));
					}
					if (!source->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Post(source->tile, source->index, true));
					CHECK(IsSpaceStructureManufactureBerthFree(*factory)); CHECK(GetSpaceVehicleCount() == hulls); save(action);
				} else if (action == "queued") {
					REQUIRE(source != nullptr); REQUIRE(factory->module->manufacture == nullptr); const auto money = Company::Get(_current_company)->money; const auto random = _random;
					const auto structures = ComputeSpaceStructureDigest(), next = GetNextSpaceVehicleGeneration();
					auto [quote, vehicle] = CmdCloneVehicle({}, tile, source->index, false); REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == Money{recipe.capital}); CHECK(vehicle == VehicleID::Invalid());
					CHECK(ComputeSpaceStructureDigest() == structures); CHECK(GetNextSpaceVehicleGeneration() == next); CHECK(Company::Get(_current_company)->money == money);
					CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]); PrepareCloneNativeRenderer();
					REQUIRE(Command<Commands::CloneVehicle>::Post(CcCloneVehicle, tile, source->index, false));
					REQUIRE(factory->module->manufacture != nullptr); const auto &job = *factory->module->manufacture; REQUIRE(job.clone.has_value());
					CHECK(job.clone->source == original); CHECK(job.clone->name == source->name); CHECK(job.clone->orders == source->space_orders.entries);
					CHECK(job.phase == OrbitalManufacturePhase::Queued); CHECK(job.product == SpaceCraftRef{}); CHECK(GetSpaceVehicleCount() == hulls); CHECK(GetNextSpaceVehicleGeneration() == next);
					CHECK(Company::Get(_current_company)->money == money - Money{recipe.capital}); CHECK(FindWindowById(WindowClass::SpaceConstruction, 0) != nullptr);
					CHECK(std::get<0>(CmdCloneVehicle(DoCommandFlag::Execute, tile, source->index, false)).Failed()); CHECK(CloneSourceAssets(*source) == assets); save(action);
				// Original Earth assembly owns its real immediate hull; copied nonempty orbital instructions must refuse before that distinct commissioning path pays or consumes.
				} else if (action == "earth" || action == "earth-complete" || action == "guards") {
					REQUIRE(model == 15); REQUIRE(source != nullptr); const auto zone = GetLaunchFacilityZone(origin->launch_center->tile, origin->launch_center->rotation, LaunchFacilityZone::Workshop); REQUIRE(zone.has_value());
					if (action == "guards") {
						const auto digest = ComputeSpaceStructureDigest(), fleet = ComputeSpaceVehicleDigest(), utility = ComputeUtilityPowerDigest(); const auto money = Company::Get(_current_company)->money;
						REQUIRE(!source->space_orders.entries.empty()); REQUIRE(CmdAssembleLaunchCraft({}, home.station, home.generation, model).Succeeded());
						CHECK(std::get<0>(CmdCloneVehicle(DoCommandFlag::Execute, zone->tile, source->index, false)).Failed()); CHECK(std::get<0>(CmdCloneVehicle(DoCommandFlag::Execute, tile, source->index, true)).Failed());
						REQUIRE(Company::CanAllocateItem()); auto *other = Company::Create(); SetCompanyManagerFaceStyle(other->face, 0);
						{
							AutoRestoreBackup foreign(_current_company, other->index);
							CHECK(std::get<0>(CmdCloneVehicle(DoCommandFlag::Execute, tile, source->index, false)).Failed());
							CHECK(std::get<0>(CmdCloneVehicle(DoCommandFlag::Execute, tile, source->index, true)).Failed());
						}
						delete other;
						CHECK(ComputeSpaceStructureDigest() == digest); CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(Company::Get(_current_company)->money == money);
					} else if (action == "earth") {
						REQUIRE(Command<Commands::EditSpaceOrder>::Post(source->index, source->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
						const auto money = Company::Get(_current_company)->money; PrepareCloneNativeRenderer();
						REQUIRE(Command<Commands::CloneVehicle>::Post(CcCloneVehicle, zone->tile, source->index, false));
						built = origin->launch_operations->workshop; auto *product = ResolveSpaceCraft(built); REQUIRE(product != nullptr);
						CHECK(product->construction.placement == SpacePlacement::Assembling); CHECK(product->construction.home == home); CHECK(product->construction.assembly == SpaceStructureRef{});
						CHECK(product->name == "Clone input 08"); CHECK(product->space_orders.entries.empty()); CHECK(GetSpaceVehicleCount() == hulls + 1); CHECK(Company::Get(_current_company)->money == money - Money{recipe.capital});
					} else {
						built = origin->launch_operations->workshop; auto *product = ResolveSpaceCraft(built); REQUIRE(product != nullptr); world.Advance(product->construction.remaining_ticks);
						CHECK(ResolveSpaceCraft(built) == product); CHECK(product->construction.placement == SpacePlacement::SurfaceStored); CHECK(product->name == "Clone input 08"); CHECK(GetSpaceVehicleCount() == hulls);
					}
					save(action);
				} else {
					REQUIRE(factory->module->manufacture != nullptr); auto &job = *factory->module->manufacture; REQUIRE(job.clone.has_value()); CHECK(job.clone->source == original);
					if (action == "reserved") { REQUIRE(job.phase == OrbitalManufacturePhase::Queued); world.Advance(1); CHECK(job.phase == OrbitalManufacturePhase::Reserved); CHECK(job.work_ticks == 0); }
					// Both checkpoints advance only original common-grant work; the final opportunity alone may create one stopped independently configured product.
					else if (action == "before" || action == "complete") {
						const auto target = action == "before" ? recipe.work_ticks - 1 : recipe.work_ticks; REQUIRE(job.work_ticks < target);
						const auto owner = factory->state.owner; const auto year = TimerGameEconomy::year; const auto capital = Company::Get(owner)->yearly_expenses[0][ExpensesType::NewVehicles];
						const auto name = MakeClonedVehicleName(job.clone->name); const auto entries = job.clone->orders;
						world.Advance(target - job.work_ticks); CHECK(job.work_ticks == target); CHECK(job.work_energy_j == uint64_t{target} * 750);
						// Real elapsed work also runs ordinary fleet operating costs. Only the existing vehicle-purchase expense owner can prove that the already paid request receives no second capital debit.
						if (TimerGameEconomy::year == year) {
							CHECK(Company::Get(owner)->yearly_expenses[0][ExpensesType::NewVehicles] == capital);
						} else {
							// A bounded work child can cross one economy year; the authoritative ledger rotates the old capital column and starts the new one at zero.
							CHECK(TimerGameEconomy::year.base() == year.base() + 1); CHECK(Company::Get(owner)->yearly_expenses[1][ExpensesType::NewVehicles] == capital);
							CHECK(Company::Get(owner)->yearly_expenses[0][ExpensesType::NewVehicles] == Money{0});
						}
						if (action == "complete") {
							REQUIRE(job.phase == OrbitalManufacturePhase::Complete); built = job.product; auto *product = ResolveSpaceCraft(built); REQUIRE(product != nullptr);
							CHECK(GetSpaceVehicleCount() == hulls + 1); CHECK(product->name == name); CHECK(product->space_orders.entries == entries); CHECK(product->space_orders.cursor == 0);
							CHECK_FALSE(product->space_orders.started); CHECK(product->space_orders.work_ticks == 0); CHECK(product->space_orders.timer_ticks == 0); CHECK(product->vehstatus.Test(VehState::Stopped));
							CHECK(product->cargo.TotalCount() == 0); CHECK(product->cargo_type == product->GetEngine()->GetDefaultCargoType()); CHECK(GetSpaceStructureBerth(*product, *factory) == std::optional<uint8_t>{0});
							// Every common/heavy input belongs to this actual new product once; source freight and service packets are never substituted for the bill.
							for (size_t part = 0; part < MAX_ORBITAL_MANUFACTURE_INPUTS; ++part) { CHECK(job.inputs[part].TotalCount() == 0); CHECK(job.consumed[part] == (part < recipe.inputs.size() ? recipe.inputs[part].quantity : 0)); }
							REQUIRE(product->manifest != nullptr); for (const auto &stock : product->manifest->supplies) CHECK(stock.TotalCount() == 0);
						} else { CHECK(job.phase == OrbitalManufacturePhase::Working); CHECK(GetSpaceVehicleCount() == hulls); }
					} else if (action == "repeat") {
						REQUIRE(job.phase == OrbitalManufacturePhase::Complete); built = job.product; auto *product = ResolveSpaceCraft(built); REQUIRE(product != nullptr); const auto name = product->name; const auto entries = product->space_orders.entries;
						const auto paid = job.work_energy_j; world.Advance(3); { AutoRestoreBackup pause(_pause_mode, PauseModes{}); TickOrbitalManufacturingWork(); TickOrbitalManufacturingWork(); }
						CHECK(job.product == built); CHECK(product->name == name); CHECK(product->space_orders.entries == entries); CHECK(job.work_energy_j == paid); CHECK(GetSpaceVehicleCount() == hulls);
					} else if (action == "mutate") {
						REQUIRE(source != nullptr); const auto frozen_name = job.clone->name; const auto entries = job.clone->orders;
						REQUIRE(Command<Commands::RenameVehicle>::Post(source->index, std::string{"Changed source 55"})); REQUIRE(!GetAsteroidField().bodies.empty()); CloneVisitBody(*source, GetAsteroidField().bodies.front().id);
						REQUIRE(Command<Commands::StartStopVehicle>::Post(source->tile, source->index, true)); CHECK(job.clone->name == frozen_name); CHECK(job.clone->orders == entries);
					} else if (action == "source-loss") {
						REQUIRE(source != nullptr); REQUIRE(source->cargo.TotalCount() == 0); REQUIRE_FALSE(HasSpaceManifestAssets(*source));
						// Explicit destruction fixture: the pending job reserves the source's factory home, so native sale cannot return there. Destroy this actual empty hull through its real destructor; grant no recipe, refund, pose or replacement identity.
						delete source; source = nullptr;
						CHECK(ResolveSpaceCraft(original) == nullptr); CHECK(job.clone->source == original); CHECK(GetSpaceVehicleCount() == hulls - 1);
					} else if (action == "transfer") {
						REQUIRE(Company::CanAllocateItem()); auto *company = Company::Create(); SetCompanyManagerFaceStyle(company->face, 0); const auto owner = company->index;
						// Transfer through the actual native lifecycle owner; the job was paid before transfer and no new company money, material or receipt is granted.
						ChangeOwnershipOfCompanyItems(_current_company, owner);
						// Native takeover can already switch the local player while restoring the old command actor. Rejoin that actual local actor before the normal player-switch owner enforces its precondition.
						_current_company = _local_company; SetLocalCompany(owner);
						CHECK(factory->state.owner == owner); REQUIRE(ResolveSpaceCraft(original) != nullptr); CHECK(ResolveSpaceCraft(original)->owner == owner);
						REQUIRE(factory->module->manufacture != nullptr); REQUIRE(factory->module->manufacture->clone.has_value()); CHECK(factory->module->manufacture->clone->source == original);
					} else {
						REQUIRE(action == "cancel"); const auto money = Company::Get(_current_company)->money; const auto energy = ComputeUtilityPowerDigest();
						std::array<uint, MAX_ORBITAL_MANUFACTURE_INPUTS> returned{};
						// Cancellation returns the complete new unconsumed recipe to the original inventory while the separately embodied source and its assets remain intact.
						for (size_t part = 0; part < recipe.inputs.size(); ++part) returned[part] = world.core->module->stock[GetCargoTypeByLabel(recipe.inputs[part].label)].TotalCount() + job.inputs[part].TotalCount();
						REQUIRE(Command<Commands::CancelOrbitalManufacture>::Post(factory->index, factory->generation, world.core->index, world.core->generation));
						CHECK(factory->module->manufacture == nullptr); CHECK(Company::Get(_current_company)->money == money); CHECK(ComputeUtilityPowerDigest() == energy); CHECK(GetSpaceVehicleCount() == hulls);
						for (size_t part = 0; part < recipe.inputs.size(); ++part) CHECK(world.core->module->stock[GetCargoTypeByLabel(recipe.inputs[part].label)].TotalCount() == returned[part]);
					}
					if (source != nullptr) CHECK(CloneSourceAssets(*source) == assets);
					save(action);
				}
			}
			REQUIRE(ValidateSpaceVehicles());
			// These are actual queued/pre/post controls of the original factory and once-only paid clone product, not an invented immediate vehicle.
			if (action == "queued" || action == "before" || action == "complete") {
				if (action != "queued") PrepareCloneNativeRenderer(); ShowSpaceConstructionWindow(factory_ref);
				auto *window = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(window != nullptr); CaptureNativeArtWindow(*window, paths.images / fmt::format("clone-{}-{}.ppm", action, model));
			}
			const auto *surviving = ResolveSpaceCraft(original);
			fmt::print("MANUFACTURE_CLONE native=true phase={} model={} original_id={} original_generation={} original_alive={} original_assets={} procurement=false\n",
				action, model, original.vehicle.base(), original.generation, surviving != nullptr, surviving == nullptr ? 0 : CloneSourceAssets(*surviving));
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(clone_observer, &observe); IConsole::CmdRegister("manufacturing_clone_test", ObserveManufacturingClone);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
