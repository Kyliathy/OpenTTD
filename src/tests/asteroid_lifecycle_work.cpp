/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_lifecycle_work.cpp Native lifecycle operation/storage bounds at exact population fixtures.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group33.
 * These are isolated native field/index components, not paid procurement or
 * whole-game FPS measurements. Geometry, historical prefix and explicit due
 * checkpoints are declared separately from actual native event/timer calls.
 */
#include "../stdafx.h"
#include "asteroid_work_fixture.h"
#include "probe_mining_fixture.h"
#include "../asteroid_cargo.h"
#include "../asteroid_lifecycle.h"
#include "../space_mining_inventory.h"
#include "../cargopacket.h"
#include <memory>
#include "../safeguards.h"

/** @param usage Actual read-only native container occupancy. @return Container counts/capacities, explicitly excluding allocator-specific heap overhead. */
static nlohmann::json LifecycleStorageRecord(const AsteroidStorageUsage &usage)
{
	return {{"live_bodies", usage.bodies}, {"body_capacity", usage.body_capacity}, {"lookup_rows", usage.lookup_rows}, {"lookup_buckets", usage.lookup_buckets},
		{"knowledge_rows", usage.knowledge}, {"knowledge_capacity", usage.knowledge_capacity}, {"index_rows", usage.index.membership_rows},
		{"index_hash_buckets", usage.index.membership_buckets}, {"geometric_bucket_headers", usage.index.bucket_headers},
		{"index_live_slots", usage.index.live_slots}, {"index_retained_slots", usage.index.retained_slots}, {"empty_retained_slots", usage.index.empty_retained_slots}};
}

/** @param ids Strict original identities whose next straight proposals will be terminal. Declare geometry only; native retirement must still earn every weight and remove every live row. */
static void StageLifecycleWorkExits(std::span<const OrbitalBodyID> ids)
{
	auto field = GetAsteroidField(); const auto bounds = *GetSpacePlaneBounds(field.width, field.height);
	// Project only last-contained positions. Retained headings, stocks and
	// fractional carry remain canonical; no live ID or timer is replaced here.
	for (const auto id : ids) {
		auto *body = FindAsteroidBody(field, id); REQUIRE(body); const auto exit = ForecastAsteroidExit(*body, bounds); REQUIRE(exit);
		body->motion = exit->last_safe_motion;
	}
	REQUIRE(InstallAsteroidField(std::move(field)));
}

/** @param work Accumulated actual native proposal/commit operations. @return Exact newly issued body after one explicitly declared due checkpoint; this helper claims no elapsed interval. */
static OrbitalBodyID CommitLifecycleWorkDue(AsteroidWorkMeasurement &work)
{
	auto field = GetAsteroidField(); REQUIRE(field.renewal.credits != 0); field.renewal.ticks_remaining = 0;
	REQUIRE(InstallAsteroidField(std::move(field))); const auto issued = GetAsteroidField().next_body_id;
	AsteroidLifecycleResult result;
	/** Native quote/commit alone publishes one body; the diagnostic scope never authorizes it. */
	MeasureAsteroidWork(work, [&] { result = CommitAsteroidRenewal(PlanNativeAsteroidRenewal()); });
	REQUIRE(result.status == AsteroidLifecycleStatus::Spawned); REQUIRE(result.spawned); CHECK(result.spawned->base() == issued);
	CHECK(GetAsteroidField().next_body_id == issued + 1); CHECK(GetMutableAsteroidBody(*result.spawned)->size != GetAsteroidField().renewal.last_exit->size);
	return *result.spawned;
}

/** @return Genuine insufficient-credit native intervals with a live field; original bodies are recentered only between intervals, never during their measured straight travel. */
static nlohmann::json MeasureInsufficientLifecycleIntervals()
{
	ProbeExtractionRig rig; auto field = *GenerateAsteroidField(64, 64, ASTEROID_WORK_SEED, AsteroidDensity::Normal); field.elapsed_ticks = 1;
	const auto small = std::ranges::find(field.bodies, AsteroidSize::Small, &AsteroidBody::size); REQUIRE(small != field.bodies.end());
	const auto id = small->id; rig.ResetField(std::move(field)); StageLifecycleWorkExits(std::array{id});
	REQUIRE(RetireAsteroidBatch(std::array{id}).status == AsteroidLifecycleStatus::Retired);
	const auto issued = GetAsteroidField().next_body_id; AsteroidWorkMeasurement waiting; uint64_t ticks = 0;
	// Each real interval must close once without an eligible different size.
	// Recenter surviving geometry between intervals so unrelated genuine exits
	// cannot legitimately add credit and change this one-Small-credit boundary.
	for (uint interval = 0; interval < 32; ++interval) {
		field = GetAsteroidField();
		for (auto &body : field.bodies) body.motion.position = {32 * SPACE_Q16, 32 * SPACE_Q16};
		REQUIRE(InstallAsteroidField(std::move(field))); const auto duration = GetAsteroidField().renewal.ticks_remaining;
		REQUIRE(duration >= GetAsteroidRenewalDelayBounds().first);
		/** Every saved tick is earned by the real field owner; no due deadline is assigned in this interval trace. */
		MeasureAsteroidWork(waiting, [&] { for (uint32_t tick = 0; tick < duration; ++tick) TickAsteroidField(); }); ticks += duration;
		CHECK(GetAsteroidField().next_body_id == issued); CHECK(GetAsteroidField().renewal.credits == 1);
		CHECK(GetAsteroidField().renewal.last_exit->size == AsteroidSize::Small); CHECK(GetAsteroidField().bodies.size() == 9);
		CHECK(GetAsteroidField().renewal.ticks_remaining >= GetAsteroidRenewalDelayBounds().first);
	}
	CHECK(waiting.work.Get(SpaceWorkKind::RenewalAttempts) == 32); CHECK(waiting.work.Get(SpaceWorkKind::RenewalEntryCandidates) == 0);
	CHECK(waiting.work.Get(SpaceWorkKind::BodySteps) == 9 * ticks);
	return {{"intervals", 32}, {"actual_field_ticks", ticks}, {"credits_retained", 1}, {"bodies_retained", 9}, {"measured", AsteroidWorkRecord(waiting)}};
}

/** Native large/sparse fields must have bounded actual event work and live storage, including high-ID packet history. */
TEST_CASE("Native lifecycle event work and storage follow live rows rather than lifetime IDs", "[.asteroid-lifecycle-work]")
{
	const char *output = std::getenv("OTTD_LIFECYCLE_WORK_OUTPUT"); REQUIRE(output);
	const auto &scenario = SelectedAsteroidWorkCase(); nlohmann::json report{{"_documentation", "r2sff1-space-fixes-features-1.md33; isolated native field components, declared geometry/history/due checkpoints; not whole-game FPS"},
		{"case", scenario.name}, {"initial_population", scenario.bodies}, {"width", scenario.width}, {"height", scenario.height}};
	{
		ProbeExtractionRig rig; AsteroidWorkMeasurement generation; std::optional<AsteroidField> generated;
		/** Only the canonical generator supplies the initial exact population/content. */
		MeasureAsteroidWork(generation, [&] { generated = GenerateAsteroidField(scenario.width, scenario.height, ASTEROID_WORK_SEED, scenario.density); });
		REQUIRE(generated); REQUIRE(generated->bodies.size() == scenario.bodies); PrepareAsteroidWorkGeometry(*generated, scenario);
		generated->elapsed_ticks = std::max<uint64_t>(1, generated->elapsed_ticks); rig.ResetField(std::move(*generated));
		const auto random = _random; const auto initial = InspectAsteroidStorage();
		const auto large = std::ranges::find(GetAsteroidField().bodies, AsteroidSize::Large, &AsteroidBody::size); REQUIRE(large != GetAsteroidField().bodies.end());
		const auto first = large->id; StageLifecycleWorkExits(std::array{first}); REQUIRE(RetireAsteroidBatch(std::array{first}).status == AsteroidLifecycleStatus::Retired);
		AsteroidWorkMeasurement births; auto surviving = CommitLifecycleWorkDue(births); REQUIRE(GetAsteroidField().bodies.size() == scenario.bodies);
		AsteroidWorkMeasurement capacity;
		// Only the supported maximum live population can qualify actual capacity
		// refusal; smaller supported worlds are not mislabeled full fields.
		if (scenario.bodies == MAX_ORBITAL_BODIES) {
			auto field = GetAsteroidField(); field.renewal.ticks_remaining = 0; REQUIRE(InstallAsteroidField(std::move(field)));
			const auto before = GetAsteroidField(); AsteroidLifecycleResult result;
			/** Capacity admission must close one real attempt without inserting an identity or spending credit. */
			MeasureAsteroidWork(capacity, [&] { result = CommitAsteroidRenewal(PlanNativeAsteroidRenewal()); });
			CHECK(result.status == AsteroidLifecycleStatus::Capacity); CHECK(result.attempt_closed); CHECK_FALSE(result.spawned);
			CHECK(GetAsteroidField().next_body_id == before.next_body_id); CHECK(GetAsteroidField().renewal.credits == before.renewal.credits);
			CHECK(capacity.work.Get(SpaceWorkKind::RenewalAttempts) == 1);
		}
		AsteroidWorkMeasurement motion;
		/** Eight actual ordinary field opportunities publish one motion/index update per live body; no whole-field selector is authorized by motion. */
		MeasureAsteroidWork(motion, [&] { for (uint step = 0; step < 8; ++step) TickAsteroidField(); });
		CHECK(motion.work.Get(SpaceWorkKind::BodySteps) == uint64_t{8} * scenario.bodies);
		CHECK(motion.work.Get(SpaceWorkKind::IndexUpdates) == uint64_t{8} * scenario.bodies);
		CHECK(motion.work.Get(SpaceWorkKind::MiningSelections) == 0); CHECK(motion.work.Get(SpaceWorkKind::InterceptForecasts) == 0);
		CHECK(motion.work.Get(SpaceWorkKind::RenewalAttempts) == 0);
		auto field = GetAsteroidField(); field.knowledge.clear(); std::vector<OrbitalBodyID> burst;
		// Declared disclosure receipts exercise actual linear filtering. The native
		// batch must remove every retired row without keeping dead-body tombstones.
		for (const auto *body : GetAsteroidBodiesInIDOrder(field)) {
			field.knowledge.push_back({body->id, 1}); if (body->id != surviving) burst.push_back(body->id);
		}
		REQUIRE(InstallAsteroidField(std::move(field))); StageLifecycleWorkExits(burst); AsteroidWorkMeasurement retirement;
		/** One sorted native burst publishes original weighted credits and live-only index/disclosure storage. */
		MeasureAsteroidWork(retirement, [&] { REQUIRE(RetireAsteroidBatch(burst).status == AsteroidLifecycleStatus::Retired); });
		CHECK(retirement.work.Get(SpaceWorkKind::IndexUnlinks) == burst.size()); REQUIRE(GetAsteroidField().bodies.size() == 1);
		const auto after_burst = InspectAsteroidStorage(); CHECK(after_burst.lookup_rows == 1); CHECK(after_burst.knowledge == 1);
		CHECK(after_burst.index.membership_rows == 1); CHECK(after_burst.index.live_slots == 1); CHECK(after_burst.index.empty_retained_slots == 0);
		field = GetAsteroidField(); const uint64_t prefix = 65'536 - field.next_body_id;
		// This schema boundary is explicitly declared historical turnover. Original
		// live IDs, current credit, timer and both streams are preserved; later
		// high-ID issuance/retirement uses only the actual native transaction owner.
		field.next_body_id = 65'536; field.renewal.exited_by_size[0] += (prefix + 1) / 2; field.renewal.exited_by_size[1] += prefix / 2;
		REQUIRE(ValidateAsteroidField(field)); REQUIRE(InstallAsteroidField(std::move(field)));
		StageLifecycleWorkExits(std::array{surviving}); REQUIRE(RetireAsteroidBatch(std::array{surviving}).status == AsteroidLifecycleStatus::Retired);
		surviving = CommitLifecycleWorkDue(births); REQUIRE(surviving.base() == 65'536);
		const auto source = PrepareAsteroidMiningSource(*GetMutableAsteroidBody(surviving)); REQUIRE(source);
		const auto packets_before = CargoPacket::GetNumItems();
		// One actual native packet allocation measures retained inline provenance.
		// It is an isolated storage fixture, not claimed paid extraction;32B owns
		// the ordinary native high-ID extraction and joined-custody proof.
		REQUIRE(CargoPacket::CanAllocateItem());
		std::unique_ptr<CargoPacket> packet(CargoPacket::Create(StationID::Invalid(), 1, *source)); REQUIRE(packet);
		const auto sparse_before = InspectAsteroidStorage(); AsteroidWorkMeasurement turnover;
		// Repeated real high-ID turnover must preserve one packet while every
		// retired body/index/knowledge row disappears from native live ownership.
		for (uint cycle = 0; cycle < 64; ++cycle) {
			const auto retired = surviving; StageLifecycleWorkExits(std::array{retired});
			/** Each native publication removes one real live record and preserves the actual packet's historical Source. */
			MeasureAsteroidWork(turnover, [&] { REQUIRE(RetireAsteroidBatch(std::array{retired}).status == AsteroidLifecycleStatus::Retired); });
			CHECK(GetMutableAsteroidBody(retired) == nullptr); CHECK(packet->GetSource() == *source);
			surviving = CommitLifecycleWorkDue(births); CHECK(surviving.base() == retired.base() + 1);
			CHECK(CargoPacket::GetNumItems() == packets_before + 1); CHECK(GetAsteroidField().knowledge.empty());
		}
		const auto sparse_after = InspectAsteroidStorage();
		CHECK(sparse_after.bodies == 1); CHECK(sparse_after.body_capacity <= 4); CHECK(sparse_after.lookup_rows == 1); CHECK(sparse_after.lookup_buckets <= 16);
		CHECK(sparse_after.index.membership_rows == 1); CHECK(sparse_after.index.retained_slots <= 4); CHECK(sparse_after.index.empty_retained_slots == 0);
		CHECK(sparse_after.knowledge == 0); CHECK(sparse_after.knowledge_capacity <= 1);
		CHECK(sparse_after.index.bucket_headers == initial.index.bucket_headers);
		CHECK(births.work.Get(SpaceWorkKind::RenewalAttempts) == 66); CHECK(births.work.Get(SpaceWorkKind::RenewalEntryCandidates) <= 32 * 66);
		CHECK(turnover.work.Get(SpaceWorkKind::RetirementParticipants) == 0);
		StageLifecycleWorkExits(std::array{surviving}); REQUIRE(RetireAsteroidBatch(std::array{surviving}).status == AsteroidLifecycleStatus::Retired);
		const auto total_weight = GetAsteroidField().renewal.credits; AsteroidWorkMeasurement empty; uint64_t empty_ticks = 0;
		// A valid empty field retains actual weighted credits. Every interval is
		// earned by real native ticks; birth occurs only when that interval expires.
		for (uint interval = 0; interval < 16; ++interval) {
			REQUIRE(GetAsteroidField().bodies.empty()); const auto duration = GetAsteroidField().renewal.ticks_remaining;
			const auto issued = GetAsteroidField().next_body_id; REQUIRE(duration >= GetAsteroidRenewalDelayBounds().first);
			/** No live body means zero motion work; the sole due opportunity may publish exactly one new finite body. */
			MeasureAsteroidWork(empty, [&] { for (uint32_t tick = 0; tick < duration; ++tick) TickAsteroidField(); }); empty_ticks += duration;
			REQUIRE(GetAsteroidField().next_body_id == issued + 1); REQUIRE(GetAsteroidField().bodies.size() == 1);
			surviving = OrbitalBodyID{issued}; CHECK(GetMutableAsteroidBody(surviving)->size != GetAsteroidField().renewal.last_exit->size);
			StageLifecycleWorkExits(std::array{surviving}); REQUIRE(RetireAsteroidBatch(std::array{surviving}).status == AsteroidLifecycleStatus::Retired);
			CHECK(GetAsteroidField().renewal.credits == total_weight); CHECK(packet->GetSource() == *source);
		}
		CHECK(empty.work.Get(SpaceWorkKind::BodySteps) == 0); CHECK(empty.work.Get(SpaceWorkKind::RenewalAttempts) == 16);
		CHECK(empty.work.Get(SpaceWorkKind::RenewalEntryCandidates) <= 32 * 16);
		CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
		report.update({{"generation", AsteroidWorkRecord(generation)}, {"motion", AsteroidWorkRecord(motion)}, {"burst", AsteroidWorkRecord(retirement)},
			{"capacity", AsteroidWorkRecord(capacity)}, {"births", AsteroidWorkRecord(births)}, {"turnover", AsteroidWorkRecord(turnover)},
			{"initial_storage", LifecycleStorageRecord(initial)}, {"after_burst_storage", LifecycleStorageRecord(after_burst)},
			{"high_before_storage", LifecycleStorageRecord(sparse_before)}, {"high_after_storage", LifecycleStorageRecord(sparse_after)},
			{"declared_historical_prefix", prefix}, {"high_id_cycles", 64}, {"actual_retained_packets", 1}, {"inline_source_bytes", sizeof(Source)},
			{"native_packet_bytes", sizeof(CargoPacket)}, {"empty_intervals", 16}, {"empty_actual_ticks", empty_ticks}, {"empty_work", AsteroidWorkRecord(empty)}});
	}
	// The no-eligible-size boundary belongs to a valid still-populated field;
	// an empty field must conserve all original weight rather than fake one credit.
	if (scenario.name == "small") report["insufficient_credit"] = MeasureInsufficientLifecycleIntervals();
	std::ofstream result(output); REQUIRE(result.good()); result << report.dump(2) << '\n'; result.flush(); REQUIRE(result.good());
	fmt::print("LIFECYCLE_WORK case={} initial={} high_cycles=64 empty_intervals=16 native=true paid=false\n", scenario.name, scenario.bodies);
}
