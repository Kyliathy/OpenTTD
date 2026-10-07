/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_art_geometry.cpp Measure actual native consumer geometry before selecting M1 raster source footprints.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md, Group 9
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-b-m1-authoring.md (source/consumer reconciliation)
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "../articulated_vehicles.h"
#include "../industry.h"

#include "../safeguards.h"

/** Native definitions and built receiver instances supply source geometry; the observer generates no pixels or replacement gameplay objects. */
TEST_CASE("M1 source briefs use actual articulated machinery and receiver layouts", "[m1-art-geometry]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundWorkflowWorld world;
	const uint standard = 1 + CountArticulatedParts(GetTBMEngine(0));
	const uint deep = 1 + CountArticulatedParts(GetTBMEngine(1));
	CHECK(standard == 2);
	CHECK(deep == 2);
	// Original plus actual native definition mappings and constructed instances share the same layout/footprint owner;
	// a planned art rectangle cannot resize a receiver's funded world cells.
	for (const Industry *receiver : {world.earth, world.stone}) {
		const auto *spec = GetIndustrySpec(receiver->type);
		REQUIRE(spec != nullptr);
		REQUIRE(spec->layouts.size() == 1);
		uint width = 0, height = 0;
		for (const auto &cell : spec->layouts.front()) {
			width = std::max<uint>(width, cell.ti.x + 1);
			height = std::max<uint>(height, cell.ti.y + 1);
		}
		CHECK(receiver->location.w == width);
		CHECK(receiver->location.h == height);
		fmt::print("M1_RECEIVER_GEOMETRY kind={} footprint={}x{} cells={}\n", to_underlying(GetSpoilReceiver(receiver->index)->kind), width, height, spec->layouts.front().size());
	}
	fmt::print("M1_CUTTER_GEOMETRY standard_parts={} deep_parts={}\n", standard, deep);
}
