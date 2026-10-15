/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_release_geometry.cpp Exact release-circle, clipped transport and current-command-order acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group18.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_orders.h"
#include "../multilayer_space_content.h"
#include "../asteroid.h"
#include "../station_base.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include <filesystem>
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

/** @param point Explicit isolated Q16 transport boundary, never a fabricated native vehicle pose.
 * @param target Exact isolated recovery reference. @return Copied transport row for pure geometry/schema admission only.
 */
static SpaceDescentRecord GeometryDescent(SpacePoint point, LaunchCenterRef target)
{
	SpaceDescentRecord row; row.pod = {SpacePodID{3}, 9}; row.id = 1; row.center = target;
	row.sender = VehicleID{2}; row.sender_generation = 7; row.cargo = CargoType{0}; row.quantity = 10;
	row.remaining_ticks = GetMultiLayerSpaceBudget().descent_ticks; row.release_point = point;
	row.packets.push_back({CargoPacketID{5}, StationID::Invalid()}); return row;
}

/** @param row Explicit copied descent boundary. @param centers Actual proposed immutable anchor records.
 * @return Whether its sole pod/sender/packet reference can bind the explicitly selected center's fixed circle.
 */
static bool GeometryLinks(const SpaceDescentRecord &row, std::span<const LaunchCenterRecord> centers)
{
	SpacePodRecord pod; pod.ref = row.pod; pod.state.custody = SpacePodCustody::Descent; pod.state.position = row.release_point;
	SpaceManifestRecord manifest; manifest.craft = {row.sender, row.sender_generation}; manifest.returns_remaining = 2; manifest.pending_returns = 1;
	return ValidateSpaceDescentLinks(std::array{row}, std::array{pod}, std::array{manifest}, centers, 64);
}

/** Anchor/axis/Pythagorean/Q16 boundaries exercise the same fixed-point predicate used before release and by saved descent target validation. */
TEST_CASE("Orbital release geometry includes its exact circle and refuses the next fixed-point unit", "[orbital-release-geometry]")
{
	const SpacePoint anchor{24 * SPACE_Q16, 24 * SPACE_Q16}; const LaunchCenterRef target{StationID{1}, 4};
	const std::array centers{LaunchCenterRecord{target.station, {TileIndex{24 + 24 * 64}, target.generation, 0}}};
	CHECK(IsInsideLaunchCenterDropZone(anchor, anchor)); CHECK(GeometryLinks(GeometryDescent(anchor, target), centers));
	// Every sign/orientation uses the same inclusive circle; a valid diagonal and its one-Q16-unit outward neighbor cannot silently become a square or Manhattan catchment.
	for (const int sx : {-1, 1}) for (const int sy : {-1, 1}) {
		for (const auto offset : {SpacePoint{20 * SPACE_Q16, 0}, SpacePoint{0, 20 * SPACE_Q16}, SpacePoint{12 * SPACE_Q16, 16 * SPACE_Q16}, SpacePoint{14 * SPACE_Q16, 14 * SPACE_Q16}}) {
			const SpacePoint point{anchor.x + sx * offset.x, anchor.y + sy * offset.y};
			CHECK(IsInsideLaunchCenterDropZone(point, anchor)); CHECK(GeometryLinks(GeometryDescent(point, target), centers));
		}
		for (const auto offset : {SpacePoint{20 * SPACE_Q16 + 1, 0}, SpacePoint{0, 20 * SPACE_Q16 + 1}, SpacePoint{12 * SPACE_Q16, 16 * SPACE_Q16 + 1}, SpacePoint{15 * SPACE_Q16, 15 * SPACE_Q16}}) {
			const SpacePoint point{anchor.x + sx * offset.x, anchor.y + sy * offset.y};
			CHECK_FALSE(IsInsideLaunchCenterDropZone(point, anchor)); CHECK_FALSE(GeometryLinks(GeometryDescent(point, target), centers));
		}
	}
}

/** Map limits clip saved releases without wrapping, and overlap never chooses a different target or enlarges either original circle. */
TEST_CASE("Orbital release transport clips every map edge and binds an explicit overlapping target", "[orbital-release-geometry]")
{
	// Four corners test both signs at each actual map bound. Circle arithmetic may extend beyond the plane, but transport rejects those points before any tile-index wrapping can occur.
	for (const int x : {0, 63}) for (const int y : {0, 63}) {
		const SpacePoint anchor{x * SPACE_Q16, y * SPACE_Q16}; const LaunchCenterRef target{StationID{1}, 4};
		const auto row = GeometryDescent(anchor, target); CHECK(ValidateSpaceDescentRecords(std::array{row}, 2, 64, 64));
		for (const bool horizontal : {false, true}) {
			auto outside = row;
			if (horizontal) outside.release_point.x += x == 0 ? -1 : 1;
			else outside.release_point.y += y == 0 ? -1 : 1;
			CHECK(IsInsideLaunchCenterDropZone(outside.release_point, anchor)); CHECK_FALSE(ValidateSpaceDescentRecords(std::array{outside}, 2, 64, 64));
		}
		const SpacePoint opposite{(63 - x) * SPACE_Q16, y * SPACE_Q16}; CHECK_FALSE(IsInsideLaunchCenterDropZone(opposite, anchor));
	}
	const LaunchCenterRef first{StationID{1}, 4}, second{StationID{2}, 6};
	const std::array centers{LaunchCenterRecord{first.station, {TileIndex{20 + 20 * 64}, first.generation, 0}}, LaunchCenterRecord{second.station, {TileIndex{27 + 20 * 64}, second.generation, 0}}};
	const SpacePoint overlap{24 * SPACE_Q16, 22 * SPACE_Q16};
	CHECK(GeometryLinks(GeometryDescent(overlap, first), centers)); CHECK(GeometryLinks(GeometryDescent(overlap, second), centers));
	CHECK_FALSE(GeometryLinks(GeometryDescent(overlap, {first.station, first.generation + 1}), centers));
	CHECK_FALSE(GeometryLinks(GeometryDescent(overlap, {StationID{3}, first.generation}), centers));
	const SpacePoint first_only{0, 20 * SPACE_Q16}; CHECK(GeometryLinks(GeometryDescent(first_only, first), centers)); CHECK_FALSE(GeometryLinks(GeometryDescent(first_only, second), centers));
}

static std::function<void()> *release_order_observer = nullptr; ///< Only an observer; native movement, commands and paid cargo owners supply the evidence.

/** @param args Parameter-free native callback. @return Whether the installed loaded-world observer ran. */
static bool ObserveReleaseOrder(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(release_order_observer != nullptr); (*release_order_observer)(); return true;
}

/** Reuse an original genuinely mined load, reject an old inside quote after real outward travel, and replay both foreign/valid command orders at the current inside point. */
TEST_CASE("Moving paid cargo release rechecks current position and target before either command ordering mutates packets", "[.release-command-order]")
{
	const char *input = std::getenv("OTTD_RELEASE_ORDER_SOURCE"), *mode = std::getenv("OTTD_RELEASE_ORDER_MODE"); REQUIRE(input != nullptr); REQUIRE(mode != nullptr); const std::string ordering{mode};
	REQUIRE((ordering == "valid-first" || ordering == "invalid-first"));
	const auto paths = GetTestCaseArtifactPaths("space", "release-command-order"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Observe only the ordinary loaded original paid miner; assertion failure is propagated through native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AutoRestoreBackup running(_pause_mode, PauseModes{}); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			SpaceVehicle *miner = nullptr;
			// Discover the original paid hull by its canonical role; no replacement body or private movement coordinate can satisfy this observer.
			for (auto *vehicle : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*vehicle) == 15) { REQUIRE(miner == nullptr); miner = vehicle; }
			REQUIRE(miner != nullptr); REQUIRE(miner->manifest != nullptr); REQUIRE(miner->manifest->pods.size() == 1); REQUIRE(miner->cargo.TotalCount() == 10);
			const auto craft = GetSpaceCraftReference(*miner); const auto home = miner->construction.home; const auto shell = miner->manifest->pods.front();
			auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr); const auto anchor = *GetLaunchCenterPlaneReference(center->launch_center->tile); LaunchCenterRef foreign{};
			// The supplied paid world already has a real foreign overlapping center; geometry overlap cannot confer its owner's release authority.
			for (const auto ref : GetLaunchCenterReferences()) if (ResolveLaunchCenter(ref)->owner != miner->owner) { REQUIRE(foreign == LaunchCenterRef{}); foreign = ref; }
			REQUIRE(foreign != LaunchCenterRef{}); auto *company = Company::Get(miner->owner); const auto deliveries = company->cur_economy.delivered_cargo[miner->cargo_type];
			/** @param suffix Exact before/after command-order checkpoint, written by the native save owner. */
			auto save = [&](std::string_view suffix) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("release-order-{}-{}-423.sav", ordering, suffix)), false) == SaveLoadResult::Ok);
			};
			/** @param target Exact proposed current recovery center. @return Actual public execution receipt for the original paid shell and ten physical units. */
			auto release = [&](LaunchCenterRef target) { return Command<Commands::ReleaseOrbitalCargo>::Post(craft.vehicle, craft.generation, target.station, target.generation, shell.pod, shell.generation, uint16_t{10}); };
			/** @param target Foreign, stale or currently out-of-circle target. Refusal must preserve every packet/reservation/manifest and all company/RNG owners. */
			auto refuse = [&](LaunchCenterRef target) {
				const auto vehicles = ComputeSpaceVehicleDigest(), pods = ComputeSpacePodDigest(), manifest = ComputeSpaceManifestDigest(), descent = ComputeSpaceDescentDigest(); const auto cash = company->money; const auto seeds = _random;
				CHECK_FALSE(release(target)); CHECK(ComputeSpaceVehicleDigest() == vehicles); CHECK(ComputeSpacePodDigest() == pods); CHECK(ComputeSpaceManifestDigest() == manifest); CHECK(ComputeSpaceDescentDigest() == descent);
				CHECK(company->money == cash); CHECK(_random.state[0] == seeds.state[0]); CHECK(_random.state[1] == seeds.state[1]);
			};
			/** @param done Actual current-position/completion predicate. @param reason Physical travel being observed. */
			auto advance = [&](const std::function<bool()> &done, std::string_view reason) {
				const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(65); uint ticks = 0;
				// Count synchronized native opportunities until the real spacecraft crosses the required boundary; no saved position, lease, cargo or timer is altered by this observer.
				while (!done()) { INFO(reason); REQUIRE(ticks < 2000); REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; } REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++ticks;
				}
			};
			/** @param action Native Visit action. @param target Real current natural body; the command owns navigation and position changes. */
			auto order = [&](SpaceOrderAction action, SpaceOrderTarget target) {
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.vehicle, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.vehicle, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, action, target.kind, target.id, target.generation));
			};
			save("before"); REQUIRE(CanReleaseOrbitalCargo(craft, home, shell, 10)); REQUIRE(CmdReleaseOrbitalCargo({}, craft.vehicle, craft.generation, home.station, home.generation, shell.pod, shell.generation, 10).Succeeded());
			const AsteroidBody *destination = nullptr;
			// Choose an existing unused real body beyond the circle, so only ordinary navigation makes an initially valid release command become invalid.
			for (const auto &body : GetAsteroidField().bodies) {
				if (body.mining_slot != VehicleID::Invalid() || body.survey_slot != INVALID_SPACE_SURVEY) continue;
				if (SpaceDistanceSquared(body.motion.position, anchor) > uint64_t{24 * SPACE_Q16} * (24 * SPACE_Q16)) { destination = &body; break; }
			}
			REQUIRE(destination != nullptr); order(SpaceOrderAction::Visit, {SpaceOrderTargetKind::Body, destination->id.base(), 0});
			advance([&] { return !IsInsideLaunchCenterDropZone(miner->position, anchor); }, "real outward circle crossing"); refuse(home); CHECK(miner->cargo.TotalCount() == 10);
			REQUIRE(Command<Commands::SetSpaceMission>::Post(craft.vehicle, UINT32_MAX)); // Public recall clears the old Visit and moves this loaded craft toward its owned home circle, retaining every packet.
			advance([&] { return IsInsideLaunchCenterDropZone(miner->position, anchor); }, "real owned-circle return"); REQUIRE(CanReleaseOrbitalCargo(craft, home, shell, 10)); REQUIRE(miner->position != anchor);
			const auto jobs = CaptureSpaceDescents().size(); const auto cash = company->money;
			if (ordering == "invalid-first") refuse(foreign);
			REQUIRE(release(home)); CHECK(miner->cargo.TotalCount() == 0); REQUIRE(CaptureSpaceDescents().size() == jobs + 1); REQUIRE(ResolveSpacePod(shell)->descent != nullptr);
			CHECK(ResolveSpacePod(shell)->descent->center == home); CHECK(ResolveSpacePod(shell)->descent->release_point == miner->position); CHECK(company->money == cash);
			if (ordering == "valid-first") refuse(foreign);
			refuse(home); // A repeated command for the now-descending same paid shell cannot remove or reserve another ten units.
			CHECK(GetSpaceCraftReference(*miner) == craft); CHECK(company->cur_economy.delivered_cargo[miner->cargo_type] == deliveries); save("after");
			fmt::print("RELEASE_ORDER native=true ordering={} actual_navigation=true stale_inside_quote_refused=true explicit_target=true once_only=true\n", ordering);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(release_order_observer, &observe); IConsole::CmdRegister("release_order_test", ObserveReleaseOrder);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
