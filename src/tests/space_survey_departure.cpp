/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_survey_departure.cpp Lost-body native scan and ownerless Return/Waiting contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group26.
 * Ownerless phases, counters and terminal ledger inputs are declared components, not native hulls, earned scans or executed lifecycle events.
 */
#include "../stdafx.h"
#include "asteroid_snapshot_fixture.h"
#include "../3rdparty/catch2/catch.hpp"
#include "probe_mining_fixture.h"
#include "space_return_fixture.h"
#include "../saveload/asteroid_field_sl.h"
#include "../space_departure.h"
#include "../vehicle_cmd.h"
#include <set>
#include "../safeguards.h"

/** @param phase Declared original ownerless checkpoint. @return Valid isolated field with one observation record and no native rights. */
static AsteroidField SurveyDepartureField(SpaceSurveyPhase phase)
{
	auto field = GenerateAsteroidField(64, 64, 53022769049978ULL, AsteroidDensity::Normal); REQUIRE(field);
	auto &body = field->bodies.front(); SpaceSurveyPreview preview;
	preview.position = {body.motion.position.x + SPACE_Q16, body.motion.position.y - SPACE_Q16 / 2};
	preview.home = {body.motion.position.x - 4 * SPACE_Q16, body.motion.position.y - 3 * SPACE_Q16};
	preview.target = body.id; preview.phase = phase; preview.completed_surveys = 7;
	preview.remaining_ticks = phase == SpaceSurveyPhase::Attached ? 148 : phase == SpaceSurveyPhase::Waiting ? 74 : 0;
	if (phase == SpaceSurveyPhase::Attached) body.survey_slot = preview.id;
	field->surveys.push_back(preview); REQUIRE(ValidateAsteroidField(*field)); return *field;
}

/** @param field Original copied component. @param bodies Existing identities to retire together. Fund declared terminal history and remove only live records/knowledge after preview release. */
static void RetireSurveyComponentBodies(AsteroidField &field, std::span<const OrbitalBodyID> bodies)
{
	std::vector<AsteroidExitRecord> exits;
	// The complete original batch funds its own dedicated size credits. No
	// reserve, cargo, native hull or earned scan is supplied by this fixture.
	for (const auto id : bodies) { const auto *body = FindAsteroidBody(field, id); REQUIRE(body); exits.push_back({id, {body->size, body->resource_class}}); }
	const auto renewal = ApplyAsteroidExitCredit(field.renewal, exits, field.next_body_id); REQUIRE(renewal);
	field.elapsed_ticks = std::max<uint64_t>(field.elapsed_ticks, 1); // Declared terminal history is evolved; pristine birth cannot already contain exits.
	ForgetSpaceSurveyBodyTargets(field, bodies); field.renewal = *renewal;
	// Stable IDs survive compact erasure as history only; no former vector
	// ordinal can retain an observation lease or company disclosure.
	for (const auto id : bodies) { RemoveAsteroidKnowledge(field, id); REQUIRE(EraseAsteroidBody(field, id)); }
	REQUIRE(ValidateAsteroidField(field));
}

/** Original synchronized observation keeps its earned scan cycle and grants no native resource or disclosure rights. */
TEST_CASE("Original ownerless preview controller still earns repeated scans without extracting", "[space-survey-departure]")
{
	ProbeExtractionRig rig; const auto generated = GenerateAsteroidField(64, 64, 77, AsteroidDensity::Normal); REQUIRE(generated);
	rig.ResetField(*generated); REQUIRE(PrepareSpaceSurveyPreview()); auto field = GetAsteroidField(); std::set<SpaceSurveyPhase> phases;
	// Offer the existing synchronized body/preview opportunities. Validation
	// after every publication detects a lost lease or hidden timed phase while
	// all four original controller phases and repeated real scans are exercised.
	for (uint tick = 0; tick < 3000; ++tick) {
		AdvanceContainedAsteroidSnapshot(field); REQUIRE(ValidateAsteroidField(field));
		for (const auto &preview : field.surveys) phases.insert(preview.phase);
	}
	CHECK(phases.size() == 4);
	for (const auto &preview : field.surveys) CHECK(preview.completed_surveys >= 2);
	// Ownerless observation never acquires finite resource or native custody.
	for (const auto &body : field.bodies) CHECK(body.remaining_reserve == FindAsteroidBody(*generated, body.id)->remaining_reserve);
	CHECK(Vehicle::GetNumItems() == 0); CHECK(field.knowledge.empty());
}

/** Every old phase loses only incomplete target work; original coordinates/home/history survive current codec admission. */
TEST_CASE("Ownerless target retirement retains ordinary home return without a live body", "[space-survey-departure]")
{
	const auto phase = GENERATE(SpaceSurveyPhase::Approach, SpaceSurveyPhase::Attached, SpaceSurveyPhase::Return, SpaceSurveyPhase::Waiting);
	auto field = SurveyDepartureField(phase); const auto original = field.surveys.front();
	const auto old = EncodeAsteroidField(field); REQUIRE(old); CHECK(DecodeAsteroidField(*old, AsteroidFieldWireEra::Lifecycle));
	const std::array retired{original.target}; RetireSurveyComponentBodies(field, retired);
	auto &preview = field.surveys.front(); CHECK(preview.target == INVALID_ORBITAL_BODY); CHECK(preview.position == original.position);
	CHECK(preview.home == original.home); CHECK(preview.completed_surveys == original.completed_surveys); CHECK(preview.travel_remainder == original.travel_remainder);
	CHECK(preview.phase == (phase == SpaceSurveyPhase::Waiting ? SpaceSurveyPhase::Waiting : SpaceSurveyPhase::Return));
	const auto bytes = EncodeAsteroidField(field); REQUIRE(bytes);
	const auto decoded = DecodeAsteroidField(*bytes, AsteroidFieldWireEra::SurveyDeparture); REQUIRE(decoded);
	CHECK(decoded->surveys == field.surveys); CHECK(ComputeAsteroidFieldDigest(*decoded) == ComputeAsteroidFieldDigest(field));
	CHECK_FALSE(DecodeAsteroidField(*bytes, AsteroidFieldWireEra::Lifecycle));
	// Advance only the original ownerless controller. Return and its finite
	// pause must finish without dereferencing the removed original target.
	for (uint tick = 0; preview.phase != SpaceSurveyPhase::Approach; ++tick) { REQUIRE(tick < 1000); AdvanceSpaceSurveyPreviews(field); }
	CHECK(preview.target == OrbitalBodyID{1}); CHECK(preview.completed_surveys == original.completed_surveys); CHECK(ValidateAsteroidField(field));
}

/** Empty fields are valid while observation models finish returning and retain an explicit no-target wait. */
TEST_CASE("Last ownerless target exit leaves bounded Return and Waiting", "[space-survey-departure]")
{
	auto field = SurveyDepartureField(SpaceSurveyPhase::Attached); std::vector<OrbitalBodyID> ids;
	for (const auto *body : GetAsteroidBodiesInIDOrder(field)) ids.push_back(body->id);
	RetireSurveyComponentBodies(field, ids); REQUIRE(field.bodies.empty()); const auto renewal = field.renewal;
	const auto home = field.surveys.front().home; const auto completed = field.surveys.front().completed_surveys;
	// Repeated real preview opportunities cannot mint a resource, native hull,
	// scan or replacement credit merely because no live target remains.
	for (uint tick = 0; tick < 1000; ++tick) AdvanceSpaceSurveyPreviews(field);
	const auto &preview = field.surveys.front(); CHECK(preview.phase == SpaceSurveyPhase::Waiting); CHECK(preview.target == INVALID_ORBITAL_BODY);
	CHECK(preview.position == home); CHECK(preview.remaining_ticks > 0); CHECK(preview.remaining_ticks <= 74); CHECK(preview.completed_surveys == completed);
	CHECK(field.renewal == renewal); CHECK(field.bodies.empty()); CHECK(ValidateAsteroidField(field));
	const auto bytes = EncodeAsteroidField(field); REQUIRE(bytes); const auto decoded = DecodeAsteroidField(*bytes, AsteroidFieldWireEra::SurveyDeparture); REQUIRE(decoded);
	CHECK(decoded->surveys == field.surveys); CHECK(decoded->renewal == renewal);
}

/** Stable live IDs select successors regardless of compact storage, wrapping only after the greatest surviving identity. */
TEST_CASE("Ownerless target selection traverses surviving IDs without modulo three", "[space-survey-departure]")
{
	auto field = SurveyDepartureField(SpaceSurveyPhase::Waiting); auto &preview = field.surveys.front();
	std::ranges::reverse(field.bodies); REQUIRE(RebuildAsteroidBodyLookup(field));
	preview.target = OrbitalBodyID{2}; preview.remaining_ticks = 1; AdvanceSpaceSurveyPreviews(field);
	CHECK(preview.target == OrbitalBodyID{3}); CHECK(preview.phase == SpaceSurveyPhase::Approach);
	preview.phase = SpaceSurveyPhase::Waiting; preview.target = OrbitalBodyID{field.next_body_id - 1}; preview.remaining_ticks = 1;
	AdvanceSpaceSurveyPreviews(field); CHECK(preview.target == OrbitalBodyID{0}); CHECK(ValidateAsteroidField(field));
}

/** Ownerless attachment release retains its valid original point before an unsafe next body offset is published. */
TEST_CASE("Ownerless unsafe attachment loses no earned survey at the edge", "[space-survey-departure]")
{
	auto field = SurveyDepartureField(SpaceSurveyPhase::Attached); auto &body = field.bodies.front(); auto &preview = field.surveys.front();
	const auto bounds = *GetSpacePlaneBounds(field.width, field.height); const auto exit = ForecastAsteroidExit(body, bounds); REQUIRE(exit);
	const auto safety = ForecastSpaceAttachmentSafety(body.motion, *exit, {SPACE_Q16, -SPACE_Q16 / 2}, bounds, {}); REQUIRE(safety); REQUIRE(safety->first_unsafe_tick);
	const auto projected = ProjectSpaceMotionStraight(body.motion, bounds, *safety->first_unsafe_tick - 1); REQUIRE(projected);
	body.motion = projected->proposed; field.elapsed_ticks = 1;
	preview.position = {body.motion.position.x + SPACE_Q16, body.motion.position.y - SPACE_Q16 / 2}; REQUIRE(ValidateAsteroidField(field));
	const auto position = preview.position; const auto completed = preview.completed_surveys;
	ReleaseUnsafeSpaceSurveyPreviews(field, {}); CHECK(preview.position == position); CHECK(preview.target == INVALID_ORBITAL_BODY);
	CHECK(preview.phase == SpaceSurveyPhase::Return); CHECK(preview.remaining_ticks == 0); CHECK(preview.completed_surveys == completed);
	CHECK(body.survey_slot == INVALID_SPACE_SURVEY); CHECK(ValidateAsteroidField(field));
}

/** No-target validation grants only bounded released Return/Waiting, never a missing resource, hidden scan or lease. */
TEST_CASE("No-target ownerless admission cannot acquire pursuit work or a hidden lease", "[space-survey-departure]")
{
	auto field = SurveyDepartureField(SpaceSurveyPhase::Attached); const std::array retired{OrbitalBodyID{0}};
	RetireSurveyComponentBodies(field, retired); auto &preview = field.surveys.front();
	SECTION("pursuit needs live target") { preview.phase = SpaceSurveyPhase::Approach; CHECK_FALSE(ValidateAsteroidField(field)); }
	SECTION("attachment needs live target") { preview.phase = SpaceSurveyPhase::Attached; preview.remaining_ticks = 148; CHECK_FALSE(ValidateAsteroidField(field)); }
	SECTION("missing identity is not the sentinel") { preview.target = retired.front(); CHECK_FALSE(ValidateAsteroidField(field)); }
	SECTION("return has no hidden scan timer") { preview.remaining_ticks = 1; CHECK_FALSE(ValidateAsteroidField(field)); }
	SECTION("return has no hidden body lease") { field.bodies.front().survey_slot = preview.id; CHECK_FALSE(ValidateAsteroidField(field)); }
	SECTION("physical last center applies") { preview.position.x = static_cast<int32_t>(field.width - 1) * SPACE_Q16 + 1; CHECK_FALSE(ValidateAsteroidField(field)); }
}

/** Native partial scan loss keeps a genuine earlier disclosure while clearing only the current original action's uncommitted work. */
TEST_CASE("Native survey target exit discards only partial work and preserves stopped choice", "[space-survey-departure]")
{
	SpaceReturnRig rig; auto &job = rig.Start(19); rig.Work(GetSpaceVehicleConstruction(19)->work_ticks);
	auto *craft = ResolveSpaceCraft(job.product); REQUIRE(craft);
	// Two original actions make the first scan an earned company receipt and
	// the second a genuine incomplete interval when the target disappears.
	for (uint8_t index = 0; index < 2; ++index) REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert,
		index, SpaceOrderAction::Survey, SpaceOrderTargetKind::Body, 0, 0).Succeeded());
	for (uint tick = 0; craft->space_orders.cursor != 1 || craft->space_orders.work_ticks == 0; ++tick) { REQUIRE(tick < 5000); rig.Step(); }
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft->index, true).Succeeded());
	const auto knowledge = GetAsteroidField().knowledge; REQUIRE_FALSE(knowledge.empty());
	const auto position = craft->position; const auto entries = craft->space_orders.entries;
	const std::array removed{OrbitalBodyID{0}}; ForgetSpaceBodySurveyOrders(removed);
	CHECK(craft->space_orders.entries == entries); CHECK(craft->space_orders.cursor == 1); CHECK_FALSE(craft->space_orders.started);
	CHECK(craft->space_orders.work_ticks == 0); CHECK(craft->space_orders.timer_ticks == 0); CHECK_FALSE(craft->space_orders.approach);
	CHECK(craft->space_orders.blocked == SpaceOrderBlock::TargetRemoved); CHECK(craft->position == position); CHECK(craft->vehstatus.Test(VehState::Stopped));
	CHECK(GetAsteroidField().knowledge == knowledge); CHECK(ValidateNativeSpaceOrders());
	auto field = GetAsteroidField(); RetireSurveyComponentBodies(field, removed); REQUIRE(InstallAsteroidField(std::move(field)));
	REQUIRE(craft->Tick()); CHECK(craft->position == position); CHECK(craft->space_orders.cursor == 1); CHECK(ValidateNativeSpaceOrders());
}

/** Only the current affected natural Visit loses travel admission; a future coincident target and another current body remain unchanged. */
TEST_CASE("Native lost-body Visit release does not complete or restart unrelated actions", "[space-survey-departure]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(4, false);
	REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 0,
		SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Succeeded());
	rig.Step(); REQUIRE(craft.space_orders.started); REQUIRE(craft.space_orders.approach);
	auto &other = rig.ProbeAt({20 * SPACE_Q16, 20 * SPACE_Q16}, false);
	// The other native hull retains a distinct current destination and a future
	// Visit to the departing body. Only current action ownership is releasable.
	for (uint8_t index = 0; index < 2; ++index) REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, other.index, other.generation, SpaceOrderEdit::Insert,
		index, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, index == 0 ? 1 : 0, 0).Succeeded());
	rig.Step(); const auto untouched = other.space_orders; const auto other_position = other.position;
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded()); const auto position = craft.position;
	const std::array removed{OrbitalBodyID{0}}; ForgetSpaceBodySurveyOrders(removed);
	CHECK(craft.space_orders.cursor == 0); CHECK(craft.space_orders.blocked == SpaceOrderBlock::TargetRemoved); CHECK_FALSE(craft.space_orders.approach);
	CHECK(craft.position == position); CHECK(craft.vehstatus.Test(VehState::Stopped)); CHECK(other.space_orders == untouched); CHECK(other.position == other_position);
	CHECK(ValidateNativeSpaceOrders());
}
