/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_pod.h Native passive pod pool, paid construction and single custody.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups8C/13
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; a paid shell retains its actual Surface history at an orbital factory product.
 */
#ifndef SPACE_POD_H
#define SPACE_POD_H
#include "space_pod_type.h"
#include "space_production.h"
#include "company_type.h"

struct SpacePod;
struct LaunchOperationsRecord;
struct SpaceManifestRecord;
using SpacePodPool = Pool<SpacePod, SpacePodID, 16>;
extern SpacePodPool _space_pod_pool;

/** Exclusive original shell custody; historical Surface location never grants an additional storage or recovery owner. */
enum class SpacePodCustody : uint8_t { Assembling, CenterStored, Craft, Unhomed, Drifting, Descent, End };
/** Actual paid object. Center/craft slots hold typed references, never shell copies. */
struct SpacePodState {
	Owner owner = OWNER_NONE;
	SpacePodCustody custody = SpacePodCustody::Unhomed;
	LaunchCenterRef home, origin;
	SpaceCraftRef craft;
	TileIndex surface_tile = INVALID_TILE; ///< Actual last Surface custody tile; an orbital factory carrier has no Earth workshop and retains this shell's history.
	SpacePoint position{};
	uint16_t remaining_ticks = 0, content_version = 0;
	uint32_t capital_paid = 0;
	bool paused = false;
	/** @param other Saved paid shell state. @return Whether custody, work and physical provenance agree. */
	bool operator==(const SpacePodState &other) const = default;
};
/** Sole native passive asset owner; carrying references never create another shell or descent inventory. */
struct SpacePod : SpacePodPool::PoolItem<&_space_pod_pool> {
	uint64_t generation = 0;
	SpacePodState state;
	std::unique_ptr<SpaceDescentJob> descent;
	/** @param index Native pool identity; allocation alone grants no paid construction, materials or custody. */
	SpacePod(SpacePodID index) : SpacePodPool::PoolItem<&_space_pod_pool>(index) {}
	/** Relinquish only this actual shell's workshop/carrier references and its uniquely owned descent packets. */
	~SpacePod();
};
/** Detached saved identity/state; packet custody, when present, remains with the contained native descent owner. */
struct SpacePodRecord { SpacePodRef ref; SpacePodState state; };
/** @param pod Original native shell. @return Exact shell incarnation, never a copy or a carrier's ID. */
SpacePodRef GetSpacePodReference(const SpacePod &pod);
/** @param ref Exact shell incarnation. @return Matching actual shell, or nullptr after loss/reuse. */
SpacePod *ResolveSpacePod(SpacePodRef ref);
/** @return Next monotonic native shell generation, retaining retired issuance history. */
uint64_t GetNextSpacePodGeneration();
/** @param center Exact Surface facility. @return Stored/assembling paid shells counted against its real capacity. */
uint CountLaunchPods(LaunchCenterRef center);
/** @param center Exact Surface facility. @return Whether stored/assembling shells retain a dependency on it. */
bool HasLaunchPods(LaunchCenterRef center);
/** Advance original owned Earth workshop jobs once per normal native opportunity; pauses and lost slots earn no work. */
void TickSpacePods();
/** @param craft Actual sole carrier; follow its physical position while retaining a factory product's shell's real last Surface location. */
void SyncSpacePods(const SpaceVehicle &craft);
/** Check physical Surface custody without inventing an Earth home for a locally manufactured orbital craft.
 * @param pod Actual/candidate paid shell with a separately validated bounded Surface history tile.
 * @param craft Same native or same-file paid construction receipt; caller validates its complete factory provenance.
 * @return Whether the shell follows a real carrier Surface tile, or retains its own history at a genuine orbital factory product.
 */
bool IsSpacePodCarrierSurfaceTileValid(const SpacePodState &pod, const SpaceConstructionState &craft);
/** @param craft Actual disappearing carrier; retain each real shell as drifting/unhomed rather than destroying or duplicating it. */
void ForgetSpacePodCraft(const SpaceVehicle &craft);
/** @param center Exact lost facility; preserve paid shells and partial work under explicit unhomed custody. */
void ForgetSpacePodCenter(LaunchCenterRef center);
/** @param old_owner Company losing its physical assets. @param new_owner Actual takeover owner, or invalid for retained ownerless assets. */
void ChangeSpacePodOwnership(Owner old_owner, Owner new_owner);
/** Reset shell issuance/staging during native world teardown; does not settle or grant a live asset. */
void ResetSpacePods();
/** @return Copied actual shell identity, custody and paid provenance in native pool order. */
std::vector<SpacePodRecord> CaptureSpacePods();
/** @param records Copied shell states. @param next_generation Saved issuance boundary. @param width Saved plane width. @param height Saved plane height.
 * @return Whether every identity, receipt, timer, position and Surface history is bounded before native installation.
 */
bool ValidateSpacePodRecords(std::span<const SpacePodRecord> records, uint64_t next_generation, uint32_t width, uint32_t height);
/** @param pods Copied paid shells. @param craft Same-file paid hull receipts. @param operations Actual Surface workshop references. @param manifests Sole carrier pod references.
 * @return Whether custody is exclusive and every physical location belongs to its real workshop/carrier domain.
 */
bool ValidateSpacePodLinks(std::span<const SpacePodRecord> pods, std::span<const SpaceProductionRecord> craft,
	std::span<const LaunchOperationsRecord> operations, std::span<const SpaceManifestRecord> manifests);
/** @return Whether all real shell receipts, owners, physical positions and unique custody links agree. */
bool ValidateSpacePods();
/** @param records Complete detached original shell receipts. @param next_generation Saved issuance history.
 * @return Whether an empty native shell pool installed all records without assembling or granting any new asset.
 */
bool InstallLoadedSpacePods(std::span<const SpacePodRecord> records, uint64_t next_generation);
/** @return Deterministic actual paid shell identity/custody/history digest. */
uint64_t ComputeSpacePodDigest();
/** Discard unpublished file staging without touching a real shell or its cargo. */
void ResetSpacePodLoadStaging();
/** @return Whether the bounded staged original shell family installed under the native owner. */
bool AcceptLoadedSpacePods();
/** Validate copied same-file shell, native owner/position and custody links without accessing the open world's pools. */
void ValidateSpacePodLoadPreview();
#endif
