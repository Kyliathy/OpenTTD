/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_processor_native.cpp Paid native processor construction and resource-route acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group11
 * Only retained paid worlds and actual public commands may supply facilities, packets, work, utility or travel.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_func.h"
#include "../console_internal.h"
#include "../orbital_recipe.h"
#include "../fios.h"
#include "../station_base.h"
#include "../timer/timer_game_tick.h"
#include "../vehicle_cmd.h"
#include "../core/random_func.hpp"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *processor_observer = nullptr; ///< Invocation callback, without a parallel gameplay owner.

/** @param args Parameter-free native console callback. @return Whether the actual loaded-world observer ran. */
static bool ObserveOrbitalProcessor(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(processor_observer); (*processor_observer)(); return true;
}

/** Leave a retained physical berth through an actual native navigation order before another paid carrier approaches.
 * @param world Actual loaded-world command/tick adapter.
 * @param craft Existing paid orbital craft to depart, retaining all actual freight and service owners.
 */
static void DepartProcessorBerth(AssemblyNativeWorld &world, SpaceVehicle &craft)
{
	REQUIRE_FALSE(GetAsteroidField().bodies.empty()); const auto before = craft.position;
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, GetAsteroidField().bodies.front().id.base(), uint64_t{0}));
	/** @return Whether the native navigation tick actually vacated this old point; a command quote or a changed target alone cannot free the receiving berth. */
	world.Await([&] { return craft.position != before; });
}

/** Buy one actual processor blueprint at a nearby site admitted by the original native placement command.
 * @param world Existing paid Core and command/tick context; no anchor or footprint is installed by this helper.
 * @param kind Canonical specialized processor kind to construct.
 * @return Actual newly issued native project, with its original capital paid through the command framework.
 */
static SpaceStructure &BuildPaidProcessorBlueprint(AssemblyNativeWorld &world, SpaceBuildKind kind)
{
	REQUIRE(world.core->state.x >= 3); REQUIRE(world.core->state.y >= 3);
	const auto before = ComputeSpaceStructureDigest(); const auto money = Company::Get(_current_company)->money;
	TileIndex selected = INVALID_TILE;
	// Native footprints include their reserved south approaches. Quote the complete bounded adjacent patch so an omitted candidate cannot manufacture a placement failure; only the original command decides geometry and Core adjacency.
	for (int dy = -3; dy <= 3 && selected == INVALID_TILE; ++dy) {
		// Each row remains query-only; stop at the first actually admitted cell without deleting another owner, moving a hull or reserving a rejected footprint.
		for (int dx = -3; dx <= 3; ++dx) {
			const auto tile = TileXYL(world.core->state.x + dx, world.core->state.y + dy, MapLayer::Space);
			if (CmdBuildSpaceBlueprint({}, tile, kind).Succeeded()) { selected = tile; break; }
		}
	}
	REQUIRE(selected != INVALID_TILE); CHECK(ComputeSpaceStructureDigest() == before); CHECK(Company::Get(_current_company)->money == money);
	REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(selected, kind));
	SpaceStructure *result = nullptr;
	// Resolve only the actual newly published original role; this scenario never manufactures a parallel site/generation identity or a completed frame.
	for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == kind) { REQUIRE(result == nullptr); result = site; }
	REQUIRE(result); CHECK(Company::Get(_current_company)->money == money - GetSpaceConstructionDefinition(kind)->capital);
	return *result;
}

/** Queue one actual finite resource hold for a precise existing processor, without injecting cargo or changing a pose.
 * @param miner Original paid empty orbital mining hull.
 * @param receiver Actual operational native processor accepting this resource.
 * @param kind Canonical asteroid resource class to extract.
 * @param quantity Whole native hold quantity required from an available finite body.
 * @return Existing body identity selected by real distance and lease admission.
 */
static OrbitalBodyID QueueProcessorMining(SpaceVehicle &miner, const SpaceStructure &receiver, AsteroidClass kind, uint16_t quantity)
{
	REQUIRE(miner.cargo.TotalCount() == 0); REQUIRE(GetSpaceMiningBlock(miner) == SpaceOrderBlock::None);
	const auto point = GetSpaceStructureApproach(receiver.state); const AsteroidBody *body = nullptr;
	// Only an available actual resource/lease can finance the complete transported hold; nearest moving geometry determines the route without editing a body or reserve.
	for (const auto &candidate : GetAsteroidField().bodies) {
		if (candidate.resource_class != kind || candidate.remaining_reserve < quantity || candidate.mining_slot != VehicleID::Invalid() || candidate.survey_slot != INVALID_SPACE_SURVEY) continue;
		if (body == nullptr || SpaceDistanceSquared(point, candidate.motion.position) < SpaceDistanceSquared(point, body->motion.position)) body = &candidate;
	}
	REQUIRE(body);
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner.index, miner.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner.index, miner.generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, body->id.base(), uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner.index, miner.generation, SpaceOrderEdit::Insert, uint8_t{1}, SpaceOrderAction::Unload, SpaceOrderTargetKind::Structure, uint32_t{receiver.index.base()}, receiver.generation));
	return body->id;
}

/** @param kind Existing specialized role in this actual paid scenario. @return Its unique actual native project, without creating or copying an owner. */
static SpaceStructure &FindProcessor(SpaceBuildKind kind)
{
	SpaceStructure *result = nullptr;
	// Every phase resolves the same originally issued native incarnation; a duplicate role cannot silently substitute a different paid stock/work owner.
	for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == kind) { REQUIRE(result == nullptr); result = site; }
	REQUIRE(result); return *result;
}

/** @return Original unique paid small miner from this retained world's actual pool, never an allocated substitute. */
static SpaceVehicle &FindProcessorMiner()
{
	SpaceVehicle *result = nullptr;
	// Only the original paid15 role supplies this scenario's finite resources; no hull, hold or lease is created by lookup.
	for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 15) { REQUIRE(result == nullptr); result = craft; }
	REQUIRE(result); return *result;
}

/** @param choice Original published four-byte alternative. @return Stable big-endian command argument, without a local recipe ordinal or numerical definition. */
static uint32_t ProcessorChoice(MultiLayerRecipeID choice)
{
	uint32_t result = 0; for (const auto byte : choice) result = (result << 8) | byte; return result;
}

/** Execute the original player control without writing any private work or material state.
 * @param site Actual owned native processing incarnation.
 * @param control Explicit start/pause/resume/cancel operation.
 * @param choice Original published alternative for Start; empty preserves the native command's default semantics.
 */
static void ControlProcessor(SpaceStructure &site, OrbitalRecipeControl control, MultiLayerRecipeID choice = {})
{
	REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(site.index, site.generation, ProcessorChoice(choice), static_cast<uint8_t>(control)));
}

/** Drive actual paid construction, finite mining, processing and independent persistence through original native commands and opportunities. */
TEST_CASE("Paid native processors conserve construction production and restored work", "[.orbital-processor-native]")
{
	const char *source = std::getenv("OTTD_ORBITAL_PROCESSOR_SOURCE"); REQUIRE(source);
	const char *selected = std::getenv("OTTD_ORBITAL_PROCESSOR_PHASE"); const std::string_view phase = selected ? selected : "inspected";
	const char *replay = std::getenv("OTTD_ORBITAL_PROCESSOR_RESAVE"); const bool resave = replay != nullptr && std::string_view{replay} == "1";
	const auto paths = GetTestCaseArtifactPaths("space", "orbital-processor-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve assertions across ordinary engine shutdown while examining only actual accepted native owners. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			REQUIRE(ValidateSpaceStructures()); REQUIRE(ValidateOrbitalRecipes());
			const auto entry = CaptureOrbitalRecipes(); const auto entry_structures = ComputeSpaceStructureDigest(), entry_power = ComputeUtilityPowerDigest();
			const auto entry_money = Company::Get(_current_company)->money; const auto entry_random = _random; const auto entry_tick = TimerGameTick::counter;
			if (!entry.empty()) {
				// Every actual processing checkpoint must pass the independent same-file copied reader before controls run. A preview may inspect this file's owners but cannot advance or borrow the active world's paid work, packets, clock or money.
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(_load_check_data.orbital_recipe_rows == entry); CHECK(CaptureOrbitalRecipes() == entry);
				CHECK(ComputeSpaceStructureDigest() == entry_structures); CHECK(ComputeUtilityPowerDigest() == entry_power);
				CHECK(Company::Get(_current_company)->money == entry_money); CHECK(TimerGameTick::counter == entry_tick);
				CHECK(_random.state[0] == entry_random.state[0]); CHECK(_random.state[1] == entry_random.state[1]);
			}
			auto *center = ResolveLaunchCenter(world.rocket->construction.home); REQUIRE(center);
			// Exact configured labels join original Surface and Core packet stocks; a favorable total in another cargo or world cannot fund construction.
			for (const auto label : {CargoLabel{"STEL"}, CargoLabel{"GOOD"}, CargoLabel{"PROP"}, CargoLabel{"NORE"}, CargoLabel{"WICE"}, CargoLabel{"RARE"}}) {
				const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo));
				fmt::print("PROCESSOR_SUPPLY cargo={} weight={} surface={} core={}\n", cargo, CargoSpec::Get(cargo)->weight,
					center->goods[cargo].AvailableCount(), world.core->module->stock[cargo].TotalCount());
			}
			// Resolve every already paid role and actual physical hold before selecting a miner or carrier; no native identity or orbital arrival is invented.
			for (const auto *craft : SpaceVehicle::Iterate()) {
				fmt::print("PROCESSOR_CRAFT id={} generation={} model={} placement={} held={} returns={} fuel={} stopped={} x={} y={} block={}\n", craft->index.base(), craft->generation,
					GetSpaceVehicleModel(*craft), to_underlying(craft->construction.placement), craft->cargo.TotalCount(),
					craft->manifest ? craft->manifest->returns_remaining : 0,
					craft->manifest ? craft->manifest->supplies[0].TotalCount() : 0, craft->vehstatus.Test(VehState::Stopped),
					craft->position.x, craft->position.y, to_underlying(craft->space_orders.blocked));
			}
			if (resave) {
				// An own-resave process only admits and rewrites the actual saved owner graph; it must not repeat this checkpoint's original production or native control operation.
				CHECK(CaptureOrbitalRecipes() == entry); CHECK(ComputeSpaceStructureDigest() == entry_structures);
			} else if (phase == "steel") {
				auto *builder = world.builders.front();
				// Start only an actual stopped orbital builder through its public player command; a Surface-stored rocket instead follows its original launch controller.
				if (builder->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Post(builder->tile, builder->index, true));
				REQUIRE(builder->cargo.TotalCount() == 0); REQUIRE(world.core->state.x >= 3);
				const auto tile = TileXYL(world.core->state.x - 3, world.core->state.y, MapLayer::Space);
				const auto money = Company::Get(_current_company)->money; const auto digest = ComputeSpaceStructureDigest();
				REQUIRE(CmdBuildSpaceBlueprint({}, tile, SpaceBuildKind::Steelworks).Succeeded());
				CHECK(Company::Get(_current_company)->money == money); CHECK(ComputeSpaceStructureDigest() == digest);
				REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(tile, SpaceBuildKind::Steelworks));
				SpaceStructure *steelworks = nullptr;
				// Resolve only the actual newly paid native module kind; the observer never issues a local structure identity or installs a frame.
				for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Steelworks) { REQUIRE(steelworks == nullptr); steelworks = site; }
				REQUIRE(steelworks); CHECK(Company::Get(_current_company)->money == money - GetSpaceConstructionDefinition(SpaceBuildKind::Steelworks)->capital);
				// The early paid checkpoint retains two builders at its old Core point. A real departure to the newly paid empty job resolves that physical congestion before any rocket resupply; no pose or berth is rewritten.
				world.Order(*builder, *steelworks, SpaceOrderAction::Construct);
				/** @return Whether the actual builder reached this new paid construction approach, still with no material or work grant. */
				world.Await([&] { return GetSpaceStructureBerth(*builder, *steelworks).has_value(); });
				const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}); const auto available = center->goods[steel].AvailableCount();
				const auto missions = world.rocket->transition->missions_completed;
				world.Import(*builder, steel, 12); CHECK(center->goods[steel].AvailableCount() == available - 12);
				CHECK(world.rocket->transition->missions_completed == missions + 1);
				world.Order(*builder, *steelworks, SpaceOrderAction::Construct);
				/** @return Whether native construction delivered only its original Steel bill, retaining the two extra real units aboard. */
				world.Await([&] { return steelworks->job.inputs[0].TotalCount() == 10; });
				CHECK(builder->cargo.TotalCount() == 2); CHECK(steelworks->job.inputs[1].TotalCount() == 0);
				CHECK(steelworks->job.work_ticks == 0); CHECK(steelworks->module == nullptr);
			} else if (phase == "steel-frame") {
				SpaceStructure *steelworks = nullptr;
				// Resolve the saved paid job and both original carriers through native ownership, not a parallel fixture identity table.
				for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Steelworks) { REQUIRE(steelworks == nullptr); steelworks = site; }
				REQUIRE(steelworks); REQUIRE(steelworks->job.inputs[0].TotalCount() == 10); REQUIRE(steelworks->job.inputs[1].TotalCount() == 0);
				REQUIRE(steelworks->job.work_ticks == 0); REQUIRE(steelworks->module == nullptr);
				if (world.core->module->producer == UtilityProducerID{UINT32_MAX}) {
					// A genuine inventory-only416Core never gains generation merely by loading. Its original explicit commissioning command must issue the actual panel before any new shared construction work is possible.
					const auto power = ComputeUtilityPowerDigest(); REQUIRE(CmdCommissionOrbitalServices({}, world.core->index, world.core->generation).Succeeded());
					CHECK(ComputeUtilityPowerDigest() == power);
					REQUIRE(Command<Commands::CommissionOrbitalServices>::Post(world.core->index, world.core->generation));
					REQUIRE(GetUtilityPowerProducer(world.core->module->producer));
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == 0);
				}
				SpaceVehicle *steel_carrier = nullptr, *goods_carrier = nullptr;
				// The actual occupied construction approach selects the Steel carrier; the other original empty builder receives the next physical Surface shipment.
				for (auto *craft : world.builders) {
					if (GetSpaceStructureBerth(*craft, *steelworks)) steel_carrier = craft; else goods_carrier = craft;
				}
				REQUIRE(steel_carrier); REQUIRE(goods_carrier); REQUIRE(steel_carrier->cargo.TotalCount() == 2); REQUIRE(goods_carrier->cargo.TotalCount() == 0);
				const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}); const auto available = center->goods[goods].AvailableCount();
				const auto missions = world.rocket->transition->missions_completed;
				world.Import(*goods_carrier, goods, 40); CHECK(center->goods[goods].AvailableCount() == available - 40);
				CHECK(world.rocket->transition->missions_completed == missions + 1);
				fmt::print("PROCESSOR_PROGRESS phase=steel-frame goods_arrived={} panel={}\n", goods_carrier->cargo.TotalCount(), world.core->module->producer.base());
				DepartProcessorBerth(world, *steel_carrier); world.Order(*goods_carrier, *steelworks, SpaceOrderAction::Construct);
				INFO("Waiting for shared native Steelworks frame work after actual Goods delivery and original Core commissioning");
				/** @return Whether actual shared construction work completed this already paid and physically supplied frame. */
				world.Await([&] { return steelworks->state.phase == SpaceStructurePhase::FrameComplete; });
				CHECK(steelworks->job.work_ticks == 740); CHECK((steelworks->job.consumed == std::array<uint16_t, 2>{10, 16}));
				CHECK(steelworks->job.inputs[0].TotalCount() == 0); CHECK(steelworks->job.inputs[1].TotalCount() == 0);
				CHECK(steelworks->module == nullptr); CHECK(goods_carrier->cargo.TotalCount() == 24); CHECK(steel_carrier->cargo.TotalCount() == 2);
				CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == uint64_t{740} * 750);
			} else if (phase == "steel-active") {
				SpaceStructure *steelworks = nullptr;
				// Only the actual saved paid frame may activate; an inert blueprint or another processor cannot stand in for this native construction receipt.
				for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Steelworks) { REQUIRE(steelworks == nullptr); steelworks = site; }
				REQUIRE(steelworks); REQUIRE(steelworks->module == nullptr); REQUIRE(steelworks->job.work_ticks == 740);
				SpaceVehicle *steel_carrier = nullptr, *goods_carrier = nullptr;
				// Retained actual freight distinguishes the original carriers after their independently saved arrival/work; neither role receives an invented inventory.
				for (auto *craft : world.builders) {
					if (craft->cargo_type == GetCargoTypeByLabel(CargoLabel{"GOOD"})) goods_carrier = craft; else steel_carrier = craft;
				}
				REQUIRE(steel_carrier); REQUIRE(goods_carrier); REQUIRE(steel_carrier->cargo.TotalCount() == 2); REQUIRE(goods_carrier->cargo.TotalCount() == 24);
				world.Order(*steel_carrier, *world.core, SpaceOrderAction::Visit);
				/** @return Whether the original Steel carrier physically returned from its temporary Body visit to the now free Core. */
				world.Await([&] { return GetSpaceStructureBerth(*steel_carrier, *world.core).has_value(); });
				const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
				const auto state = ComputeSpaceStructureDigest(); REQUIRE(CmdActivateOrbitalModule({}, steelworks->index, steelworks->generation).Succeeded());
				CHECK(ComputeSpaceStructureDigest() == state);
				REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(steelworks->index, steelworks->generation)); world.Advance(74);
				REQUIRE(IsOrbitalModuleOperational(*steelworks)); REQUIRE(steelworks->module->processing);
				CHECK_FALSE(steelworks->module->processing->enabled); CHECK(steelworks->module->processing->paid_lifetime_j == 0);
				REQUIRE(Command<Commands::ConnectOrbitalServices>::Post(steelworks->index, steelworks->generation, world.core->index, world.core->generation));
				REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(steelworks->index, steelworks->generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Start)));
				world.Advance(3); CHECK(GetOrbitalRecipeWait(*steelworks) == OrbitalRecipeWait::Input);
				CHECK(steelworks->module->processing->paid_lifetime_j == 0); CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
				// The next specialized paid job is deliberately supplied only with original carried Goods, leaving all Steel work and production for the actual mining path.
				auto &goodsworks = BuildPaidProcessorBlueprint(world, SpaceBuildKind::Goodsworks);
				world.Order(*goods_carrier, goodsworks, SpaceOrderAction::Construct);
				/** @return Whether this carrier actually vacated the Steelworks approach and delivered the next complete Goods bill to its own paid job. */
				world.Await([&] { return goodsworks.job.inputs[1].TotalCount() == 16; });
				CHECK(goodsworks.job.inputs[0].TotalCount() == 0); CHECK(goodsworks.job.work_ticks == 0); CHECK(goodsworks.module == nullptr);
				CHECK(goods_carrier->cargo.TotalCount() == 8); CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
			} else if (phase == "miner-deployed") {
				SpaceVehicle *miner = nullptr; SpaceStructure *steelworks = nullptr;
				// This acceptance deploys the already paid original small miner; an extra factory hull or a locally allocated proxy cannot supply its resource route.
				for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 15) { REQUIRE(miner == nullptr); miner = craft; }
				for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Steelworks) steelworks = site;
				REQUIRE(miner); REQUIRE(miner->manifest); REQUIRE(steelworks); REQUIRE(IsOrbitalModuleOperational(*steelworks));
				REQUIRE(miner->construction.placement == SpacePlacement::SurfaceStored); REQUIRE(miner->cargo.TotalCount() == 0);
				const auto count = GetSpaceVehicleCount(); const auto next_generation = GetNextSpaceVehicleGeneration();
				const auto home = world.rocket->construction.home;
				// Only actual existing Surface parts fill an empty original service compartment; native capacity and packet movement remain authoritative for the later real work cycle.
				for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) {
					if (miner->manifest->supplies[supply].TotalCount() == 0) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, miner->index, miner->generation, supply, uint16_t{1}, false));
					CHECK(miner->manifest->supplies[supply].TotalCount() == 1);
				}
				const AsteroidBody *body = nullptr; const auto receiver = GetSpaceStructureApproach(steelworks->state);
				// Choose real finite metallic stock near the actual processor while respecting existing extraction/survey leases; neither reserve nor moving target position is manufactured.
				for (const auto &candidate : GetAsteroidField().bodies) {
					if (candidate.resource_class != AsteroidClass::Metallic || candidate.remaining_reserve < 20 || candidate.mining_slot != VehicleID::Invalid() || candidate.survey_slot != INVALID_SPACE_SURVEY) continue;
					if (body == nullptr || SpaceDistanceSquared(receiver, candidate.motion.position) < SpaceDistanceSquared(receiver, body->motion.position)) body = &candidate;
				}
				REQUIRE(body); const auto reserve = body->remaining_reserve, extracted = miner->extracted_units;
				REQUIRE(Command<Commands::CarryLaunchProbe>::Post(world.rocket->index, world.rocket->generation, miner->index, miner->generation, false));
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, world.rocket->index, world.rocket->generation, uint8_t{0}, uint16_t{15}, false));
				const auto missions = world.rocket->transition->missions_completed, burned = world.rocket->manifest->fuel_burned;
				const auto money = Company::Get(_current_company)->money;
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(world.rocket->index, world.rocket->generation, SpaceOrderTargetKind::Body, body->id.base(), uint64_t{0}));
				CHECK(Company::Get(_current_company)->money == money - GetMultiLayerSpaceBudget().mission_fee);
				/** @return Whether the original carrier completed its paid passenger deployment and real Surface recovery. */
				world.Await([&] { return world.rocket->transition->missions_completed == missions + 1; });
				REQUIRE(miner->construction.placement == SpacePlacement::Orbital); CHECK(world.rocket->manifest->fuel_burned == burned + 15);
				CHECK(GetSpaceVehicleCount() == count); CHECK(GetNextSpaceVehicleGeneration() == next_generation);
				CHECK(miner->extracted_units == extracted); CHECK(body->remaining_reserve == reserve); CHECK(miner->cargo.TotalCount() == 0);
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, body->id.base(), uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Insert, uint8_t{1}, SpaceOrderAction::Unload, SpaceOrderTargetKind::Structure, uint32_t{steelworks->index.base()}, steelworks->generation));
				CHECK(GetSpaceMiningBlock(*miner) == SpaceOrderBlock::None); CHECK(GetOrbitalRecipeWait(*steelworks) == OrbitalRecipeWait::Input);
				CHECK(steelworks->module->processing->paid_lifetime_j == 0);
			} else if (phase == "ore-delivered" || phase == "steel-partial" || phase == "steel-batch" || phase == "steel-stock") {
				// These successive saved checkpoints share the same actual mine/Steelworks owners; each phase advances only its admitted predecessor's physical inputs and paid progress.
				SpaceVehicle *miner = nullptr; SpaceStructure *steelworks = nullptr;
				// Resolve original live roles rather than restoring a unit frame, copied work scalar or a replacement stock container.
				for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 15) miner = craft;
				for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Steelworks) steelworks = site;
				REQUIRE(miner); REQUIRE(steelworks); REQUIRE(steelworks->module); REQUIRE(steelworks->module->processing);
				auto &processing = *steelworks->module->processing;
				const auto ore = GetCargoTypeByLabel(CargoLabel{"NORE"}), steel = GetCargoTypeByLabel(CargoLabel{"STEL"});
				if (phase == "ore-delivered") {
					REQUIRE(miner->space_orders.entries.size() == 2); REQUIRE(steelworks->module->stock[ore].TotalCount() == 0);
					const auto *body = GetMutableAsteroidBody(OrbitalBodyID{miner->space_orders.entries[0].target.id}); REQUIRE(body);
					const auto reserve = body->remaining_reserve, extracted = miner->extracted_units;
					const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
					// Pause preserves a real whole transported input checkpoint. Mining, travel and unloading must still use the original finite body, hold and actual module admission.
					REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(steelworks->index, steelworks->generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Pause)));
					/** @return Whether native extraction and the queued precise return delivered the whole original20-unit hold into this paid processor. */
					world.Await([&] { return miner->space_orders.cursor == miner->space_orders.entries.size() && miner->cargo.TotalCount() == 0 && steelworks->module->stock[ore].TotalCount() == 20; }, 12000);
					CHECK(miner->extracted_units == extracted + 20); CHECK(body->remaining_reserve == reserve - 20);
					CHECK(processing.paid_lifetime_j == 0); CHECK(steelworks->module->stock[steel].TotalCount() == 0);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
				} else if (phase == "steel-partial") {
					REQUIRE_FALSE(processing.enabled); REQUIRE(processing.paid_lifetime_j == 0); REQUIRE(steelworks->module->stock[ore].TotalCount() == 20);
					const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
					REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(steelworks->index, steelworks->generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Resume)));
					world.Advance(17); CHECK(processing.paid_lifetime_j == uint64_t{17} * 750); CHECK(processing.electrical->Capture().paid_j == uint64_t{17} * 750);
					CHECK(steelworks->module->stock[ore].TotalCount() == 20); CHECK(steelworks->module->stock[steel].TotalCount() == 0);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used + uint64_t{17} * 750);
				} else {
					const auto target = phase == "steel-batch" ? uint64_t{1} : uint64_t{5};
					REQUIRE(processing.enabled); REQUIRE(processing.electrical);
					world.Advance(phase == "steel-batch" ? 57 : 4 * 74);
					CHECK(processing.batches.at(MultiLayerRecipeID{"OSTL"}) == target); CHECK(processing.paid_lifetime_j == target * 55500);
					CHECK(processing.electrical->Capture().paid_j == 0); CHECK(processing.discarded_paid_j == 0);
					CHECK(processing.consumed.at(CargoLabel{"NORE"}) == target * 4); CHECK(processing.produced.at(CargoLabel{"STEL"}) == target * 2);
					CHECK(processing.residue_mass_sixteenths.at(MultiLayerRecipeID{"OSTL"}) == target * 32);
					CHECK(steelworks->module->stock[ore].TotalCount() == 20 - target * 4); CHECK(steelworks->module->stock[steel].TotalCount() == target * 2);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == 555000 + target * 55500);
					if (phase == "steel-stock") {
						const auto before = CaptureOrbitalRecipes(); const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
						world.Advance(3); CHECK(CaptureOrbitalRecipes() == before); CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
						CHECK(GetOrbitalRecipeWait(*steelworks) == OrbitalRecipeWait::Input);
					}
				}
			} else if (phase == "goods-frame" || phase == "goods-active") {
				// Both checkpoints keep the same paid Goodsworks and transport only real Steel output; the separate phases retain an ordinary frame and missing-secondary-input boundary.
				SpaceVehicle *miner = nullptr; SpaceStructure *steelworks = nullptr, *goodsworks = nullptr;
				// Find only original native roles and specialized sites; ownership remains in their existing packet containers throughout construction and onward transport.
				for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 15) miner = craft;
				for (auto *site : SpaceStructure::Iterate()) {
					if (site->state.kind == SpaceBuildKind::Steelworks) steelworks = site;
					if (site->state.kind == SpaceBuildKind::Goodsworks) goodsworks = site;
				}
				REQUIRE(miner); REQUIRE(steelworks); REQUIRE(goodsworks);
				SpaceVehicle *steel_carrier = nullptr, *goods_carrier = nullptr;
				// The retained actual Goods hold identifies its carrier; the other original builder handles the explicitly partitioned Steel product.
				for (auto *craft : world.builders) {
					if (craft->cargo_type == GetCargoTypeByLabel(CargoLabel{"GOOD"}) && craft->cargo.TotalCount() != 0) goods_carrier = craft; else steel_carrier = craft;
				}
				REQUIRE(steel_carrier); REQUIRE(goods_carrier);
				const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
				const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
				if (phase == "goods-frame") {
					REQUIRE(goodsworks->module == nullptr); REQUIRE(steel_carrier->cargo.TotalCount() == 2); REQUIRE(goods_carrier->cargo.TotalCount() == 8);
					REQUIRE(steelworks->module->stock[steel].TotalCount() == 10); REQUIRE(goodsworks->job.inputs[1].TotalCount() == 16);
					DepartProcessorBerth(world, *miner); world.Order(*steel_carrier, *steelworks, SpaceOrderAction::Visit);
					/** @return Whether this real builder reached the completed Steelworks product buffer after the original miner actually departed. */
					world.Await([&] { return GetSpaceStructureBerth(*steel_carrier, *steelworks).has_value(); });
					REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(steelworks->index, steelworks->generation, steel_carrier->index, steel_carrier->generation, steel, uint16_t{8}, true));
					CHECK(steelworks->module->stock[steel].TotalCount() == 2); CHECK(steel_carrier->cargo.TotalCount() == 10);
					world.Order(*goods_carrier, *world.core, SpaceOrderAction::Visit);
					/** @return Whether the Goods carrier vacated its incomplete job and physically reached the original Core's retained stock. */
					world.Await([&] { return GetSpaceStructureBerth(*goods_carrier, *world.core).has_value(); });
					REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(world.core->index, world.core->generation, goods_carrier->index, goods_carrier->generation, goods, uint16_t{8}, true));
					CHECK(world.core->module->stock[goods].TotalCount() == 0); CHECK(goods_carrier->cargo.TotalCount() == 16);
					world.Order(*steel_carrier, *goodsworks, SpaceOrderAction::Construct);
					/** @return Whether native work consumed this job's complete bill, including exactly eight newly produced Steel units and two retained Earth units. */
					world.Await([&] { return goodsworks->state.phase == SpaceStructurePhase::FrameComplete; });
					CHECK(goodsworks->job.work_ticks == 740); CHECK((goodsworks->job.consumed == std::array<uint16_t, 2>{10, 16}));
					CHECK(goodsworks->job.inputs[0].TotalCount() == 0); CHECK(goodsworks->job.inputs[1].TotalCount() == 0);
					CHECK(goodsworks->module == nullptr); CHECK(steel_carrier->cargo.TotalCount() == 0);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used + uint64_t{740} * 750);
				} else {
					REQUIRE(goodsworks->module == nullptr); REQUIRE(goodsworks->job.work_ticks == 740); REQUIRE(steel_carrier->cargo.TotalCount() == 0);
					REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(goodsworks->index, goodsworks->generation)); world.Advance(74);
					REQUIRE(IsOrbitalModuleOperational(*goodsworks)); REQUIRE(goodsworks->module->processing);
					REQUIRE(Command<Commands::ConnectOrbitalServices>::Post(goodsworks->index, goodsworks->generation, world.core->index, world.core->generation));
					REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(goodsworks->index, goodsworks->generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Start)));
					world.Order(*steel_carrier, *steelworks, SpaceOrderAction::Visit);
					/** @return Whether the real carrier reached the two explicitly unreserved Steel products left after the complete construction allocation. */
					world.Await([&] { return GetSpaceStructureBerth(*steel_carrier, *steelworks).has_value(); });
					REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(steelworks->index, steelworks->generation, steel_carrier->index, steel_carrier->generation, steel, uint16_t{2}, true));
					CHECK(steelworks->module->stock[steel].TotalCount() == 0);
					world.Order(*steel_carrier, *goodsworks, SpaceOrderAction::Visit);
					/** @return Whether native navigation returned the same real products to the newly active specialized Goods buffer. */
					world.Await([&] { return GetSpaceStructureBerth(*steel_carrier, *goodsworks).has_value(); });
					REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(goodsworks->index, goodsworks->generation, steel_carrier->index, steel_carrier->generation, steel, uint16_t{2}, false));
					world.Advance(3); CHECK(GetOrbitalRecipeWait(*goodsworks) == OrbitalRecipeWait::Input);
					CHECK(goodsworks->module->stock[steel].TotalCount() == 2); CHECK(goodsworks->module->stock[GetCargoTypeByLabel(CargoLabel{"RARE"})].TotalCount() == 0);
					CHECK(goodsworks->module->stock[goods].TotalCount() == 0); CHECK(goodsworks->module->processing->paid_lifetime_j == 0);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
				}
			} else if (phase == "rare-delivered" || phase == "goods-partial" || phase == "goods-batch") {
				// Real Rare-Mineral arrival and its two paid-work checkpoints share one actual Goods owner; no earlier Steel input may disappear while the missing secondary input is in transit.
				SpaceVehicle *miner = nullptr, *carrier = nullptr; SpaceStructure *steelworks = nullptr, *goodsworks = nullptr;
				// Each role is resolved from this loaded world's actual paid pool; no copied module or scalar material view supplies the input.
				for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 15) miner = craft;
				for (auto *craft : world.builders) if (craft->cargo.TotalCount() == 0) carrier = craft;
				for (auto *site : SpaceStructure::Iterate()) {
					if (site->state.kind == SpaceBuildKind::Steelworks) steelworks = site;
					if (site->state.kind == SpaceBuildKind::Goodsworks) goodsworks = site;
				}
				REQUIRE(miner); REQUIRE(carrier); REQUIRE(steelworks); REQUIRE(goodsworks); REQUIRE(goodsworks->module); REQUIRE(goodsworks->module->processing);
				auto &processing = *goodsworks->module->processing;
				const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), rare = GetCargoTypeByLabel(CargoLabel{"RARE"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
				if (phase == "rare-delivered") {
					REQUIRE(goodsworks->module->stock[steel].TotalCount() == 2); REQUIRE(goodsworks->module->stock[rare].TotalCount() == 0);
					world.Order(*carrier, *steelworks, SpaceOrderAction::Visit);
					/** @return Whether this actual carrier vacated the receiving Goodsworks approach before the explicit mineral return. */
					world.Await([&] { return GetSpaceStructureBerth(*carrier, *steelworks).has_value(); });
					REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(goodsworks->index, goodsworks->generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Pause)));
					const auto body_id = QueueProcessorMining(*miner, *goodsworks, AsteroidClass::Rich, 20);
					const auto *body = GetMutableAsteroidBody(body_id); REQUIRE(body); const auto reserve = body->remaining_reserve, extracted = miner->extracted_units;
					const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
					/** @return Whether native finite extraction, travel and whole-hold handling delivered the actual secondary resource without consuming the waiting Steel. */
					world.Await([&] { return miner->space_orders.cursor == miner->space_orders.entries.size() && miner->cargo.TotalCount() == 0 && goodsworks->module->stock[rare].TotalCount() == 20; }, 12000);
					CHECK(body->remaining_reserve == reserve - 20); CHECK(miner->extracted_units == extracted + 20);
					CHECK(goodsworks->module->stock[steel].TotalCount() == 2); CHECK(goodsworks->module->stock[goods].TotalCount() == 0);
					CHECK(processing.paid_lifetime_j == 0); CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
				} else if (phase == "goods-partial") {
					REQUIRE_FALSE(processing.enabled); REQUIRE(processing.paid_lifetime_j == 0);
					const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
					REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(goodsworks->index, goodsworks->generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Resume)));
					world.Advance(17); CHECK(processing.paid_lifetime_j == uint64_t{17} * 750); CHECK(processing.electrical->Capture().paid_j == uint64_t{17} * 750);
					CHECK(goodsworks->module->stock[steel].TotalCount() == 2); CHECK(goodsworks->module->stock[rare].TotalCount() == 20); CHECK(goodsworks->module->stock[goods].TotalCount() == 0);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used + uint64_t{17} * 750);
				} else {
					REQUIRE(processing.enabled); REQUIRE(processing.electrical->Capture().paid_j == uint64_t{17} * 750);
					world.Advance(57); CHECK(processing.batches.at(MultiLayerRecipeID{"OGDS"}) == 1); CHECK(processing.paid_lifetime_j == 55500);
					CHECK(processing.electrical->Capture().paid_j == 0); CHECK(processing.consumed.at(CargoLabel{"STEL"}) == 2); CHECK(processing.consumed.at(CargoLabel{"RARE"}) == 1);
					CHECK(processing.produced.at(CargoLabel{"GOOD"}) == 4); CHECK(processing.residue_mass_sixteenths.at(MultiLayerRecipeID{"OGDS"}) == 16);
					CHECK(goodsworks->module->stock[steel].TotalCount() == 0); CHECK(goodsworks->module->stock[rare].TotalCount() == 19); CHECK(goodsworks->module->stock[goods].TotalCount() == 4);
					const auto before = CaptureOrbitalRecipes(); const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
					world.Advance(3); CHECK(CaptureOrbitalRecipes() == before); CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
				}
			} else if (phase == "refinery-ore" || phase == "refinery-stock" || phase == "refinery-frame" || phase == "refinery-active" || phase == "refinery-nore") {
				// This paid third facility uses retained delivered Goods and newly processed Steel; its staged construction never receives a prefab or a numerical work grant.
				auto &miner = FindProcessorMiner(); auto &steelworks = FindProcessor(SpaceBuildKind::Steelworks); auto &goodsworks = FindProcessor(SpaceBuildKind::Goodsworks);
				const auto ore = GetCargoTypeByLabel(CargoLabel{"NORE"}), steel = GetCargoTypeByLabel(CargoLabel{"STEL"});
				if (phase == "refinery-ore") {
					SpaceVehicle *goods_carrier = nullptr, *steel_carrier = nullptr;
					// Retained real freight chooses the material carrier; the other original empty builder must actually vacate the next raw-input berth.
					for (auto *craft : world.builders) { if (craft->cargo.TotalCount() == 16) goods_carrier = craft; else steel_carrier = craft; }
					REQUIRE(goods_carrier); REQUIRE(steel_carrier); REQUIRE(steel_carrier->cargo.TotalCount() == 0);
					// The genuine early world retains one obsolete empty Storage reservation on this edge. Explicit player cancellation frees its site while retaining the original capital record and every existing physical owner.
					for (auto *site : SpaceStructure::Iterate()) {
						if (site->state.kind != SpaceBuildKind::Storage || site->state.phase != SpaceStructurePhase::Blueprint || site->state.x != world.core->state.x + 3 || site->state.y != world.core->state.y) continue;
						REQUIRE(site->state.owner == _current_company); REQUIRE(site->module == nullptr); REQUIRE(site->job.work_ticks == 0);
						REQUIRE(site->job.inputs[0].TotalCount() == 0); REQUIRE(site->job.inputs[1].TotalCount() == 0);
						const auto capital = site->state.capital_paid; const auto money = Company::Get(_current_company)->money;
						REQUIRE(Command<Commands::CancelSpaceBlueprint>::Post(site->index, site->generation));
						CHECK(site->state.phase == SpaceStructurePhase::Cancelled); CHECK(site->state.capital_paid == capital); CHECK(Company::Get(_current_company)->money == money);
					}
					auto &refinery = BuildPaidProcessorBlueprint(world, SpaceBuildKind::Refinery);
					world.Order(*goods_carrier, refinery, SpaceOrderAction::Construct);
					/** @return Whether the original retained16Goods physically entered this new paid job, without any construction Steel or work yet. */
					world.Await([&] { return refinery.job.inputs[1].TotalCount() == 16; });
					CHECK(refinery.job.inputs[0].TotalCount() == 0); CHECK(refinery.job.work_ticks == 0); CHECK(goods_carrier->cargo.TotalCount() == 0);
					world.Order(*steel_carrier, *world.core, SpaceOrderAction::Visit);
					/** @return Whether the original carrier really freed the Steelworks return point after the Goods carrier left the Core. */
					world.Await([&] { return GetSpaceStructureBerth(*steel_carrier, *world.core).has_value(); });
					REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(goodsworks.index, goodsworks.generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Pause)));
					REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(steelworks.index, steelworks.generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Pause)));
					const auto body_id = QueueProcessorMining(miner, steelworks, AsteroidClass::Metallic, 20);
					const auto *body = GetMutableAsteroidBody(body_id); REQUIRE(body); const auto reserve = body->remaining_reserve, extracted = miner.extracted_units;
					const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
					/** @return Whether another actual finite20NORE return entered the paused processor rather than being fabricated for the third construction bill. */
					world.Await([&] { return miner.space_orders.cursor == miner.space_orders.entries.size() && miner.cargo.TotalCount() == 0 && steelworks.module->stock[ore].TotalCount() == 20; }, 12000);
					CHECK(body->remaining_reserve == reserve - 20); CHECK(miner.extracted_units == extracted + 20);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
				} else if (phase == "refinery-stock") {
					REQUIRE(steelworks.module->stock[ore].TotalCount() == 20); REQUIRE(steelworks.module->stock[steel].TotalCount() == 0);
					const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
					REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(steelworks.index, steelworks.generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Resume)));
					world.Advance(5 * 74); const auto &processing = *steelworks.module->processing;
					CHECK(processing.batches.at(MultiLayerRecipeID{"OSTL"}) == 10); CHECK(processing.paid_lifetime_j == 10 * uint64_t{55500});
					CHECK(processing.consumed.at(CargoLabel{"NORE"}) == 40); CHECK(processing.produced.at(CargoLabel{"STEL"}) == 20);
					CHECK(processing.residue_mass_sixteenths.at(MultiLayerRecipeID{"OSTL"}) == 320);
					CHECK(steelworks.module->stock[ore].TotalCount() == 0); CHECK(steelworks.module->stock[steel].TotalCount() == 10);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used + 5 * uint64_t{55500});
				} else {
					auto &refinery = FindProcessor(SpaceBuildKind::Refinery);
					if (phase == "refinery-frame") {
						SpaceVehicle *waiting = nullptr, *carrier = nullptr;
						// The actual retained construction berth identifies its empty Goods builder; the other original builder will transport all ten newly made Steel units.
						for (auto *craft : world.builders) { if (GetSpaceStructureBerth(*craft, refinery)) waiting = craft; else carrier = craft; }
						REQUIRE(waiting); REQUIRE(carrier); REQUIRE(refinery.job.inputs[1].TotalCount() == 16); REQUIRE(refinery.job.inputs[0].TotalCount() == 0);
						world.Order(miner, goodsworks, SpaceOrderAction::Visit);
						/** @return Whether the real miner vacated the product loading point for the actual construction carrier. */
						world.Await([&] { return GetSpaceStructureBerth(miner, goodsworks).has_value(); });
						world.Order(*carrier, steelworks, SpaceOrderAction::Visit);
						/** @return Whether this empty paid builder reached the ten real Steel products reserved for this construction bill. */
						world.Await([&] { return GetSpaceStructureBerth(*carrier, steelworks).has_value(); });
						REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(steelworks.index, steelworks.generation, carrier->index, carrier->generation, steel, uint16_t{10}, true));
						CHECK(steelworks.module->stock[steel].TotalCount() == 0); CHECK(carrier->cargo.TotalCount() == 10);
						world.Order(*waiting, *world.core, SpaceOrderAction::Visit);
						/** @return Whether its original Goods builder actually vacated the incomplete refinery approach before the complete Steel bill arrives. */
						world.Await([&] { return GetSpaceStructureBerth(*waiting, *world.core).has_value(); });
						const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
						world.Order(*carrier, refinery, SpaceOrderAction::Construct);
						/** @return Whether the original shared construction owner consumed this complete10Steel/16Goods bill after740actual opportunities. */
						world.Await([&] { return refinery.state.phase == SpaceStructurePhase::FrameComplete; });
						CHECK(refinery.job.work_ticks == 740); CHECK((refinery.job.consumed == std::array<uint16_t, 2>{10, 16}));
						CHECK(refinery.module == nullptr); CHECK(carrier->cargo.TotalCount() == 0);
						CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used + uint64_t{740} * 750);
					} else if (phase == "refinery-active") {
						REQUIRE(refinery.module == nullptr); const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
						REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(refinery.index, refinery.generation)); world.Advance(74);
						REQUIRE(IsOrbitalModuleOperational(refinery)); REQUIRE(refinery.module->processing);
						CHECK(refinery.module->processing->consumer == UtilityConsumerID{UINT32_MAX}); CHECK_FALSE(refinery.module->processing->electrical);
						REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(refinery.index, refinery.generation, ProcessorChoice(MultiLayerRecipeID{"OREF"}), static_cast<uint8_t>(OrbitalRecipeControl::Start)));
						CHECK(refinery.module->processing->choice == MultiLayerRecipeID{"OREF"});
						REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(refinery.index, refinery.generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Pause)));
						CHECK(refinery.module->processing->unpowered_work_ticks == 0); CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
					} else {
						REQUIRE(phase == "refinery-nore"); REQUIRE(refinery.module); REQUIRE_FALSE(refinery.module->processing->enabled);
						SpaceVehicle *carrier = nullptr;
						// Only the real completed-frame builder at this exact approach must depart; its empty hold grants no refinery input.
						for (auto *craft : world.builders) if (GetSpaceStructureBerth(*craft, refinery)) carrier = craft;
						REQUIRE(carrier); world.Order(*carrier, steelworks, SpaceOrderAction::Visit);
						/** @return Whether native navigation freed the newly active unpowered processor for its own finite mineral return. */
						world.Await([&] { return GetSpaceStructureBerth(*carrier, steelworks).has_value(); });
						const auto body_id = QueueProcessorMining(miner, refinery, AsteroidClass::Metallic, 20);
						const auto *body = GetMutableAsteroidBody(body_id); REQUIRE(body); const auto reserve = body->remaining_reserve, extracted = miner.extracted_units;
						const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
						/** @return Whether another original finite20NORE hold reached this distinct processor while preserving all earlier Steel/Goods production. */
						world.Await([&] { return miner.space_orders.cursor == miner.space_orders.entries.size() && miner.cargo.TotalCount() == 0 && refinery.module->stock[ore].TotalCount() == 20; }, 12000);
						CHECK(body->remaining_reserve == reserve - 20); CHECK(miner.extracted_units == extracted + 20);
						CHECK(refinery.module->processing->unpowered_work_ticks == 0); CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
					}
				}
			} else if (phase == "refinery-partial" || phase == "refinery-metal" || phase == "refinery-retarget" || phase == "ice-delivered" || phase == "ice-partial" || phase == "ice-batch") {
				// Both saved unpowered alternatives earn real baseline opportunities; changing the choice cannot reinterpret unfinished work or consume the retained input for another recipe.
				auto &refinery = FindProcessor(SpaceBuildKind::Refinery); auto &miner = FindProcessorMiner();
				REQUIRE(refinery.module); REQUIRE(refinery.module->processing); auto &processing = *refinery.module->processing;
				const auto ore = GetCargoTypeByLabel(CargoLabel{"NORE"}), metals = GetCargoTypeByLabel(CargoLabel{"RFMT"}), ice = GetCargoTypeByLabel(CargoLabel{"WICE"}), propellant = GetCargoTypeByLabel(CargoLabel{"PROP"});
				REQUIRE(IsValidCargoType(metals)); REQUIRE(IsValidCargoType(propellant));
				const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
				if (phase == "refinery-partial") {
					REQUIRE_FALSE(processing.enabled); REQUIRE(processing.choice == MultiLayerRecipeID{"OREF"}); REQUIRE(refinery.module->stock[ore].TotalCount() == 20);
					ControlProcessor(refinery, OrbitalRecipeControl::Resume); world.Advance(37);
					CHECK(processing.unpowered_work_ticks == 37); CHECK(refinery.module->stock[ore].TotalCount() == 20); CHECK(refinery.module->stock[metals].TotalCount() == 0);
					ControlProcessor(refinery, OrbitalRecipeControl::Pause); const auto before = CaptureOrbitalRecipes();
					CHECK(CmdControlOrbitalRecipe({}, refinery.index, refinery.generation, ProcessorChoice(MultiLayerRecipeID{"PICE"}), static_cast<uint8_t>(OrbitalRecipeControl::Start)).Failed());
					CHECK(CaptureOrbitalRecipes() == before); ControlProcessor(refinery, OrbitalRecipeControl::Resume);
				} else if (phase == "refinery-metal") {
					REQUIRE(processing.choice == MultiLayerRecipeID{"OREF"}); REQUIRE(processing.unpowered_work_ticks == 37);
					world.Advance(37); CHECK(processing.unpowered_work_ticks == 0); CHECK(processing.batches.at(MultiLayerRecipeID{"OREF"}) == 1);
					CHECK(processing.consumed.at(CargoLabel{"NORE"}) == 4); CHECK(processing.produced.at(CargoLabel{"RFMT"}) == 2);
					CHECK(processing.residue_mass_sixteenths.at(MultiLayerRecipeID{"OREF"}) == 0);
					CHECK(refinery.module->stock[ore].TotalCount() == 16); CHECK(refinery.module->stock[metals].TotalCount() == 2);
				} else if (phase == "refinery-retarget") {
					REQUIRE(processing.unpowered_work_ticks == 37); ControlProcessor(refinery, OrbitalRecipeControl::Pause);
					const auto before = CaptureOrbitalRecipes();
					CHECK(CmdControlOrbitalRecipe({}, refinery.index, refinery.generation, ProcessorChoice(MultiLayerRecipeID{"PICE"}), static_cast<uint8_t>(OrbitalRecipeControl::Start)).Failed());
					CHECK(CaptureOrbitalRecipes() == before); ControlProcessor(refinery, OrbitalRecipeControl::Cancel);
					CHECK(processing.unpowered_work_ticks == 0); CHECK(refinery.module->stock[ore].TotalCount() == 20);
					ControlProcessor(refinery, OrbitalRecipeControl::Start, MultiLayerRecipeID{"PICE"}); world.Advance(3);
					CHECK(processing.choice == MultiLayerRecipeID{"PICE"}); CHECK(processing.unpowered_work_ticks == 0); CHECK(GetOrbitalRecipeWait(refinery) == OrbitalRecipeWait::Input);
					CHECK(refinery.module->stock[metals].TotalCount() == 0); CHECK(refinery.module->stock[propellant].TotalCount() == 0);
				} else if (phase == "ice-delivered") {
					REQUIRE(processing.choice == MultiLayerRecipeID{"OREF"}); REQUIRE(processing.unpowered_work_ticks == 0); REQUIRE(processing.batches.at(MultiLayerRecipeID{"OREF"}) == 1);
					ControlProcessor(refinery, OrbitalRecipeControl::Pause); ControlProcessor(refinery, OrbitalRecipeControl::Start, MultiLayerRecipeID{"PICE"}); ControlProcessor(refinery, OrbitalRecipeControl::Pause);
					const auto body_id = QueueProcessorMining(miner, refinery, AsteroidClass::Carbonaceous, 20);
					const auto *body = GetMutableAsteroidBody(body_id); REQUIRE(body); const auto reserve = body->remaining_reserve, extracted = miner.extracted_units;
					/** @return Whether native finite Water-Ice extraction reached the explicitly selected unpowered recipe while preserving the old mineral stock/history. */
					world.Await([&] { return miner.space_orders.cursor == miner.space_orders.entries.size() && miner.cargo.TotalCount() == 0 && refinery.module->stock[ice].TotalCount() == 20; }, 12000);
					CHECK(body->remaining_reserve == reserve - 20); CHECK(miner.extracted_units == extracted + 20);
					CHECK(processing.choice == MultiLayerRecipeID{"PICE"}); CHECK(processing.unpowered_work_ticks == 0);
					CHECK(refinery.module->stock[ore].TotalCount() == 16); CHECK(refinery.module->stock[metals].TotalCount() == 2);
				} else if (phase == "ice-partial") {
					REQUIRE(processing.choice == MultiLayerRecipeID{"PICE"}); REQUIRE_FALSE(processing.enabled); REQUIRE(refinery.module->stock[ice].TotalCount() == 20);
					ControlProcessor(refinery, OrbitalRecipeControl::Resume); world.Advance(37);
					CHECK(processing.unpowered_work_ticks == 37); CHECK(refinery.module->stock[ice].TotalCount() == 20); CHECK(refinery.module->stock[propellant].TotalCount() == 0);
				} else {
					REQUIRE(phase == "ice-batch"); REQUIRE(processing.unpowered_work_ticks == 37); world.Advance(37);
					CHECK(processing.unpowered_work_ticks == 0); CHECK(processing.batches.at(MultiLayerRecipeID{"PICE"}) == 1); CHECK(processing.batches.at(MultiLayerRecipeID{"OREF"}) == 1);
					CHECK(processing.consumed.at(CargoLabel{"WICE"}) == 2); CHECK(processing.produced.at(CargoLabel{"PROP"}) == 1);
					CHECK(processing.residue_mass_sixteenths.at(MultiLayerRecipeID{"PICE"}) == 0);
					CHECK(refinery.module->stock[ice].TotalCount() == 18); CHECK(refinery.module->stock[propellant].TotalCount() == 1);
					CHECK(refinery.module->stock[ore].TotalCount() == 16); CHECK(refinery.module->stock[metals].TotalCount() == 2);
					ControlProcessor(refinery, OrbitalRecipeControl::Pause); const auto before = CaptureOrbitalRecipes();
					world.Advance(3); CHECK(CaptureOrbitalRecipes() == before);
				}
				CHECK_FALSE(processing.electrical); CHECK(processing.consumer == UtilityConsumerID{UINT32_MAX}); CHECK(processing.paid_lifetime_j == 0);
				CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
			} else if (phase == "steel-outage" || phase == "steel-restored" || phase == "steel-cancelled" || phase == "steel-restarted") {
				// Pending paid work must survive loss of its original panel and explicit cancellation; neither outage nor restart may refund electricity, consume a material prefix or settle one batch twice.
				auto &steelworks = FindProcessor(SpaceBuildKind::Steelworks); REQUIRE(steelworks.module); REQUIRE(steelworks.module->processing);
				auto &processing = *steelworks.module->processing; REQUIRE(processing.electrical);
				const auto ore = GetCargoTypeByLabel(CargoLabel{"NORE"}), steel = GetCargoTypeByLabel(CargoLabel{"STEL"});
				const auto used = GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j;
				if (phase == "steel-outage") {
					REQUIRE(processing.electrical->Capture().paid_j == 12750); const auto before = CaptureOrbitalRecipes();
					REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(world.core->index, world.core->generation, true)); world.Advance(11);
					CHECK(GetOrbitalRecipeWait(steelworks) == OrbitalRecipeWait::Power); CHECK(CaptureOrbitalRecipes() == before);
					CHECK(steelworks.module->stock[ore].TotalCount() == 20); CHECK(steelworks.module->stock[steel].TotalCount() == 0);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
				} else if (phase == "steel-restored") {
					REQUIRE_FALSE(world.core->module->power_enabled); REQUIRE(processing.electrical->Capture().paid_j == 12750);
					REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(world.core->index, world.core->generation, false)); world.Advance(57);
					CHECK(processing.paid_lifetime_j == 55500); CHECK(processing.electrical->Capture().paid_j == 0);
					CHECK(processing.batches.at(MultiLayerRecipeID{"OSTL"}) == 1); CHECK(processing.discarded_paid_j == 0);
					CHECK(steelworks.module->stock[ore].TotalCount() == 16); CHECK(steelworks.module->stock[steel].TotalCount() == 2);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used + uint64_t{57} * 750);
				} else if (phase == "steel-cancelled") {
					REQUIRE(processing.electrical->Capture().paid_j == 12750); ControlProcessor(steelworks, OrbitalRecipeControl::Cancel);
					CHECK_FALSE(processing.enabled); CHECK(processing.electrical->Capture().paid_j == 0); CHECK(processing.discarded_paid_j == 12750);
					CHECK(processing.paid_lifetime_j == 12750); CHECK(steelworks.module->stock[ore].TotalCount() == 20); CHECK(steelworks.module->stock[steel].TotalCount() == 0);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used);
				} else {
					REQUIRE_FALSE(processing.enabled); REQUIRE(processing.discarded_paid_j == 12750); REQUIRE(processing.electrical->Capture().paid_j == 0);
					ControlProcessor(steelworks, OrbitalRecipeControl::Start, MultiLayerRecipeID{"OSTL"}); world.Advance(74);
					CHECK(processing.batches.at(MultiLayerRecipeID{"OSTL"}) == 1); CHECK(processing.paid_lifetime_j == 12750 + 55500);
					CHECK(processing.discarded_paid_j == 12750); CHECK(processing.electrical->Capture().paid_j == 0);
					CHECK(steelworks.module->stock[ore].TotalCount() == 16); CHECK(steelworks.module->stock[steel].TotalCount() == 2);
					CHECK(GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j == used + 55500);
				}
			} else {
				REQUIRE(phase == "inspected");
			}
			REQUIRE(ValidateSpaceStructures()); REQUIRE(ValidateOrbitalRecipes());
			world.Save(fmt::format("processor-{}-438.sav", phase));
			if (resave) {
				CHECK(CaptureOrbitalRecipes() == entry); CHECK(ComputeSpaceStructureDigest() == entry_structures); CHECK(ComputeUtilityPowerDigest() == entry_power);
				CHECK(Company::Get(_current_company)->money == entry_money); CHECK(TimerGameTick::counter == entry_tick);
				CHECK(_random.state[0] == entry_random.state[0]); CHECK(_random.state[1] == entry_random.state[1]);
			}
			fmt::print("ORBITAL_PROCESSOR native=true phase={} added_stock=false resave={}\n", phase, resave); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(processor_observer, &observe); IConsole::CmdRegister("orbital_processor_test", ObserveOrbitalProcessor);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
