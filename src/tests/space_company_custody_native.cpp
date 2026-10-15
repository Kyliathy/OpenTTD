/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_company_custody_native.cpp Actual company deletion refusal and native ownership-entry transfer of genuine paid cargo/work.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group15.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../asteroid.h"
#include "../station_base.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_cmd.h"
#include "../economy_func.h"
#include "../network/network_type.h"
#include "../core/random_func.hpp"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include <filesystem>
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include "../safeguards.h"

static std::function<void()> *company_custody_observer = nullptr; ///< Observer only; real ownership/deletion boundaries own the transaction.

/** @param args Parameter-free loaded-world observer. @return Whether the installed native callback ran. */
static bool ObserveSpaceCompanyCustody(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(company_custody_observer != nullptr); (*company_custody_observer)(); return true;
}

/** Verify one exact refusal or ownership-entry transfer from the original paid miner, real cargo and ongoing paid descent. */
TEST_CASE("Company liquidation preserves unsettled Space cargo and takeover retains exact owners", "[.space-company-custody]")
{
	const char *input = std::getenv("OTTD_SPACE_COMPANY_SOURCE"), *phase_env = std::getenv("OTTD_SPACE_COMPANY_PHASE");
	REQUIRE(input != nullptr); REQUIRE(phase_env != nullptr); const std::string phase{phase_env};
	const auto paths = GetTestCaseArtifactPaths("space", "space-company-custody"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Load only genuine paid owners and propagate assertions through native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			REQUIRE(Company::GetNumItems() == 2); SpaceVehicle *miner = nullptr;
			// The retained414 world has its original one paid miner, ten real held units, one paid carried shell and a separate ten-unit descent.
			for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 15) { REQUIRE(miner == nullptr); miner = craft; }
			REQUIRE(miner != nullptr); REQUIRE(miner->cargo.TotalCount() == 10); REQUIRE(miner->manifest != nullptr); REQUIRE(miner->manifest->pending_returns == 1);
			const auto craft_ref = GetSpaceCraftReference(*miner); const auto home = miner->construction.home; const auto owner = miner->owner;
			auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr); const auto descents = CaptureSpaceDescents(); REQUIRE(descents.size() == 1); REQUIRE(descents.front().remaining_ticks == 111);
			/** Prove command query/execution cannot erase or re-home this actual company's native cargo, paid work, money or randomness. */
			auto refusal = [&] {
				const auto vehicles = ComputeSpaceVehicleDigest(), pods = ComputeSpacePodDigest(), jobs = ComputeSpaceDescentDigest(); const auto random = _random;
				CHECK(CmdCompanyCtrl({}, CompanyCtrlAction::Delete, static_cast<CompanyID>(miner->owner), CompanyRemoveReason::Manual, ClientID::Invalid).Failed());
				CHECK(CmdCompanyCtrl({DoCommandFlag::Execute}, CompanyCtrlAction::Delete, static_cast<CompanyID>(miner->owner), CompanyRemoveReason::Bankrupt, ClientID::Invalid).Failed());
				CHECK(Company::GetNumItems() == 2); CHECK(ComputeSpaceVehicleDigest() == vehicles); CHECK(ComputeSpacePodDigest() == pods); CHECK(ComputeSpaceDescentDigest() == jobs);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]); CHECK(ResolveSpaceCraft(craft_ref) == miner); CHECK(miner->cargo.TotalCount() == 10);
			};
			refusal();
			if (phase == "transfer") {
				CompanyID buyer = CompanyID::Invalid();
				// Use the real existing foreign company from the paid predecessor, never a local invented company ID or private copied cargo owner.
				for (const auto *company : Company::Iterate()) if (company->index != owner) { buyer = company->index; break; }
				REQUIRE(buyer != CompanyID::Invalid()); const auto fleet = CaptureSpaceProduction(); const auto shells = CaptureSpacePods(); const auto held = *miner->cargo.Packets();
				ChangeOwnershipOfCompanyItems(owner, buyer); // Actual native takeover item-entry; company-auction capital pricing is not claimed by this fixture.
				REQUIRE(ValidateSpaceVehicles()); CHECK(ResolveSpaceCraft(craft_ref) == miner); CHECK(miner->owner == buyer); CHECK(ResolveLaunchCenter(home) == center); CHECK(center->owner == buyer);
				CHECK(miner->construction.home == home); CHECK(*miner->cargo.Packets() == held); const auto transferred = CaptureSpaceDescents(); REQUIRE(transferred.size() == 1);
				CHECK(transferred.front().id == descents.front().id); CHECK(transferred.front().pod == descents.front().pod); CHECK(transferred.front().center == descents.front().center);
				CHECK(transferred.front().packets == descents.front().packets); CHECK(transferred.front().remaining_ticks == descents.front().remaining_ticks); CHECK(GetIncomingSpaceDescent(home, miner->cargo_type) == 10);
				// Every original paid hull and pod retains its native incarnation/history while changing only through the actual company item-transfer owner.
				for (const auto &record : fleet) { const auto *craft = ResolveSpaceCraft(record.craft); REQUIRE(craft != nullptr); CHECK(craft->owner == buyer); CHECK(craft->construction == record.state); }
				for (const auto &record : shells) { const auto *pod = ResolveSpacePod(record.ref); REQUIRE(pod != nullptr); CHECK(pod->state.owner == buyer); CHECK(pod->generation == record.ref.generation); }
				CHECK(CmdReleaseOrbitalCargo({}, miner->index, miner->generation, home.station, home.generation, miner->manifest->pods.front().pod, miner->manifest->pods.front().generation, 10).Failed()); // Original command actor no longer owns these packets.
				AutoRestoreBackup buyer_actor(_current_company, buyer); refusal();
				CHECK(CmdRetargetSpaceDescent({}, descents.front().pod.pod, descents.front().pod.generation, home.station, home.generation).Succeeded());
				CHECK(CmdRemoveLaunchCenter({}, center->launch_center->tile).Failed());
				CHECK(CmdCompanyCtrl({}, CompanyCtrlAction::Delete, static_cast<CompanyID>(owner), CompanyRemoveReason::Manual, ClientID::Invalid).Succeeded()); // The now-empty original company has no remaining Space custody to block deletion.
			} else { REQUIRE(phase == "refusal"); }
			uint64_t extracted = 0, packets = 0;
			// Company control cannot silently refund the asteroid debit or destroy any of its twenty real packets.
			for (const auto &body : GetAsteroidField().bodies) extracted += body.initial_reserve - body.remaining_reserve;
			for (const auto *packet : CargoPacket::Iterate()) if (packet->GetSource().type == SourceType::Asteroid) packets += packet->Count();
			CHECK(extracted == 20); CHECK(packets == 20); REQUIRE(ValidateSpaceVehicles());
			AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / ("space-company-" + phase + "-423.sav")), false) == SaveLoadResult::Ok);
			fmt::print("SPACE_COMPANY native=true phase={} conserved=20 removal_refused=true ownership_entry=true\n", phase);
		} catch (...) { failure = std::current_exception(); }
		// The real ownership entry may select the receiver as local company; after the observer's scoped actor restores, native input must resume with that actual local actor.
		_current_company = _local_company;
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(company_custody_observer, &observe); IConsole::CmdRegister("space_company_test", ObserveSpaceCompanyCustody);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
