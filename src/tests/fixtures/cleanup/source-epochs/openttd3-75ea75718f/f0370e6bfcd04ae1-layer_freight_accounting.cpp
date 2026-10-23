/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_freight_accounting.cpp Actual cargo vectors and same-XY station isolation at the finite freight boundary. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 12 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mine_production_fixture.h"
#include "../linkgraph/linkgraph.h"

#include "../safeguards.h"

/** Equivalent actual vehicle movement on each physical layer earns the same horizontal distance, with no stacked-Y reward. */
TEST_CASE("Cargo distance uses equal horizontal movement on every layer", "[layer-freight-accounting]")
{
	MineAdmissionRig rig;
	// Exercise both same-layer trips and explicit cross-layer endpoint geometry; route feasibility is a separate portal/fleet obligation.
	for (MapLayer source : {MapLayer::Surface, MapLayer::Underground, MapLayer::DeepUnderground}) {
		for (MapLayer destination : {MapLayer::Surface, MapLayer::Underground, MapLayer::DeepUnderground}) {
			REQUIRE(CargoPacket::CanAllocateItem());
			CargoPacket *packet = CargoPacket::Create(StationID::Invalid(), 4, Source{IndustryID::Invalid(), SourceType::Industry});
			packet->UpdateLoadingTile(TileXYL(10, 10, source));
			CHECK(packet->GetDistance(TileXYL(20, 15, destination)) == 15);
			CHECK(packet->GetDistance(TileXYL(10, 10, destination)) == 0);
			delete packet;
		}
	}
}

/** After an explicit vertical vehicle leg, unpaid movement inside a receiving station must remain unpaid on the next leg. */
TEST_CASE("Vertical freight travel cannot hide a free move inside a station", "[layer-freight-accounting]")
{
	MineAdmissionRig rig;
	REQUIRE(CargoPacket::CanAllocateItem());
	CargoPacket *packet = CargoPacket::Create(StationID::Invalid(), 4, Source{IndustryID::Invalid(), SourceType::Industry});
	const TileIndex origin = TileXYL(10, 10, MapLayer::Underground);
	const TileIndex arrival = TileXYL(10, 10, MapLayer::Surface);
	const TileIndex departure = TileXYL(20, 10, MapLayer::Surface);
	packet->UpdateLoadingTile(origin);
	packet->UpdateUnloadingTile(arrival);
	packet->UpdateLoadingTile(departure);
	CHECK(packet->GetDistance(departure) == 0);
	CHECK(packet->GetDistance(TileXYL(23, 10, MapLayer::Surface)) == 3);
	CHECK(packet->GetDistance(TileXYL(23, 10, MapLayer::DeepUnderground)) == 3);
	CHECK(packet->GetFirstStation() == StationID::Invalid());
	CHECK(packet->Count() == 4);
	delete packet;
}

/** Normalizing the largest supported stacked origin preserves the existing signed-16-bit saved vector representation. */
TEST_CASE("Freight plane projection preserves large-map signed cargo vectors", "[layer-freight-accounting]")
{
	MineAdmissionRig rig;
	Map::Allocate(64, MAX_MAP_SIZE, 4); // This coordinate-only fixture needs actual geometry, not regenerated reserve or gameplay tile contents.
	REQUIRE(CargoPacket::CanAllocateItem());
	CargoPacket *packet = CargoPacket::Create(StationID::Invalid(), 4, Source{IndustryID::Invalid(), SourceType::Industry});
	packet->UpdateLoadingTile(TileXYL(10, MAX_MAP_SIZE - 20, MapLayer::DeepUnderground));
	CHECK(packet->GetDistance(TileXYL(20, MAX_MAP_SIZE - 25, MapLayer::Surface)) == 15);
	CHECK(packet->GetDistance(TileXYL(10, MAX_MAP_SIZE - 20, MapLayer::Surface)) == 0);
	delete packet;
}

/** A foreign-layer station at identical XY cannot collect a mine's finite stock or acquire a demand edge from catchment alone. */
TEST_CASE("Same XY stations on different layers gain no finite cargo or demand edge", "[layer-freight-accounting]")
{
	MineProductionRig rig;
	const TileIndex underground = TileAddWrap(rig.request.tile, 3, 0);
	Station *local = rig.StationAt(underground);
	Station *surface = rig.StationAt(MultiLayerMap::ProjectToSurface(underground));
	const CargoType cargo = rig.industry->produced.front().cargo;
	local->goods[cargo].rating = UINT8_MAX;
	local->goods[cargo].last_speed = 100;
	surface->goods[cargo].rating = UINT8_MAX;
	surface->goods[cargo].last_speed = 100;
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	CHECK(PrepareExactStationSupply(surface->index, cargo, 4, {rig.industry->index, SourceType::Industry}, MapLayer::Underground, _current_company).amount == 0);
	CHECK(DispatchLayerMineCargo(*rig.industry) == 4);
	CHECK(local->goods[cargo].TotalCount() == 4);
	CHECK(surface->goods[cargo].TotalCount() == 0);
	CHECK(surface->goods[cargo].link_graph == LinkGraphID::Invalid());
	REQUIRE(local->goods[cargo].link_graph != LinkGraphID::Invalid());
	CHECK(LinkGraph::Get(local->goods[cargo].link_graph)->Size() == 1);
	CHECK_FALSE(rig.industry->stations_near.contains(surface));
}
