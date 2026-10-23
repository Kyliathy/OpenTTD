/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_goods_source.cpp Earn original1975 Goods before the dependent Crate useful route.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md task15CB.1.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * This Surface source/receiver gate does not claim an autonomous capsule route or actual upstream TBM/bootstrap.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "tube_goods_source_fixture.h"
#include "../safeguards.h"

/** @param rig Actual1975 native source participants. Earn the original Goods with native production, physical Wood delivery and a complete empty return, retaining the world for the dependent Crate route. */
void EarnTubeGoodsSource(TubeGoodsSourceRig &rig)
{
	REQUIRE(rig.sawmill->produced[0].waiting == 0); REQUIRE(CargoPacket::GetNumItems() == 0);
	rig.wood_pickup->goods[rig.wood].rating = UINT8_MAX; rig.wood_pickup->goods[rig.wood].last_speed = 100;
	uint opportunities = 0;
	// Original Forest dispatch requires more than its native30-unit minimum.
	// Earn complete original primary batches until the first genuine dispatch
	// lot exists, rather than lowering that threshold or supplying extra stock.
	const auto minimum = GetIndustrySpec(rig.forest->type)->minimal_cargo;
	while (rig.forest->produced[0].waiting <= minimum && opportunities++ < 1024) { rig.RunSource(); OnTick_Industry(); }
	REQUIRE(opportunities <= 1024); rig.supplied = rig.forest->produced[0].waiting; REQUIRE(rig.supplied > minimum);
	REQUIRE(_game_mode == GameMode::Normal);
	uint source_cells = 0;
	// Visit only genuine completed cells of the original Forest, as the native
	// tile loop does; an empty/missing footprint cannot supply a packet receipt.
	for (const auto tile : rig.forest->location) if (rig.forest->TileBelongsToIndustry(tile)) {
		REQUIRE(IsIndustryCompleted(tile)); ++source_cells; _tile_type_procs[TileType::Industry]->tile_loop_proc(tile);
	}
	REQUIRE(source_cells > 0);
	INFO(fmt::format("forest remaining={} nearby={} production={} transported={} rating={} speed={} town={} houses={} berth={}",
		rig.forest->produced[0].waiting, rig.forest->stations_near.size(), rig.forest->produced[0].history[THIS_MONTH].production,
		rig.forest->produced[0].history[THIS_MONTH].transported, rig.wood_pickup->goods[rig.wood].rating,
		rig.wood_pickup->goods[rig.wood].last_speed, rig.receiver->index.base(), rig.receiver->cache.num_houses, rig.town_berth.base()));
	REQUIRE(rig.wood_pickup->goods[rig.wood].TotalCount() == rig.supplied);
	REQUIRE(rig.sawmill->produced[0].waiting == 0);
	const auto initial_income = Company::Get(_current_company)->cur_economy.income;
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, rig.wood_front->index, true).Succeeded());
	bool carried = false, empty_return = false; uint steps = 0;
	// Normal orders must earn pickup, final Wood delivery and the complete
	// empty home return. The sum follows both native holds and actual Goods
	// output so an arrival callback cannot silently duplicate or lose material.
	while (steps++ < 10000) {
		rig.RunSource();
		CHECK(rig.wood_pickup->goods[rig.wood].TotalCount() + rig.wood_wagon->cargo.StoredCount() + rig.sawmill->produced[0].waiting == rig.supplied);
		if (rig.wood_wagon->cargo.StoredCount() > 0) {
			carried = true;
			for (const auto *packet : *rig.wood_wagon->cargo.Packets()) {
				CHECK(packet->GetSource() == Source{rig.forest->index, SourceType::Industry});
				CHECK(packet->GetFirstStation() == rig.wood_pickup->index); CHECK(packet->GetCompanySupplyOrder() == 0);
			}
		}
		// Whole-consist home storage after paid delivery is the earned return;
		// a hidden locomotive with its wagon still outside, an initial stopped
		// train or an unfinished native payment cannot satisfy closure.
		if (rig.sawmill->produced[0].waiting == rig.supplied && rig.wood_front->IsChainInDepot() && rig.wood_front->tile == rig.depots[0] && rig.wood_front->cargo_payment == nullptr) { empty_return = true; break; }
	}
	INFO(DescribeTrain(rig.wood_front)); REQUIRE(steps < 10000); REQUIRE(carried); REQUIRE(empty_return);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.wood] == rig.supplied);
	CHECK(Company::Get(_current_company)->cur_economy.income > initial_income);
	CHECK(rig.wood_wagon->cargo.StoredCount() == 0); CHECK(rig.wood_pickup->goods[rig.wood].TotalCount() == 0);
	CHECK(rig.goods_pickup->goods[rig.goods].TotalCount() == 0);
	CHECK(TimerGameCalendar::year == TimerGameCalendar::Year{1975});
	fmt::print("GOODS_SOURCE year=1975 source=original_forest converted={} primary_opportunities={} route_ticks={} town={} houses={} berth={} wood_income={}\n",
		rig.supplied, opportunities, steps, rig.receiver->index.base(), rig.receiver->cache.num_houses, rig.town_berth.base(),
		(Company::Get(_current_company)->cur_economy.income - initial_income).base());
}

/** Real original Wood must physically reach the Sawmill before any ordinary Goods exists for the Crate route. */
TEST_CASE("Original1975 Surface production earns genuine Goods for Crate", "[tube-useful-goods-source]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubeGoodsSourceRig rig; EarnTubeGoodsSource(rig);
}
