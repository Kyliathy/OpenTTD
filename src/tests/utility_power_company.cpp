/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power_company.cpp Real company ownership events preserve original electrical settlement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Group15B
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-utility-lifecycle-policy.md company rows
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "utility_power_fixture.h"
#include "../economy_func.h"
#include "../core/backup_type.hpp"

#include "../safeguards.h"

/**
 * Register and allocate one real company's54J local publication.
 * @param owner Actual fixture company, never a synthetic owner ID.
 * @return Ledger-issued consumer with its actual current reservation.
 */
static UtilityConsumerID AllocateCompanyPower(CompanyID owner)
{
	const auto producer = RegisterUtilityPowerProducer(owner, MapLayer::Surface, UtilityGenerationKind::External);
	REQUIRE(producer.has_value());
	const std::array sources{*producer};
	const auto consumer = RegisterUtilityPowerConsumer(owner, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 54, sources);
	REQUIRE(consumer.has_value());
	REQUIRE(PublishUtilityGeneration(*producer, 1, 2, 1000) == UtilityPowerError::None);
	AutoRestoreBackup system(_current_company, OWNER_DEITY);
	const std::array requests{UtilityPowerRequest{*consumer, 54}};
	REQUIRE(AllocateUtilityPower(1, requests) == UtilityPowerError::None);
	return *consumer;
}

/** Actual takeover transfers both sides of original lineage and invalidates an old owner's prepared ticket. */
TEST_CASE("Actual company takeover retains utility settlement and transfers accounts", "[utility-power-company]")
{
	ExcavationFixture world;
	UtilityPowerReset utility;
	AutoRestoreBackup local(_local_company, CompanyID::Invalid());
	const auto old_owner = _current_company;
	const auto consumer = AllocateCompanyPower(old_owner);
	UtilityCompanyWork work(old_owner);
	REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::Consume, {consumer, 1}, 10), &work).error == UtilityPowerError::None);
	const auto ticket = PrepareUtilityMutation(UtilityMutationKind::Consume, {consumer, 1}, 1);
	REQUIRE(ticket.error == UtilityPowerError::None);
	REQUIRE(Company::CanAllocateItem());
	std::unique_ptr<Company> buyer{Company::Create()};
	const auto before = CaptureUtilityPowerState();
	ChangeOwnershipOfCompanyItems(old_owner, buyer->index);
	AutoRestoreBackup actor(_current_company, buyer->index);
	const auto after = CaptureUtilityPowerState();
	CHECK(after.producers[0].owner == buyer->index);
	CHECK(after.consumers[0].owner == buyer->index);
	CHECK(after.consumers[0].active);
	CHECK(after.producers[0].consumed_lifetime_j == 10);
	CHECK(after.producers[0].generated_lifetime_j == before.producers[0].generated_lifetime_j);
	CHECK(after.consumers[0].shares[0].consumed_j == 10);
	CHECK(GetUtilityAllocation(consumer)->granted_energy_j == 44);
	CHECK(CommitUtilityMutation(ticket, &work).error == UtilityPowerError::StaleMutation);
	CHECK(ValidateUtilityPowerState(after, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Actual removal, pool-ID reuse and reload retain only retired history, with no old grant accessible to the new company. */
TEST_CASE("Actual company removal cannot revive utility accounts through pool identity reuse", "[utility-power-company]")
{
	ExcavationFixture world;
	UtilityPowerReset utility;
	AutoRestoreBackup local(_local_company, CompanyID::Invalid());
	const auto old_owner = _current_company;
	const auto consumer = AllocateCompanyPower(old_owner);
	UtilityCompanyWork work(old_owner);
	REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::Consume, {consumer, 1}, 10), &work).error == UtilityPowerError::None);
	const auto ticket = PrepareUtilityMutation(UtilityMutationKind::Consume, {consumer, 1}, 1);
	ChangeOwnershipOfCompanyItems(old_owner, INVALID_OWNER);
	world.RemoveCompany();
	const auto retired = CaptureUtilityPowerState();
	CHECK_FALSE(retired.producers[0].active);
	CHECK_FALSE(retired.consumers[0].active);
	CHECK(retired.producers[0].reserved_j == 0);
	CHECK(retired.producers[0].consumed_lifetime_j == 10);
	CHECK(retired.consumers[0].shares[0].released_j == 44);
	CHECK(ValidateUtilityPowerState(retired, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	auto corrupt = retired;
	corrupt.consumers[0].linked_producers.push_back(UtilityProducerID{0});
	CHECK(ValidateUtilityPowerState(corrupt, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::InvalidReference);
	corrupt = retired;
	corrupt.consumers[0].shares[0].released_j = 0;
	CHECK(ValidateUtilityPowerState(corrupt, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::InvalidQuantity);
	REQUIRE(Company::CanAllocateItem());
	std::unique_ptr<Company> replacement{Company::Create()};
	REQUIRE(replacement->index == old_owner);
	AutoRestoreBackup actor(_current_company, replacement->index);
	CHECK_FALSE(GetUtilityAllocation(consumer).has_value());
	CHECK(PublishUtilityGeneration(UtilityProducerID{0}, 1, 2, 1000) == UtilityPowerError::InvalidReference);
	CHECK(PrepareUtilityMutation(UtilityMutationKind::Consume, {consumer, 1}, 1).error == UtilityPowerError::InvalidReference);
	CHECK(CommitUtilityMutation(ticket, &work).error == UtilityPowerError::StaleMutation);
	ResetUtilityPowerState();
	StageLoadedUtilityPowerState(retired);
	REQUIRE(AcceptLoadedUtilityPowerState());
	CHECK_FALSE(GetUtilityAllocation(consumer).has_value());
	CHECK(RegisterUtilityPowerProducer(replacement->index, MapLayer::Surface, UtilityGenerationKind::External) == UtilityProducerID{1});
	const std::array sources{UtilityProducerID{1}};
	CHECK(RegisterUtilityPowerConsumer(replacement->index, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 54, sources) == UtilityConsumerID{1});
	REQUIRE(Company::CanAllocateItem());
	std::unique_ptr<Company> buyer{Company::Create()};
	ChangeOwnershipOfCompanyItems(replacement->index, buyer->index);
	const auto taken = CaptureUtilityPowerState();
	CHECK(taken.producers[0].owner == old_owner);
	CHECK(taken.consumers[0].owner == old_owner);
	CHECK_FALSE(taken.consumers[0].active);
	CHECK(taken.producers[1].owner == buyer->index);
	CHECK(taken.consumers[1].owner == buyer->index);
	CHECK(ValidateUtilityPowerState(taken, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}
