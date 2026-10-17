/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_network_native.cpp Original paid heavy commands traverse actual native server serialization and frame admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9DC
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group20; generation-bound synchronized mission admission.
 * Local/network twins consume dated paid worlds; native ticks alone earn mining, service, movement and recovery.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../network/network.h"
#include "../network/network_internal.h"
#include "../orbital_service.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_network_observer = nullptr; ///< Invocation observer only; actual native owners retain all physical state.

/** @param args Parameter-free callback in the actual loaded world. @return Whether the installed observer ran. */
static bool ObserveHeavyNetwork(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_network_observer != nullptr); (*heavy_network_observer)(); return true;
}

/** @return Original physical/work authority for same-frame and repeated-drain checks. */
static auto HeavyNetworkAuthority()
{
	return std::array<uint64_t, 11>{ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest(), ComputeUtilityPowerDigest(), ComputeSpaceManifestDigest(),
		ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(), ComputeSpaceOrdersDigest(), ComputeAsteroidFieldDigest(GetAsteroidField()),
		ComputeLaunchProductionDigest(), ComputeLaunchCenterDigest(), ComputeSpaceDescentDigest()};
}

/** Dispatch an actual typed command locally or through native SendNet/distribution/PostFromNet.
 * @tparam Tcommand Original registered command, retaining its native argument serializer.
 * @tparam Targs Original typed argument values accepted by that command.
 * @param network Whether to exercise native server queues instead of immediate local admission.
 * @param company Actual owner whose native command authorization is checked.
 * @param args Complete original typed command arguments; no private state callback substitutes execution.
 */
template <Commands Tcommand, typename... Targs>
static void DispatchHeavyNetwork(bool network, CompanyID company, Targs... args)
{
	const auto before = HeavyNetworkAuthority(); const auto elapsed = GetAsteroidField().elapsed_ticks;
	const auto money = Company::Get(company)->money; const auto random = _random;
	AutoRestoreBackup server(_network_server, network); AutoRestoreBackup networking(_networking, network);
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); AutoRestoreBackup frame(_frame_counter); AutoRestoreBackup maximum(_frame_counter_max, _frame_counter);
	if (network) {
		Command<Tcommand>::SendNet({}, company, args...); CHECK(HeavyNetworkAuthority() == before);
		NetworkDistributeCommands(); NetworkExecuteLocalCommandQueue(); CHECK(HeavyNetworkAuthority() == before); CHECK(Company::Get(company)->money == money);
		CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
		++_frame_counter; NetworkExecuteLocalCommandQueue(); const auto committed = HeavyNetworkAuthority(); const auto settled_money = Company::Get(company)->money;
		NetworkExecuteLocalCommandQueue(); CHECK(HeavyNetworkAuthority() == committed); CHECK(Company::Get(company)->money == settled_money); NetworkFreeLocalCommandQueue();
	} else {
		REQUIRE(Command<Tcommand>::Post(args...));
	}
	CHECK(GetAsteroidField().elapsed_ticks == elapsed);
}

/** Replace this actual hull's queue through original typed command admission.
 * @param craft Original paid native hull.
 * @param entry One actual Visit destination; movement still requires later native opportunities.
 * @param network Whether both clear and insert traverse original server queues.
 */
static void OrderHeavyNetwork(SpaceVehicle &craft, const SpaceOrder &entry, bool network)
{
	DispatchHeavyNetwork<Commands::EditSpaceOrder>(network, craft.owner, craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0});
	DispatchHeavyNetwork<Commands::EditSpaceOrder>(network, craft.owner, craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, entry.action, entry.target.kind, entry.target.id, entry.target.generation);
	CHECK(craft.space_orders.entries == std::vector<SpaceOrder>{entry});
}

/** Actual paid mining, reserved service, last-shell descent and precise podless return have identical local/server outcomes. */
TEST_CASE("Original heavy commands serialize and settle once through native server frames", "[.heavy-network-native]")
{
	const char *source = std::getenv("OTTD_HEAVY_NETWORK_SOURCE"), *chosen = std::getenv("OTTD_HEAVY_NETWORK_CASE"), *selected = std::getenv("OTTD_HEAVY_NETWORK_STAGE"), *mode = std::getenv("OTTD_HEAVY_NETWORK_MODE");
	REQUIRE(source != nullptr); REQUIRE(chosen != nullptr); REQUIRE(selected != nullptr); REQUIRE(mode != nullptr);
	const std::string_view scenario{chosen}, stage{selected}; const bool network = std::string_view{mode} == "network";
	const auto paths = GetTestCaseArtifactPaths("space", "heavy-network-native"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve loaded-world assertions through normal application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			auto *heavy = ResolveSpaceCraft({VehicleID{22}, 8}), *builder = ResolveSpaceCraft({VehicleID{18}, 4}); REQUIRE(heavy != nullptr); REQUIRE(builder != nullptr); REQUIRE(heavy->manifest != nullptr);
			auto *dock = ResolveSpaceStructure({SpaceStructureID{4}, 5}); const auto fleet = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration();
			auto *center = ResolveLaunchCenter(world.rocket->construction.home); REQUIRE(center != nullptr); const auto home = GetLaunchCenterReference(*center); const auto cargo = GetCargoTypeByLabel(CargoLabel{"NORE"});
			if (scenario == "mining") {
				if (stage == "initial") {
					const AsteroidBody *target = nullptr;
					// Select the same actual nearby moving metallic source as the accepted role proof, preserving finite stock, lease and native geometry.
					for (const auto &body : GetAsteroidField().bodies) {
						if (body.resource_class != AsteroidClass::Metallic || body.remaining_reserve < 40 || body.mining_slot != VehicleID::Invalid() || body.survey_slot != INVALID_SPACE_SURVEY) continue;
						// Both velocity components must be zero to exclude motion; this case needs real attachment tracking, not an invented stationary source.
						if (body.motion.velocity_x == 0 && body.motion.velocity_y == 0) continue;
						if (target == nullptr || SpaceDistanceSquared(heavy->position, body.motion.position) < SpaceDistanceSquared(heavy->position, target->motion.position)) target = &body;
					}
					REQUIRE(target != nullptr); DispatchHeavyNetwork<Commands::SetSpaceMissionChecked>(network, heavy->owner, heavy->index, heavy->generation, target->id.base()); world.Advance(1);
					CHECK(heavy->mission.phase == SpaceMissionPhase::Approach); CHECK(heavy->mission.target == target->id);
				} else {
					REQUIRE(stage == "complete");
					/** @return Whether this original hull physically acquired its finite body's exclusive native lease. */
					world.Await([&] { return heavy->mission.phase == SpaceMissionPhase::Mining; }, 12000); world.Advance(37);
					CHECK(heavy->mission.work_ticks == 37); CHECK(heavy->cargo.TotalCount() == 0); CHECK(GetMutableAsteroidBody(heavy->mission.target)->mining_slot == heavy->index);
				}
			} else if (scenario == "service") {
				REQUIRE(dock != nullptr); auto &first = dock->module->service; auto &second = dock->module->dock_service;
				if (stage == "initial") {
					REQUIRE(first.paused); REQUIRE(first.remaining_ticks == 259); REQUIRE(second.remaining_ticks == 135);
					DispatchHeavyNetwork<Commands::ServiceOrbitalProbe>(network, heavy->owner, dock->index, dock->generation, heavy->index, heavy->generation, OrbitalServiceControl::Resume); world.Advance(17);
					CHECK(first.remaining_ticks == 242); CHECK(second.remaining_ticks == 135); CHECK_FALSE(first.paused);
				} else {
					REQUIRE(stage == "complete"); REQUIRE(first.remaining_ticks == 242); REQUIRE(second.remaining_ticks == 135); world.Advance(377); world.Advance(9);
					CHECK(first.remaining_ticks == 0); CHECK(second.remaining_ticks == 0); CHECK(heavy->manifest->services_completed == 1); CHECK(heavy->manifest->returns_remaining == 2); CHECK(dock->module->services_completed == 2);
				}
			} else if (scenario == "descent") {
				if (stage == "initial") {
					REQUIRE(heavy->cargo.TotalCount() == 40); REQUIRE(heavy->manifest->pods.size() == 1);
					OrderHeavyNetwork(*heavy, {SpaceOrderAction::Visit, {SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation}}, network); const auto anchor = *GetLaunchCenterPlaneReference(center->launch_center->tile);
					/** @return Whether actual travel reached the authorized inclusive Surface release region. */
					world.Await([&] { return IsInsideLaunchCenterDropZone(heavy->position, anchor); }, 12000); const auto shell = heavy->manifest->pods.front();
					DispatchHeavyNetwork<Commands::ReleaseOrbitalCargo>(network, heavy->owner, heavy->index, heavy->generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{40});
					const auto jobs = CaptureSpaceDescents(); REQUIRE(jobs.size() == 1); auto *pod = ResolveSpacePod(jobs.front().pod); REQUIRE(pod != nullptr); REQUIRE(pod->descent != nullptr); REQUIRE(pod->descent->remaining_ticks >= 111);
					world.Advance(pod->descent->remaining_ticks - 111); CHECK(pod->descent->quantity == 40); CHECK(pod->descent->sender_model == 21); CHECK(heavy->manifest->pending_returns == 1);
				} else {
					REQUIRE(stage == "complete"); const auto jobs = CaptureSpaceDescents(); REQUIRE(jobs.size() == 1); REQUIRE(jobs.front().remaining_ticks == 111); const auto shell = jobs.front().pod; const auto stock = center->goods[cargo].TotalCount();
					world.Advance(111); CHECK(CaptureSpaceDescents().empty()); CHECK(ResolveSpacePod(shell) == nullptr); CHECK(center->goods[cargo].TotalCount() == stock + 40); CHECK(heavy->manifest->returns_remaining == 1); CHECK(heavy->manifest->pending_returns == 0);
					world.Advance(10); CHECK(center->goods[cargo].TotalCount() == stock + 40);
				}
			} else {
				REQUIRE(scenario == "orbital"); REQUIRE(dock != nullptr);
				if (stage == "initial") {
					REQUIRE(heavy->cargo.TotalCount() == 40); REQUIRE(heavy->space_orders.blocked == SpaceOrderBlock::ApproachBusy);
					OrderHeavyNetwork(*builder, {SpaceOrderAction::Visit, {SpaceOrderTargetKind::Structure, dock->index.base(), dock->generation}}, network);
				} else {
					REQUIRE(stage == "complete"); const auto stock = world.core->module->stock[cargo].TotalCount();
					/** @return Whether original physical departure admitted the whole actual Core unload. */
					world.Await([&] { return heavy->space_orders.cursor == heavy->space_orders.entries.size(); }, 12000);
					CHECK(heavy->cargo.TotalCount() == 0); CHECK(heavy->manifest->returns_remaining == 0); CHECK(world.core->module->stock[cargo].TotalCount() == stock + 40); world.Advance(10); CHECK(world.core->module->stock[cargo].TotalCount() == stock + 40);
				}
			}
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == generation);
			world.Save(fmt::format("heavy-network-{}-{}-{}-437.sav", scenario, stage, mode)); fmt::print("HEAVY_NETWORK native=true case={} stage={} mode={}\n", scenario, stage, mode); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(heavy_network_observer, &observe); IConsole::CmdRegister("heavy_network_test", ObserveHeavyNetwork); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
