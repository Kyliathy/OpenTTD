/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_content_identity.cpp Exercise incompatible packages and first-wins cargo ambiguity. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../cargotype.h"
#include "../multilayer_content_identity.h"
#include "../multilayer_content.h"

#include "../safeguards.h"

/**
 * Preserve the actual cargo definitions across isolated identity tests.
 * The public CargoSpec accessor keeps this fixture on the registry's own boundary.
 */
class CargoIdentityFixture {
	std::array<CargoSpec, NUM_CARGO> original; ///< Actual pre-test cargo definitions.
	CargoTypes cargo_mask; ///< Original enabled-slot mask restored with the definitions.

public:
	/** Snapshot existing definitions and prepare two preserved Temperate cargo meanings. */
	CargoIdentityFixture() : cargo_mask(_cargo_mask)
	{
		// Restore every slot on teardown so a failed test cannot leak new cargo meanings into later tests.
		for (uint i = 0; i < NUM_CARGO; ++i) this->original[i] = *CargoSpec::Get(i);
		for (uint i = 0; i < NUM_CARGO; ++i) *CargoSpec::Get(i) = CargoSpec{};
		_cargo_mask = {};
		CargoSpec::Get(1)->label = CT_COAL;
		CargoSpec::Get(1)->bitnum = 1;
		_cargo_mask.Set(CargoType{1});
		CargoSpec::Get(8)->label = CT_IRON_ORE;
		CargoSpec::Get(8)->bitnum = 8;
		_cargo_mask.Set(CargoType{8});
		BuildCargoLabelMap();
	}

	/** Restore definitions and rebuild the derived first-wins label index. */
	~CargoIdentityFixture()
	{
		for (uint i = 0; i < NUM_CARGO; ++i) *CargoSpec::Get(i) = this->original[i];
		_cargo_mask = this->cargo_mask;
		BuildCargoLabelMap();
	}

	/**
	 * Supply required meanings in either preserved slots or a deliberately different live arrangement.
	 * @param labels Immutable profile requirements to make available for activation.
	 * @param scramble Place definitions in descending slots to expose accidental slot assumptions.
	 */
	void Populate(std::span<const CargoLabel> labels, bool scramble = false)
	{
		for (uint i = 0; i < NUM_CARGO; ++i) *CargoSpec::Get(i) = CargoSpec{};
		_cargo_mask = {};
		// The ordinary fixture skips original void slot 11; the alternate arrangement proves identity is label-based.
		for (uint i = 0; i < labels.size(); ++i) {
			uint slot = scramble ? NUM_CARGO - 1 - i : (i < 11 ? i : i + 1);
			CargoSpec::Get(slot)->label = labels[i];
			CargoSpec::Get(slot)->bitnum = slot;
			_cargo_mask.Set(CargoType{static_cast<uint8_t>(slot)});
		}
		BuildCargoLabelMap();
	}
};

/**
 * Construct an exact active package configuration for a narrow identity test.
 * @param identity Expected GRFID/digest copied into the candidate configuration.
 * @param version Definition version reported by the candidate package.
 * @return Single activated candidate, ready for controlled mutation in a test.
 */
static GRFConfigList MakeIdentityCandidate(GRFIdentifier identity, uint32_t version)
{
	GRFConfigList list;
	auto candidate = std::make_unique<GRFConfig>();
	candidate->ident = identity;
	candidate->version = version;
	candidate->status = GRFStatus::Activated;
	list.push_back(std::move(candidate));
	return list;
}

/** Verify multilayer required package rejects substitutions. */
TEST_CASE("Multilayer required package rejects substitutions", "[multilayer-content-identity]")
{
	CargoIdentityFixture fixture;
	const std::array labels = {CT_COAL, CT_IRON_ORE};
	RequiredMultiLayerContent required{{GrfID{"MLVP"}, {}}, 1, labels};
	auto candidate = MakeIdentityCandidate(required.package, required.version);
	CHECK(CheckMultiLayerContentIdentity(required, candidate) == MultiLayerContentIdentityError::None);

	SECTION("Missing or inactive package refuses activation") {
		GRFConfigList missing;
		CHECK(CheckMultiLayerContentIdentity(required, missing) == MultiLayerContentIdentityError::MissingPackage);
		candidate.front()->status = GRFStatus::NotFound;
		CHECK(CheckMultiLayerContentIdentity(required, candidate) == MultiLayerContentIdentityError::InactivePackage);
	}
	SECTION("Compatible substitution with matching GRFID is insufficient") {
		candidate.front()->flags.Set(GRFConfigFlag::Compatible);
		CHECK(CheckMultiLayerContentIdentity(required, candidate) == MultiLayerContentIdentityError::ChangedPackage);
	}
	SECTION("Changed bytes and versions refuse activation") {
		candidate.front()->ident.md5sum[0] = 1;
		CHECK(CheckMultiLayerContentIdentity(required, candidate) == MultiLayerContentIdentityError::ChangedPackage);
		candidate.front()->ident = required.package;
		candidate.front()->version = 2;
		CHECK(CheckMultiLayerContentIdentity(required, candidate) == MultiLayerContentIdentityError::WrongVersion);
	}
	SECTION("Repeated package ownership is ambiguous") {
		auto duplicate = MakeIdentityCandidate(required.package, required.version);
		candidate.push_back(std::move(duplicate.front()));
		CHECK(CheckMultiLayerContentIdentity(required, candidate) == MultiLayerContentIdentityError::ChangedPackage);
	}
	SECTION("Malformed required meanings cannot publish a profile") {
		const std::array repeated = {CT_COAL, CT_COAL};
		required.labels = repeated;
		CHECK(CheckMultiLayerContentIdentity(required, candidate) == MultiLayerContentIdentityError::InvalidRequirement);
		required.labels = {};
		CHECK(CheckMultiLayerContentIdentity(required, candidate) == MultiLayerContentIdentityError::InvalidRequirement);
	}
}

/** Verify multilayer required labels reject missing and duplicate meanings. */
TEST_CASE("Multilayer required labels reject missing and duplicate meanings", "[multilayer-content-identity]")
{
	CargoIdentityFixture fixture;
	const std::array labels = {CT_COAL, CT_COPPER_ORE};
	RequiredMultiLayerContent required{{GrfID{"MLVP"}, {}}, 1, labels};
	auto candidate = MakeIdentityCandidate(required.package, required.version);
	CHECK(CheckMultiLayerContentIdentity(required, candidate) == MultiLayerContentIdentityError::MissingCargo);

	*CargoSpec::Get(12) = *CargoSpec::Get(GetCargoTypeByLabel(CT_IRON_ORE));
	CargoSpec::Get(12)->label = CT_COPPER_ORE;
	CargoSpec::Get(12)->bitnum = 12;
	_cargo_mask.Set(CargoType{12});
	BuildCargoLabelMap();
	CHECK(CheckMultiLayerContentIdentity(required, candidate) == MultiLayerContentIdentityError::None);

	*CargoSpec::Get(13) = *CargoSpec::Get(12);
	CargoSpec::Get(13)->bitnum = 13;
	_cargo_mask.Set(CargoType{13});
	BuildCargoLabelMap();
	CHECK(GetCargoTypeByLabel(CT_COPPER_ORE) == CargoType{12});
	CHECK(CheckMultiLayerContentIdentity(required, candidate) == MultiLayerContentIdentityError::DuplicateCargo);
}

/** Verify multilayer profile preserves Classic and rejects unsupported combinations. */
TEST_CASE("Multilayer profile preserves Classic and rejects unsupported combinations", "[multilayer-profile-selection]")
{
	CargoIdentityFixture fixture;
	ResetMultiLayerProfileSelection();
	CHECK(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::Classic);
	CHECK(GetCurrentMultiLayerProfileSelection().legacy);
	CHECK(GetCurrentMultiLayerProfileSelection().version == 0);
	CHECK_FALSE(GetCurrentMultiLayerProfileSelection().editor_override);
	CHECK(GetMultiLayerContentProfile(MultiLayerGameplayProfileID::Invalid) == nullptr);
	CHECK(GetMultiLayerContentProfile(MultiLayerGameplayProfileID::Classic)->required_content == nullptr);

	GRFConfigList candidate;
	CHECK(PrepareMultiLayerProfileContent(MultiLayerGameplayProfileID::Invalid, LandscapeType::Temperate, candidate) == MultiLayerProfileError::UnknownProfile);
	CHECK(PrepareMultiLayerProfileContent(MultiLayerGameplayProfileID::MultilayerIndustry, LandscapeType::Arctic, candidate) == MultiLayerProfileError::UnsupportedClimate);
	CHECK(candidate.empty());
	CHECK(ActivateMultiLayerProfile(MultiLayerGameplayProfileID::MultilayerIndustry, LandscapeType::Temperate, candidate) == MultiLayerProfileError::MissingPackage);
	CHECK(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::Classic);

	candidate = MakeIdentityCandidate({GrfID{"FORE"}, {}}, 1);
	CHECK(PrepareMultiLayerProfileContent(MultiLayerGameplayProfileID::MultilayerIndustry, LandscapeType::Temperate, candidate) == MultiLayerProfileError::UnsupportedContent);
	CHECK(candidate.size() == 1);
	CHECK(PrepareMultiLayerProfileContent(MultiLayerGameplayProfileID::Classic, LandscapeType::Tropic, candidate) == MultiLayerProfileError::None);
	CHECK(ActivateMultiLayerProfile(MultiLayerGameplayProfileID::Classic, LandscapeType::Tropic, candidate) == MultiLayerProfileError::None);
	CHECK(GetCurrentMultiLayerProfileSelection().legacy);
}

/** Verify multilayer activation publishes only an exact complete profile. */
TEST_CASE("Multilayer activation publishes only an exact complete profile", "[multilayer-profile-selection]")
{
	CargoIdentityFixture fixture;
	ResetMultiLayerProfileSelection();
	const auto *profile = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(profile != nullptr);
	REQUIRE(profile->required_content != nullptr);
	CHECK(profile->required_content->labels.size() == 30);
	const auto &required = *profile->required_content;
	auto candidate = MakeIdentityCandidate(required.package, required.version);
	CHECK(ActivateMultiLayerProfile(profile->id, LandscapeType::Temperate, candidate) == MultiLayerProfileError::AmbiguousCargo);
	CHECK(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::Classic);

	SECTION("Activated catalogue preserves the Classic envelope") {
		fixture.Populate(required.labels);
		CHECK(GetCargoTypeByLabel(CT_COAL) == CargoType{1});
		CHECK(GetCargoTypeByLabel(CT_IRON_ORE) == CargoType{8});
		CHECK(GetCargoTypeByLabel(CT_COPPER_ORE) == CargoType{12});
	}
	SECTION("A different runtime arrangement has the same profile identity") {
		fixture.Populate(required.labels, true);
		CHECK(GetCargoTypeByLabel(CT_COAL) != CargoType{1});
	}
	CHECK(ActivateMultiLayerProfile(profile->id, LandscapeType::Temperate, candidate) == MultiLayerProfileError::None);
	CHECK(GetCurrentMultiLayerProfileSelection().id == profile->id);
	CHECK(GetCurrentMultiLayerProfileSelection().version == profile->version);
	CHECK_FALSE(GetCurrentMultiLayerProfileSelection().legacy);
	CHECK_FALSE(GetCurrentMultiLayerProfileSelection().editor_override);
	CHECK(ActivateMultiLayerProfile(profile->id, LandscapeType::Toyland, candidate) == MultiLayerProfileError::UnsupportedClimate);
	CHECK(GetCurrentMultiLayerProfileSelection().id == profile->id);
	ResetMultiLayerProfileSelection();
}
