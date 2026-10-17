/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file legacy_descent_native.cpp Genuine433 small-miner descent input before the later heavy sender-receipt extension.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBBA
 * Only starting-fuel declares missing one-trip Surface fixture propellant; all hulls, pods, parts, mining, flight and descent owners are original.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *legacy_descent_observer = nullptr; ///< Process-local observer; it owns no gameplay state.

/** @param args Parameter-free loaded-world console invocation. @return Whether the installed original-world observer ran. */
static bool ObserveLegacyDescent(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(legacy_descent_observer != nullptr); (*legacy_descent_observer)(); return true;
}

/** Capture one genuine433 paid small-miner milestone through original native commands, packets and common simulation ticks. */
TEST_CASE("Original small miner produces a genuine pre-heavy descent and recovers once", "[.legacy-descent-native]")
{
	const char *source = std::getenv("OTTD_LEGACY_DESCENT_SOURCE"), *selected = std::getenv("OTTD_LEGACY_DESCENT_PHASE"); REQUIRE(source != nullptr); REQUIRE(selected != nullptr);
	const std::string_view phase{selected}; const auto paths = GetTestCaseArtifactPaths("space", "legacy-descent-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve actual loaded-world assertion failures through ordinary application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths); auto &rocket = *world.rocket;
			SpaceVehicle *small = nullptr;
			// Select the exact original native pool identity without guessing its incarnation or allocating a substitute hull.
			for (auto *candidate : SpaceVehicle::Iterate()) if (candidate->index == VehicleID{16}) small = candidate;
			auto *heavy = ResolveSpaceCraft({VehicleID{22}, 8}); REQUIRE(small != nullptr); REQUIRE(heavy != nullptr); REQUIRE(rocket.index == VehicleID{15}); REQUIRE(rocket.transition != nullptr);
			REQUIRE(GetSpaceVehicleModel(*small) == 15); REQUIRE(small->manifest != nullptr); REQUIRE(heavy->manifest != nullptr); REQUIRE(rocket.manifest != nullptr);
			const auto ref = GetSpaceCraftReference(*small); auto &manifest = *small->manifest; const auto heavy_pods = heavy->manifest->pods; const auto fleet = GetSpaceVehicleCount();
			const auto next = GetNextSpaceVehicleGeneration(), pod_next = GetNextSpacePodGeneration(); const auto shells = SpacePod::GetNumItems(); const auto heavy_pose = heavy->position;
			const auto home = rocket.construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr); const auto cargo = GetCargoTypeByLabel(CargoLabel{"NORE"}); REQUIRE(IsValidCargoType(cargo));
			if (phase == "starting-fuel") {
				REQUIRE(small->construction.placement == SpacePlacement::SurfaceStored); REQUIRE(manifest.pods.size() == 2); REQUIRE(manifest.service_ticks == 0);
				auto &stock = center->goods[GetSpaceSupplyCargo(SpaceSupply::Propellant)].GetOrCreateData().cargo; const auto &budget = GetMultiLayerSpaceBudget();
				const uint required = uint{budget.outbound_fuel} + budget.recovery_fuel; REQUIRE(stock.AvailableCount() < required); const auto missing = static_cast<uint16_t>(required - stock.AvailableCount());
				// The retained transport fixture exhausted its declared Earth fuel. Separately declare only one trip's missing Surface input; this grants no paid asset, orbital stock or procurement acceptance.
				REQUIRE(CargoPacket::CanAllocateItem()); stock.Append(CargoPacket::Create(center->index, missing, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
				CHECK(stock.AvailableCount() == required); fmt::print("LEGACY_DESCENT starting_fuel_fixture=true added={} procurement_acceptance=false\n", missing);
			} else if (phase == "deployed") {
				const auto anchor = *GetLaunchCenterPlaneReference(center->launch_center->tile); const AsteroidBody *target = nullptr;
				// Select an actual distant metallic body with enough finite reserve for the complete small hold; its original identity and moving coordinates remain authoritative.
				for (const auto &body : GetAsteroidField().bodies) {
					if (body.resource_class != AsteroidClass::Metallic || body.remaining_reserve < 20) continue;
					if (target == nullptr || SpaceDistanceSquared(anchor, body.motion.position) > SpaceDistanceSquared(anchor, target->motion.position)) target = &body;
				}
				REQUIRE(target != nullptr); const auto body = target->id; const auto completed = rocket.transition->missions_completed;
				REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket.index, rocket.generation, small->index, small->generation, false));
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket.index, rocket.generation, uint8_t{0}, uint16_t{15}, false));
				const auto money = Company::Get(_current_company)->money;
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket.index, rocket.generation, SpaceOrderTargetKind::Body, body.base(), uint64_t{0}));
				CHECK(Company::Get(_current_company)->money == money - GetMultiLayerSpaceBudget().mission_fee);
				/** @return Whether the same actual rocket recovered after deploying the original paid small hull. */
				world.Await([&] { return rocket.transition->missions_completed == completed + 1; }); REQUIRE(small->construction.placement == SpacePlacement::Orbital);
				REQUIRE(small->space_orders.entries.empty());
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(small->index, small->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, body.base(), uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(small->index, small->generation, SpaceOrderEdit::Insert, uint8_t{1}, SpaceOrderAction::Unload, SpaceOrderTargetKind::LaunchCenter, uint32_t{home.station.base()}, home.generation));
				CHECK(manifest.pods.size() == 2); CHECK(manifest.returns_remaining == 2); CHECK(small->cargo.TotalCount() == 0);
			} else if (phase == "returning") {
				REQUIRE(small->space_orders.entries.size() == 2); const auto body = OrbitalBodyID{small->space_orders.entries[0].target.id}; const auto reserve = GetMutableAsteroidBody(body)->remaining_reserve;
				/** @return Whether normal finite extraction filled the original20t hold and its own Mine action released its exclusive lease for Earth return. */
				world.Await([&] { return small->cargo.TotalCount() == 20 && small->space_orders.cursor == 1; }, 12000);
				CHECK(small->extracted_units == 20); CHECK(GetMutableAsteroidBody(body)->remaining_reserve == reserve - 20); CHECK(GetMutableAsteroidBody(body)->mining_slot == VehicleID::Invalid());
				CHECK(manifest.pending_returns == 0); CHECK(manifest.returns_remaining == 2); CHECK(manifest.pods.size() == 2);
			} else if (phase == "partial") {
				/** @return Whether the actual typed Earth Unload handed the entire hold to a paid original shell's native descent. */
				world.Await([&] { return !CaptureSpaceDescents().empty(); }, 12000); auto records = CaptureSpaceDescents(); REQUIRE(records.size() == 1);
				auto *pod = ResolveSpacePod(records.front().pod); REQUIRE(pod != nullptr); REQUIRE(pod->descent != nullptr); REQUIRE(pod->descent->remaining_ticks >= 111);
				world.Advance(pod->descent->remaining_ticks - 111); CHECK(pod->descent->remaining_ticks == 111); CHECK(pod->descent->quantity == 20); CHECK(pod->descent->packets.TotalCount() == 20);
				CHECK(pod->descent->sender == ref.vehicle); CHECK(pod->descent->sender_generation == ref.generation); CHECK(small->cargo.TotalCount() == 0);
				CHECK(manifest.pending_returns == 1); CHECK(manifest.returns_remaining == 2); CHECK(manifest.pods.size() == 1);
			} else if (phase == "recovered") {
				const auto rows = CaptureSpaceDescents(); REQUIRE(rows.size() == 1); const auto pod = rows.front().pod; const auto stock = center->goods[cargo].TotalCount(); const auto extracted = small->extracted_units;
				/** @return Whether original descent recovery transferred the same packets once and expended the same original paid shell. */
				world.Await([&] { return CaptureSpaceDescents().empty(); }); CHECK(ResolveSpacePod(pod) == nullptr); CHECK(SpacePod::GetNumItems() == shells - 1);
				CHECK(center->goods[cargo].TotalCount() == stock + 20); CHECK(small->extracted_units == extracted); CHECK(manifest.pending_returns == 0); CHECK(manifest.returns_remaining == 1);
				world.Advance(10); CHECK(center->goods[cargo].TotalCount() == stock + 20); CHECK(manifest.returns_remaining == 1); CHECK(manifest.pods.size() == 1);
			} else {
				REQUIRE(phase == "resave"); // Reconstruct only original native ownership; this observer earns no progress or free work on load.
			}
			CHECK(GetSpaceCraftReference(*small) == ref); CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == next); CHECK(GetNextSpacePodGeneration() == pod_next);
			CHECK(heavy->position == heavy_pose); CHECK(heavy->extracted_units == 40); CHECK(heavy->cargo.TotalCount() == 0); CHECK(heavy->manifest->pods == heavy_pods);
			CHECK(heavy->manifest->returns_remaining == 1); CHECK(heavy->manifest->pending_returns == 0); CHECK(heavy->manifest->service_ticks == 0); CHECK(heavy->manifest->services_completed == 0);
			CHECK(heavy->manifest->supplies[1].TotalCount() == 2); CHECK(heavy->manifest->supplies[2].TotalCount() == 2);
			world.Save(fmt::format("legacy-descent-{}-433.sav", phase)); fmt::print("LEGACY_DESCENT native=true phase={} vehicle={} generation={} held={} extracted={} pending={} returns={}\n", phase,
				small->index.base(), small->generation, small->cargo.TotalCount(), small->extracted_units, manifest.pending_returns, manifest.returns_remaining); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(legacy_descent_observer, &observe); IConsole::CmdRegister("legacy_descent_test", ObserveLegacyDescent); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
