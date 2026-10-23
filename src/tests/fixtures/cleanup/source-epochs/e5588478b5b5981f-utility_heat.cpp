/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_heat.cpp Actual shared native identities/meters and typed thermal settlement boundaries; operating physical plants remain their geothermal owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group2B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md Group15
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../utility_power.h"
#include "../core/backup_type.hpp"
#include "../economy_func.h"
#include "../safeguards.h"

/** Actual company/profile/shared native ledger fixture; no source/consumer ID, balance or private energy store is manufactured. */
class UtilityHeatRig {
public:
	ExcavationFixture world; ///< Canonical actual Native/Deep/calendar/company owners.
	/** Start one empty actual utility owner before typed registration tests. */
	UtilityHeatRig() { ResetUtilityPowerState(); }
	/** Clear fixture-owned actual utility history before its company/profile teardown. */
	~UtilityHeatRig() { ResetUtilityPowerState(); }
};

/** Declared unit-boundary whole benefit; this fixture tests native atomic/domain admission without claiming an operating geothermal plant or transported stock. */
class UtilityDomainBenefit : public UtilityMutationParticipant {
public:
	UtilityEnergyDomain domain; ///< Actual declared physical benefit domain.
	std::optional<UtilityHeatUse> use; ///< Immutable direct/generator role when thermal.
	uint64_t completed_j = 0; ///< Explicit test benefit receipt, not another utility inventory or production claim.
	bool accept = true; ///< Whole unchanged refusal injection before any benefit/energy debit.
	/** @param domain Actual declared benefit domain. @param use Thermal role, absent for electrical work. */
	UtilityDomainBenefit(UtilityEnergyDomain domain, std::optional<UtilityHeatUse> use = std::nullopt) : domain(domain), use(use) {}
	/** @return This whole test benefit's immutable quantity domain. */
	UtilityEnergyDomain EnergyDomain() const override { return this->domain; }
	/** @return This whole test benefit's immutable thermal role. */
	std::optional<UtilityHeatUse> ThermalUse() const override { return this->use; }
	/** @param proposal Exact prepared shared owner quantity. @return Whole benefit admission without changing its receipt or any utility balance. */
	bool Validate(const UtilityMutationProposal &proposal) const override { return this->accept && proposal.domain == this->domain; }
	/** @param proposal Exact revalidated native use. @return Whole receipt or unchanged refusal; the native ledger commits the same typed quantity only after success. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override
	{
		if (!this->accept) return false;
		this->completed_j += proposal.quantity_j; return true;
	}
};

/** The same canonical allocators issue thermal/electrical IDs, while immutable role/domain/source gates prevent cross-domain registration and publication. */
TEST_CASE("Native heat identities share the actual utility allocator and reject electrical substitution", "[utility-heat-domain]")
{
	UtilityHeatRig rig; const auto owner = _current_company;
	const auto before = ComputeUtilityPowerDigest(); const auto estimate = RegisterUtilityHeatProducer(owner, false); REQUIRE(estimate.has_value());
	CHECK(ComputeUtilityPowerDigest() == before); const auto heat = RegisterUtilityHeatProducer(owner); REQUIRE(heat == estimate);
	const auto electric = RegisterUtilityPowerProducer(owner, MapLayer::DeepUnderground, UtilityGenerationKind::External); REQUIRE(electric.has_value());
	CHECK(*electric != *heat); CHECK(GetUtilityPowerProducer(*heat)->meter.domain == UtilityEnergyDomain::Heat); CHECK(HasUtilityHeatState());
	const std::array heat_sources{*heat}, electric_sources{*electric};
	CHECK_FALSE(RegisterUtilityPowerConsumer(owner, MapLayer::DeepUnderground, UtilityConsumerEffect::TubeWork, 100, heat_sources).has_value());
	CHECK_FALSE(RegisterUtilityHeatConsumer(owner, UtilityHeatUse::DirectRecipe, 100, electric_sources).has_value());
	CHECK_FALSE(RegisterUtilityPowerConsumer(owner, MapLayer::DeepUnderground, UtilityConsumerEffect::HeatGenerator, 100, {}).has_value());
	CHECK(PublishUtilityGeneration(*heat, 1, 100, 1) == UtilityPowerError::InvalidReference);
	CHECK(PublishUtilityHeatGeneration(*electric, 1, 100, 1) == UtilityPowerError::InvalidReference);
	REQUIRE(PublishUtilityHeatGeneration(*heat, 1, 17, 37) == UtilityPowerError::None);
	CHECK(GetUtilityPowerProducer(*heat)->meter.generation_energy_j == 16); CHECK(GetUtilityPowerProducer(*heat)->meter.remainder_mj == 983);
	CHECK(PublishUtilityHeatGeneration(*heat, 1, 17, 37) == UtilityPowerError::DuplicatePublication);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Direct and generator roles share one source budget in frozen ID order; native electrical allocation retains its separate supply and cannot overwrite thermal grants. */
TEST_CASE("Native heat reservations and whole use preserve independent electrical quantities", "[utility-heat-domain]")
{
	UtilityHeatRig rig; const auto heat = RegisterUtilityHeatProducer(_current_company); REQUIRE(heat.has_value());
	const auto electric = RegisterUtilityPowerProducer(_current_company, MapLayer::DeepUnderground, UtilityGenerationKind::External); REQUIRE(electric.has_value());
	const auto direct = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::DirectRecipe, 20000, std::array{*heat}); REQUIRE(direct.has_value());
	const auto powered = RegisterUtilityPowerConsumer(_current_company, MapLayer::DeepUnderground, UtilityConsumerEffect::TubeWork, 20000, std::array{*electric}); REQUIRE(powered.has_value());
	const auto generator = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::ElectricalGenerator, 20000, std::array{*heat}); REQUIRE(generator.has_value());
	REQUIRE(PublishUtilityHeatGeneration(*heat, 1, 1000, 1000) == UtilityPowerError::None);
	REQUIRE(PublishUtilityGeneration(*electric, 1, 1000, 400) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		CHECK(AllocateUtilityHeat(1, std::array{UtilityPowerRequest{*powered, 1}}) == UtilityPowerError::InvalidReference);
		CHECK(AllocateUtilityPower(1, std::array{UtilityPowerRequest{*direct, 1}}) == UtilityPowerError::InvalidReference);
		REQUIRE(AllocateUtilityHeat(1, std::array{UtilityPowerRequest{*generator, 17000}, UtilityPowerRequest{*direct, 17000}}) == UtilityPowerError::None);
		REQUIRE(AllocateUtilityPower(1, std::array{UtilityPowerRequest{*powered, 15000}}) == UtilityPowerError::None);
	}
	REQUIRE(GetUtilityHeatAllocation(*direct).has_value()); CHECK(GetUtilityHeatAllocation(*direct)->granted_energy_j == 17000);
	CHECK(GetUtilityHeatAllocation(*generator)->granted_energy_j == 10000); CHECK(GetUtilityAllocation(*powered)->granted_energy_j == 10800);
	CHECK_FALSE(GetUtilityAllocation(*direct).has_value()); CHECK_FALSE(GetUtilityHeatAllocation(*powered).has_value());
	CHECK(PrepareUtilityMutation(UtilityMutationKind::Consume, {*generator, 1}, 1).error == UtilityPowerError::InvalidReference);
	const auto proposal = PrepareUtilityHeatMutation(UtilityMutationKind::Consume, {*generator, 1}, 5000); REQUIRE(proposal.error == UtilityPowerError::None);
	const auto before = ComputeUtilityPowerDigest();
	UtilityDomainBenefit wrong_domain(UtilityEnergyDomain::Electricity), wrong_role(UtilityEnergyDomain::Heat, UtilityHeatUse::DirectRecipe);
	CHECK(CommitUtilityMutation(proposal, &wrong_domain).error == UtilityPowerError::Refused);
	CHECK(CommitUtilityMutation(proposal, &wrong_role).error == UtilityPowerError::Refused); CHECK(ComputeUtilityPowerDigest() == before);
	UtilityDomainBenefit benefit(UtilityEnergyDomain::Heat, UtilityHeatUse::ElectricalGenerator);
	benefit.accept = false; CHECK(CommitUtilityMutation(proposal, &benefit).error == UtilityPowerError::Refused); CHECK(ComputeUtilityPowerDigest() == before);
	benefit.accept = true; REQUIRE(CommitUtilityMutation(proposal, &benefit).error == UtilityPowerError::None); CHECK(benefit.completed_j == 5000);
	CHECK(CommitUtilityMutation(proposal, &benefit).error == UtilityPowerError::StaleMutation); CHECK(benefit.completed_j == 5000);
	CHECK(GetUtilityPowerProducer(*heat)->consumed_lifetime_j == 5000); CHECK(GetUtilityPowerProducer(*electric)->consumed_lifetime_j == 0);
	CHECK(GetUtilityAllocation(*powered)->granted_energy_j == 10800);
	const auto release = PrepareUtilityHeatMutation(UtilityMutationKind::ReleaseUnused, {*generator, 1}, 5000); REQUIRE(release.error == UtilityPowerError::None);
	REQUIRE(CommitUtilityMutation(release).error == UtilityPowerError::None); CHECK(GetUtilityHeatAllocation(*generator)->granted_energy_j == 0);
	CHECK(GetUtilityHeatAllocation(*direct)->granted_energy_j == 17000);
	CHECK(PrepareUtilityHeatMutation(UtilityMutationKind::TransferLayer, {*direct, 1}, 1).error != UtilityPowerError::None);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	CHECK_FALSE(IsUtilityHeatAllocated()); CHECK_FALSE(GetUtilityHeatAllocation(*direct).has_value());
	CHECK(GetUtilityPowerProducer(*heat)->generated_lifetime_j == 27000); CHECK(GetUtilityPowerProducer(*heat)->consumed_lifetime_j == 5000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Subdivision retains the same source-owned fraction/lifetime energy; every domain closes once before the actual shared system epoch rotates. */
TEST_CASE("Native heat source carry survives subdivided actual owner intervals", "[utility-heat-domain]")
{
	UtilityHeatRig rig; const auto source = RegisterUtilityHeatProducer(_current_company); REQUIRE(source.has_value());
	// Repeated actual ledger epochs use one fixed27-ms tick each, rather than
	// resetting the vent fraction or pretending that a query elapsed more time.
	for (uint interval = 1; interval <= 37; ++interval) {
		REQUIRE(PublishUtilityHeatGeneration(*source, interval, 17, 1) == UtilityPowerError::None);
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityHeat(interval, {}) == UtilityPowerError::None);
		REQUIRE(AllocateUtilityPower(interval, {}) == UtilityPowerError::None);
		REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	CHECK(GetUtilityPowerProducer(*source)->generated_lifetime_j == 16); CHECK(GetUtilityPowerProducer(*source)->meter.remainder_mj == 983);
	CHECK(GetUtilityPowerProducer(*source)->meter.generation_energy_j == 0); CHECK(GetUtilityPowerProducer(*source)->consumed_lifetime_j == 0);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Corrupt domain/role/source/phase/route scalars refuse unpublished restoration wholesale, preserving the actual accepted owner and once-only fractions/grants. */
TEST_CASE("Native heat restored rows cannot reinterpret electrical or thermal authority", "[utility-heat-stage]")
{
	UtilityHeatRig rig; const auto source = RegisterUtilityHeatProducer(_current_company); REQUIRE(source.has_value());
	const auto consumer = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::DirectRecipe, 1000, std::array{*source}); REQUIRE(consumer.has_value());
	REQUIRE(PublishUtilityHeatGeneration(*source, 1, 100, 100) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(AllocateUtilityHeat(1, std::array{UtilityPowerRequest{*consumer, 500}}) == UtilityPowerError::None);
	}
	auto rows = CaptureUtilityPowerState();
	SECTION("source domain") { rows.producers[0].meter.domain = UtilityEnergyDomain::Electricity; }
	SECTION("consumer domain") { rows.consumers[0].meter.domain = UtilityEnergyDomain::Electricity; }
	SECTION("electrical effect in thermal meter") { rows.consumers[0].effect = UtilityConsumerEffect::TubeWork; }
	SECTION("solar thermal source") { rows.producers[0].kind = UtilityGenerationKind::SolarFarm; }
	SECTION("missing thermal phase") { rows.heat_allocated = false; }
	SECTION("thermal electrical export") { rows.consumers[0].shares[0].transferred_j = 1; }
	SECTION("invalid fraction") { rows.producers[0].meter.remainder_mj = 1000; }
	const auto before = ComputeUtilityPowerDigest();
	CHECK(ValidateUtilityPowerState(rows, GetCurrentMultiLayerProfileSelection()) != UtilityPowerError::None);
	StageLoadedUtilityPowerState(rows); CHECK_FALSE(AcceptLoadedUtilityPowerState()); CHECK(ComputeUtilityPowerDigest() == before);
	CHECK(GetUtilityHeatAllocation(*consumer)->granted_energy_j == 270); // The source supplied100W *100ticks *27ms; refusal retains this actual bounded grant rather than the500J request.
}

/** Real company takeover moves the same thermal identities/carry/use; removal releases only unused shares and cannot revive energy through recycled company identity. */
TEST_CASE("Native heat company lifecycle retains exact source and settled use", "[utility-heat-company]")
{
	UtilityHeatRig rig; const auto owner = _current_company;
	const auto source = RegisterUtilityHeatProducer(owner); REQUIRE(source.has_value());
	const auto consumer = RegisterUtilityHeatConsumer(owner, UtilityHeatUse::DirectRecipe, 1000, std::array{*source}); REQUIRE(consumer.has_value());
	REQUIRE(PublishUtilityHeatGeneration(*source, 1, 17, 100) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(AllocateUtilityHeat(1, std::array{UtilityPowerRequest{*consumer, 1000}}) == UtilityPowerError::None);
	}
	UtilityDomainBenefit benefit(UtilityEnergyDomain::Heat, UtilityHeatUse::DirectRecipe);
	const auto use = PrepareUtilityHeatMutation(UtilityMutationKind::Consume, {*consumer, 1}, 20); REQUIRE(use.error == UtilityPowerError::None);
	REQUIRE(CommitUtilityMutation(use, &benefit).error == UtilityPowerError::None);
	REQUIRE(Company::CanAllocateItem()); Company *successor = Company::Create(); const auto new_owner = successor->index;
	ChangeOwnershipOfCompanyItems(owner, new_owner);
	CHECK(GetUtilityPowerProducer(*source)->owner == new_owner); CHECK(GetUtilityPowerConsumer(*consumer)->owner == new_owner);
	CHECK(GetUtilityPowerProducer(*source)->meter.remainder_mj == 900); CHECK(GetUtilityPowerProducer(*source)->consumed_lifetime_j == 20);
	_current_company = new_owner; CHECK(GetUtilityHeatAllocation(*consumer)->granted_energy_j == 25);
	CHECK(CommitUtilityMutation(use, &benefit).error == UtilityPowerError::StaleMutation);
	ChangeOwnershipOfCompanyItems(new_owner, INVALID_OWNER);
	CHECK_FALSE(GetUtilityPowerProducer(*source)->active); CHECK_FALSE(GetUtilityPowerConsumer(*consumer)->active);
	CHECK(GetUtilityPowerProducer(*source)->consumed_lifetime_j == 20); CHECK(GetUtilityPowerProducer(*source)->reserved_j == 0);
	CHECK(GetUtilityPowerConsumer(*consumer)->shares.front().consumed_j == 20); CHECK(GetUtilityPowerConsumer(*consumer)->shares.front().released_j == 25);
	CHECK_FALSE(GetUtilityHeatAllocation(*consumer).has_value());
	_current_company = owner; delete successor;
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}
