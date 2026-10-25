/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file newgame_layer_policy.cpp Pending settings, native activation and Classic construction admission.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical command and view-independent ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md accepted profile boundary.
 * The existing funded site is a fixture precondition; these checks exercise the actual settings/activation owners,
 * not a complete generated-world or raster acceptance run.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "../underground_gui.h"
#include "../map_layer_gui.h"
#include "../tbm_area_preview.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/** Copy all pending new-game settings through the same native entry used by StartGeneratingLandscape. */
extern void MakeNewgameSettingsLive();

/** Pending selection is not live permission; accepting Classic cannot acquire a cutter even at a pre-existing site. */
TEST_CASE("Native new-game synchronization keeps Classic separate from active underground permission", "[newgame-layer-policy]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world;
	LayerWindowTestUI ui(WindowClass::UndergroundConstruction, _current_company);
	AutoRestoreBackup live(_settings_game);
	AutoRestoreBackup pending(_settings_newgame);
	AutoRestoreBackup old_vehicle_defaults(_old_vds);
	const auto accepted = GetCurrentMultiLayerProfileSelection();
	REQUIRE(accepted.id == MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(GetLayerPortalDefaultTBMModel(world.portal) == 0);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto projects = ComputeTBMProjectDigest();
	const auto money = Company::Get(_current_company)->money;
	const auto vehicles = Vehicle::GetNumItems();
	const std::array random{_random.state[0], _random.state[1]};
	// This accepted-world fixture predates the menu selector; declare its matching current-world setting before testing a different pending choice.
	_settings_game.game_creation.gameplay_profile = to_underlying(accepted.id);
	_settings_newgame = _settings_game;
	_settings_newgame.game_creation.gameplay_profile = to_underlying(MultiLayerGameplayProfileID::Classic);
	_settings_newgame.game_creation.starting_year = TimerGameCalendar::Year{2050};
	// Menu preferences must not disable or grant machinery in the already accepted world before new-world initialization.
	CHECK(GetCurrentMultiLayerProfileSelection().id == accepted.id);
	CHECK(GetLayerPortalDefaultTBMModel(world.portal) == 0);
	MakeNewgameSettingsLive();
	CHECK(_settings_game.game_creation.gameplay_profile == to_underlying(MultiLayerGameplayProfileID::Classic));
	CHECK(_settings_game.game_creation.starting_year == TimerGameCalendar::Year{2050});
	CHECK(GetCurrentMultiLayerProfileSelection().id == accepted.id);
	// Generation resets policy before loading/accepting definitions. Classic preparation must not append first-party content.
	ResetMultiLayerProfileSelection();
	GRFConfigList candidate;
	const auto id = static_cast<MultiLayerGameplayProfileID>(_settings_game.game_creation.gameplay_profile);
	REQUIRE(PrepareMultiLayerProfileContent(id, _settings_game.game_creation.landscape, candidate) == MultiLayerProfileError::None);
	CHECK(candidate.empty());
	REQUIRE(ActivateMultiLayerProfile(id, _settings_game.game_creation.landscape, candidate) == MultiLayerProfileError::None);
	const auto classic = GetCurrentMultiLayerProfileSelection();
	CHECK(classic.id == MultiLayerGameplayProfileID::Classic);
	CHECK_FALSE(IsTBMCutterAvailable(0, MapLayer::Underground, classic, TimerGameCalendar::date));
	CHECK(GetLayerPortalDefaultTBMModel(world.portal) == UINT16_MAX);
	CHECK(ShowUndergroundConstructionToolbar(MapLayer::Underground, classic) == nullptr);
	CHECK(ShowUndergroundConstructionToolbar(MapLayer::Underground, accepted) == nullptr);
	// Legacy/unknown policy snapshots cannot bypass the live-world admission even when their model/date metadata looks current.
	auto legacy = accepted;
	legacy.legacy = true;
	CHECK(ShowUndergroundConstructionToolbar(MapLayer::Underground, legacy) == nullptr);
	const auto quote = PreviewTBMCommissionArea(_current_company, world.portal, uint16_t{0}, uint8_t{2}, uint8_t{2},
		world.origin->index, world.rock, GetLayerPortalDoorTile(world.rock), false, ExcavationPlanIntent::StraightTunnel);
	CHECK(quote.purchase.Failed());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeTBMProjectDigest() == projects);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(Vehicle::GetNumItems() == vehicles);
	CHECK((std::array{_random.state[0], _random.state[1]} == random));
}
