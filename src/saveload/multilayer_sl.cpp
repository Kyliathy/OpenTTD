/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_sl.cpp One-row metadata load/preview/fixup scaffold; feature records remain owner-specific. @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../multilayer_state.h"
#include "../fios.h"

#include "../safeguards.h"

/** Fixed wire transport; flags are checked before conversion to boolean gameplay policy. */
struct MultiLayerMetadataWire {
	uint16_t schema_version = MULTILAYER_STATE_SCHEMA_VERSION; ///< Header schema compatibility.
	uint8_t profile_id = 0; ///< Stable profile enum wire value.
	uint16_t profile_version = 0; ///< Exact gameplay contract revision.
	uint8_t policy_flags = 1; ///< Bit 0 legacy, bit 1 explicit editor override; unknown bits refuse.
	GRFIdentifier required_content{}; ///< Exact required identity or empty Classic identity.
	uint32_t content_version = 0; ///< Package version, independent of policy/schema.
	uint32_t feature_flags = 0; ///< Unsupported feature records cannot hide behind metadata.
	uint64_t geology_seed = 0; ///< Feature seed root, never a Surface RNG draw during load.
	uint64_t orbital_seed = 0; ///< Separate orbital seed root.
};

static const SaveLoad _multilayer_metadata_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("schema_version", SLE_OBJECT_ADDRESS(MultiLayerMetadataWire, schema_version)),
	SaveLoad::Variable<VarFileType::U8>("profile_id", SLE_OBJECT_ADDRESS(MultiLayerMetadataWire, profile_id)),
	SaveLoad::Variable<VarFileType::U16>("profile_version", SLE_OBJECT_ADDRESS(MultiLayerMetadataWire, profile_version)),
	SaveLoad::Variable<VarFileType::U8>("policy_flags", SLE_OBJECT_ADDRESS(MultiLayerMetadataWire, policy_flags)),
	SaveLoad::Variable<VarFileType::Label>("required_content.grfid", SLE_OBJECT_ADDRESS(MultiLayerMetadataWire, required_content.grfid)),
	SaveLoad::Array<VarFileType::U8, MD5_HASH_BYTES>("required_content.md5sum", SLE_OBJECT_ADDRESS(MultiLayerMetadataWire, required_content.md5sum)),
	SaveLoad::Variable<VarFileType::U32>("content_version", SLE_OBJECT_ADDRESS(MultiLayerMetadataWire, content_version)),
	SaveLoad::Variable<VarFileType::U32>("feature_flags", SLE_OBJECT_ADDRESS(MultiLayerMetadataWire, feature_flags)),
	SaveLoad::Variable<VarFileType::U64>("geology_seed", SLE_OBJECT_ADDRESS(MultiLayerMetadataWire, geology_seed)),
	SaveLoad::Variable<VarFileType::U64>("orbital_seed", SLE_OBJECT_ADDRESS(MultiLayerMetadataWire, orbital_seed)),
};

/** One mandatory row for new saves, with nonmutating preview and post-pointer structural validation. */
struct MLMDChunkHandler : ChunkHandler {
	/** Construct the unique MLMD table handler without allocating any feature pool. */
	MLMDChunkHandler() : ChunkHandler("MLMD", ChunkType::Table) {}

	/** Serialize the accepted owners into one bounded wire row; snapshots are not live policy. */
	void Save() const override
	{
		const auto schema = CaptureMultiLayerStateSchema();
		if (ValidateMultiLayerStateReferences(schema) != MultiLayerStateError::None) SlErrorCorrupt("Invalid multilayer metadata on save");
		MultiLayerMetadataWire wire;
		wire.schema_version = schema.schema_version;
		wire.profile_id = to_underlying(schema.profile.id);
		wire.profile_version = schema.profile.version;
		wire.policy_flags = (schema.profile.legacy ? 1 : 0) | (schema.profile.editor_override ? 2 : 0);
		wire.required_content = schema.required_content;
		wire.content_version = schema.content_version;
		wire.feature_flags = schema.feature_flags;
		wire.geology_seed = schema.geology_seed;
		wire.orbital_seed = schema.orbital_seed;
		SlTableHeader(_multilayer_metadata_desc);
		SlSetArrayIndex(0);
		SlObject(&wire, _multilayer_metadata_desc);
	}

	/**
	 * Parse/check one wire row for either full load or preview without publishing policy.
	 * @return Validated metadata snapshot; corrupt/missing/extra data raises the ordinary save error.
	 */
	MultiLayerStateSchema ReadMetadata() const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::MultiLayerMetadata)) SlErrorCorrupt("Unexpected multilayer metadata in old save version");
		const auto description = SlTableHeader(_multilayer_metadata_desc);
		if (description.size() != std::size(_multilayer_metadata_desc)) SlErrorCorrupt("Invalid multilayer metadata field count");
		// Missing policy fields must not silently downgrade a malformed explicit profile into default Classic.
		for (const auto &required : _multilayer_metadata_desc) {
			if (std::ranges::find(description, required.name, &SaveLoad::name) == description.end()) SlErrorCorrupt("Missing mandatory multilayer metadata field");
		}
		if (SlIterateArray() != 0) SlErrorCorrupt("Missing multilayer metadata row");
		MultiLayerMetadataWire wire;
		SlObject(&wire, description);
		if (SlIterateArray() != -1) SlErrorCorrupt("Too many multilayer metadata rows");
		if ((wire.policy_flags & ~3) != 0) SlErrorCorrupt("Unknown multilayer policy flags");
		MultiLayerStateSchema schema;
		schema.schema_version = wire.schema_version;
		schema.profile = {static_cast<MultiLayerGameplayProfileID>(wire.profile_id), wire.profile_version, (wire.policy_flags & 1) != 0, (wire.policy_flags & 2) != 0};
		schema.required_content = wire.required_content;
		schema.content_version = wire.content_version;
		schema.feature_flags = wire.feature_flags;
		schema.geology_seed = wire.geology_seed;
		schema.orbital_seed = wire.orbital_seed;
		// Feature bits require their versioned descriptor family; old metadata cannot request chunks its format never owned.
		if (IsSavegameVersionBefore(SaveLoadVersion::GeologyExcavation) && schema.feature_flags != 0) SlErrorCorrupt("Unexpected geology feature in old save version");
		// Earlier geology-only wire formats cannot request a later job descriptor family.
		if (IsSavegameVersionBefore(SaveLoadVersion::ExcavationJobs) && (schema.feature_flags & MULTILAYER_FEATURE_EXCAVATION_JOBS) != 0) SlErrorCorrupt("Unexpected excavation jobs in old save version");
		if (IsSavegameVersionBefore(SaveLoadVersion::ExcavationPasses) && (schema.feature_flags & MULTILAYER_FEATURE_EXCAVATION_PASSES) != 0) SlErrorCorrupt("Unexpected excavation pass in old save version"); // Older formats cannot request a descriptor family they never owned.
		if (IsSavegameVersionBefore(SaveLoadVersion::LayerPortalWork) && (schema.feature_flags & MULTILAYER_FEATURE_PORTAL_WORK) != 0) SlErrorCorrupt("Unexpected portal work in old save version"); // Explicit work records require their actual versioned descriptor family.
		if (IsSavegameVersionBefore(SaveLoadVersion::TBMProjects) && (schema.feature_flags & MULTILAYER_FEATURE_TBM_PROJECTS) != 0) SlErrorCorrupt("Unexpected TBM projects in old save version"); // Typed projects require their versioned descriptor family.
		if (IsSavegameVersionBefore(SaveLoadVersion::SpoilReceivers) && (schema.feature_flags & MULTILAYER_FEATURE_SPOIL_RECEIVERS) != 0) SlErrorCorrupt("Unexpected spoil receivers in old save version"); // Receiver state requires its versioned descriptor family.
		if (IsSavegameVersionBefore(SaveLoadVersion::TBMExchange) && (schema.feature_flags & MULTILAYER_FEATURE_TBM_EXCHANGE) != 0) SlErrorCorrupt("Unexpected TBM exchange in old save version"); // Assignments and sets require their versioned descriptor family.
		// Finite mine bindings require their actual descriptor version; older saves cannot request invented claims.
		if (IsSavegameVersionBefore(SaveLoadVersion::LayerMines) && (schema.feature_flags & MULTILAYER_FEATURE_MINES) != 0) SlErrorCorrupt("Unexpected mines in old save version");
		// Saved recipe counters require their actual descriptor version, never invented defaults in an older file.
		if (IsSavegameVersionBefore(SaveLoadVersion::LayerProcessors) && (schema.feature_flags & MULTILAYER_FEATURE_PROCESSORS) != 0) SlErrorCorrupt("Unexpected processors in old save version");
		// Utility history requires exact epoch/fraction/grant transport; old files
		// cannot request these records through a metadata bit alone.
		if (IsSavegameVersionBefore(SaveLoadVersion::UtilityPower) && (schema.feature_flags & MULTILAYER_FEATURE_UTILITY_POWER) != 0) SlErrorCorrupt("Unexpected utility power in old save version");
		if (ValidateMultiLayerStateReferences(schema) != MultiLayerStateError::None) SlErrorCorrupt("Unsupported or invalid multilayer metadata");
		return schema;
	}

	/** Stage parsed load transport; activated cargo/package validation happens before gameplay in AfterLoadGame. */
	void Load() const override
	{
		StageLoadedMultiLayerStateSchema(this->ReadMetadata());
	}

	/** Recheck metadata after ordinary pointer fixups, without inventing or repairing future feature references. */
	void FixPointers() const override
	{
		// Cleanup's pointer-nulling phase must not revalidate or repair feature transport from a failed candidate.
		if (!SlIsFixingPointers()) return;
		const auto *schema = GetPendingMultiLayerStateSchema();
		// OpenTTD dispatches every registered fixup even when its chunk was absent. Legacy absence is valid; new-version presence is checked before activation.
		if (schema == nullptr) return;
		if (ValidateMultiLayerStateReferences(*schema) != MultiLayerStateError::None) SlErrorCorrupt("Invalid multilayer metadata fixup");
	}

	/**
	 * Validate preview metadata without touching the current selected profile, seed roots or load staging.
	 * @param len Ordinary table payload length supplied by the preview dispatcher; table parser owns it.
	 */
	void LoadCheck([[maybe_unused]] size_t len) const override
	{
		const auto schema = this->ReadMetadata();
		_load_check_data.multilayer_schema_version = schema.schema_version;
		_load_check_data.gameplay_profile_id = to_underlying(schema.profile.id);
		_load_check_data.gameplay_profile_version = schema.profile.version;
		_load_check_data.multilayer_feature_flags = schema.feature_flags;
	}
};

static const MLMDChunkHandler MLMD;
static const ChunkHandlerRef multilayer_chunk_handlers[] = {MLMD};
extern const ChunkHandlerTable _multilayer_chunk_handlers(multilayer_chunk_handlers);
