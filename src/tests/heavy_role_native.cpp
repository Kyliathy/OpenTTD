/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_role_native.cpp Original paid miners contend for one moving finite body and earn their own immutable work/upkeep.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9D
 * Genuine descent recovery precedes competition. Native commands/ticks alone change leases, positions, reserve, cargo or work; no test asset or allowance is granted.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../orbital_service.h"
#include "../space_mining_tuning.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "../timer/timer_game_economy.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_role_observer = nullptr; ///< Invocation callback only; original native owners retain every physical asset.

/** @param args Parameter-free callback in the actual loaded world. @return Whether the installed observer ran. */
static bool ObserveHeavyRole(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_role_observer != nullptr); (*heavy_role_observer)(); return true;
}

/** @return Original physical/work authority for mixed-query, refusal and copied-preview purity. */
static auto HeavyRoleAuthority()
{
	return std::array<uint64_t, 11>{ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest(), ComputeUtilityPowerDigest(), ComputeSpaceManifestDigest(),
		ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(), ComputeSpaceOrdersDigest(), ComputeAsteroidFieldDigest(GetAsteroidField()),
		ComputeLaunchProductionDigest(), ComputeLaunchCenterDigest(), ComputeSpaceDescentDigest()};
}

/** Verify the native per-hull financial owner across actual unpaused work or waiting opportunities.
 * @param world Original-world tick adapter; never a private work clock.
 * @param craft Actual paid hull whose expense ledger is observed.
 * @param ticks Exact common native opportunities to execute.
 * @param rate Immutable active or idle daily cost from this role's published contract.
 */
static void AdvanceHeavyRoleCost(AssemblyNativeWorld &world, SpaceVehicle &craft, uint ticks, uint16_t rate)
{
	const auto day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	const uint64_t accrued = craft.mission.cost_remainder + uint64_t{rate} * ticks;
	REQUIRE(ticks <= day); const auto profit = craft.profit_this_year; const auto year = TimerGameEconomy::year;
	world.Advance(ticks);
	CHECK(craft.mission.cost_remainder == accrued % day);
	// A bounded contract day may cross the actual economic year. Include this same hull's native previous-year carry after its normal reset; never override dates or borrow a company-wide expense total.
	Money observed = craft.profit_this_year;
	if (TimerGameEconomy::year != year) observed += craft.profit_last_year;
	CHECK(observed == profit - Money{static_cast<int64_t>(accrued / day)} * 256);
}

/** Two original paid miners retain exclusive physical lease, finite output, capacity, upkeep and unchanged legacy behavior. */
TEST_CASE("Actual heavy and small miners share one moving finite body without borrowing work", "[.heavy-role-native]")
{
	const char *source = std::getenv("OTTD_HEAVY_ROLE_SOURCE"), *selected = std::getenv("OTTD_HEAVY_ROLE_PHASE"); REQUIRE(source != nullptr); REQUIRE(selected != nullptr);
	const std::string_view phase{selected}; const auto paths = GetTestCaseArtifactPaths("space", "heavy-role-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve actual-world assertions through ordinary application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			auto *heavy = ResolveSpaceCraft({VehicleID{22}, 8}), *small = ResolveSpaceCraft({VehicleID{16}, 2}); REQUIRE(heavy != nullptr); REQUIRE(small != nullptr);
			REQUIRE(heavy->manifest != nullptr); REQUIRE(small->manifest != nullptr); REQUIRE(GetSpaceVehicleModel(*heavy) == 21); REQUIRE(GetSpaceVehicleModel(*small) == 15);
			const auto &tuning = *GetSpaceMiningTuning(21), &legacy = *GetSpaceMiningTuning(15);
			const auto fleet = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration(), pod_generation = GetNextSpacePodGeneration();
			auto *center = ResolveLaunchCenter(world.rocket->construction.home); REQUIRE(center != nullptr);
			if (const char *corrupt = std::getenv("OTTD_HEAVY_ROLE_CORRUPT")) {
				const auto authority = HeavyRoleAuthority(); const auto money = Company::Get(_current_company)->money; const auto random = _random;
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(HeavyRoleAuthority() == authority); CHECK(Company::Get(_current_company)->money == money);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]); fmt::print("HEAVY_ROLE preview_reject=true authority_unchanged=true\n");
			} else if (phase == "recovered") {
				const auto jobs = CaptureSpaceDescents(); REQUIRE(jobs.size() == 1); const auto &job = jobs.front();
				REQUIRE(job.sender == small->index); REQUIRE(job.sender_generation == small->generation); REQUIRE(job.sender_model == SPACE_DESCENT_LEGACY_SENDER_MODEL);
				REQUIRE(job.quantity == 20); REQUIRE(job.remaining_ticks == 111); const auto stock = center->goods[job.cargo].TotalCount(); const auto pods = SpacePod::GetNumItems();
				/** @return Whether the original pod-owned legacy descent completed through its actual native scheduler. */
				world.Await([&] { return CaptureSpaceDescents().empty(); });
				CHECK(center->goods[job.cargo].TotalCount() == stock + 20); CHECK(ResolveSpacePod(job.pod) == nullptr); CHECK(SpacePod::GetNumItems() == pods - 1);
				CHECK(small->manifest->pending_returns == 0); CHECK(small->manifest->returns_remaining == 1);
				AdvanceHeavyRoleCost(world, *heavy, 10, tuning.idle_per_day); CHECK(center->goods[job.cargo].TotalCount() == stock + 20); CHECK(CaptureSpaceDescents().empty());
			} else if (phase == "approach") {
				REQUIRE(CaptureSpaceDescents().empty()); REQUIRE(heavy->cargo.TotalCount() == 0); REQUIRE(small->cargo.TotalCount() == 0);
				const AsteroidBody *target = nullptr;
				// Actual available metallic geometry chooses the closest original finite body; no reserve, pose or lease is constructed to make this pursuit pass.
				for (const auto &body : GetAsteroidField().bodies) {
					if (body.resource_class != AsteroidClass::Metallic || body.remaining_reserve < 40 || body.mining_slot != VehicleID::Invalid() || body.survey_slot != INVALID_SPACE_SURVEY) continue;
					// This pursuit must exercise an actual moving source; static bodies cannot establish attachment tracking under native drift.
					if (body.motion.velocity_x == 0 && body.motion.velocity_y == 0) continue;
					if (target == nullptr || SpaceDistanceSquared(heavy->position, body.motion.position) < SpaceDistanceSquared(heavy->position, target->motion.position)) target = &body;
				}
				REQUIRE(target != nullptr); const auto authority = HeavyRoleAuthority();
				CHECK(CmdSetSpaceMission({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, heavy->index, target->id.base()).Succeeded()); CHECK(HeavyRoleAuthority() == authority);
				REQUIRE(Command<Commands::SetSpaceMission>::Post(heavy->index, target->id.base())); world.Advance(1);
				CHECK(heavy->cargo_cap == GetSpaceFreightCapacity(21, heavy->cargo_type)); CHECK(heavy->cargo_cap == 40); CHECK(heavy->mission.target == target->id);
			} else if (phase == "attached") {
				/** @return Whether actual pursuit gained this hull's own native mining lease, without fabricated physical arrival. */
				world.Await([&] { return heavy->mission.phase == SpaceMissionPhase::Mining; }, 12000);
				CHECK(heavy->mission.work_ticks == 0); CHECK(heavy->cargo.TotalCount() == 0); CHECK(GetMutableAsteroidBody(heavy->mission.target)->mining_slot == heavy->index);
			} else if (phase == "partial") {
				auto *body = GetMutableAsteroidBody(heavy->mission.target); REQUIRE(body != nullptr); REQUIRE(body->mining_slot == heavy->index);
				const auto reserve = body->remaining_reserve; const auto position = body->motion.position;
				AdvanceHeavyRoleCost(world, *heavy, 37, tuning.active_per_day); CHECK(heavy->mission.work_ticks == 37); CHECK(heavy->cargo.TotalCount() == 0); CHECK(body->remaining_reserve == reserve);
				CHECK(body->motion.position != position); CHECK(heavy->position == SpacePoint{body->motion.position.x + heavy->mission.attachment_offset.x, body->motion.position.y + heavy->mission.attachment_offset.y});
				const auto authority = HeavyRoleAuthority(); const auto random = _random;
				CHECK(CmdSetSpaceMission({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, heavy->index, body->id.base()).Succeeded());
				CHECK(CmdServiceLaunchProbe({}, heavy->index, heavy->generation, false).Failed());
				CHECK(CmdEditSpaceOrder({}, heavy->index, heavy->generation + 1, SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Failed());
				CHECK(HeavyRoleAuthority() == authority); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				// Paused callbacks cannot earn extraction, native motion, money or allocation work; only the actual unpaused scheduler owns these opportunities.
				{ AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); for (uint i = 0; i < 4; ++i) StateGameLoop(); }
				CHECK(HeavyRoleAuthority() == authority);
			} else if (phase == "contended") {
				auto *body = GetMutableAsteroidBody(heavy->mission.target); REQUIRE(body != nullptr); REQUIRE(body->mining_slot == heavy->index); REQUIRE(heavy->mission.work_ticks == 37);
				const auto reserve = body->remaining_reserve; const auto extracted = heavy->extracted_units; const auto legacy_extracted = small->extracted_units;
				REQUIRE(Command<Commands::SetSpaceMission>::Post(small->index, body->id.base()));
				const auto legacy_profit = small->profit_this_year; const auto legacy_year = TimerGameEconomy::year; const auto day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
				const uint64_t idle_accrual = small->mission.cost_remainder + uint64_t{legacy.idle_per_day} * 37;
				AdvanceHeavyRoleCost(world, *heavy, 37, tuning.active_per_day);
				// The real waiting small hull pays only its original idle rate while the heavy owns extraction; a blocked request cannot borrow another role's active cost or work.
				Money legacy_observed = small->profit_this_year;
				if (TimerGameEconomy::year != legacy_year) legacy_observed += small->profit_last_year;
				CHECK(small->mission.cost_remainder == idle_accrual % day); CHECK(legacy_observed == legacy_profit - Money{static_cast<int64_t>(idle_accrual / day)} * 256);
				CHECK(heavy->cargo.TotalCount() == tuning.extraction_units); CHECK(heavy->extracted_units == extracted + tuning.extraction_units); CHECK(body->remaining_reserve == reserve - tuning.extraction_units);
				CHECK(body->mining_slot == heavy->index); CHECK(small->cargo_cap == 20); CHECK(small->mission.work_ticks == 0); CHECK(small->extracted_units == legacy_extracted); CHECK(small->cargo.TotalCount() == 0);
				SpacePoint waiting; CHECK(GetSpaceApproachDestination(*small, waiting) == SpaceOrderBlock::ApproachBusy);
				const AsteroidBody *incompatible = nullptr;
				// Loaded actual ore cannot be relabelled by requesting a different resource, even when a new target is otherwise finite and valid.
				for (const auto &candidate : GetAsteroidField().bodies) if (GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(candidate.resource_class)].cargo) != heavy->cargo_type) { incompatible = &candidate; break; }
				REQUIRE(incompatible != nullptr); const auto authority = HeavyRoleAuthority();
				CHECK(CmdSetSpaceMission({}, heavy->index, incompatible->id.base()).Failed()); CHECK(CmdSetSpaceMission(DoCommandFlag::Execute, heavy->index, incompatible->id.base()).Failed()); CHECK(HeavyRoleAuthority() == authority);
			} else if (phase == "legacy-attached") {
				const auto target = heavy->mission.target; auto *body = GetMutableAsteroidBody(target); REQUIRE(body != nullptr); REQUIRE(body->mining_slot == heavy->index);
				const auto reserve = body->remaining_reserve; REQUIRE(heavy->cargo.TotalCount() == tuning.extraction_units);
				REQUIRE(Command<Commands::SetSpaceMission>::Post(heavy->index, INVALID_ORBITAL_BODY.base())); CHECK(body->mining_slot == VehicleID::Invalid()); CHECK(heavy->cargo.TotalCount() == tuning.extraction_units);
				/** @return Whether the real waiting original small miner acquired the released physical body lease by normal pursuit. */
				world.Await([&] { return small->mission.phase == SpaceMissionPhase::Mining; }, 12000);
				CHECK(small->mission.target == target); CHECK(small->mission.work_ticks == 0); CHECK(body->mining_slot == small->index); CHECK(body->remaining_reserve == reserve); CHECK(heavy->cargo.TotalCount() == tuning.extraction_units);
			} else if (phase == "legacy-work") {
				auto *body = GetMutableAsteroidBody(small->mission.target); REQUIRE(body != nullptr); REQUIRE(body->mining_slot == small->index); REQUIRE(small->mission.work_ticks == 0);
				const auto reserve = body->remaining_reserve; const auto extracted = small->extracted_units;
				AdvanceHeavyRoleCost(world, *small, legacy.extraction_ticks, legacy.active_per_day);
				CHECK(small->cargo.TotalCount() == legacy.extraction_units); CHECK(small->extracted_units == extracted + legacy.extraction_units); CHECK(body->remaining_reserve == reserve - legacy.extraction_units);
				CHECK(heavy->cargo.TotalCount() == tuning.extraction_units); CHECK(body->mining_slot == small->index);
			} else {
				REQUIRE(phase == "inspect"); // Ordinary admission observes exact owners without issuing work or a replacement command.
			}
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(GetNextSpacePodGeneration() == pod_generation);
			CHECK(heavy->manifest->returns_remaining == 1); CHECK(heavy->manifest->pending_returns == 0); CHECK(heavy->manifest->services_completed == 0); CHECK(heavy->manifest->pods.size() == 2);
			CHECK(small->manifest->returns_remaining == 1); CHECK(small->manifest->pending_returns == 0); CHECK(small->manifest->services_completed == 0); CHECK(small->manifest->pods.size() == 1);
			// Each original loose bill remains aboard its own hull throughout mining/lease tests; querying service or recovering existing freight consumes no service parts.
			for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) { CHECK(heavy->manifest->supplies[supply].TotalCount() == 2); CHECK(small->manifest->supplies[supply].TotalCount() == 1); }
			if (std::getenv("OTTD_HEAVY_ROLE_CORRUPT") == nullptr) world.Save(fmt::format("heavy-role-{}-437.sav", phase));
			fmt::print("HEAVY_ROLE native=true phase={} heavy_held={} heavy_extracted={} legacy_held={} legacy_extracted={} heavy_work={} legacy_work={}\n", phase,
				heavy->cargo.TotalCount(), heavy->extracted_units, small->cargo.TotalCount(), small->extracted_units, heavy->mission.work_ticks, small->mission.work_ticks); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(heavy_role_observer, &observe); IConsole::CmdRegister("heavy_role_test", ObserveHeavyRole); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
