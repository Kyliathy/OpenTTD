/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_cmd.cpp Native paid-order command, retaining the single purchase invoice/cursor owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB.
 */

#include "stdafx.h"
#include "surface_supply_cmd.h"
#include "table/strings.h"
#include "safeguards.h"

/**
 * Translate actual source-owner results without a second purchase/refund invoice.
 * @param error Existing complete admission/settlement outcome, including native refund balance limits.
 * @return Localized refusal or zero extra fee; physical stock and company money stay with their original owners.
 */
static CommandCost SurfaceSupplyCommandResult(SurfaceSupplyError error)
{
	switch (error) {
		case SurfaceSupplyError::None: return CommandCost(ExpensesType::Other, Money{0}); // Purchase owner already charges; native command processing must never debit a second invoice.
		case SurfaceSupplyError::Date: return CommandCost(STR_ERROR_SURFACE_SUPPLY_DATE);
		case SurfaceSupplyError::Funds: return CommandCost(STR_ERROR_SURFACE_SUPPLY_FUNDS);
		case SurfaceSupplyError::DailyCap: return CommandCost(STR_ERROR_SURFACE_SUPPLY_CAP);
		case SurfaceSupplyError::Stock: return CommandCost(STR_ERROR_SURFACE_SUPPLY_STOCK);
		case SurfaceSupplyError::Stale: return CommandCost(STR_ERROR_SURFACE_SUPPLY_STALE);
		case SurfaceSupplyError::RefundLimit: return CommandCost(STR_ERROR_SURFACE_SUPPLY_REFUND_LIMIT);
		default: return CommandCost(STR_ERROR_SURFACE_SUPPLY_REFUSED);
	}
}

/** @copydoc CmdOrderSurfaceSupply */
CommandCost CmdOrderSurfaceSupply(DoCommandFlags flags, StationID station, CargoLabel label, uint16_t quantity, SurfaceSupplyOrderID expected_order)
{
	if (expected_order != GetNextSurfaceSupplyOrderID()) return SurfaceSupplyCommandResult(SurfaceSupplyError::Stale);
	const auto quote = PrepareSurfaceSupplyOrder(station, label, quantity);
	return SurfaceSupplyCommandResult(PlaceSurfaceSupplyOrder(quote, flags.Test(DoCommandFlag::Execute)));
}

/** @copydoc CmdCancelSurfaceSupply */
CommandCost CmdCancelSurfaceSupply(DoCommandFlags flags, SurfaceSupplyOrderID order, uint16_t expected_quantity)
{
	const auto quote = PrepareSurfaceSupplyCancellation(order);
	if (quote.error != SurfaceSupplyError::None) return SurfaceSupplyCommandResult(quote.error);
	if (quote.quantity != expected_quantity) return SurfaceSupplyCommandResult(SurfaceSupplyError::Stale);
	return SurfaceSupplyCommandResult(CancelSurfaceSupplyOrder(quote, flags.Test(DoCommandFlag::Execute)));
}
