/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_cross_owner.cpp One legacy stock history across actual native cargo owners.
	* @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical ownership and non-paying purpose.
	* @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group19.
	* Original stock, floor and empty hull/module objects are declared components.
	* Transfers use real native operations; this is not a driven route, built orbital service or original-save acceptance.
	*/
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "native_cargo_custody_fixture.h"
#include "surface_supply_legacy_fixture.h"
#include "../surface_supply_cmd.h"
#include "../command_func.h"
#include "../transport_tube.h"
#include "../transport_tube_cargo.h"
#include "../hub_cargo.h"
#include "../safeguards.h"

/** Exact checked legacy settlement through the native finance kernel.
	* @param expected Actual original-source units that the caller observed.
	* @return Once-only frozen refund for receipt1, or refusal with all owners unchanged.
	*/
static CommandCost CancelCrossOwnerLegacy(uint16_t expected)
{
	return Command<Commands::CancelLegacySurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, INVALID_TILE,
	 std::make_tuple(SurfaceSupplyOrderID{1}, expected));
}

/** A continuous physical remainder survives tube refusal, launch/orbital handling, consumption and return without a second refund or any transport income. */
TEST_CASE("Legacy paid stock crosses conventional tube launch and orbital owners without reopening refunded units", "[surface-supply-cross-owner]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeCargoCustodyRig rig; LegacySupplyComponent legacy(rig.station);
	AutoRestoreBackup calendar(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2080}, 0, 1));
	const auto tube = InitialiseTubeRailType(rig.ActivatedContent()); REQUIRE(tube != INVALID_RAILTYPE); Company::Get(_current_company)->avail_railtypes.Set(tube);
	const auto interchange_tile = TileXYL(40, 40, MapLayer::Underground);
	rig.Open(std::array{TileXYL(40, 39, MapLayer::Underground), interchange_tile, TileXYL(40, 41, MapLayer::Underground)});
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, interchange_tile, tube, Axis::Y, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	auto *interchange = Station::GetByTile(interchange_tile); auto *source = Station::Get(rig.station); const auto cargo = GetCargoTypeByLabel(CargoLabel{"RFMT"});
	auto &origin = source->goods[cargo].GetOrCreateData().cargo; auto &queue = interchange->goods[cargo].GetOrCreateData().cargo;
	auto &handling = source->launch_hub->handling[0]->packets; auto &manifest = rig.craft->manifest->supplies[1];
	auto &module = rig.dock->module->stock[cargo]; auto &service = rig.dock->module->GetServiceJobs()[0]->inputs[0];
	// Teardown witnesses only: each listed container remains its sole physical
	// owner throughout the transfers and shared custody checks below.
	for (auto *list : {&origin, &queue, &handling, &manifest, &module, &service}) rig.Track(*list);
	rig.Own(origin, legacy.Packet()); legacy.Restore(legacy.Candidate());
	const auto cash = Company::Get(_current_company)->money;
	REQUIRE(origin.Load(30, &rig.train->cargo, {}, source->xy) == 30); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->dispatched == 30);
	REQUIRE(CancelCrossOwnerLegacy(70).Succeeded()); CHECK(origin.TotalCount() == 0); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->refunded == 70);
	const Money settled = cash + 1750; CHECK(Company::Get(_current_company)->money == settled); CHECK(ValidateSurfaceSupplyOrderBook());
	// Native ordinary stock consumes1000 of the1024 shared units. Only24 of
	// the30 shipped legacy units may stage; six must remain aboard unchanged.
	rig.Own(queue, legacy.Packet(1000, false, 0, Source{Source::Invalid, SourceType::Industry}));
	rig.train->last_station_visited = interchange->index; REQUIRE(CargoPayment::CanAllocateItem());
	std::unique_ptr<CargoPayment> payment{CargoPayment::Create(rig.train)}; rig.train->cargo_payment = payment.get();
	REQUIRE(rig.train->cargo.Stage(false, interchange->index, {}, OrderUnloadType::Transfer, &interchange->goods[cargo], cargo, payment.get(), interchange_tile));
	CHECK(rig.train->cargo.ActionCount(VehicleCargoList::MoveToAction::Transfer) == 24); CHECK(rig.train->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) == 6);
	REQUIRE(rig.train->cargo.Unload(30, &queue, cargo, payment.get(), interchange_tile) == 24);
	CHECK(queue.TotalCount() == 1024); CHECK(rig.train->cargo.TotalCount() == 6); CHECK(GetTubeInterchangeTransferRoom(interchange->index, cargo, &queue) == 0);
	CHECK(payment->route_profit == 0); CHECK(payment->visual_transfer == 0); CHECK(payment->visual_profit == 0); payment.reset();
	CHECK(Company::Get(_current_company)->money == settled); CHECK(ValidateSurfaceSupplyOrderBook());
	// Consume only the declared ordinary prefix through the actual list/cache
	// owner. No legacy unit or simulated counter is edited to create tube room.
	REQUIRE(HubCargoTransfer::Consume(queue, 1000)); CHECK(queue.TotalCount() == 24);
	REQUIRE(CargoPayment::CanAllocateItem()); payment.reset(CargoPayment::Create(rig.train)); rig.train->cargo_payment = payment.get();
	REQUIRE(rig.train->cargo.Stage(false, interchange->index, {}, OrderUnloadType::Transfer, &interchange->goods[cargo], cargo, payment.get(), interchange_tile));
	REQUIRE(rig.train->cargo.Unload(6, &queue, cargo, payment.get(), interchange_tile) == 6); CHECK(payment->visual_transfer == 0); payment.reset();
	REQUIRE(queue.Load(30, &rig.train->cargo, {}, interchange_tile) == 30); CHECK(queue.TotalCount() == 0); CHECK(ValidateSurfaceSupplyOrderBook());
	REQUIRE(HubCargoTransfer::Move(rig.train->cargo, handling, 30, source->xy)); CHECK(ValidateSurfaceSupplyOrderBook());
	REQUIRE(HubCargoTransfer::Move(handling, manifest, 30)); CHECK(ValidateSurfaceSupplyOrderBook());
	REQUIRE(HubCargoTransfer::Move(manifest, module, 30)); CHECK(ValidateSurfaceSupplyOrderBook());
	REQUIRE(HubCargoTransfer::Move(module, service, 30)); CHECK(ValidateSurfaceSupplyOrderBook());
	REQUIRE(HubCargoTransfer::Consume(service, 10)); CHECK(ValidateSurfaceSupplyOrderBook());
	REQUIRE(HubCargoTransfer::Move(service, module, 20)); REQUIRE(HubCargoTransfer::Move(module, manifest, 20));
	REQUIRE(HubCargoTransfer::Move(manifest, handling, 20)); REQUIRE(HubCargoTransfer::Move(handling, origin, 20));
	CHECK(origin.TotalCount() == 20); CHECK(origin.CountUnshippedCompanySupply(1) == 0);
	CHECK(GetSurfaceSupplyOrder(1)->Legacy()->refunded == 70); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->dispatched == 30);
	CHECK(GetSurfaceSupplyOrder(1)->Legacy()->closed); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 100);
	CHECK(CancelCrossOwnerLegacy(20).Failed()); CHECK(CancelCrossOwnerLegacy(0).Failed()); CHECK(CancelCrossOwnerLegacy(70).Failed());
	// Exactly20 returned units plus10 consumed and70 refunded account for the
	// original100. Every survivor retains non-paying, already-shipped purpose.
	uint remaining = 0;
	for (const auto &facts : LegacyComponentPacketFacts()) {
	 remaining += facts.quantity; CHECK(facts.source.IsCompanySupply()); CHECK(facts.order == 1); CHECK(facts.shipped); CHECK(facts.feeder_share == 0);
	}
	CHECK(remaining == 20); CHECK(Company::Get(_current_company)->money == settled); CHECK(ValidateSurfaceSupplyOrderBook());
}
