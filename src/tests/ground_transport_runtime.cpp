/**
 * @file ground_transport_runtime.cpp Actual conventional transport UI and viewport source observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20BB; mode-specific current capture ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHC.
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
 * Native company-availability boundary fixtures qualify presentation only;
 * driven routes, complete map/foreground proof and economic acceptance remain separate.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../3rdparty/nlohmann/json.hpp"
#include "test_artifact_paths.h"
#include "native_art_render_fixture.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../underground_vehicle.h"
#include "../engine_func.h"
#include "../engine_gui.h"
#include "../engine_base.h"
#include "../vehicle_func.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../timer/timer_game_tick.h"
#include "../core/random_func.hpp"
#include "../multilayer_sprite.h"
#include "ground_transport_runtime_fixture.h"

#include "../safeguards.h"

static std::function<void()> *ground_transport_observer = nullptr; ///< Isolated native startup callback, without saved state.

/**
 * Observe this process's actual loaded native transport presentation.
 * @param arguments Parameter-free startup command; no external simulation input is accepted.
 * @return True after the explicit native callback runs.
 */
static bool ObserveGroundTransport(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(ground_transport_observer != nullptr);
	(*ground_transport_observer)();
	return true;
}

/**
 * Capture actual available/locked purchase renderers for real native conventional identities.
 * @param paths This process's isolated capture/report directories.
 * @return Actual source/UI observations; map/body/foreground coverage is not inferred.
 */
static nlohmann::json CaptureGroundTransportPurchases(const TestArtifactPaths &paths)
{
	nlohmann::json frames = nlohmann::json::array();
	// Resolve real engine mappings first. Catalogue numbers select only this
	// art scope; they are never cast into runtime pool or GRF-local identities.
	for (uint16_t model = 4; model <= 8; ++model) {
		const auto id = GetUndergroundEngine(model);
		REQUIRE(Engine::IsValidID(id));
		Engine &engine = *Engine::Get(id);
		const auto *capability = GetUndergroundEngineCapability(id);
		REQUIRE(capability != nullptr);
		REQUIRE(capability->model_id == model);
		REQUIRE(CheckUndergroundVehicleCapability(id, MapLayer::Underground).Succeeded());
		AutoRestoreBackup availability(engine.company_avail);
		// These two explicit company-flag boundaries feed the actual existing
		// IsEngineBuildable owner. The native date/profile gate remains active;
		// no renderer grants permission by selecting an unlocked picture.
		for (bool available : {true, false}) {
			if (available) {
				engine.company_avail.Set(_local_company);
			} else {
				engine.company_avail.Reset(_local_company);
			}
			const bool buildable = IsEngineBuildable(id, engine.type, _local_company);
			REQUIRE(buildable == available);
			REQUIRE(GetMultiLayerEnginePurchaseSprite(id, buildable).has_value());
			const auto basename = fmt::format("purchase-model-{:02}-{}", model, available ? "available" : "locked");
			const auto money = Company::Get(_current_company)->money;
			const auto random = _random;
			const auto interactive = _interactive_random;
			const auto clock = TimerGameTick::counter;
			const auto company_flags = engine.company_avail;
			const auto palette = GetEnginePalette(id, _local_company);
			/** Draw the actual purchase/list engine renderer at its ordinary native GUI baseline. */
			const std::function<void()> paint = [&] {
				DrawVehicleEngine(0, 159, 80, 40, id, palette, EngineImageType::Purchase);
			};
			CaptureNativeArtCanvas(160, 80, paths.images / (basename + ".ppm"), paint);
			CHECK(Company::Get(_current_company)->money == money);
			CHECK(TimerGameTick::counter == clock);
			CHECK(engine.company_avail == company_flags);
			CHECK(_random.state[0] == random.state[0]);
			CHECK(_random.state[1] == random.state[1]);
			CHECK(_interactive_random.state[0] == interactive.state[0]);
			CHECK(_interactive_random.state[1] == interactive.state[1]);
			frames.push_back({{"asset_id", fmt::format("purchase.model-{:02}", model)}, {"model", model}, {"engine", id.base()},
				{"available", buildable}, {"basename", basename}, {"palette", palette}, {"image_type", "Purchase"},
				{"width", 160}, {"height", 80}, {"baseline", {80, 40}}});
		}
	}
	return frames;
}

/** Actual native engine/company owners produce current conventional purchase views without changing simulation state. */
TEST_CASE("Current conventional transport observer draws actual native purchase guards", "[.ground-transport-runtime]")
{
	const auto root = FindTestRepositoryRoot();
	const char *source_value = std::getenv("OTTD_GROUND_TRANSPORT_SOURCE");
	REQUIRE(source_value != nullptr);
	const auto source = ResolveTestInputPath(std::filesystem::path{source_value});
	const char *mode = std::getenv("OTTD_GROUND_TRANSPORT_MODE");
	const std::string scope = mode == nullptr ? "purchases" : mode;
	// Each real bounded mode keeps separate output/stage ownership inside one
	// selected graph. Unknown modes cannot create an arbitrary artifact namespace.
	REQUIRE((scope == "purchases" || scope == "rail" || scope == "road"));
	const auto paths = GetTestArtifactPaths("selected-ground", "transport-" + scope);
	const auto config = GetTestRuntimeConfig();
	bool observed = false;
	std::exception_ptr failure;
	/** Observe the actual loaded UI, retaining explicit company availability fixtures and draw-only purity. */
	std::function<void()> capture = [&] {
		try {
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			REQUIRE(Company::IsValidID(_local_company));
			AutoRestoreBackup actor(_current_company, _local_company);
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			const bool bodies = scope != "purchases";
			const auto frames = bodies ? CaptureGroundTransportBodies(paths, scope == "road") : CaptureGroundTransportPurchases(paths);
			REQUIRE(frames.size() == (bodies ? (scope == "road" ? 210 : 289) : 10));
			nlohmann::json palette = nlohmann::json::array();
			// Preserve the native display palette for later exact source/UI RGB
			// comparison, including existing company remaps and locked source art.
			for (const auto &colour : _cur_palette.palette) palette.push_back({colour.r, colour.g, colour.b});
			nlohmann::json report{{"_documentation", "@see r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHC; native purchase views only"},
				{"frames", frames}, {"palette", palette}, {"fixture_company_availability", true},
				{"map_body_coverage", bodies}, {"station_foreground_coverage", false}, {"source_pixel_verification", "pending"},
				{"drawing_owners_unchanged", true}, {"transported_bootstrap_claimed", false}};
			std::ofstream output(paths.reports / ("transport-" + scope + ".json"), std::ios::binary);
			output << report.dump(2) << '\n';
			output.close();
			REQUIRE(output.good());
			if (bodies) {
				fmt::print("GROUND_TRANSPORT_BODIES scope={} frames={} packet_owners=true\n", scope, frames.size());
			} else {
				fmt::print("GROUND_TRANSPORT_PURCHASES models=5 actual_engine_views=10 company_date_guards=true map_coverage=pending\n");
			}
		} catch (...) {
			failure = std::current_exception();
		}
		// A failing observer must still restore the headless backend before
		// its final dirty-window pass, preserving the original diagnostic.
		BlitterFactory::SelectBlitter("null");
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		observed = true;
		_exit_game = true;
	};
	AutoRestoreBackup observer(ground_transport_observer, &capture);
	IConsole::CmdRegister("ground_transport_art", ObserveGroundTransport);
	const std::string config_path = config.string();
	const std::string source_path = source.string();
	std::array<std::string_view, 17> arguments{"openttd_ground_viewport_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple",
		"-I", "original_dos", "-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
