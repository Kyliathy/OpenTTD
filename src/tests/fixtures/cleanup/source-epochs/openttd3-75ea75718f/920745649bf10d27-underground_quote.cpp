/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_quote.cpp Whole-platform/approach quotes and exact refused-cell presentation. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 3B */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "excavation_fixture.h"
#include "../station_cmd.h"
#include "../station_base.h"
#include "../newgrf_station.h"
#include "../rail.h"
#include "../economy_func.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** A partial station quote names the first actual blocked tile, then prices every platform cell after excavation. */
TEST_CASE("Underground station quotes retain full footprints and explain the blocked cell", "[underground-footprint-quote]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ExcavationFixture fixture;
	AutoRestoreBackup<CompanyID> local(_local_company, _current_company);
	AutoRestoreBackup<Money> station_price(_price[Price::BuildStationRail], Money{100});
	ResetRailTypes();
	StationClass::Reset();
	// Both rock layers must use the command's same platform and two-ended approach geometry.
	for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		const TileIndex anchor = TileXYL(20, 20, layer);
		fixture.Open(std::array{anchor});
		const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
		const auto cash = Company::Get(_current_company)->money;
		const auto stations = Station::GetNumItems();
		auto blocked = CmdBuildRailStation({}, anchor, RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false);
		REQUIRE(blocked.GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
		const std::string explanation = blocked.GetEncodedMessage().GetDecodedString();
		CHECK(explanation.find("21 x 20") != std::string::npos);
		CHECK(explanation.find("Underground") != std::string::npos);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
		CHECK(Company::Get(_current_company)->money == cash);
		CHECK(Station::GetNumItems() == stations);
		fixture.Open(std::array{TileXYL(19, 20, layer), TileXYL(21, 20, layer), TileXYL(22, 20, layer)});
		const auto open = ComputeGeologyStateDigest(GetGeologyWorldState());
		const auto quote = CmdBuildRailStation({}, anchor, RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false);
		INFO("Station quote error: " << quote.GetErrorMessage().base());
		REQUIRE(quote.Succeeded());
		CHECK(quote.GetCost() >= Money{200});
		const auto shorter = CmdBuildRailStation({}, anchor, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false);
		REQUIRE(shorter.Succeeded());
		CHECK(shorter.GetCost() < quote.GetCost());
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == open);
		CHECK(Company::Get(_current_company)->money == cash);
		CHECK(Station::GetNumItems() == stations);
	}
}
