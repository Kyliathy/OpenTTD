/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_work_costs-2.cpp Actual native probe component costs with matching canonical map geometry and original finite extraction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group21
 * Isolated legacy15 participants claim no paid procurement; native commands, arbitration, ticks and packet transactions own all measured work.
 */
#include "../stdafx.h"
#include "asteroid_work_fixture.h"
#include "probe_mining_fixture.h"
#include "../company_base.h"
#include "../space_mining_tuning.h"
#include "../safeguards.h"

/** Measure an isolated actual native fleet under one declared initial-pose policy.
 * @param scenario Exact native geometry/population, including declared clustered field inputs.
 * @param mining Whether real assignment and one actual opportunity first earn attachment from declared adjacent initial hull positions.
 * @return Original component operation/time records, actual cargo/debit and native money changes; no scalar stand-ins finance the result.
 */
static nlohmann::json MeasureNativeProbeWork(const AsteroidWorkCase &scenario, bool mining)
{
	ProbeExtractionRig rig; auto generated = GenerateAsteroidField(scenario.width, scenario.height, ASTEROID_WORK_SEED, scenario.density); REQUIRE(generated);
	PrepareAsteroidWorkGeometry(*generated, scenario); rig.ResetField(std::move(*generated));
	std::vector<OrbitalBodyID> targets;
	// Native IDs come only from the actual generated metallic records. The clustered attached case has one physical berth; its full64-hull distant approach still measures the shared native fleet bound.
	const uint32_t limit = mining && scenario.clustered ? 1 : MAX_SPACE_VEHICLES;
	for (const auto &body : GetAsteroidField().bodies) {
		if (body.resource_class != AsteroidClass::Metallic) continue;
		targets.push_back(body.id); if (targets.size() == limit) break;
	}
	REQUIRE(!targets.empty()); const auto count = static_cast<uint32_t>(targets.size()); std::vector<SpaceVehicle *> probes;
	// Declared initial poses create no work/cargo. Distant origins are separated above the asteroid band; adjacent origins still need real command admission and one actual native opportunity to attach.
	for (uint32_t index = 0; index < count; ++index) {
		const auto point = GetMutableAsteroidBody(targets[index])->motion.position;
		const SpacePoint adjacent{point.x + SPACE_Q16, point.y - SPACE_Q16 / 2};
		const uint32_t origin_columns = (scenario.width - 4) / 2;
		const SpacePoint distant{static_cast<int32_t>(2 + 2 * (index % origin_columns)) * SPACE_Q16,
			static_cast<int32_t>(2 + 2 * (index / origin_columns)) * SPACE_Q16};
		probes.push_back(&rig.ProbeAt(mining ? adjacent : distant, false));
	}
	AsteroidWorkMeasurement assignment; bool assigned = true;
	/** Actual commands retain native ownership, generation, role and finite-target admission rather than replacing the mission controller with benchmark state. */
	MeasureAsteroidWork(assignment, [&] { for (uint32_t index = 0; index < count; ++index) assigned &= CmdSetSpaceMissionChecked(DoCommandFlag::Execute, probes[index]->index, probes[index]->generation, targets[index].base()).Succeeded(); });
	REQUIRE(assigned);
	if (mining) {
		// Native body motion, shared arbitration and the original hull's Tick must actually earn every first attachment; no Mining phase or ready work is installed.
		rig.Step();
		for (const auto *probe : probes) { REQUIRE(probe->mission.phase == SpaceMissionPhase::Mining); REQUIRE(probe->mission.work_ticks == 0); REQUIRE(probe->cargo.TotalCount() == 0); }
	}
	const uint32_t steps = mining ? 74 : 32; const auto before_resources = AsteroidWorkResources(GetAsteroidField()); const auto money = Company::Get(_current_company)->money;
	AsteroidWorkMeasurement field, approach, craft; bool survived = true;
	// Measure original native components separately. Assertions/reporting stay outside the interval; these are Space subsystem costs, excluding terrain, unrelated vehicle types and rendering.
	for (uint32_t tick = 0; tick < steps; ++tick) {
		++TimerGameTick::counter;
		/** Publish actual moving bodies, native index changes and original attached transforms. */
		MeasureAsteroidWork(field, [&] { TickAsteroidField(); });
		/** Admit all original target requests once in the native stable priority order. */
		MeasureAsteroidWork(approach, [&] { PrepareSpaceApproachTick(); });
		/** Tick only the actual native hulls; each original cost/packet/finite-reserve transaction remains in SpaceVehicle::Tick. */
		MeasureAsteroidWork(craft, [&] { for (auto *probe : SpaceVehicle::Iterate()) survived &= probe->Tick(); });
	}
	REQUIRE(survived); REQUIRE(ValidateSpaceVehicles()); const auto after_resources = AsteroidWorkResources(GetAsteroidField());
	CHECK(field.work.Get(SpaceWorkKind::BodySteps) == uint64_t{scenario.bodies} * steps);
	CHECK(field.work.Get(SpaceWorkKind::IndexUpdates) == uint64_t{scenario.bodies} * steps);
	CHECK(field.work.Get(SpaceWorkKind::AttachmentVisits) == uint64_t{count} * steps);
	CHECK(approach.work.Get(SpaceWorkKind::ApproachCraftVisits) == uint64_t{2} * count * steps);
	CHECK(approach.work.Get(SpaceWorkKind::ApproachOccupantChecks) == uint64_t{count} * count * steps);
	CHECK(approach.work.Get(SpaceWorkKind::PlannerRequests) == uint64_t{count} * steps);
	CHECK(approach.work.Get(SpaceWorkKind::PlannerSlotTests) <= uint64_t{3} * count * count * steps);
	CHECK(approach.work.Get(SpaceWorkKind::PlannerOwnerTests) <= uint64_t{count} * count * steps);
	CHECK(craft.work.Get(SpaceWorkKind::CraftTicks) == uint64_t{count} * steps);
	CHECK(craft.work.Get(SpaceWorkKind::BodyLookups) <= uint64_t{32} * count * steps);
	CHECK(craft.work.Get(SpaceWorkKind::QueryCandidates) == 0);
	const auto units = mining ? 2U * count : 0;
	CHECK(craft.work.Get(SpaceWorkKind::ExtractionAttempts) == (mining ? count : 0)); CHECK(craft.work.Get(SpaceWorkKind::ExtractionUnits) == units);
	const int64_t expected_cost = (mining ? 500 : 43) * count; CHECK(money - Company::Get(_current_company)->money == Money{expected_cost});
	uint32_t held = 0;
	// Every original native hull must own its actual emitted packet count and work remainder; an aggregate profiler count alone cannot prove extraction or retained approach state.
	for (uint32_t index = 0; index < count; ++index) {
		const auto *probe = probes[index]; const auto *body = GetMutableAsteroidBody(targets[index]);
		CHECK(probe->cargo.TotalCount() == (mining ? 2 : 0)); CHECK(probe->extracted_units == (mining ? 2 : 0)); CHECK(probe->mission.work_ticks == 0);
		CHECK(probe->mission.phase == (mining ? SpaceMissionPhase::Mining : SpaceMissionPhase::Approach)); CHECK(body->initial_reserve - body->remaining_reserve == (mining ? 2 : 0)); held += probe->cargo.TotalCount();
	}
	CHECK(held == units);
	// Water Ice and Rare Earth remain independent untouched resources. Only actual native Nickel packets finance the measured finite debit.
	for (uint32_t resource = 0; resource < 3; ++resource) {
		CHECK(before_resources["initial"][resource] == after_resources["initial"][resource]);
		CHECK(before_resources["remaining"][resource].get<uint64_t>() - after_resources["remaining"][resource].get<uint64_t>() == (resource == 0 ? units : 0));
	}
	return {{"mining", mining}, {"probes", count}, {"steps", steps}, {"assignment", AsteroidWorkRecord(assignment)}, {"field", AsteroidWorkRecord(field)},
		{"approach", AsteroidWorkRecord(approach)}, {"craft", AsteroidWorkRecord(craft)}, {"held", held}, {"money_debit", expected_cost},
		{"before_resources", before_resources}, {"after_resources", after_resources}, {"final_digest", ComputeAsteroidFieldDigest(GetAsteroidField())}};
}

/** Actual approaching and attached native fleets must keep body work linear and target/craft work independent of total field population. */
TEST_CASE("Native probe component work remains bounded at exact asteroid populations", "[.asteroid-probe-work]")
{
	const auto &scenario = SelectedAsteroidWorkCase();
	const auto approach = MeasureNativeProbeWork(scenario, false), mining = MeasureNativeProbeWork(scenario, true);
	WriteAsteroidWork(scenario, "probes", {{"approach", approach}, {"mining", mining}, {"isolated_legacy_hulls", true}, {"whole_game_frame_rate_measured", false}});
}
