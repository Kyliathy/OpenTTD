/* This file is part of OpenTTD. Licensed under the GNU GPL version 2. */
/** @file normal_ground_merge.cpp Current genuine paid-world continuation entry points.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md
 * Inputs are verified merged successors of exact retained normally financed
 * originals. These callbacks create no replacement world, cash or cargo stock.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "normal_ground_boring_fixture.h"
#include "normal_ground_useful_fixture.h"
#include "../safeguards.h"

/** Continue the original paid Standard project through real cuts and spoil custody. */
TEST_CASE("Merged Standard paid original continues native work", "[.normal-ground-boring]")
{
    RunNormalGroundBoring();
}

/** Continue the original paid Deep project while preserving its completed Standard history. */
TEST_CASE("Merged Deep paid original continues native work", "[.normal-ground-deep-work]")
{
    RunNormalGroundBoring(1, "normal-ground-deep-work");
}

/** Continue the original bent-room work and retained paid-machine reuse. */
TEST_CASE("Merged irregular paid original continues native work", "[.normal-ground-irregular-work]")
{
    RunNormalGroundBoring(0, "normal-ground-irregular-work");
}

/** Run the original paid modal/tube route with actual useful production and settlement. */
TEST_CASE("Merged paid original carries useful native freight", "[.normal-ground-useful]")
{
    RunNormalGroundUsefulOperation();
}
