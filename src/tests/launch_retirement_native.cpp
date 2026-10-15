/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_retirement_native.cpp Stand-alone recovery-center demolition refusal with actual recovered mined cargo.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group16.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../space_vehicle.h"
#include "../space_pod.h"
#include "../asteroid.h"
#include "../station_base.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include "../rail_type.h"
#include "../newgrf_station.h"
#include "../command_func.h"
#include "../vehicle_cmd.h"
#include "../vehiclelist.h"
#include "../order_base.h"
#include "../order_cmd.h"
#include "../company_func.h"
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

static std::function<void()> *retirement_observer = nullptr; ///< Observer only; actual station cargo and demolition command own the decision.

/** @param args Parameter-free loaded-world callback. @return Whether the installed observer ran. */
static bool ObserveLaunchRetirement(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(retirement_observer != nullptr); (*retirement_observer)(); return true;
}

/** Consume the genuinely paid alternate recovery center after its ten-unit descent completed, without artificial inventory. */
TEST_CASE("Stand-alone launch recovery demolition cannot strand its actual returned cargo", "[.launch-retirement]")
{
	const char *input = std::getenv("OTTD_LAUNCH_RETIREMENT_SOURCE"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "launch-retirement"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Inspect only actual paid retained owners and propagate assertion failure through native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AutoRestoreBackup running(_pause_mode, PauseModes{}); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			SpaceVehicle *miner = nullptr;
			// The genuine retarget predecessor retains one original miner with ten held units and ten already recovered into the separately paid center.
			for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 15) { REQUIRE(miner == nullptr); miner = craft; }
			REQUIRE(miner != nullptr); REQUIRE(miner->cargo.TotalCount() == 10); Station *receiver = nullptr;
			// Select only the actual owned alternate center containing the completed descent; neither the miner's home nor the separately paid foreign center qualifies.
			for (const auto ref : GetLaunchCenterReferences()) {
				auto *candidate = ResolveLaunchCenter(ref);
				if (candidate->owner == miner->owner && ref != miner->construction.home && candidate->goods[miner->cargo_type].TotalCount() == 10) { REQUIRE(receiver == nullptr); receiver = candidate; }
			}
			REQUIRE(receiver != nullptr); const auto reference = GetLaunchCenterReference(*receiver); const auto tile = receiver->launch_center->tile;
			CHECK_FALSE(HasLaunchCraft(reference)); CHECK_FALSE(HasLaunchPods(reference)); CHECK_FALSE(HasSpaceDescentForCenter(reference));
			CHECK_FALSE(receiver->facilities.Test(StationFacility::Train)); CHECK_FALSE(receiver->facilities.Test(StationFacility::TruckStop)); CHECK_FALSE(receiver->facilities.Test(StationFacility::Dock)); CHECK_FALSE(receiver->facilities.Test(StationFacility::Airport));
			const auto digest = ComputeSpaceVehicleDigest(); const auto stock = receiver->goods[miner->cargo_type].GetData().cargo.Packets(); const auto packets = *stock;
			CHECK(CmdRemoveLaunchCenter({}, tile).Failed()); CHECK(CmdRemoveLaunchCenter({DoCommandFlag::Execute}, tile).Failed());
			CHECK(ResolveLaunchCenter(reference) == receiver); CHECK(receiver->goods[miner->cargo_type].TotalCount() == 10); CHECK(*receiver->goods[miner->cargo_type].GetData().cargo.Packets() == packets); CHECK(ComputeSpaceVehicleDigest() == digest);
			uint64_t extracted = 0, actual = 0;
			// Demolition cannot erase the finite debit or either of its real physical packet owners.
			for (const auto &body : GetAsteroidField().bodies) extracted += body.initial_reserve - body.remaining_reserve;
			for (const auto *packet : CargoPacket::Iterate()) if (packet->GetSource().type == SourceType::Asteroid) actual += packet->Count();
			CHECK(extracted == 20); CHECK(actual == 20); REQUIRE(ValidateSpaceVehicles());
			SpaceVehicle *rocket = nullptr;
			// The original materially paid rocket also remains in this genuine predecessor; ordinary airport/hangar commands cannot adopt it as an aircraft.
			for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 14) { REQUIRE(rocket == nullptr); rocket = craft; }
			REQUIRE(rocket != nullptr); const auto fleet = ComputeSpaceVehicleDigest(); Order airport_order; airport_order.MakeGoToStation(receiver->index);
			const VehicleListIdentifier aircraft_list{VehicleListType::Company, VehicleType::Aircraft, rocket->owner};
			// Both estimation and synchronized execution must reject the ordinary aircraft command family before any cargo, fuel or typed mission changes.
			for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
				CHECK(CmdInsertOrder(flags, rocket->index, 0, airport_order).Failed());
				CHECK(CmdSendVehicleToDepot(flags, rocket->index, {}, aircraft_list).Failed());
				CHECK(std::get<0>(CmdBuildVehicle(flags, tile, rocket->engine_type, false, rocket->cargo_type, ClientID{})).Failed());
				CHECK(std::get<0>(CmdCloneVehicle(flags, tile, rocket->index, false)).Failed());
				CHECK(std::get<0>(CmdRefitVehicle(flags, rocket->index, rocket->cargo_type, 0, false, true, 1)).Failed());
			}
			CHECK(ComputeSpaceVehicleDigest() == fleet);
			AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
			{
				AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / "launch-retirement-refused-423.sav"), false) == SaveLoadResult::Ok);
			}
			TileIndex rail = INVALID_TILE;
			// Find an actual legal adjacent loading point using the native placement query; no terrain or station inventory is synthesized to exercise the joined-freight branch.
			for (const auto cell : *GetLaunchCenterFootprint(tile, receiver->launch_center->rotation)) {
				for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
					const auto candidate = TileAddByDiagDir(cell, direction);
					if (CmdBuildRailStation({}, candidate, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, receiver->index, false).Succeeded()) { rail = candidate; break; }
				}
				if (rail != INVALID_TILE) break;
			}
			REQUIRE(rail != INVALID_TILE);
			REQUIRE(Command<Commands::BuildRailStation>::Post(rail, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, receiver->index, false));
			REQUIRE(receiver->facilities.Test(StationFacility::Train)); REQUIRE(CmdRemoveLaunchCenter({}, tile).Succeeded());
			REQUIRE(Command<Commands::RemoveLaunchCenter>::Post(tile)); CHECK(ResolveLaunchCenter(reference) == nullptr);
			CHECK(Station::GetIfValid(receiver->index) == receiver); CHECK(IsRailStationTile(rail));
			CHECK(receiver->goods[miner->cargo_type].TotalCount() == 10); CHECK(*receiver->goods[miner->cargo_type].GetData().cargo.Packets() == packets);
			REQUIRE(ValidateSpaceVehicles());
			{
				AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / "launch-retirement-joined-423.sav"), false) == SaveLoadResult::Ok);
			}
			fmt::print("LAUNCH_RETIREMENT native=true actual_recovered=10 no_other_work=true cargo_preserved=true\n");
			fmt::print("LAUNCH_RETIREMENT joined_freight=true actual_recovered=10 ordinary_airport_refused=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(retirement_observer, &observe); IConsole::CmdRegister("launch_retirement_test", ObserveLaunchRetirement);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
