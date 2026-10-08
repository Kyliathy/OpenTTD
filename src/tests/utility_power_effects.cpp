/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power_effects.cpp Actual ledger-backed bounded work and real Town/custom-GS authority fixtures.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 9
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-g-content-freeze.md native response caps
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "utility_power_fixture.h"
#include "../core/backup_type.hpp"
#include "../town.h"
#include "../town_cmd.h"
#include "../town_kdtree.h"

#include "../safeguards.h"

/** Small actual public work owner; only its validated complete participant may advance this fixture's physical work/revision. */
class UtilityTestWorkOwner final : public UtilityWorkParticipant {
public:
	UtilityConsumerEffect kind; ///< Actual work family whose response is admitted.
	uint64_t work_ticks = 0; ///< This facility owner's work authority, never another utility balance.
	uint64_t revision = 1; ///< Successful work changes the actual owner revision once.
	bool admit = true, commit = true; ///< Independent whole-owner refusal boundaries.
	/** @param kind Actual declared fixture work family. */
	explicit UtilityTestWorkOwner(UtilityConsumerEffect kind) : kind(kind) {}
	/** @param effect Exact positive bounded response. @return Whether this actual work family/counter still admits the whole action. */
	bool Validate(const UtilityWorkEffect &effect) const override
	{
		// Work requires the exact family and finite actual owner room; a
		// refused/baseline-zero response cannot create progress ahead of energy.
		return this->admit && effect.kind == this->kind && effect.work_economy_ticks > 0 && effect.work_economy_ticks <= 1000000 - this->work_ticks;
	}
	/** @param effect Exact validated response. @return Complete physical work/revision change or unchanged refusal. */
	bool Commit(const UtilityWorkEffect &effect) noexcept override
	{
		if (!this->commit) return false;
		this->work_ticks += effect.work_economy_ticks; ++this->revision;
		return true;
	}
};

/** Actual company/native ledger supplies one finite complete published consumer offer. */
class UtilityEffectRig {
public:
	ExcavationFixture world; ///< Actual native company/calendar/map/content ownership.
	UtilityPowerReset ledger; ///< Reset before the surrounding actual world teardown.
	UtilityProducerID producer{UINT32_MAX}; ///< Owner-issued original generation reference.
	UtilityConsumerID consumer{UINT32_MAX}; ///< Owner-issued bounded service/city reference.
	uint64_t demand = 0; ///< Exact frozen-rate request for this current source interval.
	/** @param effect Actual registered family. @param ticks Requested synchronized work opportunity. @param supply_ticks Actual source opportunity; zero uses the full request. */
	UtilityEffectRig(UtilityConsumerEffect effect, uint32_t ticks = 100, uint32_t supply_ticks = 0)
	{
		const auto contracts = GetUtilityConsumerEffects();
		const auto contract = std::ranges::find(contracts, effect, &UtilityConsumerEffectContract::effect);
		REQUIRE(contract != contracts.end());
		demand = IntegrateUtilityGeneration(contract->demand_rate_w, ticks, 0)->energy_j;
		const auto source = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::External);
		REQUIRE(source.has_value()); producer = *source;
		const auto target = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, effect, demand, std::array{producer});
		REQUIRE(target.has_value()); consumer = *target;
		REQUIRE(PublishUtilityGeneration(producer, 1, contract->demand_rate_w, supply_ticks == 0 ? ticks : supply_ticks) == UtilityPowerError::None);
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(1, std::array{UtilityPowerRequest{consumer, demand}}) == UtilityPowerError::None);
	}
};

/** Actual accepted energy advances one bounded owner response; refusals and exhausted/read-only offers produce no phantom work. */
TEST_CASE("Native utility work effects settle exact energy with bounded actual progress", "[utility-power-effects]")
{
	const auto kind = GENERATE(UtilityConsumerEffect::ProcessorBoost, UtilityConsumerEffect::TubeWork, UtilityConsumerEffect::AssemblyWork);
	UtilityEffectRig rig(kind);
	UtilityTestWorkOwner work(kind);
	const auto before = ComputeUtilityPowerDigest();
	work.admit = false;
	CHECK(ApplyUtilityWorkEffect(rig.consumer, 100, work).error == UtilityPowerError::Refused);
	work.admit = true; work.commit = false;
	CHECK(ApplyUtilityWorkEffect(rig.consumer, 100, work).error == UtilityPowerError::Refused);
	CHECK(ComputeUtilityPowerDigest() == before);
	CHECK(work.work_ticks == 0);
	work.commit = true;
	const auto result = ApplyUtilityWorkEffect(rig.consumer, 100, work);
	REQUIRE(result.error == UtilityPowerError::None);
	CHECK(result.quantity_j == rig.demand);
	CHECK(work.work_ticks == (kind == UtilityConsumerEffect::ProcessorBoost ? 25 : 100));
	CHECK(work.revision == 2);
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0);
	CHECK(CaptureUtilityPowerState().producers.front().consumed_lifetime_j == rig.demand);
	CHECK(ApplyUtilityWorkEffect(rig.consumer, 100, work).error == UtilityPowerError::InsufficientGrant);
	CHECK(ApplyUtilityWorkEffect(UtilityConsumerID{UINT32_MAX}, 100, work).error == UtilityPowerError::InvalidReference);
	CHECK(ApplyUtilityWorkEffect(rig.consumer, 0, work).error == UtilityPowerError::InvalidQuantity);
	CHECK(work.revision == 2);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Partial supply admits only paid whole service ticks or the capped proportional processor benefit. */
TEST_CASE("Partial utility supply bounds work and preserves energy without an admitted benefit", "[utility-power-effects]")
{
	const auto kind = GENERATE(UtilityConsumerEffect::ProcessorBoost, UtilityConsumerEffect::TubeWork, UtilityConsumerEffect::AssemblyWork);
	UtilityEffectRig rig(kind, 100, 45);
	UtilityTestWorkOwner work(kind);
	const uint64_t available = GetUtilityAllocation(rig.consumer)->granted_energy_j;
	REQUIRE(available == rig.demand * 45 / 100);
	const auto result = ApplyUtilityWorkEffect(rig.consumer, 100, work);
	REQUIRE(result.error == UtilityPowerError::None);
	CHECK(result.quantity_j == available);
	CHECK(work.work_ticks == (kind == UtilityConsumerEffect::ProcessorBoost ? 11 : 45));
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0);
	CHECK(CaptureUtilityPowerState().producers.front().consumed_lifetime_j == available);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Rounding a processor offer down to zero benefit leaves its actual source reservation available. */
TEST_CASE("Subthreshold processor offer consumes nothing", "[utility-power-effects]")
{
	UtilityEffectRig rig(UtilityConsumerEffect::ProcessorBoost, 100, 1);
	UtilityTestWorkOwner work(UtilityConsumerEffect::ProcessorBoost);
	const auto before = ComputeUtilityPowerDigest();
	CHECK(ApplyUtilityWorkEffect(rig.consumer, 100, work).error == UtilityPowerError::InsufficientGrant);
	CHECK(ComputeUtilityPowerDigest() == before);
	CHECK(work.work_ticks == 0);
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == rig.demand / 100);
}

/** GS takeover before native settlement expires the offer without publishing consumption or reviving it on relinquishment. */
TEST_CASE("Custom town authority expires an unused native offer", "[utility-power-effects]")
{
	UtilityEffectRig rig(UtilityConsumerEffect::TownGrowth);
	REQUIRE(Town::CanAllocateItem());
	std::unique_ptr<Town> town(Town::Create(TileXYL(10, 10, MapLayer::Surface)));
	RebuildTownKdtree();
	RefreshUtilityTownGrowthRate(town->index);
	const uint16_t baseline = town->growth_rate;
	REQUIRE(BindUtilityTownConsumer(rig.consumer, town->index) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(CmdTownGrowthRate(DoCommandFlag::Execute, town->index, 37).Succeeded());
		CHECK(CaptureUtilityPowerState().producers.front().consumed_lifetime_j == 0);
		CHECK(CaptureUtilityPowerState().producers.front().reserved_j == 0);
		REQUIRE(CmdTownGrowthRate(DoCommandFlag::Execute, town->index, 0).Succeeded());
	}
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0);
	CHECK(town->growth_rate == baseline);
	CHECK(GetUtilityTownPowerBonus(town->index) == 0);
	CHECK(ApplyUtilityTownEffect(rig.consumer).error == UtilityPowerError::Refused);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Real town rate benefits from one metered native offer, yields to actual GS commands and cannot transfer settled benefit to a reused ID. */
TEST_CASE("Native town utility growth is metered once and respects custom GS authority", "[utility-power-effects]")
{
	UtilityEffectRig rig(UtilityConsumerEffect::TownGrowth);
	REQUIRE(Town::CanAllocateItem());
	std::unique_ptr<Town> town(Town::Create(TileXYL(10, 10, MapLayer::Surface)));
	RebuildTownKdtree();
	RefreshUtilityTownGrowthRate(town->index);
	const uint16_t baseline = town->growth_rate;
	REQUIRE(baseline > 1);
	const auto digest = ComputeUtilityPowerDigest();
	CHECK(ApplyUtilityTownEffect(rig.consumer).error == UtilityPowerError::Refused);
	CHECK(ComputeUtilityPowerDigest() == digest);
	REQUIRE(BindUtilityTownConsumer(rig.consumer, town->index, false) == UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == digest);
	REQUIRE(BindUtilityTownConsumer(rig.consumer, town->index) == UtilityPowerError::None);
	const auto other = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::TownGrowth, rig.demand, std::array{rig.producer});
	REQUIRE(other.has_value());
	CHECK(BindUtilityTownConsumer(*other, town->index) == UtilityPowerError::InvalidTopology);
	REQUIRE(ApplyUtilityTownEffect(rig.consumer).error == UtilityPowerError::None);
	CHECK(GetUtilityTownPowerBonus(town->index) == 25);
	CHECK(town->growth_rate == std::max<uint>(1, static_cast<uint>(baseline) * 100 / 125));
	CHECK_FALSE(town->flags.Test(TownFlag::CustomGrowth));
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0);
	CHECK(CaptureUtilityPowerState().producers.front().consumed_lifetime_j == rig.demand);
	CHECK(ApplyUtilityTownEffect(rig.consumer).error == UtilityPowerError::Refused);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(CmdTownGrowthRate(DoCommandFlag::Execute, town->index, 37).Succeeded());
		CHECK(town->growth_rate == 37);
		CHECK(GetUtilityTownPowerBonus(town->index) == 0);
		REQUIRE(CmdTownGrowthRate(DoCommandFlag::Execute, town->index, 0).Succeeded());
	}
	CHECK_FALSE(town->flags.Test(TownFlag::CustomGrowth));
	CHECK(GetUtilityTownPowerBonus(town->index) == 0);
	CHECK(town->growth_rate == baseline);
	CHECK(ApplyUtilityTownEffect(rig.consumer).error == UtilityPowerError::Refused);
	CHECK(CaptureUtilityPowerState().producers.front().consumed_lifetime_j == rig.demand);
	const TownID old = town->index;
	town.reset();
	CHECK(CaptureUtilityPowerState().consumers.front().town == TownID::Invalid());
	REQUIRE(Town::CanAllocateItem());
	std::unique_ptr<Town> replacement(Town::Create(TileXYL(11, 10, MapLayer::Surface)));
	RebuildTownKdtree();
	CHECK(replacement->index == old);
	CHECK(GetUtilityTownPowerBonus(replacement->index) == 0);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}
