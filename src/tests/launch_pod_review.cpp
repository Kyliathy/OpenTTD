/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_pod_review.cpp Actual paid passive shell, packet and atomic-recovery presentation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JDC
 * Original qualified saves supply every shell, packet and countdown. Only an
 * explicitly requested single normal work opportunity may complete a ready job.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../3rdparty/nlohmann/json.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../openttd.h"
#include "../gfxinit.h"
#include "../blitter/factory.hpp"
#include "../launch_assembly_gui.h"
#include "../launch_center.h"
#include "../launch_operations.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../space_pod.h"
#include "../space_sprite.h"
#include "../station_base.h"
#include "../asteroid.h"
#include "../timer/timer_game_tick.h"
#include "../core/random_func.hpp"
#include "native_art_render_fixture.h"
#include <chrono>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *pod_review_observer = nullptr; ///< Invocation callback with no owned shell or native work.

/** Invoke this review only after ordinary native save loading.
 * @param args Parameter-free startup command. @return Whether the observer ran.
 */
static bool ObservePodReview(std::span<std::string_view> args)
{
    REQUIRE(args.size() == 1); REQUIRE(pod_review_observer); (*pod_review_observer)(); return true;
}

/** Read every actual saved physical owner touched by passive cargo presentation.
 * @return Existing hull/manifest/pod/flight/center/production/descent/handling digests.
 */
static auto PodReviewDigests()
{
    return std::array{ComputeSpaceVehicleDigest(), ComputeSpaceManifestDigest(), ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(),
        ComputeLaunchCenterDigest(), ComputeLaunchProductionDigest(), ComputeSpaceDescentDigest(), ComputeSurfaceHandlingDigest()};
}

/** Observe actual source selection, custody and packet ownership without earning work.
 * @return Native pool records; carried/stored shells have no free drawable source pose.
 */
static nlohmann::json ObserveNativePods()
{
    nlohmann::json rows = nlohmann::json::array();
    // The actual native pool remains the only shell/packet owner. Source queries
    // cannot duplicate carried assets or leave a consumed recovery marker alive.
    for (const auto *pod : SpacePod::Iterate()) {
        const auto pose = GetSpacePodArtPose(*pod);
        nlohmann::json row{{"pod", pod->index.base()}, {"generation", pod->generation}, {"custody", to_underlying(pod->state.custody)},
            {"position", {pod->state.position.x, pod->state.position.y}}, {"drawable_space_only", pose.has_value()}};
        if (pose) row["art"] = {{"state", pose->state}, {"frame", pose->frame}, {"direction", to_underlying(pose->direction)}, {"sprite", *ResolveSpaceSprite(*pose)}};
        if (pod->descent) {
            const auto &job = *pod->descent;
            row["descent"] = {{"remaining", job.remaining_ticks}, {"blocked", to_underlying(job.blocked)}, {"cargo", job.cargo},
                {"packets", job.packets.AvailableCount()}, {"quantity", job.quantity}, {"release_point", {job.release_point.x, job.release_point.y}},
                {"center", job.center.station.base()}, {"center_generation", job.center.generation}};
        }
        rows.push_back(row);
    }
    return rows;
}

/** Read-only native world and actual inventory window observation around passive recovery. */
TEST_CASE("Actual paid pods retain packets and disappear exactly at native atomic recovery", "[.launch-pod-review]")
{
    const char *source = std::getenv("OTTD_POD_REVIEW_SOURCE"); REQUIRE(source);
    const auto paths = GetTestCaseArtifactPaths("space", "launch-pod-review"); const auto config = GetTestRuntimeConfig();
    bool observed = false; std::exception_ptr failure;
    /** Preserve actual failures through ordinary native shutdown without repairing state. */
    std::function<void()> observe = [&] {
        try {
            AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_local_company));
            REQUIRE(BlitterFactory::SelectBlitter("8bpp-optimized")); GfxLoadSprites();
            REQUIRE_FALSE(GetLaunchCenterReferences().empty()); auto *center = ResolveLaunchCenter(GetLaunchCenterReferences().front()); REQUIRE(center);
            REQUIRE_FALSE(SpacePod::Iterate().empty()); auto *focus = *SpacePod::Iterate().begin();
            const auto focus_ref = GetSpacePodReference(*focus); const auto home = GetLaunchCenterReference(*center);
            const auto point = focus->state.position;
            const auto camera = TileXYL(std::clamp(point.x / SPACE_Q16, 1, static_cast<int>(Map::SizeX()) - 2),
                std::clamp(point.y / SPACE_Q16, 1, static_cast<int>(Map::SizeY()) - 2), MapLayer::Space);
            const auto before = PodReviewDigests(); const auto tick_before = TimerGameTick::counter;
            const auto seeds = std::array{_random.state[0], _random.state[1]}; const auto money = Company::Get(_local_company)->money;
            auto first = ObserveNativePods();
            {
                AutoRestoreBackup labels(_display_opt); _display_opt = {};
                CaptureNativeArtViewport(camera, ZoomLevel::Normal, paths.images / "space-before.ppm", paths.images / "space-before.indexed");
                CaptureNativeArtViewport(center->launch_center->tile, ZoomLevel::Normal, paths.images / "surface-before.ppm", paths.images / "surface-before.indexed");
            }
            ShowLaunchAssemblyWindow(home); auto *window = FindWindowById(WindowClass::LaunchAssembly, home.station.base()); REQUIRE(window);
            CaptureNativeArtWindow(*window, paths.images / "inventory-before-selection.ppm");
            const auto panel = window->GetWidget<NWidgetBase>(0)->GetCurrentRect(); uint craft_rows = 0;
            // Actual same-home hull rows precede this fixture's first real shell.
            // Click the native public widget rather than assigning private GUI
            // state or creating a surrogate Vehicle for a passive asset.
            for (const auto *craft : SpaceVehicle::Iterate()) if (craft->construction.home == home) ++craft_rows;
            window->OnClick({panel.left + 12, panel.top + 8 + static_cast<int>(6 + craft_rows) * (GetCharacterHeight(FontSize::Normal) + 3) + 1}, 0, 1);
            REQUIRE(GetLaunchPodWindowSelection(home) == focus_ref);
            CaptureNativeArtWindow(*window, paths.images / "pod-selected-before.ppm");
            CHECK(PodReviewDigests() == before); CHECK(TimerGameTick::counter == tick_before);
            CHECK(std::array{_random.state[0], _random.state[1]} == seeds); CHECK(Company::Get(_local_company)->money == money);
            nlohmann::json recovery = nullptr; const char *mode = std::getenv("OTTD_POD_REVIEW_MODE");
            // One actual earned opportunity is necessary to test the SAME open
            // window's stale shell selection across native atomic recovery.
            // Existing original snapshots alone cannot retain that GUI state.
            if (mode != nullptr && std::string_view{mode} == "recover") {
                REQUIRE(focus->descent); REQUIRE(focus->descent->remaining_ticks == 1);
                const auto cargo = focus->descent->cargo; const auto quantity = focus->descent->packets.AvailableCount();
                const auto held = center->goods[cargo].HasData() ? center->goods[cargo].GetData().cargo.AvailableCount() : 0; const auto count = SpacePod::GetNumItems();
                const auto elapsed = GetAsteroidField().elapsed_ticks;
                {
                    AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
                    // Normal native simulation performs the original whole packet
                    // handoff and shell deletion. No timer, cargo or custody field
                    // is assigned or interpolated by this presentation observer.
                    while (GetAsteroidField().elapsed_ticks == elapsed) {
                        REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop();
                        if (GetAsteroidField().elapsed_ticks == elapsed) std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
                CHECK(GetAsteroidField().elapsed_ticks == elapsed + 1); CHECK(ResolveSpacePod(focus_ref) == nullptr);
                CHECK(SpacePod::GetNumItems() == count - 1); CHECK(GetLaunchPodWindowSelection(home) == SpacePodRef{});
                CHECK(center->goods[cargo].GetData().cargo.AvailableCount() == held + quantity);
                const auto after_work = PodReviewDigests();
                CaptureNativeArtViewport(camera, ZoomLevel::Normal, paths.images / "space-after.ppm", paths.images / "space-after.indexed");
                CaptureNativeArtViewport(center->launch_center->tile, ZoomLevel::Normal, paths.images / "surface-after.ppm", paths.images / "surface-after.indexed");
                CaptureNativeArtWindow(*window, paths.images / "pod-spent-after.ppm"); CHECK(PodReviewDigests() == after_work);
                recovery = {{"earned_ticks", 1}, {"spent_pod", focus_ref.pod.base()}, {"spent_generation", focus_ref.generation},
                    {"quantity", quantity}, {"station_before", held}, {"station_after", center->goods[cargo].GetData().cargo.AvailableCount()},
                    {"deleted_exact_shell", true}, {"stale_selection_empty", true}, {"assigned_work_or_cargo", false}};
            }
            std::ofstream output(paths.reports / "native-pod-review.json");
            output << nlohmann::json{{"before", first}, {"after", ObserveNativePods()}, {"recovery", recovery}, {"source_pose_grants", false},
                {"capture_only_ticks", 0}, {"selected_pod", focus_ref.pod.base()}, {"selected_generation", focus_ref.generation}}.dump(2) << '\n';
            REQUIRE(output.good()); observed = true;
        } catch (...) { failure = std::current_exception(); }
        _exit_game = true;
    };
    AutoRestoreBackup callback(pod_review_observer, &observe); IConsole::CmdRegister("launch_pod_review", ObservePodReview);
    const std::string config_arg = config.string();
    std::array<std::string_view, 17> args = {"openttd_space_art_test.exe", "-v", "null:ticks=2", "-b", "8bpp-optimized", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
    CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
