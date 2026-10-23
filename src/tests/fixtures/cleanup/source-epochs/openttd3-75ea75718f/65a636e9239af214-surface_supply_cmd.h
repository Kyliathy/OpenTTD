/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_cmd.h Synchronized native Surface purchase admission; source history alone owns its invoice.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB.
 */

#ifndef SURFACE_SUPPLY_CMD_H
#define SURFACE_SUPPLY_CMD_H

#include "command_type.h"
#include "surface_supply.h"
#include "misc/endian_buffer.hpp"

/**
 * Transport the exact canonical label bytes, independently of runtime cargo slot ordering.
 * @param buffer Native command writer, with its existing bounded buffer policy.
 * @param label Immutable four-byte cargo meaning; no translated name or numeric slot is sent.
 * @return The same writer after exactly four bytes.
 */
template <typename Tcont, typename Titer>
inline EndianBufferWriter<Tcont, Titer> &operator<<(EndianBufferWriter<Tcont, Titer> &buffer, const CargoLabel &label)
{
	for (uint8_t byte : label) buffer << byte;
	return buffer;
}

/**
 * Restore all four label bytes through the native command reader without allocation.
 * @param buffer Native bounded command reader; its normal truncation/refusal policy stays authoritative.
 * @param label Output meaning, validated against the actual seed/registry owner before settlement.
 * @return The same reader after the complete four-byte value.
 */
inline EndianBufferReader &operator>>(EndianBufferReader &buffer, CargoLabel &label)
{
	for (uint8_t &byte : label) buffer >> byte;
	return buffer;
}

/**
 * Place one whole paid order through the source's complete current admission.
 * @param flags Pure estimate or synchronized execution.
 * @param station Actual owned Surface freight station.
 * @param label Existing selected construction cargo meaning.
 * @param quantity Whole units to buy, never a silently reduced partial order.
 * @param expected_order Observed monotonic cursor; replay/retry after settlement must refuse without another purchase.
 * @return Native refusal or zero additional command fee; the source owner debits the quoted invoice once.
 */
CommandCost CmdOrderSurfaceSupply(DoCommandFlags flags, StationID station, CargoLabel label, uint16_t quantity, SurfaceSupplyOrderID expected_order);

DEF_CMD_TRAIT(Commands::OrderSurfaceSupply, CmdOrderSurfaceSupply, {}, CommandType::OtherManagement)

/**
 * Close one existing order and refund only exact original-source undispatched units.
 * @param flags Pure estimate or synchronized execution.
 * @param order Original paid purchase identity; closed/retried/foreign requests refuse unchanged.
 * @param expected_quantity Source units observed in the user's current cancellation quote; a changed pickup refuses before any credit.
 * @return Native refusal or zero added fee; the source owner removes real units and credits its frozen invoice once.
 */
CommandCost CmdCancelSurfaceSupply(DoCommandFlags flags, SurfaceSupplyOrderID order, uint16_t expected_quantity);

DEF_CMD_TRAIT(Commands::CancelSurfaceSupply, CmdCancelSurfaceSupply, {}, CommandType::OtherManagement)

#endif /* SURFACE_SUPPLY_CMD_H */
