/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_refund_stock.cpp Exact native source selection, stale refusal and cache conservation.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md actual cargo ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-c-supply-receipts.md finite legacy cancellation.
 * Declared linked stock tests selection only; canonical receipt admission and financial settlement remain separate command gates.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "surface_supply_fixture.h"
#include "../surface_supply_order_runtime.h"
#include "../hub_cargo.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/** Declare one native queue fragment, without granting its link live receipt or refund authority.
 * @param stock Actual component native queue.
 * @param next Original routing bucket.
 * @param quantity Declared whole units.
 * @param order Optional exact original legacy identity; zero means unlinked stock.
 * @param shipped Original departure evidence, not a new accounting event.
 * @param source Permanent source purpose; ordinary freight remains a positive exclusion control.
 */
static void RefundStockPacket(StationCargoList &stock, StationID next, uint16_t quantity, uint64_t order = 0, bool shipped = false,
	Source source = Source{CompanyID{0}, SourceType::CompanySupply})
{
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *packet = CargoPacket::Create(StationID{3}, quantity, source);
	if (order != 0) REQUIRE(packet->RestoreCompanySupplyOrder(order, shipped));
	stock.Append(packet, next);
}

/** Capture exact actual routing and packet evidence for unchanged-refusal comparisons.
 * @param stock Actual queue to observe without allocating gameplay packets or changing caches.
 * @return Original routing, identities, quantities, ages, links, flags and purpose in native traversal order.
 */
static auto RefundStockEvidence(const StationCargoList &stock)
{
	std::vector<std::tuple<StationID, CargoPacketID, uint16_t, uint16_t, uint64_t, bool, Source>> rows;
	// Preserve each real fragment and its routing owner, rather than comparing
	// only a total that could hide an unauthorized split, merge or relabelling.
	for (StationCargoList::ConstIterator it(stock.Packets()->begin()); it != stock.Packets()->end(); ++it) {
		const auto *packet = *it;
		rows.emplace_back(it.GetKey(), packet->index, packet->Count(), packet->GetPeriodsInTransit(),
			packet->GetCompanySupplyOrder(), packet->HasCompanySupplyShipped(), packet->GetSource());
	}
	return rows;
}

/** Independent routing, age and purpose facts prove exact removal without changing unrelated stock, cash or history. */
TEST_CASE("Legacy refund selection removes only exact unshipped source fragments and updates native age caches", "[surface-supply-refund-stock]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig;
	const uint64_t order = (uint64_t{1} << 40) + 7;
	StationCargoList source{};
	RefundStockPacket(source, StationID{3}, 6, order);
	// Give only the first eligible fragment a distinct declared age. Correct
	// removal must retire its weighted cache contribution, not just its quantity.
	for (int age = 0; age < 10; ++age) HubCargoTransfer::Age(source);
	RefundStockPacket(source, StationID{9}, 4, order, false, Source{Source::Invalid, SourceType::CompanySupply}); // Unknown/retired buyer never erases permanent purpose or the exact legacy link.
	RefundStockPacket(source, StationID{3}, 5, order, true);
	RefundStockPacket(source, StationID{9}, 3, order + 1);
	RefundStockPacket(source, StationID::Invalid(), 7);
	RefundStockPacket(source, StationID::Invalid(), 8, 0, false, Source{CompanyID{0}, SourceType::Industry});
	REQUIRE(source.AvailableCount() == 33); REQUIRE(source.PeriodsInTransit() == 1);
	CHECK(source.CountUnshippedCompanySupply(order) == 10); CHECK(source.CountUnshippedCompanySupply(order + 1) == 3);
	CHECK(source.CountUnshippedCompanySupply(0) == 0);
	const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
	const auto cash = Company::Get(_current_company)->money; const auto book = CaptureSurfaceSupplyOrders();
	REQUIRE(source.RemoveUnshippedCompanySupply(order, 10));
	CHECK(source.AvailableCount() == 23); CHECK(source.TotalCount() == 23); CHECK(source.ReservedCount() == 0);
	CHECK(source.PeriodsInTransit() == 0); CHECK(CargoPacket::GetNumItems() == packets - 2);
	CHECK(source.CountUnshippedCompanySupply(order) == 0); CHECK(source.CountUnshippedCompanySupply(order + 1) == 3);
	const auto remaining = RefundStockEvidence(source); REQUIRE(remaining.size() == 4);
	CHECK(std::get<2>(remaining[0]) == 5); CHECK(std::get<4>(remaining[0]) == order); CHECK(std::get<5>(remaining[0]));
	CHECK(std::get<2>(remaining[1]) == 3); CHECK(std::get<4>(remaining[1]) == order + 1);
	CHECK(std::get<2>(remaining[2]) == 7); CHECK(std::get<6>(remaining[2]).IsCompanySupply());
	CHECK(std::get<2>(remaining[3]) == 8); CHECK_FALSE(std::get<6>(remaining[3]).IsCompanySupply());
	CHECK(Company::Get(_current_company)->money == cash); CHECK(CaptureSurfaceSupplyOrders() == book);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** A stale expected quantity or zero identity cannot partially remove matching native source stock. */
TEST_CASE("Legacy refund removal refuses changed exact counts and zero identities without touching fragments", "[surface-supply-refund-stock]")
{
	StationCargoList source{};
	RefundStockPacket(source, StationID{3}, 6, 7); RefundStockPacket(source, StationID{9}, 4, 7);
	RefundStockPacket(source, StationID{3}, 5, 7, true); RefundStockPacket(source, StationID::Invalid(), 8);
	const auto fault = GENERATE(0, 1, 2, 3);
	const uint64_t order = fault == 3 ? 0 : 7;
	const uint16_t expected = fault == 0 ? 9 : fault == 1 ? 11 : 0;
	const auto before = RefundStockEvidence(source); const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
	CHECK_FALSE(source.RemoveUnshippedCompanySupply(order, expected));
	CHECK(RefundStockEvidence(source) == before); CHECK(source.TotalCount() == 23); CHECK(source.CountUnshippedCompanySupply(7) == 10);
	CHECK(CargoPacket::GetNumItems() == packets); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Zero-eligible selection never consumes returned/carried stock or closes/refunds an order by itself. */
TEST_CASE("Legacy zero-eligible selection leaves returned and other-owner packets untouched", "[surface-supply-refund-stock]")
{
	StationCargoList source{}; VehicleCargoList carrier{};
	RefundStockPacket(source, StationID::Invalid(), 5, 7, true);
	RefundStockPacket(source, StationID::Invalid(), 4);
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *carried = CargoPacket::Create(StationID{3}, uint16_t{6}, Source{CompanyID{0}, SourceType::CompanySupply});
	REQUIRE(carried->RestoreCompanySupplyOrder(7, true)); carrier.Append(carried);
	const auto before = RefundStockEvidence(source); const auto carried_id = carried->index;
	const auto packets = CargoPacket::GetNumItems(); const auto book = CaptureSurfaceSupplyOrders(); const auto random = _random;
	CHECK(source.CountUnshippedCompanySupply(7) == 0); REQUIRE(source.RemoveUnshippedCompanySupply(7, 0));
	CHECK_FALSE(source.RemoveUnshippedCompanySupply(7, 1)); CHECK_FALSE(source.RemoveUnshippedCompanySupply(0, 0));
	CHECK(RefundStockEvidence(source) == before); CHECK((*carrier.Packets()->begin())->index == carried_id);
	CHECK(carried->Count() == 6); CHECK(carried->GetCompanySupplyOrder() == 7); CHECK(carried->HasCompanySupplyShipped());
	CHECK(source.TotalCount() == 9); CHECK(carrier.TotalCount() == 6); CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}
