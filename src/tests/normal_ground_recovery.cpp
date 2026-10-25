/* This file is part of OpenTTD. Licensed under the GNU GPL version 2. */
/** @file normal_ground_recovery.cpp Genuine saved-wait replay and settled irregular access.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md
 * These observers load exact retained successors of the completed paid runs.
 * They construct no replacement world, cash, cargo, cut or installed machinery.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "normal_ground_runtime_fixture.h"
#include "normal_ground_video_fixture.h"
#include "save_first_tick_fixture.h"
#include "../layer_portal_cmd.h"
#include "../linkgraph/linkgraphschedule.h"
#include <thread>
#include "../safeguards.h"

/** Serialize an actual normally paused frame twice without advancing its owners.
 * @param name Explicit output stem in this recovery invocation's save directory.
 */
static void SaveGenuineRecoveryFrame(std::string_view name)
{
    REQUIRE(_pause_mode == PauseModes{PauseMode::Normal});
    const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-recovery");
    const auto tick = TimerGameTick::counter;
    for (const auto suffix : {"", "-repeat"}) {
        REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", fmt::format("{}{}.sav", name, suffix))), false) == SaveLoadResult::Ok);
    }
    CHECK(TimerGameTick::counter == tick);
}

/** Finish the selected genuine irregular project's public final-access sequence. */
static void ConvertGenuineIrregularAccess()
{
    AutoRestoreBackup actor{_current_company, _local_company};
    const auto *company = Company::Get(_current_company);
    const auto opening = company->money;
    const auto tick = TimerGameTick::counter;
    const auto random = _random;
    const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
    const auto rock = CountNormalGroundRockStates();
    const auto projects = CaptureTBMProjects(); REQUIRE(projects.size() == 1);
    const auto project = projects.front(); REQUIRE(IsTBMProjectFinished(project.phase));
    REQUIRE(GetTBMProjectReleaseProblem(project.id) == INVALID_STRING_ID);
    const auto assignments = CaptureTBMExchangeAssignments(); REQUIRE(assignments.size() == 1);
    const auto hauler_id = assignments.front().hauler;
    SaveGenuineRecoveryFrame("conversion-before");
    REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
    PayNormalFreightCommand<Commands::ControlTBMProject>("retire_settled_irregular", project.id, TBMProjectAction::Retire);
    CHECK(CaptureTBMProjects().empty()); CHECK(CaptureTBMExchangeAssignments().empty()); CHECK(CaptureTBMWagonSets().empty());
    auto *hauler = Train::GetIfValid(hauler_id); REQUIRE(hauler != nullptr);
    if (!hauler->vehstatus.Test(VehState::Stopped)) {
        PayNormalFreightCommand<Commands::StartStopVehicle>("stop_released_irregular_hauler", hauler->tile, hauler_id, true);
    }
    const auto *cutter = Train::GetIfValid(project.cutter); REQUIRE(cutter != nullptr);
    CHECK(cutter->vehstatus.Test(VehState::Stopped)); CHECK(hauler->vehstatus.Test(VehState::Stopped));
    const auto support = CaptureTBMSupport(); REQUIRE_FALSE(support.empty());
    Money conversion = 0;
    // Convert only actual released support. Each tile retains its original
    // frozen native charge; no new floor, material refund or free track appears.
    for (const auto &piece : support) {
        REQUIRE_FALSE(IsTBMSupportInUse(piece.tile)); conversion += piece.conversion_price;
        PayNormalFreightCommand<Commands::ConvertTBMSupport>("irregular_permanent_support", piece.tile);
    }
    CHECK(CaptureTBMSupport().empty());
    REQUIRE(GetLayerPortalWorkState(project.portal) == LayerPortalWorkState::Sealed);
    PayNormalFreightCommand<Commands::ChangeLayerPortalConnection>("irregular_ordinary_access", project.portal, true);
    CHECK(GetLayerPortalWorkState(project.portal) == LayerPortalWorkState::Connected);
    CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
    CHECK(CountNormalGroundRockStates() == rock);
    CHECK(TimerGameTick::counter == tick); CHECK(std::ranges::equal(random.state, _random.state));
    REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
    SaveGenuineRecoveryFrame("conversion-after");
    fmt::print("GENUINE_GROUND_CONVERSION tick={} project={} portal={} support={} conversion={} opening_cash={} cash={} loan={} excavated={} complete=1\n",
        tick, project.id.base(), project.portal.base(), support.size(), conversion.base(), opening.base(), company->money.base(), company->current_loan.base(), rock[2]);
}

/** Inspect a selected retained paid successor through the actual native application.
 * @param action Paused public conversion or exact saved-wait first-frame observer.
 */
static void RunGenuineRecovery(const std::function<void()> &action)
{
    const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
    bool completed = false; std::exception_ptr failure;
    /** Dispatch only after normal afterload; retain callback errors for Catch2. */
    std::function<void()> inspect = [&] {
        try {
            REQUIRE(_game_mode == GameMode::Normal); REQUIRE_FALSE(_networking);
            REQUIRE(Company::IsValidID(_local_company));
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            // A restored linkgraph worker may hold Normal. Let only its own
            // controller finish; no game-loop tick refills a landscaping budget.
            while (_pause_mode.Test(PauseMode::LinkGraph)) {
                REQUIRE(std::chrono::steady_clock::now() < deadline);
                StateGameLoop_LinkGraphPauseControl();
                if (_pause_mode.Test(PauseMode::LinkGraph)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            action(); completed = true;
        } catch (...) { failure = std::current_exception(); }
        _exit_game = true;
    };
    AutoRestoreBackup hook{_normal_ground_newgame_observer, &inspect};
    IConsole::CmdRegister("normal_ground_recovery", ObserveNormalGroundNewgame);
    const std::string config = GetTestRuntimeConfig().string();
    std::array<std::string_view, 17> arguments{"openttd_tube_test.exe", "-v", "ground-acceptance-null", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
    CHECK(openttd_main(arguments) == 0);
    if (failure) std::rethrow_exception(failure);
    REQUIRE(completed);
}

/** Retire/convert only the actually settled irregular paid-world successor. */
TEST_CASE("Genuine irregular settled work becomes permanent public access", "[.normal-ground-conversion]")
{
    RunGenuineRecovery(ConvertGenuineIrregularAccess);
}

/** Replay the actual useful-route loaded wait through native reload and first tick. */
TEST_CASE("Genuine loaded useful wait resumes the same complete first frame", "[.normal-ground-wait]")
{
    /** Compare real accounts, packets, RNG and all domains without another purchase. */
    const auto replay = [] {
        const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
        const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-recovery");
        CheckFirstNativeTick(input, std::nullopt, paths.saves / "useful-wait");
    };
    RunGenuineRecovery(replay);
}
