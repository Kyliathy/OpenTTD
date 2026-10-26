/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file space_preview.cpp Motion, exact population, bounded indexing and real native demo save/reload.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md GroupP1
 */
#include "../stdafx.h"
#include "asteroid_snapshot_fixture.h"
#include <thread>
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../core/random_func.hpp"
#include "../vehicle_type.h"
#include "../multilayer_state.h"
#include "../company_func.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../settings_type.h"
#include "../saveload/saveload_func.h"
#include "../window_gui.h"
#include "../gui.h"
#include "../launch_center.h"
#include "../water_map.h"
#include "../clear_map.h"
#include "../void_map.h"
#include "../map_layer.h"
#include "../widgets/genworld_widget.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "layer_save_fixture.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include <set>
#include "../safeguards.h"

TEST_CASE("Space preserves historical vehicle type bytes and shared integer reflection", "[space-preview]")
{
	STATIC_REQUIRE(to_underlying(VehicleType::Train) == 0);
	STATIC_REQUIRE(to_underlying(VehicleType::Aircraft) == 3);
	STATIC_REQUIRE(to_underlying(VehicleType::Effect) == 4);
	STATIC_REQUIRE(to_underlying(VehicleType::Disaster) == 5);
	const auto band = GetSpaceTravelBand(64, 64);
	CHECK(band.min == SpacePoint{2 * SPACE_Q16, 18 * SPACE_Q16});
	CHECK(band.max == SpacePoint{61 * SPACE_Q16, 45 * SPACE_Q16});
	for (int32_t speed : {4096, 8192, 16384}) {
		SpaceMotion motion{{20 * SPACE_Q16, 30 * SPACE_Q16}, speed, 0, 0, 0};
		for (int i = 0; i < 74; ++i) AdvanceSpaceMotion(motion, band);
		CHECK(motion.position.x == 20 * SPACE_Q16 + speed);
		CHECK(motion.remainder_x == 0);
	}
	SpaceMotion corner{band.max, 16384, 16384, 73, 73};
	AdvanceSpaceMotion(corner, band);
	CHECK(corner.position == SpacePoint{band.max.x - 222, band.max.y - 222});
	CHECK(corner.velocity_x == -16384);
	CHECK(corner.velocity_y == -16384);
	CHECK(corner.remainder_x == 29);
	SpaceMotion exact{{band.max.x - 1, band.min.y}, 74, -74, 0, 0};
	AdvanceSpaceMotion(exact, band);
	CHECK(exact.position == SpacePoint{band.max.x, band.min.y + 1});
	CHECK(exact.velocity_x == -74);
	CHECK(exact.velocity_y == 74);
	CHECK(SpaceDistanceSquared({0, 0}, {4095 * SPACE_Q16, 4095 * SPACE_Q16}) == 2ULL * 4095 * 4095 * SPACE_Q16 * SPACE_Q16);
}

TEST_CASE("Asteroid population uses every explicit area row and independent generation streams", "[space-preview]")
{
	constexpr uint32_t rows[][3] = {{6,10,14},{12,20,28},{24,40,56},{48,80,112},{96,160,224},{192,320,448},
		{384,640,896},{768,1280,1792},{1536,2560,3584},{2304,3840,5376},{3072,5120,7168},{4608,7680,10752},{6144,10240,14336}};
	for (uint32_t x = 64; x <= 4096; x *= 2) for (uint32_t y = 64; y <= 4096; y *= 2) for (uint8_t d = 0; d < 3; ++d) {
		const auto row = std::countr_zero(x) + std::countr_zero(y) - 12;
		CHECK(GetMultiLayerAsteroidPopulation(x, y, d) == rows[row][d]);
	}
	CHECK(GetMultiLayerAsteroidPopulation(63, 64, 1) == 0);
	CHECK(GetMultiLayerAsteroidPopulation(128, 32, 1) == 0);
	CHECK(GetMultiLayerAsteroidPopulation(64, 64, 3) == 0);
	const auto simulation = _random, interactive = _interactive_random;
	for (AsteroidDensity density : {AsteroidDensity::Sparse, AsteroidDensity::Normal, AsteroidDensity::Dense}) {
		const auto field = GenerateAsteroidField(64, 64, 12345, density);
		REQUIRE(field);
		CHECK(ValidateAsteroidField(*field));
		CHECK(field == GenerateAsteroidField(64, 64, 12345, density));
		CHECK(field != GenerateAsteroidField(64, 64, 54321, density));
		std::array<uint32_t, 3> classes{};
		std::set<std::pair<int32_t, int32_t>> positions;
		for (const auto &body : field->bodies) {
			++classes[to_underlying(body.resource_class)];
			CHECK(positions.emplace(body.motion.position.x, body.motion.position.y).second);
			CHECK(field->band.Contains(body.motion.position));
			CHECK(body.initial_reserve == body.remaining_reserve);
			CHECK(body.initial_reserve >= 100);
		}
		if (density == AsteroidDensity::Sparse) CHECK(classes == std::array<uint32_t, 3>{3, 2, 1});
		if (density == AsteroidDensity::Normal) CHECK(classes == std::array<uint32_t, 3>{6, 3, 1});
	}
	CHECK(_random.state[0] == simulation.state[0]); CHECK(_random.state[1] == simulation.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]); CHECK(_interactive_random.state[1] == interactive.state[1]);
	const auto maximum = GenerateAsteroidField(4096, 4096, 1, AsteroidDensity::Dense);
	REQUIRE(maximum);
	CHECK(maximum->bodies.size() == 14336);
	CHECK(ValidateAsteroidField(*maximum));
	CHECK_FALSE(GenerateAsteroidField(64, 32, 1, AsteroidDensity::Dense));
}

TEST_CASE("Space buckets move clustered IDs in bounded work and return stable local results", "[space-preview]")
{
	SpaceSpatialIndex index;
	index.Reset(4096, 4096, MAX_ORBITAL_BODIES);
	for (uint32_t i = 0; i < MAX_ORBITAL_BODIES; ++i) index.Update(OrbitalBodyID{i}, {10 * SPACE_Q16, 10 * SPACE_Q16});
	for (uint32_t i = 0; i < MAX_ORBITAL_BODIES; ++i) index.Update(OrbitalBodyID{i}, {30 * SPACE_Q16, 30 * SPACE_Q16});
	CHECK(index.Query({0, 0}, {15 * SPACE_Q16, 15 * SPACE_Q16}).empty());
	const auto results = index.Query({16 * SPACE_Q16, 16 * SPACE_Q16}, {31 * SPACE_Q16, 31 * SPACE_Q16});
	REQUIRE(results.size() == MAX_ORBITAL_BODIES);
	CHECK(std::ranges::is_sorted(results));
	CHECK(results.front().base() == 0);
	CHECK(results.back().base() == MAX_ORBITAL_BODIES - 1);
}

TEST_CASE("Predeployed survey probes pursue attach depart and repeat without extracting resources", "[space-preview]")
{
	const auto generated = GenerateAsteroidField(64, 64, 77, AsteroidDensity::Normal);
	REQUIRE(generated);
	REQUIRE(InstallAsteroidField(*generated));
	REQUIRE(PrepareSpaceSurveyPreview());
	auto field = GetAsteroidField();
	std::set<SpaceSurveyPhase> phases;
	for (uint32_t i = 0; i < 3000; ++i) {
		AdvanceContainedAsteroidSnapshot(field);
		REQUIRE(ValidateAsteroidField(field));
		for (const auto &survey : field.surveys) phases.insert(survey.phase);
	}
	CHECK(phases.size() == 4);
	for (const auto &survey : field.surveys) CHECK(survey.completed_surveys >= 2);
	for (const auto &body : field.bodies) CHECK(body.remaining_reserve == generated->bodies[body.id.base()].remaining_reserve);
	// Stable first-ID wins one slot; the second controller never steals it.
	field = *generated;
	const auto point = field.bodies.front().motion.position;
	const SpacePoint attachment{point.x + SPACE_Q16, point.y - SPACE_Q16 / 2};
	field.surveys = {{SpaceSurveyID{0}, 15, attachment, attachment, OrbitalBodyID{0}}, {SpaceSurveyID{1}, 15, attachment, attachment, OrbitalBodyID{0}}};
	AdvanceContainedAsteroidSnapshot(field);
	CHECK(field.bodies.front().survey_slot == SpaceSurveyID{0});
	CHECK(field.surveys[1].phase == SpaceSurveyPhase::Approach);
	CHECK(ValidateAsteroidField(field));
	field.bodies.front().motion.position = field.band.max;
	field.bodies.front().motion.velocity_x = 16384; field.bodies.front().motion.velocity_y = 0;
	field.surveys.front().position = {field.band.max.x + SPACE_Q16, field.band.max.y - SPACE_Q16 / 2};
	AdvanceContainedAsteroidSnapshot(field);
	CHECK(field.bodies.front().motion.velocity_x == 16384); // The historical placement strip is not a physical map edge.
	CHECK(field.surveys.front().position == SpacePoint{field.bodies.front().motion.position.x + SPACE_Q16, field.bodies.front().motion.position.y - SPACE_Q16 / 2});
	CHECK(ValidateAsteroidField(field));
	field.surveys.front().remaining_ticks = 0;
	CHECK_FALSE(ValidateAsteroidField(field));
	ResetAsteroidField();
}

static std::function<void()> *_space_demo_observer = nullptr;
/** @param args Exact registered console tokens. @return Whether the current invocation's actual native observer ran. */
static bool SpaceDemoObserver(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(_space_demo_observer != nullptr); (*_space_demo_observer)(); return true;
}

/** Real native full-world load, simulation, save writer and viewport capture. */
TEST_CASE("Space preview prepares or resumes ordinary playable checkpoints", "[.space-demo]")
{
	const char *action = std::getenv("OTTD_SPACE_DEMO_ACTION"); REQUIRE(action != nullptr);
	const bool prepare = std::string_view{action} == "prepare";
	const auto paths = GetTestCaseArtifactPaths("space", "preview");
	const auto config = GetTestRuntimeConfig();
	const char *input = std::getenv("OTTD_SPACE_DEMO_SOURCE");
	const auto source = prepare ? FindTestRepositoryRoot() / "src/tests/fixtures/demos/underground/mine-haulage-403.sav" : std::filesystem::path{input == nullptr ? "" : input};
	REQUIRE(std::filesystem::is_regular_file(source));
	bool observed = false; std::exception_ptr failure;
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			if (prepare) {
				REQUIRE(GetAsteroidField().bodies.empty()); // Genuine demo3 stays empty on ordinary load.
				// This explicitly staged copy starts at the model15 introduction;
				// changing the demo calendar grants no launch facility or cargo.
				TimerGameCalendar::SetDate(TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1990}, 0, 1), 0);
				TimerGameEconomy::SetDate(TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{1990}, 0, 1), 0);
				REQUIRE(GenerateWorldAsteroids(_settings_game.game_creation.generation_seed));
				REQUIRE(PrepareSpaceSurveyPreview());
			}
			REQUIRE(ValidateAsteroidField(GetAsteroidField()));
			REQUIRE(GetAsteroidField().surveys.size() == 3);
			// The null video backend selects its null blitter at startup. Restore
			// the actual rasterizer before screenshot-style offscreen rendering.
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			for (SpriteID sprite = SPR_SPACE_PREVIEW_ASTEROID; sprite <= SPR_SPACE_PREVIEW_PROBE + 1; ++sprite) {
				REQUIRE(SpriteExists(sprite)); REQUIRE(GetSpriteType(sprite) == SpriteType::Normal);
				REQUIRE(GetOriginFile(sprite) != nullptr);
			}
			SetViewportLayer(GetMainWindow(), MapLayer::Space);
			const auto centre = TileXYL(GetAsteroidField().width / 2, GetAsteroidField().height / 2 - 2, MapLayer::Space);
			ScrollMainWindowToTile(centre, true);
			auto save = [&](std::string_view name) {
				AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("{}.sav", name)), false) == SaveLoadResult::Ok);
			};
			if (prepare) save("space-survey-approach-1990-404");
			const auto initial = GetAsteroidField();
			{
				AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
				for (uint i = 0; i < 10; ++i) StateGameLoop();
				CHECK(GetAsteroidField() == initial);
			}
			AutoRestoreBackup running(_pause_mode, PauseModes{});
			auto expected = initial;
			bool attached_saved = false;
			SetViewportLayer(GetMainWindow(), MapLayer::Surface);
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
			for (uint i = 0; i < 2000;) {
				REQUIRE(std::chrono::steady_clock::now() < deadline);
				if (i == 1000) SetViewportLayer(GetMainWindow(), MapLayer::Space);
				const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
				if (GetAsteroidField().elapsed_ticks == before) {
					CHECK(GetAsteroidField() == expected); // CargoDist can pause the real simulation between work ticks.
					std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue;
				}
				REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); AdvanceContainedAsteroidSnapshot(expected);
				REQUIRE(GetAsteroidField() == expected);
				if (!attached_saved && std::ranges::any_of(expected.surveys, [](const auto &p) { return p.phase == SpaceSurveyPhase::Attached; })) {
					attached_saved = true;
					if (prepare) {
						SetViewportLayer(GetMainWindow(), MapLayer::Space); save("space-survey-attached-1990-404");
						CaptureNativeArtViewport(centre, ZoomLevel::Normal, paths.images / "space-attached.ppm");
						SetViewportLayer(GetMainWindow(), MapLayer::Surface);
					}
				}
				++i;
			}
			REQUIRE(attached_saved);
			for (const auto &survey : GetAsteroidField().surveys) CHECK(survey.completed_surveys >= 1);
			for (const auto &body : GetAsteroidField().bodies) CHECK(body.remaining_reserve == initial.bodies[body.id.base()].remaining_reserve);
			CaptureNativeArtViewport(centre, ZoomLevel::Normal, paths.images / "space-running.ppm");
			save("space-survey-continued-404");
			fmt::print("SPACE_DEMO action={} bodies={} probes=3 ticks=2000 exact=true hidden=true reserves_unchanged=true\n", action, GetAsteroidField().bodies.size());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_space_demo_observer, &observe);
	IConsole::CmdRegister("space_demo", SpaceDemoObserver);
	const std::string source_path = source.string(), config_path = config.string();
	std::array<std::string_view, 17> args = {"RelWithDebInfo/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}

/** The production generation window must operate the saved population selector. */
TEST_CASE("Asteroid population selector uses actual native generation widgets", "[.asteroid-population-ui]")
{
	const auto paths = GetTestCaseArtifactPaths("space", "population-ui");
	const auto config = GetTestRuntimeConfig();
	const auto source = FindTestRepositoryRoot() / "src/tests/fixtures/demos/underground/mine-haulage-403.sav";
	bool observed = false; std::exception_ptr failure;
	std::function<void()> observe = [&] {
		try {
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			AutoRestoreBackup settings(_settings_newgame, _settings_newgame);
			_settings_newgame.game_creation.gameplay_profile = 1;
			_settings_newgame.game_creation.landscape = LandscapeType::Temperate;
			ShowGenerateLandscape();
			auto *window = FindWindowByClass(WindowClass::GenerateLandscape);
			REQUIRE(window != nullptr);
			for (uint8_t density = 0; density < 3; ++density) {
				window->OnDropdownSelect(WID_GL_ASTEROID_PULLDOWN, density, 0);
				CHECK(_settings_newgame.game_creation.asteroid_density == density);
				CaptureNativeArtWindow(*window, paths.images / fmt::format("population-{}.ppm", density));
				CHECK_FALSE(window->IsWidgetDisabled(WID_GL_ASTEROID_PULLDOWN));
			}
			window->OnDropdownSelect(WID_GL_PROFILE_PULLDOWN, 0, 0);
			CaptureNativeArtWindow(*window, paths.images / "population-classic.ppm");
			CHECK(window->IsWidgetDisabled(WID_GL_ASTEROID_PULLDOWN));
			CHECK(_settings_newgame.game_creation.asteroid_density == 2);
			window->OnDropdownSelect(WID_GL_PROFILE_PULLDOWN, 1, 0);
			CaptureNativeArtWindow(*window, paths.images / "population-restored.ppm");
			CHECK_FALSE(window->IsWidgetDisabled(WID_GL_ASTEROID_PULLDOWN));
			CHECK(_settings_newgame.game_creation.asteroid_density == 2);
			fmt::print("ASTEROID_POPULATION_UI dropdown=true classic_disabled=true restored=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_space_demo_observer, &observe);
	IConsole::CmdRegister("population_ui_check", SpaceDemoObserver);
	const std::string source_path = source.string(), config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}

/** Exercise real map water classes and clipped perimeter reads for the production facility. */
TEST_CASE("Launch center geometry distinguishes Surface water and exact independent circles", "[launch-center-geometry]")
{
	AutoRestoreBackup edges(_settings_game.construction.freeform_edges, false);
	Map::Allocate(64, 64, 4);
	for (const auto tile : Map::Iterate()) {
		if (IsInnerTile(tile)) MakeClear(tile, ClearGround::Grass, 0);
		else MakeVoid(tile);
	}
	for (uint8_t rotation = 0; rotation < 4; ++rotation) {
		const auto area = GetLaunchCenterFootprint(TileXYL(10, 10, MapLayer::Surface), rotation);
		REQUIRE(area);
		CHECK(area->w == (rotation % 2 == 0 ? 4 : 3));
		CHECK(area->h == (rotation % 2 == 0 ? 3 : 4));
		CHECK_FALSE(HasLaunchCenterWaterAccess(*area));
		for (const auto edge : std::array{TileXYL(10, 9, MapLayer::Surface), TileXYL(9, 10, MapLayer::Surface),
			TileXYL(10 + area->w, 10, MapLayer::Surface), TileXYL(10, 10 + area->h, MapLayer::Surface)}) {
			MakeSea(edge);
			CHECK(HasLaunchCenterWaterAccess(*area));
			CHECK_FALSE(HasLaunchCenterWaterAccess(*area, edge));
			MakeClear(edge, ClearGround::Grass, 0);
		}
	}
	const auto area = *GetLaunchCenterFootprint(TileXYL(10, 10, MapLayer::Surface), 0);
	const auto north = TileXYL(10, 9, MapLayer::Surface), west = TileXYL(9, 10, MapLayer::Surface);
	MakeSea(TileXYL(9, 9, MapLayer::Surface));
	CHECK_FALSE(HasLaunchCenterWaterAccess(area)); // Diagonal contact is insufficient.
	MakeSea(area.tile);
	CHECK_FALSE(HasLaunchCenterWaterAccess(area)); // Internal water does not satisfy an outer edge.
	MakeClear(area.tile, ClearGround::Grass, 0);
	MakeShore(north);
	CHECK_FALSE(HasLaunchCenterWaterAccess(area));
	MakeCanal(north, OWNER_NONE, 0);
	CHECK(HasLaunchCenterWaterAccess(area));
	MakeRiver(north, 0);
	CHECK(HasLaunchCenterWaterAccess(area));
	// A clear water subtype alone does not make a sloped tile navigable.
	for (const auto make_water : std::array<void (*)(TileIndex), 3>{
		[](TileIndex tile) { MakeSea(tile); },
		[](TileIndex tile) { MakeCanal(tile, OWNER_NONE, 0); },
		[](TileIndex tile) { MakeRiver(tile, 0); }}) {
		make_water(north);
		SetTileHeight(north, 1);
		CHECK_FALSE(IsLaunchCenterNavigableWater(north));
		CHECK_FALSE(HasLaunchCenterWaterAccess(area));
		SetTileHeight(north, 0);
		CHECK(IsLaunchCenterNavigableWater(north));
		CHECK(HasLaunchCenterWaterAccess(area));
	}
	MakeSea(west);
	CHECK(HasLaunchCenterWaterAccess(area, north));
	MakeClear(west, ClearGround::Grass, 0);
	CHECK_FALSE(HasLaunchCenterWaterAccess(area, north));
	MakeClear(north, ClearGround::Grass, 0);
	for (const auto layer : {MapLayer::Underground, MapLayer::Space, MapLayer::DeepUnderground}) {
		const auto other = TileXYL(10, 9, layer);
		MakeSea(other);
		CHECK_FALSE(IsLaunchCenterNavigableWater(other));
		CHECK_FALSE(HasLaunchCenterWaterAccess(area));
		CHECK_FALSE(GetLaunchCenterFootprint(other, 0));
		CHECK_FALSE(GetLaunchCenterPlaneReference(other));
	}
	CHECK_FALSE(IsLaunchCenterNavigableWater(INVALID_TILE));
	CHECK_FALSE(GetLaunchCenterFootprint(INVALID_TILE, 0));
	CHECK_FALSE(GetLaunchCenterFootprint(area.tile, 4));
	CHECK_FALSE(GetLaunchCenterFootprint(TileXYL(61, 61, MapLayer::Surface), 0));
	CHECK_FALSE(GetLaunchCenterFootprint(TileXYL(60, 62, MapLayer::Surface), 0));
	CHECK(GetLaunchCenterFootprint(TileXYL(60, 61, MapLayer::Surface), 0));
	CHECK_FALSE(HasLaunchCenterWaterAccess(TileArea(TileXYL(0, 0, MapLayer::Surface), 4, 3)));
	MakeSea(TileXYL(0, 61, MapLayer::Underground));
	CHECK_FALSE(HasLaunchCenterWaterAccess(TileArea(TileXYL(60, 61, MapLayer::Surface), 4, 3)));
	CHECK_FALSE(HasLaunchCenterWaterAccess(TileArea(INVALID_TILE, 4, 3)));
	CHECK(GetLaunchCenterPlaneReference(area.tile) == SpacePoint{10 * SPACE_Q16, 10 * SPACE_Q16});
	for (const auto query : {IsInsideLaunchCenterDropZone, IsInsideLaunchCenterCollectionArea}) {
		CHECK(query({20 * SPACE_Q16, 0}, {0, 0}));
		CHECK(query({-20 * SPACE_Q16, 0}, {0, 0}));
		CHECK_FALSE(query({20 * SPACE_Q16 + 1, 0}, {0, 0}));
		CHECK(query({14 * SPACE_Q16, 14 * SPACE_Q16}, {0, 0}));
		CHECK(query({12 * SPACE_Q16, 16 * SPACE_Q16}, {0, 0}));
		CHECK_FALSE(query({12 * SPACE_Q16, 16 * SPACE_Q16 + 1}, {0, 0}));
		CHECK_FALSE(query({15 * SPACE_Q16, 15 * SPACE_Q16}, {0, 0}));
		CHECK_FALSE(query({INT32_MIN, INT32_MAX}, {INT32_MAX, INT32_MIN}));
	}
}

/** Independent canonical tables and a whole-field ledger, rather than self-validation alone. */
TEST_CASE("Asteroid class definitions and generated budgets match canonical finite resource tables", "[space-preview]")
{
	constexpr std::array<CargoLabel, 3> labels{CargoLabel{"NORE"}, CargoLabel{"WICE"}, CargoLabel{"RARE"}};
	constexpr uint32_t minimum[] = {400, 800, 100}, maximum[] = {800, 1600, 200};
	constexpr uint32_t expected_mean[] = {1080, 2160, 270};
	const auto &definitions = GetMultiLayerAsteroidDefinitions();
	for (size_t i = 0; i < 3; ++i) {
		CHECK(definitions[i].cargo == labels[i]);
		CHECK(definitions[i].small_min == minimum[i]);
		CHECK(definitions[i].small_max == maximum[i]);
		CHECK((definitions[i].small_min + definitions[i].small_max) * 18 / 20 == expected_mean[i]);
	}
	CHECK(minimum[0] / 20 == 20); CHECK(maximum[0] * 4 / 20 == 160);
	CHECK(minimum[1] / 20 == 40); CHECK(maximum[1] * 4 / 20 == 320);
	CHECK(minimum[2] / 20 == 5); CHECK(maximum[2] * 4 / 20 == 40);
	const auto field = GenerateAsteroidField(256, 256, 0x20261012, AsteroidDensity::Normal);
	REQUIRE(field); REQUIRE(field->bodies.size() == 160);
	std::array<uint32_t, 3> counts{}, reserves{};
	for (const auto &body : field->bodies) {
		const size_t resource = to_underlying(body.resource_class);
		const uint32_t multiplier = 1U << to_underlying(body.size);
		CHECK(body.initial_reserve >= minimum[resource] * multiplier);
		CHECK(body.initial_reserve <= maximum[resource] * multiplier);
		CHECK(body.initial_reserve == body.remaining_reserve);
		CHECK(body.initial_reserve % multiplier == 0);
		++counts[resource]; reserves[resource] += body.initial_reserve;
	}
	CHECK(counts == std::array<uint32_t, 3>{96, 48, 16});
	for (size_t resource = 0; resource < 3; ++resource) {
		CHECK(reserves[resource] >= counts[resource] * minimum[resource]);
		CHECK(reserves[resource] <= counts[resource] * maximum[resource] * 4);
	}
	CHECK(96 * expected_mean[0] + 48 * expected_mean[1] + 16 * expected_mean[2] == 211680);
	const auto total = reserves[0] + reserves[1] + reserves[2];
	CHECK(total >= 78400); CHECK(total <= 627200);
	fmt::print("ASTEROID_RESOURCE_BUDGET seed=0x20261012 bodies=160 NORE={} WICE={} RARE={} total={} expected_mean=211680 bounds=78400..627200\n", reserves[0], reserves[1], reserves[2], total);
}
