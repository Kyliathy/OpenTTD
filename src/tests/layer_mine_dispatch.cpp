/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_mine_dispatch.cpp Exact finite output tickets and real station packets. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 8 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mine_production_fixture.h"

#include "../safeguards.h"

/**
 * Create a real catchment-associated station with loading service and a declared rating.
 * @param rig Actual funded native mine/world fixture.
 * @param rating Station rating to exercise throughput admission.
 * @return Actual fixture-owned station; the shared fixture removes its real packets and station.
 */
static Station *MakeMineSupplyStation(MineProductionRig &rig, uint8_t rating = UINT8_MAX)
{
	Station *station = rig.StationAt(TileAddWrap(rig.request.tile, 3, 0));
	GoodsEntry &goods = station->goods[rig.industry->produced[0].cargo];
	goods.rating = rating;
	goods.last_speed = 100;
	REQUIRE(rig.industry->stations_near.contains(station));
	return station;
}

/**
 * Count real reserve, mine output and station packets at a conservation checkpoint.
 * @param rig Actual finite producer.
 * @param station Actual receiving station, or nullptr before any station exists.
 * @return Whole accountable units, including pre-existing station cargo in the checkpoint.
 */
static uint64_t MineSupplyTotal(const MineProductionRig &rig, const Station *station)
{
	const auto &output = rig.industry->produced.front();
	return rig.Reserve() + output.waiting + (station == nullptr ? 0 : station->goods[output.cargo].TotalCount());
}

/** No service means stored finite output; zero rating and vertical/foreign catchment offer no alternative handoff. */
TEST_CASE("Finite mine output remains when no eligible station can take it", "[layer-mine-dispatch]")
{
	MineProductionRig rig;
	AutoRestoreBackup selective_service(_settings_game.order.selectgoods, false);
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	Station *station = nullptr;
	SECTION("No station") {}
	SECTION("Zero station rating") { station = MakeMineSupplyStation(rig, 0); }
	SECTION("Station has no loading service") { station = MakeMineSupplyStation(rig); station->goods[rig.industry->produced[0].cargo].last_speed = 0; _settings_game.order.selectgoods = true; }
	SECTION("Unowned neighbouring station") { station = MakeMineSupplyStation(rig); station->owner = OWNER_NONE; }
	SECTION("Coincident Surface station") { station = rig.StationAt(MultiLayerMap::ProjectToSurface(TileAddWrap(rig.request.tile, 3, 0))); }
	const auto total = MineSupplyTotal(rig, station);
	const auto revision = GetLayerMineState(rig.industry->index)->revision;
	CHECK(DispatchLayerMineCargo(*rig.industry) == 0);
	CHECK(rig.industry->produced[0].waiting == 4);
	CHECK(rig.industry->produced[0].history[THIS_MONTH].production == 4);
	CHECK(rig.industry->produced[0].history[THIS_MONTH].transported == 0);
	CHECK(GetLayerMineState(rig.industry->index)->revision == revision);
	CHECK(MineSupplyTotal(rig, station) == total);
}

/** Whole-unit station supply leaves legacy fractional credit untouched, even across repeated small outputs. */
TEST_CASE("Finite station tickets conserve rated partial output and reject duplicate commit", "[layer-mine-dispatch]")
{
	MineProductionRig rig;
	Station *station = MakeMineSupplyStation(rig, 127);
	const CargoType cargo = rig.industry->produced[0].cargo;
	station->goods[cargo].amount_fract = 77;
	rig.SetReserve(3);
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 3);
	const auto total = MineSupplyTotal(rig, station);
	const auto ticket = PrepareLayerMineDispatch(*rig.industry);
	CHECK(ticket.station.amount == 2);
	CHECK(station->goods[cargo].TotalCount() == 0);
	CHECK(CommitLayerMineDispatch(*rig.industry, ticket) == 2);
	CHECK(CommitLayerMineDispatch(*rig.industry, ticket) == 0);
	CHECK(rig.industry->produced[0].waiting == 1);
	CHECK(station->goods[cargo].TotalCount() == 2);
	CHECK(station->goods[cargo].amount_fract == 77);
	CHECK(rig.industry->produced[0].history[THIS_MONTH].production == 3);
	CHECK(rig.industry->produced[0].history[THIS_MONTH].transported == 2);
	CHECK(DispatchLayerMineCargo(*rig.industry) == 1);
	CHECK(station->goods[cargo].TotalCount() == 3);
	CHECK(station->goods[cargo].amount_fract == 77);
	CHECK(GetLayerMineState(rig.industry->index)->dispatched == 3);
	CHECK(MineSupplyTotal(rig, station) == total);
}

/** Actual packet-backed queue space constrains the exact receipt; a full queue preserves remaining mine material. */
TEST_CASE("Finite supply respects partial station storage and narrow history capacity", "[layer-mine-dispatch]")
{
	MineProductionRig rig;
	Station *station = MakeMineSupplyStation(rig);
	const CargoType cargo = rig.industry->produced[0].cargo;
	REQUIRE(CargoPacket::CanAllocateItem());
	// Existing real packet stock sets a queue-capacity checkpoint; it is neither a shadow count nor a fabricated allocator.
	station->goods[cargo].GetOrCreateData().cargo.Append(CargoPacket::Create(station->index, UINT16_MAX - 1, Source{rig.industry->index, SourceType::Industry}), StationID::Invalid());
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	const auto total = MineSupplyTotal(rig, station);
	CHECK(DispatchLayerMineCargo(*rig.industry) == 1);
	CHECK(station->goods[cargo].TotalCount() == EXACT_STATION_SUPPLY_CAPACITY);
	CHECK(rig.industry->produced[0].waiting == 3);
	CHECK(DispatchLayerMineCargo(*rig.industry) == 0);
	CHECK(MineSupplyTotal(rig, station) == total);
	rig.industry->produced[0].history[THIS_MONTH].production = UINT16_MAX - 1;
	CHECK(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 1);
	CHECK(rig.industry->produced[0].history[THIS_MONTH].production == UINT16_MAX);
	const auto reserve = rig.Reserve();
	CHECK(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 0);
	CHECK(rig.Reserve() == reserve);
}

/** Stale source/service/queue and edited quotes refuse without partial station supply or source debit. */
TEST_CASE("Finite dispatch tickets revalidate both physical owners", "[layer-mine-dispatch]")
{
	MineProductionRig rig;
	Station *station = MakeMineSupplyStation(rig);
	const CargoType cargo = rig.industry->produced[0].cargo;
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	auto ticket = PrepareLayerMineDispatch(*rig.industry);
	SECTION("Source revision changed") { ProduceLayerMineCargo(*rig.industry, 1, true); }
	SECTION("Station rating changed") { --station->goods[cargo].rating; }
	SECTION("Edited quote amount") { ++ticket.station.amount; }
	SECTION("Existing queue changed") {
		REQUIRE(CargoPacket::CanAllocateItem());
		station->goods[cargo].GetOrCreateData().cargo.Append(CargoPacket::Create(station->index, 1, Source{rig.industry->index, SourceType::Industry}), StationID::Invalid());
	}
	const auto total = MineSupplyTotal(rig, station);
	const auto waiting = rig.industry->produced[0].waiting;
	const auto station_waiting = station->goods[cargo].TotalCount();
	CHECK(CommitLayerMineDispatch(*rig.industry, ticket) == 0);
	CHECK(rig.industry->produced[0].waiting == waiting);
	CHECK(station->goods[cargo].TotalCount() == station_waiting);
	CHECK(GetLayerMineState(rig.industry->index)->dispatched == 0);
	CHECK(MineSupplyTotal(rig, station) == total);
}

/** Rate/recession modifiers reduce work before material exists; stock and station dispatch never discard a percentage. */
TEST_CASE("Finite mine scale and recession preserve deterministic earned work", "[layer-mine-dispatch]")
{
	MineProductionRig rig;
	uint ticks = 296;
	SECTION("Quarter industry rate") { _settings_game.economy.industry_cargo_scale = 25; }
	SECTION("Recession halves work") { _economy.fluct = 0; ticks = 148; }
	const auto reserve = rig.Reserve();
	// Repeated real economy ticks test the saved scale remainder rather than a once-rounded allowance.
	for (uint tick = 0; tick < ticks; ++tick) OnTick_Industry();
	CHECK(rig.industry->produced[0].waiting == 4);
	CHECK(rig.Reserve() == reserve - 4);
	CHECK(rig.industry->produced[0].history[THIS_MONTH].production == 4);
	CHECK(GetLayerMineState(rig.industry->index)->scale_fraction == 0);
}
