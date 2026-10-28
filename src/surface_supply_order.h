/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order.h Detached paid-order metadata and original-owner admission inputs.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native stock/ownership invariants.
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
#include <variant>
#include <functional>
#include <set>

static constexpr size_t MAX_SURFACE_SUPPLY_RECORDS = 16384; ///< Existing new-purchase admission and original A440 transport bound; never enlarged by legacy import.
static constexpr size_t MAX_LEGACY_SURFACE_SUPPLY_RECORDS = 65535; ///< Complete retained legacy/mixed history bound; additional purchases still obey the16384 policy.
using SurfaceSupplyOrderID = uint64_t; ///< Monotonic intent/receipt identity; zero is invalid and retired IDs are never reissued.

/** Purchase commitments and irreversible receipts; native consumers separately own later material consumption. */
enum class SurfaceSupplyOrderState : uint8_t {
	Pending, ///< Paid but not dispatched; no usable native packet exists and cancellation may refund once.
	Fulfilled, ///< Original native station took physical custody once; later shipment/consumption can never refund this order.
	Cancelled, ///< Unshipped commitment was released and its original charge refunded once; it owns no usable packet.
	Lost, ///< Original company disappeared before dispatch; no active refund/stock right survives.
};

/** Native A evidence; only this receipt can hold a new refundable Pending commitment. */
struct NativeSupplyReceipt {
	CompanyID purchaser = CompanyID::Invalid(); ///< Original native payer, retained as history even if its pool slot later retires.
	uint16_t origin_x = 0, origin_y = 0; ///< Original Surface-local coordinates from native layer helpers, never a hidden destination or plane offset.
	Money cost{0}; ///< Actual frozen whole charge; cancellation can return this amount only before native dispatch.
	uint64_t inflation_prices = 0; ///< Original native Q16 price factor; current inflation cannot reprice a saved refund.
	TimerGameCalendar::Year purchase_year{0}; ///< Original calendar availability gate, independent of economy-day caps.
	SourceID packet_buyer = Source::Invalid; ///< Original payer provenance or retired sentinel; neither determines current container ownership.
	SurfaceSupplyOrderState state = SurfaceSupplyOrderState::Pending; ///< Pending commitment or irreversible native settlement history.
	CargoPacketID issued_packet = CargoPacketID::Invalid(); ///< Fulfilled issuance history only; splitting/merging/consumption may retire this index, so it grants no live packet/refund right.
	/** @param other Original native evidence. @return Exact scalar equality, with no owner/packet lookup. */
	bool operator==(const NativeSupplyReceipt &other) const = default;
};

/** Imported B evidence; original payer, inflation, lost anchor and issuance packet are structurally unavailable. */
struct TableSupplyReceipt {
	Money unit_price{0}; ///< Exact saved positive tariff, never reverse-engineered into native Q16 inflation.
	int32_t calendar_date = -1; ///< Actual original calendar date, independent of economy-day allowance.
	uint16_t refunded = 0; ///< Units already removed and credited; never usable cargo or renewed cap.
	uint16_t dispatched = 0; ///< Units whose first departure ended legacy refund rights permanently.
	bool closed = false; ///< Original cancellation/source/company loss; an open legacy receipt is never native Pending.
	/** @param other Exact original counters/tariff/date. @return Equality without consulting today's price or stock. */
	bool operator==(const TableSupplyReceipt &other) const = default;
};

/** One identity in the sole sorted book; the tagged receipt is history, never another inventory. */
struct SurfaceSupplyOrderRecord {
	SurfaceSupplyOrderID id = 0; ///< Original monotonically issued identity; B uint32 IDs widen without remapping.
	CompanyID owner = CompanyID::Invalid(); ///< Current rights/cap owner, distinct from an original native payer.
	StationID origin = StationID::Invalid(); ///< Exact source, or B's explicit source-loss sentinel; no reused-slot claim.
	CargoLabel label{}; ///< Stable canonical material, independent of a runtime cargo slot.
	uint16_t quantity = 0; ///< Whole original purchase, never remaining physical stock.
	TimerGameEconomy::Date purchase_day{0}; ///< Original economy cap day.
	std::variant<std::monostate, NativeSupplyReceipt, TableSupplyReceipt> receipt{NativeSupplyReceipt{}}; ///< Unknown transport tags become invalid monostate, never an inferred native receipt.

	/** @return Native evidence only, or nullptr; callers must never fabricate it for a legacy receipt. */
	NativeSupplyReceipt *Native() { return std::get_if<NativeSupplyReceipt>(&this->receipt); }
	/** @return Read-only native evidence, or nullptr for legacy/invalid provenance. */
	const NativeSupplyReceipt *Native() const { return std::get_if<NativeSupplyReceipt>(&this->receipt); }
	/** @return Original legacy evidence only, or nullptr; reading confers no refund or stock permission. */
	TableSupplyReceipt *Legacy() { return std::get_if<TableSupplyReceipt>(&this->receipt); }
	/** @return Read-only legacy evidence, or nullptr for native/invalid provenance. */
	const TableSupplyReceipt *Legacy() const { return std::get_if<TableSupplyReceipt>(&this->receipt); }
	/** @param other Complete tagged receipt. @return Equality of identity/provenance/history, not live custody. */
	bool operator==(const SurfaceSupplyOrderRecord &other) const = default;
};

/** Complete bounded order-book transport; publishing this value alone never installs a runtime owner. */
struct SurfaceSupplyOrderSnapshot {
	SurfaceSupplyOrderID next_id = 1; ///< Next never-before-issued intent; gaps in retained terminal history cannot reopen older identities.
	std::vector<SurfaceSupplyOrderRecord> records; ///< Strict original identity order; native owner must retain all pending and current-day charge-bearing records.
	/** @param other Complete detached history. @return Exact unchanged-input comparison; neither book is installed. */
	bool operator==(const SurfaceSupplyOrderSnapshot &other) const = default;
};

/** Fingerprint the sole admitted tagged book, preserving unavailable legacy facts structurally.
 * @param snapshot Complete canonical receipt history in issued-identity order.
 * @return Portable field-wise diagnostic over issuer, common facts and the exact Native/LegacyBvariant; physical stock remains in the packet owner.
 */
uint64_t ComputeSurfaceSupplyOrderDigest(const SurfaceSupplyOrderSnapshot &snapshot);

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

/** Find one exact identity in an already validated sorted canonical book.
 * @param snapshot Detached or installed complete history; no ordinal/id-minus-one indexing is permitted.
 * @param id Original uint64 identity, including losslessly widened B IDs.
 * @return Borrowed exact row or nullptr for an unissued/retired identity; mutation invalidates the view.
 */
const SurfaceSupplyOrderRecord *FindSurfaceSupplyOrderRecord(const SurfaceSupplyOrderSnapshot &snapshot, SurfaceSupplyOrderID id);

/** Find a mutable row using the same canonical exact-identity search.
 * @param snapshot Actual mutable book owned by the caller; this function does not create another store.
 * @param id Exact original identity to find, never a vector position.
 * @return Borrowed row or nullptr; caller still owns validation and settlement authority.
 */
SurfaceSupplyOrderRecord *FindSurfaceSupplyOrderRecord(SurfaceSupplyOrderSnapshot &snapshot, SurfaceSupplyOrderID id);

/** Count used/reserved daily allowance from an already admitted sole order-book snapshot, across every origin.
 * @param snapshot Previously admitted bounded complete records; native owner must not prune current-day or pending commitments.
 * @param company Current native cap owner, including explicitly transferred takeover commitments.
 * @param label Canonical construction-supply meaning.
 * @param day Actual economy day to count, independent of calendar technology dates.
 * @return Whole native Pending/Fulfilled purchases plus every matching original legacy purchase, or no value for unsupported input; no purchase is authorized.
 */
std::optional<uint32_t> CountSurfaceSupplyDayUsage(const SurfaceSupplyOrderSnapshot &snapshot, CompanyID company, CargoLabel label, TimerGameEconomy::Date day);

/** Check one actual first-departure amount against an already admitted canonical legacy receipt.
 * @param snapshot Complete sorted book; this predicate neither publishes it nor requires current-world defaults.
 * @param id Exact legacy identity; native, unknown and zero identities confer no legacy dispatch history.
 * @param quantity Nonzero actual units about to leave original refundable custody.
 * @return Whether the whole amount fits purchased minus refunded minus previously dispatched units, without mutation.
 */
bool CanRecordSurfaceSupplyDispatch(const SurfaceSupplyOrderSnapshot &snapshot, SurfaceSupplyOrderID id, uint16_t quantity);

/** Record a preflighted first departure against the same finite canonical history.
 * @param snapshot Sole caller-owned mutable canonical book, never a second live order store.
 * @param id Exact original legacy receipt identity.
 * @param quantity Actual once-only departing units; zero or excess refuses before changing any counter.
 * @return Complete checked counter update or unchanged refusal. No packets, money, daily allowance or closure change.
 * Closed/company-loss history may record a final real departure; closure never creates more finite entitlement.
 */
bool RecordSurfaceSupplyDispatch(SurfaceSupplyOrderSnapshot &snapshot, SurfaceSupplyOrderID id, uint16_t quantity);

/** Immutable scalars from one original native or same-file copied packet row; no container is inferred here. */
struct SurfaceSupplyPacketFacts {
	CargoPacketID packet = CargoPacketID::Invalid(); ///< Exact original allocated identity, checked against the lookup request.
	uint16_t quantity = 0; ///< Actual positive physical units, never a receipt-derived replacement amount.
	Source source{Source::Invalid, SourceType::Industry}; ///< Original canonical purpose after any explicitly admitted legacy wire conversion.
	Money feeder_share{0}; ///< Actual original feeder credit; paid CompanySupply must remain non-paying.
	SurfaceSupplyOrderID order = 0; ///< Optional exact legacy link; native A supplies legitimately retain zero.
	bool shipped = false; ///< Original irreversible first-departure evidence, not inferred from this lookup.
	/** @param other Original packet scalars. @return Exact unchanged-input equality, without resolving an owner. */
	bool operator==(const SurfaceSupplyPacketFacts &other) const = default;
};

/** Resolve one identity against the same immutable candidate throughout ownership and allocated-row observation.
 * @param packet Original allocated identity to resolve, never an ordinal.
 * @return Exact original row or absence; copied adapters must never substitute the active world's packet pool.
 */
using SurfaceSupplyPacketLookup = std::function<std::optional<SurfaceSupplyPacketFacts>(CargoPacketID packet)>;

/** Shared scalar/reference admission followed by streaming custody over one canonical book and immutable candidate facts.
 * Keeps only transient identity/completeness evidence and per-receipt totals; it owns no packets, money, caps or live book.
 * Ordinary source/reference/orphan rules remain with their existing owners; this checker never creates ordinary custody from allocation.
 */
class SurfaceSupplyCustodyCheck {
	const SurfaceSupplyOrderSnapshot &snapshot; ///< Original validated book, required to remain alive and unchanged throughout this check.
	const SurfaceSupplyRecordContext &context; ///< Same candidate's original profile/map/company/station facts; never an active-world substitute.
	SurfaceSupplyPacketLookup lookup; ///< One original packet source shared by both passes; no mutable or live fallback is allowed.
	size_t expected_packets; ///< Independently known complete original pool/table cardinality, not a count invented by visited owners.
	size_t allocated_packets = 0; ///< Actual unique ordered original rows observed after owner traversal.
	CargoPacketID previous_packet = CargoPacketID::Invalid(); ///< Enforces strict original allocated-ID ordering and excludes duplicate final observations.
	std::set<CargoPacketID> pending_owned; ///< Visited actual identities not yet matched by the final original allocated-row pass.
	std::vector<uint32_t> outstanding, unshipped; ///< Finite physical totals by retained row position found through exact ID lookup, never id-minus-one.
	bool allocated_phase = false; ///< Once final enumeration starts, a late invented owner cannot repair an earlier orphan.
	bool valid = true; ///< Any refusal is sticky and cannot be repaired by later favorable observations.
	/** @return False after irreversibly recording this check's refusal; original inputs remain untouched. */
	bool Refuse() { this->valid = false; return false; }
	/** Read and check canonical original packet scalars without assigning them a container.
	 * @param id Exact original identity to resolve.
	 * @return Valid original facts or absence with sticky refusal; no producer or live-state fallback runs here.
	 */
	std::optional<SurfaceSupplyPacketFacts> LookupPacket(CargoPacketID id);
public:
	/** Start a bounded check through the existing receipt validator, without publishing any candidate.
	 * @param snapshot Complete sorted canonical history, immutable until this check is destroyed.
	 * @param context Same candidate's original reference facts, immutable for both admission and later queue checks.
	 * @param lookup Stable read-only lookup into this candidate's original packet pool/table.
	 * @param packet_count Independent complete original allocated cardinality; excludes premature success after an omitted pool pass.
	 */
	SurfaceSupplyCustodyCheck(const SurfaceSupplyOrderSnapshot &snapshot, const SurfaceSupplyRecordContext &context, SurfaceSupplyPacketLookup lookup, size_t packet_count);
	/** Observe one actual container reference before the final allocated-row pass.
	 * @param packet Original native or same-file packet identity held by this container.
	 * @param label Actual container's resolved canonical cargo meaning, never inferred from the receipt.
	 * @param station Actual goods-queue identity, or Invalid for carriers, compartments and jobs.
	 * @return Whole reference/finite-history agreement or sticky refusal; no input is changed.
	 */
	bool AddOwned(CargoPacketID packet, CargoLabel label, StationID station);
	/** Observe every original allocated row once in strictly increasing identity order after all real containers.
	 * @param packet Actual original pool/table row identity, never a fabricated owner reference.
	 * @return Whether this row matches ownership/purpose policy; paid orphans refuse, ordinary orphan policy stays with native owners.
	 */
	bool ObserveAllocated(CargoPacketID packet);
	/** @return True only for an unrefused complete original enumeration with every visited owner matched; this grants no publication or refund. */
	bool Valid() const { return this->valid && this->allocated_packets == this->expected_packets && this->pending_owned.empty(); }
};

#endif /* SURFACE_SUPPLY_ORDER_H */
