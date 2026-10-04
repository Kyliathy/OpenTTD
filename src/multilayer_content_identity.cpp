/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_content_identity.cpp Validate exact content without substituting cargo meanings. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "stdafx.h"
#include "multilayer_content_identity.h"
#include "cargotype.h"

#include "safeguards.h"

/**
 * Validate one exact package independently of not-yet-loaded cargo definitions.
 * @param required Pinned identity/version from the immutable profile.
 * @param entries Actual resolved content configuration.
 * @param require_activation Whether the staged loader must already have activated the package.
 * @return Deterministic package refusal or None, without changing configuration.
 */
MultiLayerContentIdentityError CheckMultiLayerContentPackageIdentity(const RequiredMultiLayerContent &required, const GRFConfigList &entries, bool require_activation)
{
	if (required.package.grfid.Empty()) return MultiLayerContentIdentityError::InvalidRequirement;
	const GRFConfig *candidate = nullptr;
	// Preserve one package owner; duplicate entries cannot establish a saved content meaning.
	for (const auto &entry : entries) {
		if (entry->ident.grfid != required.package.grfid) continue;
		if (candidate != nullptr) return MultiLayerContentIdentityError::ChangedPackage;
		candidate = entry.get();
	}
	if (candidate == nullptr) return MultiLayerContentIdentityError::MissingPackage;
	if (candidate->status == GRFStatus::NotFound) return MultiLayerContentIdentityError::InactivePackage;
	if (candidate->status == GRFStatus::Disabled) return MultiLayerContentIdentityError::InactivePackage;
	// Preview may inspect a found package before activation, but gameplay cannot publish an unactivated registry.
	if (require_activation && candidate->status != GRFStatus::Activated) return MultiLayerContentIdentityError::InactivePackage;
	if (candidate->flags.Test(GRFConfigFlag::Compatible)) return MultiLayerContentIdentityError::ChangedPackage;
	if (candidate->ident.md5sum != required.package.md5sum) return MultiLayerContentIdentityError::ChangedPackage;
	if (candidate->version != required.version) return MultiLayerContentIdentityError::WrongVersion;
	return MultiLayerContentIdentityError::None;
}

/**
 * Validate the authoritative profile's identity against the actual activated registry.
 * @param required Exact package digest/version and unique required cargo labels.
 * @param activated Configuration entries retained by the staged content loader.
 * @return Deterministic first error, without changing the candidate definitions.
 */
MultiLayerContentIdentityError CheckMultiLayerContentIdentity(const RequiredMultiLayerContent &required, const GRFConfigList &activated)
{
	if (required.package.grfid.Empty()) return MultiLayerContentIdentityError::InvalidRequirement;
	if (required.labels.empty()) return MultiLayerContentIdentityError::InvalidRequirement;
	if (required.labels.size() > NUM_CARGO) return MultiLayerContentIdentityError::InvalidRequirement;
	// An ambiguous requirement cannot establish a saved cargo meaning, even if the live map seems valid.
	for (size_t i = 0; i < required.labels.size(); ++i) {
		if (required.labels[i].Empty()) return MultiLayerContentIdentityError::InvalidRequirement;
		if (required.labels[i] == CT_INVALID) return MultiLayerContentIdentityError::InvalidRequirement;
		if (std::find(required.labels.begin(), required.labels.begin() + i, required.labels[i]) != required.labels.begin() + i) return MultiLayerContentIdentityError::InvalidRequirement;
	}

	const auto package_error = CheckMultiLayerContentPackageIdentity(required, activated, true);
	if (package_error != MultiLayerContentIdentityError::None) return package_error;

	// Count every live slot for each required meaning; BuildCargoLabelMap silently skips duplicate labels.
	for (CargoLabel label : required.labels) {
		uint matches = 0;
		for (const CargoSpec *cargo : CargoSpec::Iterate()) {
			if (cargo->label == label) ++matches;
		}
		if (matches == 0) return MultiLayerContentIdentityError::MissingCargo;
		if (matches > 1) return MultiLayerContentIdentityError::DuplicateCargo;
	}
	return MultiLayerContentIdentityError::None;
}
