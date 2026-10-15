/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file cargo_return_native.cpp Genuine outside-circle recall, explicit Unload and precise Visit with actual mined packet history.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group14.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_orders.h"
#include "../asteroid.h"
#include "../station_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include <filesystem>
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *return_observer = nullptr; ///< Loaded-world observer only; native motion and paid owners do all work.

/** Actual immutable history copied from mined packets before their original hold relinquishes custody. */
struct CargoReturnPacketHistory {
	CargoPacketID id; ///< Actual original packet identity; a whole-hold move creates no replacement packet.
	Source source; ///< Actual asteroid source identity.
	TileIndex tile; ///< Original extraction plane/tile.
	StationID first, next; ///< Original CargoDist pickup/next-hop history.
	uint16_t age; ///< Canonical age before normal movement/release.
};

/** @param args Parameter-free loaded-world console callback. @return Whether the installed observer ran. */
static bool ObserveCargoReturn(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(return_observer != nullptr); (*return_observer)(); return true;
}

/** Execute one actual cargo-return phase from the genuine miner's paid outside-circle admission predecessor. */
TEST_CASE("Cargo return stops inside its explicit owned circle while Visit retains precise arrival", "[.cargo-return-policy]")
{
	const char *input = std::getenv("OTTD_CARGO_RETURN_SOURCE"), *phase_env = std::getenv("OTTD_CARGO_RETURN_PHASE");
	REQUIRE(input != nullptr); REQUIRE(phase_env != nullptr); const std::string phase{phase_env};
	const auto paths = GetTestCaseArtifactPaths("space", "cargo-return-policy"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect only the ordinary loaded real paid miner and propagate assertions through native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			SpaceVehicle *miner = nullptr;
			// Resolve the sole original model15 hull from the genuine predecessor, rather than commissioning or privately relocating a boundary craft.
			for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == 15) { REQUIRE(miner == nullptr); miner = v; }
			REQUIRE(miner != nullptr); REQUIRE(miner->manifest != nullptr); const auto home = miner->construction.home;
			auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr); const auto anchor = *GetLaunchCenterPlaneReference(center->launch_center->tile);
			/** @param action Requested native action. @param target Actual generation-qualified explicitly selected center. */
			auto set_order = [&](SpaceOrderAction action, LaunchCenterRef target) {
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Insert, uint8_t{0}, action, SpaceOrderTargetKind::LaunchCenter, uint32_t{target.station.base()}, target.generation));
			};
			/** @param ticks Actual synchronized opportunities; the original idle miner may accrue published upkeep but no cargo income. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Count actual native opportunities and verify expense/remainder; render calls and stalled loop attempts cannot move or pay the cargo.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks;
					const auto profit = miner->profit_this_year; const uint accrual = miner->mission.cost_remainder + 100; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); CHECK(miner->profit_this_year == profit - Money{accrual / 74} * 256); CHECK(miner->mission.cost_remainder == accrual % 74); ++done;
				}
			};
			if (phase == "visit") {
				REQUIRE(miner->cargo.TotalCount() == 0); REQUIRE(CaptureSpaceDescents().size() == 1); const auto shell = CaptureSpaceDescents().front().pod;
				REQUIRE(miner->position != anchor); REQUIRE(IsInsideLaunchCenterDropZone(miner->position, anchor)); set_order(SpaceOrderAction::Visit, home);
				// Visit must actually traverse the remaining in-circle distance to its precise service/navigation anchor; descent progresses independently during that flight.
				for (uint elapsed = 0; miner->space_orders.cursor != miner->space_orders.entries.size(); ++elapsed) { REQUIRE(elapsed < 4000); advance(1); }
				CHECK(miner->position == anchor); CHECK(ResolveSpacePod(shell) == nullptr); CHECK(CaptureSpaceDescents().empty()); CHECK(center->goods[miner->cargo_type].TotalCount() == 20);
				const auto next = GetNextSpaceDescentID(); set_order(SpaceOrderAction::Unload, home); advance(1);
				CHECK(miner->space_orders.blocked == SpaceOrderBlock::Completed); CHECK(GetNextSpaceDescentID() == next); CHECK(center->goods[miner->cargo_type].TotalCount() == 20);
			} else {
				REQUIRE(miner->cargo.TotalCount() == 10); REQUIRE(miner->manifest->pods.size() == 1); REQUIRE(CaptureSpaceDescents().empty()); REQUIRE_FALSE(IsInsideLaunchCenterDropZone(miner->position, anchor));
				if (phase == "recall") {
					REQUIRE(Command<Commands::SetSpaceMission>::Post(miner->index, UINT32_MAX));
					// The public recall moves the real loaded hull until the owned circle is reached; no exact home rendezvous or automatic release is needed.
					for (uint elapsed = 0; miner->mission.phase != SpaceMissionPhase::Holding; ++elapsed) { REQUIRE(elapsed < 4000); advance(1); }
					CHECK(IsInsideLaunchCenterDropZone(miner->position, anchor)); CHECK(miner->position != anchor); CHECK(miner->cargo.TotalCount() == 10); CHECK(CaptureSpaceDescents().empty());
				} else {
					REQUIRE(phase == "unload"); LaunchCenterRef foreign;
					// The genuine retained world contains an actually paid foreign overlapping center; only explicit owned identities can authorize packet recovery.
					for (const auto ref : GetLaunchCenterReferences()) if (ResolveLaunchCenter(ref)->owner != miner->owner) { foreign = ref; break; }
					REQUIRE(foreign.generation != 0); const auto point = miner->position;
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
					const auto digest = ComputeSpaceVehicleDigest();
					// The public edit command rejects a foreign target before queue installation; test that stronger guard without manufacturing an invalid active order.
					CHECK(CmdEditSpaceOrder({}, miner->index, miner->generation, SpaceOrderEdit::Insert, 0, SpaceOrderAction::Unload, SpaceOrderTargetKind::LaunchCenter, foreign.station.base(), foreign.generation).Failed());
					CHECK_FALSE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Unload, SpaceOrderTargetKind::LaunchCenter, uint32_t{foreign.station.base()}, foreign.generation));
					CHECK(ComputeSpaceVehicleDigest() == digest); advance(3);
					CHECK(miner->space_orders.entries.empty()); CHECK(miner->position == point); CHECK(miner->cargo.TotalCount() == 10); CHECK(CaptureSpaceDescents().empty());
					std::vector<CargoReturnPacketHistory> history; Money share = 0;
					// Copy only immutable provenance from the actual hold; valid native packet merging may change IDs while preserving these histories and totals.
					for (const auto *packet : *miner->cargo.Packets()) { history.push_back({packet->index, packet->GetSource(), packet->GetSourceTile(), packet->GetFirstStation(), packet->GetNextHop(), packet->GetPeriodsInTransit()}); share += packet->GetFeederShare(); }
					set_order(SpaceOrderAction::Unload, home);
					// Ordinary motion crosses the circle and atomically gives the original paid shell custody; storage/service refusal would retain the same loaded order.
					for (uint elapsed = 0; CaptureSpaceDescents().empty(); ++elapsed) { REQUIRE(elapsed < 4000); advance(1); }
					REQUIRE(CaptureSpaceDescents().size() == 1); const auto descent = CaptureSpaceDescents().front(); CHECK(descent.center == home); CHECK(descent.release_point == miner->position);
					CHECK(descent.release_point != anchor); CHECK(IsInsideLaunchCenterDropZone(descent.release_point, anchor)); CHECK(miner->cargo.TotalCount() == 0); CHECK(miner->space_orders.blocked == SpaceOrderBlock::Completed);
					Money received_share = 0; uint received = 0;
					// Every physical descent packet retains an actual old source/routing history and nondecreasing canonical age; totals and feeder shares cannot be cloned or reset.
					for (const auto &[id, hop] : descent.packets) {
						const auto *packet = CargoPacket::Get(id); received += packet->Count(); received_share += packet->GetFeederShare();
						/** @param old Actual pre-release history. @return Whether this surviving original packet retains its exact immutable source/route; equal-source fragments may have different ages. */
						auto matches = [&](const CargoReturnPacketHistory &old) { return old.id == id && old.source.id == packet->GetSource().id && old.source.type == packet->GetSource().type && old.tile == packet->GetSourceTile() && old.first == packet->GetFirstStation() && old.next == hop; };
						const auto old = std::find_if(history.begin(), history.end(), matches); REQUIRE(old != history.end()); CHECK(packet->GetPeriodsInTransit() >= old->age);
					}
					CHECK(received == 10); CHECK(received_share == share); CHECK(center->goods[miner->cargo_type].TotalCount() == 10);
				}
			}
			uint64_t extracted = 0, packets = 0;
			// Natural reserve debit and all physical asteroid packets continue to account for the same twenty original mined units across release and precise Visit.
			for (const auto &body : GetAsteroidField().bodies) extracted += body.initial_reserve - body.remaining_reserve;
			for (const auto *packet : CargoPacket::Iterate()) if (packet->GetSource().type == SourceType::Asteroid) packets += packet->Count();
			CHECK(extracted == 20); CHECK(packets == 20); REQUIRE(ValidateSpaceVehicles());
			AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / ("cargo-return-" + phase + "-423.sav")), false) == SaveLoadResult::Ok);
			fmt::print("CARGO_RETURN native=true phase={} actual_navigation=true explicit_target=true conserved=20 no_cargo_income=true\n", phase);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(return_observer, &observe); IConsole::CmdRegister("cargo_return_test", ObserveCargoReturn);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
