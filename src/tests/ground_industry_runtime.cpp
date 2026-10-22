/**
 * @file ground_industry_runtime.cpp Actual current application/viewport observer for selected industry art.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20BB; explicit retained source and isolated capture modes.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * The immutable collector fixture establishes the real application loader and
 * renderer. Fixture construction captures alone do not qualify complete selected
 * state/foreground/source-pixel coverage or transported gameplay progress.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../3rdparty/nlohmann/json.hpp"
#include "test_artifact_paths.h"
#include "layer_save_fixture.h"
#include "native_art_render_fixture.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../multilayer_sprite.h"
#include "../geothermal_collector.h"
#include "../industry.h"
#include "../timer/timer_game_tick.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../layer_processor_presentation.h"
#include "../economy_func.h"
#include "../geothermal.h"
#include "../utility_power.h"
#include "ground_industry_runtime_fixture.h"

#include "../safeguards.h"

static std::function<void(std::string_view)> *ground_industry_observer = nullptr; ///< Test-only startup callback; no saved or production owner.

/**
 * Invoke this process's native observation after actual world loading finishes.
 * @param arguments Parameter-free isolated startup command; no gameplay input is accepted.
 * @return True after the explicit observer executes.
 */
static bool ObserveGroundIndustryArt(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(ground_industry_observer != nullptr);
	(*ground_industry_observer)(arguments[0]);
	return true;
}

/**
 * Snapshot native simulation owners without introducing a display inventory or controller.
 * @return Existing source/stock/work/utility/cash/calendar/RNG values for exact before/after drawing comparison.
 */
static auto CaptureGroundIndustryDrawOwners()
{
	std::vector<std::tuple<IndustryID, CargoType, uint32_t>> outputs;
	// Mine side-state digests exclude physical Industry output. Observe those
	// actual slots separately, so drawing cannot hide a cargo debit or credit.
	for (const Industry *industry : Industry::Iterate()) {
		for (const auto &slot : industry->produced) outputs.emplace_back(industry->index, slot.cargo, slot.waiting);
	}
	return std::tuple(ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeLayerMineDigest(), ComputeLayerProcessorDigest(),
		ComputeGeothermalCollectorDigest(), ComputeGeothermalSiteDigest(), ComputeUtilityPowerDigest(),
		Company::Get(_current_company)->money, TimerGameTick::counter, TimerGameCalendar::date,
		_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1], std::move(outputs));
}

/**
 * Capture one actual native source state and phase without advancing its simulation owners.
 * @param industry Actual loaded native industry, with its own physical construction/stock/status.
 * @param state Accepted source state label determined by the native state owner.
 * @param frame Source construction or synchronized visual phase0..3.
 * @param paths This native process's owned output context.
 * @param frames Append-only scene observations; no gameplay state is stored here.
 */
static void CaptureGroundIndustryState(const Industry &industry, std::string_view state, uint8_t frame,
	const TestArtifactPaths &paths, nlohmann::json &frames)
{
	// Set an explicit observer-only phase, restored before returning. Sampling
	// this clock does not run any game tick, mine extraction or processing batch.
	const auto phase_tick = static_cast<TimerGameTick::TickCounter>((TimerGameTick::counter / 32) * 32 + frame * 8);
	AutoRestoreBackup visual_clock(TimerGameTick::counter, phase_tick);
	const auto *capability = GetLayerIndustryCapability(industry.type);
	REQUIRE(capability != nullptr);
	const auto origin = industry.location.tile;
	const auto centre = TileAddXY(origin, industry.location.w / 2, industry.location.h / 2);
	const auto basename = fmt::format("{}-{}-{}", capability->asset_id, state, frame);
	const auto before = CaptureGroundIndustryDrawOwners();
	REQUIRE(GetMultiLayerIndustrySprite(origin, false, phase_tick).has_value());
	REQUIRE(GetMultiLayerIndustrySprite(origin, true, phase_tick).has_value());
	CaptureNativeArtViewport(centre, ZoomLevel::Normal, paths.images / (basename + ".ppm"), paths.images / (basename + ".idx"));
	CHECK(CaptureGroundIndustryDrawOwners() == before);
	frames.push_back({{"asset_id", capability->asset_id}, {"industry", industry.index.base()}, {"origin", origin.base()},
		{"centre", centre.base()}, {"width", industry.location.w}, {"height", industry.location.h},
		{"layer", to_underlying(GetTileLayer(origin))}, {"scene", fmt::format("{}-{}", state, frame)},
		{"state", state}, {"frame", frame}, {"basename", basename}, {"observer_phase_tick", phase_tick}});
}

/**
 * Observe all selected industry states through the actual loaded application and native state owners.
 * @param paths Exact process-local native captures/reports; all state staging remains art-fixture-only.
 */
static void CaptureGroundIndustryStates(const TestArtifactPaths &paths)
{
	AutoRestoreBackup actor(_current_company, _local_company);
	AutoRestoreBackup pause(_pause_mode, PauseModes{});
	AutoRestoreBackup fluctuation(_economy.fluct, int16_t{1});
	AutoRestoreBackup scale(_settings_game.economy.industry_cargo_scale, uint16_t{100});
	AutoRestoreBackup display(_display_opt);
	_display_opt = {};
	const auto mines = CaptureLayerMines();
	const auto processors = CaptureLayerProcessors();
	REQUIRE(mines.size() == 7);
	REQUIRE(processors.size() == 4);
	std::vector<IndustryID> industries;
	for (const auto &mine : mines) industries.push_back(mine.industry);
	for (const auto &processor : processors) industries.push_back(processor.industry);
	nlohmann::json frames = nlohmann::json::array();
	// Each actual native facility moves through explicitly staged construction
	// flags before completion; a draw cannot supply a missing construction stage.
	for (const auto id : industries) {
		Industry &industry = *Industry::Get(id);
		const bool mine = GetLayerMineState(id) != nullptr;
		for (uint8_t stage = 0; stage < 4; ++stage) {
			for (const TileIndex tile : industry.location) {
				if (!industry.TileBelongsToIndustry(tile)) continue;
				REQUIRE_FALSE(IsIndustryCompleted(tile));
				SetIndustryConstructionStage(tile, stage);
			}
			if (mine) {
				REQUIRE(GetLayerMineOperationalStatus(industry) == LayerMineOperationalStatus::Construction);
			} else {
				REQUIRE(GetLayerProcessorOperationalStatus(industry) == LayerProcessorOperationalStatus::Construction);
			}
			CaptureGroundIndustryState(industry, "construction", stage, paths, frames);
		}
		// Completion is a disclosed native-map fixture boundary, not earned
		// labour. Subsequent presentation reads actual sole stock/status owners.
		for (const TileIndex tile : industry.location) {
			if (industry.TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
		}
		industry.prod_level = PRODLEVEL_DEFAULT;
		if (mine) {
			REQUIRE(GetLayerMineOperationalStatus(industry) == LayerMineOperationalStatus::Operating);
			// Four synchronized operating phases must expose the authored parts
			// without extracting reserve or earning any native production work.
			for (uint8_t frame = 0; frame < 4; ++frame) CaptureGroundIndustryState(industry, "working", frame, paths, frames);
			{
				AutoRestoreBackup held(_pause_mode, PauseModes{PauseMode::Normal});
				REQUIRE(GetLayerMineOperationalStatus(industry) == LayerMineOperationalStatus::Paused);
				CaptureGroundIndustryState(industry, "blocked", 0, paths, frames);
			}
			auto snapshot = CaptureGeologyState();
			const auto deposit = GetLayerMineState(id)->deposit;
			const auto found = std::ranges::find(snapshot.deposits, deposit, &GeologyDeposit::id);
			REQUIRE(found != snapshot.deposits.end());
			// Explicit finite-depletion art boundary: the existing geological
			// loader validates it; no cargo is credited or mining history earned.
			found->remaining_reserve = 0;
			GeologyWorldState depleted;
			REQUIRE(PrepareGeologyRestoration(snapshot, depleted) == GeologyError::None);
			StageLoadedGeologyState(std::move(depleted));
			AcceptLoadedGeologyState();
			REQUIRE(GetLayerMineOperationalStatus(industry) == LayerMineOperationalStatus::Depleted);
			CaptureGroundIndustryState(industry, "depleted", 0, paths, frames);
		} else {
			REQUIRE(GetLayerProcessorOperationalStatus(industry) == LayerProcessorOperationalStatus::Idle);
			CaptureGroundIndustryState(industry, "ready", 0, paths, frames);
			const auto &recipe = *GetLayerProcessorRecipe(industry);
			// Stage only the immutable selected recipe's exact input bill through
			// its sole native incoming-stock owner. This earns no batch/work ticks.
			for (const auto &role : recipe.recipe.inputs) {
				REQUIRE(ReceiveLayerProcessorCargo(industry, GetCargoTypeByLabel(role.label), role.quantity) == role.quantity);
			}
			REQUIRE(GetLayerProcessorOperationalStatus(industry) == LayerProcessorOperationalStatus::Working);
			REQUIRE(GetLayerProcessorState(id)->work_ticks == 0);
			for (uint8_t frame = 0; frame < 4; ++frame) CaptureGroundIndustryState(industry, "working", frame, paths, frames);
			{
				AutoRestoreBackup held(_pause_mode, PauseModes{PauseMode::Normal});
				REQUIRE(GetLayerProcessorOperationalStatus(industry) == LayerProcessorOperationalStatus::Paused);
				CaptureGroundIndustryState(industry, "blocked", 0, paths, frames);
			}
		}
	}
	REQUIRE(frames.size() == 110);
	nlohmann::json report{{"_documentation", "@see r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHB.2; actual native staged state matrix"},
		{"frames", frames}, {"fixture_floor", true}, {"fixture_capital", 1000000000}, {"fixture_construction_flags", true},
		{"fixture_recipe_inputs", true}, {"fixture_depletion", true}, {"observer_phase_clock_restored", true},
		{"construction_states_only", false}, {"full_state_coverage", true}, {"source_pixel_verification", "pending"},
		{"loaded_checkpoint", true}, {"transported_bootstrap_claimed", false}, {"owners_unchanged_during_render", true}};
	std::ofstream output(paths.reports / "industry-fixture.json", std::ios::binary);
	output << report.dump(2) << '\n';
	output.close();
	REQUIRE(output.good());
	fmt::print("GROUND_INDUSTRY_STATES industries=11 viewports=110 native_statuses=true owners_unchanged=true source_pixels=pending\n");
}

/**
 * Build and capture explicit current selected fixture geometry, retaining one loadable native scene.
 * @param paths This process's repository-owned artifact context.
 * @param create True to construct native facilities; false to inspect their actual saved/rebuilt owners after reload.
 */
static void CaptureGroundIndustryFixtures(const TestArtifactPaths &paths, bool create)
{
	AutoRestoreBackup actor(_current_company, _local_company);
	// This capital is a disclosed art-fixture prerequisite, not an earned
	// player balance or economic acceptance. Only the isolated loaded copy is
	// changed; every supplied/retained input save and ordinary game stays exact.
	AutoRestoreBackup capital(Company::Get(_current_company)->money, Money{1000000000});
	std::vector<IndustryID> industries;
	if (create) {
		industries = BuildGroundIndustryFixtures();
	} else {
		const auto mines = CaptureLayerMines();
		const auto processors = CaptureLayerProcessors();
		REQUIRE(mines.size() == 7);
		REQUIRE(processors.size() == 4);
		// Reload must resolve the actual saved owner tables and physical native
		// industries. It cannot reconstruct the scene by funding replacements.
		for (const auto &mine : mines) industries.push_back(mine.industry);
		for (const auto &processor : processors) industries.push_back(processor.industry);
	}
	REQUIRE(industries.size() == 11);
	const auto construction = ComputeGeothermalCollectorDigest();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto mines = ComputeLayerMineDigest();
	const auto processors = ComputeLayerProcessorDigest();
	const auto money = Company::Get(_current_company)->money;
	const auto tick = TimerGameTick::counter;
	SavedRandomSeeds before;
	SaveRandomSeeds(&before);
	nlohmann::json frames = nlohmann::json::array();
	AutoRestoreBackup display(_display_opt);
	_display_opt = {};
	// The native command creates every actual3x3/4x4 world footprint. Capture
	// the real sorter and decoded source pixels for each selected construction
	// scene, without treating the remaining source states as already verified.
	for (const auto id : industries) {
		const Industry &industry = *Industry::Get(id);
		const auto *capability = GetLayerIndustryCapability(industry.type);
		REQUIRE(capability != nullptr);
		const auto origin = industry.location.tile;
		const auto centre = TileAddXY(origin, industry.location.w / 2, industry.location.h / 2);
		// A scene label must describe actual native construction flags, rather
		// than assuming that command/editor placement supplied a particular phase.
		for (const TileIndex tile : industry.location) {
			if (!industry.TileBelongsToIndustry(tile)) continue;
			REQUIRE_FALSE(IsIndustryCompleted(tile));
			REQUIRE(GetIndustryConstructionStage(tile) == 0);
		}
		REQUIRE(GetMultiLayerIndustrySprite(origin, false, tick).has_value());
		const auto basename = std::string(capability->asset_id) + "-construction-0";
		CaptureNativeArtViewport(centre, ZoomLevel::Normal, paths.images / (basename + ".ppm"), paths.images / (basename + ".idx"));
		frames.push_back({{"asset_id", capability->asset_id}, {"industry", id.base()}, {"origin", origin.base()},
			{"centre", centre.base()}, {"width", industry.location.w}, {"height", industry.location.h},
			{"layer", to_underlying(GetTileLayer(origin))}, {"scene", "construction-0"}, {"basename", basename}});
	}
	SavedRandomSeeds after;
	SaveRandomSeeds(&after);
	CHECK(ComputeGeothermalCollectorDigest() == construction);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeLayerMineDigest() == mines);
	CHECK(ComputeLayerProcessorDigest() == processors);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(TimerGameTick::counter == tick);
	CHECK(std::ranges::equal(before.random.state, after.random.state));
	CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	const auto checkpoint = paths.saves / "selected-industries-fixture.sav";
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(checkpoint), false) == SaveLoadResult::Ok);
	nlohmann::json report{{"_documentation", "@see r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHB; isolated native art-fixture construction only"},
		{"frames", frames}, {"fixture_floor", true}, {"fixture_capital", 1000000000}, {"fixture_command_bodies", true},
		{"construction_states_only", true}, {"full_state_coverage", false}, {"source_pixel_verification", "pending"},
		{"loaded_checkpoint", !create},
		{"transported_bootstrap_claimed", false}, {"owners_unchanged_during_render", true}};
	const auto report_path = paths.reports / "industry-fixture.json";
	std::ofstream output(report_path, std::ios::binary);
	output << report.dump(2) << '\n';
	output.close();
	REQUIRE(output.good());
	// The existing native capture recorder accepts images only. The caller's
	// explicit process report binds this native JSON/checkpoint alongside the
	// image writer records without weakening that shared ownership contract.
	fmt::print("GROUND_INDUSTRY_FIXTURE industries=11 construction_viewports=11 owners_unchanged=true source_pixel_verification=pending\n");
	if (!create) fmt::print("GROUND_INDUSTRY_RELOAD actual_saved_mines=7 actual_saved_processors=4 replacement_funding=false\n");
}

/** Current native application loads the immutable baseline before any selected industry fixture construction. */
TEST_CASE("Current selected industry observer loads a real native world and viewport", "[.ground-industry-runtime]")
{
	const auto root = FindTestRepositoryRoot();
	const auto source = ResolveTestInputPath(root / "src/tests/fixtures/geothermal/fixture389-collector-funded-read.sav");
	const auto config = GetTestRuntimeConfig();
	bool observed = false;
	std::exception_ptr failure;
	/** Observe native rendering or build the explicitly requested isolated fixture.
	 * @param command Exact startup command choosing bootstrap versus current fixture construction.
	 */
	std::function<void(std::string_view)> capture = [&](std::string_view command) {
		// One selected graph can observe the retained collector and staged
		// industry states independently. Their actual command determines separate
		// capture ownership so neither can replace another leaf's native evidence.
		const auto scenario = command == "ground_industry_art" ? "retained-collector" :
			command == "ground_industry_states" ? "industry-states" : "industries";
		const auto paths = GetTestArtifactPaths("selected-ground", scenario);
		try {
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			const auto collectors = CaptureGeothermalCollectors();
			REQUIRE(collectors.size() == 1);
			const Industry *industry = Industry::Get(collectors[0].industry);
			const auto tile = industry->location.tile;
			REQUIRE(GetMultiLayerCollectorSprite(tile, false, TimerGameTick::counter).has_value());
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			if (command == "ground_industry_fixture") CaptureGroundIndustryFixtures(paths, true);
			if (command == "ground_industry_reload") CaptureGroundIndustryFixtures(paths, false);
			if (command == "ground_industry_states") CaptureGroundIndustryStates(paths);
			// Fixture commands legitimately consume their existing setup RNG.
			// The fixture's eleven captures already have their own purity gate;
			// snapshot this retained collector immediately before its own draw.
			const auto construction = ComputeGeothermalCollectorDigest();
			const auto money = Company::Get(_local_company)->money;
			const auto tick = TimerGameTick::counter;
			SavedRandomSeeds before;
			SaveRandomSeeds(&before);
			CaptureNativeArtViewport(TileAddXY(tile, 1, 1), ZoomLevel::Normal,
				paths.images / "bootstrap-collector.ppm", paths.images / "bootstrap-collector.idx");
			SavedRandomSeeds after;
			SaveRandomSeeds(&after);
			CHECK(ComputeGeothermalCollectorDigest() == construction);
			CHECK(Company::Get(_local_company)->money == money);
			CHECK(TimerGameTick::counter == tick);
			CHECK(std::ranges::equal(before.random.state, after.random.state));
			CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			fmt::print("GROUND_INDUSTRY_BOOTSTRAP actual_application=true actual_viewport=true new_industry_coverage=false\n");
		} catch (...) {
			failure = std::current_exception();
		}
		// A failed fixture must also restore the null driver's no-drawing
		// backend before its final dirty-window pass. Preserve/rethrow the real
		// fixture exception instead of hiding it behind a headless repaint fault.
		BlitterFactory::SelectBlitter("null");
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		observed = true;
		_exit_game = true;
	};
	AutoRestoreBackup observer(ground_industry_observer, &capture);
	IConsole::CmdRegister("ground_industry_art", ObserveGroundIndustryArt);
	IConsole::CmdRegister("ground_industry_fixture", ObserveGroundIndustryArt);
	IConsole::CmdRegister("ground_industry_reload", ObserveGroundIndustryArt);
	IConsole::CmdRegister("ground_industry_states", ObserveGroundIndustryArt);
	const std::string config_path = config.string();
	// Consume the same real source hook observed by the shared process runner;
	// an evidence-only alias must never claim a different loaded review world.
	const char *checkpoint = std::getenv("OTTD_GROUND_INDUSTRY_SOURCE");
	const std::string source_path = checkpoint == nullptr ? source.string() :
		ResolveTestInputPath(std::filesystem::path{checkpoint}).string();
	std::array<std::string_view, 17> arguments{"openttd_ground_viewport_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple",
		"-I", "original_dos", "-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
