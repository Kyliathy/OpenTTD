/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_onboard_service_native.cpp Actual original heavy service, saved interruptions and typed ReturnService using paid predecessor parts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CC
 * Every physical hull, part, pod and exhausted allowance comes from immutable paid producer saves; the observer supplies no stock, pose, work or quota.
 * The optional target-loss branch invokes native retirement/reinstallation on the same actual paid footprint and explicitly recovers the original Surface rocket; it claims no newly paid infrastructure.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../space_order_actions.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_onboard_observer = nullptr; ///< Non-owning process callback; actual native manifest/packet owners remain authoritative.

/** @param args Parameter-free loaded-world console callback. @return Whether the installed original-world observer ran. */
static bool ObserveHeavyOnboard(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_onboard_observer != nullptr); (*heavy_onboard_observer)(); return true;
}

/** @return Complete original physical/work digests used to prove successful mixed queries and detached checks cannot mutate the active world. */
static auto HeavyServiceAuthority()
{
	return std::array<uint64_t, 11>{ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest(), ComputeUtilityPowerDigest(), ComputeSpaceManifestDigest(),
		ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(), ComputeSpaceOrdersDigest(), ComputeAsteroidFieldDigest(GetAsteroidField()),
		ComputeLaunchProductionDigest(), ComputeLaunchCenterDigest(), ComputeSpaceDescentDigest()};
}

/** Execute one actual bounded service milestone, retaining original parts/quota/history across normal command and ordinary-load boundaries. */
TEST_CASE("Actual heavy onboard service earns and consumes its original complete paid bill", "[.heavy-onboard-service-native]")
{
	const char *source = std::getenv("OTTD_HEAVY_ONBOARD_SOURCE"), *selected = std::getenv("OTTD_HEAVY_ONBOARD_PHASE"); REQUIRE(source != nullptr); REQUIRE(selected != nullptr);
	const std::string_view phase{selected}; const auto paths = GetTestCaseArtifactPaths("space", "heavy-onboard-service-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve actual loaded-world assertion failures through normal application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths); const SpaceCraftRef original{VehicleID{22}, 8}; auto *craft = ResolveSpaceCraft(original);
			REQUIRE(craft != nullptr); REQUIRE(craft->manifest != nullptr); REQUIRE(GetSpaceVehicleModel(*craft) == 21); auto &manifest = *craft->manifest;
			const auto fleet = GetSpaceVehicleCount(); const auto pods = SpacePod::GetNumItems(); const auto generation = GetNextSpaceVehicleGeneration(), pod_generation = GetNextSpacePodGeneration();
			if (const char *corrupt = std::getenv("OTTD_HEAVY_ONBOARD_CORRUPT")) {
				const auto authority = HeavyServiceAuthority(); const auto money = Company::Get(_current_company)->money; const auto random = _random;
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(HeavyServiceAuthority() == authority); CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("HEAVY_ONBOARD preview_reject=true authority_unchanged=true\n");
			} else if (phase == "premature") {
				REQUIRE(manifest.returns_remaining == 1); REQUIRE(manifest.supplies[1].TotalCount() == 2); REQUIRE(manifest.supplies[2].TotalCount() == 2);
				const auto authority = HeavyServiceAuthority(); const auto money = Company::Get(_current_company)->money;
				CHECK(CmdServiceLaunchProbe({}, craft->index, craft->generation, false).Failed()); CHECK(CmdServiceLaunchProbe(DoCommandFlag::Execute, craft->index, craft->generation, false).Failed());
				CHECK(HeavyServiceAuthority() == authority); CHECK(Company::Get(_current_company)->money == money); CHECK(manifest.service_ticks == 0); CHECK(manifest.services_completed == 0);
			} else if (phase == "prepared") {
				REQUIRE(manifest.returns_remaining == 0); REQUIRE(manifest.pending_returns == 0); REQUIRE(manifest.service_ticks == 0);
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				const auto authority = HeavyServiceAuthority(); const auto money = Company::Get(_current_company)->money;
				CHECK(CmdServiceLaunchProbe({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, craft->index, craft->generation, false).Succeeded()); CHECK(HeavyServiceAuthority() == authority); CHECK(Company::Get(_current_company)->money == money);
				REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(craft->index, craft->generation, false)); CHECK(manifest.service_ticks == 296); CHECK(manifest.service_location == SpaceServiceLocation::Onboard);
			} else if (phase == "partial") {
				REQUIRE(manifest.service_ticks == 296); const auto profit = craft->profit_this_year; world.Advance(37); CHECK(manifest.service_ticks == 259);
				CHECK(craft->profit_this_year == profit - Money{500} * 256); //37of74 actual opportunities at the immutable1000/day active rate preserve the original fractional upkeep owner.
			} else if (phase == "paused") {
				REQUIRE(manifest.service_ticks == 259); REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(craft->index, craft->generation, true)); world.Advance(9);
				CHECK(manifest.service_ticks == 259); CHECK(manifest.service_paused);
			} else if (phase == "resumed") {
				REQUIRE(manifest.service_paused); REQUIRE(manifest.service_ticks == 259); const auto authority = HeavyServiceAuthority();
				CHECK(CmdServiceLaunchProbe({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, craft->index, craft->generation, false).Succeeded()); CHECK(HeavyServiceAuthority() == authority);
				REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(craft->index, craft->generation, false)); CHECK(manifest.service_ticks == 259); world.Advance(17); CHECK(manifest.service_ticks == 242); CHECK_FALSE(manifest.service_paused);
			} else if (phase == "last-tick") {
				REQUIRE(manifest.service_ticks == 242); world.Advance(241); CHECK(manifest.service_ticks == 1); const auto authority = HeavyServiceAuthority();
				CHECK(CmdServiceLaunchProbe({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, craft->index, craft->generation, true).Succeeded()); CHECK(HeavyServiceAuthority() == authority); CHECK_FALSE(manifest.service_paused);
			} else if (phase == "ordered") {
				REQUIRE(manifest.returns_remaining == 0); REQUIRE(manifest.service_ticks == 0); const auto home = world.rocket->construction.home; REQUIRE(ResolveLaunchCenter(home) != nullptr);
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::ReturnService, SpaceOrderTargetKind::LaunchCenter, uint32_t{home.station.base()}, home.generation));
				/** @return Whether real travel and the physically reached typed action started the existing paid onboard owner. */
				world.Await([&] { return manifest.service_ticks != 0; }); REQUIRE(manifest.service_ticks >= 259); world.Advance(manifest.service_ticks - 259);
				CHECK(manifest.service_ticks == 259); CHECK(IsSpaceOnboardServiceOrder(*craft)); CHECK(craft->space_orders.cursor == 0);
			} else if (phase == "target-lost") {
				REQUIRE(manifest.service_ticks == 259); REQUIRE(IsSpaceOnboardServiceOrder(*craft)); const auto old = world.rocket->construction.home;
				auto *center = ResolveLaunchCenter(old); REQUIRE(center != nullptr); const auto tile = center->launch_center->tile; const auto rotation = center->launch_center->rotation;
				// Explicit native retirement fixture on the same paid footprint: the old queue target must retain its own marked history, while a reused station incarnation cannot lend automatic authority. Actual Surface hull recovery uses its normal command, without moving its pose or issuing another body.
				ForgetLaunchCenter(*center); InstallNewLaunchCenter(*center, tile, rotation); const auto current = GetLaunchCenterReference(*center); CHECK(current != old);
				REQUIRE(Command<Commands::RecoverLaunchCraft>::Post(current.station, current.generation, world.rocket->index, world.rocket->generation));
				CHECK(manifest.service_ticks == 259); CHECK(craft->space_orders.blocked == SpaceOrderBlock::TargetRemoved); CHECK(IsSpaceOnboardServiceOrder(*craft));
				CHECK(craft->space_orders.entries[craft->space_orders.cursor].target.generation == old.generation);
			} else if (phase == "complete" || phase == "ordered-complete" || phase == "lost-complete") {
				// Both routes finish the same original manifest owner. The typed route advances only after actual completion, never from arrival, quotes or supply alone.
				REQUIRE(manifest.services_completed == 0); REQUIRE(manifest.service_ticks == (phase == "complete" ? 1 : 259));
				/** @return Whether296 original earned opportunities actually consumed both complete part bills and restored allowance once. */
				world.Await([&] { return manifest.services_completed == 1; }); CHECK(manifest.service_ticks == 0); CHECK(manifest.returns_remaining == 2); CHECK(manifest.pending_returns == 0);
				CHECK(manifest.supplies[1].TotalCount() == 0); CHECK(manifest.supplies[2].TotalCount() == 0); CHECK(manifest.service_location == SpaceServiceLocation::EarthWorkshop);
				world.Advance(10); CHECK(manifest.services_completed == 1); CHECK(manifest.returns_remaining == 2); CHECK(manifest.service_ticks == 0);
				if (phase == "ordered-complete") CHECK(craft->space_orders.cursor == craft->space_orders.entries.size());
			} else {
				REQUIRE(phase == "resave"); // Ordinary reconstruction grants no work, material, history or allowance.
			}
			CHECK(GetSpaceCraftReference(*craft) == original); CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(GetNextSpacePodGeneration() == pod_generation); CHECK(SpacePod::GetNumItems() == pods);
			CHECK(craft->cargo.TotalCount() == 0); CHECK(CaptureSpaceDescents().empty()); CHECK(craft->construction.home == LaunchCenterRef{}); CHECK(craft->construction.surface_tile == INVALID_TILE);
			if (std::getenv("OTTD_HEAVY_ONBOARD_CORRUPT") == nullptr) {
				// Manual, reached-order and explicitly lost-target work share the same complete bill and once-only native completion owner.
				const bool complete = phase == "complete" || phase == "ordered-complete" || phase == "lost-complete";
				// Operation-specific quantities prove real consumption; a plain reconstruction retains whichever actual partial/completed producer history its input already owns.
				if (phase != "resave") {
					CHECK(manifest.supplies[1].TotalCount() == (complete ? 0 : 2)); CHECK(manifest.supplies[2].TotalCount() == (complete ? 0 : 2));
					CHECK(manifest.services_completed == (complete ? 1 : 0)); CHECK(manifest.pods.size() == (phase == "premature" ? 2 : 1)); CHECK(craft->extracted_units == (phase == "premature" ? 40 : 80));
				}
				world.Save(fmt::format("heavy-onboard-{}-435.sav", phase));
			}
			fmt::print("HEAVY_ONBOARD native=true phase={} work={} services={} allowance={}\n", phase, manifest.service_ticks, manifest.services_completed, manifest.returns_remaining); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(heavy_onboard_observer, &observe); IConsole::CmdRegister("heavy_onboard_test", ObserveHeavyOnboard); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
