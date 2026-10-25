/* This file is part of OpenTTD. Licensed under the GNU GPL version 2. */
/** @file merged_runtime_gallery.cpp Zero-tick native views of exact retained visible worlds.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md
 * No world, cargo, date, physical pose or purchase is supplied by this observer.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include <any>
#include <charconv>
#include <filesystem>
#include <set>
#include "normal_ground_runtime_fixture.h"
#include "native_art_render_fixture.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../space_structure.h"
#include "../launch_center.h"
#include "../geothermal_generator.h"
#include "../geothermal_riser.h"
#include "../vehicle_gui.h"
#include "../vehicle_func.h"
#include "../core/string_consumer.hpp"
#include "../safeguards.h"

static std::function<void()> *gallery_observer = nullptr; ///< Test-only native startup hook; no saved owner.

/** @param args Native zero-argument startup command. @return Whether this invocation's observer ran. */
static bool ObserveMergedGallery(std::span<std::string_view> args)
{
    REQUIRE(args.size() == 1); REQUIRE(gallery_observer != nullptr); (*gallery_observer)(); return true;
}

/** @param dialect Explicit A/B original lineage, or merged. @param digest Independent whole original SHA256. @return Same-attempt original hint; merged files need none. */
static std::optional<SaveOriginHint> GalleryOrigin(std::string_view dialect, std::string_view digest)
{
    if (dialect == "merged") return std::nullopt;
    REQUIRE((dialect == "A" || dialect == "B")); REQUIRE(digest.size() == 64);
    SaveOriginHint hint{dialect == "A" ? SaveDialect::DestinationA : SaveDialect::DonorB, {}};
    // Bind every original digest byte; a filename cannot choose a collision dialect.
    for (size_t index = 0; index < hint.source_digest.size(); ++index) {
        unsigned value = 0; const auto *begin = digest.data() + 2 * index;
        const auto parsed = std::from_chars(begin, begin + 2, value, 16);
        REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == begin + 2); hint.source_digest[index] = static_cast<uint8_t>(value);
    }
    return hint;
}

/** @param path Newly owned full native boundary. Write actual owners without a gameplay opportunity. */
static void SaveGalleryBoundary(const std::filesystem::path &path)
{
    AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
    AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
    REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** @param train Actual original visible capsule. @param zoom Native view scale. Verify real picking and foreign-layer refusal without moving it. */
static void PickGalleryCapsule(const Train &train, ZoomLevel zoom)
{
    Viewport view{}; view.width = 640; view.height = 360; view.zoom = zoom; view.layer = GetVehicleDisplayLayer(&train);
    view.virtual_left = (train.coord.left + train.coord.right) / 2 - ScaleByZoom(320, zoom);
    view.virtual_top = (train.coord.top + train.coord.bottom) / 2 - ScaleByZoom(180, zoom);
    const auto *picked = CheckClickOnVehicle(view, 320, 180); REQUIRE(picked != nullptr); CHECK(picked->First() == &train);
    view.layer = MapLayer::Surface; const auto *foreign = CheckClickOnVehicle(view, 320, 180);
    // A coincident Surface object cannot acquire this rock-plane capsule's selection.
    CHECK((foreign == nullptr || foreign->First() != &train));
}

/** Exact originals produce native viewport pixels while preserving every serialized owner and clock. */
TEST_CASE("Merged runtime gallery draws exact retained worlds without work", "[.merged-runtime-gallery]")
{
    const char *source = std::getenv("OTTD_GALLERY_SOURCE"), *digest = std::getenv("OTTD_GALLERY_DIGEST");
    const char *origin = std::getenv("OTTD_GALLERY_ORIGIN"), *kind = std::getenv("OTTD_GALLERY_KIND");
    const char *bootstrap = std::getenv("OTTD_GALLERY_BOOTSTRAP");
    REQUIRE(source != nullptr); REQUIRE(digest != nullptr); REQUIRE(origin != nullptr); REQUIRE(kind != nullptr); REQUIRE(bootstrap != nullptr);
    const auto paths = GetTestCaseArtifactPaths("multilayer-art", "merged-runtime-gallery");
    const auto hint = GalleryOrigin(origin, digest); bool observed = false; std::exception_ptr failure;
    /** Capture only ordinary loaded state; all physical setup belongs to the retained input. */
    std::function<void()> capture = [&] {
        try {
            REQUIRE(SaveOrLoad(source, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false, hint) == SaveLoadResult::Ok);
            REQUIRE(_pause_mode.Any()); AutoRestoreBackup actor(_current_company, _local_company);
            REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
            SaveGalleryBoundary(paths.Output("saves", "before.sav")); SaveGalleryBoundary(paths.Output("saves", "before-repeat.sav"));
            std::vector<std::pair<std::string, TileIndex>> focuses;
            Train *capsule = nullptr;
            if (std::string_view{kind} == "capsule") {
                const char *choice = std::getenv("OTTD_GALLERY_MODEL"); REQUIRE(choice != nullptr);
                const auto model = ParseInteger<uint16_t>(choice); REQUIRE(model.has_value());
                // Select the actual original model and visible physical front;
                // no replacement hull, refit, pose input or cargo is created.
                for (auto *vehicle : Vehicle::Iterate()) {
                    if (vehicle->type != VehicleType::Train) continue;
                    auto *train = Train::From(vehicle);
                    if (train->engine_type != GetTubeEngine(*model)) continue;
                    if (!train->IsPrimaryVehicle()) continue;
                    if (train->vehstatus.Test(VehState::Hidden)) continue;
                    capsule = train; break;
                }
                REQUIRE(capsule != nullptr); REQUIRE(capsule->cargo.StoredCount() > 0);
                focuses.emplace_back(fmt::format("capsule-{}", *model), capsule->tile);
                fmt::print("MERGED_GALLERY_CAPSULE model={} vehicle={} cargo={} tile={} tick={} pose_supplied=0\n", *model, capsule->index.base(), capsule->cargo.StoredCount(), capsule->tile.base(), TimerGameTick::counter);
            } else if (std::string_view{kind} == "geothermal") {
                // Every actual retained plant/riser supplies its own tile/state;
                // the renderer alone decides its construction/working/wait art.
                for (const auto *industry : Industry::Iterate()) {
                    if (GetGeothermalGeneratorState(industry->index) != nullptr) focuses.emplace_back(fmt::format("generator-{}", industry->index.base()), industry->location.tile);
                    if (GetGeothermalRiserState(industry->index) != nullptr) focuses.emplace_back(fmt::format("riser-{}", industry->index.base()), industry->location.tile);
                }
            } else {
                REQUIRE(std::string_view{kind} == "space"); std::set<SpaceBuildKind> seen;
                // Capture one actual original of every present Space/orbital
                // kind, then each existing Surface launch anchor; no site is built.
                for (const auto *site : SpaceStructure::Iterate()) {
                    if (!seen.insert(site->state.kind).second) continue;
                    focuses.emplace_back(fmt::format("space-{}", to_underlying(site->state.kind)), TileXYL(site->state.x, site->state.y, MapLayer::Space));
                }
                for (const auto &center : CaptureLaunchCenters()) focuses.emplace_back(fmt::format("launch-{}", center.station.base()), center.anchor.tile);
            }
            REQUIRE_FALSE(focuses.empty());
            // Both native zooms and consecutive full rasters expose clipping,
            // casing occlusion and observation-induced changes independently.
            for (const auto &[name, tile] : focuses) {
                for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
                    for (uint repeat = 0; repeat < 2; ++repeat) CaptureNativeArtViewport(tile, zoom,
                        paths.images / fmt::format("{}-zoom-{}-{}.ppm", name, to_underlying(zoom), repeat));
                    if (capsule != nullptr) PickGalleryCapsule(*capsule, zoom);
                }
            }
            SaveGalleryBoundary(paths.Output("saves", "after.sav")); SaveGalleryBoundary(paths.Output("saves", "after-repeat.sav"));
            fmt::print("MERGED_GALLERY kind={} focuses={} simulation_ticks=0 original_pose=true\n", kind, focuses.size()); observed = true;
        } catch (...) { failure = std::current_exception(); }
        // The null driver must finish with its own painter after a capture or
        // refusal, rather than hiding the original exception in a repaint fault.
        BlitterFactory::SelectBlitter("null"); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
        _exit_game = true;
    };
    AutoRestoreBackup hook(gallery_observer, &capture); IConsole::CmdRegister("merged_gallery", ObserveMergedGallery);
    const auto config = GetTestRuntimeConfig().string();
    std::array<std::string_view, 17> arguments{"openttd_ground_viewport_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
        "-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", bootstrap};
    CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); REQUIRE(observed);
}
