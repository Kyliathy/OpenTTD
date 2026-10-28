/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure.h Native Space occupancy, construction and contained Core facilities.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group13B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-6.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C; removed processors retain bounded spent history in this same native authority.
 */
#ifndef SPACE_STRUCTURE_H
#define SPACE_STRUCTURE_H
#include "space_structure_type.h"
#include "space.h"
#include "orbital_station.h"
#include "multilayer_space_content.h"
#include "company_type.h"
#include "multilayer_recipe.h"
#include <map>

/** Existing native site lifetime owner, forward declared for its original pool and generation-qualified references. */
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
/** Query an actual positive freight unload before navigation, through the original module exchange admission.
 * @param craft Exact native hull carrying the offered units.
 * @param site Exact owned operational candidate module.
 * @param quantity Positive original freight quantity, subject to stock/fragment capacity and original heavy return allowance.
 * @return Whether the original unload can be planned except for current arrival/Holding. Shared slot eligibility is checked but reserves nothing; CmdMoveOrbitalFreight must still admit the real arrival.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group19.
 */
bool CanNavigateOrbitalFreightUnload(SpaceCraftRef craft, SpaceStructureRef site, uint16_t quantity);
/** Bounded spent receipts retained by the same native structure authority after an empty pool owner is demolished.
 * These aggregates own no packets, energy, berth or replacement object identity.
 */
struct SpaceStructureRetirementHistory {
	uint64_t structures_removed = 0; ///< Number of native incarnations explicitly removed, including empty blueprints.
	std::array<uint64_t, 2> consumed{}; ///< Spent canonical STEL/GOOD receipts; demolition never refunds either material.
	uint64_t services_completed = 0; ///< Completed native services formerly recorded by removed modules.
	std::map<MultiLayerRecipeID, uint64_t> processing_batches; ///< Bounded canonical recipe history from removed processors; physical products are never reconstructed from these spent receipts.
	uint64_t processing_discarded_paid_j = 0; ///< Already spent unfinished electrical work explicitly abandoned before processor removal.
	/** @param other Historical receipts to compare. @return Whether every aggregate native receipt agrees. */
	bool operator==(const SpaceStructureRetirementHistory &other) const = default;
};
/** @return Read-only spent receipts from the native structure authority, with no retired-object registry. */
const SpaceStructureRetirementHistory &GetSpaceStructureRetirementHistory();
/** Install only already admitted same-file processing aggregates into the original structure authority.
 * @param history Complete original SPST history prefix plus independently validated SPRC processing fields.
 * @return Whether the unchanged original structure/material/service prefix still agrees; creates no retired native object or packet.
 */
bool InstallLoadedSpaceStructureProcessingHistory(const SpaceStructureRetirementHistory &history);
/** Pure lifecycle admission result; refusal never removes a physical owner, packet, grant or historical receipt. */
enum class OrbitalRemovalBlock : uint8_t {
	None, InvalidOwner, Inventory, Service, Construction, Vehicle, Dependent, Connection, Utility, History, End,
};
/** Classify actual whole removal admission without moving any accountable item.
 * @param site Exact existing native site whose generation remains authoritative.
 * @param owner Company asking to remove it; foreign and retired company incarnations refuse.
 * @return First deterministic refusal, or None only when every physical/dependency/utility owner admits empty removal.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group5.
 */
OrbitalRemovalBlock ClassifyOrbitalModuleRemoval(const SpaceStructure &site, CompanyID owner);
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
 * @param history Same-file native retired receipts; default empty only for older saves/isolated pre-removal fixtures.
 * @return Whether all records and exclusive packet references are structurally consistent.
 */
bool ValidateSpaceStructureRecords(std::span<const SpaceStructureRecord> records, uint64_t next, uint32_t width, uint32_t height, uint64_t native_tick = UINT64_MAX, SpaceStructureRetirementHistory history = {});
/** @return Whether actual sites, packet owners and material quantities are consistent. */
bool ValidateSpaceStructures();
/** Reconstruct native owners only after their referenced packet owners have loaded.
 * @param records Validated copied transport. @param next Saved next incarnation.
 * @param history Same-file spent receipts; old saves and isolated older fixtures receive zeros.
 * @return Whether installation preserved every physical packet reference.
 */
bool InstallLoadedSpaceStructures(std::span<const SpaceStructureRecord> records, uint64_t next, SpaceStructureRetirementHistory history = {});
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
