/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_descent_native.cpp Actual heavy40t release, immutable sender loss, independent preview and genuine433 descent continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB
 * Inputs are immutable paid producer saves. Sender loss is an explicit native destruction fixture, including lost onboard parts and the remaining drifting shell.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../launch_operations.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_descent_observer = nullptr; ///< Process-local observer; every gameplay owner remains native.

/** @param args Parameter-free loaded-world console callback. @return Whether the installed original-world observer executed. */
static bool ObserveHeavyDescent(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_descent_observer != nullptr); (*heavy_descent_observer)(); return true;
}

/** @return Exact original native physical/work/receipt digests used to prove queries and copied checks preserve the active world. */
static auto DescentAuthority()
{
	return std::array<uint64_t, 11>{ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest(), ComputeUtilityPowerDigest(), ComputeSpaceManifestDigest(),
		ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(), ComputeSpaceOrdersDigest(), ComputeAsteroidFieldDigest(GetAsteroidField()),
		ComputeLaunchProductionDigest(), ComputeLaunchCenterDigest(), ComputeSpaceDescentDigest()};
}

/** Execute one bounded actual current descent milestone or a genuine old producer's upgrade/recovery, with no cargo/pose/work/allowance grant. */
TEST_CASE("Actual heavy descent retains physical mass and genuine older recovery", "[.heavy-descent-native]")
{
	const char *source = std::getenv("OTTD_HEAVY_DESCENT_SOURCE"), *selected = std::getenv("OTTD_HEAVY_DESCENT_PHASE"); REQUIRE(source != nullptr); REQUIRE(selected != nullptr);
	const std::string_view phase{selected}; const auto paths = GetTestCaseArtifactPaths("space", "heavy-descent-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve actual loaded-world assertion failures through ordinary application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths); const SpaceCraftRef original{VehicleID{22}, 8}; auto *craft = ResolveSpaceCraft(original);
			const auto fleet = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration(), pod_generation = GetNextSpacePodGeneration(); const auto shells = SpacePod::GetNumItems();
			auto *center = ResolveLaunchCenter(world.rocket->construction.home); REQUIRE(center != nullptr); const auto home = GetLaunchCenterReference(*center); const auto cargo = GetCargoTypeByLabel(CargoLabel{"NORE"}); REQUIRE(IsValidCargoType(cargo));
			if (const char *corrupt = std::getenv("OTTD_HEAVY_DESCENT_CORRUPT")) {
				const auto authority = DescentAuthority(); const auto money = Company::Get(_current_company)->money; const auto random = _random;
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(DescentAuthority() == authority); CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("HEAVY_DESCENT preview_reject=true authority_unchanged=true\n");
			} else if (phase == "returning") {
				REQUIRE(craft != nullptr); REQUIRE(craft->manifest != nullptr); REQUIRE(craft->manifest->pods.size() == 2); REQUIRE(craft->cargo.TotalCount() == 0); REQUIRE(craft->extracted_units == 40);
				const auto anchor = *GetLaunchCenterPlaneReference(center->launch_center->tile); const AsteroidBody *target = nullptr;
				// A real distant metallic body supplies finite cargo and a real out-of-circle return leg; no fixture coordinate or resource debit substitutes extraction.
				for (const auto &body : GetAsteroidField().bodies) {
					if (body.resource_class != AsteroidClass::Metallic || body.remaining_reserve < 40) continue;
					if (target == nullptr || SpaceDistanceSquared(anchor, body.motion.position) > SpaceDistanceSquared(anchor, target->motion.position)) target = &body;
				}
				REQUIRE(target != nullptr); const auto body = target->id; const auto reserve = target->remaining_reserve;
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, body.base(), uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Insert, uint8_t{1}, SpaceOrderAction::Unload, SpaceOrderTargetKind::LaunchCenter, uint32_t{home.station.base()}, home.generation));
				/** @return Whether normal per-model extraction filled the same original heavy hold and released the actual mining lease for Earth return. */
				world.Await([&] { return craft->cargo.TotalCount() == 40 && craft->space_orders.cursor == 1; }, 12000);
				CHECK(craft->extracted_units == 80); CHECK(GetMutableAsteroidBody(body)->remaining_reserve == reserve - 40); CHECK(GetMutableAsteroidBody(body)->mining_slot == VehicleID::Invalid());
				const auto authority = DescentAuthority(); const auto pod = craft->manifest->pods.front();
				CHECK(CmdReleaseOrbitalCargo({}, craft->index, craft->generation, home.station, home.generation, pod.pod, pod.generation, 41).Failed());
				CHECK(CmdReleaseOrbitalCargo({}, craft->index, craft->generation + 1, home.station, home.generation, pod.pod, pod.generation, 40).Failed());
				CHECK(DescentAuthority() == authority); CHECK(craft->manifest->pending_returns == 0); CHECK(craft->manifest->returns_remaining == 1);
			} else if (phase == "partial") {
				REQUIRE(craft != nullptr); REQUIRE(craft->manifest != nullptr);
				// A normal Visit supplies real in-circle geometry without handling cargo. A successful mixed query must then retain the actual40t hold, original shell and pending allowance before the explicit Earth Unload runs.
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::LaunchCenter, uint32_t{home.station.base()}, home.generation));
				const auto anchor = *GetLaunchCenterPlaneReference(center->launch_center->tile);
				/** @return Whether actual native travel reached the inclusive Surface release region, independently of an orbital module berth. */
				world.Await([&] { return IsInsideLaunchCenterDropZone(craft->position, anchor); }, 12000);
				const auto before_query = DescentAuthority(); const auto quoted_shell = craft->manifest->pods.front(); const auto quoted_money = Company::Get(_current_company)->money;
				CHECK(CmdReleaseOrbitalCargo({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, craft->index, craft->generation, home.station, home.generation, quoted_shell.pod, quoted_shell.generation, 40).Succeeded());
				CHECK(DescentAuthority() == before_query); CHECK(Company::Get(_current_company)->money == quoted_money); CHECK(craft->cargo.TotalCount() == 40); CHECK(CaptureSpaceDescents().empty());
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Unload, SpaceOrderTargetKind::LaunchCenter, uint32_t{home.station.base()}, home.generation));
				/** @return Whether the actual Earth order committed the whole heavy hold to its own paid shell. */
				world.Await([&] { return !CaptureSpaceDescents().empty(); }, 12000); const auto rows = CaptureSpaceDescents(); REQUIRE(rows.size() == 1);
				auto *pod = ResolveSpacePod(rows.front().pod); REQUIRE(pod != nullptr); REQUIRE(pod->descent != nullptr); REQUIRE(pod->descent->remaining_ticks >= 111);
				world.Advance(pod->descent->remaining_ticks - 111); const auto &job = *pod->descent;
				CHECK(job.quantity == 40); CHECK(job.remaining_ticks == 111); CHECK(job.packets.TotalCount() == 40); CHECK(job.sender_model == 21); CHECK(job.unit_weight == 16); CHECK(job.freight_mass == 640);
				CHECK(craft->cargo.TotalCount() == 0); CHECK(craft->manifest->pending_returns == 1); CHECK(craft->manifest->returns_remaining == 1); CHECK(craft->manifest->pods.size() == 1);
				const auto authority = DescentAuthority(); const auto money = Company::Get(_current_company)->money;
				CHECK(CmdRetargetSpaceDescent({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, pod->index, pod->generation, home.station, home.generation).Succeeded());
				CHECK(DescentAuthority() == authority); CHECK(Company::Get(_current_company)->money == money);
			} else if (phase == "sender-lost") {
				REQUIRE(craft != nullptr); REQUIRE(craft->manifest != nullptr); REQUIRE(craft->manifest->pods.size() == 1); const auto drifting = craft->manifest->pods.front();
				const auto rows = CaptureSpaceDescents(); REQUIRE(rows.size() == 1); REQUIRE(rows.front().sender_model == 21); const auto receipt = rows.front();
				// Explicit native destruction fixture: the lost hull's actual onboard2RFMT/2ELEC are destroyed by their sole manifest owner; its remaining shell drifts at the actual pose. The separate paid descent/40t owner must remain intact without a replacement body, refund or invented home.
				delete craft; craft = nullptr; CHECK(ResolveSpaceCraft(original) == nullptr); CHECK(GetSpaceVehicleCount() == fleet - 1);
				const auto after = CaptureSpaceDescents(); REQUIRE(after.size() == 1); CHECK(after.front().packets == receipt.packets); CHECK(after.front().sender_model == 21); CHECK(after.front().freight_mass == 640);
				const auto *retained = ResolveSpacePod(drifting); REQUIRE(retained != nullptr); CHECK(retained->state.custody == SpacePodCustody::Drifting); CHECK(retained->state.surface_tile != INVALID_TILE);
			} else if (phase == "legacy-upgrade") {
				const auto rows = CaptureSpaceDescents(); REQUIRE(rows.size() == 1); const auto &row = rows.front();
				CHECK(row.sender == VehicleID{16}); CHECK(row.sender_generation == 2); CHECK(row.quantity == 20); CHECK(row.remaining_ticks == 111);
				CHECK(row.sender_model == SPACE_DESCENT_LEGACY_SENDER_MODEL); CHECK(row.unit_weight == 0); CHECK(row.freight_mass == 0);
			} else if (phase == "recovered" || phase == "orphan-recovered" || phase == "legacy-recovered") {
				// These branches share the actual once-only recovery owner; only the immutable producer role and whether the original sender survives differ.
				const auto rows = CaptureSpaceDescents(); REQUIRE(rows.size() == 1); const auto row = rows.front(); const auto stock = center->goods[row.cargo].TotalCount(); const auto pod = row.pod;
				/** @return Whether normal retained descent work has transferred its actual packets and expended its original shell once. */
				world.Await([&] { return CaptureSpaceDescents().empty(); }); CHECK(center->goods[row.cargo].TotalCount() == stock + row.quantity); CHECK(ResolveSpacePod(pod) == nullptr); CHECK(SpacePod::GetNumItems() == shells - 1);
				if (phase == "legacy-recovered") {
					auto *sender = ResolveSpaceCraft({VehicleID{16}, 2}); REQUIRE(sender != nullptr); REQUIRE(sender->manifest != nullptr); CHECK(sender->manifest->pending_returns == 0); CHECK(sender->manifest->returns_remaining == 1);
				} else if (phase == "orphan-recovered") {
					CHECK(craft == nullptr); CHECK(ResolveSpaceCraft(original) == nullptr); CHECK(row.sender_model == 21); CHECK(row.freight_mass == 640);
				} else {
					REQUIRE(craft != nullptr); CHECK(craft->manifest->pending_returns == 0); CHECK(craft->manifest->returns_remaining == 0); CHECK(craft->manifest->pods.size() == 1);
				}
				world.Advance(10); CHECK(center->goods[row.cargo].TotalCount() == stock + row.quantity); CHECK(CaptureSpaceDescents().empty());
			} else {
				REQUIRE(phase == "resave"); // Ordinary reconstruction grants no descent work, cargo, shell or allowance.
			}
			CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(GetNextSpacePodGeneration() == pod_generation);
			if (phase != "sender-lost") CHECK(GetSpaceVehicleCount() == fleet);
			if (craft != nullptr) { CHECK(GetSpaceCraftReference(*craft) == original); CHECK(craft->manifest->services_completed == 0); CHECK(craft->manifest->service_ticks == 0); CHECK(craft->manifest->supplies[1].TotalCount() == 2); CHECK(craft->manifest->supplies[2].TotalCount() == 2); }
			if (std::getenv("OTTD_HEAVY_DESCENT_CORRUPT") == nullptr) world.Save(fmt::format("heavy-descent-{}-434.sav", phase));
			fmt::print("HEAVY_DESCENT native=true phase={} sender_present={} descents={}\n", phase, craft != nullptr, CaptureSpaceDescents().size()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(heavy_descent_observer, &observe); IConsole::CmdRegister("heavy_descent_test", ObserveHeavyDescent); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
