/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_refusal.cpp Public native refusals on exact active and final-use normal originals.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6DDD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * No fixture builder,private field assignment or substitute ownership participates.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "normal_ground_freight_fixture.h"
#include "normal_ground_video_fixture.h"
#include "../tbm_cmd.h"
#include "../tbm_exchange.h"
#include "../underground_vehicle.h"
#include "../transport_tube.h"
#include "../landscape_cmd.h"
#include "../vehicle_func.h"
#include "../linkgraph/linkgraphschedule.h"
#include <thread>
#include "../table/strings.h"
#include "../safeguards.h"

/** @param stage Exact before/after/repeat identity. Pure native save retains all quantities for independent comparison. */
static void SaveNormalGroundRefusal(std::string_view stage)
{
	REQUIRE(_pause_mode == PauseModes{PauseMode::Normal});
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-refusal");
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", fmt::format("{}.sav", stage))), false) == SaveLoadResult::Ok);
}

/** Active actual cutting cannot authorize ordinary traffic,unpaid rail,early retirement or permanent conversion. */
static void RefuseNormalGroundActive()
{
	const auto projects = CaptureTBMProjects(); REQUIRE(projects.size() == 1); const auto &project = projects.front();
	const auto *job = GetExcavationJob(project.job); REQUIRE(job != nullptr); REQUIRE(job->cursor > 0); REQUIRE(job->cursor < job->route.size());
	REQUIRE_FALSE(IsLayerPortalOpenToNetwork(project.portal));
	REQUIRE(CheckLayerPortalWorkAccess(project.portal, project.cutter, project.job) == LayerPortalWorkError::None);
	const auto traffic = CheckLayerPortalWorkAccess(project.portal, VehicleID{2}, project.job);
	REQUIRE(traffic != LayerPortalWorkError::None);
	const auto solid = TileXYL(110, 110, MapLayer::Underground);
	REQUIRE(GetGeologyWorldState().terrain.Get(solid) == ExcavationState::Solid);
	const std::array footprint{solid}; REQUIRE(CheckExcavatedFootprint(footprint).error != ExcavationError::None);
	const auto rail_quote = Command<Commands::BuildRail>::Do({}, solid, RAILTYPE_RAIL, Track::Y, false);
	REQUIRE(rail_quote.Failed());
	REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, solid, RAILTYPE_RAIL, Track::Y, false).Failed());
	TileIndex support = INVALID_TILE;
	for (const auto &candidate : CaptureTBMSupport()) if (candidate.project == project.id && IsTBMSupportInUse(candidate.tile)) {
		support = candidate.tile;
		if (EnsureNoVehicleOnGround(support).Succeeded()) break;
	}
	REQUIRE(support != INVALID_TILE);
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		const auto conversion = Command<Commands::ConvertTBMSupport>::Do(flags, support);
		REQUIRE(conversion.Failed()); REQUIRE(conversion.GetErrorMessage() == STR_ERROR_TBM_SUPPORT_IN_USE);
		REQUIRE(Command<Commands::ControlTBMProject>::Do(flags, project.id, TBMProjectAction::Retire).Failed());
	}
	const auto release = GetTBMProjectReleaseProblem(project.id); REQUIRE(release != INVALID_STRING_ID);
	fmt::print("NORMAL_GROUND_REFUSAL_ACTIVE project={} job={} cutter={} nonparticipant=2 traffic_error={} solid={} rail_error={} support={} vacant={} conversion_error={} release_error={} cursor={} earth={} rock={}\n",
		project.id.base(), project.job.base(), project.cutter.base(), to_underlying(traffic), solid.base(), rail_quote.GetErrorMessage().base(),
		support.base(), EnsureNoVehicleOnGround(support).Succeeded() ? 1 : 0, STR_ERROR_TBM_SUPPORT_IN_USE.base(), release.base(), job->cursor, job->completed_earth_t, job->completed_rock_t);
}

/** Actual final-use machinery stays protected and native insufficient-funds admission cannot mint another capsule. */
static void RefuseNormalGroundFinal()
{
	REQUIRE(CaptureTBMProjects().empty());
	const auto home = TileXYL(96, 66, MapLayer::Underground);
	const Train *capsule = nullptr;
	for (const auto *front : Train::Iterate()) if (front->IsFrontEngine() && front->engine_type == GetTubeEngine(9) && front->tile == home) {
		REQUIRE(capsule == nullptr); capsule = front;
	}
	REQUIRE(capsule != nullptr); REQUIRE(capsule->IsStoppedInDepot()); REQUIRE(capsule->cargo.StoredCount() == 0);
	const auto demolition = Command<Commands::LandscapeClear>::Do({}, home); REQUIRE(demolition.Failed());
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, home).Failed());
	const auto coal = GetCargoTypeByLabel(CT_COAL);
	const auto purchase_depot = TileXYL(103, 69, MapLayer::Underground); // Actual empty far depot;the home slot is legitimately occupied.
	REQUIRE(IsEngineBuildable(capsule->engine_type, VehicleType::Train, _current_company));
	const auto quote = ExtractCommandCost(Command<Commands::BuildVehicle>::Do(DoCommandFlag::QueryCost, purchase_depot, capsule->engine_type, false, coal, ClientID::Invalid));
	REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() > Company::Get(_current_company)->money);
	const auto funds = ExtractCommandCost(Command<Commands::BuildVehicle>::Do({}, purchase_depot, capsule->engine_type, false, coal, ClientID::Invalid));
	REQUIRE(funds.Failed()); REQUIRE(funds.GetErrorMessage() == STR_ERROR_NOT_ENOUGH_CASH_REQUIRES_CURRENCY);
	REQUIRE(Company::Get(_current_company)->current_loan == Money{300000});
	// Exercise native generic funds admission while construction is publicly
	// unpaused,without permitting a simulation frame between these controls.
	REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
	REQUIRE_FALSE(Command<Commands::BuildVehicle>::Post(purchase_depot, capsule->engine_type, false, coal, ClientID::Invalid));
	REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
	fmt::print("NORMAL_GROUND_REFUSAL_FINAL capsule={} home={} demolition_error={} purchase_depot={} purchase_quote={} funds_error={} cash={} loan={}\n",
		capsule->index.base(), home.base(), demolition.GetErrorMessage().base(), purchase_depot.base(), quote.GetCost().base(), funds.GetErrorMessage().base(), Company::Get(_current_company)->money.base(), Company::Get(_current_company)->current_loan.base());
}

/** Exact ordinarily loaded original must remain byte-identical after all real native refusal attempts. */
TEST_CASE("Original normal-world active and useful owners refuse unsafe native actions", "[.normal-ground-refusal]")
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"), *scope_text = std::getenv("OTTD_NORMAL_GROUND_REFUSAL");
	REQUIRE(input != nullptr); REQUIRE(scope_text != nullptr); const std::string_view scope{scope_text}; REQUIRE((scope == "active" || scope == "final"));
	bool completed = false; std::exception_ptr failure;
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
			while (_pause_mode.Test(PauseMode::LinkGraph)) {
				REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop_LinkGraphPauseControl();
				if (_pause_mode.Test(PauseMode::LinkGraph)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			REQUIRE(_game_mode == GameMode::Normal); REQUIRE_FALSE(_networking);
			SaveNormalGroundRefusal("before");
			if (scope == "active") RefuseNormalGroundActive(); else RefuseNormalGroundFinal();
			SaveNormalGroundRefusal("after"); SaveNormalGroundRefusal("repeat"); completed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &inspect}; IConsole::CmdRegister("normal_ground_refusal", ObserveNormalGroundNewgame);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "ground-acceptance-null", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); REQUIRE(completed);
}
