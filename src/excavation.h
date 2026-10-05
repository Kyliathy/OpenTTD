/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation.h Persistent rock-plane state and sparse geological authority. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md */

#ifndef EXCAVATION_H
#define EXCAVATION_H

#include "map_layer.h"
#include "cargotype.h"
#include "company_type.h"
#include "vehicle_type.h"
#include "station_type.h"
#include "economy_type.h"
#include "multilayer_content.h"

/** Fixed two-bit terrain meanings; the fourth encoding is corrupt, never an open cavity. */
enum class ExcavationState : uint8_t {
	Solid = 0, Excavating = 1, Excavated = 2, Reserved = 3,
};

/** Stable sparse geological identity; UINT32_MAX is invalid and IDs are never reused in a world. */
using GeologyDepositID = StrongType::Typedef<uint32_t, struct GeologyDepositIDTag, StrongType::Compare>;

/** Compact inclusive horizontal seam span, independent of whatever infrastructure occupies its tiles. */
struct GeologySpan {
	uint16_t y = 0; ///< Layer-local row.
	uint16_t first_x = 0; ///< Inclusive first seam column.
	uint16_t last_x = 0; ///< Inclusive last seam column.
};

/** Saved ground hardness; these identities select the frozen content's cut yield/work contract. */
enum class GeologyHardness : uint8_t { Soft, Mixed, Hard, Basalt };

/** Sparse deposit authority; visibility and the rebuilt spatial index never own a second reserve. */
struct GeologyDeposit {
	GeologyDepositID id{UINT32_MAX}; ///< Stable record identity, separate from future mine identity.
	MapLayer layer = MapLayer::Invalid; ///< Exactly one rock layer.
	CargoLabel resource{}; ///< Stable cargo meaning, resolved through the selected profile.
	GeologyHardness hardness = GeologyHardness::Soft; ///< Frozen cutting difficulty.
	CompanyID claimant = CompanyID::Invalid(); ///< Exclusive claim; invalid means unclaimed.
	uint64_t initial_reserve = 0; ///< Original finite quantity in tonnes.
	uint64_t remaining_reserve = 0; ///< Sole recoverable reserve balance.
	uint64_t surveyed_companies = 0; ///< Saved optional company information; current Native planning is public/free, never excavation permission.
	std::vector<GeologySpan> spans; ///< At most 64 canonical nonoverlapping spans; no per-tile heap object.
};

/**
 * Two bits per tile over exactly Underground and Deep Underground.
 * Tile constructors can replace all m-fields without changing this authority.
 * World-tile access rejects other layers, void tiles and stale map dimensions.
 * Direct writes belong to generation/load and later validated excavation transactions.
 */
class PackedExcavationState {
	uint32_t plane_size = 0; ///< Geometry captured at allocation, checked against the current map.
	std::vector<uint8_t> bytes; ///< Four cells per byte, Underground followed by Deep; not a tile-type cache.

	/**
	 * Resolve a playable world tile through the layer controller to its packed cell.
	 * @param tile Explicit world tile; camera selection never supplies its layer.
	 * @return Rock-cell index, or no result for non-rock/void/stale geometry.
	 */
	std::optional<uint32_t> Index(TileIndex tile) const;

public:
	/**
	 * Allocate/reset both rock planes to solid without allocating the ordinary map.
	 * @param cells_per_plane Bounded plane size; zero releases this world's authority.
	 * @return False for an unsupported plane size, leaving the old state unchanged.
	 */
	bool Reset(uint32_t cells_per_plane);

	/**
	 * Read a playable rock tile without interpreting tile type as cavity state.
	 * @param tile Explicit world tile.
	 * @return Decoded state or no result for invalid addressing/corrupt reserved bits.
	 */
	std::optional<ExcavationState> Get(TileIndex tile) const;

	/**
	 * Store one accepted geological state without changing infrastructure.
	 * @param tile Explicit playable rock tile.
	 * @param state Real terrain state; reserved/out-of-range encodings refuse.
	 * @return Whether the complete validated write succeeded.
	 */
	bool Set(TileIndex tile, ExcavationState state);

	/**
	 * Restore validated packed transport independently of live infrastructure metadata.
	 * @param cells_per_plane Explicit geometry captured by the feature header.
	 * @param payload Exact two-plane bytes; reserved encodings refuse before publication.
	 * @return Whether the whole payload was accepted, leaving old state unchanged on failure.
	 */
	bool Restore(uint32_t cells_per_plane, std::span<const uint8_t> payload);

	/** @return Actual packed payload bytes, excluding the vector object's sparse overhead. */
	size_t StorageBytes() const { return this->bytes.size(); }

	/** @return Captured number of cells in one plane; zero means absent legacy storage. */
	uint32_t PlaneSize() const { return this->plane_size; }

	/** @return Read-only canonical packed transport, never a mutable parallel terrain authority. */
	std::span<const uint8_t> Data() const { return this->bytes; }
};

static constexpr uint32_t MAX_GEOLOGY_DEPOSITS = 4096; ///< Sparse record cap, independent of map cell count.
static constexpr uint32_t MAX_GEOLOGY_SPANS = 64; ///< Each seam has bounded canonical shape and bounded lookup work.
static constexpr uint32_t GEOLOGY_BUCKET_SIZE = 16; ///< Layer-local spatial bucket width/height.
static constexpr uint32_t MAX_GEOLOGY_BUCKET_ENTRIES = 32; ///< Maximum candidates inspected by any tile lookup.

/** Specific geological refusal; invalid/overlapping/claimed state never causes partial reserve mutation. */
enum class GeologyError : uint8_t { None, InvalidID, InvalidShape, InvalidResource, InvalidReserve, Limit, Overlap, ClaimConflict, Depleted, UnsupportedMap };

/**
 * One sparse deposit authority with a rebuildable, bounded spatial cache.
 * IDs increase monotonically, including across deletion; lookup order cannot choose an overlapping owner.
 * Read-only references remain valid only until the next store mutation.
 */
class GeologyDepositStore {
	std::vector<GeologyDeposit> deposits; ///< Canonical increasing-ID records; these are saved authority.
	std::map<uint32_t, std::vector<GeologyDepositID>> buckets; ///< Rebuilt cache; no reserve/claim lives here.
	uint32_t next_id = 0; ///< Saved monotonic identity cursor, never lowered by deletion.

	/**
	 * Locate a mutable record for an already validated producer transaction.
	 * @param id Exact deposit reference; no translated cargo name or runtime slot is accepted.
	 * @return Record or nullptr for absent/deleted/invalid identity.
	 */
	GeologyDeposit *Mutable(GeologyDepositID id);

public:
	/** @return The next fresh identity; caller assigns it before inserting one generation/editor record. */
	GeologyDepositID NextID() const { return GeologyDepositID{this->next_id}; }

	/**
	 * Validate and insert a canonical bounded seam, atomically refusing overlaps and malformed authority.
	 * @param deposit Fresh next-ID record; shape is canonicalized without changing any existing record.
	 * @return Specific refusal or success; no gameplay mine is created by insertion.
	 */
	GeologyError Insert(GeologyDeposit deposit);

	/**
	 * Resolve a geological record by exact stable identity.
	 * @param id Deposit reference, including possible deleted/invalid IDs.
	 * @return Read-only authority or nullptr.
	 */
	const GeologyDeposit *Get(GeologyDepositID id) const;

	/**
	 * Resolve the unique seam containing a playable world tile with bounded candidate/span work.
	 * @param tile Explicit rock-layer tile; co-located layers have independent ownership.
	 * @return Exact deposit ID or invalid, including outside rock layers/void cells.
	 */
	GeologyDepositID Find(TileIndex tile) const;

	/**
	 * Claim a finite seam for one company before a mine or excavation consumer uses its reserve.
	 * @param id Existing nondepleted deposit.
	 * @param company Actual company identity; spectator/none/deity cannot claim ore.
	 * @return Refusal or success; repeated claims by the same company do not debit reserve.
	 */
	GeologyError Claim(GeologyDepositID id, CompanyID company);

	/**
	 * Release company ownership when the last bound producer is removed, preserving physical reserve.
	 * @param id Existing deposit.
	 * @param company Current exclusive claimant.
	 * @return Refusal or success; release cannot replenish a depleted seam.
	 */
	GeologyError ReleaseClaim(GeologyDepositID id, CompanyID company);

	/**
	 * Debit the sole reserve during a complete validated mine/cut transaction.
	 * Excavation reports destroyed ore separately from Earth/Rock; later mines cannot extract it again.
	 * @param id Existing claimed deposit.
	 * @param company Authorized consumer company.
	 * @param amount Tonnes admitted/destroyed by this complete transaction, never a survey/estimate.
	 * @return Refusal or success; insufficient reserve cannot create a partial debit.
	 */
	GeologyError ConsumeReserve(GeologyDepositID id, CompanyID company, uint64_t amount);

	/**
	 * Debit destroyed recoverable ore without manufacturing a permanent mine claim for the cutter.
	 * @param id Actual finite seam touched by the complete cut.
	 * @param company Real cut owner; other-company claims refuse.
	 * @param amount Validated finite loss, independent of Earth/Rock cargo yield.
	 * @param expected_remaining Exact preflight balance, revalidated before coordinated commitment.
	 * @return Complete debit or unchanged refusal; no identity, cache or cargo is created.
	 */
	GeologyError DebitExcavationReserve(GeologyDepositID id, CompanyID company, uint64_t amount, uint64_t expected_remaining);

	/**
	 * Remove an editor/generation record, without reusing its ID or replenishing another seam.
	 * Normal mine demolition uses ReleaseClaim and must retain geology instead.
	 * @param id Existing geological identity.
	 * @return Whether an existing record was removed; invalid identities change nothing.
	 */
	bool Erase(GeologyDepositID id);

	/** Rebuild only the spatial cache from canonical records; no reserve/claim/visibility changes. */
	void RebuildIndex();

	/**
	 * Restore canonical authority and the monotonic cursor, rebuilding addresses/buckets rather than saving them.
	 * @param records Validated bounded records in increasing saved-ID order; deleted IDs may leave holes.
	 * @param saved_next_id Cursor strictly beyond every existing ID, including historical deletion.
	 * @return Refusal or complete replacement; no failure creates a partial geological store.
	 */
	GeologyError RestoreRecords(std::span<const GeologyDeposit> records, uint32_t saved_next_id);

	/** @return Canonical read-only authority for descriptors; addresses/buckets are excluded from serialization. */
	std::span<const GeologyDeposit> Records() const { return this->deposits; }

	/** @return Logical cache key/vector/entry payload bytes; allocator/node bookkeeping is reported separately. */
	size_t IndexPayloadBytes() const;
};

static constexpr uint16_t GEOLOGY_GENERATOR_VERSION = 1; ///< Saved algorithm identity, independent of profile/package/save versions.

/** Stable natural thermal-source identity; invalid is all-ones, independent of mines or utility producers. */
using GeothermalVentID = StrongType::Typedef<uint32_t, struct GeothermalVentIDTag, StrongType::Compare>;
static constexpr uint32_t MAX_GEOTHERMAL_VENT_FIELDS = 256; ///< Bounded natural source records, never per-consumer generated bonuses.

/** Renewable bounded Deep source geometry; discovery/clearing creates neither freight nor an allocated utility grant. */
struct GeothermalVentField {
	GeothermalVentID id{UINT32_MAX}; ///< Stable saved natural identity.
	uint16_t generation_version = 1; ///< Independent versioned vent substream/shape contract.
	MapLayer layer = MapLayer::DeepUnderground; ///< Natural heat exists only on Deep, not at coincident Surface coordinates.
	uint16_t x = 0; ///< Inclusive layer-local shape origin column.
	uint16_t y = 0; ///< Inclusive shape origin row.
	uint8_t width = 4; ///< Bounded rectangular influence/source footprint, at most 16.
	uint8_t height = 4; ///< Bounded source footprint height.
	uint64_t thermal_capacity_w = 0; ///< Physical shared thermal rate ceiling; plant/grid owners allocate it exactly once.
};

struct GeologyWorldState;
struct GeologyGenerationResult;

/**
 * Check the supported geometry budget before starting a new geological world.
 * @param width Map plane width in tiles.
 * @param height Map plane height in tiles.
 * @return Whether all ten seam/mining-approach envelopes and a distinct Deep vent envelope can fit.
 */
bool CheckGeologyGenerationSize(uint32_t width, uint32_t height);

/**
 * Derive version-1 geology seed without consuming any synchronized/interactive RNG.
 * @param world_seed Original synchronized world-generation seed.
 * @return Independent deterministic 64-bit feature root, owned/saved by metadata after publication.
 */
uint64_t DeriveGeologySeed(uint32_t world_seed);

/**
 * Prepare guaranteed seams, then bounded optional seams over the already initialized rock planes.
 * @param geology_seed Independent versioned local seed; no Surface random stream is inspected or changed.
 * @return Complete candidate or refusal, without changing map tiles or accepted live geology.
 */
GeologyGenerationResult PrepareGeologyGeneration(uint64_t geology_seed);

/** @return Read-only accepted geology; absence retains legacy free-ground behavior. */
const GeologyWorldState &GetGeologyWorldState();

/** Reset feature authority before map allocation/load; this never changes the accepted profile owner. */
void ResetGeologyWorldState();

/**
 * Generate and publish only for an accepted Multilayer candidate before company/script startup.
 * @param world_seed Synchronized world-generation input, never a new Random() draw.
 * @return Refusal or complete publication; Classic never allocates rock authority.
 */
GeologyError GenerateWorldGeology(uint32_t world_seed);

/**
 * Hash actual geological/job authority using explicit widths/order, excluding spatial/claim caches and object padding.
 * @param state Actual accepted or validated prepared owner value.
 * @return Portable canonical diagnostic digest; no allocation, mutation or RNG is required.
 */
uint64_t ComputeGeologyStateDigest(const GeologyWorldState &state);

/**
 * Resolve actual natural source geometry without allocating heat or inferring a cross-layer link.
 * @param state Read-only accepted/prepared owner records.
 * @param tile Explicit playable Deep tile.
 * @return Lowest stable-ID overlapping field for planning selection, or nullptr; consumers bind an explicit chosen ID.
 */
const GeothermalVentField *FindGeothermalVentField(const GeologyWorldState &state, TileIndex tile);

/**
 * Validate bounded vent authority independently of mineral overlap.
 * @param fields Canonical increasing-ID fields from the actual geological owner.
 * @param saved_next_id Monotonic cursor beyond every retained source.
 * @return Structural/rate/identity refusal; no field is deleted merely because ore overlaps its influence.
 */
GeologyError ValidateGeothermalVentFields(std::span<const GeothermalVentField> fields, uint32_t saved_next_id);

/** Stable geometry/job identity, allocated only by geology's job owner; all-ones is invalid. */
using ExcavationJobID = StrongType::Typedef<uint32_t, struct ExcavationJobIDTag, StrongType::Compare>;

/** Saved job phase; cancelled/completed cavities and committed outputs never become unfinished reservations again. */
enum class ExcavationJobState : uint8_t { Proposed, Reserved, Working, Paused, Completed, Cancelled };

/** Stable visible refusal/pause reasons, independent of toolbar/camera state. */
enum class ExcavationError : uint8_t {
	None, InvalidTile, MixedLayers, SolidRock, ActiveCut, LockedDeep, InvalidPolicy, InvalidJob,
	Conflict, InvalidOwner, MissingSupport, InvalidRoute, IllegalTurn, OutsideAuthorization,
	InsufficientCapacity, InsufficientFunds, StaleParticipant, AlreadyCompleted, Limit,
	NoProgress, ///< Incomplete synchronized work; no material, money, reserve or terrain transaction occurs.
};

/** Full-footprint refusal with a stable first failing world tile; validation never modifies terrain/RNG/inventory. */
struct ExcavationFootprintResult {
	ExcavationError error = ExcavationError::None; ///< Deterministic refusal reason.
	TileIndex failure_tile = INVALID_TILE; ///< First failing explicit world tile, or invalid on success.
};

/** One authorized advancing segment; engine Direction values preserve the native eight viewport-aligned headings. */
struct ExcavationCutGeometry {
	TileIndex cutter_position = INVALID_TILE; ///< End pose, not permission for a stationary rotation.
	Direction heading = Direction::Invalid; ///< Forward/left45/right45 only; route producer verifies it.
	std::vector<TileIndex> swept_tiles; ///< Complete cutter/consist envelope, including diagonal corners.
	std::vector<TileIndex> support_tiles; ///< Required backed retreat/support, separate from final requested floor.
	/** @param other Segment to compare. @return Whether pose, heading and both envelopes match. */
	bool operator==(const ExcavationCutGeometry &other) const = default;
};

/** Frozen geometric pass/return/capacity terms; these are planned limits, never another wagon inventory or progress cursor. */
struct ExcavationPassTerms {
	uint64_t earth_capacity_t = 64; ///< Explicit proposed Earth wagon space; actual participants still revalidate current capacity.
	uint64_t rock_capacity_t = 128; ///< Independent proposed Rock space, never borrowed from Earth.
	std::vector<TileIndex> exchange_path; ///< Ordered supported backing from commissioning anchor to the actual exchange/pocket anchor.
	std::vector<uint16_t> exchange_after; ///< Complete cut counts at which loaded backing/exchange is required; final return included.
	uint64_t fingerprint = 0; ///< Static route/authorization/return/capacity proof, excluding mutable progress and stock.
	/** @param other Terms to compare. @return Whether every frozen field matches. */
	bool operator==(const ExcavationPassTerms &other) const = default;
};

/**
 * Public saved job layout; Groups 10/10B own reservation/control/persistence.
 * Geometry owns route/claims/progress. TBM controllers own actual vehicles, wagons, funds and running costs.
 * Statistical committed totals cannot be unloaded or treated as another cargo inventory.
 */
struct ExcavationJob {
	ExcavationJobID id{UINT32_MAX}; ///< Stable owner-allocated job identity.
	CompanyID company = CompanyID::Invalid(); ///< Saved real owner; invalid removal causes recoverable pause/cancel.
	VehicleID cutter = VehicleID::Invalid(); ///< Actual controller reference, never a geological surrogate vehicle.
	StationID origin = StationID::Invalid(); ///< Actual exchange/origin reference resolved by its owner.
	uint16_t model_id = UINT16_MAX; ///< Frozen logical cutter capability, mapped to engines by the later TBM owner.
	TileIndex commissioning_anchor = INVALID_TILE; ///< Explicit portal work pocket, never general clear ground.
	ExcavationJobState state = ExcavationJobState::Proposed; ///< One phase owner.
	ExcavationError pause_reason = ExcavationError::None; ///< Visible recoverable reason.
	std::vector<TileIndex> requested_tiles; ///< Canonical final floor area, independent of intended rail/road/tube/mine use.
	std::vector<TileIndex> authorized_tiles; ///< Explicit accepted floor plus charged approach/turn work; no silent outside cuts.
	std::vector<ExcavationCutGeometry> route; ///< Accepted bounded advancing/support geometry.
	uint32_t cursor = 0; ///< Sole next-cut cursor; a TBM project holds a job reference instead of a duplicate cursor.
	uint64_t partial_work_ticks = 0; ///< Saved synchronized partial work, never render/wall time.
	uint64_t revision = 0; ///< Reservation/route revision invalidating stale cut proposals.
	uint64_t completed_earth_t = 0; ///< Historical committed statistic, not cargo custody.
	uint64_t completed_rock_t = 0; ///< Historical committed statistic, not a hidden buffer.
	Money completed_work_cost = 0; ///< Historical paid-cut total; actual cash remains with the company/controller.
	std::optional<ExcavationPassTerms> pass_terms; ///< Frozen validated backing/capacity input; legacy absence cannot authorize a complete cut.
	/** @param other Job to compare. @return Whether every authoritative field matches, for revalidating an unchanged proposal. */
	bool operator==(const ExcavationJob &other) const = default;
};

/** Sole live geological authority; transient generation proposals use the same value layout before publication. */
struct GeologyWorldState {
	uint16_t generator_version = 0; ///< Zero means absent legacy geology; never retroactively generate solid rock on load.
	PackedExcavationState terrain; ///< Independent packed rock-plane state.
	GeologyDepositStore deposits; ///< Finite sparse reserves/claims and rebuilt lookup.
	std::vector<GeothermalVentField> vents; ///< Stable natural thermal sources, independent of mineral reserves.
	uint32_t next_vent_id = 0; ///< Monotonic natural identity cursor, not a utility producer allocator.
	std::vector<ExcavationJob> jobs; ///< Increasing stable-ID saved route/progress records.
	uint32_t next_job_id = 0; ///< Saved job allocator cursor; completed/cancelled IDs are never reused.
	std::map<TileIndex, ExcavationJobID> job_claims; ///< Rebuilt exclusive active-footprint cache, never serialized authority.
};

/** Fully prepared candidate or explicit refusal; a failed proposal never publishes partial mandatory deposits. */
struct GeologyGenerationResult {
	GeologyError error = GeologyError::None; ///< Unsupported space/content/storage is reported before activation.
	GeologyWorldState state; ///< Candidate authority, independent of the live world until complete.
};

static constexpr uint32_t MAX_EXCAVATION_JOBS = 128; ///< Bounded sparse history/live jobs, independent of map size.
static constexpr uint32_t MAX_EXCAVATION_JOB_TILES = 4096; ///< One canonical authorized area.
static constexpr uint32_t MAX_EXCAVATION_ROUTE = 1024; ///< One accepted bounded advancing pass.
static constexpr uint32_t MAX_EXCAVATION_CLAIMS = 65536; ///< Maximum exclusive active cells across the world.
static constexpr uint64_t MAX_EXCAVATION_WORK_TICKS = 1000000; ///< Bounded synchronized partial work, never wall-clock time.

/** Reservation result; an estimated identity is not allocated until execute succeeds. */
struct ExcavationJobResult {
	ExcavationError error = ExcavationError::None; ///< Complete refusal/success.
	ExcavationJobID id{UINT32_MAX}; ///< Actual allocated job on execute; prospective next identity on estimate.
	TileIndex failure_tile = INVALID_TILE; ///< First stable geometry/claim refusal location.
};

/** Actual controller's read-only physical support participant; geology does not look through private train pools. */
class ExcavationWorkParticipant {
public:
	/** Destroy through the shared support boundary. */
	virtual ~ExcavationWorkParticipant() = default;
	/**
	 * Check the controller's real support/retreat and bound vehicle/origin references.
	 * @param job Actual accepted route/progress record.
	 * @param cut Exact next segment whose supports must remain usable.
	 * @return True only for actual available support; false produces a recoverable pause without paid outputs.
	 */
	virtual bool IsSupported(const ExcavationJob &job, const ExcavationCutGeometry &cut) const = 0;
};

/**
 * Validate bounded canonical route geometry, independently of a TBM runtime/controller.
 * @param geology Read-only actual terrain/deposit owner.
 * @param job Canonical proposed or restored record.
 * @return Stable route/layer/authorization refusal; this function allocates no identity or claim.
 */
ExcavationFootprintResult ValidateExcavationJobGeometry(const GeologyWorldState &geology, const ExcavationJob &job);

/**
 * Rebuild exclusive unfinished-cut lookup and validate all active packed cells before publication.
 * @param geology Complete prepared restoration; its old cache changes only on full success.
 * @return Specific refusal; jobs and terrain remain canonical saved authority.
 */
GeologyError RebuildExcavationJobClaims(GeologyWorldState &geology);

/**
 * Validate optional bound references through their real public owners, without assuming an unreleased TBM engine mapping.
 * @param job Actual sparse geometry record; invalid cutter/origin denotes an explicit unbound reservation.
 * @return Whether references resolve, or the job is explicitly paused with no company after owner removal.
 */
bool ValidateExcavationJobReferences(const ExcavationJob &job);

/**
 * Reserve one complete proposed route through the live geological owner.
 * @param proposal Unallocated canonical route, owned by the current real company.
 * @param execute False validates/estimates only; true allocates the fresh ID and exclusive cells once.
 * @return Complete reservation or unchanged refusal; actual TBM binding/command UI remain Plan 4.
 */
ExcavationJobResult ReserveExcavationJob(const ExcavationJob &proposal, bool execute);

/**
 * Read one actual sparse job without exposing mutable progress or another cursor owner.
 * @param id Exact typed identity.
 * @return Actual owner record or nullptr; references expire at the next job-store mutation.
 */
const ExcavationJob *GetExcavationJob(ExcavationJobID id);

/**
 * Accumulate synchronized work only while the actual company and external support remain valid.
 * @param id Exact accepted job identity.
 * @param ticks Positive simulation work supplied by its controller, never render/wall time.
 * @param participant Actual read-only support owner; loss pauses without material/cost writes.
 * @return Success or visible recoverable pause/refusal; complete-cut outputs are exclusively Group 11.
 */
ExcavationError AdvanceExcavationWork(ExcavationJobID id, uint64_t ticks, const ExcavationWorkParticipant &participant);

/**
 * Pause accepted unfinished work without releasing its reservation or changing committed history.
 * @param id Exact owned job.
 * @param reason Visible non-success reason; the current owner/deity initiates this command-side action.
 * @return Success or unchanged identity/permission refusal.
 */
ExcavationError PauseExcavationJob(ExcavationJobID id, ExcavationError reason);

/**
 * Cancel only unfinished claimed cells, preserving existing cavities and committed statistics.
 * @param id Exact owned job; only its current company/deity may cancel.
 * @return Success or unchanged refusal; no cash or cargo refund is created.
 */
ExcavationError CancelExcavationJob(ExcavationJobID id);

/**
 * Release closed sparse history after the actual controller removes its dependent project references.
 * @param id Exact completed/cancelled owned job; live work refuses.
 * @return Complete retirement or unchanged refusal; IDs are never reused and no money/material is refunded.
 */
ExcavationError RetireExcavationJob(ExcavationJobID id);

/** Read-only actual material/funds participant snapshot supplied by the TBM owner; geology never walks private wagon pools. */
struct ExcavationMaterialSnapshot {
	uint64_t revision = 0; ///< External owner stamp revalidated immediately before irreversible writes.
	uint64_t earth_space_t = 0; ///< Explicit attached permitted-wagon capacity, independent of Rock space.
	uint64_t rock_space_t = 0; ///< Rock capacity cannot absorb Earth or create an invisible hold.
	uint32_t support_units = 0; ///< Actual available supports/materials under that owner's authority.
	Money available_funds = 0; ///< Actual caller/company affordability snapshot, never another cash balance owner.
};

/** Explicit recoverable ore destroyed by a complete cut; this is a reserve debit, never freight cargo or hidden spoil inventory. */
struct ExcavationReserveDebit {
	GeologyDepositID deposit{UINT32_MAX}; ///< Sole finite owner.
	CargoLabel resource{}; ///< Meaning of the lost recoverable seam, preserved for the result ledger.
	uint64_t tonnes = 0; ///< Finite recoverable loss clipped to the actual remaining balance.
	uint64_t expected_remaining = 0; ///< Actual preflight reserve stamp; rebuilt cache addresses are excluded.
	/** @param other Complete authoritative debit to compare. @return Whether all captured fields match. */
	bool operator==(const ExcavationReserveDebit &other) const = default;
};

/** Exact complete-cut proposal/receipt; partial or refused work carries no generated material or charge. */
struct ExcavationCutResult {
	ExcavationError error = ExcavationError::None; ///< No-progress/refusal or complete proposal.
	TileIndex failure_tile = INVALID_TILE; ///< Stable failing location.
	uint32_t new_cells = 0; ///< Newly solid cells only; completed cavities cannot yield twice.
	uint64_t earth_t = 0; ///< Whole Earth delivery required to fit the real sink.
	uint64_t rock_t = 0; ///< Independent whole Rock delivery.
	uint32_t support_units = 0; ///< Exact charged support requirement.
	Money cost = 0; ///< Complete work/support charge, distinct from external running costs.
	uint64_t work_ticks = 0; ///< Required synchronized geological work.
	std::vector<ExcavationReserveDebit> reserve_debits; ///< Actual seam loss, separate from fixed Earth/Rock spoil yields.
	/** @param other Complete captured proposal to compare. @return Whether all authoritative receipt fields match. */
	bool operator==(const ExcavationCutResult &other) const = default;
};

/** Exact job/revision/cursor request; the authoritative accepted route supplies geometry, not client-provided tile permission. */
struct ExcavationCutRequest {
	ExcavationJobID job{UINT32_MAX}; ///< Actual reserved job.
	uint64_t job_revision = 0; ///< Expected validated route/claim revision.
	uint32_t cursor = 0; ///< Expected next complete segment; no replay of earlier outputs.
};

/**
 * External transaction participant owned by the actual TBM/controller.
 * Implementations validate real supports/wagons/funds and commit them once with geology;
 * failed commits must leave every external quantity unchanged. Operational integration remains Plan 4/5.
 */
class ExcavationMaterialParticipant {
public:
	/** Destroy through the shared transaction interface. */
	virtual ~ExcavationMaterialParticipant() = default;
	/** @return Read-only current capacity/material/funds identity snapshot, with no inventory/RNG mutation. */
	virtual ExcavationMaterialSnapshot Capture() const = 0;
	/**
	 * Revalidate the actual external owner immediately before a complete coordinated commit.
	 * @param expected Snapshot used by pure preflight.
	 * @param delta Complete proposed material/support/cost change.
	 * @return Whether all actual participants still accept exactly this transaction; validation changes nothing.
	 */
	virtual bool Validate(const ExcavationMaterialSnapshot &expected, const ExcavationCutResult &delta) const = 0;
	/**
	 * Commit the already revalidated actual materials/funds once, without partial-failure writes.
	 * @param expected Validated external owner/revision.
	 * @param delta Complete material/support/cost receipt.
	 * @return Success, or unchanged failure; controller guarantees no-throw atomic participant behavior.
	 */
	virtual bool Commit(const ExcavationMaterialSnapshot &expected, const ExcavationCutResult &delta) noexcept = 0;
};

/**
 * Validate every caller-supplied footprint/approach tile against accepted live policy and geological authority.
 * @param footprint Explicit world tiles, not a camera-relative rectangle.
 * @return Stable first refusal without changing terrain, funds, stock or RNG; behavior is produced in Group 8.
 */
ExcavationFootprintResult CheckExcavatedFootprint(std::span<const TileIndex> footprint);

/**
 * Validate an explicit owner snapshot for pure geometry/technology tests and command estimates.
 * @param geology Actual owner snapshot, never a copied foreign tile/cargo registry.
 * @param policy Explicit saved profile policy.
 * @param calendar_date Synchronized typed game-calendar date.
 * @param footprint Complete ordered world footprint/approaches.
 * @return First refusal with no mutation; behavior belongs to Group 8.
 */
ExcavationFootprintResult CheckExcavatedFootprint(const GeologyWorldState &geology, const MultiLayerProfileSelection &policy, TimerGameCalendar::Date calendar_date, std::span<const TileIndex> footprint);

/**
 * Restricted commissioning geometry issued by the portal/TBM owner, never general construction permission.
 * Plan 3/4 must bind this record to their real commissioned vehicle before issuing it.
 * The ordinary footprint predicate deliberately has no pocket parameter.
 */
struct ExcavationCommissioningPocket {
	VehicleID assigned_cutter = VehicleID::Invalid(); ///< Only the actual assigned TBM may occupy this pocket.
	TileIndex anchor = INVALID_TILE; ///< Explicit rock-layer portal anchor.
	std::vector<TileIndex> tiles; ///< At most nine cells inside the anchor's conservative 3x3 envelope.
};

/**
 * Validate restricted commissioning occupancy without excavating or granting ordinary construction rights.
 * @param pocket Producer-issued geometry bound to its actual commissioned TBM.
 * @param actor Actual vehicle supplied by that owner, never a company-wide bypass.
 * @param tile Requested occupancy cell.
 * @return Whether this exact assigned actor may occupy that pocket cell; issuance remains Plan 3/4.
 */
bool CheckExcavationCommissioningPocket(const ExcavationCommissioningPocket &pocket, VehicleID actor, TileIndex tile);

/**
 * Derive the conservative 3x3 cutter envelope centred on one tile of its own plane.
 * The portal pocket, the portal's source support and the solid-starter rules all use this one geometry.
 * @param centre Explicit world tile, for example a commissioning anchor housing.
 * @return Nine cells in ascending tile order, or empty when the envelope would wrap or leave the plane.
 */
std::vector<TileIndex> GetExcavationCutterEnvelope(TileIndex centre);

/**
 * Solid commissioning start, resolved through the issuing portal owner. Geometry and permission only:
 * it never grants excavation credit, cleared floor or vehicle admission. It applies while the anchor
 * housing's own rock cell is still unopened (Solid before reservation, Excavating until cut 0 commits).
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-f-solid-starter-contract.md
 */
struct ExcavationStarterResult {
	ExcavationError error = ExcavationError::None; ///< None when no solid start applies, or when its issuer is current.
	TileIndex failure_tile = INVALID_TILE; ///< The anchor on refusal.
	bool required = false; ///< True while the anchor cell is unopened, so cut 0 must open and pay for the whole pocket.
	Direction exit_heading = Direction::Invalid; ///< Only legal first advancing heading: out through the housing door.
	std::vector<TileIndex> pocket; ///< Restricted occupancy envelope that cut 0 must sweep; never initial floor.
};

/**
 * Decide whether a job or proposal starts inside a solid portal pocket and, if so, revalidate its issuing pair.
 * @param job Proposal with an invalid ID (its pair must still be Sealed and unassigned) or a published job
 *            (its pair must be Commissioning exactly this job).
 * @return No solid start, a current issuer with exit heading and pocket, or a stable refusal at the anchor.
 *         Reads only; terrain, claims, pair state, funds and RNG are unchanged.
 */
ExcavationStarterResult ResolveExcavationStarter(const ExcavationJob &job);

/**
 * Check the first advancing segment of a solid start against its issuer's geometry.
 * @param job Canonical route; route[0] must leave the housing.
 * @param exit_heading Door heading supplied by the issuing portal.
 * @param pocket Envelope from GetExcavationCutterEnvelope around the anchor.
 * @return Success, or a stable refusal when route[0] does not face the door, misses a pocket cell
 *         (which would leave that rock unpaid) or claims support outside the pocket.
 */
ExcavationFootprintResult CheckExcavationStarterGeometry(const ExcavationJob &job, Direction exit_heading, std::span<const TileIndex> pocket);

/**
 * Adapt geological footprint refusal to the ordinary construction command boundary.
 * @param footprint Complete explicit footprint and approaches within one layer.
 * @return Empty command cost on success, otherwise the stable geological error message.
 */
CommandCost CheckExcavationConstruction(std::span<const TileIndex> footprint);

/**
 * Preflight a complete rectangle before any automatic clearing or tile creation.
 * @param tile Explicit world-tile origin.
 * @param width Positive layer-local width.
 * @param height Positive layer-local height.
 * @param entrance Outward approach direction, or Invalid for none.
 * @param through Also require the opposite approaches for through platforms/stops.
 * @return Complete-footprint permission or unchanged refusal; coordinates cannot wrap planes.
 */
CommandCost CheckExcavationConstructionArea(TileIndex tile, uint width, uint height, DiagDirection entrance = DiagDirection::Invalid, bool through = false);

/**
 * Check a depot/end and its entrance before building or rotating it.
 * @param tile Explicit footprint tile.
 * @param entrance Direction from the building to its approach.
 * @return Permission for both cells, preserving legacy non-geological behavior.
 */
CommandCost CheckExcavationConstructionEntrance(TileIndex tile, DiagDirection entrance);

/**
 * Paint scenario-author terrain only behind the saved explicit override and actual editor mode.
 * @param footprint Complete explicit rock-layer cells; active cuts and occupied infrastructure refuse.
 * @param excavated Select open floor (true) or solid rock (false), never a job reservation.
 * @param execute False validates only; true applies the already validated whole footprint.
 * @return Stable refusal or success; no spoil, reserve, cash or RNG is created by editor painting.
 */
ExcavationFootprintResult PaintExcavationTerrain(std::span<const TileIndex> footprint, bool excavated, bool execute);

/**
 * Commit one authoritative job cut together with its actual external material/funds owner.
 * @param request Exact job/revision/cursor, revalidated before any writes.
 * @param participant Controller-owned capacity, material and cash transaction.
 * @return Complete receipt or unchanged refusal; Group 11 produces and tests this behavior.
 */
ExcavationCutResult CommitExcavationCut(const ExcavationCutRequest &request, ExcavationMaterialParticipant &participant);

/**
 * Compute a complete cut quote without consuming work, terrain, ore, supports, stock, cash or RNG.
 * @param request Exact accepted job/revision/cursor.
 * @param material Actual external owner capacity/support/funds snapshot.
 * @return Complete proposal or zero-quantity refusal; work readiness is checked separately by commit.
 */
ExcavationCutResult PrepareExcavationCut(const ExcavationCutRequest &request, const ExcavationMaterialSnapshot &material);

/**
 * Cheap, refusal-free estimate of the job's next cut: the same per-cell terms PrepareExcavationCut charges, summed over
 * the cut's still-unopened swept cells. It lets a controller bound its per-tick work and name a precise material wait
 * without running the full geometry validation every tick. Commit still prepares and validates the cut completely.
 * @param id Exact job.
 * @return New cells, both spoil quantities, support units, cost and total work ticks (reserve debits are not
 *         estimated); InvalidJob or AlreadyCompleted when there is no next cut.
 */
ExcavationCutResult EstimateExcavationCut(ExcavationJobID id);

/** Canonical immutable planning proposal; intended road/rail/tube/mine use is deliberately absent from geometry acceptance. */
struct ExcavationOutline {
	ExcavationError error = ExcavationError::None; ///< Complete geometry/technology refusal.
	TileIndex failure_tile = INVALID_TILE; ///< Stable canonical first failing location.
	MapLayer layer = MapLayer::Invalid; ///< Explicit rock-layer domain.
	std::vector<TileIndex> requested_tiles; ///< Canonical final floor, independent of infrastructure type.
	std::vector<TileIndex> authorized_tiles; ///< Explicit accepted working domain; not automatic terrain permission.
	std::vector<TileIndex> extra_tiles; ///< Exactly the accepted working space beyond final requested floor.
	ExcavationCutResult extra_estimate; ///< Read-only bounded extra spoil/support/time quote, recomputed by actual cut execution.
};

/**
 * Normalize a connected final floor without allocating claims, changing terrain or guessing its final land use.
 * @param layer Explicit Underground/Deep layer, independent of viewport selection.
 * @param tiles User corridor/branch/room cells; duplicates normalize in stable world-tile order.
 * @return Canonical final/authorized floor or stable empty refusal.
 */
ExcavationOutline NormalizeExcavationOutline(MapLayer layer, std::span<const TileIndex> tiles);

/**
 * Publish only caller-accepted extra geometry and its read-only cost, never silently adding a planner's turn/approach cells.
 * @param outline Canonical requested final floor, revalidated before use.
 * @param accepted_extra Explicit accepted approach/turn/working cells.
 * @param model_id Immutable logical TBM design for the quote; actual binding remains with its controller.
 * @return Complete requested/authorized/extra sets and quote, or unchanged refusal.
 */
ExcavationOutline AuthorizeExcavationOutline(const ExcavationOutline &outline, std::span<const TileIndex> accepted_extra, uint16_t model_id);

/**
 * Quote arbitrary bounded new working cells through the same frozen hardness terms as actual cuts.
 * @param tiles Complete canonical rock cells being previewed; open cells cost/yield nothing.
 * @param model_id Immutable TBM design whose layer/hardness capability must apply.
 * @return Read-only whole-cell support/spoil/work estimate or zero-quantity refusal; no claim or inventory is created.
 */
ExcavationCutResult QuoteExcavationCells(std::span<const TileIndex> tiles, uint16_t model_id);

/**
 * Cutting difficulty of one rock cell, by the same rule quotes and cuts use (deposit hardness, else layer default).
 * @param geology Read-only geological owner.
 * @param tile Rock-layer cell.
 * @return The cell's hardness.
 */
GeologyHardness GetExcavationCellHardness(const GeologyWorldState &geology, TileIndex tile);

/** Explicit planning pose and minimum consist history; points are geometry, never invented vehicle identities. */
struct ExcavationPlanningPose {
	TileIndex position = INVALID_TILE; ///< Cutter centre in the explicit rock layer.
	Direction heading = Direction::Invalid; ///< One native advancing heading.
	std::vector<TileIndex> history; ///< Head first, then ordered occupied/support centres for the planned consist; 3..16 points.
};

/** One complete advancing geometry transition; stationary/reversing rotations have no representation. */
struct ExcavationPlanningStep {
	ExcavationError error = ExcavationError::None; ///< Full shape/boundary refusal.
	TileIndex failure_tile = INVALID_TILE; ///< Stable failed world location.
	ExcavationPlanningPose next; ///< Shifted head/consist history after the legal advance.
	ExcavationCutGeometry cut; ///< Conservative cutter/diagonal/consist sweep and old backed support.
};

/** Deterministic operation bounds; elapsed CPU/wall time never selects different multiplayer routes. */
struct ExcavationPlanningLimits {
	uint32_t states = 4096; ///< Maximum nodes retained for one target search; hard cap 8192.
	uint32_t expansions = 8192; ///< Shared command budget across all selected targets; hard cap 32768.
	uint32_t depth = 256; ///< Maximum advancing segments in one joined target pass.
	uint32_t segments = MAX_EXCAVATION_ROUTE; ///< Maximum total accepted proposal segments.
};

/** Read-only coverage proposal; a visible partial route is not an accepted full-floor job or terrain mutation. */
struct ExcavationRouteProposal {
	ExcavationError error = ExcavationError::None; ///< Complete success, visible bounded partial, or refusal.
	TileIndex failure_tile = INVALID_TILE; ///< Uncovered/refused target or geometry location.
	bool partial = false; ///< Legal prefix needs explicit user acceptance/revised outline; never silently marks the full room complete.
	uint32_t expansions = 0; ///< Exact deterministic operation budget consumed.
	std::vector<ExcavationCutGeometry> route; ///< Bounded legal advancing geometry, no claims or physical movement.
	std::vector<TileIndex> covered_targets; ///< Requested floor actually opened/already open by the proposal.
	std::vector<TileIndex> remaining_targets; ///< Explicit unresolved floor.
	ExcavationCutResult estimate; ///< Whole proposal's independent spoil/support/time, recomputed at execution.
	ExcavationPlanningPose end; ///< Actual proposed final pose/history.
};

/**
 * Construct one complete legal advancing cutter/consist sweep without changing world state.
 * @param pose Explicit current pose and head-first support history.
 * @param next_heading Straight/left45/right45 direction; every accepted transition advances one tile.
 * @return Complete conservative geometry or unchanged shape/boundary refusal.
 */
ExcavationPlanningStep AdvanceExcavationPlanningPose(const ExcavationPlanningPose &pose, Direction next_heading);

/**
 * Plan stable target coverage inside only the explicitly authorized working domain.
 * @param outline Canonical requested/accepted working floor from Group 12.
 * @param start Actual supported cutter pose and planned consist history.
 * @param model_id Immutable TBM layer/hardness capability.
 * @param limits Deterministic per-target/state/depth and whole-command work bounds.
 * @return Legal full/visible partial proposal or refusal; physical driving remains Plan 4.
 */
ExcavationRouteProposal PlanExcavationRoute(const ExcavationOutline &outline, const ExcavationPlanningPose &start, uint16_t model_id, const ExcavationPlanningLimits &limits = {});

/** Complete readonly pass validation; returned terms become actual saved job input only through the reservation owner. */
struct ExcavationPassResult {
	ExcavationError error = ExcavationError::None; ///< Unsupported retreat/capacity/ownership refusal.
	TileIndex failure_tile = INVALID_TILE; ///< Stable first failure.
	ExcavationPassTerms terms; ///< Frozen proposed wagon/return limits with deterministic complete-cut exchange breakpoints.
};

/**
 * Validate every forward prefix's supported backing and independent complete-cut spoil capacity.
 * @param job Proposed canonical geometry, before claiming any solid cell.
 * @param terms Explicit proposed capacities and anchor-to-exchange path; an empty path selects the commissioning anchor.
 * @return Complete frozen input or unchanged refusal; no route/fund/cargo authority is invented.
 */
ExcavationPassResult PrepareExcavationPass(const ExcavationJob &job, const ExcavationPassTerms &terms);

/**
 * Validate frozen input structure without recomputing spoil from already completed ground.
 * @param job Actual canonical saved/proposed job.
 * @param terms Frozen accepted backing/capacity terms.
 * @return Stable unchanged refusal or structurally valid terms.
 */
ExcavationFootprintResult ValidateExcavationPassTerms(const ExcavationJob &job, const ExcavationPassTerms &terms);

/**
 * Fingerprint static pass inputs using actual saved fields, independent of cursor/stock/cache addresses.
 * @param job Canonical accepted/proposed route and authorized floor.
 * @param terms Explicit backing/capacity/exchange terms; fingerprint member is excluded from its own calculation.
 * @return Portable proof recomputed on reserve/load/execution.
 */
uint64_t ComputeExcavationPassFingerprint(const ExcavationJob &job, const ExcavationPassTerms &terms);

/**
 * Derive actual supported reverse centre path without creating a backward cut or changing cutter orientation.
 * @param job Canonical saved route and validated terms.
 * @param completed_cuts Prefix count at which the cutter backs to the exchange/pocket.
 * @return Reverse route followed by the explicit exchange path, or empty for invalid/unvalidated input.
 */
std::vector<TileIndex> GetExcavationRetreatPath(const ExcavationJob &job, uint32_t completed_cuts);

/** Actual controller/haulage owner's withdrawal check; geology never reads private cutter/hauler pools to manufacture clearance. */
class ExcavationWithdrawalParticipant {
public:
	/** Destroy through the explicit controller boundary. */
	virtual ~ExcavationWithdrawalParticipant() = default;
	/**
	 * Check actual cutter and hauler withdrawal before replacing support.
	 * @param job Actual saved job whose work rail is proposed for removal.
	 * @param removed_support Exact support cells to change.
	 * @return True only after both actual actors have withdrawn; this check mutates nothing.
	 */
	virtual bool HasWithdrawn(const ExcavationJob &job, std::span<const TileIndex> removed_support) const = 0;
};

/**
 * Validate replacement support after actual withdrawal, preserving an explicit alternative backed route.
 * @param job Actual accepted job.
 * @param removed_support Work support being replaced, for example by a final tube corridor.
 * @param alternative_backing Ordered explicit replacement route from the current cutter centre to exchange.
 * @param participant Actual cutter/hauler withdrawal owner.
 * @return Read-only stable refusal/success; no support or terrain is removed by this predicate.
 */
ExcavationFootprintResult CheckExcavationSupportReplacement(const ExcavationJob &job, std::span<const TileIndex> removed_support, std::span<const TileIndex> alternative_backing, const ExcavationWithdrawalParticipant &participant);

#endif /* EXCAVATION_H */
