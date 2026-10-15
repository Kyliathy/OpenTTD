/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_content.cpp Validate and publish one authoritative profile at world activation. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "stdafx.h"
#include "multilayer_content.h"
#include "table/multilayer_content_manifest.h"

#include "safeguards.h"

/**
 * Bind the generated immutable identity using the existing MD5Hash container's public API.
 * @return Exact package version/digest and required labels from the single source catalogue.
 */
static RequiredMultiLayerContent MakeMultiLayerRequirements()
{
	RequiredMultiLayerContent required{{_multilayer_package_id, {}}, _multilayer_package_version, _multilayer_required_labels};
	std::copy(std::begin(_multilayer_package_digest), std::end(_multilayer_package_digest), required.package.md5sum.begin());
	return required;
}

/** @return Sole shared Industry Q16, tick and travel-strip defaults, unchanged from the delivered Space producer. */
const MultiLayerSpaceTuning &GetMultiLayerSpaceTuning()
{
	static const MultiLayerSpaceTuning tuning;
	return tuning;
}

static const RequiredMultiLayerContent _multilayer_required_content = MakeMultiLayerRequirements(); ///< Manifest bound to the generated definition bytes.
static MultiLayerProfileSelection _multilayer_selection; ///< World-owned policy; persistence is supplied by the schema handoff.
static const MultiLayerGameplayProfile _classic_profile{MultiLayerGameplayProfileID::Classic, 0, nullptr}; ///< Unmodified Classic/legacy contract.
static const MultiLayerGameplayProfile _industry_profile{
	.id = MultiLayerGameplayProfileID::MultilayerIndustry,
	.version = _multilayer_profile_version,
	.required_content = &_multilayer_required_content,
	.asteroid_resources = GetMultiLayerAsteroidDefinitions(),
	.asteroid_sizes = GetMultiLayerAsteroidSizeDefinitions(),
	.space_tuning = &GetMultiLayerSpaceTuning(),
}; ///< First-party contract, including immutable finite asteroid content; Classic exposes no field definitions.

/**
 * Restore saved exceptions only after the selected definitions are checked again.
 * @param selection Validated saved profile/version and policy flags.
 * @param landscape Loaded world climate.
 * @param activated Actual activated package and cargo definitions.
 * @return Refusal or None; the current profile remains the sole accepted-policy authority.
 */
MultiLayerProfileError RestoreMultiLayerProfileSelection(const MultiLayerProfileSelection &selection, LandscapeType landscape, const GRFConfigList &activated)
{
	const auto *profile = GetMultiLayerContentProfile(selection.id);
	if (profile == nullptr) return MultiLayerProfileError::UnknownProfile;
	if (selection.version != profile->version) return MultiLayerProfileError::UnknownProfile;
	const auto error = ActivateMultiLayerProfile(selection.id, landscape, activated);
	if (error != MultiLayerProfileError::None) return error;
	_multilayer_selection = selection;
	return MultiLayerProfileError::None;
}

/** Existing baseline meanings used by manufacturing; their original definitions remain untouched. */
static constexpr MultiLayerCargoContract _baseline_recipe_cargo_contracts[] = {
	{CT_COAL, MultiLayerCargoForm::DryBulk, "tonnes"},
	{CT_IRON_ORE, MultiLayerCargoForm::DryBulk, "tonnes"},
	{CT_STEEL, MultiLayerCargoForm::Ingots, "tonnes"},
	{CT_OIL, MultiLayerCargoForm::Liquid, "kilolitres"},
	{CT_GOODS, MultiLayerCargoForm::Packaged, "crates"},
};

/**
 * Read physical traits without allocating or replacing a cargo definition.
 * @param label Required recipe/refit meaning, independently of its current slot.
 * @return Authoritative declared form/unit, or nullptr for an unsupported recipe cargo.
 */
const MultiLayerCargoContract *GetMultiLayerCargoContract(CargoLabel label)
{
	// Baseline traits describe preserved meanings; added traits come from the same catalogue as the GRF bytes.
	for (const auto &cargo : _baseline_recipe_cargo_contracts) {
		if (cargo.label == label) return &cargo;
	}
	for (const auto &cargo : _multilayer_cargo_contracts) {
		if (cargo.label == label) return &cargo;
	}
	return nullptr;
}

/**
 * Resolve immutable content by its saved identity.
 * @param id Stable profile identity; no runtime cargo slot is used as a key.
 * @return Immutable profile or nullptr when the identity is unsupported.
 */
const MultiLayerGameplayProfile *GetMultiLayerContentProfile(MultiLayerGameplayProfileID id)
{
	switch (id) {
		case MultiLayerGameplayProfileID::Classic: return &_classic_profile;
		case MultiLayerGameplayProfileID::MultilayerIndustry: return &_industry_profile;
		default: return nullptr;
	}
}

/**
 * Apply the shared Deep calendar contract without inferring policy from editor/view state.
 * @param selection Explicit saved identity/version and exception flags.
 * @param calendar_date Calendar date supplied by the synchronized caller.
 * @return Whether the profile allows Deep operations at this date.
 */
bool CheckDeepLayerAvailability(const MultiLayerProfileSelection &selection, TimerGameCalendar::Date calendar_date)
{
	const auto *profile = GetMultiLayerContentProfile(selection.id);
	if (profile == nullptr) return false;
	if (selection.version != profile->version) return false;
	if (selection.id == MultiLayerGameplayProfileID::Classic) return true;
	// Only Classic carries migrated legacy behavior; setting a legacy flag must not bypass a new profile's technology gate.
	if (selection.legacy) return false;
	if (selection.editor_override) return true;
	const auto unlock = TimerGameCalendar::ConvertYMDToDate(profile->deep_unlock_year, 0, 1);
	return calendar_date >= unlock;
}

/** Read accepted world policy without exposing a mutable settings or pool reference. */
const MultiLayerProfileSelection &GetCurrentMultiLayerProfileSelection()
{
	return _multilayer_selection;
}

/** Initialize the absence-of-profile policy before any candidate world is accepted. */
void ResetMultiLayerProfileSelection()
{
	_multilayer_selection = {};
}

/**
 * Check supported profile/climate and exclusive first-party package ownership.
 * @param id Requested stable profile identity.
 * @param landscape Requested climate.
 * @param candidate Actual selected configuration, including system baseset GRFs.
 * @return Specific refusal cause without altering definitions or world state.
 */
static MultiLayerProfileError CheckProfileCombination(MultiLayerGameplayProfileID id, LandscapeType landscape, const GRFConfigList &candidate)
{
	const auto *profile = GetMultiLayerContentProfile(id);
	if (profile == nullptr) return MultiLayerProfileError::UnknownProfile;
	if (id == MultiLayerGameplayProfileID::Classic) return MultiLayerProfileError::None;
	if (landscape != LandscapeType::Temperate) return MultiLayerProfileError::UnsupportedClimate;
	// Only baseset system definitions and the pinned package may share this initial economy.
	// Static/external content needs a named tested adapter; arbitrary load order cannot redefine a cargo meaning.
	for (const auto &entry : candidate) {
		if (entry->flags.Test(GRFConfigFlag::System)) continue;
		if (entry->ident.grfid != profile->required_content->package.grfid) return MultiLayerProfileError::UnsupportedContent;
	}
	return MultiLayerProfileError::None;
}

/**
 * Add the distributed first-party package to a supported new-world candidate.
 * @param id Requested stable identity.
 * @param landscape Requested climate.
 * @param candidate Resolved new-game configuration to be activated by the loader.
 * @return Preparation refusal; no live world policy is published here.
 */
MultiLayerProfileError PrepareMultiLayerProfileContent(MultiLayerGameplayProfileID id, LandscapeType landscape, GRFConfigList &candidate)
{
	auto error = CheckProfileCombination(id, landscape, candidate);
	if (error != MultiLayerProfileError::None) return error;
	if (id == MultiLayerGameplayProfileID::Classic) return MultiLayerProfileError::None;
	const auto &required = *GetMultiLayerContentProfile(id)->required_content;
	// Keep an explicitly selected exact package, otherwise append the distributed definition once.
	for (const auto &entry : candidate) {
		if (entry->ident.grfid == required.package.grfid) {
			if (entry->ident.md5sum != required.package.md5sum) return MultiLayerProfileError::ChangedPackage;
			if (entry->version != required.version) return MultiLayerProfileError::ChangedPackage;
			return MultiLayerProfileError::None;
		}
	}
	auto content = std::make_unique<GRFConfig>("multilayer_industry.grf");
	if (!FillGRFDetails(*content, false)) return MultiLayerProfileError::MissingPackage;
	if (content->ident.md5sum != required.package.md5sum) return MultiLayerProfileError::ChangedPackage;
	if (content->version != required.version) return MultiLayerProfileError::ChangedPackage;
	candidate.push_back(std::move(content));
	return MultiLayerProfileError::None;
}

/**
 * Publish one accepted new-world policy after the exact activated registry passes.
 * @param id Requested profile identity.
 * @param landscape Candidate world climate.
 * @param activated Loader-owned configuration after definition activation.
 * @return Refusal cause; no failed candidate may publish Multilayer policy.
 */
MultiLayerProfileError ActivateMultiLayerProfile(MultiLayerGameplayProfileID id, LandscapeType landscape, const GRFConfigList &activated)
{
	auto error = CheckProfileCombination(id, landscape, activated);
	if (error != MultiLayerProfileError::None) return error;
	const auto *profile = GetMultiLayerContentProfile(id);
	if (profile->required_content != nullptr) {
		switch (CheckMultiLayerContentIdentity(*profile->required_content, activated)) {
			case MultiLayerContentIdentityError::None: break;
			case MultiLayerContentIdentityError::MissingPackage:
			case MultiLayerContentIdentityError::InactivePackage: return MultiLayerProfileError::MissingPackage;
			case MultiLayerContentIdentityError::MissingCargo:
			case MultiLayerContentIdentityError::DuplicateCargo: return MultiLayerProfileError::AmbiguousCargo;
			default: return MultiLayerProfileError::ChangedPackage;
		}
	}
	_multilayer_selection = {id, profile->version, id == MultiLayerGameplayProfileID::Classic, false};
	return MultiLayerProfileError::None;
}

/**
 * Explain selection refusal without disguising it as a compatible replacement.
 * @param error Preparation or activation result.
 * @return Stable plain-language diagnostic for UI and dedicated logging.
 */
std::string_view GetMultiLayerProfileErrorMessage(MultiLayerProfileError error)
{
	switch (error) {
		case MultiLayerProfileError::None: return {};
		case MultiLayerProfileError::UnknownProfile: return "Unknown multilayer gameplay profile.";
		case MultiLayerProfileError::UnsupportedClimate: return "Multilayer Industry currently supports the Temperate climate.";
		case MultiLayerProfileError::UnsupportedContent: return "Multilayer Industry requires its first-party content; the selected external set has no supported adapter.";
		case MultiLayerProfileError::MissingPackage: return "The required Multilayer Industry content package is missing or inactive.";
		case MultiLayerProfileError::ChangedPackage: return "The Multilayer Industry package does not match the required identity and version.";
		case MultiLayerProfileError::AmbiguousCargo: return "Required Multilayer Industry cargo meanings are missing or duplicated.";
	}
	return "Unknown multilayer profile refusal.";
}
