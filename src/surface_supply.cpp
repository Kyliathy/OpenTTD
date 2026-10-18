/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply.cpp Whole paid-order admission over original station, company and economy owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13C.1.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB; one canonical scope/price calculation for quotes and saved receipts.
 * Runtime order/settlement/persistence remain Group13CC; packet purpose is delivered by13CB.
 */
#include "stdafx.h"
#include "surface_supply.h"
#include "multilayer_bootstrap.h"
#include "company_base.h"
#include "station_base.h"
#include "cargotype.h"
#include "cargopacket.h"
#include "economy_base.h"
#include "map_func.h"
#include "safeguards.h"

/** @copydoc GetSurfaceSupplyDefinition */
const MultiLayerSurfaceSupply *GetSurfaceSupplyDefinition(CargoLabel label)
{
	// Only the scheduled construction seed may enter this producer. Later catalogue supplies need their own runtime scope, not another cargo registry.
	const bool seed = label == CargoLabel{"STEL"} || label == CargoLabel{"RFMT"} || label == CargoLabel{"ELEC"} || label == CargoLabel{"BATY"};
	if (!seed) return nullptr;
	const auto definitions = GetMultiLayerSurfaceSupplies();
	const auto found = std::ranges::find(definitions, label, &MultiLayerSurfaceSupply::label);
	return found == definitions.end() ? nullptr : &*found;
}

/** @copydoc CalculateSurfaceSupplyCost */
std::optional<Money> CalculateSurfaceSupplyCost(CargoLabel label, uint16_t quantity, uint64_t inflation_prices)
{
	const auto *definition = GetSurfaceSupplyDefinition(label);
	// A complete price exists only for a supported positive purchase; invalid metadata cannot become a free order or a partial quote.
	if (definition == nullptr || quantity == 0) return std::nullopt;
	const uint64_t nominal = uint64_t{definition->price_per_unit} * quantity;
	// Preserve the original native Q16 inflation policy and bound multiplication before arithmetic; neither a wrapped charge nor a forged sub-normal factor may finance stock.
	if (inflation_prices < (uint64_t{1} << 16) || nominal > static_cast<uint64_t>(INT64_MAX) / inflation_prices) return std::nullopt;
	return Money{static_cast<int64_t>((nominal * inflation_prices) >> 16)};
}

/** @copydoc PrepareSurfaceSupplyOrder */
SurfaceSupplyOrderQuote PrepareSurfaceSupplyOrder(const SurfaceSupplyOrderRequest &request, const SurfaceSupplyOrderView &view)
{
	if (view.profile != MultiLayerGameplayProfileID::MultilayerIndustry) return {SurfaceSupplyOrderError::Profile};
	if (!view.company_exists) return {SurfaceSupplyOrderError::Company};
	// Procurement begins only at this buyer's real Surface freight facility;
	// ownership, a matching coordinate or an airport/Space view cannot grant it.
	if (!view.station_exists || view.station_owner != request.buyer || view.station_layer != MapLayer::Surface || !view.loading_station) return {SurfaceSupplyOrderError::Station};
	const auto *definition = GetSurfaceSupplyDefinition(request.label);
	// This producer delivers the already scheduled four-material construction
	// seed. Other published future supplies need their own explicit runtime scope.
	if (definition == nullptr || !IsValidCargoType(view.cargo) || view.cargo != GetCargoTypeByLabel(request.label)) return {SurfaceSupplyOrderError::Cargo};
	if (request.quantity == 0) return {SurfaceSupplyOrderError::Quantity};
	if (view.year < definition->introduction_year) return {SurfaceSupplyOrderError::Date};
	// Already shipped/reserved units across every supply station spend the same
	// daily company allowance. Check the prefix before subtracting to avoid wrap.
	if (view.company_quantity_today > definition->capacity_per_day || request.quantity > definition->capacity_per_day - view.company_quantity_today) return {SurfaceSupplyOrderError::DailyLimit};
	if (view.station_quantity > UINT32_MAX - request.quantity) return {SurfaceSupplyOrderError::StationSpace};
	if (!view.packet_room) return {SurfaceSupplyOrderError::PacketSpace};
	const auto cost = CalculateSurfaceSupplyCost(request.label, request.quantity, view.inflation_prices);
	if (!cost) return {SurfaceSupplyOrderError::Price};
	if (view.available_money < *cost) return {SurfaceSupplyOrderError::Funds};
	return {SurfaceSupplyOrderError::None, view.cargo, request.quantity, *cost};
}

/** @copydoc CaptureSurfaceSupplyOrderView */
SurfaceSupplyOrderView CaptureSurfaceSupplyOrderView(const SurfaceSupplyOrderRequest &request, uint32_t company_quantity_today)
{
	SurfaceSupplyOrderView view;
	view.profile = GetCurrentMultiLayerProfileSelection().id;
	view.year = TimerGameCalendar::ConvertDateToYMD(TimerGameCalendar::date).year;
	view.inflation_prices = _economy.inflation_prices;
	view.company_quantity_today = company_quantity_today;
	view.cargo = GetCargoTypeByLabel(request.label);
	view.packet_room = CargoPacket::CanAllocateItem();
	if (const auto *company = Company::GetIfValid(request.buyer)) {
		view.company_exists = true;
		view.available_money = company->money;
	}
	if (const auto *station = Station::GetIfValid(request.origin)) {
		view.station_exists = true;
		view.station_owner = station->owner;
		view.station_layer = GetTileLayer(station->xy);
		view.loading_station = station->facilities.Any({StationFacility::Train, StationFacility::TruckStop});
		if (IsValidCargoType(view.cargo)) view.station_quantity = station->goods[view.cargo].TotalCount();
	}
	return view;
}
