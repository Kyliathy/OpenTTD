/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power_mutation.cpp Focused whole-participant consumption/release and persisted retry owner checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 8
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-v-utility-settlement-contract.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../utility_power.h"
#include "../command_type.h"
#include "../core/backup_type.hpp"
#include "utility_power_fixture.h"

#include "../safeguards.h"

/** Real native accounts, two once-published origins and one oversubscribed consumer, cleared before actual company teardown. */
class UtilityMutationRig {
public:
	ExcavationFixture world; ///< Real activated native profile, calendar, company and explicit world layers.
	UtilityProducerID first{UINT32_MAX}, second{UINT32_MAX}; ///< Ledger-issued generation IDs.
	UtilityConsumerID consumer{UINT32_MAX}; ///< Ledger-issued actual same-owner consumer.
	/** Register/publish/allocate through actual owners; no copied grant or synthetic source state is injected. */
	UtilityMutationRig()
	{
		ResetUtilityPowerState();
		const auto a = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::External);
		const auto b = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::Coal);
		REQUIRE(a.has_value()); REQUIRE(b.has_value()); first = *a; second = *b;
		const auto c = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 100, std::array{first, second});
		REQUIRE(c.has_value()); consumer = *c;
		REQUIRE(PublishUtilityGeneration(first, 1, 2, 1000) == UtilityPowerError::None);
		REQUIRE(PublishUtilityGeneration(second, 1, 1, 1000) == UtilityPowerError::None);
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(1, std::array{UtilityPowerRequest{consumer, 100}}) == UtilityPowerError::None);
	}
	/** Release this fixture's ledger before the actual native company/world owners are destroyed. */
	~UtilityMutationRig() { ResetUtilityPowerState(); }
};

/** Complete participant failure leaves actual money, source slices and operation sequence unchanged; successful use settles exactly once. */
TEST_CASE("Native utility consumption revalidates the whole actual participant and saved retry identity", "[utility-power-mutation]")
{
	UtilityMutationRig rig;
	UtilityCompanyWork work(_current_company);
	const auto ticket = PrepareUtilityMutation(UtilityMutationKind::Consume, {rig.consumer, 1}, 60);
	REQUIRE(ticket.error == UtilityPowerError::None);
	const auto digest = ComputeUtilityPowerDigest();
	const Money money = Company::Get(_current_company)->money;
	CHECK(CommitUtilityMutation(ticket).error == UtilityPowerError::Refused);
	work.admit = false;
	CHECK(CommitUtilityMutation(ticket, &work).error == UtilityPowerError::Refused);
	work.admit = true; work.commit = false;
	CHECK(CommitUtilityMutation(ticket, &work).error == UtilityPowerError::Refused);
	CHECK(ComputeUtilityPowerDigest() == digest);
	CHECK(Company::Get(_current_company)->money == money);
	auto changed = ticket; ++changed.quantity_j;
	CHECK(CommitUtilityMutation(changed, &work).error == UtilityPowerError::StaleMutation);
	work.commit = true;
	const auto receipt = CommitUtilityMutation(ticket, &work);
	REQUIRE(receipt.error == UtilityPowerError::None);
	CHECK(receipt.quantity_j == 60);
	CHECK(receipt.operation_sequence == ticket.sequence_before + 1);
	CHECK(Company::Get(_current_company)->money == money - Money{5});
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 21);
	const auto state = CaptureUtilityPowerState();
	CHECK(state.producers[0].consumed_lifetime_j == 54);
	CHECK(state.producers[1].consumed_lifetime_j == 6);
	CHECK(state.producers[0].reserved_j == 0);
	CHECK(state.producers[1].reserved_j == 21);
	auto forged_history = state;
	++forged_history.producers[1].consumed_lifetime_j;
	CHECK(ValidateUtilityPowerState(forged_history, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::InvalidQuantity);
	CHECK(CommitUtilityMutation(ticket, &work).error == UtilityPowerError::StaleMutation);
	ResetUtilityPowerState(); StageLoadedUtilityPowerState(state);
	REQUIRE(AcceptLoadedUtilityPowerState());
	CHECK(CommitUtilityMutation(ticket, &work).error == UtilityPowerError::StaleMutation);
	CHECK(Company::Get(_current_company)->money == money - Money{5});
	REQUIRE(RemoveUtilityPowerProducer(rig.first) == UtilityPowerError::None);
	REQUIRE(RemoveUtilityPowerProducer(rig.second) == UtilityPowerError::None);
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0);
	CHECK(GetUtilityAllocation(rig.consumer)->unmet_energy_j == 40);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	CHECK(CaptureUtilityPowerState().producers[0].consumed_lifetime_j == 54);
	CHECK(CaptureUtilityPowerState().producers[1].consumed_lifetime_j == 6);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Unused release expires exactly its positive free quantity, preserves real money/use and cannot be replayed or transferred implicitly. */
TEST_CASE("Native unused utility release preserves original use and refuses stale or excessive actions", "[utility-power-mutation]")
{
	UtilityMutationRig rig;
	const auto digest = ComputeUtilityPowerDigest();
	const Money money = Company::Get(_current_company)->money;
	CHECK(PrepareUtilityMutation(UtilityMutationKind::Consume, {rig.consumer, 1}, 82).error == UtilityPowerError::InsufficientGrant);
	CHECK(PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {rig.consumer, 1}, 0).error == UtilityPowerError::InvalidQuantity);
	CHECK(PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {rig.consumer, 2}, 1).error == UtilityPowerError::InvalidInterval);
	CHECK(PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {rig.consumer, 1}, 1).error == UtilityPowerError::InvalidTopology);
	const auto ticket = PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {rig.consumer, 1}, 10);
	REQUIRE(ticket.error == UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == digest);
	REQUIRE(CommitUtilityMutation(ticket).error == UtilityPowerError::None);
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 71);
	CHECK(GetUtilityAllocation(rig.consumer)->unmet_energy_j == 29);
	CHECK(CaptureUtilityPowerState().consumers.front().shares.front().released_j == 10);
	CHECK(CaptureUtilityPowerState().producers.front().consumed_lifetime_j == 0);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(CommitUtilityMutation(ticket).error == UtilityPowerError::StaleMutation);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}
