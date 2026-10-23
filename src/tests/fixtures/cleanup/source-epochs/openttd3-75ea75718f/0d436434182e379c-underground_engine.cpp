/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_engine.cpp Actual conventional EIDS definitions, reconstruction and twelve-label transport coverage. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 7 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../underground_vehicle.h"
#include "../engine_func.h"
#include "../road.h"
#include "../multilayer_industry.h"
#include "../safeguards.h"

/** Real EIDS reconstruction preserves existing engines and immutable capability identity, without a feature-local registry. */
TEST_CASE("Conventional underground engines extend real mappings without replacing original or TBM definitions", "[underground-engine]")
{
	TBMEngineFixture fixture;
	const auto original = fixture.stock->index;
	const auto cutter = GetTBMEngine(0);
	REQUIRE(InitialiseUndergroundEngineDefinitions(fixture.package));
	CHECK(Engine::GetNumItems() == 10);
	std::array<EngineID, 7> ids;
	const auto catalogue = GetMultiLayerVehicleCapabilities();
	// Each original catalogue row resolves to one actual engine and its exact shared cargo mask, independently of pool numbering.
	for (uint16_t model = 2; model <= 8; ++model) {
		ids[model - 2] = GetUndergroundEngine(model);
		REQUIRE(Engine::IsValidID(ids[model - 2]));
		const auto *capability = GetUndergroundEngineCapability(ids[model - 2]);
		REQUIRE(capability != nullptr);
		CHECK(capability == &*std::ranges::find(catalogue, model, &LayerVehicleCapability::model_id));
		CHECK(capability->model_id == model);
		const auto *engine = Engine::Get(ids[model - 2]);
		CHECK(engine->info.base_intro == TimerGameCalendar::ConvertYMDToDate(capability->introduction_year, 0, 1));
		CHECK(engine->type == (model < 7 ? VehicleType::Train : VehicleType::Road));
		CHECK(engine->GetDefaultCargoType() == GetCargoTypeByLabel(capability->cargo_labels.empty() ? CargoLabel{"COAL"} : capability->cargo_labels.front()));
		if (model >= 5) {
			CargoTypes exact;
			for (CargoLabel label : capability->cargo_labels) exact.Set(GetCargoTypeByLabel(label));
			CHECK(engine->info.refit_mask == exact);
		}
		CHECK(CheckUndergroundVehicleCapability(engine->index, MapLayer::Underground).Succeeded());
		CHECK(CheckUndergroundVehicleCapability(engine->index, MapLayer::Space).Failed());
	}
	REQUIRE(InitialiseUndergroundEngineDefinitions(fixture.package));
	CHECK(Engine::GetNumItems() == 10);
	for (uint16_t model = 2; model <= 8; ++model) CHECK(GetUndergroundEngine(model) == ids[model - 2]);
	CHECK(GetTBMEngine(0) == cutter);
	CHECK(Engine::Get(original) == fixture.stock);
	CHECK(GetUndergroundEngineCapability(original) == nullptr);
	CHECK(GetUndergroundEngineCapability(cutter) == nullptr);
	CHECK(GetUndergroundEngine(0) == EngineID::Invalid());
	CHECK(GetUndergroundEngine(9) == EngineID::Invalid());
	CHECK_FALSE(InitialiseUndergroundEngineDefinitions({}));
	CHECK(Engine::GetNumItems() == 10);
}

/** Normal engine startup respects canonical native dates even in an older-starting world, while retaining its ordinary company availability owner. */
TEST_CASE("Ordinary native engine startup uses exact published introductions", "[underground-engine-startup]")
{
	TBMEngineFixture fixture;
	ResetRoadTypes(); // This formerly rail-only producer fixture also exercises the real roadtype availability owner here.
	REQUIRE(InitialiseUndergroundEngineDefinitions(fixture.package));
	AutoRestoreBackup starting_year(_settings_game.game_creation.starting_year, TimerGameCalendar::Year{1950});
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	const auto aging = TimerGameCalendar::ConvertDateToYMD(TimerGameCalendar::date);
	// Actual normal startup may randomize original engines, but all native depot designs become available on their canonical date.
	for (uint16_t model = 2; model <= 8; ++model) {
		Engine *engine = Engine::Get(GetUndergroundEngine(model));
		StartupOneEngine(engine, aging, 420);
		CHECK(engine->intro_date == engine->info.base_intro);
		CHECK(engine->company_avail.Test(_current_company));
		CHECK(IsEngineBuildable(engine->index, engine->type, _current_company));
	}
}

/** Every mine output and spoil has an actual rail and road option from its canonical available date, with exact sealed/refit restrictions. */
TEST_CASE("Native resource and spoil transport coverage uses real definitions and synchronized dates", "[underground-engine]")
{
	TBMEngineFixture fixture;
	REQUIRE(InitialiseUndergroundEngineDefinitions(fixture.package));
	/** Explicit expected resource-to-native-model coverage; simulation still resolves all cargo meanings by label. */
	struct CargoRoute {
		CargoLabel cargo; ///< Shared registry meaning, not a runtime slot.
		uint16_t rail_model; ///< Canonical wagon design, hauled by model 4.
		uint16_t road_model; ///< Canonical truck/carrier design.
		int year; ///< Earliest actual carrier date independent of later mine layer unlock.
	};
	const CargoRoute routes[] = {
		{CargoLabel{"COAL"}, 5, 7, 1950}, {CargoLabel{"IORE"}, 5, 7, 1950},
		{CargoLabel{"CORE"}, 5, 7, 1950}, {CargoLabel{"BAUX"}, 5, 7, 1950},
		{CargoLabel{"SALT"}, 5, 7, 1950}, {CargoLabel{"LITH"}, 5, 7, 1950},
		{CargoLabel{"NORE"}, 5, 7, 1950}, {CargoLabel{"COBT"}, 5, 7, 1950},
		{CargoLabel{"URAN"}, 6, 8, 1980}, {CargoLabel{"RARE"}, 6, 8, 1980},
		{CargoLabel{"ERTH"}, 2, 7, 1950}, {CargoLabel{"ROCK"}, 3, 7, 1950},
	};
	// Cover the entire supplied mine recipe family, so an omitted new resource cannot silently pass a manually shortened transport table.
	for (const auto &mine : GetMultiLayerMineRecipes()) CHECK(std::ranges::find(routes, mine.resource, &CargoRoute::cargo) != std::end(routes));
	for (const auto &route : routes) {
		INFO(std::string(route.cargo.begin(), route.cargo.end()));
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{route.year}, 0, 1);
		const auto cargo = GetCargoTypeByLabel(route.cargo);
		REQUIRE(IsValidCargoType(cargo));
		for (const auto model : {route.rail_model, route.road_model}) {
			const auto id = GetUndergroundEngine(model);
			REQUIRE(Engine::IsValidID(id));
			CHECK(CheckUndergroundVehicleCapability(id, MapLayer::Underground, cargo).Succeeded());
			CHECK(CheckUndergroundVehicleCapability(id, MapLayer::Underground, GetCargoTypeByLabel(CargoLabel{"PASS"})).Failed());
			CHECK(CheckUndergroundVehicleCapability(id, MapLayer::DeepUnderground, cargo).Succeeded() == (route.year >= 1980));
		}
	}
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK(CheckUndergroundVehicleCapability(GetUndergroundEngine(6), MapLayer::Underground).Failed());
	CHECK(CheckUndergroundVehicleCapability(GetUndergroundEngine(8), MapLayer::Underground).Failed());
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	CHECK(CheckUndergroundVehicleCapability(GetUndergroundEngine(5), MapLayer::Underground, GetCargoTypeByLabel(CargoLabel{"URAN"})).Failed());
	CHECK(CheckUndergroundVehicleCapability(GetUndergroundEngine(7), MapLayer::Underground, GetCargoTypeByLabel(CargoLabel{"URAN"})).Failed());
	ResetMultiLayerProfileSelection();
	CHECK(CheckUndergroundVehicleCapability(GetUndergroundEngine(4), MapLayer::Underground).Failed());
}
