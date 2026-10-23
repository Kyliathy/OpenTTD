/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_adoption.cpp Complete native receipt adoption against real physical component owners.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical stock ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-h-cargo-custody.md immutable candidate joins.
 * No original saved world or paid Space continuation is loaded by these tests.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "surface_supply_legacy_fixture.h"
#include "../surface_supply_cmd.h"
#include "../launch_center.h"
#include "../safeguards.h"

/** Actual source stock admits one complete legacy book without charging, spawning or changing its original departure evidence. */
TEST_CASE("Native staged adoption joins legacy source stock without inventing purchases", "[surface-supply-adoption]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	auto &queue = Station::Get(rig.station)->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo;
	queue.Append(legacy.Packet(), StationID::Invalid()); const auto candidate = legacy.Candidate();
	const auto cash = Company::Get(_current_company)->money; const auto packets = LegacyComponentPacketFacts(); const auto random = _random;
	legacy.Restore(candidate); CHECK(CaptureSurfaceSupplyOrders() == candidate); CHECK(ValidateSurfaceSupplyOrderBook());
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 100);
	CHECK(queue.TotalCount() == 100); CHECK(queue.CountUnshippedCompanySupply(1) == 100);
	CHECK(LegacyComponentPacketFacts() == packets); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	// An absent family cannot erase the only history explaining linked stock.
	CHECK_FALSE(AcceptLoadedSurfaceSupplyOrders(false)); CHECK(CaptureSurfaceSupplyOrders() == candidate);
}

/** Changed clocks, references and receipt scalars cannot replace a previously admitted book or repair its real cargo. */
TEST_CASE("Native legacy adoption refuses contradictory candidates without changing installed owners", "[surface-supply-adoption]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	auto &queue = Station::Get(rig.station)->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo;
	queue.Append(legacy.Packet(), StationID::Invalid()); const auto installed = legacy.Candidate(); legacy.Restore(installed);
	auto candidate = installed; auto day = TimerGameEconomy::date; const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
	if (fault == 0) candidate.records[0].Legacy()->unit_price = 0;
	if (fault == 1) candidate.records[0].quantity = 0;
	if (fault == 2) { candidate.records.clear(); candidate.next_id = 1; }
	if (fault == 3) candidate.records[0].origin = StationID{999};
	if (fault == 4) day = TimerGameEconomy::Date{day.base() + 1};
	if (fault == 7) candidate.records[0].label = CargoLabel{"STEL"};
	if (fault == 8) candidate.records[0].Legacy()->dispatched = 1;
	if (fault == 9) candidate.records[0].Legacy()->closed = true;
	const auto packets = LegacyComponentPacketFacts(); const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	REQUIRE(StageLoadedSurfaceSupplyOrders(candidate, day)); CHECK_FALSE(StageLoadedSurfaceSupplyOrders(installed, day));
	if (fault != 5) REQUIRE(ObserveLoadedSurfaceSupplyEconomyDate(TimerGameEconomy::date));
	if (fault != 6) REQUIRE(ObserveLoadedSurfaceSupplyMap(Map::SizeX(), Map::SizeY(), static_cast<uint8_t>(Map::LayerCount())));
	CHECK_FALSE(AcceptLoadedSurfaceSupplyOrders(true)); CHECK(CaptureSurfaceSupplyOrders() == installed);
	CHECK(LegacyComponentPacketFacts() == packets); CHECK(queue.TotalCount() == 100); CHECK(queue.CountUnshippedCompanySupply(1) == 100);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 100);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Allocated paid stock is not custody; malformed original payer evidence cannot invent a B purchaser either. */
TEST_CASE("Native legacy adoption rejects paid orphans and invented original buyers", "[surface-supply-adoption]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	const bool orphan = GENERATE(false, true); const auto candidate = legacy.Candidate();
	auto *packet = legacy.Packet(100, false, 1, Source{orphan ? Source::Invalid : SourceID{_current_company.base()}, SourceType::CompanySupply});
	if (!orphan) Station::Get(rig.station)->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo.Append(packet, StationID::Invalid());
	const auto packets = LegacyComponentPacketFacts(); const auto cash = Company::Get(_current_company)->money;
	REQUIRE(StageLoadedSurfaceSupplyOrders(candidate, TimerGameEconomy::date)); legacy.ObserveWorld();
	CHECK_FALSE(AcceptLoadedSurfaceSupplyOrders(true)); CHECK(CaptureSurfaceSupplyOrders().records.empty());
	CHECK(GetNextSurfaceSupplyOrderID() == 1); CHECK(LegacyComponentPacketFacts() == packets); CHECK(Company::Get(_current_company)->money == cash);
	if (orphan) delete packet; // Only this deliberately unowned native component needs explicit teardown.
}

/** Two plausible real containers cannot both own one paid packet even when all scalar history is consistent. */
TEST_CASE("Native legacy adoption refuses duplicate actual station and handling ownership", "[surface-supply-adoption]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	auto *source = Station::Get(rig.station); const auto cargo = GetCargoTypeByLabel(CargoLabel{"RFMT"});
	auto *packet = legacy.Packet(100, true); auto &queue = source->goods[cargo].GetOrCreateData().cargo; queue.Append(packet, StationID::Invalid());
	const auto installed = legacy.Candidate(100, 100); legacy.Restore(installed);
	source->launch_hub = std::make_unique<LaunchCollectionHub>(); auto job = std::make_unique<SurfaceHandlingJob>(); job->cargo = cargo;
	job->packets.Append(packet, StationID::Invalid()); source->launch_hub->handling.push_back(std::move(job));
	const auto packets = LegacyComponentPacketFacts(); const auto cash = Company::Get(_current_company)->money;
	const bool staged = StageLoadedSurfaceSupplyOrders(installed, TimerGameEconomy::date);
	const bool date = ObserveLoadedSurfaceSupplyEconomyDate(TimerGameEconomy::date);
	const bool map = ObserveLoadedSurfaceSupplyMap(Map::SizeX(), Map::SizeY(), static_cast<uint8_t>(Map::LayerCount()));
	CHECK(staged); CHECK(date); CHECK(map); CHECK_FALSE(AcceptLoadedSurfaceSupplyOrders(true)); CHECK_FALSE(ValidateSurfaceSupplyOrderBook());
	CHECK(CaptureSurfaceSupplyOrders() == installed); CHECK(LegacyComponentPacketFacts() == packets); CHECK(queue.TotalCount() == 100);
	CHECK(source->launch_hub->handling[0]->packets.TotalCount() == 100); CHECK(Company::Get(_current_company)->money == cash);
	// Remove only the intentional alias before normal teardown; the actual
	// goods queue remains the unique owner and deletes its packet exactly once.
	source->launch_hub->handling[0]->packets.OnCleanPool(); source->launch_hub->handling[0]->packets.InvalidateCache(); source->launch_hub.reset();
	CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Full original history survives even beyond new-purchase policy; transport does not enlarge native purchase or decoder bounds. */
TEST_CASE("Native adoption retains all 65535 legacy rows without enlarging new purchase admission", "[surface-supply-adoption]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	AutoRestoreBackup day(TimerGameEconomy::date, TimerGameEconomy::Date{100});
	auto candidate = legacy.Candidate(1, 1); auto row = candidate.records.front(); row.purchase_day = TimerGameEconomy::Date{0}; row.Legacy()->closed = true;
	candidate.records.clear(); candidate.records.reserve(MAX_LEGACY_SURFACE_SUPPLY_RECORDS);
	// Preserve every dense original identity, including closed zero-stock
	// history; no pruning can manufacture free space or reusable IDs here.
	for (uint64_t id = 1; id <= MAX_LEGACY_SURFACE_SUPPLY_RECORDS; ++id) { row.id = id; candidate.records.push_back(row); }
	candidate.next_id = MAX_LEGACY_SURFACE_SUPPLY_RECORDS + 1; const auto cash = Company::Get(_current_company)->money; const auto packets = LegacyComponentPacketFacts();
	legacy.Restore(candidate); CHECK(CaptureSurfaceSupplyOrders() == candidate); CHECK(ValidateSurfaceSupplyOrderBook());
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 0);
	CHECK(CmdPlaceSurfaceSupplyOrder({}, rig.station, 0x52464D54, 1, candidate.next_id).Failed());
	CHECK(CmdPlaceSurfaceSupplyOrder(DoCommandFlag::Execute, rig.station, 0x52464D54, 1, candidate.next_id).Failed());
	CHECK(CaptureSurfaceSupplyOrders() == candidate); CHECK(Company::Get(_current_company)->money == cash); CHECK(LegacyComponentPacketFacts() == packets);
	auto oversized = candidate; row.id = oversized.next_id++; oversized.records.push_back(row);
	CHECK_FALSE(StageLoadedSurfaceSupplyOrders(std::move(oversized), TimerGameEconomy::date));
	SurfaceSupplyOrderSnapshot native; native.next_id = MAX_SURFACE_SUPPLY_RECORDS + 2; native.records.resize(MAX_SURFACE_SUPPLY_RECORDS + 1);
	CHECK_FALSE(StageLoadedSurfaceSupplyOrders(std::move(native), TimerGameEconomy::date)); CHECK(CaptureSurfaceSupplyOrders() == candidate);
}

/** Genuine pre-family absence can retain actual unlinked native stock without assigning it a legacy refund history. */
TEST_CASE("Old family absence preserves actual unlinked A supplies", "[surface-supply-adoption]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	auto &queue = Station::Get(rig.station)->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo;
	queue.Append(legacy.Packet(3, false, 0, Source{_current_company.base(), SourceType::CompanySupply}), StationID::Invalid());
	const auto packets = LegacyComponentPacketFacts(); REQUIRE(AcceptLoadedSurfaceSupplyOrders(false)); CHECK(ValidateSurfaceSupplyOrderBook());
	CHECK(CaptureSurfaceSupplyOrders().records.empty()); CHECK(LegacyComponentPacketFacts() == packets); CHECK(queue.TotalCount() == 3);
}
