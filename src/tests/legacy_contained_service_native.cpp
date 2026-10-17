/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file legacy_contained_service_native.cpp Preserve genuine one-part/148-opportunity Core service before heavy contained-job transport changes.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CCB
 * The frozen434 world supplies every paid hull, panel and packet. Original Core stock already owns one RFMT/ELEC pair for a deployed20; small15 and heavy21 keep their own loose parts.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../orbital_service.h"
#include "assembly_native_fixture.h"
#include "../safeguards.h"

static std::function<void()> *legacy_contained_observer = nullptr; ///< Invocation-owned callback; all gameplay work remains with original native owners.

/** @param args Parameter-free actual loaded-world callback. @return Whether the installed compatibility observer ran. */
static bool ObserveLegacyContainedService(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(legacy_contained_observer != nullptr); (*legacy_contained_observer)(); return true;
}

/** Capture genuine original Core work, its interruption and once-only settlement without changing any production or transport rule. */
TEST_CASE("Genuine original construction probe retains its paid contained Core service", "[.legacy-contained-service-native]")
{
	const char *source = std::getenv("OTTD_LEGACY_CONTAINED_SOURCE"), *selected = std::getenv("OTTD_LEGACY_CONTAINED_PHASE"); REQUIRE(source != nullptr); REQUIRE(selected != nullptr);
	const std::string_view phase{selected}; const auto paths = GetTestCaseArtifactPaths("space", "legacy-contained-service-native"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve actual loaded-world assertion failures through normal native application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AssemblyNativeWorld world(paths);
			auto *craft = world.builders.front(), *heavy = ResolveSpaceCraft({VehicleID{22}, 8}); REQUIRE(craft != nullptr); REQUIRE(heavy != nullptr); REQUIRE(GetSpaceCraftReference(*craft) == SpaceCraftRef{VehicleID{18}, 4});
			REQUIRE(GetSpaceVehicleModel(*craft) == 20); REQUIRE(GetSpaceVehicleModel(*heavy) == 21); REQUIRE(craft->manifest != nullptr); REQUIRE(heavy->manifest != nullptr);
			auto &module = *world.core->module; auto &job = module.service; auto &manifest = *craft->manifest;
			const auto fleet = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration(), core_history = module.services_completed;
			/** @return Actual original panel lifetime use, re-queried after common native settlement rather than copied into a feature account. */
			auto used = [&] { return GetUtilityPowerProducer(module.producer)->consumed_lifetime_j; };
			/** @param control Original operation on this exact deployed construction hull's contained Core berth. */
			auto control = [&](OrbitalServiceControl control) { REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(world.core->index, world.core->generation, craft->index, craft->generation, control)); };
			if (phase == "prepared") {
				REQUIRE(job.remaining_ticks == 0); REQUIRE(manifest.services_completed == 0); REQUIRE(manifest.supplies[1].TotalCount() == 0); REQUIRE(manifest.supplies[2].TotalCount() == 0);
				REQUIRE(module.stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount() == 1); REQUIRE(module.stock[GetSpaceSupplyCargo(SpaceSupply::Electronics)].TotalCount() == 1);
				// Only genuine other occupants leave by ordinary Visit; changing pose or a shared berth reservation would bypass the paid approach owner being preserved.
				for (auto *occupant : SpaceVehicle::Iterate()) {
					if (occupant == craft || occupant->owner != craft->owner || occupant->construction.placement != SpacePlacement::Orbital || occupant->position != GetSpaceStructureApproach(world.core->state)) continue;
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(occupant->index, occupant->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(occupant->index, occupant->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				}
				world.Order(*craft, *world.core, SpaceOrderAction::Visit);
				/** @return Whether original native travel admitted the exact deployed construction hull at the original Core's physical berth. */
				world.Await([&] { return GetSpaceStructureBerth(*craft, *world.core).has_value(); });
				// The original Core already owns the complete paid bill. No freight trip, hull deployment, loose-part exchange or new starting fuel is needed to reserve the unchanged old job.
				const auto structures = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest(), supplies = ComputeSpaceManifestDigest(); const auto money = Company::Get(_current_company)->money;
				CHECK(CmdServiceOrbitalProbe({}, world.core->index, world.core->generation, craft->index, craft->generation, OrbitalServiceControl::Start).Succeeded());
				CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == power); CHECK(ComputeSpaceManifestDigest() == supplies); CHECK(Company::Get(_current_company)->money == money);
				control(OrbitalServiceControl::Start); CHECK(job.remaining_ticks == 148); CHECK(job.last_work_interval == 0); CHECK(job.services_at_start == 0);
			} else if (phase == "partial") {
				REQUIRE(job.remaining_ticks == 148); const auto before = used(); world.Advance(37);
				CHECK(job.remaining_ticks == 111); CHECK(used() == before + uint64_t{37} * 750); CHECK(job.last_work_interval == GetUtilityPowerInterval());
			} else if (phase == "paused") {
				REQUIRE(job.remaining_ticks == 111); control(OrbitalServiceControl::Pause); const auto before = used(); world.Advance(9);
				CHECK(job.remaining_ticks == 111); CHECK(job.paused); CHECK(used() == before);
			} else if (phase == "complete") {
				REQUIRE(job.remaining_ticks == 111); REQUIRE(job.paused); const auto before = used(); control(OrbitalServiceControl::Resume); world.Advance(111);
				CHECK(used() == before + uint64_t{111} * 750); CHECK(job.remaining_ticks == 0); CHECK(job.craft == SpaceCraftRef{}); CHECK(module.services_completed == core_history + 1);
				CHECK(manifest.services_completed == 1); CHECK(manifest.returns_remaining == 2); CHECK(job.inputs[0].TotalCount() == 0); CHECK(job.inputs[1].TotalCount() == 0);
				world.Advance(10); CHECK(manifest.services_completed == 1); CHECK(module.services_completed == core_history + 1); CHECK(used() == before + uint64_t{111} * 750);
			} else {
				REQUIRE(phase == "resave"); // A reconstruction preserves the original job and earns no new work or allowance.
			}
			// The compatibility branch preserves the exhausted original heavy's unspent bill, paid identity, empty hold and last physical shell while the original construction probe uses its existing service producer.
			CHECK(heavy->manifest->supplies[1].TotalCount() == 2); CHECK(heavy->manifest->supplies[2].TotalCount() == 2); CHECK(heavy->manifest->services_completed == 0); CHECK(heavy->manifest->returns_remaining == 0);
			CHECK(heavy->manifest->pods.size() == 1); CHECK(heavy->extracted_units == 80); CHECK(heavy->cargo.TotalCount() == 0); CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == generation);
			CHECK(manifest.service_ticks == 0); CHECK(manifest.pending_returns == 0); CHECK(manifest.supplies[1].TotalCount() == 0); CHECK(manifest.supplies[2].TotalCount() == 0);
			// Every unfinished operation retains both complete original reserved inputs and its exact original hull service serial; a plain reconstruction may instead preserve a completed empty job.
			if (job.remaining_ticks != 0) {
				CHECK(job.craft == GetSpaceCraftReference(*craft)); CHECK(job.inputs[0].TotalCount() == 1); CHECK(job.inputs[1].TotalCount() == 1); CHECK(job.services_at_start == 0); CHECK(manifest.services_completed == 0);
			}
			world.Save(fmt::format("legacy-contained-{}-435.sav", phase)); fmt::print("LEGACY_CONTAINED native=true phase={} work={} services={} source_use={}\n", phase, job.remaining_ticks, manifest.services_completed, used()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(legacy_contained_observer, &observe); IConsole::CmdRegister("legacy_contained_test", ObserveLegacyContainedService); const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
