/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_state.h Versioned metadata and feature-owned geology/job/pass reference validation. @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md; r2mlvs2-geology-and-excavation.md */

#ifndef MULTILAYER_STATE_H
#define MULTILAYER_STATE_H

#include "multilayer_content.h"

static constexpr uint16_t MULTILAYER_STATE_SCHEMA_VERSION = 1; ///< Explicit metadata schema, independent of content package/profile versions.
static constexpr uint32_t MULTILAYER_FEATURE_GEOLOGY = 1; ///< Delivered rock/deposit descriptors; exact chunks required from save version 372.
static constexpr uint32_t MULTILAYER_FEATURE_VENTS = 2; ///< Delivered independent natural capacity; allocation/plant behavior remains with its owner.
static constexpr uint32_t MULTILAYER_FEATURE_EXCAVATION_JOBS = 4; ///< Saved sparse jobs/retired-ID cursor, requiring the complete version-373 job family.
static constexpr uint32_t MULTILAYER_FEATURE_EXCAVATION_PASSES = 8; ///< Frozen backing/capacity inputs require the version-374 pass family and actual jobs.
static constexpr uint32_t MULTILAYER_FEATURE_PORTAL_WORK = 16; ///< Canonical native portal work records require version-375 descriptors; topology stays in the map.
static constexpr uint32_t MULTILAYER_FEATURE_TBM_PROJECTS = 32; ///< Typed TBM projects (any ID ever allocated) require the version-376 project family.
static constexpr uint32_t MULTILAYER_FEATURE_SPOIL_RECEIVERS = 64; ///< Live Earth/Rock spoil receivers require the version-377 receiver family.
static constexpr uint32_t MULTILAYER_FEATURE_TBM_EXCHANGE = 128; ///< Hauler assignments/wagon sets (any ID ever allocated) require the version-378 exchange family.
static constexpr uint32_t MULTILAYER_FEATURE_MINES = 256; ///< Finite mine bindings require the version-379 family and existing geology/industry inventories. See r2mlvs6-mines-and-resource-chains.md.
static constexpr uint32_t MULTILAYER_FEATURE_PROCESSORS = 512; ///< Native recipe choice/work/counters require version-380 descriptors beside Industry stock. See r2mlvs6-mines-and-resource-chains.md.
static constexpr uint32_t MULTILAYER_FEATURE_UTILITY_POWER = 1024; ///< Native electrical epochs, fractions and grants require version-381 transport. See r2mlvs12-solar-farms-mirrors-and-eti.md.
static constexpr uint32_t MULTILAYER_FEATURE_TUBE_POWER = 2048; ///< Physical paid connections/invoices require version386 and their actual utility/portal owners. See r2mlvs16-underground-transport-tubes.md Group5.
static constexpr uint32_t MULTILAYER_FEATURE_TUBE_VEHICLES = 4096; ///< Actual capsule bindings/history require version387 and actual physical paid sources. See r2mlvs16-underground-transport-tubes.md Group6.
static constexpr uint32_t MULTILAYER_FEATURE_GEOTHERMAL_SITES = 8192; ///< Cold planning history requires388 and original geology/vent authority; no operating collector is implied. See r2mlvs17-geothermal-energy-and-industry.md Group1.

static constexpr uint32_t MULTILAYER_FEATURE_GEOTHERMAL_COLLECTORS = 16384; ///< Actual native collector construction requires389 and consumed cold/source references; no heat grant or power is implied. See r2mlvs17-geothermal-energy-and-industry.md Group1B.

static constexpr uint32_t MULTILAYER_FEATURE_UTILITY_HEAT = 32768; ///< Typed heat accounting requires390 and the same utility identity/balance owner; physical geothermal references retain Plan17.
static constexpr uint32_t MULTILAYER_FEATURE_LOCAL_UTILITY_LINKS = 65536; ///< Installed Deep Heat/Electricity cells require392 and the shared link issuer/geometry owner; no energy is implied. See r2mlvs17-geothermal-energy-and-industry.md Group3B.
static constexpr uint32_t MULTILAYER_FEATURE_GEOTHERMAL_HEAT_ROUTES = 131072; ///< Physical393 ports/current source paths require the original shared Heat and paid local link owners. See r2mlvs17-geothermal-energy-and-industry.md Group3.

/** Serialization/preview snapshot; the accepted profile remains owned by multilayer_content.cpp. */
struct MultiLayerStateSchema {
	uint16_t schema_version = MULTILAYER_STATE_SCHEMA_VERSION; ///< Unknown revisions refuse before activation.
	MultiLayerProfileSelection profile; ///< Explicit saved policy; absent chunk remains Classic/legacy.
	GRFIdentifier required_content{}; ///< Exact package identity; empty for Classic, pinned for Multilayer.
	uint32_t content_version = 0; ///< Exact definition version, separate from schema/profile version.
	uint32_t feature_flags = 0; ///< Only delivered geology/vent bits are accepted; later feature owners extend their own descriptors/references.
	uint64_t geology_seed = 0; ///< Saved feature seed root; zero until its producer initializes it, without drawing Surface RNG.
	uint64_t orbital_seed = 0; ///< Separate saved orbital seed root.
};

/** Specific structural/reference refusal, never an instruction to manufacture missing material. */
enum class MultiLayerStateError : uint8_t {
	None, UnsupportedSchema, UnsupportedProfile, InvalidPolicy, ChangedContent, UnsupportedFeatures, InvalidReference,
};

/** Feature-supplied read-only reference hook; concrete pools and removal policy stay with that feature. */
struct MultiLayerStateReferenceCheck {
	std::string_view owner; ///< Unique nonempty owner; a field cannot acquire two validation/repair authorities.
	bool (*validate)() = nullptr; ///< Inspect actual restored typed references without mutation/RNG; false refuses unsafe state.
};

/**
 * Validate metadata and actual delivered feature reference hooks before world activation.
 * @param schema Candidate serialization snapshot; no live selected policy is changed here.
 * @param checks Feature-owned read-only checks; empty for M0's empty-compatible metadata.
 * @return Specific refusal or None, without consuming or repairing any state.
 */
MultiLayerStateError ValidateMultiLayerStateReferences(const MultiLayerStateSchema &schema, std::span<const MultiLayerStateReferenceCheck> checks = {});

/**
 * Capture the one accepted profile and seed roots for save/diagnostic transport.
 * @return Metadata snapshot; never another independently mutable selected profile.
 */
MultiLayerStateSchema CaptureMultiLayerStateSchema();

/** Reset seed roots and transient load staging at world initialization; old absence stays Classic. */
void ResetMultiLayerStateSchema();

/**
 * Publish geology's completed generation seed through the one metadata seed owner.
 * @param seed Versioned geology root derived without a Surface RNG draw; called only after successful candidate generation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md
 */
void SetMultiLayerGeologySeed(uint64_t seed);

/**
 * Stage one structurally validated save row until activated content can be checked.
 * @param schema Candidate read by the save handler; this does not publish gameplay policy.
 */
void StageLoadedMultiLayerStateSchema(const MultiLayerStateSchema &schema);

/**
 * Inspect pending save metadata without mutating it or the current game.
 * @return Candidate pointer, or nullptr when the file has no metadata chunk.
 */
const MultiLayerStateSchema *GetPendingMultiLayerStateSchema();

/**
 * Accept staged metadata only after actual GRF definitions and cargo labels are ready.
 * @param landscape Actual loaded climate, owned by world settings.
 * @param activated Actual loader configuration and live cargo definitions.
 * @return Profile/content refusal, or None; only success restores the one live policy/seed roots.
 */
MultiLayerProfileError AcceptLoadedMultiLayerStateSchema(LandscapeType landscape, const GRFConfigList &activated);

/** Stable diagnostic feature ordering; concrete saved records/typed IDs stay with their owners. */
enum class MultiLayerCanonicalKind : uint8_t {
	Deposit = 1, ExcavationJob, Portal, TBMProject, Exchange, IndustryInventory, OrbitalBody,
	SpaceMission, LaunchHandling, StationConstruction, Solar, Utility, Tube, Geothermal,
	GeologyRoot, ///< Appended aggregate over actual packed/seam/vent/job authority; existing diagnostic kind IDs stay unchanged.
	PortalWorkRoot, ///< Appended pair phase/project/actor authority; physical endpoint fields remain in the ordinary map digest.
	MineRoot, ///< Appended finite bindings/work/counters; reserve and output keep their own owners. See r2mlvs6-mines-and-resource-chains.md.
	ProcessorRoot, ///< Appended native choice/work/batch/material history, with stock read from Industry. See r2mlvs6-mines-and-resource-chains.md.
	UtilityRoot, ///< Appended exact electrical epoch/meter/link/slice authority. See r2mlvs12-solar-farms-mirrors-and-eti.md Group 7B.
	TubePowerRoot, ///< Physical connection and paid invoice digest, excluding electrical balances. See r2mlvs16-underground-transport-tubes.md Group5.
	TubeVehicleRoot, ///< Capsule binding/fraction/tick digest; packets and joules stay in their own roots. See r2mlvs16-underground-transport-tubes.md Group6.
	GeothermalSiteRoot, ///< Cold planning identity/geometry/history, excluding natural capacity, delivered construction and all energy. See r2mlvs17-geothermal-energy-and-industry.md Group1.
	GeothermalCollectorRoot, ///< Actual native construction/control/work/material-receipt digest; natural capacity and energy retain their own owners. See r2mlvs17-geothermal-energy-and-industry.md Group1B.
	GeothermalHeatRouteRoot, ///< Exact original physical ports/source paths, without copied energy quantities. See r2mlvs17-geothermal-energy-and-industry.md Group3.
	End,
};

/** Explicit unsigned field encoding, avoiding pointers, compiler padding and host byte order. */
struct MultiLayerCanonicalField {
	uint16_t tag; ///< Stable owner-assigned field tag; unique in one record.
	uint8_t width; ///< Exact byte width: 1, 2, 4 or 8.
	uint64_t value; ///< Actual unsigned integer/fixed-point/reference bits, fitting width.
};

/** Read-only diagnostic view over an actual feature-owned record, not another saved-state pool. */
struct MultiLayerCanonicalRecord {
	MultiLayerCanonicalKind kind; ///< Stable feature order.
	uint32_t id; ///< Numeric value of the owner's actual typed stable ID; its reference validator owns validity.
	std::span<const MultiLayerCanonicalField> fields; ///< Authoritative fields only; no presentation/cache fields.
};

/**
 * Compute a canonical diagnostic digest of metadata and owner-supplied state views.
 * @param schema Validated metadata snapshot, including profile/schema, policy and seed roots.
 * @param records Actual feature views, sorted internally by kind/ID and explicit field tag.
 * @return Portable FNV-1a diagnostic value, or no result for invalid bounds/duplicate/encoding input.
 */
std::optional<uint64_t> ComputeMultiLayerStateDigest(const MultiLayerStateSchema &schema, std::span<const MultiLayerCanonicalRecord> records = {});

/**
 * Emit explicit authoritative metadata/digest to existing desync diagnostics.
 * @param schema Validated snapshot; no live state is changed and no feature records are fabricated.
 */
void LogMultiLayerStateDiagnostics(const MultiLayerStateSchema &schema);

#endif /* MULTILAYER_STATE_H */
