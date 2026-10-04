/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_content_identity.h Exact required-content validation. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#ifndef MULTILAYER_CONTENT_IDENTITY_H
#define MULTILAYER_CONTENT_IDENTITY_H

#include "cargo_type.h"
#include "newgrf_config.h"

/** Immutable package and cargo requirements supplied by the authoritative profile. */
struct RequiredMultiLayerContent {
	GRFIdentifier package; ///< Exact GRFID and digest; compatible replacements are insufficient.
	uint32_t version; ///< Exact package definition version, separate from save schema version.
	std::span<const CargoLabel> labels; ///< Unique required meanings, resolved against activated definitions.
};

/** Specific refusal causes, before a profile may authorize dependent gameplay. */
enum class MultiLayerContentIdentityError : uint8_t {
	None, ///< Exact activated package and unambiguous required cargo meanings.
	InvalidRequirement, ///< Empty or repeated required label, or missing package identity.
	MissingPackage, ///< Required GRFID is absent from the selected configuration.
	InactivePackage, ///< Package was missing, disabled, or failed activation.
	ChangedPackage, ///< Digest changed or the loader selected a compatible replacement.
	WrongVersion, ///< Definition version differs from the pinned version.
	MissingCargo, ///< An activated required meaning is absent.
	DuplicateCargo, ///< Multiple activated slots claim a required meaning.
};

/**
 * Validate pinned content after loading candidates and before accepting the world.
 * This reads the real CargoSpec definitions rather than the first-wins label map.
 * It consumes no cargo, funds, random values, or settings and does not publish a profile.
 * @param required Immutable package identity/version and labels owned by the profile.
 * @param activated Candidate configuration after the staged loader has finished.
 * @return First deterministic refusal cause, or None when every requirement is met.
 */
MultiLayerContentIdentityError CheckMultiLayerContentIdentity(const RequiredMultiLayerContent &required, const GRFConfigList &activated);

/**
 * Validate exact package identity before activation/for preview without reading a cargo registry.
 * @param required Pinned GRFID/digest/version; cargo meanings are checked separately after activation.
 * @param candidate Actual resolved loader/preview configuration, never an arbitrary filename assumption.
 * @param require_activation True for gameplay acceptance; false allows found pre-activation entries, never missing/disabled packages.
 * @return Exact package result, with compatible substitutions and duplicate ownership refused.
 */
MultiLayerContentIdentityError CheckMultiLayerContentPackageIdentity(const RequiredMultiLayerContent &required, const GRFConfigList &candidate, bool require_activation);

#endif /* MULTILAYER_CONTENT_IDENTITY_H */
