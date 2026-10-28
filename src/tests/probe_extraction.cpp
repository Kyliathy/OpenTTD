/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_extraction.cpp Actual finite extraction participants with declared isolated legacy hulls.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group10
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group11; shared actual departure admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group13; recalled attachment retains safe departure history.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group15; native offset/budget and simultaneous request coverage.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group16; earned final-unit exit and retained body identity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group20; generation-bound synchronized mission admission.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 8; extracted packets carry the incoming typed asteroid origin.
 * Unit hulls, package descriptors, ready-work and refusal inputs are explicit; separate paid native saves prove earned gameplay extraction.
 */
#include "../stdafx.h"
#include "../network/network.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_mining_inventory.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_mining_tuning.h"
#include "../space_approach.h"
#include "../space_departure.h"
#include "../core/backup_type.hpp"
#include "../asteroid.h"
#include "../asteroid_generation.h"
#include "../multilayer_asteroid_catalogue.h"
#include "../vehicle_func.h"
#include "../openttd.h"
#include "../timer/timer_game_tick.h"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "probe_mining_fixture.h"
#include "../safeguards.h"


/** Actual native pursuit at maximum catalogue drift must retain its frozen deadline and bounded movement from several declared starting offsets. */
TEST_CASE("Native basic probe intercepts maximum drift from bounded initial offsets", "[probe-interception]")
{
	const std::array starts{SpacePoint{21 * SPACE_Q16, 30 * SPACE_Q16 - SPACE_Q16 / 2}, SpacePoint{52 * SPACE_Q16, 22 * SPACE_Q16},
		SpacePoint{60 * SPACE_Q16, 40 * SPACE_Q16}, SpacePoint{35 * SPACE_Q16, 35 * SPACE_Q16}, SpacePoint{0, 0}};
	// Each isolated declared starting point uses the original native hull, shared arbitration and body motion. No copied geometry controller or installed mining progress can satisfy arrival.
	for (size_t index = 0; index < starts.size(); ++index) {
		// Earn the real first field opportunity before declaring changed geometry; the exact time-zero starter evidence is never bypassed by installing an elapsed counter.
		ProbeExtractionRig rig; rig.Step(); auto field = GetAsteroidField(); const auto historical = field.starter_input;
		field.bodies[0].motion = {{52 * SPACE_Q16, 30 * SPACE_Q16}, 16384, 0, 0, 0}; REQUIRE(InstallAsteroidField(std::move(field)));
		auto &craft = rig.ProbeAt(starts[index]); const auto initial = craft.mission.approach_ticks_left; const auto reserve = GetAsteroidField().bodies[0].remaining_reserve; uint elapsed = 0;
		// A started attempt never extends its assigned budget. Native physical steps, including the final quarter-tile attachment snap, remain below a whole tile.
		while (craft.mission.phase == SpaceMissionPhase::Approach) {
			REQUIRE(elapsed++ < initial); const auto position = craft.position; const auto remaining = craft.mission.approach_ticks_left; rig.Step();
			CHECK(craft.mission.approach_ticks_left <= remaining); CHECK(SpaceDistanceSquared(position, craft.position) < uint64_t{SPACE_Q16} * SPACE_Q16);
		}
		CHECK(craft.mission.phase == SpaceMissionPhase::Mining); CHECK(GetAsteroidField().bodies[0].mining_slot == craft.index); CHECK(elapsed <= initial);
		CHECK(craft.cargo.TotalCount() == 0); CHECK(craft.extracted_units == 0); CHECK(GetAsteroidField().bodies[0].remaining_reserve == reserve); CHECK(GetAsteroidField().starter_input == historical);
		if (index == 0) { CHECK(initial == 1480); CHECK(elapsed <= 632); } // Original Group3's32-tile maximum-drift geometry bound; native tolerance may attach earlier.
	}
}

/** All three genuine generated starter resources retain their frozen access bound and admit the actual basic probe without revealing quantities or creating paid supplies. */
TEST_CASE("Native generated starter pursuits fit their published historical access bound", "[probe-interception]")
{
	// The three starter identities are guaranteed resource classes, not hand-picked nearby alternatives. Explicit component cargo dimensions authorize only the isolated empty-hold refit.
	for (uint32_t target = 0; target < 3; ++target) {
		ProbeExtractionRig rig; const auto historical = GetAsteroidField().starter_input; const auto bound = GetAsteroidInitialApproachDistance(historical); REQUIRE(bound);
		const auto &body = GetAsteroidField().bodies[target]; const auto cargo = GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[target].cargo); REQUIRE(IsValidCargoType(cargo));
		AutoRestoreBackup dimension(CargoSpec::Get(cargo)->weight, uint8_t{16}); auto &craft = rig.ProbeAt(historical.preference, false);
		const SpacePoint attachment{body.motion.position.x + craft.mission.attachment_offset.x, body.motion.position.y + craft.mission.attachment_offset.y};
		CHECK(SpaceDistanceSquared(craft.position, attachment) <= uint64_t{*bound} * *bound); REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, craft.index, target).Succeeded());
		const auto initial = craft.mission.approach_ticks_left; uint elapsed = 0;
		// Actual shared pursuit must reach this exact generated starter within its original assignment, while motion leaves the historical access record untouched.
		while (craft.mission.phase == SpaceMissionPhase::Approach) { REQUIRE(elapsed++ < initial); rig.Step(); }
		CHECK(craft.mission.phase == SpaceMissionPhase::Mining); CHECK(body.mining_slot == craft.index); CHECK(craft.cargo_type == cargo); CHECK(craft.cargo.TotalCount() == 0); CHECK(GetAsteroidField().starter_input == historical);
	}
}

/** Simultaneous requests use native tick then hull ID priority, even when the closer higher ID's command is posted first. */
TEST_CASE("Native same tick body requests resolve identically in either command order", "[probe-interception]")
{
	// Reverse command arrival without changing native hull creation/identity or physical starting offsets. The lower ID is farther away, so proximity cannot accidentally explain the documented winner.
	for (bool reversed : {false, true}) {
		ProbeExtractionRig rig; auto &first = rig.Probe(5, false), &second = rig.Probe(2, false); REQUIRE(first.index < second.index);
		auto &early = reversed ? second : first; auto &late = reversed ? first : second;
		REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, early.index, uint32_t{0}).Succeeded()); REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, late.index, uint32_t{0}).Succeeded());
		REQUIRE(first.space_orders.approach); REQUIRE(second.space_orders.approach); CHECK(first.space_orders.approach->request_tick == second.space_orders.approach->request_tick);
		const auto waiting_budget = second.mission.approach_ticks_left; uint elapsed = 0;
		// One shared pre-decision arbitration must preserve exactly one actionable slot/lease. The loser retains its frozen pursuit budget and complete empty hold while waiting.
		while (first.mission.phase == SpaceMissionPhase::Approach) {
			REQUIRE(elapsed++ < 1480); rig.Step(); CHECK(first.space_orders.approach->slot == 0); CHECK(second.space_orders.approach->slot == UINT8_MAX);
			CHECK(second.mission.phase == SpaceMissionPhase::Approach); CHECK(second.mission.approach_ticks_left == waiting_budget); CHECK(second.cargo.TotalCount() == 0);
		}
		CHECK(GetAsteroidField().bodies[0].mining_slot == first.index); CHECK(first.mission.phase == SpaceMissionPhase::Mining); CHECK(first.cargo.TotalCount() == 0);
		REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, first.index, UINT32_MAX).Succeeded()); rig.Attach(second);
		CHECK(GetAsteroidField().bodies[0].mining_slot == second.index); CHECK(first.cargo.TotalCount() == 0); CHECK(second.cargo.TotalCount() == 0);
	}
}

/** A declared one-unit reserve earns its complete native work interval, then remains the same exhausted body rather than a reusable target identity. */
TEST_CASE("Native final reserve unit exits only after earned work and never reuses its body", "[probe-boundaries]")
{
	ProbeExtractionRig rig; auto field = GetAsteroidField();
	// This explicit isolated remaining-reserve input is not paid depletion. The actual controller must still earn all74 opportunities before producing its one real packet.
	field.bodies[0].remaining_reserve = 1; REQUIRE(InstallAsteroidField(std::move(field)));
	auto &craft = rig.Probe(); auto &body = rig.Attach(craft); const auto count = GetAsteroidField().bodies.size(); const auto initial = body.initial_reserve; const auto target = body.id;
	// The final tail cannot be extracted early by rounding a partial interval or pretending the configured two-unit batch exists in a one-unit reserve.
	for (uint elapsed = 0; elapsed < 73; ++elapsed) { rig.Step(); CHECK(craft.cargo.TotalCount() == 0); CHECK(body.remaining_reserve == 1); }
	CHECK(craft.mission.work_ticks == 73); rig.Step(); REQUIRE(craft.cargo.TotalCount() == 1); REQUIRE(craft.cargo.Packets()->size() == 1);
	const auto *packet = craft.cargo.Packets()->front(); const auto id = packet->index; const auto tile = packet->GetSourceTile();
	CHECK(craft.extracted_units == 1); CHECK(craft.mission.work_ticks == 0); CHECK(craft.mission.phase == SpaceMissionPhase::Return); CHECK(body.remaining_reserve == 0); CHECK(body.mining_slot == VehicleID::Invalid());
	const auto digest = ComputeSpaceVehicleDigest();
	// Both real command forms must refuse the exhausted identity; an out-of-range ID cannot become an implicit replacement target either.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(CmdSetSpaceMission(flags, craft.index, target.base()).Failed()); CHECK(CmdSetSpaceMission(flags, craft.index, static_cast<uint32_t>(count)).Failed());
	}
	CHECK(ComputeSpaceVehicleDigest() == digest);
	// More than another extraction interval must leave the same moving exhausted record and original packet, proving depletion never deletes/recycles IDs or replays the last unit.
	for (uint elapsed = 0; elapsed < 75; ++elapsed) rig.Step();
	CHECK(GetAsteroidField().bodies.size() == count); CHECK(GetMutableAsteroidBody(target) == &body); CHECK(body.id == target); CHECK(body.initial_reserve == initial); CHECK(body.remaining_reserve == 0);
	CHECK(craft.cargo.TotalCount() == 1); CHECK(craft.cargo.Packets()->front()->index == id); CHECK(craft.cargo.Packets()->front()->GetSourceTile() == tile); CHECK(craft.extracted_units == 1);
}

/** Recall ends its uncommitted attachment interval while preserving emitted packets and the body required by the shared safe departure policy. */
TEST_CASE("Recall retains the last attached body for a safe cargo preserving departure", "[probe-exits]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); auto &body = rig.Attach(craft);
	// One real batch and half another interval supply retained cargo/work; the recall regression cannot be satisfied by installing a ready-work fixture.
	for (uint elapsed = 0; elapsed < 111; ++elapsed) rig.Step();
	REQUIRE(craft.cargo.TotalCount() == 2); REQUIRE(craft.mission.work_ticks == 37); const auto reserve = body.remaining_reserve; const auto packet = craft.cargo.Packets()->front()->index;
	SECTION("Running attachment") {}
	SECTION("Stopped attachment") { craft.vehstatus.Set(VehState::Stopped); rig.Step(); CHECK(craft.mission.work_ticks == 37); }
	const auto before = ComputeSpaceVehicleDigest();
	// Both estimate forms preserve the same lease/hold/history; a tooltip or mixed estimate cannot silently cancel live extraction.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) CHECK(CmdSetSpaceMission(flags, craft.index, UINT32_MAX).Succeeded());
	CHECK(ComputeSpaceVehicleDigest() == before); CHECK(body.mining_slot == craft.index);
	REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, craft.index, UINT32_MAX).Succeeded()); CHECK(craft.mission.phase == SpaceMissionPhase::Return); CHECK(craft.mission.target == body.id); CHECK(body.mining_slot == VehicleID::Invalid()); CHECK_FALSE(craft.vehstatus.Test(VehState::Stopped));
	const auto departure = GetSpaceMiningDepartureDestination(craft, craft.home); REQUIRE(departure); CHECK(*departure != craft.position);
	// Actual cancellation discards the unfinished attachment interval once; shared departure/return must preserve the committed batch rather than earn or debit another one.
	for (uint elapsed = 0; craft.mission.phase == SpaceMissionPhase::Return; ++elapsed) { REQUIRE(elapsed < 500); rig.Step(); }
	CHECK(craft.mission.phase == SpaceMissionPhase::Holding); CHECK(craft.cargo.TotalCount() == 2); CHECK(craft.cargo.Packets()->front()->index == packet); CHECK(craft.mission.work_ticks == 0); CHECK(body.remaining_reserve == reserve); CHECK(craft.extracted_units == 2);
}

/** Real competing legacy probes acquire one actual finite attachment and keep stopped leased hulls body-relative. */
TEST_CASE("Finite probe leases remain exclusive and attached transforms precede work", "[probe-extraction]")
{
	ProbeExtractionRig rig; auto &first = rig.Probe(), &second = rig.Probe(5); auto &body = rig.Attach(first);
	CHECK(body.mining_slot == first.index); CHECK(second.mission.phase == SpaceMissionPhase::Approach); CHECK(second.cargo.TotalCount() == 0);
	const auto reserve = body.remaining_reserve; const auto point = body.motion.position; first.vehstatus.Set(VehState::Stopped);
	// Actual stopped attachment follows motion while earning no work; the waiting second hull cannot borrow the occupied lease.
	for (uint elapsed = 0; elapsed < 74; ++elapsed) rig.Step();
	CHECK(body.motion.position != point); CHECK(first.position == SpacePoint{body.motion.position.x + first.mission.attachment_offset.x, body.motion.position.y + first.mission.attachment_offset.y});
	CHECK(first.mission.work_ticks == 0); CHECK(body.remaining_reserve == reserve); CHECK(second.cargo.TotalCount() == 0); CHECK(body.mining_slot == first.index);
	REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, first.index, UINT32_MAX).Succeeded()); rig.Attach(second); CHECK(body.mining_slot == second.index); CHECK(first.cargo.TotalCount() == 0);
}

/** Additional allocation veto must preserve real reserve, hold, ready work and issuance before the same physical participant retries once. */
TEST_CASE("Finite extraction refuses whole allocation before consuming original reserve", "[probe-extraction]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); auto &body = rig.Attach(craft); craft.mission.work_ticks = 74; // Explicit ready component interval; paid native acceptance earns it through actual ticks.
	const auto reserve = body.remaining_reserve; const auto packets = CargoPacket::GetNumItems();
	CHECK_FALSE(CommitSpaceMiningExtraction(craft, body, false)); CHECK(body.remaining_reserve == reserve); CHECK(craft.cargo.TotalCount() == 0);
	CHECK(craft.mission.work_ticks == 74); CHECK(craft.extracted_units == 0); CHECK(CargoPacket::GetNumItems() == packets); CHECK(body.mining_slot == craft.index);
	REQUIRE(CommitSpaceMiningExtraction(craft, body)); CHECK(body.remaining_reserve == reserve - 2); CHECK(craft.cargo.TotalCount() == 2); CHECK(craft.extracted_units == 2); CHECK(craft.mission.work_ticks == 0);
	CHECK_FALSE(CommitSpaceMiningExtraction(craft, body)); CHECK(body.remaining_reserve == reserve - 2); CHECK(craft.cargo.TotalCount() == 2);
}

/** Native per-step quantity is bounded jointly by remaining finite reserve, actual complete-unit hold room and original earned interval. */
TEST_CASE("Finite extraction preserves small tails full holds and original packet provenance", "[probe-extraction]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); auto &body = rig.Attach(craft); const auto initial = body.remaining_reserve;
	SECTION("Final reserve unit") { body.remaining_reserve = 1; }
	SECTION("Final hold unit") { craft.cargo_cap = 1; }
	craft.mission.work_ticks = 74; const auto offered = body.remaining_reserve; const auto expected = std::min<uint32_t>({2, offered, craft.cargo_cap});
	const auto tile = TileXYL((body.motion.position.x + SPACE_Q16 / 2) / SPACE_Q16, (body.motion.position.y + SPACE_Q16 / 2) / SPACE_Q16, MapLayer::Space);
	REQUIRE(CommitSpaceMiningExtraction(craft, body)); CHECK(craft.cargo.TotalCount() == expected); CHECK(body.remaining_reserve == offered - expected);
	const auto *packet = craft.cargo.Packets()->front(); const auto id = packet->index; const auto source_tile = packet->GetSourceTile();
	CHECK(packet->GetSource().type == SourceType::Asteroid);
	// Extraction now emits a typed full-width origin (r2sff1 Group15): the body ID and mineral live in the origin, not in the 16-bit source ID.
	const auto origin = packet->GetSource().GetAsteroidOrigin(); REQUIRE(origin.has_value()); CHECK(origin->body == body.id.base());
	 CHECK(source_tile == tile); CHECK(packet->GetFirstStation() == StationID::Invalid()); CHECK(packet->GetPeriodsInTransit() == 0);
	CHECK(body.mining_slot == VehicleID::Invalid()); CHECK(craft.mission.phase == SpaceMissionPhase::Return); CHECK(craft.mission.work_ticks == 0);
	// Later body motion and native cargo return cannot rewrite the already emitted packet's production coordinates or debit another finite unit.
	for (uint elapsed = 0; elapsed < 74; ++elapsed) rig.Step();
	CHECK(craft.cargo.TotalCount() == expected); CHECK(craft.cargo.Packets()->front()->index == id); CHECK(craft.cargo.Packets()->front()->GetSourceTile() == source_tile); CHECK(body.remaining_reserve == offered - expected); CHECK(initial > 1);
}

/** Shared departure queries avoid an actual occupied exit and move the original full hold without inventing a waypoint owner. */
TEST_CASE("Returning probe chooses a shared clear homeward exit without replaying extraction", "[probe-return]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); auto &body = rig.Attach(craft);
	// Actual native unit work fills the original hold before departure; no forced primary pose or synthetic ready interval supplies this case.
	for (uint elapsed = 0; elapsed < 740; ++elapsed) rig.Step();
	REQUIRE(craft.cargo.TotalCount() == 20); REQUIRE(craft.mission.phase == SpaceMissionPhase::Return); REQUIRE(body.mining_slot == VehicleID::Invalid());
	const auto reserve = body.remaining_reserve; const auto before = ComputeSpaceVehicleDigest(); const auto preferred = GetSpaceMiningDepartureDestination(craft, craft.home); REQUIRE(preferred);
	CHECK(ComputeSpaceVehicleDigest() == before); CHECK(*preferred != craft.position);
	auto &blocker = rig.Probe(5); REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, blocker.index, UINT32_MAX).Succeeded());
	blocker.vehstatus.Set(VehState::Stopped); blocker.position = *preferred; blocker.UpdateSpacePosition(); // Explicit isolated physical-blocker input, never native paid route evidence.
	const auto alternative = GetSpaceMiningDepartureDestination(craft, craft.home); REQUIRE(alternative);
	CHECK(*alternative != *preferred); CHECK(SpaceDistanceSquared(*alternative, blocker.position) >= uint64_t{SPACE_Q16} * SPACE_Q16);
	// Normal native movement must eventually clear the original body envelope; an exit biased away from home could oscillate forever on the next direct return step.
	for (uint elapsed = 0; SpaceDistanceSquared(craft.position, body.motion.position) < uint64_t{2 * SPACE_Q16} * (2 * SPACE_Q16); ++elapsed) { REQUIRE(elapsed < 200); rig.Step(); }
	CHECK(GetSpaceMiningDepartureDestination(craft, craft.home) == craft.position); CHECK(craft.cargo.TotalCount() == 20); CHECK(body.remaining_reserve == reserve); CHECK(craft.extracted_units == 20);
}

/** Delayed assignment and Recall must retain the originally selected incarnation even when the native pool actually reuses its ID. */
TEST_CASE("Generation checked mission commands refuse retired and foreign hull incarnations", "[probe-mission-generation]")
{
	ProbeExtractionRig rig; auto &first = rig.Probe(4, false); const auto original = GetSpaceCraftReference(first); delete &first;
	auto &replacement = rig.Probe(4, false); REQUIRE(replacement.index == original.vehicle); REQUIRE(replacement.generation != original.generation);
	const auto before = ComputeSpaceVehicleDigest(); const auto field = ComputeAsteroidFieldDigest(GetAsteroidField()); const auto money = Company::Get(_current_company)->money;
	// Neither assignment nor Recall may clear a new hull's queue, lease or funds when an old selection is delivered after native ID reuse.
	for (const uint32_t target : {uint32_t{0}, UINT32_MAX}) {
		// Queries and execution must agree on stale identity without granting work or changing the replacement's mission.
		for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) {
			CHECK(CmdSetSpaceMissionChecked(flags, original.vehicle, original.generation, target).Failed());
		}
	}
	CHECK(ComputeSpaceVehicleDigest() == before); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field); CHECK(Company::Get(_current_company)->money == money);
	{
		AutoRestoreBackup actor(_current_company, COMPANY_SPECTATOR);
		CHECK(CmdSetSpaceMissionChecked(DoCommandFlag::Execute, replacement.index, replacement.generation, uint32_t{0}).Failed());
	}
	CHECK(CmdSetSpaceMissionChecked({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, replacement.index, replacement.generation, uint32_t{0}).Succeeded()); CHECK(ComputeSpaceVehicleDigest() == before);
	{
		AutoRestoreBackup network(_networking, true);
		CHECK(CmdSetSpaceMission({}, replacement.index, uint32_t{0}).Failed()); CHECK(CmdSetSpaceMission(DoCommandFlag::Execute, replacement.index, UINT32_MAX).Failed());
		REQUIRE(CmdSetSpaceMissionChecked(DoCommandFlag::Execute, replacement.index, replacement.generation, uint32_t{0}).Succeeded());
	}
	CHECK(replacement.mission.phase == SpaceMissionPhase::Approach); CHECK(replacement.mission.target == OrbitalBodyID{0});
	REQUIRE(CmdSetSpaceMissionChecked(DoCommandFlag::Execute, replacement.index, replacement.generation, UINT32_MAX).Succeeded()); CHECK(replacement.mission.phase == SpaceMissionPhase::Return);
}
