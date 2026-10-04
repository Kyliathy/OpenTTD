/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_state.cpp Metadata structural/refusal/staging fixtures before feature pools exist. @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_state.h"
#include "../saveload/saveload.h"

#include "../safeguards.h"

/** Return a representative valid feature-reference result without changing any live state. */
static bool ValidReferenceFixture() { return true; }

/** Return a representative orphan-reference refusal without repairing/allocating anything. */
static bool OrphanReferenceFixture() { return false; }

/** Check explicit metadata identity/policy and read-only owner hooks independently of undeveloped feature records. */
TEST_CASE("Metadata rejects unknown identity policy content features and orphan references", "[multilayer-state-foundation]")
{
	ResetMultiLayerProfileSelection();
	ResetMultiLayerStateSchema();
	auto schema = CaptureMultiLayerStateSchema();
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::None);
	CHECK(schema.profile.legacy);
	CHECK(schema.required_content.grfid.Empty());
	STATIC_REQUIRE(to_underlying(SaveLoadVersion::MapLayers) == 369);
	STATIC_REQUIRE(to_underlying(SaveLoadVersion::DeepUnderground) == 370);
	STATIC_REQUIRE(to_underlying(SaveLoadVersion::MultiLayerMetadata) == 371);
	schema.schema_version = 2;
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::UnsupportedSchema);
	schema.schema_version = MULTILAYER_STATE_SCHEMA_VERSION;
	schema.profile.id = MultiLayerGameplayProfileID::Invalid;
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::UnsupportedProfile);
	schema = CaptureMultiLayerStateSchema();
	schema.profile.version = 1;
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::UnsupportedProfile);
	schema.profile.version = 0;
	schema.feature_flags = 1;
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::UnsupportedFeatures);
	schema.feature_flags = 0;
	schema.profile.legacy = false;
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::InvalidPolicy);
	schema.profile.legacy = true;
	const MultiLayerStateReferenceCheck valid[] = {{"deposit", ValidReferenceFixture}};
	const MultiLayerStateReferenceCheck orphan[] = {{"deposit", OrphanReferenceFixture}};
	const MultiLayerStateReferenceCheck duplicate[] = {valid[0], valid[0]};
	CHECK(ValidateMultiLayerStateReferences(schema, valid) == MultiLayerStateError::None);
	CHECK(ValidateMultiLayerStateReferences(schema, orphan) == MultiLayerStateError::InvalidReference);
	CHECK(ValidateMultiLayerStateReferences(schema, duplicate) == MultiLayerStateError::InvalidReference);
	const auto *profile = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(profile != nullptr);
	schema.profile = {profile->id, profile->version, false, true};
	schema.required_content = profile->required_content->package;
	schema.content_version = profile->required_content->version;
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::None);
	schema.required_content.md5sum[0] ^= 1;
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::ChangedContent);
}

/** A parsed row must not publish an editor override or seed root before actual package activation succeeds. */
TEST_CASE("Load staging and failed activation retain initialized Classic authority", "[multilayer-state-foundation]")
{
	ResetMultiLayerProfileSelection();
	ResetMultiLayerStateSchema();
	CHECK(GetPendingMultiLayerStateSchema() == nullptr);
	const auto *profile = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(profile != nullptr);
	MultiLayerStateSchema schema;
	schema.profile = {profile->id, profile->version, false, true};
	schema.required_content = profile->required_content->package;
	schema.content_version = profile->required_content->version;
	schema.geology_seed = 123;
	StageLoadedMultiLayerStateSchema(schema);
	REQUIRE(GetPendingMultiLayerStateSchema() != nullptr);
	CHECK(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::Classic);
	CHECK_FALSE(GetCurrentMultiLayerProfileSelection().editor_override);
	CHECK(CaptureMultiLayerStateSchema().geology_seed == 0);
	CHECK(AcceptLoadedMultiLayerStateSchema(LandscapeType::Temperate, {}) == MultiLayerProfileError::MissingPackage);
	CHECK(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::Classic);
	CHECK(CaptureMultiLayerStateSchema().geology_seed == 0);
	ResetMultiLayerStateSchema();
}

/** Preview can validate found content before activation, while gameplay and missing/substituted packages still refuse. */
TEST_CASE("Metadata package preview uses exact identity without changing cargo slots", "[multilayer-state-package-preview]")
{
	const auto *profile = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(profile != nullptr);
	const auto &required = *profile->required_content;
	GRFConfigList candidate;
	auto &package = candidate.emplace_back(std::make_unique<GRFConfig>("multilayer_industry.grf"));
	package->ident = required.package;
	package->version = required.version;
	package->status = GRFStatus::Unknown;
	CHECK(CheckMultiLayerContentPackageIdentity(required, candidate, false) == MultiLayerContentIdentityError::None);
	CHECK(CheckMultiLayerContentPackageIdentity(required, candidate, true) == MultiLayerContentIdentityError::InactivePackage);
	package->flags.Set(GRFConfigFlag::Compatible);
	CHECK(CheckMultiLayerContentPackageIdentity(required, candidate, false) == MultiLayerContentIdentityError::ChangedPackage);
	package->flags.Reset(GRFConfigFlag::Compatible);
	package->status = GRFStatus::NotFound;
	CHECK(CheckMultiLayerContentPackageIdentity(required, candidate, false) == MultiLayerContentIdentityError::InactivePackage);
	package->status = GRFStatus::Unknown;
	package->ident.md5sum[0] ^= 1;
	CHECK(CheckMultiLayerContentPackageIdentity(required, candidate, false) == MultiLayerContentIdentityError::ChangedPackage);
}

/** Canonical encoding responds to authority changes, rejects ambiguity and excludes presentation-only state. */
TEST_CASE("Canonical state digest is portable ordered and independent of presentation", "[multilayer-state-digest]")
{
	ResetMultiLayerProfileSelection();
	ResetMultiLayerStateSchema();
	auto schema = CaptureMultiLayerStateSchema();
	const auto empty = ComputeMultiLayerStateDigest(schema);
	REQUIRE(empty);
	CHECK(*empty == 0x8f99d566ee8e00d7ULL);
	schema.geology_seed = 1;
	CHECK(ComputeMultiLayerStateDigest(schema) != empty);
	schema.geology_seed = 0;
	schema.profile.editor_override = true;
	CHECK(ComputeMultiLayerStateDigest(schema) != empty);
	schema.profile.editor_override = false;
	MultiLayerCanonicalField deposit[] = {{2, 8, 50}, {1, 4, 10}};
	const MultiLayerCanonicalField meter[] = {{1, 2, 99}};
	const MultiLayerCanonicalRecord records[] = {{MultiLayerCanonicalKind::Utility, 2, meter}, {MultiLayerCanonicalKind::Deposit, 7, deposit}};
	const auto original = ComputeMultiLayerStateDigest(schema, records);
	REQUIRE(original);
	const MultiLayerCanonicalField reversed_fields[] = {deposit[1], deposit[0]};
	const MultiLayerCanonicalRecord reordered[] = {{MultiLayerCanonicalKind::Deposit, 7, reversed_fields}, records[0]};
	CHECK(ComputeMultiLayerStateDigest(schema, reordered) == original);
	deposit[1].value = 11;
	CHECK(ComputeMultiLayerStateDigest(schema, records) != original);
	deposit[1].value = 10;
	const MultiLayerCanonicalRecord duplicate[] = {records[0], records[0]};
	CHECK_FALSE(ComputeMultiLayerStateDigest(schema, duplicate));
	const MultiLayerCanonicalField overflow[] = {{1, 1, 256}};
	const MultiLayerCanonicalRecord invalid[] = {{MultiLayerCanonicalKind::Deposit, 7, overflow}};
	CHECK_FALSE(ComputeMultiLayerStateDigest(schema, invalid));
	const MultiLayerCanonicalField duplicate_tags[] = {{1, 4, 10}, {1, 4, 11}};
	const MultiLayerCanonicalRecord ambiguous[] = {{MultiLayerCanonicalKind::Deposit, 7, duplicate_tags}};
	CHECK_FALSE(ComputeMultiLayerStateDigest(schema, ambiguous));
	const MultiLayerCanonicalRecord present_empty[] = {{MultiLayerCanonicalKind::Deposit, 7, {}}};
	CHECK(ComputeMultiLayerStateDigest(schema, present_empty) != empty);
	/** Test-only presentation wrapper; its view fields are never canonical authority. */
	struct PresentedSnapshot { MultiLayerStateSchema authority; uint8_t selected_layer; uint8_t zoom; };
	PresentedSnapshot view{schema, 0, 1};
	const auto before_view_change = ComputeMultiLayerStateDigest(view.authority, records);
	view.selected_layer = 2;
	view.zoom = 4;
	CHECK(ComputeMultiLayerStateDigest(view.authority, records) == before_view_change);
}
