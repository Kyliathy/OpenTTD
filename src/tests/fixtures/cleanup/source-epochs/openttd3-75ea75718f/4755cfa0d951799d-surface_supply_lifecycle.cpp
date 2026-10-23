/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_lifecycle.cpp Native cancellation, source loss and company transition conservation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "surface_supply_fixture.h"
#include "../surface_supply_cmd.h"
#include "../hub_cargo.h"
#include "../company_func.h"
#include "../safeguards.h"

/** A real partial hub move and return cannot renew refund rights; only untouched source stock earns its frozen credit. */
TEST_CASE("Surface cancellation removes only never dispatched original source material", "[surface-supply-cancel]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const CargoLabel label{"RFMT"};
	const auto order = PrepareSurfaceSupplyOrder(rig.origin->index, label, 10);
	REQUIRE(PlaceSurfaceSupplyOrder(order, true) == SurfaceSupplyError::None);
	const Money paid_cash = Company::Get(_current_company)->money;
	auto &source = rig.Queue(label);
	auto &relay = rig.second->goods[order.cargo].GetOrCreateData().cargo;
	REQUIRE(HubCargoTransfer::Move(source, relay, 4));
	CHECK(GetSurfaceSupplyOrder(order.next_order)->dispatched == 4);
	REQUIRE(HubCargoTransfer::Move(relay, source, 2));
	CHECK(GetSurfaceSupplyOrder(order.next_order)->dispatched == 4);
	REQUIRE(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
	const auto quote = PrepareSurfaceSupplyCancellation(order.next_order);
	REQUIRE(quote.error == SurfaceSupplyError::None);
	CHECK(quote.quantity == 6); CHECK(quote.refund == Money{1080});
	const auto digest = ComputeSurfaceSupplyDigest();
	CHECK(CancelSurfaceSupplyOrder(quote, false) == SurfaceSupplyError::None);
	CHECK(ComputeSurfaceSupplyDigest() == digest); CHECK(Company::Get(_current_company)->money == paid_cash);
	_economy.inflation_prices = uint64_t{2} << 16; // Cancellation uses the paid tariff, not today's changed import price.
	REQUIRE(CancelSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::None);
	CHECK(Company::Get(_current_company)->money == paid_cash + Money{1080});
	CHECK(source.TotalCount() == 2); CHECK(relay.TotalCount() == 2);
	CHECK(source.CountUnshippedCompanySupply(order.next_order.base()) == 0);
	CHECK(GetSurfaceSupplyOrder(order.next_order)->refunded == 6);
	CHECK(GetSurfaceSupplyOrder(order.next_order)->dispatched == 4);
	CHECK(PrepareSurfaceSupplyOrder(rig.second->index, label, 1).day_used == 10);
	CHECK(CancelSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::Stale);
	CHECK(Company::Get(_current_company)->money == paid_cash + Money{1080});
	CHECK(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
}

/** Stale quantity, actor, edited admission and wallet overflow refuse before deleting stock or paying credit. */
TEST_CASE("Surface cancellation refuses stale and overflowing whole actions unchanged", "[surface-supply-cancel-refusal]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const auto order = PrepareSurfaceSupplyOrder(rig.origin->index, CargoLabel{"STEL"}, 2);
	REQUIRE(PlaceSurfaceSupplyOrder(order, true) == SurfaceSupplyError::None);
	const auto quote = PrepareSurfaceSupplyCancellation(order.next_order);
	REQUIRE(quote.error == SurfaceSupplyError::None);
	const Money cash = Company::Get(_current_company)->money;
	SECTION("edited credit") {
		auto changed = quote; changed.refund = 201;
		CHECK(CancelSurfaceSupplyOrder(changed, true) == SurfaceSupplyError::Stale);
	}
	SECTION("wrong actor") {
		AutoRestoreBackup actor{_current_company, CompanyID::Invalid()};
		CHECK(CancelSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::Company);
	}
	SECTION("native command stale expected quantity") {
		CHECK(Command<Commands::CancelSurfaceSupply>::Do(DoCommandFlag::Execute, order.next_order, uint16_t{1}).Failed());
	}
	SECTION("refund would exceed native money") {
		AutoRestoreBackup<Money> wallet{Company::Get(_current_company)->money, Money{INT64_MAX - 100}};
		CHECK(PrepareSurfaceSupplyCancellation(order.next_order).error == SurfaceSupplyError::RefundLimit);
		CHECK(CancelSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::RefundLimit);
	}
	SECTION("negative wallet is a valid bounded credit") {
		AutoRestoreBackup<Money> wallet{Company::Get(_current_company)->money, Money{-100}};
		CHECK(PrepareSurfaceSupplyCancellation(order.next_order).error == SurfaceSupplyError::None);
		CHECK(CancelSurfaceSupplyOrder(quote, false) == SurfaceSupplyError::None);
	}
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(rig.Queue(CargoLabel{"STEL"}).TotalCount() == 2);
	CHECK_FALSE(GetSurfaceSupplyOrder(order.next_order)->closed);
	CHECK(GetSurfaceSupplyOrder(order.next_order)->refunded == 0);
}

/** Pickup reservations are already dispatched; consuming that material cannot later create a credit. */
TEST_CASE("Surface reserved and consumed material retains historical dispatch", "[surface-supply-dispatch]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const CargoLabel label{"STEL"};
	const auto order = PrepareSurfaceSupplyOrder(rig.origin->index, label, 5);
	REQUIRE(PlaceSurfaceSupplyOrder(order, true) == SurfaceSupplyError::None);
	REQUIRE(Train::CanAllocateItem()); rig.carrier = Train::Create();
	rig.carrier->cargo_type = order.cargo; rig.carrier->owner = _current_company;
	const std::array<StationID, 0> next{};
	REQUIRE(rig.Queue(label).Reserve(2, &rig.carrier->cargo, next, rig.origin->xy) == 2);
	CHECK(rig.carrier->cargo.ReservedCount() == 2);
	CHECK(GetSurfaceSupplyOrder(order.next_order)->dispatched == 2);
	CHECK(PrepareSurfaceSupplyCancellation(order.next_order).quantity == 3);
	CHECK(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
	const auto quote = PrepareSurfaceSupplyCancellation(order.next_order);
	REQUIRE(Command<Commands::CancelSurfaceSupply>::Do({}, order.next_order, quote.quantity).Succeeded());
	REQUIRE(Command<Commands::CancelSurfaceSupply>::Do(DoCommandFlag::Execute, order.next_order, quote.quantity).Succeeded());
	CHECK(rig.carrier->cargo.TotalCount() == 2);
	// Native station totals also account for the two pickup reservations now
	// physically owned by the carrier; cancellation removes only available source stock.
	CHECK(rig.Queue(label).AvailableCount() == 0);
	CHECK(rig.Queue(label).TotalCount() == 2);
	CHECK(GetSurfaceSupplyOrder(order.next_order)->refunded == 3);
	CHECK(GetSurfaceSupplyOrder(order.next_order)->dispatched == 2);
	CHECK(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
}

/** Actual source destruction applies native stock loss, keeps carried stock/history and cannot attach an old refund to a reused ID. */
TEST_CASE("Native source station loss retires refunds without losing dispatched purpose", "[surface-supply-station-loss]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const CargoLabel label{"RFMT"};
	const auto order = PrepareSurfaceSupplyOrder(rig.origin->index, label, 5);
	REQUIRE(PlaceSurfaceSupplyOrder(order, true) == SurfaceSupplyError::None);
	auto &relay = rig.second->goods[order.cargo].GetOrCreateData().cargo;
	REQUIRE(HubCargoTransfer::Move(rig.Queue(label), relay, 2));
	const Money cash = Company::Get(_current_company)->money;
	const StationID former = rig.origin->index;
	delete rig.origin; rig.origin = nullptr;
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(relay.TotalCount() == 2);
	CHECK(GetSurfaceSupplyOrder(order.next_order)->station == StationID::Invalid());
	CHECK(GetSurfaceSupplyOrder(order.next_order)->closed);
	CHECK(GetSurfaceSupplyOrder(order.next_order)->refunded == 0);
	CHECK(GetSurfaceSupplyOrder(order.next_order)->dispatched == 2);
	CHECK(PrepareSurfaceSupplyCancellation(order.next_order).error == SurfaceSupplyError::Stale);
	Station *replacement = rig.StationAt(TileXYL(24, 20, MapLayer::Surface));
	CHECK(replacement->index == former);
	CHECK(PrepareSurfaceSupplyOrder(replacement->index, label, 1).day_used == 5);
	CHECK(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
}

/** The actual whole-company transition combines already-paid caps, keeping historical stock rather than rejecting a legitimate merged day. */
TEST_CASE("Native company takeover combines Surface purchase history and source ownership", "[surface-supply-takeover]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	AutoRestoreBackup local{_local_company, CompanyID::Invalid()};
	const auto old = _current_company;
	const CargoLabel label{"STEL"};
	const auto first = PrepareSurfaceSupplyOrder(rig.origin->index, label, 200);
	REQUIRE(PlaceSurfaceSupplyOrder(first, true) == SurfaceSupplyError::None);
	REQUIRE(Company::CanAllocateItem()); std::unique_ptr<Company> buyer{Company::Create()};
	buyer->money = 1000000; buyer->clear_limit = 64U << 16;
	SurfaceSupplyOrderID second{0};
	{
		AutoRestoreBackup actor{_current_company, buyer->index};
		Station *pickup = rig.StationAt(TileXYL(26, 20, MapLayer::Surface));
		const auto quote = PrepareSurfaceSupplyOrder(pickup->index, label, 100);
		REQUIRE(PlaceSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::None);
		second = quote.next_order;
	}
	const Money cash = buyer->money;
	ChangeOwnershipOfCompanyItems(old, buyer->index);
	CHECK(buyer->money == cash);
	CHECK(rig.origin->owner == buyer->index);
	CHECK(GetSurfaceSupplyOrder(first.next_order)->owner == buyer->index);
	CHECK(GetSurfaceSupplyOrder(second)->owner == buyer->index);
	CHECK(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
	{
		AutoRestoreBackup actor{_current_company, buyer->index};
		CHECK(PrepareSurfaceSupplyOrder(rig.origin->index, label, 1).error == SurfaceSupplyError::DailyCap);
		CHECK(PrepareSurfaceSupplyCancellation(first.next_order).quantity == 200);
	}
	CHECK(PrepareSurfaceSupplyCancellation(first.next_order).error == SurfaceSupplyError::Company);
	ChangeOwnershipOfCompanyItems(buyer->index, old); // Return native fixture items before the temporary actual buyer is destroyed.
}

/** Genuine company loss retires refund claims while native ownerless queues retain non-paying purpose until normal deletion. */
TEST_CASE("Native company loss retains paid history without granting a replacement buyer", "[surface-supply-company-loss]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	AutoRestoreBackup local{_local_company, CompanyID::Invalid()};
	const auto original = _current_company;
	const CargoLabel label{"RFMT"};
	const auto order = PrepareSurfaceSupplyOrder(rig.origin->index, label, 5);
	REQUIRE(PlaceSurfaceSupplyOrder(order, true) == SurfaceSupplyError::None);
	ChangeOwnershipOfCompanyItems(original, INVALID_OWNER);
	const auto *history = GetSurfaceSupplyOrder(order.next_order);
	REQUIRE(history != nullptr);
	CHECK(history->owner == CompanyID::Invalid()); CHECK(history->station == StationID::Invalid());
	CHECK(history->closed); CHECK(history->quantity == 5); CHECK(history->refunded == 0);
	// Native bankruptcy supplies a temporary clearing balance to remove company
	// infrastructure. Retained paid/refund history, rather than that native wallet,
	// proves no implicit supply credit or replacement cancellation entitlement.
	CHECK(rig.Queue(label).AvailableCount() == 5);
	CHECK(PrepareSurfaceSupplyCancellation(order.next_order).error == SurfaceSupplyError::Stale);
	CHECK(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
	REQUIRE(Company::CanAllocateItem()); std::unique_ptr<Company> holder{Company::Create()};
	holder->money = 1000000;
	AutoRestoreBackup actor{_current_company, holder->index};
	rig.destination = rig.StationAt(TileXYL(29, 31, MapLayer::Surface));
	AutoRestoreBackup<Money> tariff{CargoSpec::Get(order.cargo)->current_payment, Money{1000000}};
	const Money before = holder->money;
	rig.Load(label, 5); // Explicit boundary pickup/arrival; this does not claim a driven bankrupt-stock service.
	rig.carrier->cargo.Stage(true, rig.destination->index, {}, OrderUnloadType::UnloadIfPossible,
		&rig.destination->goods[order.cargo], order.cargo, rig.carrier->cargo_payment, rig.destination->xy);
	REQUIRE(rig.carrier->cargo.Unload(5, &rig.destination->goods[order.cargo].GetOrCreateData().cargo,
		order.cargo, rig.carrier->cargo_payment, rig.destination->xy) == 5);
	rig.SettlePayment();
	CHECK(holder->money == before); CHECK(rig.Input(label).waiting == 5);
	CHECK(GetSurfaceSupplyOrder(order.next_order)->dispatched == 5);
	CHECK(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
}
