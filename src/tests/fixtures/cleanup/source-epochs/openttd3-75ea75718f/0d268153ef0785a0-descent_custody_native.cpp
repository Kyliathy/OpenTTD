/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file descent_custody_native.cpp Bounded actual paid-pod release/completion/service checkpoints; no synthetic cargo or center retirement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group13.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../asteroid.h"
#include "../station_base.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../water_cmd.h"
#include "../landscape.h"
#include "../multilayer_space_content.h"
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

static std::function<void()> *custody_observer = nullptr; ///< Observer only; actual commands/ticks own all work and packets.

/** @param args Parameter-free loaded-world console callback. @return Whether the installed observer ran. */
static bool ObserveDescentCustody(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(custody_observer != nullptr); (*custody_observer)(); return true;
}

/** Build a genuinely paid alternate recovery facility with its ordinary station/command owner.
 * @param point Original saved release point, which must lie inside the new center's circle.
 * @return Actual newly constructed same-company center, never a synthetic station identity.
 */
static LaunchCenterRef BuildAlternateRecoveryCenter(SpacePoint point)
{
	// Prefer existing waterside terrain, then buy canal access at a flat empty footprint; both passes remain normal paid construction transactions.
	for (uint pass = 0; pass < 2; ++pass) {
		// Search actual bounded Surface sites/rotations inside this release's circle; gameplay owns placement and rejects occupied or nonflat terrain.
		for (uint y = 1; y + 4 < Map::SizeY(); ++y) for (uint x = 1; x + 4 < Map::SizeX(); ++x) for (uint8_t rotation = 0; rotation < 4; ++rotation) {
			if (!IsInsideLaunchCenterDropZone(point, {static_cast<int32_t>(x * SPACE_Q16), static_cast<int32_t>(y * SPACE_Q16)})) continue;
			const auto tile = TileXYL(x, y, MapLayer::Surface); const auto area = GetLaunchCenterFootprint(tile, rotation); REQUIRE(area);
			// A second pass may buy the missing water edge only at an otherwise usable empty footprint; it cannot clear a paid existing facility.
			if (pass == 1 && !HasLaunchCenterWaterAccess(*area)) {
				bool flat_empty = true;
				// Inspect the complete land footprint before paying for access, keeping ordinary clear/flat terrain requirements intact.
				for (const auto cell : *area) {
					if ((!IsTileType(cell, TileType::Clear) && !IsTileType(cell, TileType::Trees)) || !IsTileFlat(cell) || GetTileZ(cell) != GetTileZ(tile)) flat_empty = false;
				}
				if (!flat_empty) continue;
				bool canal = false;
				// One paid orthogonal canal edge is sufficient; every candidate is clipped before converting to a Surface tile.
				for (uint side = 0; side < 4 && !canal; ++side) for (uint offset = 0; offset < (side < 2 ? area->h : area->w) && !canal; ++offset) {
					const int wx = static_cast<int>(x) + (side == 0 ? -1 : side == 1 ? area->w : static_cast<int>(offset));
					const int wy = static_cast<int>(y) + (side == 2 ? -1 : side == 3 ? area->h : static_cast<int>(offset));
					// Clipping and ordinary landscape ownership jointly prevent wrapped water or clearing an existing structure.
					if (wx < 1 || wy < 1 || wx + 1 >= static_cast<int>(Map::SizeX()) || wy + 1 >= static_cast<int>(Map::SizeY())) continue;
					const auto water = TileXYL(wx, wy, MapLayer::Surface);
					// Canal construction may affect only actual clear/tree land whose normal command currently admits the paid transaction.
					if ((!IsTileType(water, TileType::Clear) && !IsTileType(water, TileType::Trees)) || CmdBuildCanal({}, water, water, WaterClass::Canal, false).Failed()) continue;
					REQUIRE(Command<Commands::BuildCanal>::Post(water, water, WaterClass::Canal, false)); canal = true;
				}
				if (!canal) continue;
			}
			if (CmdBuildLaunchCenter({}, tile, rotation, NEW_STATION, true).Failed()) continue;
			REQUIRE(Command<Commands::BuildLaunchCenter>::Post(tile, rotation, NEW_STATION, true));
			return GetLaunchCenterReference(*Station::GetByTile(tile));
		}
	}
	FAIL("No ordinary paid alternate recovery center could be constructed"); return {};
}

/** Execute one explicit saved phase of the original mined cargo's two paid returns and funded onboard service. */
TEST_CASE("Paid descent custody survives immediate completion and service reload boundaries", "[.descent-custody]")
{
	const char *input = std::getenv("OTTD_DESCENT_CUSTODY_SOURCE"), *phase_env = std::getenv("OTTD_DESCENT_CUSTODY_PHASE");
	REQUIRE(input != nullptr); REQUIRE(phase_env != nullptr); const std::string phase{phase_env};
	const auto paths = GetTestCaseArtifactPaths("space", "descent-custody"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Use the ordinary loaded paid predecessor and propagate assertions after native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			SpaceVehicle *miner = nullptr;
			// Each predecessor retains the one original paid miner and genuine extracted cargo; no hull is commissioned by this observer.
			for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == 15) { REQUIRE(miner == nullptr); miner = v; }
			REQUIRE(miner != nullptr); REQUIRE(miner->manifest != nullptr); auto &manifest = *miner->manifest;
			const auto home = miner->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr);
			/** @param name Exact current phase filename, written only through the native save writer. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			/** @param ticks Actual synchronized native opportunities; also verify the original miner's published upkeep without freight income. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Count authoritative opportunities, checking expense/remainder each tick so reload cannot repeat service cost or create cargo revenue.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks;
					const auto profit = miner->profit_this_year; const uint accrual = miner->mission.cost_remainder + (IsSpaceServiceWorkTick(*miner) ? 500 : 100);
					StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); CHECK(miner->profit_this_year == profit - Money{accrual / 74} * 256);
					CHECK(miner->mission.cost_remainder == accrual % 74); ++done;
				}
			};
			const auto cargo = miner->cargo_type;
			if (phase == "release") {
				REQUIRE(CaptureSpaceDescents().size() == 1); REQUIRE(CaptureSpaceDescents().front().remaining_ticks == 111);
				REQUIRE(manifest.pods.size() == 1); REQUIRE(miner->cargo.TotalCount() == 10); const auto shell = manifest.pods.front();
				save("descent-release-before-423.sav"); const auto cash = Company::Get(_current_company)->money; const auto profit = miner->profit_this_year;
				const auto next = GetNextSpaceDescentID(); const auto headroom = GetLaunchStockHeadroom(*center, cargo); const auto point = miner->position;
				REQUIRE(Command<Commands::ReleaseOrbitalCargo>::Post(miner->index, miner->generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{10}));
				CHECK(miner->cargo.TotalCount() == 0); CHECK(manifest.pods.empty()); CHECK(manifest.pending_returns == 2); CHECK(manifest.returns_remaining == 2);
				CHECK(GetNextSpaceDescentID() == next + 1); CHECK(GetIncomingSpaceDescent(home, cargo) == 20); CHECK(GetLaunchStockHeadroom(*center, cargo) == headroom - 10);
				REQUIRE(ResolveSpacePod(shell) != nullptr); const auto &job = *ResolveSpacePod(shell)->descent;
				CHECK(job.release_point == point); CHECK(job.remaining_ticks == 148); CHECK(job.quantity == 10); CHECK(job.center == home);
				const auto digest = ComputeSpaceVehicleDigest();
				CHECK(CmdReleaseOrbitalCargo({DoCommandFlag::Execute}, miner->index, miner->generation, home.station, home.generation, shell.pod, shell.generation, 10).Failed());
				CHECK(ComputeSpaceVehicleDigest() == digest); CHECK(Company::Get(_current_company)->money == cash); CHECK(miner->profit_this_year == profit);
				CHECK(CmdServiceLaunchProbe({}, miner->index, miner->generation, false).Failed()); CHECK(CmdRemoveLaunchCenter({}, center->launch_center->tile).Failed());
			} else if (phase == "before-first") {
				REQUIRE(CaptureSpaceDescents().size() == 2); REQUIRE(CaptureSpaceDescents().front().remaining_ticks == 111); advance(110);
				CHECK(CaptureSpaceDescents().front().remaining_ticks == 1); CHECK(manifest.pending_returns == 2); CHECK(center->goods[cargo].TotalCount() == 0);
			} else if (phase == "first") {
				REQUIRE(CaptureSpaceDescents().size() == 2); const auto first = CaptureSpaceDescents().front(); REQUIRE(first.remaining_ticks == 1); advance(1);
				CHECK(ResolveSpacePod(first.pod) == nullptr); REQUIRE(CaptureSpaceDescents().size() == 1); CHECK(CaptureSpaceDescents().front().remaining_ticks == 37);
				CHECK(manifest.pending_returns == 1); CHECK(manifest.returns_remaining == 1); CHECK(center->goods[cargo].TotalCount() == 10); CHECK(GetIncomingSpaceDescent(home, cargo) == 10);
				CHECK(CmdServiceLaunchProbe({}, miner->index, miner->generation, false).Failed());
			} else if (phase == "before-second") {
				REQUIRE(CaptureSpaceDescents().size() == 1); REQUIRE(CaptureSpaceDescents().front().remaining_ticks == 37); advance(36);
				CHECK(CaptureSpaceDescents().front().remaining_ticks == 1); CHECK(manifest.pending_returns == 1); CHECK(center->goods[cargo].TotalCount() == 10);
			} else if (phase == "second") {
				REQUIRE(CaptureSpaceDescents().size() == 1); const auto second = CaptureSpaceDescents().front(); REQUIRE(second.remaining_ticks == 1); advance(1);
				CHECK(ResolveSpacePod(second.pod) == nullptr); CHECK(CaptureSpaceDescents().empty()); CHECK(manifest.pending_returns == 0); CHECK(manifest.returns_remaining == 0);
				CHECK(center->goods[cargo].TotalCount() == 20); CHECK(GetIncomingSpaceDescent(home, cargo) == 0); CHECK(SpacePod::GetNumItems() == 8);
				const auto digest = ComputeSpaceVehicleDigest(); CHECK(CmdRetargetSpaceDescent({}, second.pod.pod, second.pod.generation, home.station, home.generation).Failed()); CHECK(ComputeSpaceVehicleDigest() == digest);
			} else if (phase == "service") {
				REQUIRE(CaptureSpaceDescents().empty()); REQUIRE(manifest.returns_remaining == 0); REQUIRE(manifest.supplies[1].TotalCount() == 1); REQUIRE(manifest.supplies[2].TotalCount() == 1);
				const auto cash = Company::Get(_current_company)->money; REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(miner->index, miner->generation, false));
				CHECK(Company::Get(_current_company)->money == cash); REQUIRE(manifest.service_ticks == 148); advance(37);
				CHECK(manifest.service_ticks == 111); CHECK(manifest.services_completed == 0); CHECK(manifest.supplies[1].TotalCount() == 1); CHECK(manifest.supplies[2].TotalCount() == 1);
			} else if (phase == "before-service") {
				REQUIRE(manifest.service_ticks == 111); advance(110); CHECK(manifest.service_ticks == 1); CHECK(manifest.services_completed == 0);
				CHECK(manifest.supplies[1].TotalCount() == 1); CHECK(manifest.supplies[2].TotalCount() == 1);
			} else if (phase == "retarget") {
				REQUIRE(CaptureSpaceDescents().size() == 1); const auto before = CaptureSpaceDescents().front(); REQUIRE(before.remaining_ticks == 111);
				REQUIRE(miner->cargo.TotalCount() == 10); const auto alternate = BuildAlternateRecoveryCenter(before.release_point); REQUIRE(alternate != home);
				auto *receiver = ResolveLaunchCenter(alternate); REQUIRE(receiver != nullptr); REQUIRE(receiver->owner == miner->owner);
				CHECK(GetIncomingSpaceDescent(home, cargo) == 10); CHECK(GetIncomingSpaceDescent(alternate, cargo) == 0);
				const auto digest = ComputeSpaceVehicleDigest(); const auto cash = Company::Get(_current_company)->money;
				CHECK(CmdRetargetSpaceDescent({}, before.pod.pod, before.pod.generation, alternate.station, alternate.generation + 100).Failed()); CHECK(ComputeSpaceVehicleDigest() == digest);
				REQUIRE(Command<Commands::RetargetSpaceDescent>::Post(before.pod.pod, before.pod.generation, alternate.station, alternate.generation)); CHECK(Company::Get(_current_company)->money == cash);
				const auto after = CaptureSpaceDescents().front(); CHECK(after.id == before.id); CHECK(after.packets == before.packets); CHECK(after.remaining_ticks == before.remaining_ticks); CHECK(after.center == alternate);
				CHECK(GetIncomingSpaceDescent(home, cargo) == 0); CHECK(GetIncomingSpaceDescent(alternate, cargo) == 10); CHECK(CmdRemoveLaunchCenter({}, receiver->launch_center->tile).Failed());
				save("descent-retarget-before-423.sav"); advance(110); CHECK(ResolveSpacePod(before.pod)->descent->remaining_ticks == 1); save("descent-retarget-last-423.sav"); advance(1);
				CHECK(ResolveSpacePod(before.pod) == nullptr); CHECK(center->goods[cargo].TotalCount() == 0); CHECK(receiver->goods[cargo].TotalCount() == 10); CHECK(miner->cargo.TotalCount() == 10);
				CHECK(manifest.pending_returns == 0); CHECK(manifest.returns_remaining == 1); CHECK(GetIncomingSpaceDescent(alternate, cargo) == 0);
			} else if (phase == "replacement") {
				REQUIRE(manifest.services_completed == 1); REQUIRE(CaptureSpaceDescents().empty()); SpaceVehicle *rocket = nullptr; SpacePod *stored = nullptr;
				// Consume only existing paid hull/shell identities from this genuine completed predecessor; no replacement is granted by the fixture.
				for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == 14) { REQUIRE(rocket == nullptr); rocket = v; }
				for (auto *pod : SpacePod::Iterate()) if (pod->state.home == home && pod->state.custody == SpacePodCustody::CenterStored) { stored = pod; break; }
				REQUIRE(rocket != nullptr); REQUIRE(stored != nullptr); REQUIRE(Command<Commands::MoveLaunchPod>::Post(home.station, home.generation, rocket->index, rocket->generation, stored->index, stored->generation, false));
				const auto &definition = *GetSpaceConstructionDefinition(SpaceBuildKind::Pod); const auto next = GetNextSpacePodGeneration(); const auto cash = Company::Get(_current_company)->money;
				std::vector<std::pair<CargoType, uint>> inputs;
				// Record each published material debit from actual Surface stock before the single synchronized workshop purchase.
				for (const auto &input_material : definition.inputs) { const auto type = GetCargoTypeByLabel(input_material.label); inputs.push_back({type, center->goods[type].TotalCount()}); }
				REQUIRE(Command<Commands::AssembleLaunchPod>::Post(home.station, home.generation)); const auto replacement = center->launch_operations->pod_workshop;
				REQUIRE(replacement != SpacePodRef{}); REQUIRE(ResolveSpacePod(replacement) != nullptr); CHECK(replacement.generation == next); CHECK(replacement.pod == SpacePodID{0});
				CHECK(Company::Get(_current_company)->money == cash - Money{definition.capital}); CHECK(ResolveSpacePod(replacement)->state.capital_paid == definition.capital);
				// Each recipe input disappears once at purchase; an occupied workshop cannot charge a second order or consume further stock.
				for (uint i = 0; i < inputs.size(); ++i) CHECK(center->goods[inputs[i].first].TotalCount() == inputs[i].second - definition.inputs[i].quantity);
				const auto paid_cash = Company::Get(_current_company)->money; CHECK(CmdAssembleLaunchPod({DoCommandFlag::Execute}, home.station, home.generation).Failed()); CHECK(Company::Get(_current_company)->money == paid_cash);
				CHECK(ResolveSpacePod({replacement.pod, uint64_t{1}}) == nullptr); CHECK(CmdRetargetSpaceDescent({}, replacement.pod, uint64_t{1}, home.station, home.generation).Failed());
				save("descent-replacement-order-423.sav"); advance(definition.work_ticks); REQUIRE(ResolveSpacePod(replacement) != nullptr);
				CHECK(ResolveSpacePod(replacement)->state.custody == SpacePodCustody::CenterStored); CHECK(ResolveSpacePod(replacement)->state.capital_paid == definition.capital); CHECK(center->launch_operations->pod_workshop == SpacePodRef{});
			} else {
				REQUIRE(phase == "complete-service"); REQUIRE(manifest.service_ticks == 1); advance(1);
				CHECK(manifest.service_ticks == 0); CHECK(manifest.services_completed == 1); CHECK(manifest.returns_remaining == 2); CHECK(manifest.pending_returns == 0);
				CHECK(manifest.supplies[1].TotalCount() == 0); CHECK(manifest.supplies[2].TotalCount() == 0); CHECK(CmdServiceLaunchProbe({}, miner->index, miner->generation, false).Failed());
				advance(3); CHECK(manifest.services_completed == 1); CHECK(center->goods[cargo].TotalCount() == 20);
			}
			uint64_t extracted = 0, packets = 0;
			// Finite asteroid debit and all physical asteroid packets still total the original twenty units across every immediate completion/service checkpoint.
			for (const auto &body : GetAsteroidField().bodies) extracted += body.initial_reserve - body.remaining_reserve;
			for (const auto *packet : CargoPacket::Iterate()) if (packet->GetSource().type == SourceType::Asteroid) packets += packet->Count();
			CHECK(extracted == 20); CHECK(packets == 20); save("descent-" + phase + "-423.sav");
			fmt::print("DESCENT_CUSTODY native=true phase={} conserved=20 immediate_boundary=true service_expense_exact=true\n", phase);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(custody_observer, &observe); IConsole::CmdRegister("descent_custody_test", ObserveDescentCustody);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
