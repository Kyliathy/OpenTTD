/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order_owner.cpp Actual native owner restoration, paid command settlement and independent copied-reference boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups13CCC/13CCD.
 * Staged paid-looking records are declared restore components, never purchases or usable stock/payment evidence.
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 16 (no odr-use of CargoPacket::MAX_COUNT in Debug links)
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "surface_supply_fixture.h"
#include "../surface_supply_order_runtime.h"
#include "../surface_supply_cmd.h"
#include "../command_func.h"
#include "../timer/timer_game_tick.h"
#include "../fios.h"
#include "../saveload/saveload.h"
#include "../saveload/surface_supply_order_preview.h"
#include "../safeguards.h"

extern SaveLoadVersion _sl_version; ///< Native framework candidate era, explicitly scoped/restored for direct copied-validator component checks.

/** Run the original production station tick entry point; this observer introduces no substitute dispatch or cargo owner. */
extern void OnTick_Station();

/** Real command-created native receiving station; staged records/cash/world are explicit restoration fixtures without procurement provenance. */
class SurfaceSupplyOwnerRig : public SurfaceSupplyAdmissionRig {
public:
	/** @param serialized_native_slots Match distributed actual labels for later complete native-file component exports. */
	explicit SurfaceSupplyOwnerRig(bool serialized_native_slots = false) : SurfaceSupplyAdmissionRig(serialized_native_slots) { ResetSurfaceSupplyOrders(); }
	/** Retire only this component's metadata before its original native station/company disappear. */
	~SurfaceSupplyOwnerRig() { ResetSurfaceSupplyOrders(); }

	/** Declare one whole pending metadata receipt using the actual station/company and canonical quote, without paying or allocating an order/packet.
	 * @param quantity Entire declared request.
	 * @return Component-only original metadata and native economy-day stamp.
	 */
	SurfaceSupplyOrderTransport Pending(uint16_t quantity = 2) const
	{
		const SurfaceSupplyOrderRequest request{this->station, _current_company, CargoLabel{"STEL"}, quantity};
		const auto view = CaptureSurfaceSupplyOrderView(request, 0); const auto quote = PrepareSurfaceSupplyOrder(request, view);
		REQUIRE(quote.error == SurfaceSupplyOrderError::None);
		SurfaceSupplyOrderRecord record; record.id = 1; record.Native()->purchaser = record.owner = _current_company; record.origin = this->station;
		const auto *origin = Station::Get(this->station); record.Native()->origin_x = static_cast<uint16_t>(TileX(origin->xy)); record.Native()->origin_y = static_cast<uint16_t>(TilePlaneY(origin->xy));
		record.label = request.label; record.quantity = quantity; record.Native()->cost = quote.cost; record.Native()->inflation_prices = view.inflation_prices;
		record.Native()->purchase_year = view.year; record.purchase_day = TimerGameEconomy::date; record.Native()->packet_buyer = _current_company.base();
		return {{2, {record}}, TimerGameEconomy::date};
	}

	/** Supply explicit independent original full DATE/MAPS declarations to the restore boundary; these calls are not a native file/procurement proof. */
	void ObserveWorld() const
	{
		REQUIRE(ObserveLoadedSurfaceSupplyEconomyDate(TimerGameEconomy::date));
		REQUIRE(ObserveLoadedSurfaceSupplyMap(Map::SizeX(), Map::SizeY(), static_cast<uint8_t>(Map::LayerCount())));
	}

	/** @param transport Declared complete original metadata, never a newly purchased right. */
	void Restore(const SurfaceSupplyOrderTransport &transport) const
	{
		REQUIRE(StageLoadedSurfaceSupplyOrders(transport.snapshot, transport.saved_day)); this->ObserveWorld();
		REQUIRE(AcceptLoadedSurfaceSupplyOrders(true));
	}
};

/** Execute through the original native command finance kernel, including its independent estimate/execute recapture.
 * @param station Actual receiving Surface freight facility.
 * @param label Big-endian canonical four-byte cargo label.
 * @param quantity Entire paid quantity.
 * @param intent Saved monotonic purchase identity.
 * @param estimate Quote without publishing a paid right or expense.
 * @return Complete original financial command result.
 */
static CommandCost PlacePaidSurfaceOrder(StationID station, uint32_t label, uint16_t quantity, SurfaceSupplyOrderID intent, bool estimate = false)
{
	return Command<Commands::PlaceSurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, INVALID_TILE,
		std::make_tuple(station, label, quantity, intent));
}

/** Cancel through the original financial command kernel; direct command bodies never independently settle money.
 * @param intent Exact original unshipped identity.
 * @param estimate Quote the frozen refund without releasing the right.
 * @return Original once-only refund or unchanged refusal.
 */
static CommandCost CancelPaidSurfaceOrder(SurfaceSupplyOrderID intent, bool estimate = false)
{
	return Command<Commands::CancelSurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, INVALID_TILE, std::make_tuple(intent));
}

/** Actual station ticks must fulfill each paid whole purchase once, including native packet merging, fractional carry, ratings and source purpose. */
TEST_CASE("Paid Surface pending orders enter original station custody once without another expense", "[surface-supply-dispatch]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	const auto label = GENERATE(uint32_t{0x5354454C}, uint32_t{0x52464D54}, uint32_t{0x454C4543}, uint32_t{0x42415459});
	AutoRestoreBackup counter(TimerGameTick::counter, TimerGameTick::TickCounter{1});
	REQUIRE(PlacePaidSurfaceOrder(rig.station, label, 2, 1).Succeeded()); REQUIRE(PlacePaidSurfaceOrder(rig.station, label, 3, 2).Succeeded());
	const auto cargo = GetCargoTypeByLabel(GetSurfaceSupplyOrder(1)->label); auto &goods = Station::Get(rig.station)->goods[cargo]; goods.amount_fract = 173;
	const auto paid = Company::Get(_current_company)->yearly_expenses[0][ExpensesType::Property];
	CHECK(goods.TotalCount() == 0); OnTick_Station(); CHECK(goods.TotalCount() == 0); CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Pending);
	// Fulfillment is the already-paid delivery, independent of later liquid funds or inflation; original native arrival owns all physical units and fractional carry.
	AutoRestoreBackup funds(Company::Get(_current_company)->money, Money{0}); AutoRestoreBackup inflation(_economy.inflation_prices, uint64_t{4} << 16);
	TimerGameTick::counter = 0; OnTick_Station(); REQUIRE(goods.HasData()); CHECK(goods.TotalCount() == 5); CHECK(goods.amount_fract == 173); CHECK(goods.HasRating());
	CHECK(goods.link_graph != LinkGraphID::Invalid()); CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Fulfilled);
	CHECK(GetSurfaceSupplyOrder(2)->Native()->state == SurfaceSupplyOrderState::Fulfilled); CHECK(GetSurfaceSupplyOrder(1)->Native()->issued_packet != CargoPacketID::Invalid());
	CHECK(GetSurfaceSupplyOrder(2)->Native()->issued_packet != CargoPacketID::Invalid());
	uint32_t units = 0; const auto *packets = goods.GetData().cargo.Packets();
	// Inspect original flattened native custody after Append may merge the issued packets; historical IDs never become a second stock or refund authority.
	for (StationCargoList::ConstIterator it(packets->begin()); it != packets->end(); ++it) {
		CHECK((*it)->GetSource() == Source{_current_company.base(), SourceType::CompanySupply}); CHECK((*it)->GetFeederShare() == 0);
		CHECK((*it)->GetFirstStation() == rig.station); units += (*it)->Count();
	}
	CHECK(units == 5); CHECK(CancelPaidSurfaceOrder(1).Failed()); CHECK(CancelPaidSurfaceOrder(2).Failed());
	const auto retry = PlacePaidSurfaceOrder(rig.station, label, 2, 1); REQUIRE(retry.Succeeded()); CHECK(retry.GetCost() == 0);
	OnTick_Station(); CHECK(goods.TotalCount() == 5); CHECK(Company::Get(_current_company)->money == 0);
	CHECK(Company::Get(_current_company)->yearly_expenses[0][ExpensesType::Property] == paid); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Actual full native storage refuses ingress atomically, then admits the same paid order without a second charge when space returns. */
TEST_CASE("A full actual native receiver retains the whole paid pending order until room exists", "[surface-supply-dispatch]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	AutoRestoreBackup counter(TimerGameTick::counter, TimerGameTick::TickCounter{0});
	REQUIRE(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 1).Succeeded());
	auto &queue = Station::Get(rig.station)->goods[GetCargoTypeByLabel(CargoLabel{"STEL"})].GetOrCreateData().cargo;
	const auto cash = Company::Get(_current_company)->money; const auto expense = Company::Get(_current_company)->yearly_expenses[0][ExpensesType::Property];
	constexpr uint32_t packet_count = UINT32_MAX / CargoPacket::MAX_COUNT;
	REQUIRE(CargoPacket::CanAllocateItem(packet_count));
	// This explicit capacity component fills the actual native cache through
	// original packets, not a forged count or replacement receiver callback.
	// Spread opaque routing buckets to avoid quadratic merge scans; these
	// routes confer no operational route/station or saved-world evidence.
	for (uint32_t index = 0; index < packet_count; ++index) {
		// Pass a copy: Create() forwards by reference, which odr-uses the in-class constant and fails to link without optimisation.
		auto *packet = CargoPacket::Create(rig.station, uint16_t{CargoPacket::MAX_COUNT}, Source{_current_company.base(), SourceType::CompanySupply});
		queue.Append(packet, StationID{static_cast<uint16_t>(index % 512)});
	}
	REQUIRE(queue.TotalCount() == UINT32_MAX); const auto packets = CargoPacket::GetNumItems();
	OnTick_Station(); CHECK(queue.TotalCount() == UINT32_MAX); CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Pending); CHECK(GetSurfaceSupplyOrder(1)->Native()->issued_packet == CargoPacketID::Invalid());
	CHECK(Company::Get(_current_company)->money == cash); CHECK(Company::Get(_current_company)->yearly_expenses[0][ExpensesType::Property] == expense);
	CHECK(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 1).Succeeded()); CHECK(Company::Get(_current_company)->money == cash);
	REQUIRE(queue.Truncate(UINT32_MAX - 5) == UINT32_MAX - 5); CHECK(queue.TotalCount() == 5);
	OnTick_Station(); CHECK(queue.TotalCount() == 7); CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Fulfilled);
	CHECK(CancelPaidSurfaceOrder(1).Failed()); OnTick_Station(); CHECK(queue.TotalCount() == 7);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(Company::Get(_current_company)->yearly_expenses[0][ExpensesType::Property] == expense);
	CHECK(ValidateSurfaceSupplyOrderBook());
}

/** A changed real receiver or editor policy cannot turn pending metadata into usable physical stock; repaired actual admission resumes the original delivery only once. */
TEST_CASE("Paid Surface dispatch refuses changed station owners layers facilities and editor ticks", "[surface-supply-dispatch]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	AutoRestoreBackup counter(TimerGameTick::counter, TimerGameTick::TickCounter{0});
	REQUIRE(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 1).Succeeded());
	auto *station = Station::Get(rig.station); const auto cargo = GetCargoTypeByLabel(CargoLabel{"STEL"}); const auto cash = Company::Get(_current_company)->money;
	const auto fault = GENERATE(0, 1, 2, 3);
	{
		AutoRestoreBackup mode(_game_mode); AutoRestoreBackup owner(station->owner); AutoRestoreBackup position(station->xy); AutoRestoreBackup facilities(station->facilities);
		if (fault == 0) _game_mode = GameMode::Editor;
		if (fault == 1) station->owner = OWNER_NONE;
		if (fault == 2) station->xy = TileXYL(20, 20, MapLayer::Underground);
		if (fault == 3) station->facilities = StationFacility::BusStop;
		OnTick_Station(); CHECK(station->goods[cargo].TotalCount() == 0); CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Pending);
	}
	OnTick_Station(); CHECK(station->goods[cargo].TotalCount() == 2); CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Fulfilled);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Last-facility removal must release pending paid rights immediately while the native station sign remains alive; deleting shipped stock can never refund it. */
TEST_CASE("Native rail facility loss refunds only unshipped Surface orders once", "[surface-supply-lifecycle]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	AutoRestoreBackup counter(TimerGameTick::counter, TimerGameTick::TickCounter{0});
	const bool fulfilled = GENERATE(false, true); const auto tile = Station::Get(rig.station)->xy;
	REQUIRE(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 1).Succeeded());
	if (fulfilled) OnTick_Station();
	const auto paid_cash = Company::Get(_current_company)->money;
	const auto removed = Command<Commands::RemoveFromRailStation>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile, tile, false));
	REQUIRE(removed.Succeeded()); REQUIRE(Station::IsValidID(rig.station)); CHECK_FALSE(Station::Get(rig.station)->facilities.Test(StationFacility::Train));
	CHECK(GetSurfaceSupplyOrder(1)->Native()->state == (fulfilled ? SurfaceSupplyOrderState::Fulfilled : SurfaceSupplyOrderState::Cancelled));
	CHECK(Company::Get(_current_company)->money == paid_cash - removed.GetCost() + (fulfilled ? 0 : 200));
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"STEL"}) == (fulfilled ? 2 : 0));
	const auto settled = Company::Get(_current_company)->money; ReconcileSurfaceSupplyStation(rig.station); CHECK(CancelPaidSurfaceOrder(1).Failed());
	delete Station::Get(rig.station); CHECK(Company::Get(_current_company)->money == settled);
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	CHECK(Company::Get(_current_company)->money == settled); CHECK(CancelPaidSurfaceOrder(1).Failed()); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Physical anchor movement is an actual native owner event; a cosmetic repaint cannot move an old saved receiving reservation. */
TEST_CASE("Native station anchor changes cancel pending Surface receipts without repaint side effects", "[surface-supply-lifecycle]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	const auto tile = TileXYL(21, 20, MapLayer::Surface);
	const auto extension = CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false);
	INFO("Native anchor fixture extension error=" << extension.GetErrorMessage().base()); REQUIRE(extension.Succeeded());
	const auto cash = Company::Get(_current_company)->money; REQUIRE(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 1).Succeeded());
	Station::Get(rig.station)->UpdateVirtCoord(); CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Pending); CHECK(Company::Get(_current_company)->money == cash - 200);
	Station::Get(rig.station)->MoveSign(tile); CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Cancelled);
	CHECK(Company::Get(_current_company)->money == cash); Station::Get(rig.station)->UpdateVirtCoord(); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(CancelPaidSurfaceOrder(1).Failed()); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Canonical native takeover moves original rights and shared allowance after station ownership settles, while preserving retired buyer purpose and historical payment. */
TEST_CASE("Native company takeover transfers Surface commitments and over-cap history without duplicate stock or refunds", "[surface-supply-lifecycle]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	const bool cancel_inherited = GENERATE(false, true);
	AutoRestoreBackup local(_local_company, COMPANY_SPECTATOR); AutoRestoreBackup counter(TimerGameTick::counter, TimerGameTick::TickCounter{0});
	const auto original = _current_company; const auto cargo = GetCargoTypeByLabel(CargoLabel{"STEL"});
	REQUIRE(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 200, 1).Succeeded()); OnTick_Station();
	REQUIRE(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 56, 2).Succeeded());
	REQUIRE(Company::CanAllocateItem()); auto successor = std::unique_ptr<Company>(Company::Create()); successor->money = 1000000; successor->avail_railtypes.Set(RAILTYPE_RAIL); const auto next = successor->index;
	successor->clear_limit = 64U << 16; // Same declared fixture construction allowance as the original company; normal startup settings are absent from this component world.
	StationID other;
	{
		AutoRestoreBackup actor(_current_company, next); const auto tile = TileXYL(40, 20, MapLayer::Surface);
		const auto receiver = CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false);
		INFO("Native successor fixture station error=" << receiver.GetErrorMessage().base()); REQUIRE(receiver.Succeeded()); other = GetStationIndex(tile);
		REQUIRE(PlacePaidSurfaceOrder(other, 0x5354454C, 256, 3).Succeeded());
	}
	const auto cash = successor->money; ChangeOwnershipOfCompanyItems(original, next);
	CHECK(successor->money == cash); CHECK(Station::Get(rig.station)->owner == next); CHECK(GetSurfaceSupplyOrder(2)->Native()->state == SurfaceSupplyOrderState::Pending);
	CHECK(GetSurfaceSupplyOrder(2)->owner == next); CHECK(GetSurfaceSupplyOrder(2)->Native()->purchaser == original); CHECK(GetSurfaceSupplyOrder(2)->Native()->packet_buyer == Source::Invalid);
	CHECK(GetSurfaceSupplyOrderDayUsage(next, CargoLabel{"STEL"}) == 512); CHECK(GetSurfaceSupplyOrderDayUsage(original, CargoLabel{"STEL"}) == 0);
	{
		AutoRestoreBackup actor(_current_company, next); CHECK(PlacePaidSurfaceOrder(other, 0x5354454C, 1, 4).Failed());
		if (cancel_inherited) REQUIRE(CancelPaidSurfaceOrder(2).Succeeded());
		OnTick_Station();
		CHECK(Station::Get(rig.station)->goods[cargo].TotalCount() == (cancel_inherited ? 200 : 256)); CHECK(Station::Get(other)->goods[cargo].TotalCount() == 256);
		CHECK(GetSurfaceSupplyOrder(2)->Native()->state == (cancel_inherited ? SurfaceSupplyOrderState::Cancelled : SurfaceSupplyOrderState::Fulfilled));
		CHECK(CancelPaidSurfaceOrder(2).Failed()); CHECK(successor->money == cash + (cancel_inherited ? 5600 : 0));
		CHECK(GetSurfaceSupplyOrderDayUsage(next, CargoLabel{"STEL"}) == (cancel_inherited ? 456 : 512));
		const auto *packets = Station::Get(rig.station)->goods[cargo].GetData().cargo.Packets();
		// Existing and newly fulfilled original purchases retain retired non-paying provenance in their actual native container; the successor owns custody, not a fabricated new paid source.
		for (StationCargoList::ConstIterator it(packets->begin()); it != packets->end(); ++it) CHECK((*it)->GetSource() == Source{Source::Invalid, SourceType::CompanySupply});
		CHECK(ValidateSurfaceSupplyOrderBook());
		ChangeOwnershipOfCompanyItems(next, INVALID_OWNER);
	}
}

/** Bankruptcy seals unshipped commitments before actual native demolition and CompanyID reuse; no replacement company inherits a refund or original buyer identity. */
TEST_CASE("Native company loss retires pending Surface rights before demolition and company slot reuse", "[surface-supply-lifecycle]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	AutoRestoreBackup local(_local_company, COMPANY_SPECTATOR); const auto original = _current_company;
	REQUIRE(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 1).Succeeded());
	const auto expense = Company::Get(original)->yearly_expenses[0][ExpensesType::Property];
	ChangeOwnershipOfCompanyItems(original, INVALID_OWNER); CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Lost);
	CHECK(GetSurfaceSupplyOrder(1)->owner == CompanyID::Invalid()); CHECK(GetSurfaceSupplyOrder(1)->Native()->packet_buyer == Source::Invalid);
	CHECK(Company::Get(original)->yearly_expenses[0][ExpensesType::Property] == expense); rig.RemoveCompany();
	REQUIRE(Company::CanAllocateItem()); auto *replacement = Company::Create(); REQUIRE(replacement->index == original); replacement->money = 50000;
	{
		AutoRestoreBackup actor(_current_company, replacement->index); CHECK(CancelPaidSurfaceOrder(1).Failed());
		CHECK(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 1).Failed()); CHECK(replacement->money == 50000); CHECK(ValidateSurfaceSupplyOrderBook());
	}
	delete replacement;
}

/** Original rating loss removes abandoned customer offers while purchased stock remains in the same native list, without another expense or refund. */
TEST_CASE("Native station rating abandonment preserves paid Surface stock and discards ordinary offers", "[surface-supply-lifecycle]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	AutoRestoreBackup counter(TimerGameTick::counter, TimerGameTick::TickCounter{0}); AutoRestoreBackup selectgoods(_settings_game.order.selectgoods, true);
	REQUIRE(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 200, 1).Succeeded()); OnTick_Station();
	auto *station = Station::Get(rig.station); const auto cargo = GetCargoTypeByLabel(CargoLabel{"STEL"}); auto &goods = station->goods[cargo];
	REQUIRE(CargoPacket::CanAllocateItem()); goods.GetOrCreateData().cargo.Append(CargoPacket::Create(rig.station, uint16_t{17}, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
	const auto paid = Company::Get(_current_company)->money; CHECK(goods.TotalCount() == 217);
	goods.time_since_pickup = 254; station->delete_ctr = Ticks::STATION_RATING_TICKS - 1; TimerGameTick::counter = 1; OnTick_Station();
	CHECK(goods.TotalCount() == 200); CHECK_FALSE(goods.HasRating()); CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Fulfilled);
	CHECK(CancelPaidSurfaceOrder(1).Failed()); CHECK(Company::Get(_current_company)->money == paid); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** All four scheduled labels use actual original finance; quotes/retries preserve native money, packets, randomness and monotonic issuance. */
TEST_CASE("Paid Surface commands charge and refund frozen purchases once through native finance", "[surface-supply-commands]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	const auto label = GENERATE(uint32_t{0x5354454C}, uint32_t{0x52464D54}, uint32_t{0x454C4543}, uint32_t{0x42415459});
	auto *company = Company::Get(_current_company); const auto cash = company->money;
	const auto expense = company->yearly_expenses[0][ExpensesType::Property]; const auto running = company->cur_economy.expenses;
	const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
	const auto estimate = PlacePaidSurfaceOrder(rig.station, label, 2, 1, true); REQUIRE(estimate.Succeeded()); CHECK(estimate.GetCost() > 0);
	CHECK(GetNextSurfaceSupplyOrderID() == 1); CHECK(CaptureSurfaceSupplyOrders().records.empty()); CHECK(company->money == cash);
	CHECK(company->yearly_expenses[0][ExpensesType::Property] == expense); CHECK(company->cur_economy.expenses == running);
	const auto placed = PlacePaidSurfaceOrder(rig.station, label, 2, 1); REQUIRE(placed.Succeeded()); CHECK(placed.GetCost() == estimate.GetCost());
	CHECK(company->money == cash - placed.GetCost()); CHECK(company->yearly_expenses[0][ExpensesType::Property] == expense + placed.GetCost());
	CHECK(company->cur_economy.expenses == running - placed.GetCost()); REQUIRE(GetSurfaceSupplyOrder(1) != nullptr);
	CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Pending); CHECK(GetNextSurfaceSupplyOrderID() == 2); CHECK(ValidateSurfaceSupplyOrderBook());
	const auto retry = PlacePaidSurfaceOrder(rig.station, label, 2, 1); REQUIRE(retry.Succeeded()); CHECK(retry.GetCost() == 0);
	CHECK(company->money == cash - placed.GetCost()); CHECK(GetNextSurfaceSupplyOrderID() == 2);
	// The original paid price survives later inflation; cancellation releases only unshipped rights and returns exactly the original expense through the same native account.
	AutoRestoreBackup inflation(_economy.inflation_prices, uint64_t{3} << 16);
	const auto quote_refund = CancelPaidSurfaceOrder(1, true); REQUIRE(quote_refund.Succeeded()); CHECK(quote_refund.GetCost() == -placed.GetCost());
	CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Pending); CHECK(company->money == cash - placed.GetCost());
	const auto cancelled = CancelPaidSurfaceOrder(1); REQUIRE(cancelled.Succeeded()); CHECK(cancelled.GetCost() == -placed.GetCost());
	CHECK(company->money == cash); CHECK(company->yearly_expenses[0][ExpensesType::Property] == expense); CHECK(company->cur_economy.expenses == running);
	CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Cancelled); CHECK(CancelPaidSurfaceOrder(1).Failed());
	CHECK(PlacePaidSurfaceOrder(rig.station, label, 2, 1).Failed()); CHECK(GetNextSurfaceSupplyOrderID() == 2);
	CHECK(CargoPacket::GetNumItems() == packets); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Saved intents bind company and complete payload; execution must refuse changes to money/date/ownership after an earlier successful pure quote. */
TEST_CASE("Paid Surface commands recapture actual owners and refuse changed or foreign intents unchanged", "[surface-supply-commands]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	auto *company = Company::Get(_current_company); const auto cash = company->money; const auto packets = CargoPacket::GetNumItems();
	REQUIRE(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 1).Succeeded()); const auto paid_cash = company->money;
	CHECK(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 3, 1).Failed()); CHECK(PlacePaidSurfaceOrder(rig.station, 0x52464D54, 2, 1).Failed());
	CHECK(PlacePaidSurfaceOrder(StationID::Invalid(), 0x5354454C, 2, 1).Failed());
	CHECK(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 0).Failed()); CHECK(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 3).Failed());
	CHECK(PlacePaidSurfaceOrder(rig.station, 0x4348454D, 2, 2).Failed()); CHECK(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 0, 2).Failed());
	CHECK(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 2, true).Succeeded());
	{
		AutoRestoreBackup funds(company->money, Money{0});
		CHECK(CmdPlaceSurfaceSupplyOrder(DoCommandFlag::Execute, rig.station, 0x5354454C, 2, 2).Failed());
		// An identical already paid retry needs no new cash and must never issue a second expense.
		CHECK(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 1).Succeeded()); CHECK(company->money == 0);
	}
	{
		AutoRestoreBackup date(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1949}, 0, 1));
		CHECK(CmdPlaceSurfaceSupplyOrder(DoCommandFlag::Execute, rig.station, 0x5354454C, 2, 2).Failed());
	}
	{
		AutoRestoreBackup owner(Station::Get(rig.station)->owner, Owner{OWNER_NONE});
		CHECK(CmdPlaceSurfaceSupplyOrder(DoCommandFlag::Execute, rig.station, 0x5354454C, 2, 2).Failed());
	}
	{
		REQUIRE(Company::CanAllocateItem()); auto *foreign = Company::Create();
		{
			AutoRestoreBackup actor(_current_company, foreign->index);
			CHECK(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 2, 1).Failed()); CHECK(CancelPaidSurfaceOrder(1).Failed());
		}
		delete foreign;
	}
	CHECK(company->money == paid_cash); CHECK(paid_cash == cash - 200); CHECK(GetNextSurfaceSupplyOrderID() == 2);
	CHECK(CaptureSurfaceSupplyOrders().records.size() == 1); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"STEL"}) == 2);
	CHECK(CargoPacket::GetNumItems() == packets); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Station cloning cannot multiply daily supply; cancellation releases only its own commitment and older pending orders still own their original receiving reservation. */
TEST_CASE("Paid Surface allowance spans origins and survives day rollover with monotonic retired history", "[surface-supply-commands]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	AutoRestoreBackup day(TimerGameEconomy::date);
	const auto other_tile = TileXYL(40, 20, MapLayer::Surface);
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, other_tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	const auto other = GetStationIndex(other_tile); REQUIRE(other != rig.station);
	const auto cash = Company::Get(_current_company)->money; const auto packets = CargoPacket::GetNumItems();
	REQUIRE(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 128, 1).Succeeded());
	REQUIRE(PlacePaidSurfaceOrder(other, 0x5354454C, 128, 2).Succeeded()); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"STEL"}) == 256);
	CHECK(PlacePaidSurfaceOrder(other, 0x5354454C, 1, 3).Failed()); CHECK(GetNextSurfaceSupplyOrderID() == 3);
	REQUIRE(CancelPaidSurfaceOrder(1).Succeeded()); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"STEL"}) == 128);
	TimerGameEconomy::date = TimerGameEconomy::Date{TimerGameEconomy::date.base() + 1}; CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"STEL"}) == 0);
	REQUIRE(PlacePaidSurfaceOrder(other, 0x5354454C, 256, 3).Succeeded());
	CHECK(GetSurfaceSupplyOrder(1) == nullptr); REQUIRE(GetSurfaceSupplyOrder(2) != nullptr); CHECK(GetSurfaceSupplyOrder(2)->Native()->state == SurfaceSupplyOrderState::Pending);
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"STEL"}) == 256); CHECK(GetNextSurfaceSupplyOrderID() == 4);
	CHECK(PlacePaidSurfaceOrder(rig.station, 0x5354454C, 128, 1).Failed()); CHECK(CancelPaidSurfaceOrder(1).Failed());
	CHECK(Company::Get(_current_company)->money == cash - 38400); CHECK(CargoPacket::GetNumItems() == packets); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Complete actual-reference restoration publishes metadata once while native money/packets/RNG remain unchanged; loading never executes a paid command. */
TEST_CASE("Native Surface order owner adopts only complete metadata without charging or dispatching", "[surface-supply-owner]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	const auto cash = Company::Get(_current_company)->money; const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
	CHECK(GetNextSurfaceSupplyOrderID() == 1); CHECK(CaptureSurfaceSupplyOrders().records.empty()); CHECK(GetSurfaceSupplyOrder(1) == nullptr);
	auto transport = rig.Pending(); auto shipped = transport.snapshot.records[0]; shipped.id = 2; shipped.quantity = 3; shipped.Native()->cost = 300;
	shipped.Native()->state = SurfaceSupplyOrderState::Fulfilled; shipped.Native()->issued_packet = CargoPacketID{17};
	transport.snapshot.records.push_back(shipped); transport.snapshot.next_id = 3;
	REQUIRE(StageLoadedSurfaceSupplyOrders(transport.snapshot, transport.saved_day));
	CHECK(GetNextSurfaceSupplyOrderID() == 1); CHECK(CaptureSurfaceSupplyOrders().records.empty());
	rig.ObserveWorld(); REQUIRE(AcceptLoadedSurfaceSupplyOrders(true)); REQUIRE(ValidateSurfaceSupplyOrderBook());
	CHECK(GetNextSurfaceSupplyOrderID() == 3); CHECK(CaptureSurfaceSupplyOrders().records.size() == 2);
	REQUIRE(GetSurfaceSupplyOrder(1) != nullptr); CHECK(GetSurfaceSupplyOrder(1)->Native()->state == SurfaceSupplyOrderState::Pending);
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"STEL"}) == 5); CHECK(GetSurfaceSupplyOrder(3) == nullptr);
	CHECK(GetNextSurfaceSupplyOrderID() == 3); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"STEL"}) == 5);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** A failed later original family cannot partially replace an installed native book or borrow missing/changed time/geometry/receiver data. */
TEST_CASE("Native Surface order adoption refuses missing changed or foreign original owners unchanged", "[surface-supply-owner]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	rig.Restore(rig.Pending()); const auto cash = Company::Get(_current_company)->money; const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
	auto candidate = rig.Pending(4); candidate.snapshot.next_id = 8; candidate.snapshot.records[0].id = 7;
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6);
	if (fault == 2) candidate.saved_day = TimerGameEconomy::Date{candidate.saved_day.base() + 1};
	if (fault == 5) candidate.snapshot.records[0].origin = StationID{999};
	if (fault == 6) candidate.snapshot.records[0].Native()->cost = 1;
	REQUIRE(StageLoadedSurfaceSupplyOrders(candidate.snapshot, candidate.saved_day));
	if (fault != 0) REQUIRE(ObserveLoadedSurfaceSupplyEconomyDate(fault == 1 ? TimerGameEconomy::Date{TimerGameEconomy::date.base() + 1} : TimerGameEconomy::date));
	if (fault != 3) REQUIRE(ObserveLoadedSurfaceSupplyMap(fault == 4 ? 128 : Map::SizeX(), Map::SizeY(), static_cast<uint8_t>(Map::LayerCount())));
	CHECK_FALSE(AcceptLoadedSurfaceSupplyOrders(true));
	CHECK(GetNextSurfaceSupplyOrderID() == 2); REQUIRE(GetSurfaceSupplyOrder(1) != nullptr); CHECK(GetSurfaceSupplyOrder(7) == nullptr);
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"STEL"}) == 2);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Missing/repeated family evidence never resets current rights; genuine old absence and explicit world replacement create only empty metadata. */
TEST_CASE("Native Surface order staging seals duplicate evidence and migrates old absence to empty", "[surface-supply-owner]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig;
	CHECK_FALSE(AcceptLoadedSurfaceSupplyOrders(true));
	const auto transport = rig.Pending(); REQUIRE(StageLoadedSurfaceSupplyOrders(transport.snapshot, transport.saved_day));
	CHECK_FALSE(StageLoadedSurfaceSupplyOrders(transport.snapshot, transport.saved_day));
	REQUIRE(ObserveLoadedSurfaceSupplyEconomyDate(TimerGameEconomy::date)); CHECK_FALSE(ObserveLoadedSurfaceSupplyEconomyDate(TimerGameEconomy::date));
	REQUIRE(ObserveLoadedSurfaceSupplyMap(Map::SizeX(), Map::SizeY(), static_cast<uint8_t>(Map::LayerCount())));
	CHECK_FALSE(ObserveLoadedSurfaceSupplyMap(Map::SizeX(), Map::SizeY(), static_cast<uint8_t>(Map::LayerCount())));
	REQUIRE(AcceptLoadedSurfaceSupplyOrders(true)); CHECK(GetNextSurfaceSupplyOrderID() == 2);
	ResetSurfaceSupplyOrderLoadStaging(); REQUIRE(AcceptLoadedSurfaceSupplyOrders(false));
	CHECK(GetNextSurfaceSupplyOrderID() == 1); CHECK(CaptureSurfaceSupplyOrders().records.empty());
	CHECK_FALSE(ObserveLoadedSurfaceSupplyMap(0, 64, 4));
	rig.Restore(transport); ResetSurfaceSupplyOrders(); CHECK(GetNextSurfaceSupplyOrderID() == 1); CHECK(GetSurfaceSupplyOrder(1) == nullptr);
}

/** Copied original facts deliberately differ from the current world, proving the full preview join cannot borrow live clock/map/company/station fields. */
TEST_CASE("Copied Surface order references use their own geometry time and company station evidence", "[surface-supply-owner]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyOwnerRig rig; rig.Restore(rig.Pending());
	AutoRestoreBackup version(_sl_version, SaveLoadVersion::SurfaceSupplyOrders);
	_load_check_data.Clear(); auto transport = rig.Pending(4); transport.saved_day = TimerGameEconomy::Date{100};
	transport.snapshot.next_id = 8; auto &record = transport.snapshot.records[0]; record.id = 7; record.Native()->origin_x = 40; record.Native()->origin_y = 30; record.purchase_day = transport.saved_day;
	_load_check_data.surface_supply_orders_present = true; _load_check_data.surface_supply_orders = transport;
	_load_check_data.native_map_dimensions_present = true; _load_check_data.map_size_x = 128; _load_check_data.map_size_y = 64; _load_check_data.map_layers = 4;
	_load_check_data.native_economy_date_present = true; _load_check_data.native_economy_date = transport.saved_day;
	_load_check_data.gameplay_profile_id = to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry);
	// Explicit empty copied owners remain mandatory evidence. The remapped
	// saved cargo definition is independent of the active world's CargoSpec slots.
	_load_check_data.gameplay_profile_version = 1;
	_load_check_data.native_cargo_packets_present = true;
	_load_check_data.native_vehicle_cargo_present = true;
	_load_check_data.native_station_cargo_present = true;
	_load_check_data.native_cargo_dimensions_present = true;
	_load_check_data.native_cargo_dimension_rows.push_back({CargoType{40}, CargoLabel{"STEL"}, 8});
	_load_check_data.companies[_current_company.base()] = std::make_unique<CompanyProperties>();
	_load_check_data.native_station_owner_rows.push_back({rig.station, _current_company, TileIndex{3880}, StationFacility::Train});
	const auto cash = Company::Get(_current_company)->money; const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
	switch (fault) {
		case 1: _load_check_data.native_map_dimensions_present = false; break;
		case 2: _load_check_data.native_economy_date_present = false; break;
		case 3: _load_check_data.native_economy_date = TimerGameEconomy::Date{101}; break;
		case 4: _load_check_data.companies.clear(); break;
		case 5: _load_check_data.native_station_owner_rows[0].owner = OWNER_NONE; break;
		case 6: _load_check_data.native_station_owner_rows[0].tile = TileIndex{12072}; break;
		case 7: _load_check_data.native_station_owner_rows[0].facilities = StationFacility::BusStop; break;
		case 8: _load_check_data.surface_supply_orders_present = false; break;
		case 9: _load_check_data.native_cargo_packets_present = false; break;
		case 10: _load_check_data.native_vehicle_cargo_present = false; break;
		case 11: _load_check_data.native_station_cargo_present = false; break;
	}
	// This component supplies detached copied facts, not an active framework
	// LoadCheck action. Inspect the original validator's reason without invoking
	// full-load failure cleanup against the unrelated installed pending book.
	CHECK(GetSurfaceSupplyOrderLoadPreviewError().has_value() == (fault != 0));
	CHECK(GetNextSurfaceSupplyOrderID() == 2); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"STEL"}) == 2);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	_load_check_data.Clear();
}
