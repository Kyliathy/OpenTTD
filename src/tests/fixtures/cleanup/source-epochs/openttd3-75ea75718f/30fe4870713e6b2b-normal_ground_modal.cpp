/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_modal.cpp Actual normal-world useful modal preparation and operation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6DCDC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * A separate translation unit keeps the cumulative TBM fixture compilation bounded.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "normal_ground_supply_fixture.h"
#include "normal_ground_useful_fixture.h"
#include "../safeguards.h"

TEST_CASE("Original normal floor receives real Surface supply and paid native tube service", "[.normal-ground-supply]")
{
	RunNormalGroundUsefulSupply();
}

TEST_CASE("Actual earned normal-world tube route carries useful native Surface Coal", "[.normal-ground-useful]")
{
	RunNormalGroundUsefulOperation();
}
