/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_contained_service_native.cpp Actual paid Core heavy work, retained loss/cancellation and genuine435 unknown-role service upgrades.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC
 * All hulls, panels, parts and original allowance come from frozen paid predecessors. Sender destruction is an explicit native casualty fixture; its actual remaining pod drifts without compensation.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../orbital_service.h"
#include "../space_pod.h"
#include "../space_order_actions.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_contained_observer = nullptr; ///< Sole invocation-owned callback; original native owners retain every gameplay obligation.

/** @param args Parameter-free actual loaded-world callback. @return Whether the installed paid-world observer ran. */
static bool ObserveHeavyContainedService(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_contained_observer != nullptr); (*heavy_contained_observer)(); return true;
}

/** @return Complete original physical/work digests used to reject query or copied-preview mutation of the open valid world. */
static auto HeavyContainedAuthority()
{
	return std::array<uint64_t, 11>{ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest(), ComputeUtilityPowerDigest(), ComputeSpaceManifestDigest(),
		ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(), ComputeSpaceOrdersDigest(), ComputeAsteroidFieldDigest(GetAsteroidField()),
		ComputeLaunchProductionDigest(), ComputeLaunchCenterDigest(), ComputeSpaceDescentDigest()};
}

/** Native commands/shared opportunities own the complete heavy bill and preserve actual older contained work without inferring its absent role. */
TEST_CASE("Actual paid Core services a heavy miner and preserves genuine unknown-role work", "[.heavy-contained-service-native]")
{
	const char *source = std::getenv("OTTD_HEAVY_CONTAINED_SOURCE"), *selected = std::getenv("OTTD_HEAVY_CONTAINED_PHASE"); REQUIRE(source != nullptr); REQUIRE(selected != nullptr);
	const std::string_view phase{selected}; const auto paths = GetTestCaseArtifactPaths("space", "heavy-contained-service-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve actual loaded-world assertions through normal application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths); auto &module = *world.core->module; auto &job = module.service;
			const SpaceCraftRef original{VehicleID{22}, 8}; auto *heavy = ResolveSpaceCraft(original); const auto fleet = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration();
			/** @return Actual original Core panel settlement, never a private copied energy account. */
			auto used = [&] { return GetUtilityPowerProducer(module.producer)->consumed_lifetime_j; };
			/** @param control Original operation on the existing job or the original heavy's fresh berth. */
			auto control = [&](OrbitalServiceControl control) {
				const auto ref = job.remaining_ticks != 0 ? job.craft : original;
				REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(world.core->index, world.core->generation, ref.vehicle, ref.generation, control));
			};
			const bool legacy = phase == "legacy-upgrade" || phase == "legacy-complete";
			// A detached preview/plain reconstruction may open the deliberately lost-sender checkpoint; neither can demand or create a replacement paid hull.
			const bool orphan = phase == "orphan-cancelled" || ((phase == "corrupt" || phase == "resave") && heavy == nullptr);
			// The orphan branch deliberately retains a lost original sender; every other entry begins with the actual paid heavy hull rather than a fixture replacement.
			if (!orphan) { REQUIRE(heavy != nullptr); REQUIRE(heavy->manifest != nullptr); REQUIRE(GetSpaceVehicleModel(*heavy) == 21); }
			if (const char *corrupt = std::getenv("OTTD_HEAVY_CONTAINED_CORRUPT")) {
				const auto authority = HeavyContainedAuthority(); const auto money = Company::Get(_current_company)->money; const auto random = _random;
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(HeavyContainedAuthority() == authority); CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("HEAVY_CONTAINED preview_reject=true authority_unchanged=true\n");
			} else if (phase == "premature") {
				REQUIRE(heavy->manifest->returns_remaining == 1); REQUIRE(GetSpaceStructureBerth(*heavy, *world.core)); REQUIRE(module.stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount() == 3); REQUIRE(module.stock[GetSpaceSupplyCargo(SpaceSupply::Electronics)].TotalCount() == 3);
				const auto authority = HeavyContainedAuthority(); const auto money = Company::Get(_current_company)->money;
				CHECK(CmdServiceOrbitalProbe({}, world.core->index, world.core->generation, heavy->index, heavy->generation, OrbitalServiceControl::Start).Failed());
				CHECK(CmdServiceOrbitalProbe(DoCommandFlag::Execute, world.core->index, world.core->generation, heavy->index, heavy->generation, OrbitalServiceControl::Start).Failed());
				CHECK(HeavyContainedAuthority() == authority); CHECK(Company::Get(_current_company)->money == money);
			} else if (phase == "prepared" || phase == "pretopup") {
				// Independent exhausted/partly used entry worlds perform the same genuine travel and fully funded Core transfer; only exhausted allowance may subsequently start heavy work.
				REQUIRE(job.remaining_ticks == 0); REQUIRE(heavy->manifest->returns_remaining == (phase == "pretopup" ? 1 : 0)); REQUIRE(heavy->manifest->services_completed == 0);
				// Original real occupants depart by native Visit only. Shared approach admission, actual travel and whole part exchange must precede heavy service; no pose, reservation or inventory is assigned here.
				for (auto *occupant : SpaceVehicle::Iterate()) {
					if (occupant == heavy || occupant->owner != heavy->owner || occupant->construction.placement != SpacePlacement::Orbital || occupant->position != GetSpaceStructureApproach(world.core->state)) continue;
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(occupant->index, occupant->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(occupant->index, occupant->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				}
				world.Order(*heavy, *world.core, SpaceOrderAction::Visit);
				/** @return Whether the actual original heavy reached its admitted precise shared Core berth. */
				world.Await([&] { return GetSpaceStructureBerth(*heavy, *world.core).has_value(); });
				for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveOrbitalSupply>::Post(world.core->index, world.core->generation, heavy->index, heavy->generation, supply, uint16_t{2}, false));
				if (phase == "prepared") {
					const auto authority = HeavyContainedAuthority(); const auto money = Company::Get(_current_company)->money;
					CHECK(CmdServiceOrbitalProbe({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, world.core->index, world.core->generation, heavy->index, heavy->generation, OrbitalServiceControl::Start).Succeeded());
					CHECK(HeavyContainedAuthority() == authority); CHECK(Company::Get(_current_company)->money == money); control(OrbitalServiceControl::Start);
					CHECK(job.remaining_ticks == 296); CHECK(job.model == 21); CHECK(job.last_work_interval == 0); CHECK(job.services_at_start == 0);
				}
			} else if (phase == "partial") {
				REQUIRE(job.remaining_ticks == 296); const auto before = used(); const auto profit = heavy->profit_this_year; world.Advance(37);
				CHECK(job.remaining_ticks == 259); CHECK(used() == before + 27750); CHECK(heavy->profit_this_year == profit - Money{500} * 256);
			} else if (phase == "paused") {
				REQUIRE(job.remaining_ticks == 259); control(OrbitalServiceControl::Pause); const auto before = used(); world.Advance(9); CHECK(job.remaining_ticks == 259); CHECK(job.paused); CHECK(used() == before);
			} else if (phase == "resumed") {
				REQUIRE(job.remaining_ticks == 259); REQUIRE(job.paused); const auto authority = HeavyContainedAuthority();
				CHECK(CmdServiceOrbitalProbe({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, world.core->index, world.core->generation, heavy->index, heavy->generation, OrbitalServiceControl::Resume).Succeeded()); CHECK(HeavyContainedAuthority() == authority);
				control(OrbitalServiceControl::Resume); const auto before = used(); world.Advance(17); CHECK(job.remaining_ticks == 242); CHECK(used() == before + 12750);
			} else if (phase == "last-tick") {
				REQUIRE(job.remaining_ticks == 242); const auto before = used(); world.Advance(241); CHECK(job.remaining_ticks == 1); CHECK(used() == before + 180750);
			} else if (phase == "complete") {
				REQUIRE(job.remaining_ticks == 1); const auto before = used(), history = module.services_completed; world.Advance(1);
				CHECK(used() == before + 750); CHECK(job.remaining_ticks == 0); CHECK(job.model == UINT16_MAX); CHECK(heavy->manifest->services_completed == 1); CHECK(heavy->manifest->returns_remaining == 2); CHECK(module.services_completed == history + 1);
				world.Advance(10); CHECK(used() == before + 750); CHECK(heavy->manifest->services_completed == 1); CHECK(module.services_completed == history + 1);
			} else if (phase == "outage") {
				REQUIRE(job.remaining_ticks == 259); const auto before = used(); REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(world.core->index, world.core->generation, true)); world.Advance(9); CHECK(job.remaining_ticks == 259); CHECK(used() == before); CHECK_FALSE(job.paused);
			} else if (phase == "restored") {
				REQUIRE(job.remaining_ticks == 259); REQUIRE_FALSE(module.power_enabled); const auto before = used(); REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(world.core->index, world.core->generation, false)); world.Advance(17); CHECK(job.remaining_ticks == 242); CHECK(used() == before + 12750);
			} else if (phase == "cancelled" || phase == "orphan-cancelled") {
				// Explicit cancellation returns the same complete reserved bill, including after the original sender disappears, and refunds no spent electricity or allowance.
				REQUIRE(job.remaining_ticks == 259); REQUIRE(job.model == 21); const auto before = used(); control(OrbitalServiceControl::Cancel); CHECK(job.remaining_ticks == 0); CHECK(job.model == UINT16_MAX); world.Advance(9); CHECK(used() == before);
				CHECK(module.stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount() == 3); CHECK(module.stock[GetSpaceSupplyCargo(SpaceSupply::Electronics)].TotalCount() == 3);
			} else if (phase == "restarted") {
				REQUIRE(job.remaining_ticks == 0); REQUIRE(heavy->manifest->returns_remaining == 0); const auto before = used(); control(OrbitalServiceControl::Start); CHECK(job.model == 21); CHECK(job.remaining_ticks == 296); CHECK(job.last_work_interval == 0); CHECK(used() == before);
			} else if (phase == "sender-lost") {
				REQUIRE(job.remaining_ticks == 259); REQUIRE(job.model == 21); REQUIRE(heavy->manifest->pods.size() == 1); const auto pod = heavy->manifest->pods.front(); const auto before = used();
				// Explicit native destruction fixture: the retained service owns both whole reserved bills; the original hull's remaining paid shell drifts at its actual pose with no replacement, cargo grant or refund.
				delete heavy; heavy = nullptr; CHECK(ResolveSpaceCraft(original) == nullptr); CHECK(GetSpaceVehicleCount() == fleet - 1); world.Advance(9);
				CHECK(job.remaining_ticks == 259); CHECK(job.model == 21); CHECK(used() == before); REQUIRE(ResolveSpacePod(pod) != nullptr); CHECK(ResolveSpacePod(pod)->state.custody == SpacePodCustody::Drifting);
			} else if (legacy) {
				auto *craft = ResolveSpaceCraft({VehicleID{18}, 4}); REQUIRE(craft != nullptr); REQUIRE(craft->manifest != nullptr); REQUIRE(GetSpaceVehicleModel(*craft) == 20);
				REQUIRE(job.craft == GetSpaceCraftReference(*craft)); REQUIRE(job.model == UINT16_MAX); REQUIRE(job.remaining_ticks == 111); REQUIRE(job.services_at_start == 0); REQUIRE(job.last_work_interval == 22047); REQUIRE_FALSE(job.paused);
				if (phase == "legacy-complete") {
					const auto before = used(), history = module.services_completed; world.Advance(111); CHECK(used() == before + 83250); CHECK(job.remaining_ticks == 0); CHECK(job.model == UINT16_MAX); CHECK(craft->manifest->services_completed == 1); CHECK(craft->manifest->returns_remaining == 2); CHECK(module.services_completed == history + 1);
					world.Advance(10); CHECK(used() == before + 83250); CHECK(craft->manifest->services_completed == 1);
				}
			} else {
				REQUIRE(phase == "resave"); // Ordinary reconstruction retains its original role, packets, earned interval and history without another service event.
			}
			CHECK(GetNextSpaceVehicleGeneration() == generation);
			if (phase != "sender-lost") CHECK(GetSpaceVehicleCount() == fleet);
			// Every retained known heavy job still owns both complete physical bills, even when no surviving hull can perform work. Genuine unknown legacy jobs retain their original one-part bound.
			if (job.remaining_ticks != 0) { const auto tuning = GetOrbitalServiceJobTuning(job); REQUIRE(tuning); CHECK(job.inputs[0].TotalCount() == tuning->inputs[0]); CHECK(job.inputs[1].TotalCount() == tuning->inputs[1]); }
			if (heavy != nullptr) {
				CHECK(heavy->cargo.TotalCount() == 0); CHECK(heavy->construction.home == LaunchCenterRef{}); CHECK(heavy->construction.surface_tile == INVALID_TILE);
				// A preview preserves whichever original branch it opens; operation-specific expected quantities apply only to actual named mutations, never a generic reconstruction or detached check.
				if (phase != "resave" && std::getenv("OTTD_HEAVY_CONTAINED_CORRUPT") == nullptr) {
					const bool complete = phase == "complete", premature = phase == "premature" || phase == "pretopup";
					CHECK(heavy->manifest->services_completed == (complete ? 1 : 0)); CHECK(heavy->manifest->returns_remaining == (complete ? 2 : premature ? 1 : 0)); CHECK(heavy->manifest->pending_returns == 0);
					CHECK(heavy->manifest->supplies[1].TotalCount() == (legacy ? 2 : 0)); CHECK(heavy->manifest->supplies[2].TotalCount() == (legacy ? 2 : 0));
					CHECK(heavy->manifest->pods.size() == (premature ? 2 : 1)); CHECK(heavy->extracted_units == (premature ? 40 : 80));
				}
			}
			if (std::getenv("OTTD_HEAVY_CONTAINED_CORRUPT") == nullptr) world.Save(fmt::format("heavy-contained-{}-436.sav", phase));
			fmt::print("HEAVY_CONTAINED native=true phase={} work={} model={} source_use={}\n", phase, job.remaining_ticks, job.model, used()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(heavy_contained_observer, &observe); IConsole::CmdRegister("heavy_contained_test", ObserveHeavyContainedService); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
