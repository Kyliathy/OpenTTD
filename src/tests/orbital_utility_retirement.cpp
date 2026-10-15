/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_utility_retirement.cpp Atomic retirement boundaries for native orbital panel/work accounts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups4C/5.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../utility_power.h"
#include "../core/backup_type.hpp"
#include "excavation_fixture.h"
#include "../safeguards.h"

/** Real profile/company with isolated original ledger identities; no constructed gameplay module is implied. */
class OrbitalRetirementRig {
	ExcavationFixture world;
public:
	/** Begin the isolated ledger while the real profile/company exist. */
	OrbitalRetirementRig() { ResetUtilityPowerState(); }
	/** Release only isolated ledger records before the company/profile fixture ends. */
	~OrbitalRetirementRig() { ResetUtilityPowerState(); }
};

/** Complete isolated physical participant used solely to test settled ledger lineage, rather than claim delivered gameplay. */
class OrbitalRetirementBenefit final : public UtilityMutationParticipant {
public:
	uint32_t applied = 0;
	/** @return Exact physical account role admitted by this boundary test. */
	std::optional<UtilityConsumerEffect> EffectKind() const override { return UtilityConsumerEffect::AssemblyWork; }
	/** @param proposal Actual shared grant proposal. @return Whether this test's whole benefit can still be applied once. */
	bool Validate(const UtilityMutationProposal &proposal) const override { return this->applied == 0 && proposal.quantity_j == 500; }
	/** @param proposal Already admitted complete proposal. @return Whole accepted benefit; there is no nested ledger operation. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override { (void)proposal; ++this->applied; return true; }
};

/** Retirement preserves paid use, expires only selected unused slices and preflights every reference before mutating either account. */
TEST_CASE("Orbital account retirement is atomic and preserves unrelated reservations and settled use", "[orbital-dock-contract]")
{
	OrbitalRetirementRig rig;
	const auto panel = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::OrbitalPanel);
	const auto other_panel = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::OrbitalPanel);
	REQUIRE(panel); REQUIRE(other_panel);
	const std::array sources{*panel}, others{*other_panel};
	const auto service = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, 1500, sources);
	const auto construction = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, 750, sources);
	const auto unrelated = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, 750, others);
	REQUIRE(service); REQUIRE(construction); REQUIRE(unrelated);
	REQUIRE(PublishUtilityGeneration(*panel, 1, 100000, 1) == UtilityPowerError::None);
	REQUIRE(PublishUtilityGeneration(*other_panel, 1, 60000, 1) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		const std::array requests{UtilityPowerRequest{*service, 1500}, UtilityPowerRequest{*construction, 750}, UtilityPowerRequest{*unrelated, 750}};
		REQUIRE(AllocateUtilityPower(1, requests) == UtilityPowerError::None);
	}
	OrbitalRetirementBenefit benefit;
	const auto proposal = PrepareUtilityMutation(UtilityMutationKind::Consume, {*service, 1}, 500);
	REQUIRE(CommitUtilityMutation(proposal, &benefit).error == UtilityPowerError::None); CHECK(benefit.applied == 1);
	const auto before = ComputeUtilityPowerDigest(); const std::array selected{*service, *construction};
	const std::array invalid{*service, UtilityConsumerID{UINT32_MAX}}, duplicate{*service, *service};
	CHECK(RetireOrbitalUtilityAccounts(sources, invalid) == UtilityPowerError::InvalidReference);
	CHECK(RetireOrbitalUtilityAccounts(sources, duplicate) == UtilityPowerError::InvalidReference);
	CHECK(RetireOrbitalUtilityAccounts(sources, selected, false) == UtilityPowerError::None);
	{ AutoRestoreBackup foreign(_current_company, COMPANY_SPECTATOR); CHECK(RetireOrbitalUtilityAccounts(sources, selected) == UtilityPowerError::InvalidOwner); }
	CHECK(ComputeUtilityPowerDigest() == before);
	const auto original = CaptureUtilityPowerState();
	SECTION("Disconnect one consumer while the same original source still serves another job") {
		const std::array only_service{*service};
		REQUIRE(RetireOrbitalUtilityAccounts({}, only_service) == UtilityPowerError::None);
		CHECK(GetUtilityPowerProducer(*panel)->active); CHECK(GetUtilityPowerProducer(*panel)->reserved_j == 750);
		CHECK(GetUtilityPowerProducer(*panel)->consumed_lifetime_j == 500); CHECK_FALSE(GetUtilityPowerConsumer(*service)->active);
		CHECK(GetUtilityPowerConsumer(*service)->shares.front().consumed_j == 500); CHECK(GetUtilityPowerConsumer(*service)->shares.front().released_j == 1000);
		CHECK(GetUtilityAllocation(*construction)->granted_energy_j == 750); CHECK(GetUtilityAllocation(*unrelated)->granted_energy_j == 750);
		CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
		return;
	}
	SECTION("Retire every native account as one whole operation") {
		REQUIRE(RetireOrbitalUtilityAccounts(sources, selected) == UtilityPowerError::None);
		CHECK(CaptureUtilityPowerState().operation_sequence == original.operation_sequence + 1);
		CHECK_FALSE(GetUtilityPowerProducer(*panel)->active); CHECK(GetUtilityPowerProducer(*panel)->reserved_j == 0);
		CHECK(GetUtilityPowerProducer(*panel)->consumed_lifetime_j == 500);
		CHECK_FALSE(GetUtilityPowerConsumer(*service)->active); CHECK_FALSE(GetUtilityPowerConsumer(*construction)->active);
		CHECK(GetUtilityPowerConsumer(*service)->shares.front().consumed_j == 500);
		CHECK(GetUtilityPowerConsumer(*service)->shares.front().released_j == 1000);
		CHECK(GetUtilityPowerConsumer(*construction)->shares.front().released_j == 750);
		CHECK(GetUtilityAllocation(*unrelated)->granted_energy_j == 750); CHECK(GetUtilityPowerProducer(*other_panel)->reserved_j == 750);
		CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
		const auto retired = ComputeUtilityPowerDigest(); CHECK(RetireOrbitalUtilityAccounts(sources, selected) == UtilityPowerError::None);
		CHECK(ComputeUtilityPowerDigest() == retired);
		// A single remaining sequence identity cannot retire one of several accounts first and then fail on the rest.
		auto limited = original; limited.operation_sequence = UINT64_MAX - 1;
		ResetUtilityPowerState(); StageLoadedUtilityPowerState(limited); REQUIRE(AcceptLoadedUtilityPowerState());
		const auto bounded = ComputeUtilityPowerDigest(); CHECK(RetireOrbitalUtilityAccounts(sources, selected) == UtilityPowerError::Limit);
		CHECK(ComputeUtilityPowerDigest() == bounded);
	}
}
