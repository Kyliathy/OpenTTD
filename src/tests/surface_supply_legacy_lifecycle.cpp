/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_legacy_lifecycle.cpp Native lifecycle events over one mixed receipt book and real source queues.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical ownership and loss.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group16; closes Group15's actual takeover-event deferral.
 * Initial original legacy receipts are declared components. All later purchase/refund/owner events use native boundaries.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "surface_supply_legacy_fixture.h"
#include "../surface_supply_cmd.h"
#include "../command_func.h"
#include "../timer/timer_game_tick.h"
#include "../safeguards.h"

/** Actual native station dispatch, never a private material producer. */
extern void OnTick_Station();

/** Buy RFMT through the actual native finance kernel.
 * @param station Actual owned Surface receiver.
 * @param quantity Whole paid native units.
 * @param intent Exact monotonic uint64 identity.
 * @return Native Property settlement or unchanged refusal.
 */
static CommandCost PlaceLifecycleSupply(StationID station, uint16_t quantity, SurfaceSupplyOrderID intent)
{
	return Command<Commands::PlaceSurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, INVALID_TILE,
		std::make_tuple(station, uint32_t{0x52464D54}, quantity, intent));
}

/** Cancel only a native Pending receipt through original Property accounting.
 * @param intent Exact original native identity.
 * @return Once-only native result; this cannot cancel a legacy row.
 */
static CommandCost CancelLifecycleNative(SurfaceSupplyOrderID intent)
{
	return Command<Commands::CancelSurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, INVALID_TILE, std::make_tuple(intent));
}

/** Close actual eligible legacy stock through the original Other account.
 * @param intent Exact original legacy identity.
 * @param expected Complete actual never-shipped source count.
 * @return Whole frozen refund or unchanged refusal.
 */
static CommandCost CancelLifecycleLegacy(SurfaceSupplyOrderID intent, uint16_t expected)
{
	return Command<Commands::CancelLegacySurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, INVALID_TILE,
		std::make_tuple(intent, expected));
}

/** Legacy cancellation consumes the whole original day cap forever, while old native terminal rows can retire without changing physical purpose. */
TEST_CASE("Mixed legacy and native day usage survives cancellation rollover and sparse native retirement", "[surface-supply-legacy-lifecycle]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	AutoRestoreBackup day(TimerGameEconomy::date); const auto original_day = TimerGameEconomy::date;
	auto &queue = Station::Get(rig.station)->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo;
	queue.Append(legacy.Packet(), StationID::Invalid()); legacy.Restore(legacy.Candidate());
	REQUIRE(PlaceLifecycleSupply(rig.station, 20, 2).Succeeded()); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 120);
	REQUIRE(CancelLifecycleLegacy(1, 100).Succeeded()); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 120);
	REQUIRE(CancelLifecycleNative(2).Succeeded()); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 100);
	TimerGameEconomy::date = TimerGameEconomy::Date{original_day.base() + 1}; REQUIRE(PlaceLifecycleSupply(rig.station, 10, 3).Succeeded());
	CHECK(GetSurfaceSupplyOrder(2) == nullptr); REQUIRE(GetSurfaceSupplyOrder(1) != nullptr); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->closed);
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 10);
	CHECK(CountSurfaceSupplyDayUsage(CaptureSurfaceSupplyOrders(), _current_company, CargoLabel{"RFMT"}, original_day) == 100);
	AutoRestoreBackup counter(TimerGameTick::counter, TimerGameTick::TickCounter{0}); OnTick_Station(); REQUIRE(queue.TotalCount() == 10);
	const auto packets = LegacyComponentPacketFacts(); TimerGameEconomy::date = TimerGameEconomy::Date{original_day.base() + 2};
	REQUIRE(PlaceLifecycleSupply(rig.station, 1, 4).Succeeded()); CHECK(GetSurfaceSupplyOrder(3) == nullptr); CHECK(GetNextSurfaceSupplyOrderID() == 5);
	CHECK(PlaceLifecycleSupply(rig.station, 1, 3).Failed()); CHECK(LegacyComponentPacketFacts() == packets); CHECK(queue.TotalCount() == 10);
	CHECK(packets.size() == 1); CHECK(packets.front().source.IsCompanySupply()); CHECK(packets.front().order == 0); CHECK(packets.front().feeder_share == 0);
	CHECK(GetSurfaceSupplyOrder(1)->Legacy()->refunded == 100); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Real berth removal preserves B's sign-owned refund, while final station destruction permanently retires that original source identity. */
TEST_CASE("Legacy source rights survive actual berth removal but not station deletion or slot reuse", "[surface-supply-legacy-lifecycle]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	const auto tile = Station::Get(rig.station)->xy; const auto cargo = GetCargoTypeByLabel(CargoLabel{"RFMT"});
	Station::Get(rig.station)->goods[cargo].GetOrCreateData().cargo.Append(legacy.Packet(4), StationID::Invalid()); legacy.Restore(legacy.Candidate(4));
	const auto cash = Company::Get(_current_company)->money;
	const auto removed = Command<Commands::RemoveFromRailStation>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile, tile, false));
	REQUIRE(removed.Succeeded()); REQUIRE(Station::IsValidID(rig.station)); CHECK_FALSE(Station::Get(rig.station)->facilities.Test(StationFacility::Train));
	CHECK_FALSE(GetSurfaceSupplyOrder(1)->Legacy()->closed); CHECK(GetSurfaceSupplyOrder(1)->origin == rig.station);
	const auto quote = PrepareLegacySurfaceSupplyCancellation(1); REQUIRE(quote.has_value()); CHECK(quote->quantity == 4);
	CHECK(Company::Get(_current_company)->money == cash - removed.GetCost()); CHECK(ValidateSurfaceSupplyOrderBook());
	const auto settled = Company::Get(_current_company)->money; const auto packet_count = CargoPacket::GetNumItems();
	delete Station::Get(rig.station); CHECK(CargoPacket::GetNumItems() == packet_count - 1); CHECK(Company::Get(_current_company)->money == settled);
	CHECK(GetSurfaceSupplyOrder(1)->Legacy()->closed); CHECK(GetSurfaceSupplyOrder(1)->origin == StationID::Invalid()); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->refunded == 0);
	ReconcileSurfaceSupplyStation(rig.station, true); CHECK(Company::Get(_current_company)->money == settled); CHECK(ValidateSurfaceSupplyOrderBook());
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	REQUIRE(GetStationIndex(tile) == rig.station); CHECK(CancelLifecycleLegacy(1, 4).Failed()); CHECK(CancelLifecycleLegacy(1, 0).Failed());
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 4); REQUIRE(PlaceLifecycleSupply(rig.station, 1, 2).Succeeded());
	CHECK(GetSurfaceSupplyOrder(1)->origin == StationID::Invalid()); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** B never stored native anchor coordinates; moving the same real Surface sign cannot fabricate an anchor-loss refund or cancel its stock entitlement. */
TEST_CASE("Legacy source sign movement retains actual rights while native Pending anchor policy remains intact", "[surface-supply-legacy-lifecycle]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	auto *source = Station::Get(rig.station); auto &queue = source->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo;
	queue.Append(legacy.Packet(4), StationID::Invalid()); legacy.Restore(legacy.Candidate(4)); const auto tile = TileXYL(21, 20, MapLayer::Surface);
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	REQUIRE(PlaceLifecycleSupply(rig.station, 2, 2).Succeeded()); const auto cash = Company::Get(_current_company)->money;
	source->MoveSign(tile); CHECK(GetSurfaceSupplyOrder(2)->Native()->state == SurfaceSupplyOrderState::Cancelled);
	CHECK(Company::Get(_current_company)->money == cash + 360); CHECK_FALSE(GetSurfaceSupplyOrder(1)->Legacy()->closed); CHECK(queue.TotalCount() == 4);
	ReconcileSurfaceSupplyStation(rig.station); CHECK(Company::Get(_current_company)->money == cash + 360);
	REQUIRE(CancelLifecycleLegacy(1, 4).Succeeded()); CHECK(Company::Get(_current_company)->money == cash + 460); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Actual takeover transfers all finite legacy history after station ownership, including over-cap sums, without inventing an original payer. */
TEST_CASE("Native company takeover transfers legacy refund and mixed caps without any duplicate payment", "[surface-supply-legacy-lifecycle]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	AutoRestoreBackup local(_local_company, COMPANY_SPECTATOR); const auto original = _current_company; const auto cargo = GetCargoTypeByLabel(CargoLabel{"RFMT"});
	Station::Get(rig.station)->goods[cargo].GetOrCreateData().cargo.Append(legacy.Packet(), StationID::Invalid()); legacy.Restore(legacy.Candidate());
	REQUIRE(PlaceLifecycleSupply(rig.station, 20, 2).Succeeded()); const auto original_cash = Company::Get(original)->money;
	REQUIRE(Company::CanAllocateItem()); auto successor = std::unique_ptr<Company>(Company::Create()); successor->money = 1000000;
	successor->avail_railtypes.Set(RAILTYPE_RAIL); successor->clear_limit = 64U << 16; const auto next = successor->index; StationID other;
	{
		AutoRestoreBackup actor(_current_company, next); const auto tile = TileXYL(40, 20, MapLayer::Surface);
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		other = GetStationIndex(tile); REQUIRE(PlaceLifecycleSupply(other, 128, 3).Succeeded());
	}
	const auto cash = successor->money; const auto before = LegacyComponentPacketFacts(); ChangeOwnershipOfCompanyItems(original, next);
	CHECK(successor->money == cash); CHECK(Company::Get(original)->money == original_cash); CHECK(LegacyComponentPacketFacts() == before);
	CHECK(Station::Get(rig.station)->owner == next); CHECK(GetSurfaceSupplyOrder(1)->owner == next); CHECK_FALSE(GetSurfaceSupplyOrder(1)->Legacy()->closed);
	CHECK(GetSurfaceSupplyOrder(2)->Native()->purchaser == original); CHECK(GetSurfaceSupplyOrder(2)->Native()->packet_buyer == Source::Invalid);
	CHECK(GetSurfaceSupplyOrderDayUsage(next, CargoLabel{"RFMT"}) == 248); CHECK(GetSurfaceSupplyOrderDayUsage(original, CargoLabel{"RFMT"}) == 0);
	CHECK(CancelLifecycleLegacy(1, 100).Failed());
	{
		AutoRestoreBackup actor(_current_company, next); CHECK(PlaceLifecycleSupply(other, 1, 4).Failed()); REQUIRE(CancelLifecycleLegacy(1, 100).Succeeded());
		CHECK(successor->money == cash + 2500); CHECK(GetSurfaceSupplyOrderDayUsage(next, CargoLabel{"RFMT"}) == 248);
		REQUIRE(CancelLifecycleNative(2).Succeeded()); CHECK(successor->money == cash + 6100); CHECK(GetSurfaceSupplyOrderDayUsage(next, CargoLabel{"RFMT"}) == 228);
		CHECK(PlaceLifecycleSupply(other, 1, 4).Failed()); CHECK(ValidateSurfaceSupplyOrderBook());
		const auto expenses = successor->yearly_expenses[0]; ChangeOwnershipOfCompanyItems(next, INVALID_OWNER);
		// Native bankruptcy installs a temporary demolition budget independently
		// of paid supplies. Unchanged expense accounts, not the retired wallet,
		// prove that the still-Pending native order and legacy history never refund.
		CHECK(successor->money == Money{UINT64_MAX >> 2}); CHECK(successor->yearly_expenses[0] == expenses);
		CHECK(GetSurfaceSupplyOrder(1)->owner == CompanyID::Invalid()); CHECK(GetSurfaceSupplyOrder(1)->origin == StationID::Invalid()); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->closed);
		CHECK(ValidateSurfaceSupplyOrderBook());
	}
}

/** Company loss retains real unpaid-to-freight purpose and original history, and slot reuse grants no refund to the new company. */
TEST_CASE("Native company loss seals legacy rights while ownerless packets and ordinary purpose remain intact", "[surface-supply-legacy-lifecycle]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	AutoRestoreBackup local(_local_company, COMPANY_SPECTATOR); const auto original = _current_company; const auto cargo = GetCargoTypeByLabel(CargoLabel{"RFMT"});
	auto &queue = Station::Get(rig.station)->goods[cargo].GetOrCreateData().cargo; queue.Append(legacy.Packet(), StationID::Invalid());
	queue.Append(legacy.Packet(7, false, 0, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid()); legacy.Restore(legacy.Candidate());
	const auto expenses = Company::Get(original)->yearly_expenses[0]; const auto packets = LegacyComponentPacketFacts(); ChangeOwnershipOfCompanyItems(original, INVALID_OWNER);
	// The native demolition budget replaces cash before lifecycle callbacks;
	// zero ledger credit and zero refunded units prove loss grants no entitlement.
	const Money demolition_budget{UINT64_MAX >> 2}; CHECK(Company::Get(original)->money == demolition_budget);
	CHECK(Company::Get(original)->yearly_expenses[0] == expenses); CHECK(GetSurfaceSupplyOrder(1)->owner == CompanyID::Invalid()); CHECK(GetSurfaceSupplyOrder(1)->origin == StationID::Invalid());
	CHECK(GetSurfaceSupplyOrder(1)->Legacy()->closed); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->refunded == 0); CHECK(LegacyComponentPacketFacts() == packets);
	CHECK(queue.TotalCount() == 107); CHECK(Station::Get(rig.station)->owner == OWNER_NONE); CHECK(ValidateSurfaceSupplyOrderBook());
	const auto history = CaptureSurfaceSupplyOrders(); ChangeSurfaceSupplyOrderCompanyOwnership(original, CompanyID::Invalid()); ReconcileSurfaceSupplyStation(rig.station);
	CHECK(CaptureSurfaceSupplyOrders() == history); CHECK(Company::Get(original)->money == demolition_budget);
	CHECK(Company::Get(original)->yearly_expenses[0] == expenses); CHECK(LegacyComponentPacketFacts() == packets);
	rig.RemoveCompany(); REQUIRE(Company::CanAllocateItem()); auto *replacement = Company::Create(); REQUIRE(replacement->index == original); replacement->money = 50000;
	{
		AutoRestoreBackup actor(_current_company, replacement->index); CHECK(CancelLifecycleLegacy(1, 100).Failed()); CHECK(CancelLifecycleLegacy(1, 0).Failed());
		CHECK(GetSurfaceSupplyOrderDayUsage(replacement->index, CargoLabel{"RFMT"}) == 0); CHECK(replacement->money == 50000); CHECK(ValidateSurfaceSupplyOrderBook());
	}
	delete replacement;
}

/** All mandatory imported history counts toward the original purchase limit, while safe native retirement can free only its own slot. */
TEST_CASE("Legacy retained history keeps native admission bounded without truncation or ID reuse", "[surface-supply-legacy-lifecycle]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	AutoRestoreBackup day(TimerGameEconomy::date, TimerGameEconomy::Date{100}); auto candidate = legacy.Candidate(1, 1);
	auto row = candidate.records.front(); row.Legacy()->closed = true; row.purchase_day = TimerGameEconomy::Date{0}; candidate.records.clear();
	// Retain the complete original dense legacy prefix. Safe native retirement
	// must neither truncate it to make room nor reopen its original identities.
	for (uint64_t id = 1; id < MAX_SURFACE_SUPPLY_RECORDS; ++id) { row.id = id; candidate.records.push_back(row); }
	candidate.next_id = MAX_SURFACE_SUPPLY_RECORDS; legacy.Restore(candidate);
	REQUIRE(PlaceLifecycleSupply(rig.station, 1, MAX_SURFACE_SUPPLY_RECORDS).Succeeded()); const auto full = CaptureSurfaceSupplyOrders(); const auto cash = Company::Get(_current_company)->money;
	CHECK(full.records.size() == MAX_SURFACE_SUPPLY_RECORDS); CHECK(PlaceLifecycleSupply(rig.station, 1, MAX_SURFACE_SUPPLY_RECORDS + 1).Failed());
	CHECK(CaptureSurfaceSupplyOrders() == full); CHECK(Company::Get(_current_company)->money == cash); REQUIRE(CancelLifecycleNative(MAX_SURFACE_SUPPLY_RECORDS).Succeeded());
	CHECK(PlaceLifecycleSupply(rig.station, 1, MAX_SURFACE_SUPPLY_RECORDS + 1).Failed()); TimerGameEconomy::date = TimerGameEconomy::Date{101};
	REQUIRE(PlaceLifecycleSupply(rig.station, 1, MAX_SURFACE_SUPPLY_RECORDS + 1).Succeeded()); CHECK(GetSurfaceSupplyOrder(MAX_SURFACE_SUPPLY_RECORDS) == nullptr);
	const auto retained = CaptureSurfaceSupplyOrders(); REQUIRE(retained.records.size() == MAX_SURFACE_SUPPLY_RECORDS);
	CHECK(std::equal(candidate.records.begin(), candidate.records.end(), retained.records.begin())); CHECK(GetNextSurfaceSupplyOrderID() == MAX_SURFACE_SUPPLY_RECORDS + 2);
	CHECK(PlaceLifecycleSupply(rig.station, 1, MAX_SURFACE_SUPPLY_RECORDS).Failed()); CHECK(ValidateSurfaceSupplyOrderBook());
}
