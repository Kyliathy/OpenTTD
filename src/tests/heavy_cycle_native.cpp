/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_cycle_native.cpp Actual paid Dock service, finite mining, last-shell Surface descent and podless precise Core return.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9DB
 * Only original commands and native opportunities change physical owners; inherited starting-content procurement remains unverified.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_func.h"
#include "../console_internal.h"
#include "../orbital_service.h"
#include "../space_mining_tuning.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_cycle_observer = nullptr; ///< Invocation callback only; no parallel asset/work authority.

/** @param args Parameter-free callback in the actual loaded world. @return Whether the installed observer ran. */
static bool ObserveHeavyCycle(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_cycle_observer != nullptr); (*heavy_cycle_observer)(); return true;
}

/** Select actual nearby finite ore and queue its explicit whole-hold receiver before native work begins.
 * @param world Actual loaded-world command/tick adapter.
 * @param craft Original paid heavy miner, empty and serviced.
 * @param kind Authorized receiver type; Surface needs an actual shell, Core needs a precise berth.
 * @param id Existing receiver's native identity.
 * @param generation Existing receiver's native incarnation.
 */
static void MineHeavyCycle(AssemblyNativeWorld &world, SpaceVehicle &craft, SpaceOrderTargetKind kind, uint32_t id, uint64_t generation)
{
	REQUIRE(craft.cargo.TotalCount() == 0); const AsteroidBody *target = nullptr;
	// Choose existing available finite metallic stock by real distance; neither reserve nor arrival geometry may be manufactured for the cycle.
	for (const auto &body : GetAsteroidField().bodies) {
		if (body.resource_class != AsteroidClass::Metallic || body.remaining_reserve < 40 || body.mining_slot != VehicleID::Invalid() || body.survey_slot != INVALID_SPACE_SURVEY) continue;
		if (target == nullptr || SpaceDistanceSquared(craft.position, body.motion.position) < SpaceDistanceSquared(craft.position, target->motion.position)) target = &body;
	}
	REQUIRE(target != nullptr); const auto reserve = target->remaining_reserve, extracted = craft.extracted_units;
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, target->id.base(), uint64_t{0}));
	// A podless hull must have its actual module Unload route before extraction is admitted; neither a nearby station nor its remaining quota is a substitute.
	if (craft.manifest->pods.empty()) CHECK(GetSpaceMiningBlock(craft) == SpaceOrderBlock::PodsMissing);
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{1}, SpaceOrderAction::Unload, kind, id, generation));
	CHECK(GetSpaceMiningBlock(craft) == SpaceOrderBlock::None);
	/** @return Whether native finite extraction filled this original hold and released its actual body lease for the explicit receiver. */
	world.Await([&] { return craft.cargo.TotalCount() == 40 && craft.space_orders.cursor == 1; }, 12000);
	CHECK(craft.extracted_units == extracted + 40); CHECK(target->remaining_reserve == reserve - 40); CHECK(target->mining_slot == VehicleID::Invalid()); CHECK(craft.mission.work_ticks == 0);
}

/** Original paid bills and last shell finance a full interrupted-service/two-return heavy cycle without any additional asset fixture. */
TEST_CASE("Actual paid heavy service funds last-shell Surface and podless orbital returns", "[.heavy-cycle-native]")
{
	const char *source = std::getenv("OTTD_HEAVY_CYCLE_SOURCE"), *selected = std::getenv("OTTD_HEAVY_CYCLE_PHASE"); REQUIRE(source != nullptr); REQUIRE(selected != nullptr);
	const std::string_view phase{selected}; const auto paths = GetTestCaseArtifactPaths("space", "heavy-cycle-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve actual-world assertions through ordinary application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			auto *heavy = ResolveSpaceCraft({VehicleID{22}, 8}), *builder = ResolveSpaceCraft({VehicleID{18}, 4}), *small = ResolveSpaceCraft({VehicleID{16}, 2});
			REQUIRE(heavy != nullptr); REQUIRE(builder != nullptr); REQUIRE(small != nullptr); REQUIRE(heavy->manifest != nullptr); REQUIRE(builder->manifest != nullptr); REQUIRE(small->manifest != nullptr);
			auto *dock = ResolveSpaceStructure({SpaceStructureID{4}, 5}); REQUIRE(dock != nullptr); REQUIRE(IsOrbitalModuleOperational(*dock));
			auto &first = dock->module->service; auto &second = dock->module->dock_service;
			auto *center = ResolveLaunchCenter(world.rocket->construction.home); REQUIRE(center != nullptr); const auto home = GetLaunchCenterReference(*center);
			const auto cargo = GetCargoTypeByLabel(CargoLabel{"NORE"}); REQUIRE(IsValidCargoType(cargo));
			const auto fleet = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration(), pod_generation = GetNextSpacePodGeneration();
			/** @return Actual common Core source settlement, never a copied electrical balance. */
			auto used = [&] { return GetUtilityPowerProducer(world.core->module->producer)->consumed_lifetime_j; };
			if (phase == "paused") {
				REQUIRE(first.remaining_ticks == 259); REQUIRE(second.remaining_ticks == 148); REQUIRE(first.model == 21); REQUIRE(second.model == 20);
				REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(dock->index, dock->generation, heavy->index, heavy->generation, OrbitalServiceControl::Pause));
				const auto before = used(); world.Advance(13); CHECK(first.paused); CHECK(first.remaining_ticks == 259); CHECK(second.remaining_ticks == 135); CHECK(used() == before + 13 * 750);
			} else if (phase == "resumed") {
				REQUIRE(first.paused); REQUIRE(first.remaining_ticks == 259); REQUIRE(second.remaining_ticks == 135);
				REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(dock->index, dock->generation, heavy->index, heavy->generation, OrbitalServiceControl::Resume));
				const auto before = used(); world.Advance(17); CHECK_FALSE(first.paused); CHECK(first.remaining_ticks == 242); CHECK(second.remaining_ticks == 135); CHECK(used() == before + 17 * 750);
			} else if (phase == "serviced") {
				REQUIRE(first.remaining_ticks == 242); REQUIRE(second.remaining_ticks == 135); const auto before = used(); world.Advance(377);
				CHECK(first.remaining_ticks == 0); CHECK(second.remaining_ticks == 0); CHECK(used() == before + 377 * 750);
				world.Advance(9); CHECK(used() == before + 377 * 750); CHECK(heavy->manifest->services_completed == 1); CHECK(heavy->manifest->returns_remaining == 2);
				CHECK(builder->manifest->services_completed == 1); CHECK(dock->module->services_completed == 2); CHECK(world.core->module->services_completed == 1);
				CHECK(CmdServiceOrbitalProbe({}, dock->index, dock->generation, heavy->index, heavy->generation, OrbitalServiceControl::Start).Failed());
			} else if (phase == "surface-mined") {
				REQUIRE(heavy->manifest->pods.size() == 1); REQUIRE(heavy->manifest->returns_remaining == 2);
				MineHeavyCycle(world, *heavy, SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation); CHECK(heavy->extracted_units == 120);
			} else if (phase == "descent") {
				REQUIRE(heavy->cargo.TotalCount() == 40); REQUIRE(heavy->manifest->pods.size() == 1);
				/** @return Whether the actual Earth Unload committed this whole heavy hold to its original paid shell. */
				world.Await([&] { return !CaptureSpaceDescents().empty(); }, 12000); const auto rows = CaptureSpaceDescents(); REQUIRE(rows.size() == 1);
				auto *pod = ResolveSpacePod(rows.front().pod); REQUIRE(pod != nullptr); REQUIRE(pod->descent != nullptr); REQUIRE(pod->descent->remaining_ticks >= 111);
				world.Advance(pod->descent->remaining_ticks - 111); const auto &job = *pod->descent;
				CHECK(job.quantity == 40); CHECK(job.packets.TotalCount() == 40); CHECK(job.remaining_ticks == 111); CHECK(job.sender_model == 21); CHECK(job.unit_weight == 16); CHECK(job.freight_mass == 640);
				CHECK(heavy->cargo.TotalCount() == 0); CHECK(heavy->manifest->pods.empty()); CHECK(heavy->manifest->pending_returns == 1); CHECK(heavy->manifest->returns_remaining == 2);
			} else if (phase == "surface-recovered") {
				const auto rows = CaptureSpaceDescents(); REQUIRE(rows.size() == 1); const auto job = rows.front(); REQUIRE(job.remaining_ticks == 111);
				const auto stock = center->goods[cargo].TotalCount(); const auto pods = SpacePod::GetNumItems();
				/** @return Whether normal native descent work transferred the original packets and expended the exact paid shell. */
				world.Await([&] { return CaptureSpaceDescents().empty(); }); CHECK(center->goods[cargo].TotalCount() == stock + 40); CHECK(ResolveSpacePod(job.pod) == nullptr); CHECK(SpacePod::GetNumItems() == pods - 1);
				CHECK(heavy->manifest->pending_returns == 0); CHECK(heavy->manifest->returns_remaining == 1); world.Advance(10); CHECK(center->goods[cargo].TotalCount() == stock + 40); CHECK(CaptureSpaceDescents().empty());
			} else if (phase == "orbital-blocked") {
				REQUIRE(heavy->manifest->pods.empty()); REQUIRE(heavy->manifest->returns_remaining == 1); world.Order(*builder, *world.core, SpaceOrderAction::Visit);
				/** @return Whether the original serviced builder physically occupies the real precise Core berth. */
				world.Await([&] { return GetSpaceStructureBerth(*builder, *world.core).has_value(); }); const auto stock = world.core->module->stock[cargo].TotalCount();
				MineHeavyCycle(world, *heavy, SpaceOrderTargetKind::Structure, world.core->index.base(), world.core->generation);
				/** @return Whether the actual whole-hold return is blocked by the original physical Core occupant. */
				world.Await([&] { return heavy->space_orders.blocked == SpaceOrderBlock::ApproachBusy; }, 12000);
				CHECK(heavy->cargo.TotalCount() == 40); CHECK(heavy->manifest->returns_remaining == 1); CHECK(world.core->module->stock[cargo].TotalCount() == stock); CHECK(heavy->extracted_units == 160);
			} else if (phase == "orbital-returned") {
				REQUIRE(heavy->cargo.TotalCount() == 40); REQUIRE(heavy->space_orders.blocked == SpaceOrderBlock::ApproachBusy); const auto stock = world.core->module->stock[cargo].TotalCount();
				world.Order(*builder, *dock, SpaceOrderAction::Visit);
				/** @return Whether actual departure admitted the whole podless Core unload through its own handling owner. */
				world.Await([&] { return heavy->space_orders.cursor == heavy->space_orders.entries.size(); }, 12000);
				CHECK(heavy->cargo.TotalCount() == 0); CHECK(heavy->manifest->returns_remaining == 0); CHECK(world.core->module->stock[cargo].TotalCount() == stock + 40);
				world.Advance(10); CHECK(world.core->module->stock[cargo].TotalCount() == stock + 40); CHECK(heavy->manifest->services_completed == 1); CHECK(dock->module->services_completed == 2);
			} else {
				REQUIRE(phase == "inspect");
			}
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(GetNextSpacePodGeneration() == pod_generation);
			CHECK(small->construction.placement == SpacePlacement::SurfaceStored); CHECK(small->manifest->pods.size() == 2); CHECK(small->manifest->returns_remaining == 2); CHECK(small->manifest->services_completed == 0);
			// Every original loose bill was already reserved before this consumer; work consumes those same contained packets, never refilling another hull or the original Core.
			for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) {
				CHECK(heavy->manifest->supplies[supply].TotalCount() == 0); CHECK(builder->manifest->supplies[supply].TotalCount() == 0); CHECK(small->manifest->supplies[supply].TotalCount() == 1);
			}
			// Each active berth retains its complete immutable role bill through pause/resume; canonical empty jobs have no residual timer or reusable parts after settlement.
			for (const auto *job : dock->module->GetServiceJobs()) {
				const auto tuning = GetOrbitalServiceJobTuning(*job); REQUIRE(tuning);
				for (uint i = 0; i < 2; ++i) CHECK(job->inputs[i].TotalCount() == (job->remaining_ticks == 0 ? 0 : tuning->inputs[i]));
				if (job->remaining_ticks == 0) CHECK(job->model == UINT16_MAX);
			}
			world.Save(fmt::format("heavy-cycle-{}-437.sav", phase));
			fmt::print("HEAVY_CYCLE native=true phase={} extracted={} held={} returns={} pending={} services={} source_use={}\n", phase, heavy->extracted_units, heavy->cargo.TotalCount(), heavy->manifest->returns_remaining, heavy->manifest->pending_returns, heavy->manifest->services_completed, used()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(heavy_cycle_observer, &observe); IConsole::CmdRegister("heavy_cycle_test", ObserveHeavyCycle); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
