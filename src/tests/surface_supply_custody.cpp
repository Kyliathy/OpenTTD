/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_custody.cpp Native packet-purpose, payment and shared save-admission boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB.
 * Packets, cash, completed factory and empty carrier are declared component inputs,
 * not purchased stock, driven construction delivery or an unassisted predecessor.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "surface_supply_legacy_fixture.h"
#include "processor_fixture.h"
#include "../train.h"
#include "../map_func.h"
#include "../economy_base.h"
#include "../economy_func.h"
#include "../subsidy_base.h"
#include "../subsidy_func.h"
#include "../saveload/cargo_source_sl.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Original native material/packet/payment owners; declared component stock provides no procurement evidence. */
class SupplyCustodyRig : public ProcessorRig {
	AutoRestoreBackup<decltype(_settings_game.economy.feeder_payment_share)> feeder_share{_settings_game.economy.feeder_payment_share, 75}; ///< Explicit positive ordinary transfer tariff, restored after the component check.
public:
	Station *origin = nullptr; ///< Actual Surface packet container.
	Station *transfer = nullptr; ///< Actual intermediate Surface packet container.
	Station *destination = nullptr; ///< Actual bounded same-layer electronics receiver.
	Train *carrier = nullptr; ///< Actual component train owns every in-transit packet.
	CargoPayment *payment = nullptr; ///< Current original native accounting participant.
	CargoType cargo = INVALID_CARGO; ///< Label-resolved RFMT, one of the four scheduled purchased materials.

	/** Create native component owners and a deliberately positive ordinary freight tariff. */
	SupplyCustodyRig() : ProcessorRig(MultiLayerRecipeID{"ELEC"})
	{
		REQUIRE(Vehicle::GetNumItems() == 0);
		this->origin = this->StationAt(TileXYL(20, 20, MapLayer::Surface));
		this->transfer = this->StationAt(TileXYL(24, 20, MapLayer::Surface));
		this->destination = this->StationAt(TileXYL(29, 30, MapLayer::Surface));
		this->cargo = GetCargoTypeByLabel(CargoLabel{"RFMT"});
		CargoSpec::Get(this->cargo)->current_payment = Money{1} << 20;
		REQUIRE(Train::CanAllocateItem()); this->carrier = Train::Create();
		this->carrier->owner = _current_company; this->carrier->cargo_type = this->cargo;
	}

	/** Settle the original payment and release component carrier packets before the inherited industry/station teardown. */
	~SupplyCustodyRig()
	{
		this->ClosePayment();
		this->carrier->cargo.Truncate();
		// Retire only this component's remaining station stock through its actual cargo owners before inherited pool teardown; unrelated earlier-test packets are not this fixture's inventory.
		for (auto *station : {this->origin, this->transfer, this->destination}) {
			if (station->goods[this->cargo].HasData()) station->goods[this->cargo].GetData().cargo.Truncate();
		}
		// The empty component hull has no purchased engine/statistics. Pool teardown retires its native packet custody without invoking unrelated engine sale bookkeeping.
		_vehicle_pool.CleanPool();
	}

	/** @param source Declared original provenance/purpose. @param quantity Whole component units added to the original station container. */
	void Offer(Source source, uint16_t quantity)
	{
		REQUIRE(CargoPacket::CanAllocateItem());
		this->origin->goods[this->cargo].GetOrCreateData().cargo.Append(CargoPacket::Create(this->origin->index, quantity, source), StationID::Invalid());
	}

	/** @param station Actual next unloading station; no virtual station, payment or inventory replaces it. */
	void BeginPayment(Station &station)
	{
		REQUIRE(this->payment == nullptr); REQUIRE(CargoPayment::CanAllocateItem());
		this->carrier->last_station_visited = station.index;
		this->payment = CargoPayment::Create(this->carrier); this->carrier->cargo_payment = this->payment;
	}

	/** Commit original cash/vehicle accounting; suppress only isolated fixture display effects. */
	void ClosePayment()
	{
		AutoRestoreBackup mode(_game_mode, GameMode::Menu);
		delete this->payment; this->payment = nullptr;
	}

	/** @param station Actual unloading container. @param transfer Whether to use native transfer staging instead of final delivery. @return Exactly moved native units. */
	uint Unload(Station &station, bool transfer)
	{
		this->BeginPayment(station);
		this->carrier->cargo.Stage(!transfer, station.index, {}, transfer ? OrderUnloadType::Transfer : OrderUnloadType::UnloadIfPossible,
			&station.goods[this->cargo], this->cargo, this->payment, station.xy);
		return this->carrier->cargo.Unload(100, &station.goods[this->cargo].GetOrCreateData().cargo, this->cargo, this->payment, station.xy);
	}
};

/** Migrated stock uses the same rating, reservation, physical intake and statistics owners without recovering freight income. */
TEST_CASE("Legacy purchased stock keeps useful bounded delivery and statistics without rating loss or revenue", "[surface-supply-legacy-delivery]")
{
	SupplyCustodyRig rig; LegacySupplyComponent legacy(rig.origin->index);
	auto &origin = rig.origin->goods[rig.cargo].GetOrCreateData().cargo;
	origin.Append(legacy.Packet(8), StationID::Invalid()); legacy.Restore(legacy.Candidate(8));
	const auto cash = Company::Get(_current_company)->money; const auto history = CaptureSurfaceSupplyOrders();
	CHECK(origin.TruncateAbandonedOffers(UINT_MAX) == 0); CHECK(origin.TotalCount() == 8);
	rig.Offer(Source{Source::Invalid, SourceType::Industry}, 3);
	CHECK(origin.TruncateAbandonedOffers(UINT_MAX) == 3); CHECK(origin.TotalCount() == 8);
	CHECK(CaptureSurfaceSupplyOrders() == history); CHECK(origin.CountUnshippedCompanySupply(1) == 8);
	const std::array intermediate{rig.transfer->index};
	REQUIRE(origin.Reserve(8, &rig.carrier->cargo, intermediate, rig.origin->xy) == 8);
	CHECK(origin.CountUnshippedCompanySupply(1) == 0); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->dispatched == 8);
	REQUIRE(origin.Load(8, &rig.carrier->cargo, intermediate, rig.origin->xy) == 8); CHECK(origin.ReservedCount() == 0);
	REQUIRE(rig.Unload(*rig.transfer, true) == 8); CHECK(rig.payment->visual_transfer == 0); CHECK(rig.payment->route_profit == 0);
	rig.ClosePayment(); CHECK(Company::Get(_current_company)->money == cash);
	auto &transferred = rig.transfer->goods[rig.cargo].GetData().cargo;
	const auto *packet = transferred.Packets()->begin()->second.front();
	CHECK(packet->GetCompanySupplyOrder() == 1); CHECK(packet->HasCompanySupplyShipped()); CHECK(packet->GetSource().IsCompanySupply()); CHECK(packet->GetFeederShare() == 0);
	const std::array final{rig.destination->index}; REQUIRE(transferred.Load(8, &rig.carrier->cargo, final, rig.transfer->xy) == 8);
	REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, rig.cargo, 1023) == 1023);
	const auto delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo];
	REQUIRE(rig.Unload(*rig.destination, false) == 1); CHECK(rig.Input(CargoLabel{"RFMT"}).waiting == 1024);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == delivered + 1);
	CHECK(rig.destination->goods[rig.cargo].status.Test(GoodsEntry::State::EverAccepted));
	CHECK(rig.carrier->cargo.TotalCount() == 7); CHECK(rig.payment->route_profit == 0); CHECK(rig.payment->visual_profit == 0);
	rig.ClosePayment(); CHECK(Company::Get(_current_company)->money == cash);
	packet = rig.carrier->cargo.Packets()->front(); CHECK(packet->GetCompanySupplyOrder() == 1); CHECK(packet->HasCompanySupplyShipped());
	CHECK(packet->GetSource().IsCompanySupply()); CHECK(packet->GetFeederShare() == 0); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->dispatched == 8);
	CHECK(GetSurfaceSupplyOrder(1)->Legacy()->refunded == 0); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Native station/vehicle merging cannot erase purpose or buyer, and invalidating the original buyer cannot enable income. */
TEST_CASE("Company supplies retain native purpose through split transfer merge and buyer invalidation", "[surface-supply-custody]")
{
	SupplyCustodyRig rig;
	const Source bought{_current_company, SourceType::CompanySupply};
	const auto prior_packets = CargoPacket::GetNumItems();
	rig.Offer(bought, 8); rig.Offer(bought, 2);
	rig.Offer(Source{_current_company, SourceType::Industry}, 8);
	rig.Offer(Source{static_cast<SourceID>((_current_company.base() + 1) % MAX_COMPANIES), SourceType::CompanySupply}, 8);
	auto &origin = rig.origin->goods[rig.cargo].GetData().cargo;
	REQUIRE(origin.TotalCount() == 26); REQUIRE(CargoPacket::GetNumItems() == prior_packets + 3);
	const auto *original = origin.Packets()->begin()->second.front(); REQUIRE(original->GetSource() == bought); CHECK(original->Count() == 10);
	// Both container merge guards must reject otherwise identical physical-origin/time packets from another purpose or buyer; stock custody must never erase either distinction.
	for (const auto *other : origin.Packets()->begin()->second) {
		if (other == original) continue;
		CHECK_FALSE(StationCargoList::AreMergable(original, other)); CHECK_FALSE(VehicleCargoList::AreMergable(original, other));
	}
	const std::array next{rig.transfer->index};
	REQUIRE(origin.Load(3, &rig.carrier->cargo, next, rig.origin->xy) == 3);
	const auto *loaded = *rig.carrier->cargo.Packets()->begin();
	CHECK(loaded->index != original->index); CHECK(loaded->GetSource() == bought); CHECK(original->Count() == 7);
	CHECK(loaded->GetFirstStation() == rig.origin->index); CHECK(loaded->GetSourceTile() == rig.origin->xy);
	const auto cash = Company::Get(_current_company)->money;
	REQUIRE(rig.Unload(*rig.transfer, true) == 3);
	CHECK(rig.payment->route_profit == 0); CHECK(rig.payment->visual_transfer == 0); CHECK(rig.payment->visual_profit == 0);
	rig.ClosePayment(); CHECK(Company::Get(_current_company)->money == cash);
	auto &transferred = rig.transfer->goods[rig.cargo].GetData().cargo;
	REQUIRE(transferred.AvailableCount() == 3);
	CHECK(transferred.Packets()->begin()->second.front()->GetSource() == bought);
	const std::array final{rig.destination->index};
	REQUIRE(transferred.Load(3, &rig.carrier->cargo, final, rig.transfer->xy) == 3);
	CargoPacket::InvalidateAllFrom(bought);
	const auto retired = (*rig.carrier->cargo.Packets()->begin())->GetSource();
	CHECK_FALSE(retired.IsValid()); CHECK(retired.IsCompanySupply());
	CHECK(retired.GetNewsReference() == NewsReference{std::monostate{}}); CHECK(retired.GetFormat() == STR_JUST_COMMA);
	REQUIRE(rig.Unload(*rig.destination, false) == 3);
	CHECK(rig.Input(CargoLabel{"RFMT"}).waiting == 3); CHECK(rig.carrier->cargo.TotalCount() == 0);
	CHECK(rig.payment->route_profit == 0); CHECK(rig.payment->visual_profit == 0);
	rig.ClosePayment(); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(origin.TotalCount() + transferred.TotalCount() + rig.Input(CargoLabel{"RFMT"}).waiting == 26);
}

/** Identical real receiver/positive tariff proves the new guard suppresses bought income without breaking ordinary final income or bounded intake. */
TEST_CASE("Bought material final intake stays bounded while ordinary freight still earns income", "[surface-supply-custody]")
{
	SupplyCustodyRig rig;
	const bool bought = GENERATE(false, true);
	const Source source = bought ? Source{_current_company, SourceType::CompanySupply} : Source{Source::Invalid, SourceType::Industry};
	REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, rig.cargo, 1023) == 1023);
	rig.Offer(source, 8);
	const std::array next{rig.destination->index};
	REQUIRE(rig.origin->goods[rig.cargo].GetData().cargo.Load(8, &rig.carrier->cargo, next, rig.origin->xy) == 8);
	const auto cash = Company::Get(_current_company)->money;
	const auto delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo];
	const auto tariff = GetTransportedGoodsIncome(1, DistanceManhattan(rig.origin->xy, rig.destination->xy), 0, rig.cargo);
	REQUIRE(tariff > 0);
	REQUIRE(rig.Unload(*rig.destination, false) == 1);
	CHECK(rig.Input(CargoLabel{"RFMT"}).waiting == 1024); CHECK(rig.carrier->cargo.TotalCount() == 7);
	const auto *remaining = *rig.carrier->cargo.Packets()->begin();
	CHECK(remaining->GetSource() == source); CHECK(remaining->GetFeederShare() == 0);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == delivered + 1);
	CHECK(rig.payment->route_profit == (bought ? Money{0} : tariff)); CHECK(rig.payment->visual_profit == (bought ? Money{0} : tariff));
	rig.ClosePayment(); CHECK(Company::Get(_current_company)->money == cash + (bought ? Money{0} : tariff));
}

/** Native intermediate transfer preserves paying freight's feeder credit while bought supplies retain zero through final settlement. */
TEST_CASE("Ordinary transfer credits survive beside non-paying company supplies", "[surface-supply-custody]")
{
	SupplyCustodyRig rig;
	const bool bought = GENERATE(false, true);
	const Source source = bought ? Source{_current_company, SourceType::CompanySupply} : Source{Source::Invalid, SourceType::Industry};
	rig.Offer(source, 8);
	const std::array next{rig.transfer->index};
	REQUIRE(rig.origin->goods[rig.cargo].GetData().cargo.Load(8, &rig.carrier->cargo, next, rig.origin->xy) == 8);
	const auto cash = Company::Get(_current_company)->money;
	const auto transfer_tariff = GetTransportedGoodsIncome(8, DistanceManhattan(rig.origin->xy, rig.transfer->xy), 0, rig.cargo) * _settings_game.economy.feeder_payment_share / 100;
	REQUIRE(transfer_tariff > 0);
	REQUIRE(rig.Unload(*rig.transfer, true) == 8);
	CHECK(rig.payment->visual_transfer == (bought ? Money{0} : transfer_tariff)); CHECK(rig.payment->route_profit == 0);
	rig.ClosePayment(); CHECK(Company::Get(_current_company)->money == cash);
	auto &stock = rig.transfer->goods[rig.cargo].GetData().cargo;
	REQUIRE(stock.AvailableCount() == 8);
	CHECK(stock.Packets()->begin()->second.front()->GetSource() == source);
	CHECK(stock.Packets()->begin()->second.front()->GetFeederShare() == (bought ? Money{0} : transfer_tariff));
	const std::array final{rig.destination->index};
	REQUIRE(stock.Load(8, &rig.carrier->cargo, final, rig.transfer->xy) == 8);
	const auto final_tariff = GetTransportedGoodsIncome(8, DistanceManhattan(rig.origin->xy, rig.destination->xy), 0, rig.cargo);
	REQUIRE(rig.Unload(*rig.destination, false) == 8);
	CHECK(rig.payment->route_profit == (bought ? Money{0} : final_tariff));
	CHECK(rig.payment->visual_profit == (bought ? Money{0} : final_tariff - transfer_tariff));
	rig.ClosePayment(); CHECK(Company::Get(_current_company)->money == cash + (bought ? Money{0} : final_tariff));
	CHECK(rig.Input(CargoLabel{"RFMT"}).waiting == 8); CHECK(rig.carrier->cargo.TotalCount() == 0); CHECK(stock.TotalCount() == 0);
}

/** One real carrier's cumulative settlement must retain ordinary earnings regardless of the bought packet's position in the unloading sequence. */
TEST_CASE("Mixed bought and ordinary packets settle only the ordinary part of one real load", "[surface-supply-custody]")
{
	SupplyCustodyRig rig;
	const bool bought_first = GENERATE(false, true);
	const Source bought{_current_company, SourceType::CompanySupply};
	const Source ordinary{Source::Invalid, SourceType::Industry};
	rig.Offer(bought_first ? bought : ordinary, 8); rig.Offer(bought_first ? ordinary : bought, 8);
	const std::array next{rig.destination->index};
	REQUIRE(rig.origin->goods[rig.cargo].GetData().cargo.Load(16, &rig.carrier->cargo, next, rig.origin->xy) == 16);
	const auto cash = Company::Get(_current_company)->money;
	const auto income = GetTransportedGoodsIncome(8, DistanceManhattan(rig.origin->xy, rig.destination->xy), 0, rig.cargo);
	REQUIRE(income > 0);
	REQUIRE(rig.Unload(*rig.destination, false) == 16);
	CHECK(rig.Input(CargoLabel{"RFMT"}).waiting == 16); CHECK(rig.carrier->cargo.TotalCount() == 0);
	CHECK(rig.payment->route_profit == income); CHECK(rig.payment->visual_profit == income); CHECK(rig.payment->visual_transfer == 0);
	rig.ClosePayment(); CHECK(Company::Get(_current_company)->money == cash + income);
}

/** An offered ordinary-industry subsidy with the same numeric identity cannot be acquired by company-supply provenance. */
TEST_CASE("Company supply provenance cannot alias an offered industry subsidy", "[surface-supply-custody]")
{
	SupplyCustodyRig rig;
	REQUIRE(Subsidy::CanAllocateItem());
	auto *subsidy = Subsidy::Create(rig.cargo, Source{rig.industry->index, SourceType::Industry}, Source{rig.industry->index, SourceType::Industry}, uint16_t{12});
	CHECK_FALSE(CheckSubsidised(rig.cargo, _current_company, Source{static_cast<SourceID>(rig.industry->index.base()), SourceType::CompanySupply}, rig.destination));
	CHECK_FALSE(subsidy->IsAwarded()); CHECK(subsidy->remaining == 12);
	delete subsidy;
}

/** Both native/full and copied CAPA use this exact candidate-file predicate; legacy sources keep their original credit semantics. */
TEST_CASE("Saved company supplies require their genuine era bounded buyer and zero feeder credit", "[surface-supply-custody]")
{
	const auto version = SaveLoadVersion::CompanySupplyPackets;
	CHECK(to_underlying(version) == 439);
	CHECK(to_underlying(SourceType::Industry) == 0); CHECK(to_underlying(SourceType::Town) == 1);
	CHECK(to_underlying(SourceType::Headquarters) == 2); CHECK(to_underlying(SourceType::Asteroid) == 3); CHECK(to_underlying(SourceType::CompanySupply) == 4);
	// Each real buyer slot and the retired-source sentinel preserve the non-paying purpose; none requires the open world to contain that company.
	for (SourceID buyer = 0; buyer < MAX_COMPANIES; ++buyer) CHECK(IsValidCargoPacketSource(Source{buyer, SourceType::CompanySupply}, 0, version));
	CHECK(IsValidCargoPacketSource(Source{Source::Invalid, SourceType::CompanySupply}, 0, version));
	CHECK_FALSE(IsValidCargoPacketSource(Source{0, SourceType::CompanySupply}, 0, SaveLoadVersion::OrbitalRecipeProcessing));
	CHECK_FALSE(IsValidCargoPacketSource(Source{MAX_COMPANIES, SourceType::CompanySupply}, 0, version));
	CHECK_FALSE(IsValidCargoPacketSource(Source{OWNER_DEITY, SourceType::CompanySupply}, 0, version));
	CHECK_FALSE(IsValidCargoPacketSource(Source{0, SourceType::CompanySupply}, 1, version));
	CHECK_FALSE(IsValidCargoPacketSource(Source{Source::Invalid, SourceType::CompanySupply}, -1, version));
	CHECK_FALSE(IsValidCargoPacketSource(Source{0, static_cast<SourceType>(5)}, 0, version));
	CHECK_FALSE(IsValidCargoPacketSource(Source{0, static_cast<SourceType>(255)}, 0, version));
	// The appended meaning must not narrow historical ordinary source/credit domains or reinterpret old asteroid packets.
	for (auto type : {SourceType::Industry, SourceType::Town, SourceType::Headquarters}) {
		CHECK(IsValidCargoPacketSource(Source{Source::Invalid, type}, -30, SaveLoadVersion::TableChunks));
		CHECK(IsValidCargoPacketSource(Source{0, type}, 30, version));
	}
	CHECK(IsValidCargoPacketSource(Source{0, SourceType::Asteroid}, 30, SaveLoadVersion::SpaceMiningVehicles));
	CHECK_FALSE(IsValidCargoPacketSource(Source{MAX_ORBITAL_BODIES, SourceType::Asteroid}, 0, version));
	CHECK_FALSE(IsValidCargoPacketSource(Source{0, SourceType::Asteroid}, 0, SaveLoadVersion::TableChunks));
}
