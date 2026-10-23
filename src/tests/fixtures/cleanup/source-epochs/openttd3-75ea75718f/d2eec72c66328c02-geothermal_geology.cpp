/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geothermal_geology.cpp Natural vent guarantees, mineral overlap and capacity/identity restoration. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 6 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../excavation_state.h"
#include "../settings_type.h"
#include "../core/random_func.hpp"
#include "../multilayer_utility.h"

#include "../safeguards.h"

/** Verify independently seeded natural fields leave every required mineral and every source cell solid. */
TEST_CASE("Natural vent generation preserves ten minerals and has no free cavity or heat grant", "[geothermal-geology]")
{
	Map::Allocate(64, 128, 4);
	for (const auto layer : {MapLayer::Underground, MapLayer::DeepUnderground}) MultiLayerMap::InitialiseLayer(layer);
	const std::array random_before = {_random.state[0], _random.state[1]};
	auto first = PrepareGeologyGeneration(DeriveGeologySeed(1234));
	auto repeated = PrepareGeologyGeneration(DeriveGeologySeed(1234));
	REQUIRE(first.error == GeologyError::None);
	REQUIRE(repeated.error == GeologyError::None);
	REQUIRE(first.state.vents.size() == 1);
	CHECK(first.state.deposits.Records().size() == 10);
	const auto &vent = first.state.vents.front();
	CHECK(vent.id == GeothermalVentID{0});
	CHECK(vent.layer == MapLayer::DeepUnderground);
	CHECK(vent.generation_version == 1);
	CHECK(vent.thermal_capacity_w == 1000000);
	CHECK(vent.x == repeated.state.vents.front().x);
	CHECK(vent.y == repeated.state.vents.front().y);
	CHECK(std::array{_random.state[0], _random.state[1]} == random_before);
	// Every natural source cell remains solid; discovery cannot commission a collector or manufacture an electrical meter.
	for (uint y = vent.y; y < vent.y + static_cast<uint32_t>(vent.height); ++y) {
		for (uint x = vent.x; x < vent.x + static_cast<uint32_t>(vent.width); ++x) {
			const auto tile = TileXYL(x, y, vent.layer);
			CHECK(first.state.terrain.Get(tile) == ExcavationState::Solid);
			CHECK(FindGeothermalVentField(first.state, tile)->id == vent.id);
			CHECK(FindGeothermalVentField(first.state, MultiLayerMap::ProjectToSurface(tile)) == nullptr);
		}
	}
	CHECK_FALSE(CheckGeologyGenerationSize(32, 64));
	Map::Allocate(64, 64, 1);
	CHECK(PrepareGeologyGeneration(DeriveGeologySeed(1234)).error == GeologyError::UnsupportedMap);
}

/** Verify overlapping mineral influence survives clearing/restoration without reserve or capacity duplication. */
TEST_CASE("Natural thermal and mineral identities coexist through a saved cavity", "[geothermal-geology-state]")
{
	Map::Allocate(64, 64, 4);
	for (const auto layer : {MapLayer::Underground, MapLayer::DeepUnderground}) MultiLayerMap::InitialiseLayer(layer);
	auto generated = PrepareGeologyGeneration(DeriveGeologySeed(1234));
	REQUIRE(generated.error == GeologyError::None);
	const auto &mineral = generated.state.deposits.Records()[5];
	const auto &span = mineral.spans.front();
	generated.state.vents[0].x = span.first_x;
	generated.state.vents[0].y = span.y;
	const auto tile = TileXYL(span.first_x, span.y, MapLayer::DeepUnderground);
	REQUIRE(generated.state.terrain.Set(tile, ExcavationState::Excavated));
	GeologyStateSnapshot snapshot;
	snapshot.generator_version = generated.state.generator_version;
	snapshot.width = Map::SizeX(); snapshot.height = Map::SizeY();
	snapshot.freeform_edges = _settings_game.construction.freeform_edges;
	snapshot.next_deposit_id = generated.state.deposits.NextID().base();
	snapshot.packed.assign(generated.state.terrain.Data().begin(), generated.state.terrain.Data().end());
	snapshot.deposits.assign(generated.state.deposits.Records().begin(), generated.state.deposits.Records().end());
	snapshot.vents = generated.state.vents;
	snapshot.next_vent_id = generated.state.next_vent_id;
	GeologyWorldState restored;
	CHECK(PrepareGeologyRestoration(snapshot, restored, false) == GeologyError::None);
	CHECK(restored.deposits.Find(tile) == mineral.id);
	CHECK(restored.deposits.Get(mineral.id)->remaining_reserve == 32000);
	CHECK(FindGeothermalVentField(restored, tile)->id == GeothermalVentID{0});
	CHECK(FindGeothermalVentField(restored, tile)->thermal_capacity_w == 1000000);
	CHECK(restored.terrain.Get(tile) == ExcavationState::Excavated);
	CHECK(restored.next_vent_id == 1);
	snapshot.vents[0].thermal_capacity_w = MAX_UTILITY_RATE_W + 1;
	CHECK(PrepareGeologyRestoration(snapshot, restored, false) == GeologyError::InvalidReserve);
	snapshot.vents[0].thermal_capacity_w = 1000000;
	snapshot.vents[0].id = GeothermalVentID{1};
	CHECK(PrepareGeologyRestoration(snapshot, restored, false) == GeologyError::InvalidID);
	CHECK(restored.vents.front().id == GeothermalVentID{0});
}
