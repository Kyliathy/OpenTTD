/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_collection_native.cpp Paid multi-cargo returns and actual boundary train/lorry handling checkpoints.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group19.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_transition.h"
#include "../multilayer_space_content.h"
#include "../asteroid.h"
#include "../station_base.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include "../newgrf_station.h"
#include "../newgrf_roadstop.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../settings_type.h"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include <filesystem>
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *collection_observer = nullptr; ///< Loaded-world observer only; native commands, inventory and simulation own all gameplay.
static constexpr std::string_view BOUNDARY_RAIL_NAME = "Space boundary rail", BOUNDARY_LORRY_NAME = "Space boundary lorry";

/** @param args Parameter-free native callback. @return Whether the installed observer ran. */
static bool ObserveCollection(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(collection_observer != nullptr); (*collection_observer)(); return true;
}

/** @param model Canonical original paid role. @return Its unique actual native hull, never a newly fabricated replacement. */
static SpaceVehicle *CollectionCraft(uint16_t model)
{
	SpaceVehicle *result = nullptr;
	// Each role has exactly one original material-paid hull in the immutable supplied world; a replacement would invalidate discovery.
	for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == model) { REQUIRE(result == nullptr); result = craft; }
	REQUIRE(result != nullptr); return result;
}

/** @param name Saved cosmetic station label. @return Its unique actual Station pool owner, without a secondary identity registry. */
static Station *CollectionMember(std::string_view name)
{
	Station *result = nullptr;
	for (auto *station : Station::Iterate()) if (station->name == name) { REQUIRE(result == nullptr); result = station; } // Only the saved ordinary owner can satisfy a stage's actual member identity.
	REQUIRE(result != nullptr); return result;
}

/** Build a genuine separate single-tile freight member near the fixed circular boundary.
 * @param center Original owned receiving launch facility.
 * @param lorry Whether this member is a road loading bay instead of a rail platform.
 * @return The native newly built, named and explicitly linked station.
 */
static Station *BuildBoundaryMember(Station &center, bool lorry)
{
	const auto anchor = *GetLaunchCenterPlaneReference(center.launch_center->tile); const auto name = lorry ? BOUNDARY_LORRY_NAME : BOUNDARY_RAIL_NAME;
	// Search only real Surface positions at19-20 tiles of horizontal displacement. Whole-footprint range, terrain, ownership and normal station spread remain native placement/eligibility decisions.
	for (int dx : {20, 19}) for (int dy : {0, 1, 2, 3, 4}) {
		const int x = TileX(center.launch_center->tile) + dx, y = TilePlaneY(center.launch_center->tile) + dy;
		if (x + 1 >= static_cast<int>(Map::SizeX()) || y + 1 >= static_cast<int>(Map::SizeY())) continue;
		const auto tile = TileXYL(x, y, MapLayer::Surface); const auto point = *GetLaunchCenterPlaneReference(tile);
		if (!IsInsideLaunchCenterCollectionArea(point, anchor)) continue;
		const auto flags = CommandFlagsToDCFlags(lorry ? GetCommandFlags<Commands::BuildRoadStop>() : GetCommandFlags<Commands::BuildRailStation>());
		const auto quote = lorry ? CmdBuildRoadStop(flags, tile, 1, 1, RoadStopType::Truck, false, DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, true) :
			CmdBuildRailStation(flags, tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, true);
		if (quote.Failed()) continue;
		if (!lorry) {
			bool expandable = false;
			// Preflight a real outward adjoining tile before selecting this boundary platform. The saved plateau's first available corner is boxed in by slopes/another station; a naturally buildable expansion must remain available for the active-assignment lifecycle test.
			for (const auto direction : {DiagDirection::SW, DiagDirection::SE, DiagDirection::NW, DiagDirection::NE}) {
				const auto next = TileAddByDiagDir(tile, direction);
				if (IsInsideLaunchCenterCollectionArea(*GetLaunchCenterPlaneReference(next), anchor)) continue;
				if (CmdBuildRailStation(flags, next, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, true).Succeeded()) { expandable = true; break; }
			}
			if (!expandable) continue;
		}
		if (lorry) REQUIRE(Command<Commands::BuildRoadStop>::Post(tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck, false, DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, NEW_STATION, true));
		else REQUIRE(Command<Commands::BuildRailStation>::Post(tile, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, true));
		auto *member = Station::GetByTile(tile); REQUIRE(member != nullptr); REQUIRE(member != &center); REQUIRE(IsLaunchHubEligible(center, *member));
		REQUIRE(Command<Commands::RenameStation>::Post(member->index, std::string{name})); const auto home = GetLaunchCenterReference(center);
		REQUIRE(Command<Commands::SetLaunchHub>::Post(member->index, home.station, home.generation)); CHECK(member->spread.w == 1); CHECK(member->spread.h == 1); return member;
	}
	FAIL("No genuine Surface freight position can be built near the collection boundary"); return nullptr;
}

/** Recover actual mined ore and the original paid builder's declared Goods through real purchased shells, then observe bounded native handling. */
TEST_CASE("Paid multi-cargo returns reach separate native train and lorry members near the Surface boundary", "[.surface-collection]")
{
	const char *input = std::getenv("OTTD_COLLECTION_SOURCE"), *mode = std::getenv("OTTD_COLLECTION_STAGE"); REQUIRE(input != nullptr); REQUIRE(mode != nullptr); const std::string stage{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "surface-collection"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Advance only the ordinary loaded paid world and propagate native assertions through orderly shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AutoRestoreBackup running(_pause_mode, PauseModes{}); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			auto *rocket = CollectionCraft(14), *miner = CollectionCraft(15), *builder = CollectionCraft(20); const auto fleet = GetSpaceVehicleCount();
			const auto rocket_ref = GetSpaceCraftReference(*rocket), miner_ref = GetSpaceCraftReference(*miner), builder_ref = GetSpaceCraftReference(*builder);
			const auto home = rocket->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr); const auto anchor = *GetLaunchCenterPlaneReference(center->launch_center->tile);
			const auto ore = GetCargoTypeByLabel(CargoLabel{"NORE"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}); REQUIRE(IsValidCargoType(ore)); REQUIRE(IsValidCargoType(goods));
			const auto spread = _settings_game.station.station_spread; auto *company = Company::Get(rocket->owner); const auto ore_deliveries = company->cur_economy.delivered_cargo[ore], goods_deliveries = company->cur_economy.delivered_cargo[goods];
			/** @param suffix Exact stage boundary; the normal writer owns packets/references/finance. */
			auto save = [&](std::string_view suffix) {
				REQUIRE(ValidateSpaceVehicles()); REQUIRE(ValidateSurfaceHandling()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("collection-{}-{}-423.sav", stage, suffix)), false) == SaveLoadResult::Ok);
			};
			/** @param done Actual completion/current-position predicate. @param limit Maximum synchronized opportunities. @param reason Native operation for diagnostics. */
			auto advance = [&](const std::function<bool()> &done, uint limit, std::string_view reason) {
				const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(70); uint ticks = 0;
				// Native flight, descent, physical navigation and handling get their genuine deterministic opportunities; render/stalled calls earn no movement, inventory or fees.
				while (!done()) { INFO(reason); REQUIRE(ticks < limit); REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; } REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++ticks;
				}
			};
			/** @param member Actual linked recipient. @param cargo Physical recovered cargo. @param quantity Positive actual available units to reserve. */
			auto assign = [&](Station &member, CargoType cargo, uint16_t quantity) {
				const auto link = GetLaunchHubMembership(member.index); REQUIRE(link); REQUIRE(link->center == home);
				REQUIRE(Command<Commands::StartSurfaceHandling>::Post(member.index, link->member.generation, home.station, home.generation, cargo, quantity, false));
			};
			save("before");
			if (stage == "release") {
				REQUIRE(builder->cargo_type == goods); REQUIRE(builder->cargo.TotalCount() == 40); REQUIRE(builder->construction.placement == SpacePlacement::SurfaceStored); REQUIRE(miner->cargo.TotalCount() == 10);
				Station *historical = nullptr;
				// Settle the center's actual historical Goods to its existing separate train member before any new Goods recovery; retained saves have different inventories and those inputs must never count as newly descended freight.
				for (const auto ref : center->launch_hub->members) { auto *member = ResolveLaunchHubMember(home, ref); if (member != nullptr && member != center && member->facilities.Test(StationFacility::Train)) { REQUIRE(historical == nullptr); historical = member; } }
				REQUIRE(historical != nullptr);
				while (center->goods[goods].TotalCount() != 0) {
					const auto quantity = static_cast<uint16_t>(std::min<uint>(MAX_SURFACE_HANDLING_QUANTITY, center->goods[goods].TotalCount()));
					assign(*historical, goods, quantity); advance([&] { return !HasSurfaceHandlingForStation(historical->index); }, 100, "historical Goods staging");
				}
				REQUIRE(Command<Commands::SetLaunchHub>::Post(historical->index, StationID::Invalid(), uint64_t{0}));
				BuildBoundaryMember(*center, false); BuildBoundaryMember(*center, true); REQUIRE(builder->manifest != nullptr);
				if (builder->manifest->returns_remaining == 0) {
					// Only real paid Earth service can fund the original builder's return allowance; its existing forty-unit payload remains physically aboard during that work.
					for (uint8_t supply : {1, 2}) if (builder->manifest->supplies[supply].TotalCount() == 0) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, builder->index, builder->generation, supply, uint16_t{1}, false));
					REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(builder->index, builder->generation, false)); advance([&] { return builder->manifest->service_ticks == 0; }, 200, "paid original builder service");
				}
				SpacePodRef goods_shell{};
				// Select a genuine previously purchased stored shell through command admission, never synthesize a pod or its material/capital receipt.
				for (auto *pod : SpacePod::Iterate()) if (CmdMoveLaunchPod({}, home.station, home.generation, builder->index, builder->generation, pod->index, pod->generation, false).Succeeded()) { goods_shell = GetSpacePodReference(*pod); break; }
				REQUIRE(goods_shell != SpacePodRef{}); REQUIRE(Command<Commands::MoveLaunchPod>::Post(home.station, home.generation, builder->index, builder->generation, goods_shell.pod, goods_shell.generation, false));
				const auto fuel = static_cast<uint16_t>(GetMultiLayerSpaceBudget().outbound_fuel + GetMultiLayerSpaceBudget().recovery_fuel);
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0}, fuel, false)); REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, builder->index, builder->generation, false));
				const auto quote = CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation); REQUIRE(quote.Succeeded()); const auto cash = company->money;
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation)); CHECK(company->money == cash - quote.GetCost());
				advance([&] { return !IsSpaceFlightActive(*rocket); }, 800, "actual paid loaded-builder deployment"); REQUIRE(builder->construction.placement == SpacePlacement::Orbital);
				REQUIRE(Command<Commands::ReleaseOrbitalCargo>::Post(builder->index, builder->generation, home.station, home.generation, goods_shell.pod, goods_shell.generation, uint16_t{40}));
				const AsteroidBody *destination = nullptr;
				// Ordinary Visit moves the original mined load to a real off-center release point while the newly released Goods descend independently.
				for (const auto &body : GetAsteroidField().bodies) {
					if (body.mining_slot != VehicleID::Invalid() || body.survey_slot != INVALID_SPACE_SURVEY) continue;
					if (SpaceDistanceSquared(body.motion.position, anchor) > uint64_t{24 * SPACE_Q16} * (24 * SPACE_Q16)) { destination = &body; break; }
				}
				REQUIRE(destination != nullptr); REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, destination->id.base(), uint64_t{0}));
				advance([&] { return SpaceDistanceSquared(miner->position, anchor) >= uint64_t{2 * SPACE_Q16} * (2 * SPACE_Q16); }, 200, "actual off-center mined-cargo movement"); REQUIRE(IsInsideLaunchCenterDropZone(miner->position, anchor)); REQUIRE(miner->manifest->pods.size() == 1);
				const auto ore_shell = miner->manifest->pods.front(); REQUIRE(Command<Commands::ReleaseOrbitalCargo>::Post(miner->index, miner->generation, home.station, home.generation, ore_shell.pod, ore_shell.generation, uint16_t{10}));
				REQUIRE(ResolveSpacePod(ore_shell)->descent != nullptr); CHECK(ResolveSpacePod(ore_shell)->descent->release_point != anchor); CHECK(miner->cargo.TotalCount() == 0); CHECK(builder->cargo.TotalCount() == 0);
			} else {
				auto *rail = CollectionMember(BOUNDARY_RAIL_NAME), *lorry = CollectionMember(BOUNDARY_LORRY_NAME);
				if (stage == "handling") {
					advance([&] { return !HasSpaceDescentForCenter(home); }, 300, "actual multi-cargo descent completion"); REQUIRE(center->goods[ore].TotalCount() == 20); REQUIRE(center->goods[goods].TotalCount() == 40);
					for (auto *member : {rail, lorry}) { assign(*member, ore, 10); assign(*member, goods, 20); } // Each explicit member receives its own whole reservations; no packet can be claimed by both destinations.
					const auto before = GetAsteroidField().elapsed_ticks; advance([&] { return GetAsteroidField().elapsed_ticks > before; }, 2, "one actual partial handling opportunity"); REQUIRE(HasSurfaceHandlingForStation(rail->index)); REQUIRE(HasSurfaceHandlingForStation(lorry->index));
				} else {
					REQUIRE(HasSurfaceHandlingForStation(rail->index)); REQUIRE(HasSurfaceHandlingForStation(lorry->index));
					if (stage == "unlink") {
						const auto digest = ComputeSurfaceHandlingDigest(); const auto cash = company->money;
						CHECK(CmdSetLaunchHub({}, rail->index, StationID::Invalid(), 0).Failed()); CHECK_FALSE(Command<Commands::SetLaunchHub>::Post(rail->index, StationID::Invalid(), uint64_t{0}));
						CHECK(ComputeSurfaceHandlingDigest() == digest); CHECK(company->money == cash); // Deliberate unlink refuses active assignments before changing a link or its sole packet owner.
					} else if (stage == "expand") {
						TileIndex outside = INVALID_TILE;
						// Range-changing expansion may use either an extra platform or an extra track. Select only a real adjoining out-of-circle tile admitted by normal station commands; the retained plateau need not support every direction.
						for (const auto direction : {DiagDirection::SW, DiagDirection::SE, DiagDirection::NW, DiagDirection::NE}) {
							const auto candidate = TileAddByDiagDir(rail->xy, direction);
							if (IsInsideLaunchCenterCollectionArea(*GetLaunchCenterPlaneReference(candidate), anchor)) continue;
							const auto quote = CmdBuildRailStation(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRailStation>()), candidate, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, rail->index, true);
							fmt::print("COLLECTION_EXPANSION quote_tile={} error={}\n", candidate.base(), quote.GetErrorMessage().base());
							if (quote.Succeeded()) { outside = candidate; break; }
						}
						REQUIRE(outside != INVALID_TILE);
						// Explicit rail selection permits its neighboring separate lorry station while retaining the requested native owner; no automatic join may choose a different station for this expansion.
						REQUIRE(Command<Commands::BuildRailStation>::Post(outside, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, rail->index, true));
						CHECK_FALSE(GetLaunchHubMembership(rail->index)); CHECK_FALSE(HasSurfaceHandlingForStation(rail->index)); // Real station expansion settles the uncompleted rail assignments to the retained center before retiring its out-of-range membership.
					} else if (stage == "delete") {
						REQUIRE(lorry->goods[ore].TotalCount() == 0); REQUIRE(lorry->goods[goods].TotalCount() == 0);
						REQUIRE(Command<Commands::RemoveRoadStop>::Post(lorry->xy, uint8_t{1}, uint8_t{1}, RoadStopType::Truck, false));
						CHECK_FALSE(GetLaunchHubMembership(lorry->index)); CHECK_FALSE(HasSurfaceHandlingForStation(lorry->index)); // Removing an empty but assigned native bay returns its pending packets to the center; the surviving rail job keeps its own packets.
					} else REQUIRE(stage == "settle");
					advance([&] { return !HasSurfaceHandlingForStation(center->index); }, 100, "actual boundary collection settlement");
					if (stage == "expand") {
						CHECK(rail->goods[ore].TotalCount() == 8); CHECK(rail->goods[goods].TotalCount() == 0); CHECK(lorry->goods[ore].TotalCount() == 10); CHECK(lorry->goods[goods].TotalCount() == 20);
						CHECK(center->goods[ore].TotalCount() == 2); CHECK(center->goods[goods].TotalCount() == 20);
					} else if (stage == "delete") {
						CHECK(rail->goods[ore].TotalCount() == 10); CHECK(rail->goods[goods].TotalCount() == 20); CHECK(center->goods[ore].TotalCount() == 10); CHECK(center->goods[goods].TotalCount() == 20);
					} else {
						for (auto *member : {rail, lorry}) { CHECK(member->goods[ore].TotalCount() == 10); CHECK(member->goods[goods].TotalCount() == 20); CHECK(IsLaunchHubEligible(*center, *member)); } // Both real separate owners retain the quantities released by the original paid bodies.
						CHECK(center->goods[ore].TotalCount() == 0); CHECK(center->goods[goods].TotalCount() == 0);
						if (stage == "unlink") { REQUIRE(Command<Commands::SetLaunchHub>::Post(rail->index, StationID::Invalid(), uint64_t{0})); CHECK_FALSE(GetLaunchHubMembership(rail->index)); CHECK(rail->goods[ore].TotalCount() == 10); CHECK(rail->goods[goods].TotalCount() == 20); }
					}
				}
			}
			CHECK(GetSpaceCraftReference(*rocket) == rocket_ref); CHECK(GetSpaceCraftReference(*miner) == miner_ref); CHECK(GetSpaceCraftReference(*builder) == builder_ref);
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(_settings_game.station.station_spread == spread); CHECK(company->cur_economy.delivered_cargo[ore] == ore_deliveries); CHECK(company->cur_economy.delivered_cargo[goods] == goods_deliveries); save("after");
			fmt::print("SURFACE_COLLECTION native=true stage={} ore=20 goods=40 original_paid_hulls=true separate_boundary_members=true normal_spread=true\n", stage);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(collection_observer, &observe); IConsole::CmdRegister("surface_collection_test", ObserveCollection);
	const std::string source{input}, configuration = config.string(); std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
