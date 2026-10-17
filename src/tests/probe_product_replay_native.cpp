/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_product_replay_native.cpp Paid processor commands retain identical local/network physical owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md task8C.4
 * Original accepted paid saves supply every packet, hull, module, shell, station and work account.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_func.h"
#include "../core/random_func.hpp"
#include "../network/network.h"
#include "../network/network_internal.h"
#include "../orbital_recipe.h"
#include "../station_base.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *product_replay_observer = nullptr; ///< Per-process original game-entry observer, without independent gameplay ownership.

/** @param args Parameter-free native console arguments. @return Whether this exact loaded-world observer ran. */
static bool ObserveProductReplay(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(product_replay_observer); (*product_replay_observer)(); return true;
}

/** @return Original material, work, cargo, queue and native endpoint owners; unchanged digests detect early or repeated dispatch. */
static auto ProductReplayDigest()
{
	return std::tuple{ComputeSpaceStructureDigest(), ComputeSpaceVehicleDigest(), ComputeUtilityPowerDigest(), ComputeSpaceOrdersDigest(),
		ComputeSpaceDescentDigest(), ComputeSurfaceHandlingDigest(), ComputeLaunchCenterDigest(), ComputeLaunchHubDigest(), ComputeOrbitalRecipeDigest()};
}

/** Exercise the actual command scheduler without granting simulation opportunities.
 * @param network Whether to enqueue through original SendNet rather than local Post.
 * @param submit Original typed command submission, with the same canonical arguments in both modes.
 */
static void DispatchProductCommand(bool network, const std::function<void(bool)> &submit)
{
	const auto before = ProductReplayDigest(); const auto elapsed = GetAsteroidField().elapsed_ticks;
	const auto money = Company::Get(_current_company)->money; const auto random = _random;
	AutoRestoreBackup server(_network_server, network); AutoRestoreBackup networking(_networking, network);
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); AutoRestoreBackup frame(_frame_counter); AutoRestoreBackup maximum(_frame_counter_max, _frame_counter);
	submit(network);
	if (network) {
		// Scheduling is not execution: no physical packet, paid work, funds or allowance may change before the authoritative next frame admits the original command.
		CHECK(ProductReplayDigest() == before); CHECK(Company::Get(_current_company)->money == money);
		CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
		NetworkDistributeCommands(); NetworkExecuteLocalCommandQueue(); CHECK(ProductReplayDigest() == before);
		++_frame_counter; NetworkExecuteLocalCommandQueue(); const auto committed = ProductReplayDigest(); const auto paid = Company::Get(_current_company)->money;
		// Re-executing the already drained queue cannot repeat a transfer, reservation, charge, instruction or processing control.
		NetworkExecuteLocalCommandQueue(); CHECK(ProductReplayDigest() == committed); CHECK(Company::Get(_current_company)->money == paid); NetworkFreeLocalCommandQueue();
	}
	CHECK(GetAsteroidField().elapsed_ticks == elapsed);
}

/** @param kind Actual original constructed processor role. @return Its unique existing paid site, never a proxy or newly created module. */
static SpaceStructure &ReplayProcessor(SpaceBuildKind kind)
{
	SpaceStructure *result = nullptr;
	// Only the original actual role may supply this replay; an ambiguous pool match would silently substitute another material/work owner.
	for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == kind) { REQUIRE(result == nullptr); result = site; }
	REQUIRE(result); REQUIRE(IsOrbitalModuleOperational(*result)); return *result;
}

/** Actual paid product commands must converge under local and authoritative network dispatch. */
TEST_CASE("Paid product controls freight release and handling replay through original network dispatch", "[.probe-product-replay-native]")
{
	const char *source = std::getenv("OTTD_PROBE_PRODUCT_SOURCE"), *selected = std::getenv("OTTD_PROBE_PRODUCT_ACTION");
	REQUIRE(source); REQUIRE(selected); const std::string_view action{selected}; const bool network = std::getenv("OTTD_PROBE_PRODUCT_NETWORK") != nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "probe-product-replay"); const auto configuration = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Observe the original ordinary loaded paid world, retaining assertion failures across orderly engine shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			auto &steelworks = ReplayProcessor(SpaceBuildKind::Steelworks), &goodsworks = ReplayProcessor(SpaceBuildKind::Goodsworks);
			auto &steel_carrier = *world.builders.front(), &goods_carrier = *world.builders.back(); const auto fleet = GetSpaceVehicleCount();
			const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
			const auto home = world.rocket->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center); REQUIRE(center->launch_hub);
			const auto steel_delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[steel], goods_delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[goods];
			if (action == "processing") {
				REQUIRE_FALSE(steelworks.module->processing->enabled);
				/** @param queued Whether original SendNet owns this control submission. */
				DispatchProductCommand(network, [&](bool queued) {
					if (queued) Command<Commands::ControlOrbitalRecipe>::SendNet({}, _current_company, steelworks.index, steelworks.generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Resume));
					else REQUIRE(Command<Commands::ControlOrbitalRecipe>::Post(steelworks.index, steelworks.generation, uint32_t{0}, static_cast<uint8_t>(OrbitalRecipeControl::Resume)));
				});
				REQUIRE(steelworks.module->processing->enabled); world.Advance(370);
				CHECK(steelworks.module->stock[steel].TotalCount() == 10); CHECK(steelworks.module->processing->batches.at(MultiLayerRecipeID{"OSTL"}) == 15);
			} else if (action == "freight") {
				// Original selected export packets return to their own producers through the same explicit exchange command; no new packet or provenance is installed for replay.
				for (const auto &[site, craft, cargo, quantity] : {std::tuple{&steelworks, &steel_carrier, steel, uint16_t{2}}, std::tuple{&goodsworks, &goods_carrier, goods, uint16_t{4}}}) {
					REQUIRE(craft->cargo.TotalCount() == quantity);
					/** @param queued Whether original SendNet owns this whole actual freight submission. */
					DispatchProductCommand(network, [&](bool queued) {
						if (queued) Command<Commands::MoveOrbitalFreight>::SendNet({}, _current_company, site->index, site->generation, craft->index, craft->generation, cargo, quantity, false);
						else REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(site->index, site->generation, craft->index, craft->generation, cargo, quantity, false));
					});
					CHECK(craft->cargo.TotalCount() == 0);
				}
				CHECK(steelworks.module->stock[steel].TotalCount() == 10); CHECK(goodsworks.module->stock[goods].TotalCount() == 4);
			} else if (action == "release") {
				// Both original carriers use typed launch-center commands. Arrival at a processor berth never grants precise Core service or bypasses the original release circle.
				for (auto *craft : {&steel_carrier, &goods_carrier}) {
					for (const auto edit : {SpaceOrderEdit::Clear, SpaceOrderEdit::Insert}) {
						/** @param queued Whether original SendNet owns this typed full-width instruction. */
						DispatchProductCommand(network, [&](bool queued) {
							if (queued) Command<Commands::EditSpaceOrder>::SendNet({}, _current_company, craft->index, craft->generation, edit, uint8_t{0}, SpaceOrderAction::Unload, SpaceOrderTargetKind::LaunchCenter, uint32_t{home.station.base()}, home.generation);
							else REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, edit, uint8_t{0}, SpaceOrderAction::Unload, SpaceOrderTargetKind::LaunchCenter, uint32_t{home.station.base()}, home.generation));
						});
					}
				}
				world.Advance(1); CHECK(CaptureSpaceDescents().size() == 2); CHECK(steel_carrier.cargo.TotalCount() == 0); CHECK(goods_carrier.cargo.TotalCount() == 0);
			} else {
				REQUIRE(action == "handling"); REQUIRE(center->launch_hub->members.size() == 2);
				// Each actual linked train/lorry endpoint receives its own complete reserved share. Save before throughput so original jobs, rather than already collected packets, own these six units.
				for (const auto member : center->launch_hub->members) {
					for (const auto &[cargo, quantity] : {std::pair{steel, uint16_t{1}}, std::pair{goods, uint16_t{2}}}) {
						/** @param queued Whether original SendNet owns this exact original Surface reservation. */
						DispatchProductCommand(network, [&](bool queued) {
							if (queued) Command<Commands::StartSurfaceHandling>::SendNet({}, _current_company, member.station, member.generation, home.station, home.generation, cargo, quantity, false);
							else REQUIRE(Command<Commands::StartSurfaceHandling>::Post(member.station, member.generation, home.station, home.generation, cargo, quantity, false));
						});
					}
				}
				CHECK(center->launch_hub->handling.size() == 4); CHECK(center->goods[steel].TotalCount() == 0); CHECK(center->goods[goods].TotalCount() == 0);
			}
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[steel] == steel_delivered); CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[goods] == goods_delivered);
			REQUIRE(ValidateSpaceVehicles()); REQUIRE(ValidateSpaceStructures()); REQUIRE(ValidateOrbitalRecipes()); REQUIRE(ValidateSurfaceHandling());
			world.Save(fmt::format("probe-product-{}-438.sav", action)); fmt::print("PROBE_PRODUCT replay=true action={} network={} added_stock=false\n", action, network); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(product_replay_observer, &observe); IConsole::CmdRegister("probe_product_replay_test", ObserveProductReplay);
	const std::string config = configuration.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
