/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_delivery.cpp Real paid company-supply packet intake at the native powerhouse boundary; arrival is explicit fixture input.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8C/8E.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13C; full driven first-plant acceptance remains8F.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "geothermal_generator_fixture.h"
#include "../surface_supply.h"
#include "../surface_supply_cmd.h"
#include "../surface_supply_order_runtime.h"
#include "../station_func.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../train.h"
#include "../economy_base.h"
#include "../economy_func.h"
#include "../table/strings.h"

/** One actual canonical paid commitment, with no parallel quota, inventory or refundable right. */
struct GeneratorPaidInput {
	SurfaceSupplyOrderID next_order; ///< Sole native monotonic order identity, not a donor receipt.
	CargoType cargo; ///< Actual resolved construction meaning.
	Money cost; ///< Full cost settled by the native command kernel.
};

/** Advance only the original station dispatcher to dispatch already paid commitments into real station custody. */
extern void OnTick_Station();

/** Actual source station, paid packets and final-payment boundary; the raw packet carrier is never serialized or claimed as a driven train. */
class GeneratorPacketRig : public GeothermalGeneratorRig {
	AutoRestoreBackup<uint64_t> inflation{_economy.inflation_prices, uint64_t{1} << 16}; ///< Exact uninflated source invoice.
public:
	Station *origin = nullptr; ///< Actual owned Surface source queue.
	Station *destination = nullptr; ///< Actual Deep catchment beside the unfinished powerhouse.
	Train *carrier = nullptr; ///< Real native packet list at the explicitly supplied arrival boundary.
	/** @param model Actual canonical dated machinery to fund; stations/paid packets supply no hidden stock or driven arrival. */
	explicit GeneratorPacketRig(GeothermalGeneratorModel model = GeothermalGeneratorModel::VentTap) : GeothermalGeneratorRig(false, model)
	{
		REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Station::GetNumItems() == 0);
		ResetSurfaceSupplyOrders(); REQUIRE(this->Fund().Succeeded());
		(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
		this->origin = this->StationAt(TileXYL(20, 20, MapLayer::Surface));
		this->destination = this->StationAt(TileXYL(35, 24, MapLayer::DeepUnderground));
		REQUIRE(Train::CanAllocateItem()); this->carrier = Train::Create();
		this->carrier->owner = _current_company; this->carrier->last_station_visited = this->destination->index;
		REQUIRE(CargoPayment::CanAllocateItem()); this->carrier->cargo_payment = CargoPayment::Create(this->carrier);
	}
	/** Settle only real payment/cargo owners before station and machinery teardown; no invalid raw engine reaches a playable save. */
	~GeneratorPacketRig()
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu};
		delete this->carrier->cargo_payment; this->carrier->cargo_payment = nullptr;
		_vehicle_pool.CleanPool(); // Packet-only participants have no engine/group statistics; release retained real packets without inventing that bookkeeping.
		// Industry destruction removes its actual catchment references. Keep
		// both stations alive until that native cleanup has completed.
		// Retire machinery before its actual nearby stations and native supply metadata.
		for (Industry *industry : Industry::Iterate()) delete industry;
		_station_pool.CleanPool(); ResetSurfaceSupplyOrders();
	}
	/** @param tile Explicit empty Surface or prepared Deep berth. @return Actual same-layer native station and rebuilt catchment, never a cross-layer acceptance link. */
	Station *StationAt(TileIndex tile)
	{
		REQUIRE(Station::CanAllocateItem());
		if (GetTileLayer(tile) != MapLayer::Surface) this->Open(std::array{tile});
		Station *station = Station::Create(tile);
		station->owner = _current_company; station->town = *Town::Iterate().begin(); station->string_id = STR_SV_STNAME;
		station->facilities.Set(StationFacility::Train);
		MakeRailStation(tile, _current_company, station->index, Axis::X, 0, RAILTYPE_RAIL);
		station->train_station = TileArea(tile, 1, 1); station->spread.Add(tile); station->RecomputeCatchment();
		return station;
	}
	/** @param label Canonical actual construction cargo. @param quantity Whole native paid source units. @return Original complete funded quote, including the actual order identity. */
	GeneratorPaidInput Offer(CargoLabel label, uint16_t quantity)
	{
		REQUIRE(this->carrier->cargo.TotalCount() == 0);
		const SurfaceSupplyOrderRequest request{this->origin->index, _current_company, label, quantity};
		const auto usage = GetSurfaceSupplyOrderDayUsage(_current_company, label); REQUIRE(usage.has_value());
		const auto quote = PrepareSurfaceSupplyOrder(request, CaptureSurfaceSupplyOrderView(request, *usage));
		REQUIRE(quote.error == SurfaceSupplyOrderError::None);
		const auto intent = GetNextSurfaceSupplyOrderID();
		const uint32_t wire_label = (uint32_t{label[0]} << 24) | (uint32_t{label[1]} << 16) | (uint32_t{label[2]} << 8) | label[3];
		const auto purchase = Command<Commands::PlaceSurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, INVALID_TILE,
			std::make_tuple(this->origin->index, wire_label, quantity, intent));
		REQUIRE(purchase.Succeeded()); CHECK(purchase.GetCost() == quote.cost);
		REQUIRE(GetSurfaceSupplyOrder(intent)->Native()->state == SurfaceSupplyOrderState::Pending);
		CHECK(this->origin->goods[quote.cargo].TotalCount() == 0);
		// Only the native station interval can turn the paid commitment into actual
		// packet custody. This declared arrival fixture grants no driven journey.
		TimerGameTick::counter += Ticks::STATION_ACCEPTANCE_TICKS - TimerGameTick::counter % Ticks::STATION_ACCEPTANCE_TICKS;
		OnTick_Station();
		REQUIRE(GetSurfaceSupplyOrder(intent)->Native()->state == SurfaceSupplyOrderState::Fulfilled);
		// First station custody permanently ends native refund rights. Unlike
		// migrated donor stock, new packets acquire no legacy refund link/flag.
		const auto cancellation = Command<Commands::CancelSurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, INVALID_TILE,
			std::make_tuple(intent));
		CHECK(cancellation.Failed());
		this->carrier->cargo_type = quote.cargo;
		CargoSpec::Get(quote.cargo)->current_payment = Money{1000000}; // A purpose regression cannot hide behind an uninitialized zero freight tariff.
		const std::array<StationID, 0> next{};
		REQUIRE(this->origin->goods[quote.cargo].GetOrCreateData().cargo.Load(quantity, &this->carrier->cargo, next, this->origin->xy) == quantity);
		this->carrier->cargo.Stage(true, this->destination->index, {}, OrderUnloadType::UnloadIfPossible,
			&this->destination->goods[quote.cargo], quote.cargo, this->carrier->cargo_payment, this->destination->xy);
		return {intent, quote.cargo, quote.cost};
	}
	/** @return Exact real packet units removed by native final delivery; refused stock remains in its original carrier list. */
	uint Unload()
	{
		const auto cargo = this->carrier->cargo_type;
		return this->carrier->cargo.Unload(100, &this->destination->goods[cargo].GetOrCreateData().cargo,
			cargo, this->carrier->cargo_payment, this->destination->xy);
	}
};

/** Four actual bought packet meanings fund the sole native whole construction bill and earn no freight or subsidy income. */
TEST_CASE("Paid Surface packets enter the actual powerhouse construction owner without freight income", "[geothermal-generator-delivery]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	GeneratorPacketRig rig{model}; const auto cash = Company::Get(_current_company)->money;
	Money invoice = 0;
	// Every input travels through the actual source queue, native loading and
	// final-delivery receipt; direct scalar supply is not used by this case.
	for (const auto &input : GetMultiLayerGeothermalConstruction(rig.request.kind)->inputs) {
		const auto quote = rig.Offer(input.label, input.quantity); invoice += quote.cost;
		REQUIRE(rig.carrier->cargo.Packets()->size() == 1);
		const auto *packet = rig.carrier->cargo.Packets()->front();
		CHECK(packet->GetSource() == Source{_current_company, SourceType::CompanySupply});
		CHECK(packet->GetCompanySupplyOrder() == 0); CHECK_FALSE(packet->HasCompanySupplyShipped());
		CHECK(packet->GetFeederShare() == Money{0}); CHECK(packet->GetFirstStation() == rig.origin->index);
		CHECK(rig.Unload() == input.quantity); CHECK(rig.carrier->cargo.TotalCount() == 0);
		CHECK(rig.Generator().GetCargoAccepted(quote.cargo)->waiting == input.quantity);
		CHECK(GetSurfaceSupplyOrder(quote.next_order)->Native()->state == SurfaceSupplyOrderState::Fulfilled);
		CHECK(rig.carrier->cargo_payment->route_profit == Money{0}); CHECK(rig.carrier->cargo_payment->visual_profit == Money{0});
	}
	CHECK(invoice == Money{model == GeothermalGeneratorModel::VentTap ? 5340 : 10680}); CHECK(Company::Get(_current_company)->money == cash - invoice);
	CHECK(CargoPacket::GetNumItems() == 0); rig.Tick(static_cast<uint16_t>(GetMultiLayerGeothermalConstruction(rig.request.kind)->work_ticks));
	CHECK(IsGeothermalGeneratorConstructionComplete(rig.Generator().index));
	CHECK(Company::Get(_current_company)->money == cash - invoice);
	CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Final intake accepts only the remaining paid bill; repeated unloading cannot consume or pay for the retained original excess. */
TEST_CASE("Native powerhouse refuses surplus paid packets intact after the remaining bill", "[geothermal-generator-delivery-excess]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	GeneratorPacketRig rig;
	const auto quote = rig.Offer(CargoLabel{"STEL"}, 15);
	const Money cash = Company::Get(_current_company)->money;
	const auto packet = rig.carrier->cargo.Packets()->front()->index;
	CHECK(rig.Unload() == 12); CHECK(rig.carrier->cargo.TotalCount() == 3);
	const auto *remaining = rig.carrier->cargo.Packets()->front();
	CHECK(remaining->index == packet); CHECK(remaining->GetCompanySupplyOrder() == 0);
	CHECK_FALSE(remaining->HasCompanySupplyShipped()); CHECK(remaining->GetFirstStation() == rig.origin->index);
	CHECK(remaining->GetSource() == Source{_current_company, SourceType::CompanySupply}); CHECK(remaining->GetFeederShare() == Money{0});
	CHECK(GetSurfaceSupplyOrder(quote.next_order)->Native()->state == SurfaceSupplyOrderState::Fulfilled);
	CHECK(rig.Unload() == 0); CHECK(rig.carrier->cargo.TotalCount() == 3);
	CHECK(rig.Generator().GetCargoAccepted(quote.cargo)->waiting == 12);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(rig.carrier->cargo_payment->route_profit == Money{0}); CHECK(rig.carrier->cargo_payment->visual_profit == Money{0});
	CHECK(ValidateSurfaceSupplyOrderBook());
}
