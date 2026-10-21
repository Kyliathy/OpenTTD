/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_fleet_paid_review.cpp Ordinary loaded paid launch-world presentation observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JD
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md task16.2
 * Immutable qualified paid snapshots supply all phases, stock and physical
 * poses. Snapshot observation is read-only; the optional queue pays the original
 * recipe and earns normal native work before capture, without resource or pose grants.
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
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../multilayer_space_content.h"
#include "../command_func.h"
#include <chrono>
#include <thread>
#include "../launch_operations.h"
#include "../launch_facility_sprite.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "../vehicle_gui.h"
#include "../asteroid.h"
#include "../timer/timer_game_tick.h"
#include "../core/random_func.hpp"
#include "native_art_render_fixture.h"
#include "launch_fleet_paid_review_fixture.h"
#include "../safeguards.h"

static std::function<void()> *paid_review_observer = nullptr; ///< Invocation callback, owning no saved gameplay state.

/** Run the review only after the ordinary engine has loaded its qualified paid save.
 * @param args Parameter-free startup command. @return Whether the observer ran.
 */
static bool ObservePaidLaunchReview(std::span<std::string_view> args)
{
    REQUIRE(args.size() == 1); REQUIRE(paid_review_observer); (*paid_review_observer)(); return true;
}

/** Snapshot actual saved state owners for a read-only presentation audit.
 * @return Native hull/manifest/pod/flight/center/assembly/descent/handling digests.
 */
static auto PaidLaunchDigests()
{
    return std::array{ComputeSpaceVehicleDigest(), ComputeSpaceManifestDigest(), ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(),
        ComputeLaunchCenterDigest(), ComputeLaunchProductionDigest(), ComputeSpaceDescentDigest(), ComputeSurfaceHandlingDigest()};
}

/** Retain the exact physical identity and native source selected by a loaded paid hull.
 * @param craft Actual existing paid vehicle, including Hidden/carried/stored roles.
 * @param paths Invocation-owned captures; the actual craft's layer supplies the camera.
 * @return Native identity, custody, fuel, phase and selection observations for that same hull.
 */
static nlohmann::json CapturePaidLaunchCraft(const SpaceVehicle &craft, const TestArtifactPaths &paths)
{
    const auto model = GetSpaceVehicleModel(craft);
    const auto layer = GetVehicleDisplayLayer(&craft);
    const auto prefix = fmt::format("craft-{}-model-{}", craft.index.base(), model);
    CaptureNativeArtViewport(craft.tile, ZoomLevel::Normal, paths.images / (prefix + ".ppm"), paths.images / (prefix + ".indexed"));
    Viewport view{}; view.width = 640; view.height = 360; view.zoom = ZoomLevel::Normal; view.layer = layer;
    view.virtual_left = (craft.coord.left + craft.coord.right) / 2 - ScaleByZoom(320, view.zoom);
    view.virtual_top = (craft.coord.top + craft.coord.bottom) / 2 - ScaleByZoom(180, view.zoom);
    const bool visible = !craft.vehstatus.Test(VehState::Hidden) || IsLaunchSurfaceCraftArtVisible(craft);
    const bool selectable = visible && !craft.vehstatus.Test(VehState::Unclickable);
    const auto *picked = CheckClickOnVehicle(view, 320, 180);
    CHECK((picked == &craft) == selectable);
    view.layer = layer == MapLayer::Space ? MapLayer::Surface : MapLayer::Space;
    CHECK(CheckClickOnVehicle(view, 320, 180) != &craft);
    VehicleSpriteSeq expected; craft.GetImage(craft.direction, EngineImageType::OnMap, &expected);
    CHECK(expected == craft.sprite_cache.sprite_seq);
    nlohmann::json row{{"vehicle", craft.index.base()}, {"generation", craft.generation}, {"model", model},
        {"layer", to_underlying(layer)}, {"tile", craft.tile.base()}, {"placement", to_underlying(craft.construction.placement)},
        {"native_hidden", craft.vehstatus.Test(VehState::Hidden)}, {"visible", visible}, {"selectable", selectable}, {"correct_layer_picked", picked == &craft},
        {"wrong_layer_refused", true}, {"cargo_quantity", craft.cargo.TotalCount()}, {"cargo_type", craft.cargo_type}, {"image", prefix + ".ppm"}};
    if (craft.manifest) {
        row["supplies"] = {craft.manifest->supplies[0].TotalCount(), craft.manifest->supplies[1].TotalCount(), craft.manifest->supplies[2].TotalCount()};
        row["service_ticks"] = craft.manifest->service_ticks; row["service_paused"] = craft.manifest->service_paused;
        row["pods"] = craft.manifest->pods.size();
    }
    if (craft.transition) {
        const auto &flight = *craft.transition;
        row["flight"] = {{"phase", to_underlying(flight.phase)}, {"blocked", to_underlying(flight.blocked)}, {"remaining", flight.remaining_ticks},
            {"outbound_burned", flight.outbound_burned}, {"recovery_burned", flight.recovery_burned}, {"deployed", flight.deployed},
            {"missions_started", flight.missions_started}, {"missions_completed", flight.missions_completed}};
    }
    if (model == 14) {
        const auto direction = GetLaunchRocketArtDirection(craft); const auto pose = GetLaunchRocketArtPose(craft, direction); REQUIRE(pose);
        row["source"] = {{"state", pose->state}, {"frame", pose->frame}, {"direction", pose->direction}, {"sprite", *ResolveLaunchSprite(*pose)}};
    }
    return row;
}

/** Earn a genuine blocked-pad queue through the original paid assembly command.
 * @param center Actual retained paid facility and finite station inventory.
 * @param paths Invocation-owned work/pause/queue observation captures.
 * @return Original capital/material debits and actual completed work opportunities.
 */
nlohmann::json EarnPaidLaunchQueue(Station &center, const TestArtifactPaths &paths)
{
    AutoRestoreBackup labels(_display_opt); _display_opt = {};
    const auto home = GetLaunchCenterReference(center); const auto &definition = *GetSpaceVehicleConstruction(14);
    const auto money = Company::Get(_current_company)->money; const auto hulls = GetSpaceVehicleCount();
    std::vector<uint> inputs;
    // Quote and pay the actual original whole recipe from existing station
    // packets. No observer allocation, stock grant or money adjustment can
    // substitute for the public native commissioning transaction.
    for (const auto &input : definition.inputs) inputs.push_back(center.goods[GetCargoTypeByLabel(input.label)].GetData().cargo.AvailableCount());
    const auto quote = CmdAssembleLaunchCraft({}, home.station, home.generation, 14); REQUIRE(quote.Succeeded());
    CHECK(quote.GetCost() == Money{definition.capital}); CHECK(Company::Get(_current_company)->money == money);
    REQUIRE(Command<Commands::AssembleLaunchCraft>::Post(home.station, home.generation, uint16_t{14}));
    REQUIRE(GetSpaceVehicleCount() == hulls + 1); auto *craft = ResolveSpaceCraft(center.launch_operations->workshop); REQUIRE(craft);
    CHECK(Company::Get(_current_company)->money == money - definition.capital);
    for (size_t i = 0; i < definition.inputs.size(); ++i) CHECK(center.goods[GetCargoTypeByLabel(definition.inputs[i].label)].GetData().cargo.AvailableCount() == inputs[i] - definition.inputs[i].quantity);
    const auto workshop = *GetLaunchFacilityZone(center.launch_center->tile, center.launch_center->rotation, LaunchFacilityZone::Workshop);
    CHECK(GetLaunchFacilityArtPose(center, workshop.tile, 0)->state == 2);
    CaptureNativeArtViewport(center.launch_center->tile, ZoomLevel::Normal, paths.images / "paid-workshop-working.ppm", paths.images / "paid-workshop-working.indexed");
    REQUIRE(Command<Commands::PauseLaunchAssembly>::Post(craft->index, craft->generation, true));
    CHECK(craft->construction.remaining_ticks == definition.work_ticks);
    CHECK(GetLaunchFacilityArtPose(center, workshop.tile, 0)->state == 3);
    CaptureNativeArtViewport(center.launch_center->tile, ZoomLevel::Normal, paths.images / "paid-workshop-paused.ppm", paths.images / "paid-workshop-paused.indexed");
    REQUIRE(Command<Commands::PauseLaunchAssembly>::Post(craft->index, craft->generation, false));
    uint opportunities = 0;
    {
        // Resume the same normal simulation opportunities used by the paid
        // producer. Restore the UI pause preference afterward; no field clock,
        // countdown, phase, resource or work receipt is assigned by this loop.
        AutoRestoreBackup pause(_pause_mode, PauseModes{});
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(45);
        while (craft->construction.placement == SpacePlacement::Assembling) {
            REQUIRE(std::chrono::steady_clock::now() < deadline); REQUIRE(opportunities <= definition.work_ticks);
            const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
            if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
            REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++opportunities;
        }
    }
    CHECK(opportunities == definition.work_ticks); CHECK(craft->construction.remaining_ticks == 0);
    CHECK(craft->construction.placement == SpacePlacement::SurfaceStored); CHECK(craft->construction.capital_paid == definition.capital);
    REQUIRE_FALSE(center.launch_operations->pad_queue.empty()); CHECK(center.launch_operations->pad_queue.back() == GetSpaceCraftReference(*craft));
    REQUIRE(center.launch_operations->pad_active != SpaceCraftRef{}); CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(*craft));
    return {{"gameplay_commands", 3}, {"earned_simulation_ticks", opportunities}, {"capital_paid", definition.capital},
        {"new_vehicle", craft->index.base()}, {"source_finite_stock", inputs}, {"granted_resources", false}, {"assigned_work", false},
        {"queued_after_paid_completion", true}, {"original_active_vehicle", center.launch_operations->pad_active.vehicle.base()}};
}

/** Capture normal actual paid owners and native UI without changing physical inputs. */
TEST_CASE("Ordinary paid launch snapshots retain original owners through native presentation", "[.launch-fleet-paid-review]")
{
    const char *source = std::getenv("OTTD_LAUNCH_PAID_REVIEW_SOURCE"); REQUIRE(source);
    const auto paths = GetTestCaseArtifactPaths("space", "launch-fleet-paid-review");
    const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
    /** Retain native assertion failures across normal engine shutdown, preserving the paid world. */
    std::function<void()> observe = [&] {
        try {
            REQUIRE(Company::IsValidID(_local_company)); AutoRestoreBackup actor(_current_company, _local_company);
            // Null video deliberately installs a null encoder. Reload actual
            // distributed graphics with the real indexed renderer, never mocks.
            REQUIRE(BlitterFactory::SelectBlitter("8bpp-optimized")); GfxLoadSprites();
            REQUIRE(BlitterFactory::GetCurrentBlitter()->GetScreenDepth() == 8);
            const char *mode = std::getenv("OTTD_LAUNCH_PAID_REVIEW_MODE"); nlohmann::json paid_scenario = nullptr;
            // The optional queue scenario earns its own paid native work before
            // the read-only presentation audit; ordinary snapshot mode changes none.
            if (mode != nullptr && std::string_view{mode} == "queue") {
                REQUIRE_FALSE(GetLaunchCenterReferences().empty());
                auto *center = ResolveLaunchCenter(GetLaunchCenterReferences().front()); REQUIRE(center);
                paid_scenario = EarnPaidLaunchQueue(*center, paths);
            }
            const auto before = PaidLaunchDigests(); const auto ticks = TimerGameTick::counter;
            const auto seeds = std::array{_random.state[0], _random.state[1]};
            const auto money = Company::Get(_local_company)->money; const auto loan = Company::Get(_local_company)->current_loan;
            nlohmann::json crafts = nlohmann::json::array(), centers = nlohmann::json::array(), pods = nlohmann::json::array();
            {
                AutoRestoreBackup labels(_display_opt); _display_opt = {};
                // Optional native map labels are hidden only for occupant
                // readability; every camera still uses its original physical tile.
                for (const auto *craft : SpaceVehicle::Iterate()) crafts.push_back(CapturePaidLaunchCraft(*craft, paths));
                for (const auto ref : GetLaunchCenterReferences()) {
                    const auto *center = ResolveLaunchCenter(ref); REQUIRE(center); REQUIRE(center->launch_operations);
                    const auto &operations = *center->launch_operations;
                    CaptureNativeArtViewport(center->launch_center->tile, ZoomLevel::Normal, paths.images / fmt::format("center-{}.ppm", ref.station.base()), paths.images / fmt::format("center-{}.indexed", ref.station.base()));
                    centers.push_back({{"station", ref.station.base()}, {"generation", ref.generation}, {"anchor", center->launch_center->tile.base()},
                        {"rotation", center->launch_center->rotation}, {"queue_count", operations.pad_queue.size()}, {"pad_active", operations.pad_active.vehicle.base()},
                        {"workshop", operations.workshop.vehicle.base()}, {"service", operations.service.vehicle.base()}});
                    ShowLaunchAssemblyWindow(ref); auto *window = FindWindowById(WindowClass::LaunchAssembly, ref.station.base()); REQUIRE(window);
                    CaptureNativeArtWindow(*window, paths.images / fmt::format("center-{}-inventory.ppm", ref.station.base()));
                    const auto panel = window->GetWidget<NWidgetBase>(0)->GetCurrentRect();
                    // A genuine native inventory-row click selects the first
                    // paid hull for its existing stock/phase disclosures. No
                    // private GUI fields, craft references or gameplay commands
                    // are injected into the window by the observer.
                    window->OnClick({panel.left + 12, panel.top + 8 + 6 * (GetCharacterHeight(FontSize::Normal) + 3) + 1}, 0, 1);
                    CaptureNativeArtWindow(*window, paths.images / fmt::format("center-{}-selected-stock.ppm", ref.station.base()));
                }
            }
            // Passive shells retain their single actual pool/custody owner;
            // graphics queries never materialize another shell or packet list.
            for (const auto *pod : SpacePod::Iterate()) {
                pods.push_back({{"pod", pod->index.base()}, {"generation", pod->generation}, {"custody", to_underlying(pod->state.custody)},
                    {"cargo_quantity", pod->descent ? pod->descent->packets.AvailableCount() : 0}, {"descent", pod->descent != nullptr}});
            }
            CHECK(PaidLaunchDigests() == before); CHECK(TimerGameTick::counter == ticks);
            CHECK(std::array{_random.state[0], _random.state[1]} == seeds);
            CHECK(Company::Get(_local_company)->money == money); CHECK(Company::Get(_local_company)->current_loan == loan);
            std::ofstream output(paths.reports / "paid-native-observation.json");
            output << nlohmann::json{{"ordinary_paid_world_loaded", true}, {"component_pose_inputs", false}, {"observation_scope", "read_only_capture_after_optional_paid_scenario"}, {"gameplay_commands", 0}, {"simulation_ticks", 0},
                {"paid_scenario", paid_scenario}, {"owner_digests_before", before}, {"owner_digests_after", PaidLaunchDigests()}, {"cash", int64_t{money}}, {"loan", int64_t{loan}},
                {"crafts", crafts}, {"centers", centers}, {"pods", pods}, {"visual_review_pending", true}}.dump(2) << '\n';
            REQUIRE(output.good()); observed = true;
        } catch (...) { failure = std::current_exception(); }
        _exit_game = true;
    };
    AutoRestoreBackup observer(paid_review_observer, &observe); IConsole::CmdRegister("launch_fleet_paid_review", ObservePaidLaunchReview);
    const std::string config_arg = config.string();
    std::array<std::string_view, 17> args = {"openttd_space_art_test.exe", "-v", "null:ticks=2", "-b", "8bpp-optimized", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
    CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
