/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_collector_runtime.cpp Native collector viewport and current-save proof on disclosed fixture floor/materials.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79C
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "test_artifact_paths.h"
#include "layer_save_fixture.h"
#include "native_art_render_fixture.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include "../geothermal_collector.h"
#include "../industry.h"
#include "../multilayer_sprite.h"
#include "../utility_power.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../transparency.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

static std::function<void()> *_collector_art_observer = nullptr; ///< Isolated test hook; the production renderer has no observer state.

/**
 * Invoke the observer only after ordinary native loading resolves all saved owners.
 * @param arguments Exact zero-argument startup command; no external gameplay mutation is accepted.
 * @return Whether the isolated observer ran.
 */
static bool ObserveCollectorArt(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_collector_art_observer != nullptr);
	(*_collector_art_observer)(); return true;
}

/**
 * Capture both zooms and a current native checkpoint without any rendering side effects.
 * @param industry Actual loaded collector; native construction and Heat owners select its state.
 * @param name Declared state basename under this invocation's feature review directory.
 * @param transparent Request native industry transparency for the two rendered frames.
 */
static void CaptureCollectorState(const Industry &industry, std::string_view name, bool transparent = false)
{
	const auto paths = GetTestArtifactPaths("geothermal", "collector-art");
	const auto construction = ComputeGeothermalCollectorDigest(), sites = ComputeGeothermalSiteDigest();
	const auto utility = ComputeUtilityPowerDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto money = Company::Get(_local_company)->money;
	const auto tick = TimerGameTick::counter;
	SavedRandomSeeds before; SaveRandomSeeds(&before);
	const auto centre = TileAddXY(industry.location.tile, 1, 1);
	TransparencyOptions options = _transparency_opt;
	options.Set(TransparencyOption::Industries, transparent);
	AutoRestoreBackup transparency(_transparency_opt, options);
	AutoRestoreBackup invisibility(_invisibility_opt, TransparencyOptions{});
	// Both real viewport zooms exercise DrawTile_Industry and its sorter. The
	// selected camera/visual phase cannot earn work, material or thermal output.
	for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
		CaptureNativeArtViewport(centre, zoom, paths.images / fmt::format("collector-{}-zoom-{}.ppm", name, to_underlying(zoom)));
	}
	SavedRandomSeeds after; SaveRandomSeeds(&after);
	CHECK(std::ranges::equal(before.random.state, after.random.state));
	CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
	CHECK(ComputeGeothermalCollectorDigest() == construction); CHECK(ComputeGeothermalSiteDigest() == sites);
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_local_company)->money == money); CHECK(TimerGameTick::counter == tick);
	// The same genuine current-format checkpoint is independently loaded by
	// the ordinary game. Camera position is saved in Surface coordinates; the
	// ordinary runner explicitly scrolls to the reported Deep world tile.
	SetViewportLayer(GetMainWindow(), MapLayer::DeepUnderground);
	ScrollMainWindowToTile(centre, true);
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("collector-{}.sav", name)), false) == SaveLoadResult::Ok);
}

/** Actual funding is retained; explicit fixture deliveries/work precede capture and never claim the later Surface-transported first plant. */
TEST_CASE("Native collector renders real construction completion and shared Heat", "[.collector-art-runtime]")
{
	const auto source = ResolveTestInputPath(FindTestRepositoryRoot() / "src/tests/fixtures/geothermal/fixture389-collector-funded-read.sav");
	const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Resolve native owners, deliver the disclosed fixture bill, and freeze each earned stage before drawing. */
	std::function<void()> capture = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			AutoRestoreBackup pause(_pause_mode, PauseModes{});
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			const auto rows = CaptureGeothermalCollectors(); REQUIRE(rows.size() == 1); REQUIRE(rows[0].work_ticks == 0);
			Industry &industry = *Industry::Get(rows[0].industry);
			fmt::print("COLLECTOR_ART_LOCATION origin={} centre={} layer={}\n", industry.location.tile.base(),
				TileAddXY(industry.location.tile, 1, 1).base(), to_underlying(GetTileLayer(industry.location.tile)));
			REQUIRE(GetMultiLayerCollectorSprite(industry.location.tile, false, 0).has_value());
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			CaptureCollectorState(industry, "construction-0");
			// Exactly the frozen recipe enters native accepted stock. This is an
			// isolated art prerequisite, with no claimed procurement or carriage.
			for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->inputs) {
				REQUIRE(ReceiveGeothermalCollectorCargo(industry, GetCargoTypeByLabel(input.label), input.quantity) == input.quantity);
			}
			// Native synchronized construction owns all 296 labour ticks, actual
			// stage flags and the material receipt; the renderer only observes them.
			for (uint16_t tick = 1; tick <= 296; ++tick) {
				++TimerGameTick::counter; AdvanceGeothermalCollector(industry);
				if (tick % 74 == 0) {
					const auto name = tick == 296 ? std::string{"ready"} : fmt::format("construction-{}", tick / 74);
					CaptureCollectorState(industry, name);
				}
			}
			REQUIRE(IsGeothermalCollectorOperational(industry.index));
			REQUIRE(PublishGeothermalCollectorHeat(industry.index, GetUtilityPowerInterval(), 1) == UtilityPowerError::None);
			CaptureCollectorState(industry, "working");
			CaptureCollectorState(industry, "transparent", true);
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			fmt::print("COLLECTOR_ART_RUNTIME frames=14 construction_stages=4 completed=true current_heat=true render_owners_unchanged=true fixture_floor=true fixture_materials=true transported_bootstrap=false\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_collector_art_observer, &capture);
	IConsole::CmdRegister("collector_art", ObserveCollectorArt);
	const std::string config_path = config.string(), source_path = source.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
