/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_access_fixture.h Paid ordinary connection after actual normal-world boring and settlement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6CDCC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_NORMAL_GROUND_ACCESS_FIXTURE_H
#define TESTS_NORMAL_GROUND_ACCESS_FIXTURE_H

#include "normal_ground_boring_fixture.h"

/** Retire only settled native owners,convert their real support and connect already earned floor. */
inline void BuildNormalGroundAccess()
{
	REQUIRE(_game_mode == GameMode::Normal); REQUIRE_FALSE(_networking);
	REQUIRE(Company::IsValidID(_local_company)); AutoRestoreBackup actor{_current_company, _local_company};
	const auto *company = Company::Get(_current_company); const auto opening = company->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto rock = CountNormalGroundRockStates(); CHECK(rock[1] == 0); CHECK(rock[2] == 36); CHECK(rock[3] == 0);
	const auto projects = CaptureTBMProjects(); REQUIRE(projects.size() == 1);
	const auto project = projects.front(); REQUIRE(IsTBMProjectFinished(project.phase));
	const auto assignments = CaptureTBMExchangeAssignments(); REQUIRE(assignments.size() == 1);
	const auto hauler_id = assignments.front().hauler;
	REQUIRE(GetTBMProjectReleaseProblem(project.id) == INVALID_STRING_ID);
	const bool paused = _pause_mode.Test(PauseMode::Normal);
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
	PayNormalFreightCommand<Commands::ControlTBMProject>("retire_settled_standard", project.id, TBMProjectAction::Retire);
	REQUIRE(CaptureTBMProjects().empty()); REQUIRE(CaptureTBMExchangeAssignments().empty()); REQUIRE(CaptureTBMWagonSets().empty());
	auto *hauler = Train::GetIfValid(hauler_id); REQUIRE(hauler != nullptr);
	if (!hauler->vehstatus.Test(VehState::Stopped)) PayNormalFreightCommand<Commands::StartStopVehicle>("stop_released_hauler", hauler->tile, hauler_id, true);
	const auto *cutter = Train::GetIfValid(project.cutter); REQUIRE(cutter != nullptr);
	CHECK(cutter->vehstatus.Test(VehState::Stopped)); CHECK(hauler->vehstatus.Test(VehState::Stopped));
	const auto support = CaptureTBMSupport(); REQUIRE_FALSE(support.empty());
	Money conversion = 0;
	for (const auto &piece : support) {
		REQUIRE_FALSE(IsTBMSupportInUse(piece.tile)); conversion += piece.conversion_price;
		PayNormalFreightCommand<Commands::ConvertTBMSupport>("ordinary_support", piece.tile);
	}
	CHECK(CaptureTBMSupport().empty());
	CHECK(GetLayerPortalWorkState(project.portal) == LayerPortalWorkState::Sealed);
	PayNormalFreightCommand<Commands::ChangeLayerPortalConnection>("ordinary_access", project.portal, true);
	CHECK(GetLayerPortalWorkState(project.portal) == LayerPortalWorkState::Connected);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(CountNormalGroundRockStates() == rock); CHECK(company->current_loan == Money{300000});
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-access");
	for (const auto name : {"ordinary-access.sav", "ordinary-access-repeat.sav"})
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
	fmt::print("NORMAL_GROUND_ACCESS portal={} target={} cutter={} hauler={} support={} conversion={} opening_cash={} cash={} loan={} tick={} excavated={} state={}\n",
		project.portal.base(), GetLayerPortalPairTile(project.portal).base(), project.cutter.base(), hauler_id.base(), support.size(), conversion.base(),
		opening.base(), company->money.base(), company->current_loan.base(), TimerGameTick::counter, rock[2], to_underlying(GetLayerPortalWorkState(project.portal)));
	std::fflush(stdout);
}

/** Load the exact actually settled original and apply public final release/conversion/connection commands. */
inline void RunNormalGroundAccess()
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
	bool observed = false; std::exception_ptr failure;
	std::function<void()> build = [&]() {
		try { BuildNormalGroundAccess(); observed = true; } catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &build};
	IConsole::CmdRegister("normal_ground_access", ObserveNormalGroundNewgame);
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(),
		"-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null",
		"-I", "original_dos", "-d", "misc=4,sl=1", "-g", input};
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	REQUIRE(observed);
}

#endif /* TESTS_NORMAL_GROUND_ACCESS_FIXTURE_H */
