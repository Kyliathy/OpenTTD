/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_replacement_native.cpp Bounded loaded-world native paid replacement and once-only original retirement observers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC
 * Inputs are exact accepted paid traffic checkpoints. Only the explicitly reported age-policy fixture invokes daily hull aging without advancing calendar/economy; no hull, stock, funds, work or pose is granted.
 */
#include "../stdafx.h"
#include "assembly_native_fixture.h"
#include "../space_replacement.h"
#include "../autoreplace_cmd.h"
#include "../vehicle_cmd.h"
#include "../vehicle_func.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../group.h"
#include "../company_base.h"
#include "../station_base.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../console_internal.h"
#include "../core/string_consumer.hpp"
#include "../core/random_func.hpp"
#include "../timer/timer_game_economy.h"
#include <cstdlib>
#include "../safeguards.h"

static std::function<void()> *replacement_observer = nullptr; ///< Current explicit loaded-world observer; it never substitutes for a simulation owner.

/** @param args Actual native console tokens. @return Whether the exact installed observer ran against the loaded world. */
static bool ObserveManufacturingReplacement(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(replacement_observer != nullptr); (*replacement_observer)(); return true;
}

/** Hash original whole assets while leaving ordinary elapsed cargo aging with its native owner.
 * @param source Existing original or its actual paid replacement after handover.
 * @return Packet identities/quantities/provenance and retained service/return history, independent of hull identity and editable name/orders.
 */
static uint64_t ReplacementAssets(const SpaceVehicle &source)
{
	uint64_t hash = 1469598103934665603ULL;
	/** @param value Actual scalar asset owner field. */
	const auto add = [&](uint64_t value) { hash ^= value; hash *= 1099511628211ULL; };
	/** @param packet Actual original accountable packet; elapsed transit age is not a transfer. */
	const auto packet = [&](const CargoPacket &packet) {
		add(packet.index.base()); add(packet.Count()); add(static_cast<uint64_t>(static_cast<int64_t>(packet.GetFeederShare())));
		add(packet.GetSourceTile().base()); add(packet.GetFirstStation().base()); add(to_underlying(packet.GetSource().type)); add(packet.GetSource().id);
	};
	add(source.cargo_type); add(source.cargo.TotalCount());
	// Each whole original packet must retain its identity and physical production provenance; no new recipe can consume or relabel these holdings.
	for (const auto *owned : *source.cargo.Packets()) packet(*owned);
	add(source.manifest != nullptr);
	if (source.manifest) {
		const auto &manifest = *source.manifest; add(manifest.returns_remaining); add(manifest.pending_returns); add(manifest.services_completed); add(manifest.fuel_burned);
		// Actual service compartments move as sole owners, retaining original packet/next-hop identities and the used allowance rather than adding a fresh producer budget.
		for (const auto &stock : manifest.supplies) {
			add(stock.TotalCount());
			for (const auto &[hop, packets] : *stock.Packets()) { add(hop.base()); for (const auto *owned : packets) packet(*owned); }
		}
	}
	return hash;
}

/** Run one exact paid original-world request/work/settlement boundary, independently bounded by its invocation wrapper. */
TEST_CASE("Space replacement retains actual original owners until paid native settlement", "[.manufacturing-replacement-native]")
{
	const char *input = std::getenv("OTTD_REPLACEMENT_SOURCE"), *mode = std::getenv("OTTD_REPLACEMENT_ACTION"); REQUIRE(input != nullptr); REQUIRE(mode != nullptr);
	const char *selected = std::getenv("OTTD_REPLACEMENT_MODEL"), *id = std::getenv("OTTD_REPLACEMENT_ORIGINAL_ID"), *generation = std::getenv("OTTD_REPLACEMENT_ORIGINAL_GENERATION");
	REQUIRE(selected != nullptr); REQUIRE(id != nullptr); REQUIRE(generation != nullptr);
	const auto model_value = ParseInteger<uint16_t>(selected); const auto id_value = ParseInteger<uint32_t>(id); const auto generation_value = ParseInteger<uint64_t>(generation);
	REQUIRE(model_value.has_value()); REQUIRE(id_value.has_value()); REQUIRE(generation_value.has_value());
	const auto model = *model_value; const SpaceCraftRef original{VehicleID{*id_value}, *generation_value}; const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "manufacturing-replacement-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe genuinely loaded owners and preserve assertion failures through normal native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AutoRestoreBackup local(_local_company, _local_company);
			AssemblyNativeWorld world(paths); SpaceStructure *factory = nullptr;
			if (const char *corrupt = std::getenv("OTTD_REPLACEMENT_CORRUPT")) {
				const auto fleet = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest(), production = ComputeLaunchProductionDigest();
				const auto random = _random; const auto hulls = GetSpaceVehicleCount();
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeLaunchProductionDigest() == production);
				CHECK(GetSpaceVehicleCount() == hulls); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("MANUFACTURE_REPLACEMENT native=true preview_rejected=true unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			// Select the actual public producer, never a copied private factory ID or a screen coordinate.
			for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Assembly) { REQUIRE(factory == nullptr); factory = site; }
			REQUIRE(factory != nullptr); REQUIRE(factory->module != nullptr); SetLocalCompany(factory->state.owner);
			auto *source = ResolveSpaceCraft(original); const auto hulls = GetSpaceVehicleCount(); const auto &recipe = *GetSpaceVehicleConstruction(model);
			uint32_t aged = 0; SpaceCraftRef built; uint64_t assets = source == nullptr ? 0 : ReplacementAssets(*source);
			if (action == "upgrade-clone" || action == "legacy-clone-complete") {
				REQUIRE(factory->module->manufacture != nullptr); auto &job = *factory->module->manufacture; REQUIRE(job.clone.has_value()); CHECK_FALSE(job.replacement.has_value());
				if (action == "legacy-clone-complete") {
					REQUIRE(job.work_ticks == recipe.work_ticks - 1); world.Advance(1); CHECK(job.phase == OrbitalManufacturePhase::Complete); CHECK(GetSpaceVehicleCount() == hulls + 1);
				}
				built = job.product; CHECK_FALSE(job.replacement.has_value()); CHECK(ResolveSpaceCraft(original) == source);
			} else if (action == "prepared" || action == "earth-prepared") {
				REQUIRE(source != nullptr); REQUIRE(factory->module->manufacture == nullptr);
				if (action == "prepared") REQUIRE(GetSpaceStructureBerth(*source, *world.core).has_value());
				else REQUIRE(source->construction.placement == SpacePlacement::SurfaceStored);
				REQUIRE(GetSpaceVehicleModel(*source) == model);
				const auto *catalogue = Engine::Get(source->engine_type);
				CAPTURE(source->engine_type.base(), source->owner.base(), catalogue->company_avail.base(), catalogue->flags.base(), catalogue->IsEnabled(),
					to_underlying(_settings_game.game_creation.landscape), catalogue->info.climates.base(), TimerGameCalendar::date.base(), catalogue->intro_date.base());
				// Dated saves can retain an originally uninitialized runtime Engine mask. Its normal monthly catalogue owner must admit it; retained history is never reset by a consumer or fixture.
				if (!IsEngineBuildable(source->engine_type, VehicleType::Space, source->owner)) {
					/** @return Whether genuine normal synchronized Engine lifecycle has made this already existing native role available. */
					world.Await([&] { return IsEngineBuildable(source->engine_type, VehicleType::Space, source->owner); });
				}
				REQUIRE(IsEngineBuildable(source->engine_type, VehicleType::Space, source->owner));
				REQUIRE(Command<Commands::SetAutoreplace>::Post(DEFAULT_GROUP, source->engine_type, source->engine_type, false));
				const auto structures = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest(), next = GetNextSpaceVehicleGeneration();
				const auto money = Company::Get(source->owner)->money, value = source->value; const auto random = _random;
				// Explicit native age-policy fixture only: same-model rules force when-old. Exercise the actual hull's calendar-day owner without fabricating an age field or advancing a century of economic activity.
				while (!source->NeedsAutorenewing(Company::Get(source->owner), false)) { REQUIRE(aged < 50000); source->OnNewCalendarDay(); ++aged; }
				CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(GetNextSpaceVehicleGeneration() == next);
				CHECK(Company::Get(source->owner)->money == money); CHECK(source->value == value); CHECK(ReplacementAssets(*source) == assets);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]); CHECK(GetSpaceVehicleCount() == hulls);
			} else if (action.starts_with("earth-")) {
				REQUIRE(source != nullptr); REQUIRE(source->construction.placement == SpacePlacement::SurfaceStored);
				const auto home = source->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr); REQUIRE(center->launch_operations != nullptr);
				if (action == "earth-queued") {
					const auto money = Company::Get(source->owner)->money; const auto next = GetNextSpaceVehicleGeneration(); const auto random = _random;
					const auto digest = ComputeLaunchProductionDigest();
					CHECK(CmdAutoreplaceVehicle({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, source->index).Succeeded());
					CHECK(ComputeLaunchProductionDigest() == digest); CHECK(GetNextSpaceVehicleGeneration() == next);
					REQUIRE(Command<Commands::AutoreplaceVehicle>::Post(source->index)); built = center->launch_operations->workshop;
					auto *product = ResolveSpaceCraft(built); REQUIRE(product != nullptr); REQUIRE(product->construction.replacement.has_value());
					CHECK(product->construction.replacement->source == original); CHECK_FALSE(product->construction.replacement->settled);
					CHECK(product->construction.placement == SpacePlacement::Assembling); CHECK(GetSpaceVehicleCount() == hulls + 1); CHECK(ResolveSpaceCraft(original) == source);
					CHECK(Company::Get(source->owner)->money == money - Money{recipe.capital}); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				} else {
					built = center->launch_operations->workshop; auto *product = ResolveSpaceCraft(built); REQUIRE(product != nullptr); REQUIRE(product->construction.replacement.has_value());
					REQUIRE(product->construction.replacement->source == original); const auto unit = source->unitnumber; const auto name = source->name;
					if (action == "earth-before") {
						REQUIRE(product->construction.remaining_ticks > 1); world.Advance(product->construction.remaining_ticks - 1);
						CHECK(product->construction.remaining_ticks == 1); CHECK(ResolveSpaceCraft(original) == source); CHECK_FALSE(product->construction.replacement->settled);
					} else if (action == "earth-complete") {
						REQUIRE(product->construction.remaining_ticks == 1); const auto sale = source->value; const auto capital = Company::Get(source->owner)->yearly_expenses[0][ExpensesType::NewVehicles];
						world.Advance(1); CHECK(ResolveSpaceCraft(original) == nullptr); CHECK(GetSpaceVehicleCount() == hulls - 1);
						CHECK(product->construction.placement == SpacePlacement::SurfaceStored); CHECK(product->construction.remaining_ticks == 0); CHECK(product->construction.replacement->settled);
						const auto credit = product->construction.replacement->sale_credit;
						CHECK(Money{credit} <= sale); CHECK(product->unitnumber == unit); CHECK(product->name == name); CHECK(ReplacementAssets(*product) == assets);
						CHECK(Company::Get(product->owner)->yearly_expenses[0][ExpensesType::NewVehicles] == capital - Money{credit}); source = nullptr; // A genuine native daily depreciation during this last tick belongs to the actual retirement value, not the earlier observer quote.
					} else {
						REQUIRE(action == "earth-cancel"); const auto money = Company::Get(source->owner)->money; const auto remaining = product->construction.remaining_ticks;
						REQUIRE(Command<Commands::CancelEarthSpaceReplacement>::Post(product->index, product->generation)); CHECK_FALSE(product->construction.replacement.has_value());
						CHECK(product->construction.remaining_ticks == remaining); CHECK(GetSpaceVehicleCount() == hulls); CHECK(ResolveSpaceCraft(original) == source); CHECK(Company::Get(source->owner)->money == money);
						world.Advance(remaining); CHECK(product->construction.placement == SpacePlacement::SurfaceStored); CHECK(ResolveSpaceCraft(original) == source); CHECK(GetSpaceVehicleCount() == hulls);
					}
				}
			} else if (action == "automatic") {
				REQUIRE(source != nullptr); REQUIRE(factory->module->manufacture == nullptr); const auto next = GetNextSpaceVehicleGeneration();
				world.Order(*source, *world.core, SpaceOrderAction::Visit);
				if (source->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Post(source->tile, source->index, true));
				// Reaching this real typed Visit must enter the existing native queue. Ordinary idling never retries a cancelled request; this test deliberately supplies a new arrival event.
				world.Await([&] { return factory->module->manufacture != nullptr; });
				const auto &job = *factory->module->manufacture; REQUIRE(job.replacement.has_value()); CHECK(job.replacement->source == original); CHECK(job.replacement->resume);
				CHECK(ResolveSpaceCraft(original) == source); CHECK(source->vehstatus.Test(VehState::Stopped)); CHECK(GetSpaceVehicleCount() == hulls); CHECK(GetNextSpaceVehicleGeneration() == next);
			} else if (action == "queued") {
				REQUIRE(source != nullptr); REQUIRE(factory->module->manufacture == nullptr);
				const auto structures = ComputeSpaceStructureDigest(), next = GetNextSpaceVehicleGeneration(); const auto money = Company::Get(source->owner)->money; const auto random = _random;
				// Native policy/facility estimates must never allocate a temporary hull, touch RNG or anticipate sale credit, even when Execute and QueryCost appear together.
				for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) {
					const auto quote = CmdAutoreplaceVehicle(flags, source->index); REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == Money{recipe.capital});
					CHECK(ComputeSpaceStructureDigest() == structures); CHECK(GetNextSpaceVehicleGeneration() == next); CHECK(Company::Get(source->owner)->money == money);
					CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				}
				REQUIRE(Command<Commands::AutoreplaceVehicle>::Post(source->index)); REQUIRE(factory->module->manufacture != nullptr);
				const auto &job = *factory->module->manufacture; REQUIRE(job.replacement.has_value()); CHECK(job.replacement->source == original); CHECK_FALSE(job.replacement->settled);
				CHECK(job.phase == OrbitalManufacturePhase::Queued); CHECK(job.product == SpaceCraftRef{}); CHECK(GetSpaceVehicleCount() == hulls); CHECK(GetNextSpaceVehicleGeneration() == next);
				CHECK(Company::Get(source->owner)->money == money - Money{recipe.capital}); CHECK(ReplacementAssets(*source) == assets);
			} else {
				REQUIRE(factory->module->manufacture != nullptr); auto &job = *factory->module->manufacture; REQUIRE(job.replacement.has_value()); REQUIRE(job.replacement->source == original);
				if (action == "reserved") { REQUIRE(job.phase == OrbitalManufacturePhase::Queued); world.Advance(1); CHECK(job.phase == OrbitalManufacturePhase::Reserved); CHECK(job.work_ticks == 0); }
				else if (action == "before" || action == "complete") {
					const auto target = action == "before" ? recipe.work_ticks - 1 : recipe.work_ticks; REQUIRE(job.work_ticks < target); REQUIRE(source != nullptr);
					const auto unit = source->unitnumber; const auto group = source->group_id; const auto name = source->name; const auto entries = source->space_orders.entries; const auto cursor = source->space_orders.cursor;
					const auto owner = factory->state.owner; const auto year = TimerGameEconomy::year; const auto capital = Company::Get(owner)->yearly_expenses[0][ExpensesType::NewVehicles];
					world.Advance(target - job.work_ticks); CHECK(job.work_ticks == target); CHECK(job.work_energy_j == uint64_t{target} * 750);
					uint32_t credit = 0;
					if (action == "complete") {
						REQUIRE(job.phase == OrbitalManufacturePhase::Complete); built = job.product; auto *product = ResolveSpaceCraft(built); REQUIRE(product != nullptr);
						CHECK(ResolveSpaceCraft(original) == nullptr); CHECK(GetSpaceVehicleCount() == hulls); CHECK(product->name == name); CHECK(product->unitnumber == unit); CHECK(product->group_id == group);
						CHECK(product->space_orders.entries == entries); CHECK(product->space_orders.cursor == cursor); CHECK(ReplacementAssets(*product) == assets); CHECK(job.replacement->settled); credit = job.replacement->sale_credit;
						CHECK(GetSpaceStructureBerth(*product, *factory).has_value());
						// The actual full common/heavy recipe becomes embodied once; none of the original freight/service holdings supply its material bill.
						for (size_t part = 0; part < MAX_ORBITAL_MANUFACTURE_INPUTS; ++part) { CHECK(job.inputs[part].TotalCount() == 0); CHECK(job.consumed[part] == (part < recipe.inputs.size() ? recipe.inputs[part].quantity : 0)); }
						source = nullptr;
					} else { CHECK(job.phase == OrbitalManufacturePhase::Working); CHECK(GetSpaceVehicleCount() == hulls); CHECK(ResolveSpaceCraft(original) == source); }
					// Ordinary running costs change cash. The native purchase expense owner proves exactly one final sale credit and no second capital charge, including one bounded economy-year rotation.
					if (TimerGameEconomy::year == year) CHECK(Company::Get(owner)->yearly_expenses[0][ExpensesType::NewVehicles] == capital - Money{credit});
					else {
						CHECK(TimerGameEconomy::year.base() == year.base() + 1); CHECK(Company::Get(owner)->yearly_expenses[1][ExpensesType::NewVehicles] == capital);
						CHECK(Company::Get(owner)->yearly_expenses[0][ExpensesType::NewVehicles] == -Money{credit});
					}
				} else if (action == "repeat") {
					REQUIRE(job.phase == OrbitalManufacturePhase::Complete); built = job.product; auto *product = ResolveSpaceCraft(built); REQUIRE(product != nullptr); assets = ReplacementAssets(*product);
					const auto money = Company::Get(product->owner)->money; const auto credit = job.replacement->sale_credit; PreparedSpaceReplacement empty;
					CommitSpaceReplacement(*job.replacement, *product, std::move(empty)); CHECK(Company::Get(product->owner)->money == money); world.Advance(3);
					CHECK(job.product == built); CHECK(job.replacement->sale_credit == credit); CHECK(ReplacementAssets(*product) == assets); CHECK(GetSpaceVehicleCount() == hulls);
				} else if (action == "mutate") {
					REQUIRE(source != nullptr); REQUIRE(Command<Commands::RenameVehicle>::Post(source->index, std::string{"Replacement edited"}));
				} else if (action == "depart-source") {
					REQUIRE(source != nullptr); REQUIRE(job.work_ticks == recipe.work_ticks - 1); REQUIRE(!GetAsteroidField().bodies.empty());
					const auto body = GetAsteroidField().bodies.front().id;
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(source->index, source->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(source->index, source->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, body.base(), uint64_t{0}));
					if (source->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Post(source->tile, source->index, true));
					/** @return Whether genuine typed navigation has left the handover neighborhood; no geometry is changed by the observer. */
					world.Await([&] { return SpaceDistanceSquared(source->position, GetSpaceStructureApproach(world.core->state)) > uint64_t{SPACE_Q16 / 4} * (SPACE_Q16 / 4); });
					CHECK(job.phase == OrbitalManufacturePhase::AwaitingMaterialization); CHECK_FALSE(job.replacement->settled); CHECK(job.product == SpaceCraftRef{}); CHECK(ResolveSpaceCraft(original) == source);
					CHECK(job.inputs[0].TotalCount() == recipe.inputs[0].quantity); CHECK(ReplacementAssets(*source) == assets);
				} else if (action == "return-source") {
					REQUIRE(source != nullptr); REQUIRE(job.phase == OrbitalManufacturePhase::AwaitingMaterialization); world.Order(*source, *world.core, SpaceOrderAction::Visit);
					if (source->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Post(source->tile, source->index, true));
					/** @return Whether actual return navigation and the existing fully earned milestone committed the one real replacement. */
					world.Await([&] { return job.phase == OrbitalManufacturePhase::Complete; });
					built = job.product; auto *product = ResolveSpaceCraft(built); REQUIRE(product != nullptr); CHECK(ResolveSpaceCraft(original) == nullptr); CHECK(GetSpaceVehicleCount() == hulls);
					CHECK(ReplacementAssets(*product) == assets); CHECK(job.replacement->settled); CHECK_FALSE(product->vehstatus.Test(VehState::Stopped)); source = nullptr;
				} else if (action == "transfer") {
					REQUIRE(Company::CanAllocateItem()); auto *company = Company::Create(); SetCompanyManagerFaceStyle(company->face, 0); const auto owner = company->index;
					ChangeOwnershipOfCompanyItems(_current_company, owner); _current_company = _local_company; SetLocalCompany(owner);
					REQUIRE(source != nullptr); CHECK(source->owner == owner); CHECK(factory->state.owner == owner);
				} else if (action == "source-loss") {
					REQUIRE(source != nullptr); REQUIRE(source->cargo.TotalCount() == 0); REQUIRE_FALSE(HasSpaceManifestAssets(*source));
					// Explicit empty-original destruction fixture, not native manual-sale permission. The paid producer must remain blocked; it cannot retarget numeric reuse or credit an absent source.
					delete source; source = nullptr; CHECK(ResolveSpaceCraft(original) == nullptr); world.Advance(1); CHECK(job.phase == OrbitalManufacturePhase::AwaitingMaterialization); CHECK_FALSE(job.replacement->settled); CHECK(job.product == SpaceCraftRef{});
				} else if (action == "cancel") {
					const auto money = Company::Get(_current_company)->money; const auto energy = ComputeUtilityPowerDigest();
					std::array<uint, MAX_ORBITAL_MANUFACTURE_INPUTS> returned{};
					// Return the full retained bill to one actual inventory; sale proceeds and refunded work/capital are never part of cancellation.
					for (size_t part = 0; part < recipe.inputs.size(); ++part) returned[part] = world.core->module->stock[GetCargoTypeByLabel(recipe.inputs[part].label)].TotalCount() + job.inputs[part].TotalCount();
					REQUIRE(Command<Commands::CancelOrbitalManufacture>::Post(factory->index, factory->generation, world.core->index, world.core->generation));
					CHECK(factory->module->manufacture == nullptr); CHECK(Company::Get(_current_company)->money == money); CHECK(ComputeUtilityPowerDigest() == energy); CHECK(GetSpaceVehicleCount() == hulls);
					for (size_t part = 0; part < recipe.inputs.size(); ++part) CHECK(world.core->module->stock[GetCargoTypeByLabel(recipe.inputs[part].label)].TotalCount() == returned[part]);
				} else { FAIL("Unsupported bounded replacement operation"); }
			}
			REQUIRE(ValidateSpaceVehicles()); world.Save(fmt::format("replacement-{}-{}-429.sav", action, model));
			const auto *surviving = ResolveSpaceCraft(original); if (built == SpaceCraftRef{} && factory->module->manufacture) built = factory->module->manufacture->product;
			const auto *product = ResolveSpaceCraft(built);
			fmt::print("MANUFACTURE_REPLACEMENT native=true phase={} model={} original_id={} original_generation={} original_alive={} assets={} product_id={} product_generation={} age_fixture_days={} procurement=false\n",
				action, model, original.vehicle.base(), original.generation, surviving != nullptr, product != nullptr ? ReplacementAssets(*product) : surviving != nullptr ? ReplacementAssets(*surviving) : 0,
				built.vehicle.base(), built.generation, aged);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(replacement_observer, &observe); IConsole::CmdRegister("manufacturing_replacement_test", ObserveManufacturingReplacement);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
