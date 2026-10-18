/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order.h Detached paid-order metadata and original-owner admission inputs.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB.
 * Values here grant no live order, payment, packet, reservation, refund or saved-state activation.
 */
#ifndef SURFACE_SUPPLY_ORDER_H
#define SURFACE_SUPPLY_ORDER_H

#include "surface_supply.h"
#include "cargopacket.h"
#include "timer/timer_game_economy.h"
#include <array>
#include <span>
#include <vector>

static constexpr size_t MAX_SURFACE_SUPPLY_RECORDS = 16384; ///< Hard detached/live metadata bound; physical cargo remains exclusively in native containers.
using SurfaceSupplyOrderID = uint64_t; ///< Monotonic intent/receipt identity; zero is invalid and retired IDs are never reissued.

/** Purchase commitments and irreversible receipts; native consumers separately own later material consumption. */
enum class SurfaceSupplyOrderState : uint8_t {
	Pending, ///< Paid but not dispatched; no usable native packet exists and cancellation may refund once.
	Fulfilled, ///< Original native station took physical custody once; later shipment/consumption can never refund this order.
	Cancelled, ///< Unshipped commitment was released and its original charge refunded once; it owns no usable packet.
	Lost, ///< Original company disappeared before dispatch; no active refund/stock right survives.
};

/** One immutable purchase intent plus current commitment/terminal history; quantities here are receipts, never another inventory. */
struct SurfaceSupplyOrderRecord {
	SurfaceSupplyOrderID id = 0; ///< Original monotonically issued intent identity.
	CompanyID purchaser = CompanyID::Invalid(); ///< Original native payer, retained as history even if its pool slot later retires.
	CompanyID owner = CompanyID::Invalid(); ///< Current pending-right/cap owner; takeover may move this independently of original payment provenance.
	StationID origin = StationID::Invalid(); ///< Original receiving Surface station; terminal history grants no authority over a reused slot.
	uint16_t origin_x = 0, origin_y = 0; ///< Original Surface-local coordinates from native layer helpers, never a hidden destination or plane offset.
	CargoLabel label{}; ///< Stable canonical meaning; no runtime cargo registration is performed by these records.
	uint16_t quantity = 0; ///< Entire originally admitted purchase, not currently available stock.
	Money cost{0}; ///< Actual frozen whole charge; cancellation can return this amount only before native dispatch.
	uint64_t inflation_prices = 0; ///< Original native Q16 price factor; current inflation cannot reprice a saved refund.
	TimerGameCalendar::Year purchase_year{0}; ///< Original calendar availability gate, independent of economy-day caps.
	TimerGameEconomy::Date purchase_day{0}; ///< Actual original economy day shared by this owner's orders at every origin.
	SourceID packet_buyer = Source::Invalid; ///< Original payer provenance or retired sentinel; neither determines current container ownership.
	SurfaceSupplyOrderState state = SurfaceSupplyOrderState::Pending; ///< Pending commitment or irreversible native settlement history.
	CargoPacketID issued_packet = CargoPacketID::Invalid(); ///< Fulfilled issuance history only; splitting/merging/consumption may retire this index, so it grants no live packet/refund right.
};

/** Complete bounded order-book transport; publishing this value alone never installs a runtime owner. */
struct SurfaceSupplyOrderSnapshot {
	SurfaceSupplyOrderID next_id = 1; ///< Next never-before-issued intent; gaps in retained terminal history cannot reopen older identities.
	std::vector<SurfaceSupplyOrderRecord> records; ///< Strict original identity order; native owner must retain all pending and current-day charge-bearing records.
};

/** Candidate-owned station facts; full/copy adapters must read original native fields rather than borrow the running world's station. */
struct SurfaceSupplyStationView {
	StationID station = StationID::Invalid(); ///< Actual normal-station identity, excluding waypoints.
	Owner owner = OWNER_NONE; ///< Original native current ownership.
	uint16_t x = 0, y = 0; ///< Surface-local native coordinates associated with this exact station.
	MapLayer layer = MapLayer::Surface; ///< Actual physical layer, independent of camera/catchment.
	bool freight_facility = false; ///< Actual Train/TruckStop facility, not airport/bus/launch-only presentation.
};

/** Detached candidate-world reference context; constructing a view allocates no company, station, cash or cargo. */
struct SurfaceSupplyRecordContext {
	MultiLayerGameplayProfileID profile = MultiLayerGameplayProfileID::Classic; ///< Candidate's actual accepted profile.
	uint32_t width = 0, height = 0; ///< Native map owner's Surface-local dimensions; no local plane geometry is introduced.
	std::array<bool, MAX_COMPANIES> companies{}; ///< Candidate's original PLYR presence, never active-world company lookup.
	std::span<const SurfaceSupplyStationView> stations; ///< Strict same-file normal-station identity order, read-only.
};

/** First deterministic complete-record refusal; a failure grants no partial state or reservation. */
enum class SurfaceSupplyRecordError : uint8_t {
	None, Size, Identity, Profile, Geometry, Company, Station, Cargo, Quantity, Date, Price, State, Packet,
};

/** Validate the whole detached order book against its own native reference context.
 * @param snapshot Complete candidate commitments/history and monotonic issuance frontier.
 * @param context Candidate-owned profile/map/company/normal-station facts; no live-world fallback is permitted.
 * @return None only for the whole admitted value; neither success nor failure publishes state or spends money.
 */
SurfaceSupplyRecordError ValidateSurfaceSupplyRecords(const SurfaceSupplyOrderSnapshot &snapshot, const SurfaceSupplyRecordContext &context);

/** Count used/reserved daily allowance from an already admitted sole order-book snapshot, across every origin.
 * @param snapshot Previously admitted bounded complete records; native owner must not prune current-day or pending commitments.
 * @param company Current native cap owner, including explicitly transferred takeover commitments.
 * @param label Canonical construction-supply meaning.
 * @param day Actual economy day to count, independent of calendar technology dates.
 * @return Complete pending/fulfilled quantity, or no value for oversized/unsupported query inputs; this is read-only and authorizes no purchase.
 */
std::optional<uint32_t> CountSurfaceSupplyDayUsage(const SurfaceSupplyOrderSnapshot &snapshot, CompanyID company, CargoLabel label, TimerGameEconomy::Date day);

#endif /* SURFACE_SUPPLY_ORDER_H */
