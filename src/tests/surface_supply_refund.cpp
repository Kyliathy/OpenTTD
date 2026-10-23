/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_refund.cpp Frozen legacy refunds through actual canonical commands and physical owners.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md single physical stock ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group15; original files remain later acceptance.
 * Receipts/initial stock are explicit original components, never a test-only live installer or a paid-world predecessor.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "surface_supply_legacy_fixture.h"
#include "native_cargo_custody_fixture.h"
#include "../surface_supply_cmd.h"
#include "../command_func.h"
#include "../hub_cargo.h"
#include "../misc/endian_buffer.hpp"
#include "../safeguards.h"

/** Execute only through the real command finance kernel.
 * @param order Exact original legacy identity, never a row ordinal.
 * @param expected Complete eligible units seen by the original requester.
 * @param estimate Quote the frozen Other expense without stock, history or wallet mutation.
 * @return Original synchronized command result; no helper directly changes a wallet.
 */
static CommandCost CancelPaidLegacySupply(SurfaceSupplyOrderID order, uint16_t expected, bool estimate = false)
{
	return Command<Commands::CancelLegacySurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, INVALID_TILE,
		std::make_tuple(order, expected));
}

/** Only the new append211/uint64-plus-count ABI is admitted; original A command values and tuples remain intact. */
TEST_CASE("Checked legacy refund appends exact canonical command identity and tuple", "[surface-supply-refund]")
{
	static_assert(to_underlying(Commands::PlaceSurfaceSupplyOrder) == 209);
	static_assert(to_underlying(Commands::CancelSurfaceSupplyOrder) == 210);
	static_assert(to_underlying(Commands::CancelLegacySurfaceSupplyOrder) == 211);
	using Args = CommandTraits<Commands::CancelLegacySurfaceSupplyOrder>::Args;
	static_assert(std::is_same_v<Args, std::tuple<uint64_t, uint16_t>>);
	const Args original{UINT64_C(0x0102030405060708), uint16_t{0x090A}};
	const auto bytes = EndianBufferWriter<CommandDataBuffer>::FromValue(original);
	const std::array<uint8_t, 10> expected{8, 7, 6, 5, 4, 3, 2, 1, 10, 9};
	CHECK(std::ranges::equal(bytes, expected)); CHECK(EndianBufferReader::ToValue<Args>(bytes) == original);
}

/** Original25-unit tariff, not current native price/inflation, determines the one actual Other refund. */
TEST_CASE("Legacy refund removes exact source stock and settles frozen Other expense once", "[surface-supply-refund]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	auto &queue = Station::Get(rig.station)->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo;
	queue.Append(legacy.Packet(40), StationID::Invalid()); legacy.Restore(legacy.Candidate());
	AutoRestoreBackup facilities(Station::Get(rig.station)->facilities);
	if (GENERATE(false, true)) Station::Get(rig.station)->facilities = StationFacility::BusStop;
	AutoRestoreBackup inflation(_economy.inflation_prices, uint64_t{7} << 16);
	const auto cash = Company::Get(_current_company)->money; const auto expense = Company::Get(_current_company)->yearly_expenses[0];
	const auto running = Company::Get(_current_company)->cur_economy.expenses; const auto book = CaptureSurfaceSupplyOrders();
	const auto packets = LegacyComponentPacketFacts(); const auto random = _random;
	const auto quote = PrepareLegacySurfaceSupplyCancellation(1); REQUIRE(quote.has_value()); CHECK(quote->quantity == 40); CHECK(quote->refund == 1000);
	const auto estimate = CancelPaidLegacySupply(1, 40, true); REQUIRE(estimate.Succeeded()); CHECK(estimate.GetCost() == -1000); CHECK(estimate.GetExpensesType() == ExpensesType::Other);
	CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(LegacyComponentPacketFacts() == packets); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(Company::Get(_current_company)->yearly_expenses[0] == expense);
	CHECK(CmdCancelSurfaceSupplyOrder(DoCommandFlag::Execute, 1).Failed()); // The old native payload cannot bypass the exact legacy count.
	REQUIRE(CancelPaidLegacySupply(1, 40).Succeeded()); CHECK(queue.TotalCount() == 0); CHECK(queue.CountUnshippedCompanySupply(1) == 0);
	CHECK(GetSurfaceSupplyOrder(1)->Legacy()->refunded == 40); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->closed); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->dispatched == 0);
	CHECK(Company::Get(_current_company)->money == cash + 1000); CHECK(Company::Get(_current_company)->yearly_expenses[0][ExpensesType::Other] == expense[ExpensesType::Other] - 1000);
	CHECK(Company::Get(_current_company)->yearly_expenses[0][ExpensesType::Property] == expense[ExpensesType::Property]); CHECK(Company::Get(_current_company)->cur_economy.expenses == running);
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 100); CHECK(ValidateSurfaceSupplyOrderBook());
	const auto settled = CaptureSurfaceSupplyOrders(); CHECK_FALSE(PrepareLegacySurfaceSupplyCancellation(1)); CHECK(CancelPaidLegacySupply(1, 0).Failed()); CHECK(CancelPaidLegacySupply(1, 40).Failed());
	CHECK(CaptureSurfaceSupplyOrders() == settled); CHECK(Company::Get(_current_company)->money == cash + 1000);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Actual reservation is irrevocable departure; an old100-unit request cannot silently refund the60-unit remainder. */
TEST_CASE("Legacy refund refuses stale counts and preserves reserved and returned stock", "[surface-supply-refund]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeCargoCustodyRig rig; LegacySupplyComponent legacy(rig.station);
	auto *station = Station::Get(rig.station); auto &queue = station->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo;
	rig.Own(queue, legacy.Packet()); legacy.Restore(legacy.Candidate()); const auto original_quote = PrepareLegacySurfaceSupplyCancellation(1); REQUIRE(original_quote.has_value());
	REQUIRE(queue.Reserve(40, &rig.train->cargo, {}, station->xy) == 40); CHECK(queue.ReservedCount() == 40); CHECK(queue.AvailableCount() == 60);
	const auto packets = LegacyComponentPacketFacts(); const auto book = CaptureSurfaceSupplyOrders(); const auto cash = Company::Get(_current_company)->money;
	CHECK(CancelPaidLegacySupply(1, original_quote->quantity, true).Failed()); CHECK(CancelPaidLegacySupply(1, original_quote->quantity).Failed());
	CHECK(LegacyComponentPacketFacts() == packets); CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(Company::Get(_current_company)->money == cash);
	const bool return_before_refund = GENERATE(false, true);
	if (return_before_refund) REQUIRE(rig.train->cargo.Return(40, &queue, StationID::Invalid(), station->xy) == 40);
	const auto remaining = PrepareLegacySurfaceSupplyCancellation(1); REQUIRE(remaining.has_value()); CHECK(remaining->quantity == 60); CHECK(remaining->refund == 1500);
	REQUIRE(CancelPaidLegacySupply(1, 60).Succeeded()); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->refunded == 60); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->dispatched == 40);
	CHECK(queue.AvailableCount() == (return_before_refund ? 40 : 0)); CHECK(queue.ReservedCount() == (return_before_refund ? 0 : 40));
	CHECK(rig.train->cargo.TotalCount() == (return_before_refund ? 0 : 40)); CHECK(Company::Get(_current_company)->money == cash + 1500); CHECK(ValidateSurfaceSupplyOrderBook());
	if (!return_before_refund) REQUIRE(rig.train->cargo.Return(40, &queue, StationID::Invalid(), station->xy) == 40);
	CHECK(queue.TotalCount() == 40); CHECK(queue.CountUnshippedCompanySupply(1) == 0); CHECK(CancelPaidLegacySupply(1, 40).Failed());
	// Every surviving unit already departed; a source return cannot make it
	// payable freight or revive an entitlement that the exact command closed.
	for (const auto &facts : LegacyComponentPacketFacts()) { CHECK(facts.source.IsCompanySupply()); CHECK(facts.shipped); CHECK(facts.feeder_share == 0); }
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 100); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** No-stock closure is legitimate history, but cannot allocate goods data, create a payment or consume unrelated A material. */
TEST_CASE("Legacy zero-eligible closure grants no money or replacement source stock", "[surface-supply-refund]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	auto &goods = Station::Get(rig.station)->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})]; const bool unlinked_stock = GENERATE(false, true);
	if (unlinked_stock) goods.GetOrCreateData().cargo.Append(legacy.Packet(3, false, 0), StationID::Invalid());
	legacy.Restore(legacy.Candidate(100, 100)); const auto cash = Company::Get(_current_company)->money; const auto packets = LegacyComponentPacketFacts();
	const auto quote = PrepareLegacySurfaceSupplyCancellation(1); REQUIRE(quote.has_value()); CHECK(quote->quantity == 0); CHECK(quote->refund == 0);
	CHECK(CancelPaidLegacySupply(1, 1).Failed()); REQUIRE(CancelPaidLegacySupply(1, 0).Succeeded()); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->closed);
	CHECK(GetSurfaceSupplyOrder(1)->Legacy()->refunded == 0); CHECK(goods.HasData() == unlinked_stock); CHECK(LegacyComponentPacketFacts() == packets);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 100); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Overflow refusal precedes physical removal; negative/debt wallets and an exact INT64_MAX result remain representable. */
TEST_CASE("Legacy refund bounds its real wallet before removing any packet", "[surface-supply-refund]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	auto &queue = Station::Get(rig.station)->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo;
	queue.Append(legacy.Packet(4), StationID::Invalid()); legacy.Restore(legacy.Candidate(4));
	const int64_t initial = GENERATE(INT64_MAX - 99, INT64_MAX - 100, int64_t{-100}, INT64_MIN);
	AutoRestoreBackup cash(Company::Get(_current_company)->money, Money{initial}); const auto packets = LegacyComponentPacketFacts(); const auto book = CaptureSurfaceSupplyOrders();
	if (initial == INT64_MAX - 99) {
		CHECK_FALSE(PrepareLegacySurfaceSupplyCancellation(1)); CHECK(CancelPaidLegacySupply(1, 4, true).Failed()); CHECK(CancelPaidLegacySupply(1, 4).Failed());
		CHECK(Company::Get(_current_company)->money == Money{initial}); CHECK(LegacyComponentPacketFacts() == packets); CHECK(CaptureSurfaceSupplyOrders() == book);
	} else {
		REQUIRE(CancelPaidLegacySupply(1, 4).Succeeded()); CHECK(Company::Get(_current_company)->money == Money{initial + 100}); CHECK(queue.TotalCount() == 0);
	}
}

/** Current native references and the shared full custody join prevent source corruption from becoming a real refund. */
TEST_CASE("Legacy cancellation refuses wrong sources and excess actual material unchanged", "[surface-supply-refund]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	auto *source = Station::Get(rig.station); auto &queue = source->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo;
	queue.Append(legacy.Packet(), StationID::Invalid()); legacy.Restore(legacy.Candidate()); const auto fault = GENERATE(0, 1, 2, 3);
	const auto buyer = _current_company;
	AutoRestoreBackup owner(source->owner); AutoRestoreBackup position(source->xy); AutoRestoreBackup actor(_current_company);
	if (fault == 0) source->owner = OWNER_NONE;
	if (fault == 1) source->xy = TileXYL(20, 20, MapLayer::Underground);
	if (fault == 2) _current_company = CompanyID::Invalid();
	if (fault == 3) queue.Append(legacy.Packet(1), StationID::Invalid());
	const auto cash = Company::Get(buyer)->money; const auto packets = LegacyComponentPacketFacts(); const auto book = CaptureSurfaceSupplyOrders();
	// An invalid actor is not a valid finance-kernel caller; exercise both
	// actual command-body phases and require refusal before any settlement.
	CHECK_FALSE(PrepareLegacySurfaceSupplyCancellation(1)); CHECK(CmdCancelLegacySurfaceSupplyOrder({}, 1, 100).Failed());
	CHECK(CmdCancelLegacySurfaceSupplyOrder(DoCommandFlag::Execute, 1, 100).Failed());
	CHECK(LegacyComponentPacketFacts() == packets); CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(Company::Get(buyer)->money == cash);
}

/** An explicitly reconstructed transferred entitlement belongs to its current owner, never an invented original payer. */
TEST_CASE("Restored legacy takeover history refunds only its actual current owner", "[surface-supply-refund]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent legacy(rig.station);
	auto *source = Station::Get(rig.station); const auto original_owner = _current_company; const auto original_cash = Company::Get(original_owner)->money;
	auto &queue = source->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo; queue.Append(legacy.Packet(4), StationID::Invalid());
	auto candidate = legacy.Candidate(4); REQUIRE(Company::CanAllocateItem()); auto *successor = Company::Create(); successor->money = 1000;
	{
		AutoRestoreBackup owner(source->owner, Owner{successor->index}); candidate.records[0].owner = successor->index; legacy.Restore(candidate);
		CHECK(CancelPaidLegacySupply(1, 4).Failed());
		{
			AutoRestoreBackup actor(_current_company, successor->index); REQUIRE(CancelPaidLegacySupply(1, 4).Succeeded()); CHECK(successor->money == 1100);
			CHECK(GetSurfaceSupplyOrderDayUsage(successor->index, CargoLabel{"RFMT"}) == 4); CHECK(ValidateSurfaceSupplyOrderBook());
		}
		CHECK(Company::Get(original_owner)->money == original_cash); CHECK(queue.TotalCount() == 0);
		ResetSurfaceSupplyOrders(); // End declared restored-component history before retiring its temporary company.
	}
	delete successor;
}
