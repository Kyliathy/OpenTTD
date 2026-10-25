/* This file is part of OpenTTD. Licensed under the GNU GPL version 2. */
/** @file ground_performance_merge.cpp Existing full native dispatch and prepared Tube timing observers.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md
 * Timings observe native work; prepared component inputs never become genuine bootstrap proof.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include <any>
#include <filesystem>
#include "ground_performance_fixture.h"
#include "tube_performance_fixture.h"
