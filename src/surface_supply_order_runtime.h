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

/** @return Whether the whole current book joins actual profile/map/company/station facts and complete native packet custody; no state is repaired or published. */
bool ValidateSurfaceSupplyOrderBook();

/** Read-only cancellation facts from the sole canonical owner, never a second entitlement or balance. */
struct LegacySurfaceSupplyCancellation {
	SurfaceSupplyOrderID order = 0; ///< Exact original legacy receipt; no vector ordinal or native issuance link.
	StationID station = StationID::Invalid(); ///< Actual surviving owned Surface source, including a sign whose freight berth was removed.
	CargoType cargo = INVALID_CARGO; ///< Actual canonical material slot resolved from the original receipt label.
	uint16_t quantity = 0; ///< Complete currently eligible never-shipped source units; returned/reserved stock is excluded.
	Money refund{0}; ///< Exact eligible quantity times saved B tariff, with wallet overflow refused before any physical removal.
	/** @param other Original observed cancellation facts. @return Exact display/command snapshot equality, conferring no execution right. */
	bool operator==(const LegacySurfaceSupplyCancellation &other) const = default;
};

/** Observe complete current legacy cancellation admission without changing stock, history, caps, cash or windows.
 * @param intent Exact original receipt ID, resolved in the sole installed canonical book.
 * @return Whole current facts or refusal; current actor must own the open legacy receipt and its actual original Surface source.
 * Original saved tariff and complete native custody are authoritative. This quote never calls a donor settlement helper or uses current prices.
 */
std::optional<LegacySurfaceSupplyCancellation> PrepareLegacySurfaceSupplyCancellation(SurfaceSupplyOrderID intent);

/** Preflight a first departure against the sole installed receipt owner, without changing any owner.
 * @param id Exact original legacy receipt identity; unknown/native IDs refuse.
 * @param quantity Whole selected units, including aggregate fragments during a hub's whole-move preflight.
 * @return Whether the current original finite remainder covers the amount; no reservation or settlement occurs.
 */
bool CanRecordSurfaceSupplyDispatch(SurfaceSupplyOrderID id, uint16_t quantity);

/** Record actual first departure in the sole installed book after physical movement admission.
 * @param id Exact original legacy identity, resolved through the shared uint64 lookup.
 * @param quantity Actual newly departing units, never all original packet units before a partial split.
 * @return Complete counter update or unchanged refusal; the caller sets its packet flag only on success.
 */
bool RecordSurfaceSupplyDispatch(SurfaceSupplyOrderID id, uint16_t quantity);

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
 * Invalid native Pending receivers refund their current real owner once through Property accounting. Legacy source loss closes without credit;
 * its still-owned Surface sign retains original cancellation eligibility after berth removal. Native Fulfilled history never regains a refund.
 */
void ReconcileSurfaceSupplyStation(StationID station, bool removed = false);

/** Follow actual native company takeover/loss and retire original buyer provenance before CompanyID reuse.
 * @param old_owner Actual retiring company's original identity.
 * @param new_owner Actual takeover successor, or Invalid for loss without refund. Takeover calls after native station ownership settles; loss calls before demolition.
 * Native Pending rights and all legacy history/caps follow the successor. Legacy loss closes and clears its source before demolition without credit;
 * original native payer history, unavailable legacy buyer evidence and permanent non-paying packet purpose remain intact.
 */
void ChangeSurfaceSupplyOrderCompanyOwnership(CompanyID old_owner, CompanyID new_owner);

/** Clear only unpublished full-load transport/clock evidence; copied preview owns separate same-file data. */
void ResetSurfaceSupplyOrderLoadStaging();

/** Stage one bounded native family without adopting orders, stock, money or cap rights.
 * @param snapshot Candidate file's complete original metadata.
 * @param saved_day Candidate family's original economy-day stamp, later joined to its actual DATE field.
 * @param fixed_wire True only for original A440 RIFF bytes, requiring the detached A adapter after same-file references load; canonical tagged callers keep false.
 * @return Whether a unique family within65535 total/16384 native rows was staged; duplicate/invalid-frontier transport leaves the installed book unchanged.
 */
bool StageLoadedSurfaceSupplyOrders(SurfaceSupplyOrderSnapshot snapshot, TimerGameEconomy::Date saved_day, bool fixed_wire = false);

/** Stage the original B header without creating a live receipt, clock stamp or packet.
 * @param schema Original SSOH schema, exactly1.
 * @param count Original retained row count, at most65535.
 * @param next_order Original never-issued frontier, exactly count+1.
 * @return Unique bounded unpublished evidence; duplicate/mixed transport refuses without replacing installed rights.
 */
bool StageLoadedTableSupplyHeader(uint32_t schema, uint32_t count, uint32_t next_order);

/** Stage only B's original bounded TABLE rows; reference/clock/custody joins still precede adoption.
 * @param rows Complete original row order, never a native stock store or decoded active-world fallback.
 * @return Unique bounded unpublished transport; missing/mismatched header or semantics still refuse at whole-file acceptance.
 */
bool StageLoadedTableSupplyRows(std::vector<struct TableSupplyOrderWire> rows);

/** Retain proof that this full file's original CAPA order/shipped columns and raw packet purpose were decoded.
 * @return First complete original packet-field observation; duplicates refuse. This is not complete custody or publication authority.
 */
bool ObserveLoadedTableSupplyPacketFields();

/** Retain this original B file's explicit MLMD paid-history bit independently of its book rows.
 * @param present Exact original declaration, including false for a legitimately empty history.
 * @return First metadata observation; duplicate facts refuse and no receipt/clock/packet is created.
 */
bool ObserveLoadedTableSupplyHistory(bool present);

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

/** Adopt the whole staged original book only after native profile/map/company/station/date reconstruction and complete physical custody validation.
 * @param family_required Native save-era requirement; genuine older absence supplies an empty book only.
 * @return Complete adoption or unchanged installed metadata on refusal; loading never creates a packet, charge, refund or dispatch tick.
 */
bool AcceptLoadedSurfaceSupplyOrders(bool family_required);

/** Validate the copied candidate's complete original order family against its own native DATE/MAPS/PLYR/STNN facts, without reading/installing the active world's book or stock. */
void ValidateSurfaceSupplyOrderLoadPreview();

#endif /* SURFACE_SUPPLY_ORDER_RUNTIME_H */
