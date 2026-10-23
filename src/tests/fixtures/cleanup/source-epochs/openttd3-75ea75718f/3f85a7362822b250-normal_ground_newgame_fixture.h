/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_newgame_fixture.h Observe actual normal application generation without constructing a fixture world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_NORMAL_GROUND_NEWGAME_FIXTURE_H
#define TESTS_NORMAL_GROUND_NEWGAME_FIXTURE_H

#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../openttd.h"
#include "../network/network.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../vehicle_base.h"
#include "../core/backup_type.hpp"
#include "../core/format.hpp"
#include "../core/random_func.hpp"
#include "../saveload/saveload_func.h"
#include "../economy_func.h"
#include "../excavation.h"
#include "../layer_portal_cmd.h"
#include "../landscape.h"
#include "../safeguards.h"
#include "../timer/timer_game_calendar.h"

inline std::function<void()> *_normal_ground_newgame_observer = nullptr; ///< Invocation-only startup observation,never a world owner.

/** @param arguments Ordinary native startup console arguments. @return True after read-only observation of the generated normal world. */
inline bool ObserveNormalGroundNewgame(std::span<std::string_view> arguments)
{
	if (arguments.empty()) return true;
	if (_normal_ground_newgame_observer != nullptr) (*_normal_ground_newgame_observer)();
	return true;
}

/** Observe a genuinely generated player world and a pure native entrance quote;do not build,seed or finance anything. */
inline void CaptureNormalGroundNewgame()
{
	REQUIRE(_game_mode == GameMode::Normal);
	REQUIRE_FALSE(_networking);
	const auto profile = GetCurrentMultiLayerProfileSelection();
	REQUIRE(profile.id == MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE_FALSE(profile.legacy);
	REQUIRE_FALSE(profile.editor_override);
	REQUIRE(Company::GetNumItems() == 1);
	REQUIRE(Company::IsValidID(_local_company));
	const auto *company = Company::Get(_local_company);
	CHECK_FALSE(company->is_ai);
	CHECK(company->money == company->current_loan);
	CHECK(company->money > Money{0});
	size_t fleet = 0, effects = 0;
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->type < VehicleType::CompanyEnd) ++fleet;
		if (vehicle->type == VehicleType::Effect) ++effects;
	}
	// Native landscape warmup may create industrial smoke effects. They are
	// genuine generated presentation objects,not a paid company fleet.
	CHECK(fleet == 0);
	const auto &geology = GetGeologyWorldState();
	REQUIRE(geology.generator_version != 0);
	CHECK(geology.deposits.Records().size() >= 10);
	CHECK_FALSE(geology.vents.empty());
	CHECK(geology.jobs.empty());
	CHECK(geology.job_claims.empty());
	size_t solid = 0, opened = 0;
	// Read the native packed authority through layer semantics. Even a dry
	// Surface or a legal portal estimate must not manufacture underground floor.
	for (const auto layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		for (const auto tile : MultiLayerMap::IterateLayer(layer)) {
			const auto state = geology.terrain.Get(tile);
			if (!state.has_value()) continue;
			if (*state == ExcavationState::Solid) ++solid; else ++opened;
		}
	}
	CHECK(solid > 0);
	CHECK(opened == 0);
	const auto before = std::tuple{company->money, company->current_loan, TimerGameTick::counter,
		TimerGameCalendar::date, _random.state[0], _random.state[1], ComputeGeologyStateDigest(geology)};
	TileIndex entrance = INVALID_TILE;
	Money quote = 0;
	{
		AutoRestoreBackup actor{_current_company, _local_company};
		// Stable bounded selection asks the actual public portal command about
		// naturally generated Surface cells. No terrain clearing or fixture
		// registry can supply a convenient replacement for a refused site.
		for (uint y = 12; y + 12 < Map::SizeY() && entrance == INVALID_TILE; ++y) {
			for (uint x = 12; x + 12 < Map::SizeX(); ++x) {
				const auto tile = TileXYL(x, y, MapLayer::Surface);
				if (!IsTileType(tile, TileType::Clear) || GetTileSlope(tile) != SLOPE_FLAT) continue;
				const auto cost = Command<Commands::BuildLayerPortal>::Do({}, tile, TransportType::Rail,
					MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD);
				if (cost.Failed()) continue;
				entrance = tile;
				quote = cost.GetCost();
				break;
			}
		}
	}
	REQUIRE(entrance != INVALID_TILE);
	CHECK(quote > Money{0});
	CHECK((std::tuple{company->money, company->current_loan, TimerGameTick::counter,
		TimerGameCalendar::date, _random.state[0], _random.state[1], ComputeGeologyStateDigest(geology)} == before));
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground");
	for (const auto name : {"normal-start.sav", "normal-start-repeat.sav"}) {
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
	}
	fmt::print("NORMAL_GROUND_START seed={} width={} height={} year={} company={} cash={} loan={} max_loan={} entrance={} quote={} deposits={} vents={} solid={} opened={} tick={} fleet={} effects={}\n",
		_settings_game.game_creation.generation_seed, Map::SizeX(), Map::SizeY(), TimerGameCalendar::year.base(),
		_local_company.base(), company->money.base(), company->current_loan.base(), _economy.max_loan.base(),
		entrance.base(), quote.base(), geology.deposits.Records().size(), geology.vents.size(), solid, opened, TimerGameTick::counter, fleet, effects);
	std::fflush(stdout);
}

/** Run ordinary -g generation with the real GUI company's native startup path in an invocation-owned hidden window. */
inline void RunNormalGroundNewgame()
{
	bool observed = false;
	std::exception_ptr failure;
	std::function<void()> observe = [&]() {
		try { CaptureNormalGroundNewgame(); observed = true; } catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &observe};
	IConsole::CmdRegister("normal_ground_start", ObserveNormalGroundNewgame);
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(),
		"-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null",
		"-I", "original_dos", "-d", "misc=4,sl=1", "-G", "420", "-t", "1980", "-g"};
	std::vector<std::string_view> arguments;
	for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	REQUIRE(observed);
}

#endif /* TESTS_NORMAL_GROUND_NEWGAME_FIXTURE_H */
