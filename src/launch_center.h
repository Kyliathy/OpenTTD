/*
 * This file is part of OpenTTD, licensed under the GNU GPL version 2.
 */
/**
 * @file launch_center.h Native launch-center placement, Surface handling and orbital descent contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups1-7/7B/12-14.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; immutable descent sender/mass provenance survives the original hull.
 * Station owns the immutable anchor, hub links and Surface jobs; SpacePod owns
 * descent packets. Saved records contain packet references, never another packet
 * list. Empty or stale generation-qualified references confer no authority.
 */
#ifndef LAUNCH_CENTER_H
#define LAUNCH_CENTER_H
#include "tilearea_orthogonal.h"
#include "space.h"
#include "station_type.h"
#include "cargopacket.h"
#include "space_pod_type.h"
#include "vehicle_type.h"
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <vector>

struct Station;
struct SpaceCraftRef;
struct SpacePodRecord;
struct SpaceManifestRecord;
struct LaunchCenterRecord;

/** Native Station owns this metadata. Joining tiles never moves the build anchor. */
struct LaunchCenterAnchor {
	TileIndex tile = INVALID_TILE;
	uint64_t generation = 0; ///< Monotonic world identity; a reused StationID is a different center.
	uint8_t rotation = 0;
};

/** Station identity plus monotonic center generation; station joining never changes this pair. */
struct LaunchCenterRef {
	StationID station = StationID::Invalid();
	uint64_t generation = 0;
	/** @param other Reference to compare, including its generation. @return Whether both identify the same center. */
	bool operator==(const LaunchCenterRef &other) const = default;
};

/** Saved resumable descent failures; blocked cargo remains owned by its paid pod. */
enum class SpaceDescentBlock : uint8_t { None, TargetLost, StorageFull, CompanyRemoved, End };
inline constexpr uint16_t SPACE_DESCENT_LEGACY_SENDER_MODEL = UINT16_MAX; ///< Retained pre434 unknown role; it carries only the original conservative20t rights.
/** Owned by the one paid passive pod until its cargo reaches actual Surface inventory. */
struct SpaceDescentJob {
	/** Construct an empty owner; commit installs its identity, reservations and actual packets together. */
	SpaceDescentJob() = default;
	/** @param other Packet owner whose copying is forbidden to prevent duplicate custody. */
	SpaceDescentJob(const SpaceDescentJob &other) = delete;
	/** @param other Packet owner whose assignment is forbidden to prevent duplicate custody. */
	SpaceDescentJob &operator=(const SpaceDescentJob &other) = delete;
	uint64_t id = 0;
	LaunchCenterRef center;
	SpacePoint release_point{};
	VehicleID sender = VehicleID::Invalid();
	uint64_t sender_generation = 0;
	uint16_t sender_model = SPACE_DESCENT_LEGACY_SENDER_MODEL; ///< Immutable actual release role, retained after sender loss; old receipts stay explicitly unknown.
	uint8_t unit_weight = 0; ///< Actual CargoSpec weight at release; zero only for unchanged legacy-unknown provenance.
	uint16_t freight_mass = 0; ///< Complete released mass in sixteenths; quantity/weight arithmetic remains independently checkable after sender loss.
	CargoType cargo = INVALID_CARGO;
	uint16_t quantity = 0, remaining_ticks = 0, age_ticks = 0;
	SpaceDescentBlock blocked = SpaceDescentBlock::None;
	StationCargoList packets{};
};
/** Copied SPDJ transport; packet IDs and next hops refer to the pod's single live cargo owner. */
struct SpaceDescentRecord {
	SpacePodRef pod;
	uint64_t id = 0;
	LaunchCenterRef center;
	SpacePoint release_point{};
	VehicleID sender = VehicleID::Invalid();
	uint64_t sender_generation = 0;
	uint16_t sender_model = SPACE_DESCENT_LEGACY_SENDER_MODEL; ///< Copied immutable release role; numeric vehicle reuse grants no new authority.
	uint8_t unit_weight = 0; ///< Copied actual release dimension, never borrowed from the open world's CargoSpecs.
	uint16_t freight_mass = 0; ///< Copied total release mass; this is provenance, not a second packet/cargo owner.
	CargoType cargo = INVALID_CARGO;
	uint16_t quantity = 0, remaining_ticks = 0, age_ticks = 0;
	SpaceDescentBlock blocked = SpaceDescentBlock::None;
	std::vector<std::pair<CargoPacketID, StationID>> packets;
};
/** Query the immutable mass allowance retained by a descent after its sender disappears.
 * @param model Actual saved release role, or explicit old unknown provenance.
 * @return Sixteenth-tonne bound for15/20/21 or conservative legacy history; zero denies every unrelated role.
 */
uint16_t GetSpaceDescentMassLimit(uint16_t model);
/** Validate a detached immutable descent receipt without consulting native pools or active-world cargo definitions.
 * @param model Actual release role, or explicit retained legacy-unknown marker.
 * @param quantity Whole positive released units, bounded by the original40-unit wire/work contract.
 * @param unit_weight Original physical unit dimension; zero only for the old unknown receipt.
 * @param freight_mass Original complete mass; known receipts must equal quantity times unit weight and fit the exact role.
 * @return Whether role, dimensions and historical mass are coherent; actual native/copy owners separately validate packet custody.
 */
bool IsSpaceDescentReceiptValid(uint16_t model, uint16_t quantity, uint8_t unit_weight, uint16_t freight_mass);
/**
 * Preflight paid-shell release without moving cargo, charging income or reserving twice.
 * Checks exact identities, company, compatible deployed hull, current Q16 position,
 * mass, onboard shell, service quota, recovery slots/headroom and packet split capacity.
 * @param craft Generation-qualified native spacecraft holding the offered cargo.
 * @param center Explicit authorized target; overlapping circles never choose one implicitly.
 * @param pod Paid passive shell uniquely carried by this craft.
 * @param quantity Positive whole cargo units offered from the craft's current cargo type.
 * @return Whether release can commit now; refusal preserves every owner and ledger.
 */
bool CanReleaseOrbitalCargo(SpaceCraftRef craft, LaunchCenterRef center, SpacePodRef pod, uint16_t quantity);
/**
 * Revalidate and move the offered packets once into the paid pod's saved descent owner.
 * Retains release position and exact target generation, reserves recovery headroom
 * and one pending return, and earns no freight income. Call through the synchronized
 * command boundary; this owner-level operation does not check the acting company.
 * @param craft Generation-qualified native spacecraft relinquishing cargo custody.
 * @param center Explicit authorized recovery center.
 * @param pod Paid shell becoming the single descent packet owner.
 * @param quantity Positive whole units to move; identical to release preflight units.
 * @return Whether the transfer committed; refusal leaves the original cargo owner intact.
 */
bool CommitOrbitalCargoRelease(SpaceCraftRef craft, LaunchCenterRef center, SpacePodRef pod, uint16_t quantity);
/**
 * Preflight explicit recovery diversion using the original release position and retained packets.
 * @param pod Generation-qualified paid pod with an existing descent job.
 * @param center Authorized replacement recovery center with available slots and stock headroom.
 * @return Whether the synchronized retarget command may change this job's target.
 */
bool CanRetargetSpaceDescent(SpacePodRef pod, LaunchCenterRef center);
/** Age held packets and advance independent descent work once per simulation tick; completion transfers once. */
void TickSpaceDescents();
/** @param center Lost exact center identity; dependent jobs retain packets and receive TargetLost. */
void ForgetSpaceDescentCenter(LaunchCenterRef center);
/** @param center Exact recovery target. @param cargo Runtime cargo type. @return Units reserved by its saved descents. */
uint GetIncomingSpaceDescent(LaunchCenterRef center, CargoType cargo);
/** @param center Exact recovery target. @return Whether saved descent work still depends on this identity. */
bool HasSpaceDescentForCenter(LaunchCenterRef center);
/** @return Next monotonic descent job identity, including retired history. */
uint64_t GetNextSpaceDescentID();
/** Reset descent identity history and staging during world initialization; does not settle live cargo. */
void ResetSpaceDescents();
/** @return Copied descent transport records in pod order, without transferring packet custody. */
std::vector<SpaceDescentRecord> CaptureSpaceDescents();
/**
 * Validate copied descent geometry, timers, identities and unique packet references without pool access.
 * @param records Staged descent records.
 * @param next Next saved monotonic job ID; must exceed every live job ID.
 * @param width Saved plane width in tiles.
 * @param height Saved plane height in tiles.
 * @return Whether all records are structurally bounded and consistent.
 */
bool ValidateSpaceDescentRecords(std::span<const SpaceDescentRecord> records, uint64_t next, uint32_t width, uint32_t height);
/**
 * Check descent ownership across copied save families; independent previews may call this.
 * @param records Copied descents with their single packet-owner references.
 * @param pods Copied paid shells whose descent custody must match these jobs.
 * @param manifests Copied native supply owners with exact pending-return quotas.
 * @param centers Copied generation-qualified target centers; lost targets require a saved block.
 * @param width Saved plane width used to derive each immutable anchor's Q16 position.
 * @return Whether shell custody, geometry, quotas, bounds and packet exclusivity agree.
 */
bool ValidateSpaceDescentLinks(std::span<const SpaceDescentRecord> records, std::span<const SpacePodRecord> pods,
	std::span<const SpaceManifestRecord> manifests, std::span<const LaunchCenterRecord> centers, uint32_t width);
/** @return Whether current descents agree with actual native packets, shells, manifests and target capacity. */
bool ValidateSpaceDescents();
/**
 * Restore single pod-owned packet lists after structural and native-owner validation.
 * @param records Staged descent transport with existing CargoPacket IDs.
 * @param next Saved next job identity, preserving retired history.
 * @return Whether all jobs were installed; invalid input is refused before installation.
 */
bool InstallLoadedSpaceDescents(std::span<const SpaceDescentRecord> records, uint64_t next);
/** @return Deterministic digest of descent identities, timers, blocks and packet accounting. */
uint64_t ComputeSpaceDescentDigest();
/** Clear staged descent transport between loads without installing it into native owners. */
void ResetSpaceDescentLoadStaging();
/** @return Whether staged descents were accepted by the existing native pod/packet owners. */
bool AcceptLoadedSpaceDescents();
/** Validate copied descent preview families without changing live simulation authority. */
void ValidateSpaceDescentLoadPreview();

/** Link identity changes on unlink/reselection, even when the native StationID is reused. */
struct LaunchHubMemberRef {
	StationID station = StationID::Invalid();
	uint64_t generation = 0;
	/** @param other Membership identity to compare. @return Whether station and link generation both agree. */
	bool operator==(const LaunchHubMemberRef &other) const = default;
};

/** Saved timed job owns these actual packets until delivery or cargo-preserving settlement. */
struct SurfaceHandlingJob {
	uint64_t id = 0;
	LaunchCenterRef center;
	LaunchHubMemberRef member;
	CargoType cargo = INVALID_CARGO;
	bool reverse = false; ///< Only an explicit supply transfer may travel towards the center.
	uint16_t remaining_ticks = 0;
	uint16_t age_ticks = 0;
	StationCargoList packets{};
};

/** Native Station owns relationships and jobs; ordinary waiting stock remains in Station::goods. */
struct LaunchCollectionHub {
	std::vector<LaunchHubMemberRef> members; ///< Sorted, explicit, exclusive separate-Station memberships.
	std::vector<std::unique_ptr<SurfaceHandlingJob>> handling; ///< Ascending monotonic job ID.
	uint8_t handling_cost_remainder = 0; ///< Sixteenths of a tonne; Goods crates must not round down per packet.
};

/** Copied LHND transport; actual packets remain in the native Station-owned handling job. */
struct SurfaceHandlingRecord {
	uint64_t id = 0;
	LaunchCenterRef center;
	LaunchHubMemberRef member;
	CargoType cargo = INVALID_CARGO;
	bool reverse = false;
	uint16_t remaining_ticks = 0, age_ticks = 0;
	std::vector<std::pair<CargoPacketID, StationID>> packets;
};

/** Copied per-center fractional handling charge; shared across all cargo and jobs to prevent rounding loss. */
struct SurfaceHandlingCostRecord {
	LaunchCenterRef center;
	uint8_t remainder = 0;
};

inline constexpr uint16_t LAUNCH_HANDLING_THROUGHPUT = 8;
inline constexpr uint16_t LAUNCH_HANDLING_STOCK_CAP = 1024;
inline constexpr uint16_t MAX_SURFACE_HANDLING_QUANTITY = 64;
inline constexpr size_t MAX_SURFACE_HANDLING_JOBS = 32; ///< Per center, independent of member count.

/**
 * Preflight bounded physical handling without reserving, moving or manufacturing stock.
 * @param center Generation-qualified native launch hub.
 * @param member Exclusive separate-station membership; joined storage cannot transfer to itself.
 * @param cargo Runtime cargo type compatible with this handling direction.
 * @param quantity Positive whole units, at most MAX_SURFACE_HANDLING_QUANTITY.
 * @param reverse Whether an explicit supply transfer moves from member to center.
 * @return Whether source availability, destination headroom, job and split capacity permit a move.
 */
bool CanStartSurfaceHandling(LaunchCenterRef center, LaunchHubMemberRef member, CargoType cargo, uint16_t quantity, bool reverse);
/** @param cargo Runtime cargo type to inspect. @return Whether its canonical label is an admitted launch supply. */
bool IsLaunchSupplyCargo(CargoType cargo);
/** @param station Native storage owner. @param cargo Runtime cargo type. @return Applicable center/member stock cap. */
uint GetLaunchStockCapacity(const Station &station, CargoType cargo);
/**
 * Count free capacity after actual waiting cargo and saved incoming handling/descent reservations.
 * @param station Native storage owner, including a joined center/freight Station.
 * @param cargo Runtime cargo type whose physical capacity is requested.
 * @return Remaining whole-unit capacity; overfull storage returns zero.
 */
uint GetLaunchStockHeadroom(const Station &station, CargoType cargo);
/**
 * Revalidate and atomically move actual source packets into one saved handling owner.
 * @param center Generation-qualified hub paying mass-based fees as cargo actually moves.
 * @param member Exclusive generation-qualified separate-station link.
 * @param cargo Runtime cargo type to handle.
 * @param quantity Positive whole units to move into the bounded job.
 * @param reverse Whether this is an explicitly commanded member-to-center supply move.
 * @return Whether preparation committed; refusal leaves source packets and reservations unchanged.
 */
bool StartSurfaceHandling(LaunchCenterRef center, LaunchHubMemberRef member, CargoType cargo, uint16_t quantity, bool reverse);
/** Age jobs, dispatch eligible demand and share bounded throughput/fees once per simulation tick. */
void TickSurfaceHandling();
/** @param station Native station ID. @return Whether a saved handling job depends on it as hub or member. */
bool HasSurfaceHandlingForStation(StationID station);
/**
 * Settle dependent job packets before station deletion, membership loss or ownership change.
 * @param station Native owner whose relationships are being retired.
 * @param cleaning_pool Whether pool teardown already owns packet destruction; then detach lists only.
 */
void SettleSurfaceHandlingForStation(Station &station, bool cleaning_pool = false);
/** @return Whether live jobs, membership identities, packet ownership and cost fractions agree. */
bool ValidateSurfaceHandling();
/** @return Next monotonic handling job ID, retaining retired history across save/load. */
uint64_t GetNextSurfaceHandlingID();
/** @return Copied job records with packet references; packet custody remains with each native job. */
std::vector<SurfaceHandlingRecord> CaptureSurfaceHandling();
/** @return Copied fractional charge records for every native hub. */
std::vector<SurfaceHandlingCostRecord> CaptureSurfaceHandlingCosts();
/**
 * Validate bounded copied handling transport without resolving or changing native pools.
 * @param records Copied job identities, timers and unique packet references.
 * @param costs Copied per-center fractional mass fees.
 * @param next Saved next job ID, greater than every live job ID.
 * @return Whether structural bounds, uniqueness and fractions are valid.
 */
bool ValidateSurfaceHandlingRecords(std::span<const SurfaceHandlingRecord> records, std::span<const SurfaceHandlingCostRecord> costs, uint64_t next);
/**
 * Install handling owners only after transport and physical relationships validate.
 * @param records Staged jobs identifying existing native packets.
 * @param costs Staged per-center fee fractions.
 * @param next Saved next monotonic job ID.
 * @return Whether every job/fraction was validated and installed.
 */
bool InstallLoadedSurfaceHandling(std::span<const SurfaceHandlingRecord> records, std::span<const SurfaceHandlingCostRecord> costs, uint64_t next);
/** Clear hub handling lists and reset job identity history during world initialization. */
void ResetSurfaceHandling();
/** @return Deterministic digest of live handling owners, packet accounting, timers and charge fractions. */
uint64_t ComputeSurfaceHandlingDigest();

/** Public membership view; joined facilities expose one Station inventory and require no transfer job. */
struct LaunchHubMembership {
	LaunchCenterRef center;
	LaunchHubMemberRef member;
	bool joined = false; ///< Same native Station: normal pickup, without a Surface handling transfer.
};

/** Bounded save transport; live ownership remains with Station::launch_hub. */
struct LaunchHubRecord {
	LaunchCenterRef center;
	std::vector<LaunchHubMemberRef> members;
};

/** Bounded save transport, not an independently active station registry. */
struct LaunchCenterRecord {
	StationID station = StationID::Invalid();
	LaunchCenterAnchor anchor;
};

inline constexpr size_t MAX_LAUNCH_CENTERS = 64;
inline constexpr size_t MAX_LAUNCH_HUB_MEMBERS = 32; ///< Separate stations per center; joined facilities use no slot.

inline constexpr uint16_t LAUNCH_CENTER_DROP_RADIUS = 20;
inline constexpr uint16_t LAUNCH_CENTER_COLLECTION_RADIUS = 20;
/** @return Whether the selected native gameplay profile and calendar year admit launch-center technology. */
bool IsLaunchCenterTechnologyAvailable();

/**
 * Derive the rotated 4x3 land extent, guarding every coordinate against Surface bounds.
 * @param anchor Immutable Surface build anchor.
 * @param rotation Quarter-turn layout index, 0..3.
 * @return Complete Surface extent, or no value for invalid layer, rotation or bounds.
 */
std::optional<TileArea> GetLaunchCenterFootprint(TileIndex anchor, uint8_t rotation);
/** @param tile Guarded world tile to inspect. @return Whether it is flat, clear navigable Surface sea/canal/river. */
bool IsLaunchCenterNavigableWater(TileIndex tile);
/**
 * Inspect only orthogonal outer footprint edges for qualifying navigable water.
 * @param footprint Complete proposed Surface launch-center extent.
 * @param excluded Tile proposed for removal, or INVALID_TILE when no tile is excluded.
 * @return Whether a qualifying edge survives, without map wrapping or internal/diagonal contact.
 */
bool HasLaunchCenterWaterAccess(const TileArea &footprint, TileIndex excluded = INVALID_TILE);
/**
 * Read the immutable build anchor's layer-local orbital reference.
 * @param anchor Valid Surface build tile; station bounding-box changes do not affect it.
 * @return Plane-local Q16 reference, or no value for an invalid/non-Surface tile.
 */
std::optional<SpacePoint> GetLaunchCenterPlaneReference(TileIndex anchor);
/**
 * Check the inclusive orbital release circle using widened fixed-point geometry.
 * @param point Actual craft position in plane-local Q16 units.
 * @param anchor Immutable center reference in the same units.
 * @return Whether point is within20 tiles; this geometric result grants no transfer authority.
 */
bool IsInsideLaunchCenterDropZone(SpacePoint point, SpacePoint anchor);
/**
 * Check the independent inclusive Surface collection circle.
 * @param point Surface footprint point in plane-local Q16 units.
 * @param anchor Immutable center reference in the same units.
 * @return Whether point is within the collection radius; membership still needs full-footprint validation.
 */
bool IsInsideLaunchCenterCollectionArea(SpacePoint point, SpacePoint anchor);

/** @param ref Station/generation pair. @return Native center, or nullptr for an empty, deleted or reused identity. */
Station *ResolveLaunchCenter(LaunchCenterRef ref);
/** @param station Native Station to identify. @return Exact center reference, or an empty reference without a facility. */
LaunchCenterRef GetLaunchCenterReference(const Station &station);
/** @return Bounded derived center lookup in StationID order; rebuilt from native owners on load. */
std::span<const LaunchCenterRef> GetLaunchCenterReferences();
/** @return Whether the bounded center lookup and monotonic generation still permit a new facility. */
bool CanAllocateLaunchCenter();
/**
 * Install one new center identity, hub and slot owner after native construction preflight.
 * @param station Existing native Station receiving the facility; must not already own a center.
 * @param anchor Immutable validated Surface build tile.
 * @param rotation Validated quarter-turn layout index, 0..3.
 */
void InstallNewLaunchCenter(Station &station, TileIndex anchor, uint8_t rotation);
/** @param station Station losing the facility; retire jobs, operational references and lookup before pool reuse. */
void ForgetLaunchCenter(Station &station);
/** Clear native launch metadata, derived lookup and staging during world initialization. */
void ResetLaunchCenters();
/** @return Next monotonic center generation, retaining retired identity history. */
uint64_t GetNextLaunchCenterGeneration();
/** @return Copied center transport records from native Station owners, without allocating facilities. */
std::vector<LaunchCenterRecord> CaptureLaunchCenters();
/**
 * Validate bounded copied center identities and nonoverlapping Surface footprints without pool access.
 * @param records Staged center records.
 * @param next_generation Saved next generation, greater than every live generation.
 * @param width Saved plane width in tiles.
 * @param height Saved plane height in tiles.
 * @return Whether all identities, rotations and footprints are structurally valid.
 */
bool ValidateLaunchCenterRecords(std::span<const LaunchCenterRecord> records, uint64_t next_generation, uint32_t width, uint32_t height);
/** @return Whether current center metadata, native facility tiles, water access and derived lookup agree. */
bool ValidateLaunchCenters();
/**
 * Restore center metadata only after copied transport and actual native tiles validate.
 * @param records Staged center records; installation creates empty hub/slot owners for their own families.
 * @param next_generation Saved next center generation, including retired history.
 * @return Whether every center was validated and installed.
 */
bool InstallLoadedLaunchCenters(std::vector<LaunchCenterRecord> records, uint64_t next_generation);
/** @return Deterministic center identity/geometry digest, including paid production and slot authority. */
uint64_t ComputeLaunchCenterDigest();
/**
 * Check actual full native station footprint, Surface layer, company and rail/lorry capability.
 * @param center Native center defining the immutable collection circle.
 * @param member Native freight station to inspect, including joined center facilities.
 * @return Whether every physical station tile qualifies; ordinary catchment/sign position grants no membership.
 */
bool IsLaunchHubEligible(const Station &center, const Station &member);
/** @param member Native station ID. @return Its validated explicit or joined relationship, or no value. */
std::optional<LaunchHubMembership> GetLaunchHubMembership(StationID member);
/**
 * Resolve an exact exclusive separate-station link and recheck its current eligibility.
 * @param center Generation-qualified hub identity.
 * @param member Station ID plus monotonic link generation.
 * @return Eligible linked Station, or nullptr for a stale, invalid or out-of-range relationship.
 */
Station *ResolveLaunchHubMember(LaunchCenterRef center, LaunchHubMemberRef member);
/** @param member Native freight station to inspect. @return Eligible center choices in stable order; never enrolls it. */
std::vector<LaunchCenterRef> GetEligibleLaunchHubs(const Station &member);
/** @param center Exact hub identity. @return Its single native Station storage owner, or nullptr if stale. */
Station *GetLaunchHubStorage(LaunchCenterRef center);
/** @return Next monotonic membership generation, including retired/reselected links. */
uint64_t GetNextLaunchHubMemberGeneration();
/** @return Copied explicit membership transport; joined facilities consume no separate member slot. */
std::vector<LaunchHubRecord> CaptureLaunchHubs();
/**
 * Validate bounded copied membership records without consulting native pools.
 * @param records Staged center/member relationships with exclusive member identities.
 * @param next_generation Saved next link generation, greater than every live link generation.
 * @return Whether bounds, ordering, generation history and exclusive membership hold.
 */
bool ValidateLaunchHubRecords(std::span<const LaunchHubRecord> records, uint64_t next_generation);
/** @return Whether current memberships agree with native center identities and full-footprint eligibility. */
bool ValidateLaunchHubs();
/**
 * Restore links only after copied records and actual native footprints validate.
 * @param records Staged exclusive relationships; moves no packet or station identity.
 * @param next_generation Saved next monotonic link generation.
 * @return Whether every relationship was validated and installed.
 */
bool InstallLoadedLaunchHubs(std::vector<LaunchHubRecord> records, uint64_t next_generation);
/** @return Deterministic digest of hub identities, explicit memberships and retired generation history. */
uint64_t ComputeLaunchHubDigest();
/** Clear native hub/handling metadata and reset membership identity history during world initialization. */
void ResetLaunchHubs();
/** Revalidate native footprint/owner changes, settling invalid-link jobs before retiring relationships. */
void RevalidateLaunchHubMemberships();
/** @param station Native Station being destroyed; settle jobs and retire references before its pool slot can be reused. */
void ForgetLaunchHubStation(Station &station);
/**
 * Preflight the complete proposed water-removal set, including sequential area commands.
 * @param removed Predicate reporting every tile whose qualifying water would disappear.
 * @return Whether every existing center retains an orthogonal qualifying water edge.
 */
bool CanRemoveLaunchCenterWater(const std::function<bool(TileIndex)> &removed);

/** Native canal conversion restores qualifying flat water in the same commit. */
class ScopedLaunchCenterWaterReplacement {
	TileIndex previous;
public:
	/** @param tile Native conversion tile guaranteed to restore qualifying flat water in the same commit. */
	explicit ScopedLaunchCenterWaterReplacement(TileIndex tile);
	/** Restore the previous nested water-replacement exemption when the native conversion scope ends. */
	~ScopedLaunchCenterWaterReplacement();
	/** @param other Scope whose copying is forbidden to preserve stack-like exemption restoration. */
	ScopedLaunchCenterWaterReplacement(const ScopedLaunchCenterWaterReplacement &other) = delete;
	/** @param other Scope whose assignment is forbidden to preserve stack-like exemption restoration. */
	ScopedLaunchCenterWaterReplacement &operator=(const ScopedLaunchCenterWaterReplacement &other) = delete;
};

/** Clear staged center, hub and handling transport between load/preview operations. */
void ResetLaunchCenterLoadStaging();
/** @return Whether staged center metadata was validated and accepted on existing native facility tiles. */
bool AcceptLoadedLaunchCenters();
/** @return Whether staged memberships were validated and accepted on existing native Station owners. */
bool AcceptLoadedLaunchHubs();
/** @return Whether staged handling packets, jobs and fee fractions were validated and accepted. */
bool AcceptLoadedSurfaceHandling();
/** Validate copied center/hub/handling preview families without changing live simulation authority. */
void ValidateLaunchCenterLoadPreview();
#endif /* LAUNCH_CENTER_H */
