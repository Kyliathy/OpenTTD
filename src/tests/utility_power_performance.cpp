/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power_performance.cpp Bounded actual electrical-owner workload, separate from rendering and network profiling.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-integrated-acceptance-and-balance.md Group21B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md Group15B
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "utility_power_fixture.h"
#include "../core/backup_type.hpp"
#include <chrono>

#include "../safeguards.h"

/**
 * Measure one complete real operation without adding simulation steps or synthetic expected output.
 * @param operation Actual owner operation, including its required correctness check.
 * @return Elapsed steady-clock nanoseconds; these measure performance, never gameplay time.
 */
static int64_t MeasureUtilityOperation(const std::function<void()> &operation)
{
	const auto begin = std::chrono::steady_clock::now();
	operation();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin).count();
}

/**
 * Estimate retained owned transport payload separately from allocator/RSS overhead.
 * @param state Actual captured ledger with real vector capacities and saved shares.
 * @return Bytes of scalar records and vector storage; excludes process/allocator/temporary peak memory.
 */
static size_t UtilityPayloadBytes(const UtilityPowerState &state)
{
	size_t bytes = sizeof(state) + state.producers.capacity() * sizeof(UtilityPowerProducer) +
		state.consumers.capacity() * sizeof(UtilityPowerConsumer) + state.links.capacity() * sizeof(UtilityPowerLink);
	// Count each consumer's real eligibility/lineage storage once. This is a
	// payload estimate, not a second resource balance or a claimed RSS profile.
	for (const auto &consumer : state.consumers) {
		bytes += consumer.linked_producers.capacity() * sizeof(UtilityProducerID);
		bytes += consumer.shares.capacity() * sizeof(UtilityPowerShare);
	}
	return bytes;
}

/** Actual bounded sparse account workloads measure allocation and real company retirement while retaining exact original energy. */
TEST_CASE("Electrical owner exposes bounded allocation and retirement workload measurements", "[utility-power-performance]")
{
	const uint count = GENERATE(0U, 32U, 256U, MAX_UTILITY_INTERVAL_ACCOUNTS);
	ExcavationFixture world;
	UtilityPowerReset utility;
	const auto owner = _current_company;
	std::vector<UtilityProducerID> producers;
	std::vector<UtilityPowerRequest> requests;
	producers.reserve(count); requests.reserve(count);
	// One distinct real source per consumer bounds this sparse topology at
	// exactly count edges; neither dummy account IDs nor all-pairs grants are used.
	/** Build the bounded sparse public ledger without synthetic IDs. @return No value; registrations populate producers and requests. */
	const auto setup = [&] {
		for (uint index = 0; index < count; ++index) {
			const auto source = RegisterUtilityPowerProducer(owner, MapLayer::Surface, UtilityGenerationKind::External);
			REQUIRE(source.has_value());
			const std::array eligibility{*source};
			const auto consumer = RegisterUtilityPowerConsumer(owner, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 27, eligibility);
			REQUIRE(consumer.has_value());
			producers.push_back(*source);
			requests.push_back({*consumer, 27});
		}
	};
	const auto setup_ns = MeasureUtilityOperation(setup);
	std::array<int64_t, 5> samples{};
	// Warm one full interval first, then measure five equal owner workloads.
	// Epoch rotation/publishing happen outside the measured allocator scope;
	// rendered cameras and wall-clock durations never renew reservations.
	for (uint sample = 0; sample < samples.size() + 1; ++sample) {
		const auto epoch = CaptureUtilityPowerState().interval;
		for (const auto source : producers) REQUIRE(PublishUtilityGeneration(source, epoch, 1, 1000) == UtilityPowerError::None);
		int64_t elapsed = 0;
		{
			AutoRestoreBackup system(_current_company, OWNER_DEITY);
			/** Allocate the current epoch's real requests once. @return No value; the owning allocator validates the operation. */
			const auto allocate = [&] { REQUIRE(AllocateUtilityPower(epoch, requests) == UtilityPowerError::None); };
			elapsed = MeasureUtilityOperation(allocate);
		}
		if (sample != 0) samples[sample - 1] = elapsed;
		// Each physical request sees its own27J original slice. Queries cannot
		// change state; retirement later expires these exact unused amounts.
		for (const auto &request : requests) REQUIRE(GetUtilityAllocation(request.consumer)->granted_energy_j == 27);
		if (sample != samples.size()) {
			AutoRestoreBackup system(_current_company, OWNER_DEITY);
			REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
		}
	}
	const auto before = CaptureUtilityPowerState();
	const auto bytes = UtilityPayloadBytes(before);
	/** Retire this actual company's accounts and expire unused reservations. @return No value; the company lifecycle owner applies retirement. */
	const auto retire = [&] { ChangeUtilityPowerCompanyOwnership(owner, INVALID_OWNER); };
	const auto retirement_ns = MeasureUtilityOperation(retire);
	const auto after = CaptureUtilityPowerState();
	REQUIRE(ValidateUtilityPowerState(after, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	CHECK(after.next_producer == count);
	CHECK(after.next_consumer == count);
	// Retained identities/lifetime publication remain authoritative; all live
	// reservations expire exactly once instead of being refunded or reused.
	for (const auto &source : after.producers) { CHECK_FALSE(source.active); CHECK(source.reserved_j == 0); }
	for (const auto &consumer : after.consumers) { CHECK_FALSE(consumer.active); CHECK(consumer.shares[0].released_j == 27); }
	std::ranges::sort(samples);
	fmt::print("UTILITY_PERFORMANCE accounts={} edges={} warmup=1 samples=5 setup_ns={} allocation_p50_ns={} allocation_max_ns={} retirement_ns={} payload_bytes={} peak_rss_measured=false rendering=false\n",
		count, count, setup_ns, samples[2], samples.back(), retirement_ns, bytes);
}
