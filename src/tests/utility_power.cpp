/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power.cpp Focused real-profile/account tests for the native electrical publication/allocation owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group7
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint9.1
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../utility_power.h"
#include "../core/backup_type.hpp"
#include "../multilayer_state.h"

#include "../safeguards.h"

/** Native owner fixture resets its own ledger before actual company/profile teardown. */
class UtilityPowerRig {
public:
	ExcavationFixture world; ///< Existing real profile/calendar/company and four allocated layers.
	/** Begin an isolated native ledger without synthetic company/producer identifiers. */
	UtilityPowerRig() { ResetUtilityPowerState(); }
	/** Clear only this fixture's utility records while the real company still exists. */
	~UtilityPowerRig() { ResetUtilityPowerState(); }
};

/** Once-only checked publication preserves zero supply, fractional carry, estimates and refusal purity. */
TEST_CASE("Native electrical publication is bounded once per source and epoch", "[utility-power-ledger]")
{
	UtilityPowerRig rig;
	const CompanyID owner = _current_company;
	const auto estimate = RegisterUtilityPowerProducer(owner, MapLayer::Surface, UtilityGenerationKind::External, false);
	REQUIRE(estimate.has_value());
	CHECK(CaptureUtilityPowerState().producers.empty());
	CHECK(CaptureUtilityPowerState().next_producer == 0);
	const auto source = RegisterUtilityPowerProducer(owner, MapLayer::Surface, UtilityGenerationKind::External);
	REQUIRE(source == estimate);
	CHECK(PublishUtilityGeneration(*source, 2, 1, 1) == UtilityPowerError::InvalidInterval);
	CHECK(PublishUtilityGeneration(*source, 1, MAX_UTILITY_RATE_W + 1, 1) == UtilityPowerError::InvalidQuantity);
	CHECK(PublishUtilityGeneration(*source, 1, 1, 1, false) == UtilityPowerError::None);
	CHECK(CaptureUtilityPowerState().producers.front().meter.remainder_mj == 0);
	REQUIRE(PublishUtilityGeneration(*source, 1, 1, 1) == UtilityPowerError::None);
	CHECK(CaptureUtilityPowerState().producers.front().meter.generation_energy_j == 0);
	CHECK(CaptureUtilityPowerState().producers.front().meter.remainder_mj == 27);
	CHECK(PublishUtilityGeneration(*source, 1, 1, 1) == UtilityPowerError::DuplicatePublication);
	CHECK(CaptureUtilityPowerState().producers.front().meter.remainder_mj == 27);
	const auto zero = RegisterUtilityPowerProducer(owner, MapLayer::Surface, UtilityGenerationKind::Coal);
	REQUIRE(zero.has_value());
	REQUIRE(PublishUtilityGeneration(*zero, 1, 0, 100) == UtilityPowerError::None);
	CHECK(PublishUtilityGeneration(*zero, 1, 0, 100) == UtilityPowerError::DuplicatePublication);
	CHECK_FALSE(RegisterUtilityPowerProducer(owner, MapLayer::Invalid, UtilityGenerationKind::External).has_value());
}

/** The actual mutable ledger reuses frozen stable ordering and keeps reservations separate from paid/settled consumption. */
TEST_CASE("Native electrical grants reserve only explicit supply in stable consumer order", "[utility-power-ledger]")
{
	UtilityPowerRig rig;
	const auto first = RegisterUtilityPowerProducer(_current_company, MapLayer::Underground, UtilityGenerationKind::External);
	const auto second = RegisterUtilityPowerProducer(_current_company, MapLayer::Underground, UtilityGenerationKind::Coal);
	REQUIRE(first.has_value());
	REQUIRE(second.has_value());
	const std::array sources{*second, *first};
	const auto early = RegisterUtilityPowerConsumer(_current_company, MapLayer::Underground, UtilityConsumerEffect::TubeWork, 900, sources);
	const auto late = RegisterUtilityPowerConsumer(_current_company, MapLayer::Underground, UtilityConsumerEffect::ProcessorBoost, 900, sources);
	REQUIRE(early.has_value());
	REQUIRE(late.has_value());
	const std::array duplicate{*first, *first};
	CHECK_FALSE(RegisterUtilityPowerConsumer(_current_company, MapLayer::Underground, UtilityConsumerEffect::TubeWork, 900, duplicate).has_value());
	CHECK_FALSE(RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::TubeWork, 900, sources).has_value());
	REQUIRE(PublishUtilityGeneration(*first, 1, 20, 1000) == UtilityPowerError::None);
	REQUIRE(PublishUtilityGeneration(*second, 1, 10, 1000) == UtilityPowerError::None);
	const std::array requests{UtilityPowerRequest{*late, 900}, UtilityPowerRequest{*early, 900}};
	CHECK(AllocateUtilityPower(1, requests) == UtilityPowerError::InvalidOwner);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		const std::array excessive{UtilityPowerRequest{*early, 901}};
		CHECK(AllocateUtilityPower(1, excessive) == UtilityPowerError::InvalidQuantity);
		CHECK_FALSE(CaptureUtilityPowerState().allocated);
		CHECK(AllocateUtilityPower(1, requests, false) == UtilityPowerError::None);
		CHECK_FALSE(CaptureUtilityPowerState().allocated);
		REQUIRE(AllocateUtilityPower(1, requests) == UtilityPowerError::None);
		CHECK(AllocateUtilityPower(1, requests) == UtilityPowerError::AlreadyAllocated);
	}
	const auto before = CaptureUtilityPowerState();
	const auto money = Company::Get(_current_company)->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	// Repeated live queries cannot renew a reservation, generate power, advance
	// work or pay freight; they expose the one already allocated interval.
	for (uint paint = 0; paint < 20; ++paint) {
		const auto grant = GetUtilityAllocation(*early);
		REQUIRE(grant.has_value());
		CHECK(grant->granted_energy_j == 810);
		CHECK(grant->unmet_energy_j == 90);
		CHECK(GetUtilityAllocation(*late)->granted_energy_j == 0);
	}
	CHECK(CaptureUtilityPowerState().producers.front().reserved_j == 540);
	CHECK(CaptureUtilityPowerState().producers.back().reserved_j == 270);
	CHECK(CaptureUtilityPowerState().next_consumer == before.next_consumer);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
}

/** One system allocation serves multiple companies without allowing either to borrow the other's explicit source. */
TEST_CASE("Native electrical system epoch allocates every company together", "[utility-power-ledger]")
{
	UtilityPowerRig rig;
	const CompanyID owner = _current_company;
	const auto source = RegisterUtilityPowerProducer(owner, MapLayer::Surface, UtilityGenerationKind::External);
	REQUIRE(source.has_value());
	const std::array owned{*source};
	const auto consumer = RegisterUtilityPowerConsumer(owner, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 100, owned);
	REQUIRE(consumer.has_value());
	REQUIRE(PublishUtilityGeneration(*source, 1, 10, 1000) == UtilityPowerError::None);
	REQUIRE(Company::CanAllocateItem());
	std::unique_ptr<Company> peer(Company::Create());
	std::optional<UtilityConsumerID> peer_consumer;
	{
		AutoRestoreBackup actor(_current_company, peer->index);
		CHECK_FALSE(RegisterUtilityPowerConsumer(peer->index, MapLayer::Surface, UtilityConsumerEffect::TownGrowth, 100, owned).has_value());
		const auto peer_source = RegisterUtilityPowerProducer(peer->index, MapLayer::Surface, UtilityGenerationKind::External);
		REQUIRE(peer_source.has_value());
		const std::array peer_links{*peer_source};
		peer_consumer = RegisterUtilityPowerConsumer(peer->index, MapLayer::Surface, UtilityConsumerEffect::TownGrowth, 100, peer_links);
		REQUIRE(peer_consumer.has_value());
		REQUIRE(PublishUtilityGeneration(*peer_source, 1, 1, 1000) == UtilityPowerError::None);
		CHECK(PublishUtilityGeneration(*source, 1, 10, 1000) == UtilityPowerError::InvalidOwner);
	}
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		const std::array requests{UtilityPowerRequest{*peer_consumer, 100}, UtilityPowerRequest{*consumer, 100}};
		REQUIRE(AllocateUtilityPower(1, requests) == UtilityPowerError::None);
	}
	CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == 100);
	CHECK_FALSE(GetUtilityAllocation(*peer_consumer).has_value());
	{
		AutoRestoreBackup actor(_current_company, peer->index);
		CHECK(GetUtilityAllocation(*peer_consumer)->granted_energy_j == 27);
		CHECK_FALSE(GetUtilityAllocation(*consumer).has_value());
	}
}

/** Millijoule fractions survive real owner-issued epochs while duplicate publication and reads cannot advance them. */
TEST_CASE("Native electrical epoch rotation preserves only generation carry", "[utility-power-epoch]")
{
	UtilityPowerRig rig;
	const auto source = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::External);
	REQUIRE(source.has_value());
	const std::array links{*source};
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 10, links);
	REQUIRE(consumer.has_value());
	CHECK(BeginUtilityPowerInterval() == UtilityPowerError::InvalidOwner);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		CHECK(BeginUtilityPowerInterval() == UtilityPowerError::AllocationRequired);
	}
	// Thirty-eight single-tick publications at one watt total exactly 1.026 J.
	// Each interval is truly closed/rotated by the system, with no rate rounding
	// or consumer reservation carried across the epoch boundary.
	for (uint64_t epoch = 1; epoch <= 38; ++epoch) {
		REQUIRE(PublishUtilityGeneration(*source, epoch, 1, 1) == UtilityPowerError::None);
		const std::array requests{UtilityPowerRequest{*consumer, 10}};
		{
			AutoRestoreBackup system(_current_company, OWNER_DEITY);
			REQUIRE(AllocateUtilityPower(epoch, requests) == UtilityPowerError::None);
		}
		CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == (epoch == 38 ? 1 : 0));
		{
			AutoRestoreBackup system(_current_company, OWNER_DEITY);
			REQUIRE(BeginUtilityPowerInterval(false) == UtilityPowerError::None);
			CHECK(CaptureUtilityPowerState().interval == epoch);
			REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
			CHECK(BeginUtilityPowerInterval() == UtilityPowerError::AllocationRequired);
		}
		CHECK_FALSE(GetUtilityAllocation(*consumer).has_value());
	}
	CHECK(CaptureUtilityPowerState().producers.front().meter.remainder_mj == 26);
	CHECK(CaptureUtilityPowerState().producers.front().reserved_j == 0);
	CHECK(CaptureUtilityPowerState().producers.front().meter.generation_energy_j == 0);
	CHECK(CaptureUtilityPowerState().consumers.front().shares.empty());
}

/** Source retirement cancels only that original source's unspent slices and permanently retires its identity. */
TEST_CASE("Native electrical topology loss preserves grants from surviving sources", "[utility-power-epoch]")
{
	UtilityPowerRig rig;
	const auto lost = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::External);
	const auto surviving = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::Coal);
	REQUIRE(lost.has_value());
	REQUIRE(surviving.has_value());
	const std::array links{*surviving, *lost};
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 100, links);
	REQUIRE(consumer.has_value());
	REQUIRE(PublishUtilityGeneration(*lost, 1, 2, 1000) == UtilityPowerError::None);
	REQUIRE(PublishUtilityGeneration(*surviving, 1, 1, 1000) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		const std::array requests{UtilityPowerRequest{*consumer, 100}};
		REQUIRE(AllocateUtilityPower(1, requests) == UtilityPowerError::None);
	}
	CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == 81);
	REQUIRE(RemoveUtilityPowerProducer(*lost, false) == UtilityPowerError::None);
	CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == 81);
	REQUIRE(RemoveUtilityPowerProducer(*lost) == UtilityPowerError::None);
	CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == 27);
	CHECK(GetUtilityAllocation(*consumer)->unmet_energy_j == 73);
	const auto after = CaptureUtilityPowerState();
	CHECK_FALSE(after.producers.front().active);
	CHECK(after.producers.front().reserved_j == 0);
	CHECK(after.producers.back().reserved_j == 27);
	CHECK(after.consumers.front().shares.front().released_j == 54);
	CHECK(after.consumers.front().shares.back().released_j == 0);
	CHECK(RemoveUtilityPowerProducer(*lost) == UtilityPowerError::InvalidReference);
	CHECK(PublishUtilityGeneration(*lost, 1, 100, 1000) == UtilityPowerError::InvalidReference);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	const auto replacement = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::External);
	REQUIRE(replacement.has_value());
	CHECK(replacement->base() > surviving->base());
	CHECK(replacement != lost);
	CHECK_FALSE(RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 100, std::array{*lost}).has_value());
}

/** Unpublished transport must revalidate identity, exact source balances and fractional arithmetic before replacing the live owner. */
TEST_CASE("Staged native utility authority refuses corrupt balances and preserves retry state", "[utility-power-stage]")
{
	UtilityPowerRig rig;
	const auto source = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::External);
	REQUIRE(source.has_value());
	const std::array links{*source};
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 100, links);
	REQUIRE(consumer.has_value());
	REQUIRE(PublishUtilityGeneration(*source, 1, 3, 1000) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		const std::array requests{UtilityPowerRequest{*consumer, 100}};
		REQUIRE(AllocateUtilityPower(1, requests) == UtilityPowerError::None);
	}
	const auto saved = CaptureUtilityPowerState();
	REQUIRE(ValidateUtilityPowerState(saved, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	CHECK(HasUtilityPowerState());
	// Corruption cases edit only unpublished copies. No validator may repair
	// a missing ID, fabricated joule, wrong domain or over-promised source.
	for (uint fault = 0; fault < 8; ++fault) {
		auto bad = saved;
		switch (fault) {
			case 0: ++bad.producers.front().reserved_j; break;
			case 1: bad.producers.front().meter.remainder_mj = 1000; break;
			case 2: bad.consumers.front().linked_producers = {UtilityProducerID{UINT32_MAX}}; break;
			case 3: bad.producers.front().meter.domain = UtilityEnergyDomain::Heat; break;
			case 4: bad.producers.front().meter.generation_rate_w = 0; break;
			case 5: bad.producers.front().owner = CompanyID{255}; break;
			case 6: ++bad.next_producer; break;
			case 7: bad.producers.front().meter.accounting_interval = 0; break;
		}
		CHECK(ValidateUtilityPowerState(bad, GetCurrentMultiLayerProfileSelection()) != UtilityPowerError::None);
		StageLoadedUtilityPowerState(std::move(bad));
		CHECK_FALSE(AcceptLoadedUtilityPowerState());
		CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == 81);
	}
	ResetUtilityPowerState();
	CHECK_FALSE(HasUtilityPowerState());
	StageLoadedUtilityPowerState(saved);
	REQUIRE(AcceptLoadedUtilityPowerState());
	CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == 81);
	CHECK(CaptureUtilityPowerState().next_producer == saved.next_producer);
	CHECK(PublishUtilityGeneration(*source, 1, 3, 1000) == UtilityPowerError::AlreadyAllocated);
	// Registration during a closed allocation phase waits for the next epoch
	// to receive supply; building a new facility must not be blocked all month.
	const auto newcomer = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::External);
	REQUIRE(newcomer.has_value());
	CHECK(PublishUtilityGeneration(*newcomer, 1, 3, 1000) == UtilityPowerError::AlreadyAllocated);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	const auto omitted = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 100, links);
	REQUIRE(omitted.has_value());
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
		const std::array requests{UtilityPowerRequest{*consumer, 100}};
		REQUIRE(AllocateUtilityPower(2, requests) == UtilityPowerError::None);
	}
	// Omitted requests must expose a current zero allocation, including a
	// consumer registered during the previous closed phase; no stale lease hides.
	REQUIRE(GetUtilityAllocation(*omitted).has_value());
	CHECK(GetUtilityAllocation(*omitted)->accounting_interval == 2);
	CHECK(GetUtilityAllocation(*omitted)->granted_energy_j == 0);
}
