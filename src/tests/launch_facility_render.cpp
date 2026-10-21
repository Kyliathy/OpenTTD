/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_facility_render.cpp Actual native launch viewport qualification.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JB
 * The hash-qualified paid host loads through the ordinary engine and real sprite
 * cache. Rendering earns no simulation opportunities or resource grants.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../openttd.h"
#include "../gfxinit.h"
#include "../blitter/factory.hpp"
#include "../launch_center.h"
#include "../launch_operations.h"
#include "../launch_facility_sprite.h"
#include "../space_vehicle.h"
#include "../vehicle_gui.h"
#include "../spritecache.h"
#include "../timer/timer_game_tick.h"
#include "../core/random_func.hpp"
#include "../space_transition.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../clear_map.h"
#include "../3rdparty/nlohmann/json.hpp"
#include "native_art_render_fixture.h"
#include "../safeguards.h"

static std::function<void()> *launch_render_observer = nullptr; ///< Invocation callback, with no saved state.

/** Run the isolated capture after the engine loads its declared world.
 * @param args Parameter-free startup command. @return Whether the callback ran.
 */
static bool ObserveLaunchRender(std::span<std::string_view> args)
{
    REQUIRE(args.size() == 1); REQUIRE(launch_render_observer); (*launch_render_observer)(); return true;
}

/** Restore explicit component tile inputs through their public Tile fields. */
class LaunchRenderTile {
    Tile tile; ///< Actual borrowed cell, never a second map owner.
    std::array<uint16_t, 10> fields; ///< Original public field values restored on every exit.
public:
    /** @param index Existing paid world's cell temporarily used by this render component. */
    explicit LaunchRenderTile(TileIndex index) : tile(index), fields{tile.type(), tile.height(), tile.m1(), tile.m2(), tile.m3(), tile.m4(), tile.m5(), tile.m6(), tile.m7(), tile.m8()} {}
    /** Restore the original world even when a native rendering assertion fails. */
    ~LaunchRenderTile()
    {
        tile.type() = static_cast<uint8_t>(fields[0]); tile.height() = static_cast<uint8_t>(fields[1]);
        tile.m1() = static_cast<uint8_t>(fields[2]); tile.m2() = fields[3]; tile.m3() = static_cast<uint8_t>(fields[4]);
        tile.m4() = static_cast<uint8_t>(fields[5]); tile.m5() = static_cast<uint8_t>(fields[6]); tile.m6() = static_cast<uint8_t>(fields[7]);
        tile.m7() = static_cast<uint8_t>(fields[8]); tile.m8() = fields[9];
    }
};

/** Capture the four-view matrix as explicit inputs to the actual native sorter.
 * @param center Existing paid facility whose own generation remains unchanged.
 * @param craft Existing paid rocket, borrowed without duplicating its inventory.
 * @param worker Existing paid builder supplying explicit work/pause component inputs.
 * @param paths Invocation-owned images and machine-readable observation locations.
 */
static void CaptureLaunchMatrix(Station &center, SpaceVehicle &craft, SpaceVehicle &worker, const TestArtifactPaths &paths)
{
    REQUIRE(craft.transition); REQUIRE(center.launch_operations);
    const auto tick_before = TimerGameTick::counter;
    const auto random_before = std::array{_random.state[0], _random.state[1]};
    const auto flags_before = craft.vehstatus;
    const auto owner_before = ComputeSpaceVehicleDigest();
    const auto flight_before = ComputeSpaceTransitionDigest();
    const auto center_before = ComputeLaunchCenterDigest();
    // Disable only optional UI labels: their opaque signs are separate from
    // source hardware and would otherwise cover pixels under qualification.
    AutoRestoreBackup display(_display_opt);
    _display_opt = {};
    nlohmann::json observations = nlohmann::json::array();
    {
        AutoRestoreBackup anchor(*center.launch_center);
        AutoRestoreBackup slots(*center.launch_operations);
        AutoRestoreBackup construction(craft.construction);
        AutoRestoreBackup flight(*craft.transition);
        AutoRestoreBackup worker_input(worker.construction);
        std::vector<std::unique_ptr<LaunchRenderTile>> patch;
        REQUIRE(Map::SizeX() >= 48); REQUIRE(Map::SizeY() >= 48);
        center.launch_center->tile = TileXYL(40, 40, MapLayer::Surface);
        const auto tile = center.launch_center->tile;
        const auto height = TileHeight(tile);
        // Rotation swaps 4x3 and 3x4. Preserve the complete bounded union first;
        // public native map setters install declared render inputs only. No save,
        // command completion, resource grant or paid-game acceptance is claimed.
        for (uint y = 0; y < 5; ++y) {
            for (uint x = 0; x < 5; ++x) patch.push_back(std::make_unique<LaunchRenderTile>(TileXYL(TileX(tile) + x, TilePlaneY(tile) + y, MapLayer::Surface)));
        }
        // A distant bounded render patch avoids unrelated paid trains and signs
        // crossing source masks. Preserve one extra corner row/column so its
        // explicit flat component floor does not borrow outside terrain heights.
        for (uint y = 0; y < 5; ++y) {
            for (uint x = 0; x < 5; ++x) SetTileHeight(TileXYL(40 + x, 40 + y, MapLayer::Surface), height);
        }
        const auto home = GetLaunchCenterReference(center);
        const auto ref = GetSpaceCraftReference(craft);
        // Each orientation uses the same native twelve-cell/zone producer and
        // exact retained hull. Scene mutations are explicit test inputs restored
        // below, independent of ordinary paid gameplay qualification in62JD.
        for (uint8_t rotation = 0; rotation < 4; ++rotation) {
            center.launch_center->rotation = rotation;
            for (uint y = 0; y < 4; ++y) {
                for (uint x = 0; x < 4; ++x) MakeClear(TileXYL(TileX(tile) + x, TilePlaneY(tile) + y, MapLayer::Surface), ClearGround::Grass, 3);
            }
            const auto footprint = *GetLaunchCenterFootprint(tile, rotation);
            for (auto cell : footprint) MakeStation(cell, center.owner, center.index, StationType::LaunchCenter, rotation);
            const auto pad = *GetLaunchFacilityZone(tile, rotation, LaunchFacilityZone::Pad);
            const auto recovery = *GetLaunchFacilityZone(tile, rotation, LaunchFacilityZone::Recovery);
            const auto workshop = *GetLaunchFacilityZone(tile, rotation, LaunchFacilityZone::Workshop);
            for (std::string_view scene : {"empty", "pad-ready", "pad-preparing", "pad-ascent0", "pad-ascent1", "recovery-turnaround", "workshop-working", "workshop-blocked", "stale-pad"}) {
                auto &operations = *center.launch_operations;
                operations = {}; operations.pad_active = ref;
                craft.construction.home = home; craft.construction.placement = SpacePlacement::InTransit;
                craft.construction.surface_tile = scene == "recovery-turnaround" ? recovery.tile : pad.tile;
                auto &transition = *craft.transition; transition.recovery = home; transition.layer = MapLayer::Surface;
                transition.phase = scene == "recovery-turnaround" ? SpaceFlightPhase::Turnaround : SpaceFlightPhase::Preparing;
                transition.blocked = SpaceFlightBlock::None;
                // These two explicit source inputs exercise the original native
                // funded countdown cadence; no fuel packet is spent or fabricated.
                if (scene == "pad-ascent0" || scene == "pad-ascent1") {
                    transition.phase = SpaceFlightPhase::Ascent; transition.outbound_burned = true;
                    transition.recovery_burned = false; transition.remaining_ticks = scene == "pad-ascent0" ? 222 : 214;
                }
                if (scene == "empty") operations.pad_active = {};
                if (scene == "pad-ready") {
                    craft.construction.placement = SpacePlacement::SurfaceStored;
                    transition.phase = SpaceFlightPhase::GroundLoading;
                    operations.pad_active = {}; operations.pad_queue.push_back(ref);
                }
                if (scene == "stale-pad") ++operations.pad_active.generation;
                // The real selector sees a declared unfinished owned workshop
                // input. Its stock, cargo and actual paid progress remain unchanged.
                if (scene == "workshop-working" || scene == "workshop-blocked") {
                    operations.workshop = GetSpaceCraftReference(worker);
                    worker.construction.home = home; worker.construction.placement = SpacePlacement::Assembling;
                    worker.construction.surface_tile = workshop.tile; worker.construction.remaining_ticks = 2;
                    worker.construction.paused = scene == "workshop-blocked";
                }
                craft.UpdateSpacePosition();
                const auto name = fmt::format("view-{}-{}.ppm", rotation, scene);
                const auto corner = RemapCoords(TileX(tile) * TILE_SIZE, TileY(tile) * TILE_SIZE, TilePixelHeight(tile));
                const auto camera = RemapCoords(TileX(tile) * TILE_SIZE + 8, TileY(tile) * TILE_SIZE + 8, TilePixelHeight(tile));
                const auto contact = RemapCoords(craft.x_pos, craft.y_pos, craft.z_pos);
                const auto pose = GetLaunchRocketArtPose(craft, GetLaunchRocketArtDirection(craft)); REQUIRE(pose);
                const auto before = ComputeSpaceVehicleDigest(); const auto transition_before = ComputeSpaceTransitionDigest();
                CaptureNativeArtViewport(tile, ZoomLevel::Normal, paths.images / name, paths.images / (name + ".indexed"));
                CHECK(ComputeSpaceVehicleDigest() == before); CHECK(ComputeSpaceTransitionDigest() == transition_before);
                Viewport picking{}; picking.width = 640; picking.height = 360; picking.zoom = ZoomLevel::Normal; picking.layer = MapLayer::Surface;
                picking.virtual_left = camera.x - ScaleByZoom(320, picking.zoom); picking.virtual_top = camera.y - ScaleByZoom(180, picking.zoom);
                const Point click{UnScaleByZoom((craft.coord.left + craft.coord.right) / 2 - picking.virtual_left, picking.zoom), UnScaleByZoom((craft.coord.top + craft.coord.bottom) / 2 - picking.virtual_top, picking.zoom)};
                const auto picked = CheckClickOnVehicle(picking, click.x, click.y);
                CHECK((picked == &craft) == IsLaunchSurfaceCraftArtVisible(craft));
                picking.layer = MapLayer::Space; CHECK(CheckClickOnVehicle(picking, click.x, click.y) != &craft);
                const auto art_bounds = GetLaunchCraftArtBounds(craft);
                CHECK(art_bounds.origin.x == craft.bounds.origin.x); CHECK(art_bounds.origin.y == craft.bounds.origin.y);
                CHECK(art_bounds.origin.x + art_bounds.offset.x == 0); CHECK(art_bounds.origin.y + art_bounds.offset.y == 0);
                observations.push_back({{"image", name}, {"rotation", rotation}, {"scene", scene},
                    {"facility_contact", {320 + (corner.x - camera.x) / static_cast<int>(ZOOM_BASE), 180 + (corner.y - camera.y) / static_cast<int>(ZOOM_BASE)}},
                    {"rocket_contact", {320 + (contact.x - camera.x) / static_cast<int>(ZOOM_BASE), 180 + (contact.y - camera.y) / static_cast<int>(ZOOM_BASE)}},
                    {"rocket_visible", IsLaunchSurfaceCraftArtVisible(craft)}, {"state", pose->state}, {"frame", pose->frame}, {"direction", pose->direction}});
            }
            CaptureNativeArtViewport(TileXYL(40, 40, MapLayer::Space), ZoomLevel::Normal,
                paths.images / fmt::format("view-{}-space.ppm", rotation), paths.images / fmt::format("view-{}-space.indexed", rotation));
        }
    }
    craft.UpdateSpacePosition();
    CHECK(ComputeSpaceVehicleDigest() == owner_before); CHECK(ComputeSpaceTransitionDigest() == flight_before); CHECK(ComputeLaunchCenterDigest() == center_before);
    CHECK(TimerGameTick::counter == tick_before); CHECK(craft.vehstatus == flags_before);
    CHECK(std::array{_random.state[0], _random.state[1]} == random_before);
    std::ofstream output(paths.reports / "native-occupied-matrix.json");
    output << nlohmann::json{{"component_pose_inputs", true}, {"simulation_ticks", 0}, {"ordinary_game_reviewed", false}, {"observations", observations}}.dump(2) << '\n';
    REQUIRE(output.good());
}

/** Actual full viewport smoke qualification precedes the occupied component matrix. */
TEST_CASE("Native launch facility occupied viewport uses real source pixels", "[.launch-facility-render]")
{
    const char *source = std::getenv("OTTD_LAUNCH_RENDER_SOURCE"); REQUIRE(source);
    const auto paths = GetTestCaseArtifactPaths("multilayer-art", "launch-facility-render");
    const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
    /** Capture actual loaded native owners without granting a pose or simulation tick. */
    std::function<void()> observe = [&] {
        try {
            // The null video driver deliberately installs its null blitter at
            // startup. Select the real indexed encoder before reloading original
            // graphics, so black null output cannot masquerade as a raster proof.
            REQUIRE(BlitterFactory::SelectBlitter("8bpp-optimized"));
            GfxLoadSprites();
            REQUIRE(BlitterFactory::GetCurrentBlitter()->GetScreenDepth() == 8);
            REQUIRE_FALSE(GetLaunchCenterReferences().empty());
            // Each original registered facility supplies its own physical camera;
            // no fixture registry or copied source geometry chooses a different site.
            for (const auto ref : GetLaunchCenterReferences()) {
                const auto *center = ResolveLaunchCenter(ref); REQUIRE(center);
                CaptureNativeArtViewport(center->launch_center->tile, ZoomLevel::Normal,
                    paths.images / fmt::format("native-center-{}.ppm", ref.station.base()));
                fmt::print("LAUNCH_RENDER center={} rotation={} anchor={}\n", ref.station.base(), center->launch_center->rotation, center->launch_center->tile.base());
            }
            // Report the real retained physical owners before any later explicit
            // component inputs; this does not stand for ordinary paid-game review.
            for (const auto *craft : SpaceVehicle::Iterate()) {
                fmt::print("LAUNCH_RENDER craft={} model={} tile={} placement={} phase={} visible={}\n", craft->index.base(), GetSpaceVehicleModel(*craft), craft->tile.base(), to_underlying(craft->construction.placement), craft->transition ? to_underlying(craft->transition->phase) : 255, IsLaunchSurfaceCraftArtVisible(*craft));
            }
            auto *center = ResolveLaunchCenter(GetLaunchCenterReferences().front()); REQUIRE(center);
            SpaceVehicle *rocket = nullptr, *worker = nullptr;
            // The explicit source host owns exactly these paid roles. Reject a
            // missing/ambiguous hull rather than creating a component inventory.
            for (auto *candidate : SpaceVehicle::Iterate()) {
                if (GetSpaceVehicleModel(*candidate) == 14) { REQUIRE(rocket == nullptr); rocket = candidate; }
                if (GetSpaceVehicleModel(*candidate) == 19) { REQUIRE(worker == nullptr); worker = candidate; }
            }
            REQUIRE(rocket); REQUIRE(worker); CaptureLaunchMatrix(*center, *rocket, *worker, paths);
            observed = true;
        } catch (...) { failure = std::current_exception(); }
        _exit_game = true;
    };
    AutoRestoreBackup observer(launch_render_observer, &observe);
    IConsole::CmdRegister("launch_facility_render", ObserveLaunchRender);
    const std::string config_arg = config.string();
    std::array<std::string_view, 17> args = {"openttd_space_art_test.exe", "-v", "null:ticks=2", "-b", "8bpp-optimized", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
    CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
