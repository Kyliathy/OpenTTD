/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply.h Pure admission for the canonical paid Surface construction seed.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13C.1.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15B; missing bootstrap prerequisite only.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB; shared canonical scope/frozen pricing.
 * Quotes create no order, packet, reservation, expense or usable inventory.
 */
#ifndef SURFACE_SUPPLY_H
#define SURFACE_SUPPLY_H

#include "multilayer_content.h"
#include "station_type.h"
#include "company_type.h"
#include "cargo_type.h"
#include "economy_type.h"
#include "map_layer.h"
#include "timer/timer_game_calendar.h"
#include <optional>

/** Canonical immutable seed definition, owned by multilayer_bootstrap.h. */
struct MultiLayerSurfaceSupply;

/** Resolve only the four scheduled construction supplies in the original content catalogue.
 * @param label Stable STEL/RFMT/ELEC/BATY meaning; no runtime cargo ordinal is registered here.
 * @return Original immutable definition, or nullptr for an unsupported supply.
 */
const MultiLayerSurfaceSupply *GetSurfaceSupplyDefinition(CargoLabel label);

/** Calculate the complete frozen native charge without consulting mutable company/stock/registry owners.
 * @param label Original canonical construction-supply meaning.
 * @param quantity Positive whole requested units.
 * @param inflation_prices Original native Q16 price factor at quotation.
 * @return Checked complete charge, or no value for unsupported/zero/invalid/overflowing inputs; no spending authority.
 */
std::optional<Money> CalculateSurfaceSupplyCost(CargoLabel label, uint16_t quantity, uint64_t inflation_prices);

/** One requested whole purchase; the eventual command must revalidate its actual owners at execution. */
struct SurfaceSupplyOrderRequest {
	StationID origin = StationID::Invalid(); ///< Actual receiving Surface station; never a hidden construction destination.
	CompanyID buyer = CompanyID::Invalid(); ///< Actual company whose money and daily allowance finance the purchase.
	CargoLabel label{}; ///< Stable canonical STEL/RFMT/ELEC/BATY meaning, resolved in this world's selected registry.
	uint16_t quantity = 0; ///< Entire requested quantity, never a partial-fill instruction.
};

/** Read-only admission inputs; this detached value owns no quota, money, packets or station state.
 * Native capture reads original owners. Tests may declare detached boundaries,
 * but neither a view nor a successful quote authorizes settlement without recapture.
 */
struct SurfaceSupplyOrderView {
	MultiLayerGameplayProfileID profile = MultiLayerGameplayProfileID::Classic; ///< Actual selected gameplay profile, independent of camera/editor override.
	bool company_exists = false; ///< Same-world native buyer exists.
	bool station_exists = false; ///< Same-world native station exists.
	Owner station_owner = OWNER_NONE; ///< Native station owner, never inferred from catchment or a requested company.
	MapLayer station_layer = MapLayer::Surface; ///< Actual station anchor's physical layer.
	bool loading_station = false; ///< Actual Surface rail or lorry loading facility; bus/airport/launch-only facilities cannot supply this service.
	CargoType cargo = INVALID_CARGO; ///< Actual label resolution in the current profile; no local cargo ordinal registry.
	TimerGameCalendar::Year year{0}; ///< Actual native calendar gate.
	Money available_money{0}; ///< Existing liquid company funds; an unborrowed loan limit is not cash.
	uint32_t station_quantity = 0; ///< Original cargo list's total count, including reserved stock.
	uint32_t company_quantity_today = 0; ///< Sole producer's already used/reserved same-economy-day allowance across all supply stations.
	uint64_t inflation_prices = 0; ///< Existing economy owner's Q16 price factor.
	bool packet_room = false; ///< Actual native packet pool can admit the one whole purchase packet.
};

/** Exact first deterministic refusal; no failure reserves allowance, allocates cargo or debits money. */
enum class SurfaceSupplyOrderError : uint8_t {
	None, Profile, Company, Station, Cargo, Quantity, Date, DailyLimit, StationSpace, PacketSpace, Price, Funds,
};

/** Detached complete quote; it has no saved order identity or transferable spending authority. */
struct SurfaceSupplyOrderQuote {
	SurfaceSupplyOrderError error = SurfaceSupplyOrderError::Cargo; ///< None only when every original admission family agrees.
	CargoType cargo = INVALID_CARGO; ///< Resolved actual cargo meaning on success.
	uint16_t quantity = 0; ///< Entire admitted requested quantity on success.
	Money cost{0}; ///< Complete inflation-adjusted native Money charge, not a freight payment.
};

/** Evaluate a complete detached admission snapshot without touching native owners.
 * @param request Original company/station/label/whole-quantity request.
 * @param view Read-only original owner snapshot; declarations remain component-test inputs, not procurement acceptance.
 * @return Complete cost and cargo, or an unchanged deterministic refusal with no partial quote.
 */
SurfaceSupplyOrderQuote PrepareSurfaceSupplyOrder(const SurfaceSupplyOrderRequest &request, const SurfaceSupplyOrderView &view);

/** Read actual native station/company/economy owners without creating cargo data or changing any account.
 * @param request Requested real station/company/canonical label and complete quantity.
 * @param company_quantity_today Read-only count supplied by the sole same-day quota producer; this adapter owns no replacement cap ledger.
 * @return Detached actual native admission view; absence/foreign/wrong-layer owners remain explicit refusals.
 */
SurfaceSupplyOrderView CaptureSurfaceSupplyOrderView(const SurfaceSupplyOrderRequest &request, uint32_t company_quantity_today);

#endif /* SURFACE_SUPPLY_H */
