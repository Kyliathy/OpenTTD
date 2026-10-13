/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure.h Native Space occupancy, construction and contained Core facilities.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group13B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-6.
 */
#ifndef SPACE_STRUCTURE_H
#define SPACE_STRUCTURE_H
#include "space_structure_type.h"
#include "space.h"
#include "orbital_station.h"
#include "multilayer_space_content.h"
#include "company_type.h"

struct SpaceStructure;
using SpaceStructurePool = Pool<SpaceStructure, SpaceStructureID, 16>;
extern SpaceStructurePool _space_structure_pool;
/** FrameComplete alone grants no facilities; a contained activated module is required. */
enum class SpaceStructurePhase : uint8_t { Blueprint, FrameComplete, Cancelled, End };
/** Paid immutable site definition and construction phase, independent of camera selection. */
struct SpaceStructureState {
	Owner owner = OWNER_NONE;
	SpaceBuildKind kind = SpaceBuildKind::Core;
	SpaceStructurePhase phase = SpaceStructurePhase::Blueprint;
	uint16_t x = 0, y = 0;
	uint16_t content_version = MULTILAYER_SPACE_CONTENT_VERSION;
	uint32_t capital_paid = 0;
	/** @param other State to compare. @return Whether every paid site field agrees. */
	bool operator==(const SpaceStructureState &other) const = default;
};
/** Sole native identity and lifetime owner of a site's construction and completed facilities. */
struct SpaceStructure : SpaceStructurePool::PoolItem<&_space_structure_pool> {
	uint64_t generation = 0;
	SpaceStructureState state;
	OrbitalConstructionJob job;
	std::unique_ptr<OrbitalModule> module; ///< Absent until explicit paid-frame activation begins.
	/** @param index Native pool-issued identity, also used by save reconstruction. */
	SpaceStructure(SpaceStructureID index) : SpaceStructurePool::PoolItem<&_space_structure_pool>(index) {}
	/** Settle container cleanup; ordinary removal requires empty accountable inventories. */
	~SpaceStructure();
};
/** Detached save/preview record; reading it never creates facilities or consumes material. */
struct SpaceStructureRecord : OrbitalConstructionState {
	SpaceStructureRef ref;
	SpaceStructureState state;
	uint16_t age_ticks = 0;
	std::array<uint16_t, 2> consumed{};
	std::array<uint16_t, 2> quantities{};
	std::array<std::vector<std::pair<CargoPacketID, StationID>>, 2> inputs;
	std::optional<OrbitalModuleRecord> module;
};
/** @param site Actual native site. @return Its generation-qualified identity. */
SpaceStructureRef GetSpaceStructureReference(const SpaceStructure &site);
/** @param ref Saved identity including incarnation. @return Matching actual site, or nullptr. */
SpaceStructure *ResolveSpaceStructure(SpaceStructureRef ref);
/** @param state Authoritative site geometry. @return Its precise south approach in plane coordinates. */
SpacePoint GetSpaceStructureApproach(const SpaceStructureState &state);
/** Check the footprint and south approach outside the asteroid travel strip.
 * @param state Candidate geometry. @param width Plane width. @param height Plane height.
 * @return Whether the entire site fits the static construction band.
 */
bool IsSpaceStructureSiteValid(const SpaceStructureState &state, uint32_t width, uint32_t height);
/** @param a First site. @param b Second site. @return Whether footprints/approaches conflict; shared edges are legal. */
bool SpaceStructureSitesOverlap(const SpaceStructureState &a, const SpaceStructureState &b);
/** @param state Candidate geometry. @return Whether no retained native site reserves its footprint or approach. */
bool IsSpaceStructureSiteAvailable(const SpaceStructureState &state);
/** @return Next never-reused native incarnation. */
uint64_t GetNextSpaceStructureGeneration();
/** Reset issuance and load staging during world teardown; creates no objects. */
void ResetSpaceStructures();
/** @param old_owner Previous company. @param new_owner Takeover company, or invalid to retain unowned stock. */
void ChangeSpaceStructureOwnership(Owner old_owner, Owner new_owner);
/** @return Detached records of every actual site, including its sole packet owners. */
std::vector<SpaceStructureRecord> CaptureSpaceStructures();
/** Validate copied identities, geometry and material receipts without consulting live pools.
 * @param records Copied records. @param next Saved next incarnation. @param width Plane width. @param height Plane height.
 * @param native_tick Same-file/native simulation clock, or UINT64_MAX for a clock-independent structural fixture.
 * @return Whether all records and exclusive packet references are structurally consistent.
 */
bool ValidateSpaceStructureRecords(std::span<const SpaceStructureRecord> records, uint64_t next, uint32_t width, uint32_t height, uint64_t native_tick = UINT64_MAX);
/** @return Whether actual sites, packet owners and material quantities are consistent. */
bool ValidateSpaceStructures();
/** Reconstruct native owners only after their referenced packet owners have loaded.
 * @param records Validated copied transport. @param next Saved next incarnation.
 * @return Whether installation preserved every physical packet reference.
 */
bool InstallLoadedSpaceStructures(std::span<const SpaceStructureRecord> records, uint64_t next);
/** @return Deterministic digest of actual site, job, activation and packet history. */
uint64_t ComputeSpaceStructureDigest();
/** Discard pending detached save transport. */
void ResetSpaceStructureLoadStaging();
/** @return Whether pending transport was installed through native owners. */
bool AcceptLoadedSpaceStructures();
/** Validate copied preview transport without changing the active world. */
void ValidateSpaceStructureLoadPreview();
/** Open the native module selector and physical construction controls.
 * @param site Optional exact owned site to select; invalid/foreign references retain the normal selection.
 */
void ShowSpaceConstructionWindow(SpaceStructureRef site = {});
#endif
