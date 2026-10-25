/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power.h The one native utility identity/accounting owner, separating electrical and thermal quantities through frozen domains.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group7
 * @see docs/zz-rhr/upgrades/2026-10/r2mlvs1-g-content-freeze.md utility authority
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint9.1
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-v-utility-settlement-contract.md explicit publication/allocation/settlement/rotation order
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group2B typed shared heat before physical source/routing
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5 original paid electrical machinery references
 */

#ifndef UTILITY_POWER_H
#define UTILITY_POWER_H

#include "multilayer_utility.h"
#include "company_type.h"
#include "tile_type.h"
#include "town_type.h"
#include "station_type.h"
#include "industry_type.h"

static constexpr size_t MAX_UTILITY_POWER_CONNECTIONS = 16384; ///< Whole-ledger explicit-link cap keeps concrete save/load transport bounded.
static constexpr size_t MAX_UTILITY_POWER_SHARES = 16384; ///< Whole-ledger current slice cap, including later bounded transfers.
static constexpr size_t MAX_UTILITY_CONVERSION_ORIGINS = 16384; ///< Whole-ledger retained Heat/output source pairs; exhausted provenance refuses new use without pruning or refunds.
static constexpr size_t MAX_UTILITY_LINEAGE_LINKS = 65536; ///< Whole-epoch reconstructed physical-reference cap; parent graphs cannot hide unbounded traversal or file memory.

/** Shared per-epoch reservation-slice identity; this names an existing quantity's lineage, never another source or energy account. */
using UtilityShareID = StrongType::Typedef<uint32_t, struct UtilityShareIDTag, StrongType::Compare>;

/** Specific ledger refusal; failure leaves accounts, IDs, grants and fractional carry unchanged. */
enum class UtilityPowerError : uint8_t {
	None, UnsupportedProfile, InvalidOwner, InvalidLayer, Limit, InvalidReference, InvalidQuantity,
	InvalidInterval, DuplicatePublication, AlreadyAllocated, InvalidTopology,
	AllocationRequired,
	InsufficientGrant, StaleMutation, Refused,
};

/** Historical original thermal contribution to one exact electrical source; this attribution cannot be spent or reserved again. */
struct UtilityHeatConversionOrigin {
	UtilityProducerID producer{UINT32_MAX}; ///< Original nonreused native Heat source, including retired history.
	uint64_t heat_j = 0; ///< Lifetime actual Heat use attributed to this conversion output.
	/** @param other Candidate contribution. @return Exact original source and historical quantity equality. */
	bool operator==(const UtilityHeatConversionOrigin &other) const = default;
};

/** One native producer's immutable Heat/Electricity meter; its feature owns physical availability and generation inputs. */
struct UtilityPowerProducer {
	UtilityPowerAccount meter; ///< Frozen unit/interval/remainder fields, owned only by this ledger.
	CompanyID owner = CompanyID::Invalid(); ///< Actual company owning the source.
	UtilityGenerationKind kind = UtilityGenerationKind::External; ///< Explicit source family; imports never become generation.
	uint64_t reserved_j = 0; ///< This interval's quantity promised to consumers, distinct from settled actual use.
	bool active = true; ///< Retired source identities cannot publish again or silently bind a replacement.
	uint64_t generated_lifetime_j = 0; ///< Once-only whole generation across rotations; never increased by import or query.
	uint64_t consumed_lifetime_j = 0; ///< Settled original-source use survives rotation, loss and save/retry.
	UtilityConsumerID conversion_input{UINT32_MAX}; ///< Conversion-only original HeatGenerator consumer; absent for every earlier source kind.
	uint8_t conversion_efficiency = 0; ///< Immutable conversion percent1..100; ordinary source kinds retain zero.
	uint64_t conversion_capacity_w = 0; ///< Immutable actual rated electrical ceiling, checked for each synchronized publication duration.
	uint8_t conversion_remainder = 0; ///< Sole saved fractional-percent numerator0..99; distinct from the rate integrator's millijoule carry.
	std::vector<UtilityHeatConversionOrigin> conversion_origins; ///< Canonical lifetime original-source attribution, not another live energy balance.
};

/** Saved source slice of a consumer reservation; original, used and released quantities have one ledger owner. */
struct UtilityPowerShare {
	UtilityProducerID producer{UINT32_MAX}; ///< Exact original generation account, preserved across later settlement/transfer.
	uint64_t allocated_j = 0; ///< Once-only original interval reservation.
	uint64_t consumed_j = 0; ///< Settled actual use; source loss/expiry never refunds it.
	uint64_t released_j = 0; ///< Expired/lost unused quantity, never regenerated by a query.
	uint64_t transferred_j = 0; ///< Quantity moved to exact direct child slices; unavailable here and counted free only at its retained descendants.
	UtilityLinkID imported_link{UINT32_MAX}; ///< Last incoming native edge; genuine older generic imports retain their one-hop re-export refusal.
	UtilityShareID id{UINT32_MAX}; ///< Shared owner-issued identity within the exact accounting epoch; sorting cannot change a parent reference.
	UtilityShareID parent{UINT32_MAX}; ///< Exact preceding slice, or invalid for original local generation; children preserve their original producer.
	std::vector<UtilityLinkID> segment; ///< Only newly traversed installed cells/paid transitions; the original full path reconstructs through retained parents.
};

/** Immutable shared link role; local geometry never becomes a portal export or generation account. */
enum class UtilityLinkKind : uint8_t { ElectricalPortal, LocalCell, ElectricalRiser, ElectricalFeed };

/** Saved shared link identity; portal routes meter flow, while local cells retain installed rate/owner/domain without an energy balance. */
struct UtilityPowerLink {
	UtilityLinkID id{UINT32_MAX}; ///< Ledger-issued identity, retained after retirement and never reused.
	UtilityConsumerID source{UINT32_MAX}; ///< Portal source-side reservation; local physical cells have no energy participants.
	UtilityConsumerID destination{UINT32_MAX}; ///< Portal destination reservation; invalid for a local cell, never a new generator.
	TileIndex source_endpoint = INVALID_TILE; ///< Actual paired source endpoint or installed local cell, independent of camera geometry.
	TileIndex destination_endpoint = INVALID_TILE; ///< Reciprocal portal end or the same world cell for local infrastructure.
	uint64_t portal_revision = 0; ///< Portal commissioning incarnation; zero for a local physical cell.
	uint64_t capacity_j = 0; ///< Genuine older portal interval cap; zero for paid machinery/local cells, whose separate watt rates require an original published duration.
	uint64_t flow_j = 0; ///< Once-only portal flow; local cells own no transported/stored energy.
	bool active = true; ///< Lost/decommissioned routes cannot credit another usable import.
	UtilityLinkKind kind = UtilityLinkKind::ElectricalPortal; ///< Genuine older links retain their original commissioned export role.
	CompanyID local_owner = CompanyID::Invalid(); ///< Local-cell controller; portal ownership stays with its actual registered participants.
	UtilityEnergyDomain local_domain = UtilityEnergyDomain::Electricity; ///< Local-cell network; Heat never uses an electrical portal.
	uint64_t local_capacity_w = 0; ///< Installed local-cell rate, distinct from portal per-interval joules and generation.
	IndustryID riser = IndustryID::Invalid(); ///< Paid electrical machinery's original native identity; genuine older portal/local roles have none.
	uint64_t riser_capacity_w = 0; ///< Frozen paid electrical riser rate; the interval flow owner derives its joule opportunity, never a new generation account.
	MapLayer local_layer = MapLayer::DeepUnderground; ///< Explicit installed-cell plane, checked against native geometry; older Deep cells and nonlocal roles retain this default.
	IndustryID generator = IndustryID::Invalid(); ///< Original native generator for an ElectricalFeed; every earlier role retains Invalid, never borrowing an output identity.
};

/** One registered consumer and the explicit same-layer sources validated by its physical network owner. */
struct UtilityPowerConsumer {
	UtilityPowerAccount meter; ///< The canonical demand/grant/unmet meter; views never renew it.
	CompanyID owner = CompanyID::Invalid(); ///< Actual company owning the consumer.
	bool active = true; ///< Company removal retains settled identity but permanently revokes grants, eligibility and effects.
	UtilityConsumerEffect effect = UtilityConsumerEffect::ProcessorBoost; ///< Declared bounded native response, applied by later settlement.
	uint64_t capacity_j = 0; ///< Maximum grant/request in one interval.
	std::vector<UtilityProducerID> linked_producers; ///< Saved explicit eligibility; no catchment or camera inference.
	std::vector<UtilityPowerShare> shares; ///< Exact allocation lineage and settlement, separate from derived free-balance views.
	TownID town = TownID::Invalid(); ///< Explicit real Surface town for the town-growth consumer; never inferred from catchment or coordinates.
	uint8_t town_bonus_percent = 0; ///< Accepted bounded current-interval native bonus, applied once against normal growth.
	bool town_effect_applied = false; ///< One town effect settlement per interval; save/retry never reapplies it.
	bool town_effect_suspended = false; ///< Custom GS takeover suppresses this interval even after relinquishing custom growth.
	StationID town_terminal = StationID::Invalid(); ///< Explicit paid Surface distribution terminal; absent in genuine pre431 effect-only consumers.
	UtilityConsumerID town_supply{UINT32_MAX}; ///< Original Deep forwarding reservation, never a new generator or intermediate import account.
	TileIndex town_source_port = INVALID_TILE; ///< Original native powerhouse electrical boundary; shared paid routing discovers both crossings.
};

/** Bounded whole-ledger transport; versioned load acceptance validates every meter and reservation before publication. */
struct UtilityPowerState {
	uint64_t interval = 1; ///< Nonzero owner-issued accounting epoch, independent of render time.
	uint32_t next_producer = 0; ///< Retired IDs cannot be manufactured or reused by consumers.
	uint32_t next_consumer = 0; ///< Independent consumer identity allocator.
	bool allocated = false; ///< Exactly one electrical allocation closes electrical generation publication for this interval.
	bool heat_allocated = false; ///< Independent once-only thermal allocation closes heat publication; both domains retain the same owner-issued interval/IDs.
	uint64_t operation_sequence = 0; ///< Every accepted state change advances the saved retry/staleness identity; queries and estimates do not.
	uint32_t next_link = 0; ///< Independent nonreused utility route identity allocator.
	std::vector<UtilityPowerProducer> producers; ///< Stable owner-allocated source order.
	std::vector<UtilityPowerConsumer> consumers; ///< Stable owner-allocated consumer order.
	std::vector<UtilityPowerLink> links; ///< Saved actual commissioned eligibility and interval metered flow.
	uint64_t local_link_revision = 0; ///< Saved committed local topology/ownership changes only; energy publication/allocation never invalidates physical path caches.
	uint32_t next_share = 0; ///< Shared slice issuer for this epoch across both domains; reset only after the entire epoch's reservations clear.
};

/**
 * Issue a paid builder's local-cell identity through the existing shared link allocator.
 * @param owner Actual constructing company; physical funding/coexistence belong to its command.
 * @param domain Heat stays Deep-local; electricity also permits Underground relay cells.
 * @param tile Explicit fully excavated permitted rock cell; each domain has one active installed cell here.
 * @param capacity_w Bounded installed rate supplied by the canonical utility definition.
 * @param execute False validates/predicts without registering or spending anything.
 * @return Shared nonreused link identity or unchanged refusal; this primitive adds no energy, grant or private registry.
 */
std::optional<UtilityLinkID> RegisterUtilityLocalLink(CompanyID owner, UtilityEnergyDomain domain, TileIndex tile, uint64_t capacity_w, bool execute = true);
/** @param id Shared issued link reference. @return Borrowed immutable retained link, including retired history; re-query after mutation/reset. */
const UtilityPowerLink *GetUtilityPowerLink(UtilityLinkID id);
/** @return Borrowed bounded shared link list; callers filter actual immutable kinds, with no copied identity or energy authority. */
std::span<const UtilityPowerLink> GetUtilityPowerLinks();
/** @return Borrowed canonical consumer identity order for read-only native inspection; re-query after ledger mutation/reset and never retain balances or infer permission from this view. */
std::span<const UtilityPowerConsumer> GetUtilityPowerConsumers();
/** @return Committed local topology revision; ordinary resource queries/publications cannot change it. */
uint64_t GetUtilityLocalLinkRevision();
/** @return Whether retained local-cell identities exist, including removed historical installations; no cell or energy is created. */
bool HasUtilityLocalLinks();

/** Explicit current-interval request; registration owns its company, layer, capacity and eligible sources. */
struct UtilityPowerRequest {
	UtilityConsumerID consumer{UINT32_MAX}; ///< Registered consumer, never a feature-local ID.
	uint64_t demand_j = 0; ///< Actual bounded request, separate from production and available supply.
};

/** Transient exact same-domain source allocation checked by a physical network; the native utility owner alone publishes its quantity and lineage. */
struct UtilityHeatRoutingShare {
	UtilityConsumerID consumer{UINT32_MAX}; ///< Actual registered thermal participant.
	UtilityProducerID producer{UINT32_MAX}; ///< Actual active same-company Deep Heat meter.
	uint64_t quantity_j = 0; ///< Exact feasible route quantity; zero records eligibility without creating a reservation.
};

/**
 * Allocate physically constrained thermal routes in the existing shared owner.
 * @param interval Exact current open accounting epoch.
 * @param requests Whole feasible bounded requests, independent of grant/use.
 * @param routes Network-validated source shares in stable consumer/source order; all references/aggregate source and request bounds are revalidated atomically.
 * @param execute False estimates the complete phase without changing eligibility, meters, shares or time.
 * @return Complete once-only allocation or unchanged refusal; native quantities and source lineage are never stored by a second allocator.
 */
UtilityPowerError AllocateUtilityHeatRoutes(uint64_t interval, std::span<const UtilityPowerRequest> requests,
	std::span<const UtilityHeatRoutingShare> routes, bool execute = true);

/** Exact reservation identity combines its owner-allocated consumer and accounting epoch; never aliases a later grant. */
struct UtilityGrantID {
	UtilityConsumerID consumer{UINT32_MAX}; ///< Actual ledger consumer identity.
	uint64_t interval = 0; ///< Exact nonzero allocation epoch.
	/** @param other Reservation to compare. @return Whether consumer and epoch are identical. */
	bool operator==(const UtilityGrantID &other) const = default;
};

/** Whole utility mutation kinds; transfer additionally requires an explicitly commissioned physical link. */
enum class UtilityMutationKind : uint8_t { Consume, ReleaseUnused, TransferLayer };

/** Pure transient utility proposal; it owns no energy or inventory and is never restored as a live saved object. */
struct UtilityMutationProposal {
	UtilityPowerError error = UtilityPowerError::InvalidReference; ///< Only None authorizes commit revalidation.
	UtilityMutationKind kind = UtilityMutationKind::Consume; ///< Exact requested settlement/release/transfer.
	UtilityEnergyDomain domain = UtilityEnergyDomain::Electricity; ///< Typed prepared quantity; a changed domain invalidates the proposal and cannot borrow another grant.
	UtilityGrantID grant; ///< Source reservation, including exact epoch.
	CompanyID owner = CompanyID::Invalid(); ///< Actual company that prepared this operation.
	uint64_t quantity_j = 0; ///< Positive whole quantity to settle once.
	UtilityConsumerID destination{UINT32_MAX}; ///< Explicit transfer destination; absent for consumption/release.
	UtilityLinkID link{UINT32_MAX}; ///< Explicit commissioned route; absent for consumption/release.
	uint64_t sequence_before = 0; ///< Exact saved ledger change sequence observed during preparation.
	uint64_t seal = 0; ///< Field-wise transient proposal fingerprint detects modified quantities/targets/kinds.
};

/** Actual inventory/movement/effect participant; complete refusal leaves every material, work and money owner unchanged. */
class UtilityMutationParticipant {
public:
	/** Destroy through the public complete-transaction boundary. */
	virtual ~UtilityMutationParticipant() = default;
	/** @return Domain of the actual whole physical benefit; existing electrical participants cannot consume a thermal reservation. */
	virtual UtilityEnergyDomain EnergyDomain() const { return UtilityEnergyDomain::Electricity; }
	/** @return Actual direct-recipe/generator benefit when this participant consumes Heat; electrical or unspecified participants expose no thermal role. */
	virtual std::optional<UtilityHeatUse> ThermalUse() const { return std::nullopt; }
	/** @param proposal Exact revalidated utility proposal. @return Whether the actual complete owner transaction still accepts; this check changes nothing. */
	virtual bool Validate(const UtilityMutationProposal &proposal) const = 0;
	/** @param proposal Exact checked utility quantity. @return Complete success or unchanged refusal; no utility API may be reentered during this whole owner commit. */
	virtual bool Commit(const UtilityMutationProposal &proposal) noexcept = 0;
	/** @return Explicit native effect identity when this whole participant actually applies it; ordinary inventory/expense participants expose none. */
	virtual std::optional<UtilityConsumerEffect> EffectKind() const { return std::nullopt; }
};

/** Exact bounded work response backed by actually consumed electricity; physical progress remains with its existing feature owner. */
struct UtilityWorkEffect {
	UtilityConsumerEffect kind = UtilityConsumerEffect::TubeWork; ///< Actual registered mandatory service or optional processor benefit.
	uint64_t energy_j = 0; ///< Exact positive original-source quantity settled with the work transaction.
	uint32_t work_economy_ticks = 0; ///< Admitted mandatory work or capped optional bonus ticks; never a rendering duration.
	uint8_t bonus_percent = 0; ///< Processor-only response within the frozen25-percent cap.
};

/** Actual facility/movement work owner validates and commits its bounded response with the same electrical settlement. */
class UtilityWorkParticipant {
public:
	/** Destroy through the public complete-work boundary. */
	virtual ~UtilityWorkParticipant() = default;
	/** @param effect Exact admitted work/energy response. @return Whether actual job identity, remaining work, outputs and revision still admit the whole action without mutation. */
	virtual bool Validate(const UtilityWorkEffect &effect) const = 0;
	/** @param effect Exact already checked response. @return Complete physical work success or unchanged refusal; successful work advances its owner's saved revision. */
	virtual bool Commit(const UtilityWorkEffect &effect) noexcept = 0;
};

/** Exact once-per-epoch cross-domain request; preparation borrows the actual grant rather than creating heat or an output account. */
struct UtilityHeatConversionRequest {
	UtilityGrantID grant; ///< Existing original HeatGenerator reservation and accounting interval.
	UtilityProducerID output{UINT32_MAX}; ///< Existing conversion-only electrical source bound to that original consumer.
	uint64_t heat_j = 0; ///< Exact positive original thermal quantity proposed for use.
	uint32_t economy_ticks = 0; ///< Actual committed synchronized service duration, independent of views or wall time.
};

/** Pure sealed whole conversion proposal; it contains no spendable quantity and cannot survive another accepted ledger mutation. */
struct UtilityHeatConversionProposal {
	UtilityPowerError error = UtilityPowerError::InvalidReference; ///< Only None permits complete commit revalidation.
	UtilityHeatConversionRequest request; ///< Exact original input/output/duration identity.
	CompanyID owner = CompanyID::Invalid(); ///< Actual native controller that prepared the proposal.
	uint64_t sequence_before = 0; ///< Exact shared retry identity before preparation.
	uint64_t electric_j = 0; ///< Checked whole output derived from the original input and saved source numerator.
	uint64_t loss_j = 0; ///< Explicit whole-joule input minus output; no heat can be counted as both useful domains.
	uint8_t remainder_after = 0; ///< Proposed fractional-percent numerator; refused work leaves the saved numerator unchanged.
	uint64_t seal = 0; ///< Field-wise whole request/result fingerprint; modified tickets refuse unchanged.
};

/** One complete shared conversion receipt; historical actual use and electrical publication share the same accepted operation sequence. */
struct UtilityHeatConversionReceipt {
	UtilityPowerError error = UtilityPowerError::InvalidReference; ///< Exact admission/refusal; failed receipts settle no quantity.
	uint64_t sequence_after = 0; ///< Shared ledger identity after success or the untouched current identity on refusal.
	uint64_t heat_consumed_j = 0; ///< Exact original thermal quantity settled once.
	uint64_t electricity_generated_j = 0; ///< Exact new electrical publication, including a valid zero whole-joule result.
	uint64_t loss_j = 0; ///< Explicit whole-joule conversion loss, distinct from later unused electrical curtailment.
};

/**
 * Register one exact electrical output for an existing original HeatGenerator consumer.
 * @param owner Actual company controlling the original Heat consumer and output.
 * @param input Existing active native Deep HeatGenerator identity, with at most one retained conversion output.
 * @param efficiency_percent Immutable canonical model efficiency1..100.
 * @param capacity_w Actual canonical electrical rating, bounded by the shared rate contract.
 * @param execute False predicts the nonreused identity without allocating or changing the ledger.
 * @return Issued conversion-only electrical identity or unchanged refusal; ordinary rate registration/publication cannot bypass this binding.
 */
std::optional<UtilityProducerID> RegisterUtilityHeatConversionProducer(CompanyID owner, UtilityConsumerID input,
	uint8_t efficiency_percent, uint64_t capacity_w, bool execute = true);
/** @param request Exact original Heat/output/duration request. @return Pure checked sealed conversion; no IDs, quantities, carry, cash or participant state change. */
UtilityHeatConversionProposal PrepareUtilityHeatConversion(const UtilityHeatConversionRequest &request);
/**
 * Commit one revalidated whole conversion through the shared owner.
 * @param proposal Exact untouched current sealed proposal.
 * @param participant Actual HeatGenerator physical/work/invoice owner; validation and complete unchanged-refusal commit prohibit recursive utility mutation.
 * @return Complete original Heat use/output/loss receipt, or unchanged refusal before any partial benefit.
 */
UtilityHeatConversionReceipt CommitUtilityHeatConversion(const UtilityHeatConversionProposal &proposal, UtilityMutationParticipant &participant);
/** @param input Original HeatGenerator consumer. @param output Its original conversion source, or invalid while registration is pending. @param execute False checks complete retirement without mutation. @return Whole retirement preserving IDs, fractions and consumed history; unused reservations alone expire. */
UtilityPowerError RetireUtilityHeatConversion(UtilityConsumerID input, UtilityProducerID output, bool execute = true);
/** Complete utility transaction result, defined below; effect APIs share the same settlement receipt. */
struct UtilityMutationReceipt;

/**
 * Apply a bounded native work/service response through the actual complete work owner.
 * @param consumer Actual registered ProcessorBoost, TubeWork or AssemblyWork consumer.
 * @param requested_work_ticks Actual synchronized baseline/service opportunity owned by the calling job, at most the frozen tick bound.
 * @param participant Existing physical work owner; refusal preserves work and electrical balance.
 * @return Complete utility receipt or unchanged refusal; zero budget/opportunity applies no phantom benefit.
 */
UtilityMutationReceipt ApplyUtilityWorkEffect(UtilityConsumerID consumer, uint32_t requested_work_ticks, UtilityWorkParticipant &participant);

/**
 * Bind an actual Surface town to one explicit native town-growth consumer.
 * @param consumer Actual same-company TownGrowth reservation owner.
 * @param town Existing Surface town; custom GS ownership and duplicate native suppliers refuse.
 * @param execute False validates association without changing binding, sequence or town growth.
 * @return Complete binding or specific unchanged refusal; no effect exists until actual settlement.
 */
UtilityPowerError BindUtilityTownConsumer(UtilityConsumerID consumer, TownID town, bool execute = true);

/**
 * Attach one existing native town effect to a real paid Surface distribution terminal and original powerhouse reservation.
 * @param destination Existing same-owner Surface TownGrowth consumer bound to a real town.
 * @param source Existing Deep electrical forwarding consumer linked only to the actual native powerhouse output.
 * @param terminal Actual owned Surface rail/truck terminal in the town's ordinary local-authority area.
 * @param source_port Actual completed native powerhouse boundary with its separately paid electrical cell.
 * @param execute False validates the complete connection without allocating or changing any owner.
 * @return Complete unchanged refusal or one reference-only connection; this creates no demand, energy or additional capital bill.
 */
UtilityPowerError BindUtilityTownService(UtilityConsumerID destination, UtilityConsumerID source, StationID terminal, TileIndex source_port, bool execute = true);

/** Explicit actual machinery/recipient selection; the shared owner derives every account, physical port and bounded demand. */
struct GeothermalTownServiceRequest {
	IndustryID generator = IndustryID::Invalid(); ///< Completed actual native generator, never an arbitrary numerical rate source.
	IndustryID first_riser = IndustryID::Invalid(); ///< Actual paid Deep sender; its real destination determines the next plane.
	IndustryID second_riser = IndustryID::Invalid(); ///< Actual Underground-to-Surface sender, absent only for a direct Deep/Surface pair.
	StationID terminal = StationID::Invalid(); ///< Existing owned occupied Surface train/truck terminal.
	TownID town = TownID::Invalid(); ///< Actual native town authorized by every serving terminal tile.
};

/** Whole connection outcome; pure estimates predict identities but confer no reservation, power or physical rights. */
struct GeothermalTownServiceConnection {
	UtilityPowerError error = UtilityPowerError::Refused; ///< Whole unchanged refusal or complete shared-owner admission.
	UtilityConsumerID source{UINT32_MAX}; ///< Actual/predicted original-output forwarding account.
	UtilityConsumerID relay{UINT32_MAX}; ///< Actual/predicted Underground relay, absent for direct export.
	UtilityConsumerID destination{UINT32_MAX}; ///< Actual/predicted single native town account.
	std::array<UtilityLinkID, 2> links{UtilityLinkID{UINT32_MAX}, UtilityLinkID{UINT32_MAX}}; ///< Original paid direction identities in physical route order.
};

/**
 * Prepare or atomically connect/recover an owned native town service through explicitly selected paid physical machinery.
 * @param request Actual generator, ordered one/two-riser route, Surface terminal and authorized native town.
 * @param execute False validates the complete candidate without changing any identity, balance, stock, work, cash or RNG.
 * @return Complete connection/recovery or unchanged refusal. Recovery preserves the original town account and one/two-plane topology, keeps exact unchanged edges and retires only superseded unused rights; an identical request refuses without settlement.
 */
GeothermalTownServiceConnection ConnectGeothermalTownService(const GeothermalTownServiceRequest &request, bool execute = true);
/** @param requests Complete synchronized request set. Append only feasible real terminal/town service demands before the sole electrical allocation. */
void AppendUtilityTownServiceRequests(std::vector<UtilityPowerRequest> &requests);
/** Settle each actual exported original-source offer and its one native town effect after the common electrical allocation. */
void RunUtilityTownServices();
/** @return Whether retained explicit town services need synchronized expiry/recovery even when their last physical energy source has disappeared. */
bool HasUtilityTownServices();
/** @param terminal Station being destroyed before pool-ID reuse. Revoke its eligibility and any unconsumed/current town offer. */
void RemoveUtilityTownServiceTerminal(StationID terminal);

/**
 * Settle one current native town-power offer and apply its capped growth benefit together.
 * @param consumer Explicit bound native TownGrowth consumer of the asking company.
 * @return Complete settlement or unchanged refusal; custom GS authority, absence, no benefit and repeated application consume nothing.
 */
UtilityMutationReceipt ApplyUtilityTownEffect(UtilityConsumerID consumer);

/** @param town Existing actual town. @return Its accepted current native bonus or zero for absence, Classic/ETI, custom GS or suspended/expired effect; never renews energy. */
uint8_t GetUtilityTownPowerBonus(TownID town);

/** @param town Actual town adopting custom GS growth; expire only its unused native offer and suppress current native benefit without refunding use. */
void SuspendUtilityTownEffect(TownID town);

/** @param town Actual town being removed; unbind saved native association before its pool ID can be reused, preserving settled source history. */
void RemoveUtilityTownBindings(TownID town);

/** Complete receipt; only accepted transactions advance the saved operation identity or settle energy. */
struct UtilityMutationReceipt {
	UtilityPowerError error = UtilityPowerError::InvalidReference; ///< Exact unchanged refusal or complete success.
	uint64_t operation_sequence = 0; ///< Committed identity, or unchanged live sequence on refusal.
	uint64_t quantity_j = 0; ///< Actually consumed/released/transferred quantity; zero on refusal.
};

/**
 * Prepare a positive whole action against the actual current owned reservation without changing it.
 * @param kind Consumption, unused release or explicit commissioned layer transfer.
 * @param grant Exact current consumer/epoch identity.
 * @param quantity_j Positive bounded available joules; queries never create or renew them.
 * @param destination Actual destination consumer for transfer; invalid otherwise.
 * @param link Actual commissioned utility route for transfer; invalid otherwise.
 * @return Pure exact proposal or specific refusal; transfer requires the actual registered commissioned route.
 */
UtilityMutationProposal PrepareUtilityMutation(UtilityMutationKind kind, UtilityGrantID grant, uint64_t quantity_j,
	UtilityConsumerID destination = UtilityConsumerID{UINT32_MAX}, UtilityLinkID link = UtilityLinkID{UINT32_MAX});

/**
 * Revalidate and commit utility settlement together with its complete actual owner participant.
 * @param proposal Exact transient proposal; stale, repeated or modified proposals refuse unchanged.
 * @param participant Actual complete inventory/movement/effect participant, required for consumption; pure release needs none.
 * @return Complete receipt; failure advances no sequence and changes no energy or physical quantity.
 */
UtilityMutationReceipt CommitUtilityMutation(const UtilityMutationProposal &proposal, UtilityMutationParticipant *participant = nullptr);

/**
 * Register a directed electrical edge only through an actual owned/date-valid Connected portal pair.
 * Physical riser construction/payment remains with its feature builder; this internal boundary owns only explicit metered identity.
 * @param source Registered source-side consumer reservation of local generation.
 * @param destination Registered same-company consumer on the reciprocal endpoint layer.
 * @param source_endpoint Actual world endpoint on the source consumer's layer.
 * @param capacity_j Builder-authorized positive bounded interval flow; never an implicit generation rate.
 * @param execute False validates/predicts identity without publishing any edge or quantity.
 * @return Ledger-issued/predicted route ID, or absence for incomplete/foreign/locked topology.
 */
std::optional<UtilityLinkID> RegisterUtilityPowerLink(UtilityConsumerID source, UtilityConsumerID destination,
	TileIndex source_endpoint, uint64_t capacity_j, bool execute = true);

/**
 * Retire a route and release its still-unused destination imports without restoring source availability or settled use.
 * @param link Exact ledger-owned route identity, never reused after loss.
 * @param execute False estimates complete loss admission without changing any quantity.
 * @return Complete release or specific unchanged refusal; owner/system lifecycle authority is required.
 */
UtilityPowerError RemoveUtilityPowerLink(UtilityLinkID link, bool execute = true);

/**
 * Invalidate every metered edge attached to a physical portal before it is sealed or removed.
 * @param endpoint Either actual world endpoint; this lifecycle hook retires routes and expires only unused imports.
 */
void InvalidateUtilityPowerLinksForPortal(TileIndex endpoint);
/** @param industry Actual riser machinery being removed/abandoned. Retire its active paid edges and expire only unused imports before native pool identity reuse; original settlement remains with this owner. */
void InvalidateUtilityPowerLinksForRiser(IndustryID industry);
/** Revalidate accepted physical Heat paths after actual load. Expire only unused native slices whose original installed cells are now retired; preserve original quantities/use/history otherwise, including legacy393 lost-path state. No new grant or replacement path is inferred. */
void RevalidateUtilityHeatRoutes();

/**
 * Register an actual company's native source without adding any energy.
 * @param owner Existing company responsible for the source.
 * @param layer Actual output layer, checked against allocated/date-valid layers.
 * @param kind Actual generation family; no mirror modifier is applied here.
 * @param execute False estimates the next ID without advancing it or creating an account.
 * @return Allocated/predicted owner ID, or no value for invalid policy, identity, layer or capacity.
 */
std::optional<UtilityProducerID> RegisterUtilityPowerProducer(CompanyID owner, MapLayer layer, UtilityGenerationKind kind, bool execute = true);

/**
 * Register a bounded actual consumer with explicitly supplied physical connectivity.
 * @param owner Existing company responsible for the consumer.
 * @param layer Actual local consumption layer.
 * @param effect Native effect contract; unsupported effects refuse.
 * @param capacity_j Maximum accepted per-interval demand in joules.
 * @param sources Actual network owner's explicit same-company/same-layer eligible producers; duplicates refuse.
 * @param execute False validates/predicts without mutating accounts or IDs.
 * @return Allocated/predicted owner ID, or no value when the complete registration is invalid.
 */
std::optional<UtilityConsumerID> RegisterUtilityPowerConsumer(CompanyID owner, MapLayer layer, UtilityConsumerEffect effect,
	uint64_t capacity_j, std::span<const UtilityProducerID> sources, bool execute = true);

/**
 * Register an actual Deep thermal source through the same native producer allocator, adding no energy.
 * @param owner Existing asking company; its physical source owner must separately validate constructed collector/source availability.
 * @param execute False predicts admission/identity without consuming a native slot.
 * @return Shared native Heat producer ID, or unchanged refusal; no electrical/mirror/source quantity is fabricated.
 */
std::optional<UtilityProducerID> RegisterUtilityHeatProducer(CompanyID owner, bool execute = true);

/**
 * Register an immutable thermal consumer role with explicitly owned Deep source eligibility.
 * @param owner Existing asking company, independent of translated facility names.
 * @param use Direct recipe or electrical generator; both compete in the frozen stable ConsumerID order.
 * @param capacity_j Bounded maximum thermal joules requested per native interval.
 * @param sources Actual physical network owner's eligible same-company Deep Heat producers; coordinates alone supply no link.
 * @param execute False checks/predicts identity without reserving a consumer or energy.
 * @return Shared native Heat consumer ID, or unchanged whole admission refusal.
 */
std::optional<UtilityConsumerID> RegisterUtilityHeatConsumer(CompanyID owner, UtilityHeatUse use, uint64_t capacity_j,
	std::span<const UtilityProducerID> sources, bool execute = true);

/**
 * Publish a validated physical source's thermal rate once in the native interval; the common ledger alone retains generated heat/carry.
 * @param producer Actual registered Heat source belonging to the asking company.
 * @param interval Exact current nonterminal native accounting epoch.
 * @param rate_w Actual bounded post-availability vent heat rate; geothermal validates original source and completed machinery before calling.
 * @param economy_ticks Actual synchronized duration at the frozen27-ms tick.
 * @param execute False calculates admission without changing heat, carry, ID or publication history.
 * @return Typed complete publication or unchanged refusal; electrical sources cannot enter this path.
 */
UtilityPowerError PublishUtilityHeatGeneration(UtilityProducerID producer, uint64_t interval, uint64_t rate_w, uint32_t economy_ticks, bool execute = true);

/**
 * Reserve thermal shares in stable shared ConsumerID/ProducerID order, separately from subsequent electrical allocation.
 * @param interval Exact current native accounting epoch; heat can be allocated only once.
 * @param requests Actual registered Heat consumers and feasible bounded quantities; roles/connectivity retain their canonical owners.
 * @param execute False prepares complete domain admission without reserving or consuming any quantity.
 * @return Whole global system allocation or unchanged refusal; no heat export or electrical substitution is admitted.
 */
UtilityPowerError AllocateUtilityHeat(uint64_t interval, std::span<const UtilityPowerRequest> requests, bool execute = true);

/** @return Whether this native interval's independent thermal allocation has closed; querying never publishes or renews heat. */
bool IsUtilityHeatAllocated();
/** @return Whether thermal identity/phase history exists in the shared native owner; no snapshot or second registry is allocated. */
bool HasUtilityHeatState();

/** @return Whether retained exact-conversion producer metadata/history belongs to the sole utility owner; no energy or copied ledger is created. */
bool HasUtilityHeatConversionState();

/** @param consumer Asking company's actual registered Heat consumer. @return Remaining current thermal reservation/unmet view, or absence; queries never allocate/use/renew heat. */
std::optional<UtilityPowerAccount> GetUtilityHeatAllocation(UtilityConsumerID consumer);

/**
 * Prepare exact typed thermal use or unused release for the same whole-owner atomic settlement boundary.
 * @param kind Consume or ReleaseUnused; thermal transfer through an electrical riser always refuses.
 * @param grant Exact current shared consumer/epoch identity.
 * @param quantity_j Positive bounded thermal joules; actual consumption requires a matching Heat physical participant at commit.
 * @return Pure thermal proposal or unchanged refusal; no electricity grant can authorize this action.
 */
UtilityMutationProposal PrepareUtilityHeatMutation(UtilityMutationKind kind, UtilityGrantID grant, uint64_t quantity_j);

/**
 * Publish one source's actual post-availability rate exactly once this interval.
 * Physical production/payment/mirror eligibility belongs to the caller; this native boundary is not a player command.
 * @param producer Registered source belonging to the current company.
 * @param interval Exact current accounting epoch; stale/future publication refuses.
 * @param rate_w Actual bounded available output rate, after its one source-specific modifier.
 * @param economy_ticks Synchronized production duration, independent of cameras/wall time.
 * @param execute False estimates with the old saved remainder and changes no state.
 * @return Specific refusal or None; zero supply is a valid once-only publication.
 */
UtilityPowerError PublishUtilityGeneration(UtilityProducerID producer, uint64_t interval, uint64_t rate_w,
	uint32_t economy_ticks, bool execute = true);

/**
 * Reserve interval energy for explicit requests through the frozen deterministic allocator.
 * The synchronized system caller uses OWNER_DEITY so one company cannot close the world's allocation ahead of its peers.
 * @param interval Exact current epoch, allocated at most once after source publication.
 * @param requests Bounded distinct registered requests, each within its actual capacity.
 * @param execute False validates/plans without publishing grants or closing the generation phase.
 * @return Specific refusal or None; omitted consumers receive zero, never a renewed prior grant.
 */
UtilityPowerError AllocateUtilityPower(uint64_t interval, std::span<const UtilityPowerRequest> requests, bool execute = true);

/**
 * Inspect one current reservation without consuming or renewing it.
 * @param consumer Registered consumer of the currently asking company.
 * @return Current free-balance view derived from saved source slices, or no value for missing/foreign/unallocated state; settled use is never renewed.
 */
std::optional<UtilityPowerAccount> GetUtilityAllocation(UtilityConsumerID consumer);

/** @param producer Actual ledger-issued source ID. @return Borrowed immutable retained source, or nullptr; invalidated by ledger mutation. */
const UtilityPowerProducer *GetUtilityPowerProducer(UtilityProducerID producer);

/** @param consumer Actual ledger-issued consumer ID. @return Borrowed immutable retained consumer, or nullptr; invalidated by ledger mutation. */
const UtilityPowerConsumer *GetUtilityPowerConsumer(UtilityConsumerID consumer);

/** @return Sole ledger's current synchronized epoch identity, without copying accounts or renewing any publication. */
uint64_t GetUtilityPowerInterval();
/** @return Whether the current epoch's one global allocation is closed; views never allocate. */
bool IsUtilityPowerAllocated();

/**
 * Close the prior interval and begin generation for the next synchronized epoch.
 * @param execute False validates expiry/epoch bounds without changing anything.
 * @return Refusal or None; requires OWNER_DEITY and an allocated prior interval, curtails unused grants and preserves generation fractions.
 */
UtilityPowerError BeginUtilityPowerInterval(bool execute = true);

/**
 * Retire a source and release only its still-unused reservation slices.
 * @param producer Existing owner-allocated generation identity; never reused after retirement.
 * @param execute False estimates source-loss consequences without mutation.
 * @return Refusal or None; owner or explicit system authority may retire it, preserving already settled use and other sources' grants.
 */
UtilityPowerError RemoveUtilityPowerProducer(UtilityProducerID producer, bool execute = true);

/** Retire one native orbital facility's complete original electrical bindings atomically.
 * The physical module owner must first settle its cargo/jobs/dependents. This
 * shared owner releases only unused slices, retains all consumed lineage and
 * retires IDs permanently; it publishes no replacement account or energy.
 * @param producers Zero or one actual Space OrbitalPanel identity belonging to the caller/system.
 * @param consumers Up to two actual Space AssemblyWork identities belonging to the same caller/system; duplicates refuse.
 * @param execute False validates the whole set without changing any meter, eligibility, sequence or identity.
 * @return Complete retirement or unchanged refusal; active transfer routes require their own physical lifecycle first.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups4C/5.
 */
UtilityPowerError RetireOrbitalUtilityAccounts(std::span<const UtilityProducerID> producers, std::span<const UtilityConsumerID> consumers, bool execute = true);

/** @return Read-only whole-state copy for feature persistence and focused owner tests. */
UtilityPowerState CaptureUtilityPowerState();

/**
 * Apply actual company takeover/removal before tile ownership or company pool identity changes.
 * @param old_owner Existing company whose retained accounts and physical routes are affected.
 * @param new_owner Existing takeover company, or INVALID_OWNER for removal.
 * Transfer the current incarnation's account/source lineage together; retain earlier inactive tombstones.
 * Retire routes requiring new physical admission on takeover.
 * Removal expires only unused shares and retains inactive identities and original settlement history.
 */
void ChangeUtilityPowerCompanyOwnership(Owner old_owner, Owner new_owner);

/** Reset live and unpublished utility authority at world initialization. */
void ResetUtilityPowerState();

/** @return Stable field-wise digest of every saved utility meter, identity, connection and reservation; never hashes padding or pointers. */
uint64_t ComputeUtilityPowerDigest();

/**
 * Validate complete saved electrical authority before publishing any restored row.
 * @param state Candidate owner transport; no private registries or energy are repaired.
 * @param selection Accepted saved profile, independent of the startup selector.
 * @param check_world_refs True checks actual loaded companies/calendar/map; false supports nondestructive file preview.
 * @param layer_count Explicit copied preview layer count when world references are unavailable.
 * @return Specific identity/layer/balance refusal, or None for a complete valid candidate.
 */
UtilityPowerError ValidateUtilityPowerState(const UtilityPowerState &state, const MultiLayerProfileSelection &selection,
	bool check_world_refs = true, uint layer_count = MAX_MAP_LAYERS);

/** Reset unpublished concrete utility chunk transport at world initialization. */
void ResetUtilityPowerLoadStaging();

/** Validate the copied complete utility save family during nondestructive file preview. */
void ValidateUtilityPowerLoadPreview();

/**
 * Stage utility transport until actual company/map/profile references have loaded.
 * @param state Complete bounded candidate; publication waits for acceptance.
 */
void StageLoadedUtilityPowerState(UtilityPowerState state);

/** @return Whether staged utility records validate and publish atomically; absent older state remains empty. */
bool AcceptLoadedUtilityPowerState();

/** @return Whether actual interval/identity history requires the feature-owned utility save family. */
bool HasUtilityPowerState();

#endif /* UTILITY_POWER_H */
