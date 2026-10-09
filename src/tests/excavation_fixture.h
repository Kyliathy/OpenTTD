/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_fixture.h Focused real-profile/geology setup through public owner boundaries. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md; r2mlvs6-mines-and-resource-chains.md, actual save fixtures */

#ifndef TESTS_EXCAVATION_FIXTURE_H
#define TESTS_EXCAVATION_FIXTURE_H

#include "../excavation_state.h"
#include "../core/random_func.hpp"
#include "../newgrf_config.h"
#include "../timer/timer_game_calendar.h"
#include "../settings_type.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../openttd.h"
#include "../signal_func.h"

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
};

#endif /* TESTS_EXCAVATION_FIXTURE_H */
