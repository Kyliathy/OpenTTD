/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_order_actions_native.cpp Genuine typed freight and due-service continuation from exact accepted paid worlds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B
 * All input hulls, cargo, pods, service parts and spent work belong to immutable producer checkpoints. This observer grants no stock, allowance, pose or money.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_order_actions.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../launch_operations.h"
#include "../launch_center_cmd.h"
#include "../water_cmd.h"
#include "../landscape.h"
#include "../station_base.h"
#include "../multilayer_space_content.h"
#include "../map_layer.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/string_consumer.hpp"
#include "../core/random_func.hpp"
#include "../asteroid.h"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *probe_order_observer = nullptr; ///< Invocation-owned actual native application callback; no second simulation.
/** @param args Actual console tokens. @return Whether the explicit current loaded-world observer ran. */
static bool ObserveProbeOrderActions(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(probe_order_observer != nullptr); (*probe_order_observer)(); return true;
}

/** Buy a separate empty service destination through the original landscape and station commands.
 * @param point Original receiving center's orbital anchor; keep the alternate destination inside the same bounded neighborhood.
 * @return Actual newly paid center, with no craft, inventory or recovery claim homed there.
 */
static LaunchCenterRef BuildProbeOrderServiceCenter(SpacePoint point)
{
	// Prefer existing water, then pay for a canal beside otherwise empty flat land. Ordinary placement remains the sole authority for the complete facility.
	for (uint pass = 0; pass < 2; ++pass) {
		// Search the canonical Surface plane and legal rotations near the original route; camera selection and packed-map offsets cannot choose service geometry.
		for (const auto tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) for (uint8_t rotation = 0; rotation < 4; ++rotation) {
			const auto x = TileX(tile), y = TilePlaneY(tile);
			if (!IsInsideLaunchCenterDropZone(point, {static_cast<int32_t>(x * SPACE_Q16), static_cast<int32_t>(y * SPACE_Q16)})) continue;
			const auto area = GetLaunchCenterFootprint(tile, rotation); if (!area) continue;
			// Only the fallback pass may buy water access, and only before constructing an ordinary new facility with its own station identity.
			if (pass == 1 && !HasLaunchCenterWaterAccess(*area)) {
				bool flat_empty = true;
				// Reject the whole footprint before any paid change if it would clear another asset or require terrain alteration.
				for (const auto cell : *area) {
					// Clear/tree land at one flat height is the only eligible canal-side footprint; existing infrastructure remains untouched.
					if ((!IsTileType(cell, TileType::Clear) && !IsTileType(cell, TileType::Trees)) || !IsTileFlat(cell) || GetTileZ(cell) != GetTileZ(tile)) flat_empty = false;
				}
				if (!flat_empty) continue;
				bool canal = false;
				// Buy at most one orthogonal water edge; the native canal quote rejects unavailable land and its real command charges the current company.
				for (uint side = 0; side < 4 && !canal; ++side) for (uint offset = 0; offset < (side < 2 ? area->h : area->w) && !canal; ++offset) {
					const int wx = static_cast<int>(x) + (side == 0 ? -1 : side == 1 ? area->w : static_cast<int>(offset));
					const int wy = static_cast<int>(y) + (side == 2 ? -1 : side == 3 ? area->h : static_cast<int>(offset));
					// Clip plane coordinates before converting to tiles, so the selected access cannot wrap into a different layer or map edge.
					if (wx < 1 || wy < 1 || wx + 1 >= static_cast<int>(Map::SizeX()) || wy + 1 >= static_cast<int>(Map::SizeY())) continue;
					const auto water = TileXYL(wx, wy, MapLayer::Surface);
					// Native construction may affect only empty/tree land with an admitted paid quote; this selector cannot clear an existing facility.
					if ((!IsTileType(water, TileType::Clear) && !IsTileType(water, TileType::Trees)) || CmdBuildCanal({}, water, water, WaterClass::Canal, false).Failed()) continue;
					REQUIRE(Command<Commands::BuildCanal>::Post(water, water, WaterClass::Canal, false)); canal = true;
				}
				if (!canal) continue;
			}
			if (CmdBuildLaunchCenter({}, tile, rotation, NEW_STATION, true).Failed()) continue;
			REQUIRE(Command<Commands::BuildLaunchCenter>::Post(tile, rotation, NEW_STATION, true));
			return GetLaunchCenterReference(*Station::GetByTile(tile));
		}
	}
	FAIL("No ordinary paid alternate service center could be constructed"); return {};
}

/** Non-owning adapter for one exact paid native hull; waits and saves always use the actual application/controller. */
class ProbeOrderNativeWorld {
	const TestArtifactPaths &paths;
public:
	SpaceVehicle &craft; ///< Exact already paid original incarnation, never a newly fabricated test product.
	/** @param paths Explicit invocation output owner. @param craft Actual selected original paid hull. */
	ProbeOrderNativeWorld(const TestArtifactPaths &paths, SpaceVehicle &craft) : paths(paths), craft(craft) {}
	/** @param ticks Requested real synchronized opportunities; renderer/yield callbacks cannot earn work or consume cargo. */
	void Advance(uint ticks) const
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{});
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(45);
		// Count only actual shared field/native-vehicle intervals. CargoDist pauses can yield repeatedly without producing a second action or service step.
		for (uint done = 0; done < ticks;) {
			REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
			if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
			REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
		}
	}
	/** @param ready Actual physical completion predicate. @param maximum Bounded synchronized opportunities before this action must reach it. */
	void Await(const std::function<bool()> &ready, uint maximum = 5000) const
	{
		// A real state predicate decides arrival/settlement; a timeout never substitutes cargo recovery, paid service or physical navigation.
		for (uint tick = 0; !ready(); ++tick) { REQUIRE(tick < maximum); this->Advance(1); }
	}
	/** Replace instructions through actual synchronized native commands; old cargo and producer identities survive.
	 * @param entries Explicit typed actions and generation-qualified targets for this original craft.
	 */
	void Orders(std::initializer_list<SpaceOrder> entries) const
	{
		REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0},
			SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
		uint8_t index = 0;
		// Each instruction uses its actual command argument domain; no StationID conversion or direct saved-queue overwrite can retarget a moving body.
		for (const auto &entry : entries) {
			REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, index++,
				entry.action, entry.target.kind, entry.target.id, entry.target.generation));
		}
	}
	/** Return to the original real home, release actual cargo once and wait for due service; this creates no free pod, part or allowance. */
	void ReturnOrders() const
	{
		const auto home = craft.construction.home; REQUIRE(ResolveLaunchCenter(home) != nullptr);
		const SpaceOrderTarget target{SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation};
		this->Orders({{SpaceOrderAction::Visit, target}, {SpaceOrderAction::Unload, target}, {SpaceOrderAction::ReturnService, target}});
	}
	/** @param name Unique actual native checkpoint basename in this invocation. */
	void Save(std::string_view name) const
	{
		REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
		AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
	}
};

/** Execute one independently bounded genuine action from an explicitly selected accepted paid checkpoint, retaining all interrupted native owners. */
TEST_CASE("Typed native probe orders use actual freight descent and paid service owners", "[.probe-order-actions-native]")
{
	const char *input = std::getenv("OTTD_PROBE_ORDER_SOURCE"), *mode = std::getenv("OTTD_PROBE_ORDER_ACTION"); REQUIRE(input != nullptr); REQUIRE(mode != nullptr);
	const char *selected_id = std::getenv("OTTD_PROBE_ORDER_VEHICLE"), *selected_generation = std::getenv("OTTD_PROBE_ORDER_GENERATION");
	const char *selected_service_base = std::getenv("OTTD_PROBE_ORDER_SERVICE_BASE"); REQUIRE(selected_service_base != nullptr);
	REQUIRE(selected_id != nullptr); REQUIRE(selected_generation != nullptr);
	const auto id = ParseInteger<uint32_t>(selected_id);
	const auto generation = ParseInteger<uint64_t>(selected_generation); REQUIRE(id.has_value()); REQUIRE(generation.has_value());
	const auto baseline = ParseInteger<uint64_t>(selected_service_base); REQUIRE(baseline.has_value()); REQUIRE(*baseline < UINT64_MAX); const auto service_base = *baseline;
	const std::string_view action{mode}; const auto paths = GetTestCaseArtifactPaths("space", "probe-order-actions-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe only the actual fully loaded native world and preserve assertion failures through ordinary application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto *craft = ResolveSpaceCraft({VehicleID{*id}, *generation}); REQUIRE(craft != nullptr); REQUIRE(craft->owner == _current_company); REQUIRE(craft->manifest != nullptr);
			ProbeOrderNativeWorld world(paths, *craft); auto &manifest = *craft->manifest; const auto original = GetSpaceCraftReference(*craft);
			const auto hulls = GetSpaceVehicleCount(); const auto next = GetNextSpaceVehicleGeneration();
			if (const char *corrupt = std::getenv("OTTD_PROBE_ORDER_CORRUPT")) {
				const auto vehicles = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
				const auto random = _random; const auto money = Company::Get(craft->owner)->money;
				REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceVehicleDigest() == vehicles); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == power);
				CHECK(Company::Get(craft->owner)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("PROBE_ORDERS preview_reject=true authority_unchanged=true\n");
			} else if (action == "unload-orbital") {
				REQUIRE(GetSpaceVehicleModel(*craft) == 20); const auto quantity = craft->cargo.TotalCount(); REQUIRE(quantity == 1);
				auto *core = SpaceStructure::GetIfValid(SpaceStructureID{0}); REQUIRE(core != nullptr); const auto cargo = craft->cargo_type;
				const auto stock = core->module->stock[cargo].TotalCount();
				world.Orders({{SpaceOrderAction::Unload, {SpaceOrderTargetKind::Structure, core->index.base(), core->generation}}});
				/** @return Whether actual typed movement and the existing complete-hold exchange finished this one instruction. */
				world.Await([&] { return craft->space_orders.cursor == 1; });
				CHECK(craft->cargo.TotalCount() == 0); CHECK(core->module->stock[cargo].TotalCount() == stock + quantity);
				CHECK(craft->space_orders.blocked == SpaceOrderBlock::Completed);
			} else if (action == "service-supply") {
				REQUIRE(GetSpaceVehicleModel(*craft) == 15); REQUIRE(manifest.supplies[1].TotalCount() == 0); REQUIRE(manifest.supplies[2].TotalCount() == 0);
				const auto home = craft->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr); REQUIRE(center->launch_operations != nullptr);
				REQUIRE(!center->launch_operations->pad_queue.empty());
				// The accepted world retains two paid rockets. The original native pad queue selects its actual admitted carrier; pool-order/unique-model guesses must not substitute another physical producer.
				auto *rocket = ResolveSpaceCraft(center->launch_operations->pad_queue.front()); REQUIRE(rocket != nullptr); REQUIRE(GetSpaceVehicleModel(*rocket) == 14);
				REQUIRE(rocket->owner == craft->owner); REQUIRE(rocket->construction.home == home); world.Orders({});
				const auto missions = rocket->transition ? rocket->transition->missions_completed : 0;
				REQUIRE(rocket->manifest != nullptr);
				if (rocket->manifest->passenger != SpaceCraftRef{}) {
					// The admitted carrier already holds another real paid probe. Unload that actual passenger into original Surface storage before using the same pad owner for freight; retain the extra hull and its embodied recipe.
					const auto passenger = rocket->manifest->passenger;
					REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, passenger.vehicle, passenger.generation, true));
					REQUIRE(ResolveSpaceCraft(passenger) != nullptr); CHECK(ResolveSpaceCraft(passenger)->construction.placement == SpacePlacement::SurfaceStored);
				}
				// Both actual existing Surface labels and real fuel must pass the original native loading, paid departure, whole delivery and rocket recovery owners.
				for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation,
					rocket->index, rocket->generation, supply, uint16_t{1}, false));
				const auto &budget = GetMultiLayerSpaceBudget();
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0},
					static_cast<uint16_t>(budget.outbound_fuel + budget.recovery_fuel), false));
				const auto resupply = GetSpaceResupplyBlock(*rocket, original); CAPTURE(to_underlying(resupply)); REQUIRE(resupply == SpaceFlightBlock::None);
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, craft->index.base(), craft->generation));
				/** @return Whether the original actual paid rocket completed its real resupply and recovery. */
				world.Await([&] { return rocket->transition->missions_completed == missions + 1; });
				CHECK(manifest.supplies[1].TotalCount() == 1); CHECK(manifest.supplies[2].TotalCount() == 1);
				CHECK(craft->cargo.TotalCount() == 20); CHECK(manifest.returns_remaining == 2); CHECK(manifest.pods.size() == 2);
			} else if (action == "first-pending") {
				REQUIRE(GetSpaceVehicleModel(*craft) == 15); REQUIRE(craft->cargo.TotalCount() == 20); REQUIRE(manifest.pods.size() == 2);
				REQUIRE(manifest.returns_remaining == 2); REQUIRE(manifest.services_completed == service_base); world.ReturnOrders();
				/** @return Whether the real pod descent now retains the return claim while the reached service instruction waits. */
				world.Await([&] { return craft->space_orders.blocked == SpaceOrderBlock::ReturnsPending; });
				CHECK(manifest.pending_returns == 1); CHECK(manifest.returns_remaining == 2); CHECK(manifest.service_ticks == 0);
			} else if (action == "first-return") {
				REQUIRE(manifest.pending_returns == 1);
				/** @return Whether actual descent settled and Return continued with its partly used allowance, without purchasing premature service. */
				world.Await([&] { return craft->space_orders.cursor == 3; });
				CHECK(manifest.pending_returns == 0); CHECK(manifest.returns_remaining == 1); CHECK(manifest.services_completed == service_base); CHECK(manifest.pods.size() == 1);
			} else if (action == "mine-again") {
				REQUIRE(manifest.returns_remaining == 1); REQUIRE(craft->cargo.TotalCount() == 0);
				world.Orders({{SpaceOrderAction::Mine, {SpaceOrderTargetKind::Body, 0, 0}}});
				/** @return Whether the same original probe genuinely extracted another complete finite hold through normal lease/work owners. */
				world.Await([&] { return craft->space_orders.cursor == 1; });
				CHECK(craft->cargo.TotalCount() == 20); CHECK(manifest.returns_remaining == 1); CHECK(manifest.services_completed == service_base);
			} else if (action == "second-pending") {
				REQUIRE(craft->cargo.TotalCount() == 20); REQUIRE(manifest.pods.size() == 1); world.ReturnOrders();
				/** @return Whether the second real pod retains its own return claim, without spending the service parts early. */
				world.Await([&] { return craft->space_orders.blocked == SpaceOrderBlock::ReturnsPending; });
				CHECK(manifest.pending_returns == 1); CHECK(manifest.returns_remaining == 1); CHECK(manifest.service_ticks == 0);
			} else if (action == "service-partial") {
				REQUIRE(manifest.pending_returns == 1);
				/** @return Whether actual recovery exhausted the allowance and the original onboard producer earned exactly37 of its148 work ticks. */
				world.Await([&] { return manifest.service_ticks == 111; });
				CHECK(manifest.pending_returns == 0); CHECK(manifest.returns_remaining == 0); CHECK(manifest.services_completed == service_base);
				CHECK(IsSpaceOnboardServiceOrder(*craft)); CHECK(craft->space_orders.cursor == 2); CHECK(craft->space_orders.work_ticks == 0);
				CHECK(manifest.supplies[1].TotalCount() == 1); CHECK(manifest.supplies[2].TotalCount() == 1);
			} else if (action == "service-pause") {
				REQUIRE(manifest.service_ticks == 111); REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(craft->index, craft->generation, true));
				world.Advance(10); CHECK(manifest.service_ticks == 111); CHECK(manifest.returns_remaining == 0); CHECK(manifest.service_paused);
			} else if (action == "service-resume") {
				REQUIRE(manifest.service_paused); REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(craft->index, craft->generation, false));
				CHECK(manifest.service_ticks == 111); CHECK(manifest.returns_remaining == 0); CHECK_FALSE(manifest.service_paused);
			} else if (action == "service-complete") {
				REQUIRE(manifest.service_ticks == 111);
				/** @return Whether the original remaining work consumed its actual two parts once and completed this instruction. */
				world.Await([&] { return craft->space_orders.cursor == 3; });
				CHECK(manifest.services_completed == service_base + 1); CHECK(manifest.service_ticks == 0); CHECK(manifest.returns_remaining == 2);
				CHECK(manifest.supplies[1].TotalCount() == 0); CHECK(manifest.supplies[2].TotalCount() == 0);
			} else if (action == "service-center-loss") {
				REQUIRE(manifest.pending_returns == 1); REQUIRE(manifest.service_ticks == 0);
				const auto home = craft->construction.home; auto *origin = ResolveLaunchCenter(home); REQUIRE(origin != nullptr);
				CHECK(CmdRemoveLaunchCenter({}, origin->launch_center->tile).Failed()); // The original center still owns paid craft/recovery and must keep its normal demolition guard.
				const auto target_ref = BuildProbeOrderServiceCenter(craft->home); REQUIRE(target_ref != home);
				auto *center = ResolveLaunchCenter(target_ref); REQUIRE(center != nullptr); const auto tile = center->launch_center->tile;
				const SpaceOrderTarget target{SpaceOrderTargetKind::LaunchCenter, target_ref.station.base(), target_ref.generation};
				world.Orders({{SpaceOrderAction::Visit, target}, {SpaceOrderAction::ReturnService, target}});
				/** @return Whether the original descent actually exhausted its allowance, then physical travel reached this new paid target and earned37 genuine onboard work ticks. */
				world.Await([&] { return manifest.service_ticks == 111; });
				REQUIRE(Command<Commands::RemoveLaunchCenter>::Post(tile)); // No craft/pod/stock/recovery is homed at this separate empty target; the original receiving center and its actual assets survive.
				CHECK(ResolveLaunchCenter(target_ref) == nullptr); CHECK(ResolveLaunchCenter(home) == origin); CHECK(manifest.service_ticks == 111); CHECK(manifest.returns_remaining == 0);
				CHECK(craft->space_orders.blocked == SpaceOrderBlock::TargetRemoved); CHECK(IsSpaceOnboardServiceOrder(*craft));
			} else if (action == "service-after-loss") {
				REQUIRE(manifest.service_ticks == 111); REQUIRE(craft->space_orders.blocked == SpaceOrderBlock::TargetRemoved);
				world.Advance(112);
				CHECK(manifest.services_completed == service_base + 1); CHECK(manifest.returns_remaining == 2); CHECK(manifest.service_ticks == 0);
				CHECK(craft->space_orders.cursor == 1); CHECK(craft->space_orders.blocked == SpaceOrderBlock::TargetRemoved);
				CHECK(manifest.supplies[1].TotalCount() == 0); CHECK(manifest.supplies[2].TotalCount() == 0);
			} else if (action == "repeat") {
				REQUIRE(manifest.services_completed == service_base + 1); world.Advance(10);
				CHECK(manifest.services_completed == service_base + 1); CHECK(manifest.returns_remaining == 2); CHECK(craft->space_orders.cursor == 3);
			} else FAIL("Unknown explicit probe-order action");
			CHECK(ResolveSpaceCraft(original) == craft); CHECK(GetSpaceVehicleCount() == hulls); CHECK(GetNextSpaceVehicleGeneration() == next);
			world.Save(fmt::format("probe-orders-{}-430.sav", action));
			fmt::print("PROBE_ORDERS native=true action={} vehicle={} generation={} cursor={} work={} returns={} pending={} services={} service_ticks={} cargo={}\n",
				action, craft->index.base(), craft->generation, craft->space_orders.cursor, craft->space_orders.work_ticks, manifest.returns_remaining,
				manifest.pending_returns, manifest.services_completed, manifest.service_ticks, craft->cargo.TotalCount());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(probe_order_observer, &observe); IConsole::CmdRegister("probe_order_actions_test", ObserveProbeOrderActions);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
