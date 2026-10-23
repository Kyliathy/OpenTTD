/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply.h Paid Surface seed admission and purchase history; native cargo packets own every physical unit.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups13CB-13CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef SURFACE_SUPPLY_H
#define SURFACE_SUPPLY_H

#include "cargo_type.h"
#include "company_type.h"
#include "economy_type.h"
#include "station_type.h"
#include "core/strong_typedef_type.hpp"
#include <vector>
#include <optional>
#include <span>

struct MultiLayerSurfaceSupply;

/** Monotonic purchase identity; zero means ordinary cargo, and retired identities are never reused. */
using SurfaceSupplyOrderID = StrongType::Typedef<uint32_t, struct SurfaceSupplyOrderIDTag, StrongType::Compare>;

/** Complete admission failures; no refusal authorizes a partial order or hidden stock. */
enum class SurfaceSupplyError : uint8_t { None, Profile, Company, Station, Supply, Date, Quantity, Funds, DailyCap, Stock, Pool, Limit, Stale, Corrupt, RefundLimit };

inline constexpr size_t MAX_SURFACE_SUPPLY_ORDERS = 65535; ///< Bounded complete purchase history/load work; identities are retained even after consumption/refund.

/** Retained purchase/refund history; actual station/vehicle packets exclusively own remaining material. */
struct SurfaceSupplyOrder {
	SurfaceSupplyOrderID id{0}; ///< Nonzero monotonic native order, also retained by its actual cargo packets.
	CompanyID owner = CompanyID::Invalid(); ///< Current company after a real takeover; invalid after loss without a successor.
	StationID station = StationID::Invalid(); ///< Original owned Surface pickup; removal never teleports outstanding cargo.
	CargoLabel label{}; ///< Immutable registered physical meaning, independent of a saved runtime slot.
	int32_t economy_day = 0; ///< Ordering day for the company-wide cap; delivery/refund never buys a fresh cap.
	int32_t calendar_date = 0; ///< Actual original purchase calendar date; later technology/date changes do not rewrite its historical admission.
	uint16_t quantity = 0; ///< Whole units originally paid for, including subsequently shipped or consumed material.
	Money unit_price = 0; ///< Actual inflation-adjusted purchase tariff, frozen for any permissible refund.
	uint16_t refunded = 0; ///< History only; every refunded unit must already have been removed from native unshipped stock.
	uint16_t dispatched = 0; ///< Historical units that left source custody once; returning/reserving/consuming them never renews refund rights.
	bool closed = false; ///< No further cancellation at the original pickup; shipped stock retains its purpose.
	/** @param other Complete immutable/current purchase history. @return Whether all paid/cap/refund fields agree. */
	bool operator==(const SurfaceSupplyOrder &other) const = default;
};

/** Pure complete admission snapshot; it holds no material, cap reservation or allocated order identity. */
struct SurfaceSupplyQuote {
	SurfaceSupplyError error = SurfaceSupplyError::None; ///< Exact refusal, or a complete whole order.
	CompanyID owner = CompanyID::Invalid(); ///< Actual command actor, never inferred from a view.
	StationID station = StationID::Invalid(); ///< Actual owned Surface freight facility.
	CargoLabel label{}; ///< One of the four selected construction seed labels.
	CargoType cargo = INVALID_CARGO; ///< Currently registered slot, re-resolved before commit.
	uint16_t quantity = 0; ///< Whole request; admission never silently truncates it.
	int32_t economy_day = 0; ///< Current synchronized ordering day.
	uint32_t day_used = 0; ///< Already paid company-wide units, not a second inventory.
	uint32_t waiting = 0; ///< Existing physical station queue plus native pickup reservations.
	SurfaceSupplyOrderID next_order{0}; ///< Expected cursor only; reading a quote does not allocate it.
	Money unit_price = 0; ///< Actual current tariff, checked again at settlement.
	Money cost = 0; ///< Complete debit; the purchase owner charges it once.
	/** @param other Newly recomputed complete snapshot. @return Whether every admission/cursor/price input still agrees. */
	bool operator==(const SurfaceSupplyQuote &other) const = default;
};

/** Detached save/validation transport; native packets exclusively retain material, while this copies paid history and its cursor. */
struct SurfaceSupplyState {
	uint32_t next_order = 1; ///< Next never-used identity; complete history retains every earlier purchase.
	std::vector<SurfaceSupplyOrder> orders; ///< Unpublished scalar/cap/refund history, never additional physical stock.
};

/** Pure cancellation admission; copied history/expected source units confer no cargo, refund or cap rights. */
struct SurfaceSupplyCancellation {
	SurfaceSupplyError error = SurfaceSupplyError::None; ///< Complete unchanged-state refusal or a whole cancellation.
	SurfaceSupplyOrder order; ///< Expected actual history, revalidated before native packet removal.
	uint16_t quantity = 0; ///< Expected never-dispatched units still physically in the original source queue.
	Money refund = 0; ///< Exact frozen-tariff credit after those actual units are removed; dispatched material gets none.
	/** @param other Fresh complete cancellation admission. @return Whether history/source quantity/credit remain exact. */
	bool operator==(const SurfaceSupplyCancellation &other) const = default;
};

/**
 * Quote a complete paid seed order without changing any station, packet, history, cap or money.
 * @param station Actual owned Surface railway or lorry station.
 * @param label Registered STEL/RFMT/ELEC/BATY meaning; later seeds retain their separate dispatch.
 * @param quantity Whole requested units; zero, over-cap and partial-space requests refuse.
 * @return Complete current admission and its unchanged-state refusal, including the expected order cursor.
 */
SurfaceSupplyQuote PrepareSurfaceSupplyOrder(StationID station, CargoLabel label, uint16_t quantity);

/**
 * Revalidate and settle one exact quote through actual packet/money owners.
 * @param quote Original pure complete quote; stale, edited or already settled quotes refuse unchanged.
 * @param execute False performs the identical admission without allocation, reservations or expense.
 * @return None only after complete admission; execution publishes one purchase and its native packet once.
 */
SurfaceSupplyError PlaceSurfaceSupplyOrder(const SurfaceSupplyQuote &quote, bool execute);

/** @param id Nonzero purchase identity. @return Existing retained history, or nullptr without allocating it. */
const SurfaceSupplyOrder *GetSurfaceSupplyOrder(SurfaceSupplyOrderID id);

/** @return Expected next order identity; a query allocates nothing and cannot reset history. */
SurfaceSupplyOrderID GetNextSurfaceSupplyOrderID();

/**
 * Read this service's canonical construction-seed terms without introducing a second price/cargo catalogue.
 * @param label Selected stable construction meaning; other profile seeds retain their later owners.
 * @return Existing immutable bootstrap terms, or nullptr when this service does not offer the label.
 */
const MultiLayerSurfaceSupply *GetSurfaceSupplyTerms(CargoLabel label);

/**
 * Read the actual current tariff even when today's quantity allowance is exhausted.
 * @param label Existing selected construction meaning.
 * @return Positive inflation-adjusted native Money, or absence for invalid terms/overflow; this grants no admission.
 */
std::optional<Money> GetSurfaceSupplyUnitPrice(CargoLabel label);

/**
 * Read retained purchase history without copying stock or reserving any entitlement.
 * @return Borrowed native rows, valid only until the next synchronized purchase/reset; callers must retain IDs instead of this view.
 */
std::span<const SurfaceSupplyOrder> GetSurfaceSupplyOrderHistory();

/** Clear the feature during normal world reset or isolated fixture teardown; never use to cancel a live purchase. */
void ResetSurfaceSupplyOrders();

/** @return Detached current scalar/cap/refund history; this grants no material, refund or new purchase identity. */
SurfaceSupplyState CaptureSurfaceSupplyState();

/**
 * Validate complete historical bounds and, when requested, every actual native packet/container reference.
 * @param state Detached candidate; validation never publishes, repairs or moves its material/history.
 * @param actual_references False checks independent save/preview structure; true also resolves actual companies/stations/cargo/packets.
 * @return None only when history/cursor and all requested real custody checks agree.
 */
SurfaceSupplyError ValidateSurfaceSupplyState(const SurfaceSupplyState &state, bool actual_references);

/** @param state Detached candidate retained until ordinary afterload has reconstructed all actual packet owners. */
void StageLoadedSurfaceSupplyState(SurfaceSupplyState state);

/** @return Whether complete staged history and actual native packet custody published unchanged; failure preserves existing live history. */
bool AcceptLoadedSurfaceSupplyState();

/** @return Stable explicit-field diagnostic over actual paid history and packet-purpose fields; never a stock or cap authority. */
uint64_t ComputeSurfaceSupplyDigest();

/** Clear independent unpublished full-load header/table transport on world/load reset; never renew a paid order. */
void ResetSurfaceSupplyLoadStaging();

/** Validate copied whole-family preview structure without borrowing or changing the current world's companies/packets. */
void ValidateSurfaceSupplyLoadPreview();

/**
 * Quote closing one actual owned order without touching its native stock or spent company-day allowance.
 * @param order Existing purchase identity, not a fresh quote/cursor allocation.
 * @return Exact original-source-only refund; a fully dispatched order can close with zero credit.
 */
SurfaceSupplyCancellation PrepareSurfaceSupplyCancellation(SurfaceSupplyOrderID order);

/**
 * Revalidate complete cancellation and remove actual eligible packets before crediting money once.
 * @param quote Current complete history/source-stock/refund admission; edited/stale/closed requests refuse.
 * @param execute False estimates without removal, credit, closure or cap mutation.
 * @return None only after complete admission; dispatched/consumed stock and all original cap history remain unchanged.
 */
SurfaceSupplyError CancelSurfaceSupplyOrder(const SurfaceSupplyCancellation &quote, bool execute);

/**
 * Retire references when an actual source station is destroyed; never grant a replacement source or an implicit refund.
 * @param station Actual disappearing native identity; ordinary station ownership remains responsible for its physical cargo-loss policy.
 */
void ForgetSurfaceSupplyStation(StationID station);

/**
 * Follow the actual native company event without moving packets or erasing historical spending/caps.
 * @param old_owner Original actual company whose items changed ownership.
 * @param new_owner Actual successor, or an invalid/non-company owner for loss; takeover combines both companies' paid day history.
 */
void ChangeSurfaceSupplyOwnership(Owner old_owner, Owner new_owner);

/**
 * Record the first actual native custody departure for one paid fragment, without moving or owning it.
 * @param order Actual packet's immutable purchase identity.
 * @param quantity Positive real fragment units; the packet's permanent dispatch flag prevents a second event.
 * @return Whether the exact funded, not-refunded and not-yet-dispatched historical allowance admitted this once-only event.
 */
bool RecordSurfaceSupplyDispatch(SurfaceSupplyOrderID order, uint16_t quantity);

#endif /* SURFACE_SUPPLY_H */
