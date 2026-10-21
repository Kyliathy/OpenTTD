/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_craft_window.cpp Actual native picked-hull window dispatch and selection purity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JDB
 * Retained earned paid worlds supply every physical role. The shared original
 * queue helper alone may pay/earn a second body; window calls grant no state.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../openttd.h"
#include "../gfxinit.h"
#include "../blitter/factory.hpp"
#include "../launch_assembly_gui.h"
#include "../space_survey_gui.h"
#include "../launch_center.h"
#include "../launch_operations.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../space_pod.h"
#include "../launch_facility_sprite.h"
#include "../vehicle_gui.h"
#include "../timer/timer_game_tick.h"
#include "../core/random_func.hpp"
#include "native_art_render_fixture.h"
#include "launch_fleet_paid_review_fixture.h"
#include "../safeguards.h"

static std::function<void()> *craft_window_observer = nullptr; ///< Invocation callback with no native gameplay ownership.

/** Invoke the actual inspector review after the ordinary engine loads its paid input.
 * @param args Parameter-free startup command. @return Whether the observer ran.
 */
static bool ObserveCraftWindow(std::span<std::string_view> args)
{
    REQUIRE(args.size() == 1); REQUIRE(craft_window_observer); (*craft_window_observer)(); return true;
}

/** Verify the real map/list dispatch resolves the actual native window selection. */
TEST_CASE("Picked paid Space hulls retain exact native inspector context", "[.launch-craft-window]")
{
    const char *source = std::getenv("OTTD_CRAFT_WINDOW_SOURCE"); REQUIRE(source);
    const auto paths = GetTestCaseArtifactPaths("space", "launch-craft-window"); const auto config = GetTestRuntimeConfig();
    bool observed = false; std::exception_ptr failure;
    /** Preserve actual native assertion failures across normal shutdown without repairing gameplay owners. */
    std::function<void()> observe = [&] {
        try {
            AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_local_company));
            REQUIRE(BlitterFactory::SelectBlitter("8bpp-optimized")); GfxLoadSprites();
            const char *mode = std::getenv("OTTD_CRAFT_WINDOW_MODE"); nlohmann::json paid = nullptr;
            // Reuse the exact original paid scenario body. It earns original
            // real work and bills before the separate zero-tick window audit.
            if (mode != nullptr && std::string_view{mode} == "queue") {
                REQUIRE_FALSE(GetLaunchCenterReferences().empty()); auto *center = ResolveLaunchCenter(GetLaunchCenterReferences().front()); REQUIRE(center);
                paid = EarnPaidLaunchQueue(*center, paths);
            }
            const auto before = std::array{ComputeSpaceVehicleDigest(), ComputeSpaceManifestDigest(), ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(),
                ComputeLaunchCenterDigest(), ComputeLaunchProductionDigest(), ComputeSpaceDescentDigest(), ComputeSurfaceHandlingDigest()};
            const auto ticks = TimerGameTick::counter; const auto seeds = std::array{_random.state[0], _random.state[1]};
            const auto money = Company::Get(_local_company)->money; const auto loan = Company::Get(_local_company)->current_loan;
            nlohmann::json selections = nlohmann::json::array();
            // Actual visible map bodies and real stored model14 inventory
            // entries exercise the public window dispatcher. Hidden passengers
            // cannot acquire another visible or selectable physical identity.
            for (const auto *craft : SpaceVehicle::Iterate()) {
                const bool visible = !craft->vehstatus.Test(VehState::Hidden) || IsLaunchSurfaceCraftArtVisible(*craft);
                if (!visible && GetSpaceVehicleModel(*craft) != 14) continue;
                const auto ref = GetSpaceCraftReference(*craft); const auto home = craft->construction.home;
                if (visible) {
                    Viewport view{}; view.width = 640; view.height = 360; view.zoom = ZoomLevel::Normal; view.layer = GetVehicleDisplayLayer(craft);
                    view.virtual_left = (craft->coord.left + craft->coord.right) / 2 - ScaleByZoom(320, view.zoom);
                    view.virtual_top = (craft->coord.top + craft->coord.bottom) / 2 - ScaleByZoom(180, view.zoom);
                    CHECK(CheckClickOnVehicle(view, 320, 180) == craft);
                    view.layer = view.layer == MapLayer::Space ? MapLayer::Surface : MapLayer::Space;
                    CHECK(CheckClickOnVehicle(view, 320, 180) != craft);
                }
                ShowVehicleViewWindow(craft);
                const bool orbital = craft->construction.placement == SpacePlacement::Orbital;
                const auto selected = orbital ? GetSpaceSurveySelection() : GetLaunchCraftWindowSelection(home);
                REQUIRE(selected == ref);
                auto *window = FindWindowById(orbital ? WindowClass::SpaceSurvey : WindowClass::LaunchAssembly, orbital ? 0 : home.station.base()); REQUIRE(window);
                const auto prefix = fmt::format("selected-{}", craft->index.base());
                CaptureNativeArtWindow(*window, paths.images / (prefix + ".ppm"));
                auto stale = ref; ++stale.generation;
                CHECK_FALSE(ShowLaunchCraftWindow(stale));
                CHECK((orbital ? GetSpaceSurveySelection() : GetLaunchCraftWindowSelection(home)) == ref);
                if (!orbital) {
                    auto old_home = home; ++old_home.generation; CHECK(GetLaunchCraftWindowSelection(old_home) == SpaceCraftRef{});
                    CHECK(GetLaunchCraftWindowSelection(home) == ref);
                }
                CaptureNativeArtWindow(*window, paths.images / (prefix + "-stale-refused.ppm"));
                selections.push_back({{"vehicle", ref.vehicle.base()}, {"generation", ref.generation}, {"model", GetSpaceVehicleModel(*craft)},
                    {"layer", to_underlying(GetVehicleDisplayLayer(craft))}, {"source_action", visible ? "map" : "stored_inventory"},
                    {"selected_vehicle", selected.vehicle.base()}, {"selected_generation", selected.generation}, {"orbital_inspector", orbital}, {"image", prefix + ".ppm"}});
            }
            REQUIRE_FALSE(selections.empty());
            const auto after = std::array{ComputeSpaceVehicleDigest(), ComputeSpaceManifestDigest(), ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(),
                ComputeLaunchCenterDigest(), ComputeLaunchProductionDigest(), ComputeSpaceDescentDigest(), ComputeSurfaceHandlingDigest()};
            CHECK(before == after); CHECK(TimerGameTick::counter == ticks); CHECK(std::array{_random.state[0], _random.state[1]} == seeds);
            CHECK(Company::Get(_local_company)->money == money); CHECK(Company::Get(_local_company)->current_loan == loan);
            std::ofstream output(paths.reports / "native-craft-window.json");
            output << nlohmann::json{{"actual_generic_dispatch", true}, {"selections", selections}, {"owner_digests_before", before}, {"owner_digests_after", after},
                {"capture_ticks", 0}, {"component_pose_inputs", false}, {"paid_scenario", paid}, {"cash", int64_t{money}}, {"loan", int64_t{loan}}}.dump(2) << '\n';
            REQUIRE(output.good()); observed = true;
        } catch (...) { failure = std::current_exception(); }
        _exit_game = true;
    };
    AutoRestoreBackup callback(craft_window_observer, &observe); IConsole::CmdRegister("launch_craft_window", ObserveCraftWindow);
    const std::string config_arg = config.string();
    std::array<std::string_view, 17> args = {"openttd_space_art_test.exe", "-v", "null:ticks=2", "-b", "8bpp-optimized", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
    CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
