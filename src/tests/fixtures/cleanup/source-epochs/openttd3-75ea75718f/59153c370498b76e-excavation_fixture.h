/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_fixture.h Focused real-profile/geology setup through public owner boundaries. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md; r2mlvs6-mines-and-resource-chains.md, actual save fixtures; r2df-demo-fixes-interface-movement-and-planning.md Groups 8-9 (claimed work); r2mlvs3mp-master-plan.md Group 10R (declared rock seams) */

#ifndef TESTS_EXCAVATION_FIXTURE_H
#define TESTS_EXCAVATION_FIXTURE_H

#include "../excavation_state.h"
#include "../multilayer_recipe.h"
#include "../core/random_func.hpp"
#include "../newgrf_config.h"
#include "../timer/timer_game_calendar.h"
#include "../settings_type.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../openttd.h"
#include "../signal_func.h"
#include "../animated_tile_func.h" // Canonical native facility animation lifetime: r2mlvs17-underground-transport-tubes.md Group13CC.

/** Isolated native world with actual content activation; no fixture substitutes for gameplay cargo/terrain ownership. */
class ExcavationFixture {
	std::array<CargoSpec, NUM_CARGO> original_cargo; ///< Public registry definitions restored on teardown.
	CargoTypes original_mask = _cargo_mask; ///< Registry activation mask.
	SavedRandomSeeds seeds; ///< Both global random streams restored.
	bool edges = _settings_game.construction.freeform_edges; ///< Caller fence policy.
	decltype(_settings_game.station.station_spread) spread = _settings_game.station.station_spread; ///< Caller station footprint limit.
	TimerGameCalendar::Date date = TimerGameCalendar::date; ///< Caller synchronized calendar date.
	CompanyID company = _current_company; ///< Caller command actor.
	Company *test_company = nullptr; ///< Real pool-owned fixture company, deleted at teardown.
	GRFConfigList package; ///< Exact activated fixture package used by the shared profile restore boundary.
	GameMode mode = _game_mode; ///< Caller game/editor policy.
	PauseModes pause = _pause_mode; ///< Caller effect policy, restored after focused command checks.
public:
	/** @return This fixture's exact activated package descriptors, read-only for native catalogue producer checks. */
	const GRFConfigList &ActivatedContent() const { return this->package; }

	/** Activate an isolated native world. @param serialized_native_slots Match distributed cargo slots for native saves. @param hardness_version Zero preserves this fixture's historical geometry/rate baseline; regional acceptance explicitly selects the current version. @param world_seed Independent geological generation input. */
	explicit ExcavationFixture(bool serialized_native_slots = false, uint16_t hardness_version = 0, uint32_t world_seed = 420)
	{
		SaveRandomSeeds(&this->seeds);
		ResetGeologyWorldState();
		ResetMultiLayerProfileSelection();
		_settings_game.construction.freeform_edges = false;
		_settings_game.station.station_spread = 12;
		_game_mode = GameMode::Normal;
		_pause_mode.Set(PauseMode::Normal); // Suppress explosion effects; tests validate command state without graphics assets.
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
		REQUIRE(Company::CanAllocateItem());
		this->test_company = Company::Create();
		_current_company = this->test_company->index;
		this->test_company->avail_railtypes.Set(RAILTYPE_RAIL);
		this->test_company->avail_roadtypes.Set(ROADTYPE_ROAD);
		this->test_company->money = 1000000;
		this->test_company->clear_limit = 64U << 16;
		// This fixture replaces the whole native map. Reset its canonical display
		// registration owner as normal game initialization does, so new tube
		// facilities cannot retain tiles from an earlier isolated world.
		InitializeAnimatedTiles();
		Map::Allocate(64, 64, 4);
		// Actual layer initialization supplies fences without consuming rock-plane Surface RNG.
		for (auto layer : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space, MapLayer::DeepUnderground}) MultiLayerMap::InitialiseLayer(layer);
		const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
		// Preserve all registry slots before installing the producer's immutable required meanings.
		for (uint i = 0; i < NUM_CARGO; ++i) {
			this->original_cargo[i] = *CargoSpec::Get(i);
			*CargoSpec::Get(i) = CargoSpec{};
		}
		_cargo_mask = {};
		// Deliberately use label resolution rather than relying on the shipped runtime slot arrangement.
		for (uint i = 0; i < required.labels.size(); ++i) {
			// build_content.py preserves all twelve original slots; required baseline labels omit unused Temperate Food at slot 11.
			// Full disk fixtures must match that pinned native layout because ordinary Industry/Vehicle saves retain runtime cargo IDs.
			const uint slot = i + (serialized_native_slots && i >= 11 ? 1 : 0);
			CargoSpec::Get(slot)->label = required.labels[i];
			CargoSpec::Get(slot)->bitnum = slot;
			_cargo_mask.Set(CargoType{static_cast<uint8_t>(slot)});
		}
		BuildCargoLabelMap();
		auto config = std::make_unique<GRFConfig>();
		config->ident = required.package;
		config->version = required.version;
		config->status = GRFStatus::Activated;
		this->package.push_back(std::move(config));
		REQUIRE(ActivateMultiLayerProfile(MultiLayerGameplayProfileID::MultilayerIndustry, LandscapeType::Temperate, this->package) == MultiLayerProfileError::None);
		REQUIRE(GenerateWorldGeology(world_seed, hardness_version) == GeologyError::None);
	}
	/** Restore policy, registry, date, actor and RNG after this isolated focused world. */
	~ExcavationFixture()
	{
		UpdateSignalsInBuffer(); // Direct command-body fixtures must finish queued signals while their actual map/company owners still exist, as the normal command kernel does.
		InitializeAnimatedTiles(); // Retire only this isolated map's canonical display registrations before its native owners disappear.
		ResetGeologyWorldState();
		ResetMultiLayerProfileSelection();
		// Restore the actual registry, then rebuild its derived label cache rather than copying private cache state.
		for (uint i = 0; i < NUM_CARGO; ++i) *CargoSpec::Get(i) = this->original_cargo[i];
		_cargo_mask = this->original_mask;
		BuildCargoLabelMap();
		_settings_game.construction.freeform_edges = this->edges;
		_settings_game.station.station_spread = this->spread;
		TimerGameCalendar::date = this->date;
		_current_company = this->company;
		delete this->test_company;
		_game_mode = this->mode;
		_pause_mode = this->pause;
		RestoreRandomSeeds(this->seeds);
	}
	/**
	 * Change the explicit saved scenario-author flag through the actual content/policy owner.
	 * @param enabled Requested override; the current game/editor mode remains independently authoritative.
	 */
	void SetEditorOverride(bool enabled)
	{
		auto policy = GetCurrentMultiLayerProfileSelection();
		policy.editor_override = enabled;
		REQUIRE(RestoreMultiLayerProfileSelection(policy, LandscapeType::Temperate, this->package) == MultiLayerProfileError::None);
	}
	/** Remove only this fixture's real company; later orphan cleanup uses the explicit deity command owner. */
	void RemoveCompany()
	{
		_current_company = OWNER_DEITY;
		delete this->test_company;
		this->test_company = nullptr;
	}
	/**
	 * Prepare excavated fixture floor through the actual validated restore boundary.
	 * @param tiles Explicit test-owned rock tiles to open; this helper is absent from gameplay builds.
	 */
	void Open(std::span<const TileIndex> tiles)
	{
		auto snapshot = CaptureGeologyState();
		GeologyWorldState candidate;
		REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
		// Fixture creation is explicit and never treats a normal clearing command as excavation permission.
		for (TileIndex tile : tiles) REQUIRE(candidate.terrain.Set(tile, ExcavationState::Excavated));
		snapshot.packed.assign(candidate.terrain.Data().begin(), candidate.terrain.Data().end());
		REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
		StageLoadedGeologyState(std::move(candidate));
		AcceptLoadedGeologyState();
	}

	/**
	 * Reserve another project's work: a job of two straight SW (+x) cuts from an opened starter, whose unopened swept cells
	 * (x+2..x+3, y-1..y+1 around the anchor) are claimed. No route of a different job may sweep them (r2df Groups 8-9).
	 * @param anchor Starter cell; its 3x3 is opened first.
	 * @return The reserved job.
	 */
	ExcavationJobID ClaimStraightWork(TileIndex anchor)
	{
		std::vector<TileIndex> starter;
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) starter.push_back(TileAddWrap(anchor, x, y));
		}
		this->Open(starter);
		ExcavationJob job;
		job.company = _current_company;
		job.model_id = GetTileLayer(anchor) == MapLayer::DeepUnderground ? 1 : 0;
		job.commissioning_anchor = anchor;
		job.requested_tiles = {TileAddWrap(anchor, 2, 0)};
		for (int y = -1; y <= 1; ++y) {
			for (int x = 0; x <= 3; ++x) job.authorized_tiles.push_back(TileAddWrap(anchor, x, y));
		}
		// Both cuts advance SW from the opened starter, each sweeping its whole 3x3 body.
		for (int step = 1; step <= 2; ++step) {
			ExcavationCutGeometry cut;
			cut.cutter_position = TileAddWrap(anchor, step, 0);
			cut.heading = Direction::SW;
			cut.support_tiles = {anchor};
			for (int y = -1; y <= 1; ++y) {
				for (int x = -1; x <= 1; ++x) cut.swept_tiles.push_back(TileAddWrap(cut.cutter_position, x, y));
			}
			job.route.push_back(std::move(cut));
		}
		const auto reserved = ReserveExcavationJob(job, true);
		REQUIRE(reserved.error == ExcavationError::None);
		return reserved.id;
	}

	/**
	 * Make this world's whole rock plane one homogeneous mineral seam of the given hardness, before any job exists.
	 * Since R2TBMF task 5B.1 halved every per-cell yield (2026-10-09), the legacy all-Soft Underground fills fewer
	 * hoppers. Focused cases that need a capacity refusal or a mid-route exchange therefore declare harder rock here.
	 * Mixed rock yields 12 t Rock per cell, the former Soft Rock total, so routes keep their geometry and breakpoints.
	 * The seam replaces the generated deposits; terrain is unchanged and no floor or cargo is created. Cutting it only
	 * debits its finite reserve (r2mlvs3mp master Group 10R).
	 * @param layer Rock plane: Underground or Deep Underground.
	 * @param hardness Saved difficulty that the normal quote and cut owners read for every seam cell.
	 */
	void DeclareRockSeam(MapLayer layer, GeologyHardness hardness)
	{
		auto snapshot = CaptureGeologyState();
		REQUIRE(snapshot.jobs.empty());
		// A seam keeps its layer's frozen resource meaning, so it takes the mineral of that layer's first mine recipe.
		const auto recipes = GetMultiLayerMineRecipes();
		const auto recipe = std::ranges::find(recipes, layer, &MultiLayerMineRecipe::layer);
		REQUIRE(recipe != recipes.end());
		GeologyDeposit seam;
		seam.id = GeologyDepositID{0};
		seam.layer = layer;
		seam.resource = recipe->resource;
		seam.hardness = hardness;
		// One span per row, off the void border; a seam allows at most MAX_GEOLOGY_SPANS rows of at most 64 cells.
		const uint last_x = std::min<uint>(Map::SizeX() - 2, 64);
		const uint last_y = std::min<uint>(Map::SizeY() - 2, MAX_GEOLOGY_SPANS);
		for (uint y = 1; y <= last_y; ++y) seam.spans.push_back({static_cast<uint16_t>(y), uint16_t{1}, static_cast<uint16_t>(last_x)});
		seam.initial_reserve = seam.remaining_reserve = uint64_t{100} * last_x * last_y; // A finite 100 t quantum per cell.
		snapshot.deposits = {seam};
		snapshot.next_deposit_id = 1;
		GeologyWorldState restored;
		REQUIRE(PrepareGeologyRestoration(snapshot, restored) == GeologyError::None);
		StageLoadedGeologyState(std::move(restored));
		AcceptLoadedGeologyState();
	}

	/**
	 * Add one seam of the given hardness over the still solid cells of a rectangle that no deposit covers. Unlike
	 * DeclareRockSeam it keeps every generated deposit, job and reserve, so it may follow completed work. A further
	 * route declared Mixed keeps the 12 t Rock per cell that its Soft cells gave before R2TBMF task 5B.1 halved the
	 * yields, and with it the breakpoints of its frozen route (r2mlvs5 task 13D.1). Terrain is unchanged.
	 * @param hardness Saved difficulty that the normal quote and cut owners read for every seam cell.
	 * @param corner One corner of the rectangle, on Underground or Deep Underground.
	 * @param opposite The opposite corner, on the same layer.
	 */
	void AppendRockSeam(GeologyHardness hardness, TileIndex corner, TileIndex opposite)
	{
		const MapLayer layer = GetTileLayer(corner);
		REQUIRE(GetTileLayer(opposite) == layer);
		auto snapshot = CaptureGeologyState();
		// A seam keeps its layer's frozen resource meaning, so it takes the mineral of that layer's first mine recipe.
		const auto recipes = GetMultiLayerMineRecipes();
		const auto recipe = std::ranges::find(recipes, layer, &MultiLayerMineRecipe::layer);
		REQUIRE(recipe != recipes.end());
		GeologyDeposit seam;
		seam.id = GeologyDepositID{snapshot.next_deposit_id};
		seam.layer = layer;
		seam.resource = recipe->resource;
		seam.hardness = hardness;
		const auto &geology = GetGeologyWorldState();
		/** @param tile Cell of the rectangle. @return Whether it is still solid and outside every existing seam. */
		const auto is_free = [&geology](TileIndex tile) {
			return geology.terrain.Get(tile) == ExcavationState::Solid && geology.deposits.Get(geology.deposits.Find(tile)) == nullptr;
		};
		const uint first_x = std::min(TileX(corner), TileX(opposite)), last_x = std::max(TileX(corner), TileX(opposite));
		const uint first_y = std::min(TilePlaneY(corner), TilePlaneY(opposite)), last_y = std::max(TilePlaneY(corner), TilePlaneY(opposite));
		uint64_t cells = 0;
		// Each row adds one span per run of free cells, so no cell joins two seams and no opened floor gains a reserve.
		// The column one past the rectangle closes the row's last run.
		for (uint y = first_y; y <= last_y; ++y) {
			std::optional<uint> run;
			for (uint x = first_x; x <= last_x + 1; ++x) {
				const bool free = x <= last_x && is_free(TileXYL(x, y, layer));
				if (free && !run.has_value()) run = x;
				if (!free && run.has_value()) {
					seam.spans.push_back({static_cast<uint16_t>(y), static_cast<uint16_t>(*run), static_cast<uint16_t>(x - 1)});
					cells += x - *run;
					run.reset();
				}
			}
		}
		REQUIRE_FALSE(seam.spans.empty());
		REQUIRE(seam.spans.size() <= MAX_GEOLOGY_SPANS);
		seam.initial_reserve = seam.remaining_reserve = uint64_t{100} * cells; // A finite 100 t quantum per cell.
		snapshot.deposits.push_back(seam);
		snapshot.next_deposit_id = snapshot.next_deposit_id + 1;
		GeologyWorldState restored;
		REQUIRE(PrepareGeologyRestoration(snapshot, restored) == GeologyError::None);
		StageLoadedGeologyState(std::move(restored));
		AcceptLoadedGeologyState();
	}
};

#endif /* TESTS_EXCAVATION_FIXTURE_H */
