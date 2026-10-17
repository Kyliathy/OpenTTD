/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file processor_returns_native.cpp Actual finite processor output, paid-shell returns and linked Surface collection.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12B
 * Only original paid-world owners and public commands supply inputs, travel, handling, shells or progress.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_func.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../multilayer_space_content.h"
#include "../newgrf_roadstop.h"
#include "../newgrf_station.h"
#include "../orbital_recipe.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include "../timer/timer_game_tick.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *returns_observer = nullptr; ///< Per-invocation callback, never a second gameplay owner.
static constexpr std::string_view PRODUCT_RAIL = "Processor product rail", PRODUCT_LORRY = "Processor product lorry", HISTORICAL_RAIL = "Processor historical rail";

/** @param args Parameter-free ordinary console callback. @return Whether this exact loaded-world observer ran. */
static bool ObserveProductReturns(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(returns_observer); (*returns_observer)(); return true;
}

/** @param kind Canonical specialized role in the exact accepted producer world. @return Its unique actual original paid site. */
static SpaceStructure &ProductProcessor(SpaceBuildKind kind)
{
	SpaceStructure *result = nullptr;
	// Only the original constructed role may supply this pipeline; model/kind similarity cannot substitute another material or work owner.
	for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == kind) { REQUIRE(result == nullptr); result = site; }
	REQUIRE(result); REQUIRE(IsOrbitalModuleOperational(*result)); return *result;
}

/** @return Original unique paid small miner, preserving its finite extraction and service/pod history. */
static SpaceVehicle &ProductMiner()
{
	SpaceVehicle *result = nullptr;
	// This retained world has exactly one original15; lookup creates no hull, quota, packet or body lease.
	for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 15) { REQUIRE(result == nullptr); result = craft; }
	REQUIRE(result); return *result;
}

/** @param site Original actual processor. @param control Explicit pause/resume; no recipe choice or retained history is rewritten. */
static void ControlProductProcessor(SpaceStructure &site, OrbitalRecipeControl control)
{
	REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(site.index, site.generation, uint32_t{0}, static_cast<uint8_t>(control)));
}

/** Vacate a physical receiving berth through actual movement.
 * @param world Original command/tick context.
 * @param craft Existing empty paid craft; all actual supplies and history remain attached.
 */
static void DepartProductBerth(AssemblyNativeWorld &world, SpaceVehicle &craft)
{
	REQUIRE_FALSE(GetAsteroidField().bodies.empty()); const auto before = craft.position;
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, GetAsteroidField().bodies.front().id.base(), uint64_t{0}));
	/** @return Whether a real navigation opportunity moved this original hull away from the old receiver point. */
	world.Await([&] { return craft.position != before; });
}

/** Queue one actual complete finite Metallic hold for the original constructed Steelworks.
 * @param miner Original empty paid15 hull.
 * @param receiver Original operational processor whose paused stock will receive the ore.
 * @return Actual available nearest body, without changing its reserve, lease or moving pose.
 */
static OrbitalBodyID QueueProductOre(SpaceVehicle &miner, SpaceStructure &receiver)
{
	REQUIRE(miner.cargo.TotalCount() == 0); REQUIRE(GetSpaceMiningBlock(miner) == SpaceOrderBlock::None);
	const auto point = GetSpaceStructureApproach(receiver.state); const AsteroidBody *selected = nullptr;
	// Only a real available twenty-unit deposit and lease may finance this extra product batch. Nearest actual geometry selects the route; no scripted stock or mining progress is installed.
	for (const auto &body : GetAsteroidField().bodies) {
		if (body.resource_class != AsteroidClass::Metallic || body.remaining_reserve < 20 || body.mining_slot != VehicleID::Invalid() || body.survey_slot != INVALID_SPACE_SURVEY) continue;
		if (selected == nullptr || SpaceDistanceSquared(point, body.motion.position) < SpaceDistanceSquared(point, selected->motion.position)) selected = &body;
	}
	REQUIRE(selected);
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner.index, miner.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner.index, miner.generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, selected->id.base(), uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner.index, miner.generation, SpaceOrderEdit::Insert, uint8_t{1}, SpaceOrderAction::Unload, SpaceOrderTargetKind::Structure, uint32_t{receiver.index.base()}, receiver.generation));
	return selected->id;
}

/** Transport one original stored paid shell through the original reusable rocket's actual flight/recovery.
 * @param world Original Core, rocket, commands and synchronized opportunity context.
 * @param receiver Actual stationary paid constructor receiving one shell, never another hull's quota.
 * @return Same original paid shell now attached to that exact receiver.
 */
static SpacePodRef SupplyProductPod(AssemblyNativeWorld &world, SpaceVehicle &receiver)
{
	auto &rocket = *world.rocket; const auto home = rocket.construction.home; REQUIRE(rocket.manifest); REQUIRE(rocket.manifest->pods.empty());
	REQUIRE(rocket.cargo.TotalCount() == 0); REQUIRE(receiver.manifest); REQUIRE(receiver.manifest->pods.empty());
	const auto allowance = receiver.manifest->returns_remaining; REQUIRE(allowance > 0); const auto fuel_before = rocket.manifest->fuel_burned;
	const auto missions = rocket.transition->missions_completed; SpacePodRef selected{};
	// A successful original command quote selects a real paid CenterStored shell. Neither the miner's carried pods nor a newly invented shell can satisfy this Surface custody boundary.
	for (auto *pod : SpacePod::Iterate()) if (CmdMoveLaunchPod({}, home.station, home.generation, rocket.index, rocket.generation, pod->index, pod->generation, false).Succeeded()) { selected = GetSpacePodReference(*pod); break; }
	REQUIRE(selected != SpacePodRef{});
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(receiver.index, receiver.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	REQUIRE(Command<Commands::MoveLaunchPod>::Post(home.station, home.generation, rocket.index, rocket.generation, selected.pod, selected.generation, false));
	const auto fuel = static_cast<uint16_t>(GetMultiLayerSpaceBudget().outbound_fuel + GetMultiLayerSpaceBudget().recovery_fuel);
	REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket.index, rocket.generation, uint8_t{0}, fuel, false));
	REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket.index, rocket.generation, SpaceOrderTargetKind::Craft, uint32_t{receiver.index.base()}, receiver.generation));
	/** @return Whether actual delivery and paid recovery completed exactly this original mission; elapsed guesses cannot grant the receiver a shell. */
	world.Await([&] { return rocket.transition->missions_completed == missions + 1; });
	CHECK(rocket.construction.placement == SpacePlacement::SurfaceStored); CHECK(rocket.manifest->fuel_burned == fuel_before + fuel);
	CHECK(receiver.manifest->pods == std::vector{selected}); CHECK(receiver.manifest->returns_remaining == allowance); CHECK(rocket.manifest->pods.empty());
	return selected;
}

/** @param name Exact saved cosmetic name of a newly built native freight member. @return Its actual unique Station owner, without an alternative identity registry. */
static Station &ProductMember(std::string_view name)
{
	Station *result = nullptr;
	// The ordinary saved station is the only endpoint; a missing or duplicated name cannot silently redirect product packets.
	for (auto *station : Station::Iterate()) if (station->name == name) { REQUIRE(result == nullptr); result = station; }
	REQUIRE(result); return *result;
}

/** Build one genuine separate freight member through the original native station and exclusive linking commands.
 * @param center Actual receiving launch facility supplying its original collection circle.
 * @param lorry Whether to create a native truck bay rather than a rail platform.
 * @param name Saved endpoint name used by later ordinary continuations.
 * @return Actual paid newly built, named and linked station.
 */
static Station &BuildProductMember(Station &center, bool lorry, std::string_view name)
{
	const auto anchor = *GetLaunchCenterPlaneReference(center.launch_center->tile);
	// Query the real Surface boundary patch. Native terrain, whole-footprint range, ownership and station construction decide each site; no terrain flattening or synthetic station substitutes admission.
	for (int dx : {20, 19}) for (int dy : {0, 1, 2, 3, 4}) {
		const int x = TileX(center.launch_center->tile) + dx, y = TilePlaneY(center.launch_center->tile) + dy;
		if (x + 1 >= static_cast<int>(Map::SizeX()) || y + 1 >= static_cast<int>(Map::SizeY())) continue;
		const auto tile = TileXYL(x, y, MapLayer::Surface); if (!IsInsideLaunchCenterCollectionArea(*GetLaunchCenterPlaneReference(tile), anchor)) continue;
		const auto flags = CommandFlagsToDCFlags(lorry ? GetCommandFlags<Commands::BuildRoadStop>() : GetCommandFlags<Commands::BuildRailStation>());
		const auto quote = lorry ? CmdBuildRoadStop(flags, tile, 1, 1, RoadStopType::Truck, false, DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, true) :
			CmdBuildRailStation(flags, tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, true);
		if (quote.Failed()) continue;
		if (lorry) REQUIRE(Command<Commands::BuildRoadStop>::Post(tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck, false, DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, NEW_STATION, true));
		else REQUIRE(Command<Commands::BuildRailStation>::Post(tile, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, true));
		auto *member = Station::GetByTile(tile); REQUIRE(member); REQUIRE(member != &center); REQUIRE(IsLaunchHubEligible(center, *member));
		REQUIRE(Command<Commands::RenameStation>::Post(member->index, std::string{name})); const auto home = GetLaunchCenterReference(center);
		REQUIRE(Command<Commands::SetLaunchHub>::Post(member->index, home.station, home.generation)); return *member;
	}
	FAIL("No genuine product collection member site is admitted"); return center;
}

/** Reserve one original source holding in its uniquely owned native handling job.
 * @param center Actual original receiving center whose own packets finance the reservation.
 * @param member Exact actually linked separate Station recipient.
 * @param cargo Original physical label; no source reclassification or delivery income is allowed.
 * @param quantity Positive complete units selected for this one endpoint.
 */
static void HandleProduct(Station &center, Station &member, CargoType cargo, uint16_t quantity)
{
	const auto link = GetLaunchHubMembership(member.index); REQUIRE(link); const auto home = GetLaunchCenterReference(center); REQUIRE(link->center == home);
	REQUIRE(Command<Commands::StartSurfaceHandling>::Post(member.index, link->member.generation, home.station, home.generation, cargo, quantity, false));
}

/** @return Actual current-company spacecraft profit sum; its native one-tick delta supplies observed upkeep without inventing another expense account. */
static Money ProductSpaceProfit()
{
	Money result{};
	// Other owners cannot finance this center's handling fee. Every actual owned Space hull contributes its original native upkeep accounting once.
	for (const auto *craft : SpaceVehicle::Iterate()) if (craft->owner == _current_company) result += craft->profit_this_year;
	return result;
}

/** Actual processed output must retain one chosen owner through finite production, paid return shells and separate Surface collection. */
TEST_CASE("Constructed processors return actual products through paid shells and linked Surface members", "[.processor-returns-native]")
{
	const char *source = std::getenv("OTTD_PROCESSOR_RETURNS_SOURCE"), *selected = std::getenv("OTTD_PROCESSOR_RETURNS_PHASE"), *own = std::getenv("OTTD_PROCESSOR_RETURNS_RESAVE");
	REQUIRE(source); REQUIRE(selected); const std::string_view phase{selected}; const bool resave = own != nullptr && std::string_view{own} == "1";
	const auto paths = GetTestCaseArtifactPaths("space", "processor-returns-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Keep assertion failures across ordinary orderly engine shutdown; only the loaded paid world may supply any owner or progress. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths); auto &miner = ProductMiner();
			auto &steelworks = ProductProcessor(SpaceBuildKind::Steelworks), &goodsworks = ProductProcessor(SpaceBuildKind::Goodsworks);
			auto &steel_carrier = *world.builders.front(), &goods_carrier = *world.builders.back(); REQUIRE(&steel_carrier != &goods_carrier);
			const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}), ore = GetCargoTypeByLabel(CargoLabel{"NORE"}), propellant = GetCargoTypeByLabel(CargoLabel{"PROP"});
			REQUIRE(IsValidCargoType(steel)); REQUIRE(IsValidCargoType(goods)); REQUIRE(IsValidCargoType(ore)); REQUIRE(IsValidCargoType(propellant));
			const auto home = world.rocket->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center); REQUIRE(center->launch_hub);
			const auto fleet = GetSpaceVehicleCount(); REQUIRE(fleet == 5); const auto source_power = world.core->module->producer;
			const auto entry_recipes = CaptureOrbitalRecipes(); const auto entry_structures = ComputeSpaceStructureDigest(), entry_vehicles = ComputeSpaceVehicleDigest(), entry_power = ComputeUtilityPowerDigest();
			const auto entry_tick = TimerGameTick::counter; const auto entry_money = Company::Get(_current_company)->money; const auto entry_random = _random;
			const auto steel_delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[steel], goods_delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[goods];
			REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(_load_check_data.orbital_recipe_rows == entry_recipes); CHECK(CaptureOrbitalRecipes() == entry_recipes);
			CHECK(ComputeSpaceStructureDigest() == entry_structures); CHECK(ComputeSpaceVehicleDigest() == entry_vehicles); CHECK(ComputeUtilityPowerDigest() == entry_power);
			CHECK(Company::Get(_current_company)->money == entry_money); CHECK(TimerGameTick::counter == entry_tick);
			CHECK(_random.state[0] == entry_random.state[0]); CHECK(_random.state[1] == entry_random.state[1]);
			fmt::print("PRODUCT_STATE phase={} surface_steel={} surface_goods={} surface_propellant={} steel_output={} goods_output={} paid_pods={} steel_shells={} goods_shells={}\n", phase,
				center->goods[steel].TotalCount(), center->goods[goods].TotalCount(), center->goods[propellant].TotalCount(), steelworks.module->stock[steel].TotalCount(), goodsworks.module->stock[goods].TotalCount(),
				CaptureSpacePods().size(), steel_carrier.manifest ? steel_carrier.manifest->pods.size() : 0, goods_carrier.manifest ? goods_carrier.manifest->pods.size() : 0);
			if (resave) {
				// An own-resave rewrites this same checkpoint without mining, resupply, release, handling, commands or opportunities; later children execute the original next action separately.
				REQUIRE(phase != "inspected");
			} else if (phase == "ore-delivered") {
				if (steelworks.module->processing->enabled) ControlProductProcessor(steelworks, OrbitalRecipeControl::Pause);
				REQUIRE(steelworks.module->stock[ore].TotalCount() == 0); REQUIRE(steelworks.module->stock[steel].TotalCount() == 0); DepartProductBerth(world, steel_carrier);
				const auto body_id = QueueProductOre(miner, steelworks); const auto *body = GetMutableAsteroidBody(body_id); REQUIRE(body);
				const auto reserve = body->remaining_reserve, extracted = miner.extracted_units; const auto spent = GetUtilityPowerProducer(source_power)->consumed_lifetime_j;
				/** @return Whether actual finite ore reached this precise paused processor, retaining all previous product/construction history. */
				world.Await([&] { return miner.space_orders.cursor == miner.space_orders.entries.size() && miner.cargo.TotalCount() == 0 && steelworks.module->stock[ore].TotalCount() == 20; }, 12000);
				CHECK(body->remaining_reserve == reserve - 20); CHECK(miner.extracted_units == extracted + 20); CHECK(GetUtilityPowerProducer(source_power)->consumed_lifetime_j == spent);
			} else if (phase == "steel-stock") {
				REQUIRE(steelworks.module->stock[ore].TotalCount() == 20); REQUIRE_FALSE(steelworks.module->processing->enabled);
				const auto spent = GetUtilityPowerProducer(source_power)->consumed_lifetime_j; ControlProductProcessor(steelworks, OrbitalRecipeControl::Resume); world.Advance(5 * 74);
				const auto &history = *steelworks.module->processing; CHECK(history.batches.at(MultiLayerRecipeID{"OSTL"}) == 15); CHECK(history.consumed.at(CargoLabel{"NORE"}) == 60);
				CHECK(history.produced.at(CargoLabel{"STEL"}) == 30); CHECK(history.residue_mass_sixteenths.at(MultiLayerRecipeID{"OSTL"}) == 480);
				CHECK(steelworks.module->stock[ore].TotalCount() == 0); CHECK(steelworks.module->stock[steel].TotalCount() == 10); CHECK(GetUtilityPowerProducer(source_power)->consumed_lifetime_j == spent + 277500);
			} else if (phase == "steel-supplied") {
				world.Order(goods_carrier, goodsworks, SpaceOrderAction::Visit);
				/** @return Whether the actual Goods carrier vacated Core and reached its own receiving processor before the Steel carrier requests that Core berth. */
				world.Await([&] { return GetSpaceStructureBerth(goods_carrier, goodsworks).has_value() && goods_carrier.mission.phase == SpaceMissionPhase::Holding; });
				world.Order(steel_carrier, *world.core, SpaceOrderAction::Visit);
				/** @return Whether the original Steel carrier reached the free actual Core point in a stationary state eligible for rocket resupply. */
				world.Await([&] { return GetSpaceStructureBerth(steel_carrier, *world.core).has_value() && steel_carrier.mission.phase == SpaceMissionPhase::Holding; });
				SupplyProductPod(world, steel_carrier); CHECK(center->goods[propellant].TotalCount() == 23);
			} else if (phase == "goods-supplied") {
				SupplyProductPod(world, goods_carrier); CHECK(center->goods[propellant].TotalCount() == 8);
			} else if (phase == "products-loaded") {
				DepartProductBerth(world, miner); world.Order(steel_carrier, steelworks, SpaceOrderAction::Visit); world.Order(goods_carrier, goodsworks, SpaceOrderAction::Visit);
				/** @return Whether both original constructors actually own their respective product-buffer approaches, separate from the Surface release circle. */
				world.Await([&] { return GetSpaceStructureBerth(steel_carrier, steelworks).has_value() && GetSpaceStructureBerth(goods_carrier, goodsworks).has_value(); });
				REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(steelworks.index, steelworks.generation, steel_carrier.index, steel_carrier.generation, steel, uint16_t{2}, true));
				REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(goodsworks.index, goodsworks.generation, goods_carrier.index, goods_carrier.generation, goods, uint16_t{4}, true));
				CHECK(steel_carrier.cargo.TotalCount() == 2); CHECK(goods_carrier.cargo.TotalCount() == 4); CHECK(steelworks.module->stock[steel].TotalCount() == 8); CHECK(goodsworks.module->stock[goods].TotalCount() == 0);
			} else if (phase == "historical-cleared") {
				Station *historical = nullptr;
				// Historical Surface stock is a different physical input from returned product. Preserve it at a genuine existing freight member before recovering any new Steel/Goods into this center's waiting queues.
				for (const auto reference : center->launch_hub->members) {
					auto *member = ResolveLaunchHubMember(home, reference);
					if (member != nullptr && member != center && member->facilities.Test(StationFacility::Train)) { historical = member; break; }
				}
				if (historical == nullptr) historical = &BuildProductMember(*center, false, HISTORICAL_RAIL);
				for (const auto cargo : {steel, goods}) {
					// Every bounded original handling job owns only its complete selected historical holding; native opportunities and fees finish it before the next slice or any unlink.
					while (center->goods[cargo].AvailableCount() != 0) {
						const auto amount = static_cast<uint16_t>(std::min<uint>(MAX_SURFACE_HANDLING_QUANTITY, center->goods[cargo].AvailableCount())); HandleProduct(*center, *historical, cargo, amount);
						/** @return Whether that actual historical packet reservation finished through native throughput. */
						world.Await([&] { return !HasSurfaceHandlingForStation(historical->index); }, 100);
					}
				}
				const auto old_members = center->launch_hub->members;
				// Unlink a copied list of settled old identities: each native command changes the live vector, while all historical packets stay at their original separate Station owners.
				for (const auto reference : old_members) {
					auto *member = ResolveLaunchHubMember(home, reference); if (member == nullptr || member == center) continue;
					REQUIRE(Command<Commands::SetLaunchHub>::Post(member->index, StationID::Invalid(), uint64_t{0}));
				}
				BuildProductMember(*center, false, PRODUCT_RAIL); BuildProductMember(*center, true, PRODUCT_LORRY);
				CHECK(center->goods[steel].TotalCount() == 0); CHECK(center->goods[goods].TotalCount() == 0);
			} else if (phase == "released") {
				REQUIRE(center->goods[steel].TotalCount() == 0); REQUIRE(center->goods[goods].TotalCount() == 0);
				const auto anchor = *GetLaunchCenterPlaneReference(center->launch_center->tile);
				// Each actual carrying point already lies in the original circular release region. Typed release must retain that geometry rather than requiring the precise Core-service point or editing a native position.
				for (auto *carrier : {&steel_carrier, &goods_carrier}) {
					REQUIRE(IsInsideLaunchCenterDropZone(carrier->position, anchor)); REQUIRE(carrier->position != anchor); REQUIRE(carrier->manifest); REQUIRE(carrier->manifest->pods.size() == 1);
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(carrier->index, carrier->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(carrier->index, carrier->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Unload, SpaceOrderTargetKind::LaunchCenter, uint32_t{home.station.base()}, home.generation));
				}
				/** @return Whether both native typed Unloads handed their complete original holds to two actual paid descent owners. */
				world.Await([&] { return steel_carrier.cargo.TotalCount() == 0 && goods_carrier.cargo.TotalCount() == 0; }, 2);
				const auto jobs = CaptureSpaceDescents(); REQUIRE(jobs.size() == 2);
				for (const auto &job : jobs) { CHECK(job.sender_model == 20); CHECK(job.remaining_ticks > 0); CHECK(job.freight_mass == 32); CHECK(job.release_point != anchor); }
				CHECK(steel_carrier.manifest->pending_returns == 1); CHECK(goods_carrier.manifest->pending_returns == 1);
			} else if (phase == "recovered") {
				REQUIRE(HasSpaceDescentForCenter(home));
				/** @return Whether original descent timers and whole recovery headroom actually settled both selected products. */
				world.Await([&] { return !HasSpaceDescentForCenter(home); }, 300);
				CHECK(center->goods[steel].TotalCount() == 2); CHECK(center->goods[goods].TotalCount() == 4);
				CHECK(steel_carrier.manifest->pending_returns == 0); CHECK(goods_carrier.manifest->pending_returns == 0);
				CHECK(steel_carrier.manifest->returns_remaining == 1); CHECK(goods_carrier.manifest->returns_remaining == 1);
			} else if (phase == "handling") {
				auto &rail = ProductMember(PRODUCT_RAIL), &lorry = ProductMember(PRODUCT_LORRY); REQUIRE_FALSE(HasSurfaceHandlingForStation(center->index));
				// Each new endpoint receives its own actual source reservation. Save before the first throughput opportunity because these six units fit within one eight-unit native tick.
				for (auto *member : {&rail, &lorry}) {
					REQUIRE(member->goods[steel].TotalCount() == 0); REQUIRE(member->goods[goods].TotalCount() == 0); HandleProduct(*center, *member, steel, 1); HandleProduct(*center, *member, goods, 2);
				}
				CHECK(center->launch_hub->handling.size() == 4); CHECK(center->goods[steel].TotalCount() == 0); CHECK(center->goods[goods].TotalCount() == 0);
			} else if (phase == "collected") {
				auto &rail = ProductMember(PRODUCT_RAIL), &lorry = ProductMember(PRODUCT_LORRY); REQUIRE(center->launch_hub->handling.size() == 4);
				const auto money = Company::Get(_current_company)->money; const auto profit = ProductSpaceProfit(); const auto remainder = center->launch_hub->handling_cost_remainder;
				world.Advance(1); const auto upkeep = (profit - ProductSpaceProfit()) / 256;
				CHECK_FALSE(HasSurfaceHandlingForStation(center->index)); CHECK(money - Company::Get(_current_company)->money == upkeep + Money{4}); CHECK(center->launch_hub->handling_cost_remainder == remainder);
				for (auto *member : {&rail, &lorry}) { CHECK(member->goods[steel].TotalCount() == 1); CHECK(member->goods[goods].TotalCount() == 2); CHECK(IsLaunchHubEligible(*center, *member)); }
				CHECK(center->goods[steel].TotalCount() == 0); CHECK(center->goods[goods].TotalCount() == 0); CHECK(steelworks.module->stock[steel].TotalCount() == 8); CHECK(goodsworks.module->stock[goods].TotalCount() == 0);
			} else {
				REQUIRE(phase == "inspected"); CHECK(center->goods[steel].TotalCount() == 36); CHECK(center->goods[goods].TotalCount() == 64); CHECK(center->goods[propellant].TotalCount() == 38); CHECK(CaptureSpacePods().size() == 10);
			}
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[steel] == steel_delivered); CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[goods] == goods_delivered);
			REQUIRE(ValidateSpaceVehicles()); REQUIRE(ValidateSpaceStructures()); REQUIRE(ValidateOrbitalRecipes()); REQUIRE(ValidateSurfaceHandling()); world.Save(fmt::format("products-{}-438.sav", phase));
			if (resave) {
				CHECK(ComputeSpaceStructureDigest() == entry_structures); CHECK(ComputeSpaceVehicleDigest() == entry_vehicles); CHECK(ComputeUtilityPowerDigest() == entry_power);
				CHECK(CaptureOrbitalRecipes() == entry_recipes); CHECK(TimerGameTick::counter == entry_tick); CHECK(Company::Get(_current_company)->money == entry_money);
				CHECK(_random.state[0] == entry_random.state[0]); CHECK(_random.state[1] == entry_random.state[1]);
			}
			fmt::print("PROCESSOR_RETURNS native=true phase={} added_stock=false resave={}\n", phase, resave); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(returns_observer, &observe); IConsole::CmdRegister("processor_returns_test", ObserveProductReturns);
	const std::string configuration = config.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
