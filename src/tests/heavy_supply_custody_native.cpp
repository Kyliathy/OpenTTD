/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_supply_custody_native.cpp Original paid rocket supplies physical shells and parts to the original orbital heavy miner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CB
 * The separate starting-fuel phase declares only exhausted Surface fixture fuel, with no procurement claim. Actual supply commands create no stock, hull, shell, pose, work or allowance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../launch_operations.h"
#include "../space_mining_tuning.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *heavy_supply_observer = nullptr; ///< Process-local callback; the native world retains all gameplay authority.

/** @param args Parameter-free loaded-world console callback. @return Whether the installed original-world observer ran. */
static bool ObserveHeavySupplyCustody(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(heavy_supply_observer != nullptr); (*heavy_supply_observer)(); return true;
}

/** @return Exact public gameplay digests used to prove queries, copied checks and repeated handovers retain every original owner. */
static auto SupplyAuthority()
{
	return std::array<uint64_t, 11>{ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest(), ComputeUtilityPowerDigest(), ComputeSpaceManifestDigest(),
		ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(), ComputeSpaceOrdersDigest(), ComputeAsteroidFieldDigest(GetAsteroidField()),
		ComputeLaunchProductionDigest(), ComputeLaunchCenterDigest(), ComputeSpaceDescentDigest()};
}

/** Exercise one bounded actual resupply milestone, or reject a copied malformed save without changing the loaded paid world. */
TEST_CASE("Paid heavy supply keeps original shell custody and delivers once without service", "[.heavy-supply-custody-native]")
{
	const char *source = std::getenv("OTTD_HEAVY_SUPPLY_SOURCE"), *phase_value = std::getenv("OTTD_HEAVY_SUPPLY_PHASE"); REQUIRE(source != nullptr); REQUIRE(phase_value != nullptr);
	const std::string_view phase{phase_value}; const auto paths = GetTestCaseArtifactPaths("space", "heavy-supply-custody-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve actual loaded-world assertion failures through the application's ordinary shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			auto *heavy = ResolveSpaceCraft({VehicleID{22}, 8}); REQUIRE(heavy != nullptr); REQUIRE(GetSpaceVehicleModel(*heavy) == SPACE_HEAVY_MINER_MODEL); REQUIRE(heavy->manifest != nullptr);
			auto &rocket = *world.rocket; REQUIRE(rocket.index == VehicleID{15}); REQUIRE(rocket.manifest != nullptr); REQUIRE(rocket.transition != nullptr);
			auto &held = *heavy->manifest; auto &from = *rocket.manifest; const auto receiver = GetSpaceCraftReference(*heavy);
			const auto fleet = GetSpaceVehicleCount(); const auto shells = SpacePod::GetNumItems(); const auto generation = GetNextSpaceVehicleGeneration(), pod_generation = GetNextSpacePodGeneration();
			REQUIRE(heavy->construction.surface_tile == INVALID_TILE); REQUIRE(heavy->construction.home == LaunchCenterRef{}); REQUIRE(heavy->construction.origin == LaunchCenterRef{});
			REQUIRE(heavy->construction.assembly != SpaceStructureRef{}); REQUIRE(heavy->construction.assembly_serial != 0); REQUIRE(heavy->extracted_units == 40); REQUIRE(heavy->cargo.TotalCount() == 0);
			if (const char *corrupt = std::getenv("OTTD_HEAVY_SUPPLY_CORRUPT")) {
				const auto authority = SupplyAuthority(); const auto money = Company::Get(_current_company)->money; const auto random = _random;
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(SupplyAuthority() == authority); CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("HEAVY_SUPPLY preview_reject=true authority_unchanged=true\n");
			} else if (phase == "starting-fuel") {
				auto *center = ResolveLaunchCenter(rocket.construction.home); REQUIRE(center != nullptr); const auto cargo = GetSpaceSupplyCargo(SpaceSupply::Propellant); REQUIRE(IsValidCargoType(cargo));
				auto &stock = center->goods[cargo].GetOrCreateData().cargo; const auto &budget = GetMultiLayerSpaceBudget(); const uint required = uint{budget.outbound_fuel} + budget.recovery_fuel;
				const auto available = stock.AvailableCount(); REQUIRE(available < required); const auto missing = static_cast<uint16_t>(required - available); const auto money = Company::Get(_current_company)->money;
				// The immutable inherited transport fixture has exhausted its declared Earth fuel. This separately saved fixture extension declares only the missing one-round-trip Surface input; all original paid shells/parts and every orbital owner remain intact, and procurement acceptance stays false.
				REQUIRE(CargoPacket::CanAllocateItem()); stock.Append(CargoPacket::Create(center->index, missing, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
				CHECK(stock.AvailableCount() == required); CHECK(Company::Get(_current_company)->money == money); CHECK(from.pods.empty()); CHECK(held.pods.empty());
				fmt::print("HEAVY_SUPPLY starting_fuel_fixture=true added={} retained={} procurement_acceptance=false\n", missing, available);
			} else if (phase == "overfull") {
				REQUIRE(held.pods.empty()); REQUIRE(from.pods.empty()); REQUIRE(from.supplies[1].TotalCount() == 0); REQUIRE(from.supplies[2].TotalCount() == 0);
				auto *center = ResolveLaunchCenter(rocket.construction.home); REQUIRE(center != nullptr); const auto home = GetLaunchCenterReference(*center); uint packed = 0;
				// Pack three distinct existing paid shells through the real Surface command, deliberately exceeding the receiver's two-shell capacity without allocating a replacement shell.
				for (const auto *pod : SpacePod::Iterate()) {
					if (pod->state.custody != SpacePodCustody::CenterStored || pod->state.home != home) continue;
					REQUIRE(Command<Commands::MoveLaunchPod>::Post(home.station, home.generation, rocket.index, rocket.generation, pod->index, pod->generation, false));
					if (++packed == 3) break;
				}
				REQUIRE(packed == 3);
				// Every compartment loads real available station packets; inherited starting stock is disclosed and never replenished by the observer.
				for (uint8_t index : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket.index, rocket.generation, index, uint16_t{2}, false));
				const auto &budget = GetMultiLayerSpaceBudget(); const uint fuel = uint{budget.outbound_fuel} + budget.recovery_fuel; REQUIRE(from.supplies[0].TotalCount() <= fuel);
				if (from.supplies[0].TotalCount() < fuel) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket.index, rocket.generation, uint8_t{0}, static_cast<uint16_t>(fuel - from.supplies[0].TotalCount()), false));
				const auto authority = SupplyAuthority(); const auto money = Company::Get(_current_company)->money; const auto random = _random;
				CHECK(GetSpaceResupplyBlock(rocket, receiver) == SpaceFlightBlock::ReceiverFull);
				CHECK(CmdBeginLaunchFlight({}, rocket.index, rocket.generation, SpaceOrderTargetKind::Craft, heavy->index.base(), heavy->generation).Failed());
				CHECK(SupplyAuthority() == authority); CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			} else if (phase == "prepared") {
				REQUIRE(from.pods.size() == 3); const auto ref = from.pods.back(); const auto home = rocket.construction.home;
				REQUIRE(Command<Commands::MoveLaunchPod>::Post(home.station, home.generation, rocket.index, rocket.generation, ref.pod, ref.generation, true));
				const auto authority = SupplyAuthority(); const auto money = Company::Get(_current_company)->money; const auto random = _random;
				CHECK(GetSpaceResupplyBlock(rocket, receiver) == SpaceFlightBlock::None);
				CHECK(CmdBeginLaunchFlight({}, rocket.index, rocket.generation, SpaceOrderTargetKind::Craft, heavy->index.base(), heavy->generation).Succeeded());
				CHECK(CmdBeginLaunchFlight({}, rocket.index, rocket.generation, SpaceOrderTargetKind::Craft, heavy->index.base(), heavy->generation + 1).Failed());
				{
					AutoRestoreBackup foreign(_current_company, CompanyID{1}); CHECK(CmdBeginLaunchFlight({}, rocket.index, rocket.generation, SpaceOrderTargetKind::Craft, heavy->index.base(), heavy->generation).Failed());
				}
				CHECK(SupplyAuthority() == authority); CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			} else if (phase == "ascent") {
				const auto money = Company::Get(_current_company)->money; const auto started = rocket.transition->missions_started; const auto burned = from.fuel_burned;
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket.index, rocket.generation, SpaceOrderTargetKind::Craft, heavy->index.base(), heavy->generation));
				CHECK(Company::Get(_current_company)->money == money - GetMultiLayerSpaceBudget().mission_fee); CHECK(rocket.transition->missions_started == started + 1);
				world.Advance(GetMultiLayerSpaceBudget().prepare_ticks + 37);
				CHECK(rocket.transition->phase == SpaceFlightPhase::Ascent); CHECK(rocket.transition->remaining_ticks == GetMultiLayerSpaceBudget().flight_ticks - 37);
				CHECK(from.fuel_burned == burned + GetMultiLayerSpaceBudget().outbound_fuel); CHECK(from.supplies[0].TotalCount() == GetMultiLayerSpaceBudget().recovery_fuel);
			} else if (phase == "busy") {
				REQUIRE(rocket.transition->phase == SpaceFlightPhase::Ascent); const AsteroidBody *target = nullptr;
				// A normal long Visit makes the genuine receiver busy during arrival; coordinates remain under the existing moving-body/navigation authorities.
				for (const auto &body : GetAsteroidField().bodies) if (target == nullptr || SpaceDistanceSquared(heavy->position, body.motion.position) > SpaceDistanceSquared(heavy->position, target->motion.position)) target = &body;
				REQUIRE(target != nullptr); const auto origin = heavy->position;
				const AsteroidBody *other = nullptr;
				// A second actual distant body permits a long explicit navigation queue, keeping the receiver genuinely busy throughout ascent without relying on one trip's incidental travel duration.
				for (const auto &body : GetAsteroidField().bodies) if (other == nullptr || SpaceDistanceSquared(target->motion.position, body.motion.position) > SpaceDistanceSquared(target->motion.position, other->motion.position)) other = &body;
				REQUIRE(other != nullptr); REQUIRE(other->id != target->id);
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(heavy->index, heavy->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				// Every leg is a normal typed Visit between real body identities; cancellation after the saved busy checkpoint stops only this actual queue and never edits the hull's physical pose.
				for (uint8_t leg = 0; leg < MAX_SPACE_ORDERS; ++leg) REQUIRE(Command<Commands::EditSpaceOrder>::Post(heavy->index, heavy->generation, SpaceOrderEdit::Insert, leg, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, (leg % 2 == 0 ? target : other)->id.base(), uint64_t{0}));
				/** @return Whether the actual flight has reached a genuine whole-payload receiver-busy wait. */
				world.Await([&] { return rocket.transition->phase == SpaceFlightPhase::OrbitalDelivery && rocket.transition->blocked == SpaceFlightBlock::ReceiverBusy; });
				CHECK(heavy->position != origin); CHECK_FALSE(rocket.transition->resupplied);
			} else if (phase == "delivered") {
				REQUIRE(rocket.transition->blocked == SpaceFlightBlock::ReceiverBusy); REQUIRE(from.pods.size() == 2); const auto refs = from.pods;
				std::array<SpacePodState, 2> before; for (uint i = 0; i < refs.size(); ++i) before[i] = ResolveSpacePod(refs[i])->state;
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(heavy->index, heavy->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				/** @return Whether the original native opportunity completed the single actual paid handover. */
				world.Await([&] { return rocket.transition->resupplied; }); REQUIRE(held.pods == refs);
				// Each original shell changes only physical carrier/location: its paid provenance and last real Earth custody remain intact on the factory product with no Earth home.
				for (uint i = 0; i < refs.size(); ++i) {
					const auto *pod = ResolveSpacePod(refs[i]); REQUIRE(pod != nullptr); CHECK(pod->state.craft == receiver); CHECK(pod->state.owner == heavy->owner);
					CHECK(pod->state.position == heavy->position); CHECK(pod->state.surface_tile == before[i].surface_tile); CHECK(pod->state.surface_tile != INVALID_TILE);
					CHECK(pod->state.content_version == before[i].content_version); CHECK(pod->state.capital_paid == before[i].capital_paid); CHECK(pod->state.origin == before[i].origin);
				}
				const auto authority = SupplyAuthority(); CHECK_FALSE(CommitSpaceResupply(rocket, receiver)); CHECK(SupplyAuthority() == authority);
			} else if (phase == "complete") {
				REQUIRE(rocket.transition->resupplied); const auto completed = rocket.transition->missions_completed; const auto refs = held.pods;
				/** @return Whether the same original rocket completed recovery and turnaround into actual Surface storage. */
				world.Await([&] { return rocket.transition->phase == SpaceFlightPhase::GroundLoading; });
				CHECK(rocket.transition->missions_completed == completed + 1); CHECK(rocket.construction.placement == SpacePlacement::SurfaceStored); world.Advance(10); CHECK(held.pods == refs);
			} else {
				REQUIRE(phase == "resave"); // Ordinary reconstruction creates no paid delivery, Surface work or new inventory.
			}
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(SpacePod::GetNumItems() == shells); CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(GetNextSpacePodGeneration() == pod_generation);
			CHECK(held.returns_remaining == 1); CHECK(held.pending_returns == 0); CHECK(held.service_ticks == 0); CHECK(held.services_completed == 0);
			CHECK(heavy->construction.surface_tile == INVALID_TILE); CHECK(heavy->construction.home == LaunchCenterRef{}); CHECK(heavy->extracted_units == 40); CHECK(heavy->cargo.TotalCount() == 0);
			// Before delivery the whole original payload remains on the rocket; afterwards the same exact receiver owns all shells/parts, with no service or allowance renewal.
			if (std::getenv("OTTD_HEAVY_SUPPLY_CORRUPT") == nullptr && phase != "resave" && phase != "starting-fuel") {
				const bool delivered = phase == "delivered" || phase == "complete";
				CHECK(held.pods.size() == (delivered ? 2 : 0)); CHECK(from.pods.size() == (delivered ? 0 : phase == "overfull" ? 3 : 2));
				CHECK(held.supplies[1].TotalCount() == (delivered ? 2 : 0)); CHECK(held.supplies[2].TotalCount() == (delivered ? 2 : 0));
				CHECK(from.supplies[1].TotalCount() == (delivered ? 0 : 2)); CHECK(from.supplies[2].TotalCount() == (delivered ? 0 : 2));
			}
			if (std::getenv("OTTD_HEAVY_SUPPLY_CORRUPT") == nullptr) world.Save(fmt::format("heavy-supply-{}-433.sav", phase));
			fmt::print("HEAVY_SUPPLY native=true phase={} vehicle={} generation={} pods={} metal={} electronics={} returns={} services={}\n", phase,
				heavy->index.base(), heavy->generation, held.pods.size(), held.supplies[1].TotalCount(), held.supplies[2].TotalCount(), held.returns_remaining, held.services_completed);
			observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(heavy_supply_observer, &observe); IConsole::CmdRegister("heavy_supply_custody_test", ObserveHeavySupplyCustody); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
