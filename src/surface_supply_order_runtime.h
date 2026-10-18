/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order_runtime.h Sole native order-book queries and complete-file restoration boundary.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups13CCC/13CCD.
 * Read-only snapshots grant no purchase/refund/packet authority; synchronized commands alone create/cancel paid commitments.
 */
#ifndef SURFACE_SUPPLY_ORDER_RUNTIME_H
#define SURFACE_SUPPLY_ORDER_RUNTIME_H

#include "surface_supply_order.h"

/** Clear native commitment/history and unpublished load staging during ordinary world replacement; never refund the discarded world's money. */
void ResetSurfaceSupplyOrders();

/** @return Complete read-only copy of the sole native commitment/history owner; native station/carrier packets remain the only physical stock. */
SurfaceSupplyOrderSnapshot CaptureSurfaceSupplyOrders();

/** @return Next native monotonic intent identity; reading it issues/reserves nothing. */
SurfaceSupplyOrderID GetNextSurfaceSupplyOrderID();

/** Read a native retained intent without acquiring any spending/cargo right.
 * @param id Original retained identity; retired gaps are never a new order.
 * @return Ephemeral const view or nullptr; any native order mutation/reset invalidates the view, so callers retain IDs only.
 */
const SurfaceSupplyOrderRecord *GetSurfaceSupplyOrder(SurfaceSupplyOrderID id);

/** Read actual current-economy-day used/reserved allowance from the sole native book.
 * @param company Current original company/cap owner, including explicit takeover transfers.
 * @param label Canonical construction-supply meaning.
 * @return Complete allowance usage or unsupported query; no quote, reservation or expense is issued.
 */
std::optional<uint32_t> GetSurfaceSupplyOrderDayUsage(CompanyID company, CargoLabel label);

/** @return Whether the whole current native book still joins its actual profile/map/company/Surface-station owners; no state is repaired or published. */
bool ValidateSurfaceSupplyOrderBook();

/** Original station owner's complete whole-purchase receiver.
 * @param station Actual revalidated receiving Surface freight facility.
 * @param cargo Resolved original cargo slot.
 * @param quantity Entire already-paid quantity; partial custody must refuse before mutation.
 * @param source Original non-paying company-supply provenance.
 * @return Original issued packet identity, including one retired by immediate native merging, or Invalid on unchanged refusal.
 */
using SurfaceSupplyStationReceiver = CargoPacketID (*)(StationID station, CargoType cargo, uint16_t quantity, Source source);

/** Revalidate paid pending commitments and dispatch whole quantities through the original station owner, without another expense or refund.
 * @param receive Original native station ingress; production supplies its real UpdateStationWaiting adapter, never a detached inventory.
 * The bounded book is traversed once per station acceptance interval, independent of station count. Only the original whole-custody receipt makes a commitment Fulfilled.
 */
void DispatchPendingSurfaceSupplyOrders(SurfaceSupplyStationReceiver receive);

/** Reconcile unshipped commitments when the original receiving station changes or disappears.
 * @param station Original native identity whose facility/anchor/ownership just changed.
 * @param removed True during ordinary station destruction; pool/world cleanup never invokes this refund boundary.
 * Only invalid pending receivers refund their current real owner once through native Property accounting. Fulfilled stock/history is never refundable.
 */
void ReconcileSurfaceSupplyStation(StationID station, bool removed = false);

/** Follow actual native company takeover/loss and retire original buyer provenance before CompanyID reuse.
 * @param old_owner Actual retiring company's original identity.
 * @param new_owner Actual takeover successor, or Invalid for loss without refund. Takeover calls after native station ownership settles; loss calls before demolition.
 * Pending rights/caps follow the successor, while original purchase history and non-paying packet purpose remain intact.
 */
void ChangeSurfaceSupplyOrderCompanyOwnership(CompanyID old_owner, CompanyID new_owner);

/** Clear only unpublished full-load transport/clock evidence; copied preview owns separate same-file data. */
void ResetSurfaceSupplyOrderLoadStaging();

/** Stage one bounded native family without adopting orders, stock, money or cap rights.
 * @param snapshot Candidate file's complete original metadata.
 * @param saved_day Candidate family's original economy-day stamp, later joined to its actual DATE field.
 * @return Whether a unique bounded family was staged; duplicate or invalid-frontier transport leaves the installed book unchanged.
 */
bool StageLoadedSurfaceSupplyOrders(SurfaceSupplyOrderSnapshot snapshot, TimerGameEconomy::Date saved_day);

/** Retain the original full loader's actual DATE economy field as independent same-file evidence.
 * @param day Economy date just read by the native DATE handler, never copied from a calendar/live-world fallback.
 * @return Whether this is the first actual DATE observation; repeated evidence is corrupt transport.
 */
bool ObserveLoadedSurfaceSupplyEconomyDate(TimerGameEconomy::Date day);

/** Retain actual full MAPS evidence after native decoding, without allocating or changing geometry here.
 * @param width Original native plane width just read by MAPS.
 * @param height Original native plane height just read by MAPS.
 * @param layers Original native stored layer count just read by MAPS.
 * @return Whether this is the first valid actual MAPS observation; repeated/invalid evidence cannot lend another file geometry.
 */
bool ObserveLoadedSurfaceSupplyMap(uint32_t width, uint32_t height, uint8_t layers);

/** Adopt the whole staged original book only after native profile/map/company/station/date reconstruction.
 * @param family_required Native save-era requirement; genuine older absence supplies an empty book only.
 * @return Complete adoption or unchanged installed metadata on refusal; loading never creates a packet, charge, refund or dispatch tick.
 */
bool AcceptLoadedSurfaceSupplyOrders(bool family_required);

/** Validate the copied candidate's complete original order family against its own native DATE/MAPS/PLYR/STNN facts, without reading/installing the active world's book or stock. */
void ValidateSurfaceSupplyOrderLoadPreview();

#endif /* SURFACE_SUPPLY_ORDER_RUNTIME_H */
