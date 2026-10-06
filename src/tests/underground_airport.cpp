/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_airport.cpp Direct ordinary airport refusal under native and preserved Classic layer policy. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 9.5 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_facility_fixture.h"
#include "../safeguards.h"

/** Hidden airport controls are backed by actual command refusals on every non-Surface layer, independently of the camera or profile. */
TEST_CASE("Ordinary airport commands refuse rock and Space planes before allocation", "[underground-airport]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFacilityRig rig(0);
	REQUIRE(CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date));
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto funds = Company::Get(_current_company)->money;
	const auto seeds = _random;
	// Both native and migrated Classic policy retain the airport command's Surface-only construction domain.
	for (bool classic : {false, true}) {
		if (classic) ResetMultiLayerProfileSelection();
		for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground, MapLayer::Space}) {
			for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
				const auto result = CmdBuildAirport(flags, TileXYL(40, 40, layer), 0, 0, NEW_STATION, false);
				CHECK(result.Failed());
				CHECK(result.GetErrorMessage() == STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER);
				CHECK(Station::GetNumItems() == 0);
			}
		}
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == funds);
	CHECK(_random.state[0] == seeds.state[0]);
	CHECK(_random.state[1] == seeds.state[1]);
}
