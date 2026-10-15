/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_freight.cpp Detached module-flight receipt and shared-approach contract boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_transition.h"
#include "../space_approach.h"
#include "../multilayer_space_content.h"
#include "../fios.h"
#include "../safeguards.h"

/** @return Structurally valid detached orbital arrival; it creates no hull, paid module, packet or runtime authority. */
static SpaceTransitionRecord ModuleFreightArrival()
{
	SpaceTransitionRecord row; row.craft = {VehicleID{2}, 7}; auto &flight = row.state;
	flight.phase = SpaceFlightPhase::OrbitalDelivery; flight.layer = MapLayer::Space;
	flight.source = flight.recovery = {StationID{1}, 5}; flight.destination = {SpaceOrderTargetKind::Structure, 0, 3};
	flight.outbound_burned = true; flight.missions_started = 1; return row;
}

/** Module success remains distinct from deployment/resupply and can exist only after physical delivery entered recovery. */
TEST_CASE("Module freight preserves one role-specific flight receipt", "[orbital-freight-contract]")
{
	auto row = ModuleFreightArrival(); auto &flight = row.state;
	CHECK(ValidateSpaceTransitionRecords(std::array{row})); CHECK(IsSpaceModuleFreightApproach(flight, flight.destination));
	// Every wait retains the unresolved arrival role; none may imply an already transferred payload.
	for (auto block : {SpaceFlightBlock::ReceiverBusy, SpaceFlightBlock::ReceiverFull, SpaceFlightBlock::Approach}) {
		flight.blocked = block; CHECK(ValidateSpaceTransitionRecords(std::array{row})); CHECK(IsSpaceModuleFreightApproach(flight, flight.destination));
	}
	flight.blocked = SpaceFlightBlock::None; flight.module_delivered = true;
	CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{row})); CHECK_FALSE(IsSpaceModuleFreightApproach(flight, flight.destination));
	flight.phase = SpaceFlightPhase::ReturnRecovery; flight.recovery_burned = true; flight.remaining_ticks = GetMultiLayerSpaceBudget().flight_ticks;
	CHECK(ValidateSpaceTransitionRecords(std::array{row}));
	// Another role's flag, an unissued domain or a Surface arrival must never acquire the shared module berth.
	for (uint alternative : {0U, 1U, 2U, 3U, 4U}) {
		auto bad = row;
		if (alternative == 0) bad.state.resupplied = true;
		if (alternative == 1) bad.state.deployed = true;
		if (alternative == 2) bad.state.destination.generation = 0;
		if (alternative == 3) bad.state.destination.id = SpaceStructureID::End().base();
		if (alternative == 4) bad.state.layer = MapLayer::Surface;
		CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{bad}));
	}
	row = ModuleFreightArrival(); auto other = row.state.destination; ++other.generation;
	CHECK_FALSE(IsSpaceModuleFreightApproach(row.state, other)); other = row.state.destination; other.kind = SpaceOrderTargetKind::Blueprint;
	CHECK_FALSE(IsSpaceModuleFreightApproach(row.state, other)); row.state.layer = MapLayer::Surface;
	CHECK_FALSE(IsSpaceModuleFreightApproach(row.state, row.state.destination));
	row = ModuleFreightArrival(); row.state.phase = SpaceFlightPhase::Preparing; row.state.layer = MapLayer::Surface;
	row.state.outbound_burned = false; row.state.remaining_ticks = GetMultiLayerSpaceBudget().prepare_ticks;
	CHECK(ValidateSpaceTransitionRecords(std::array{row})); CHECK_FALSE(IsSpaceModuleFreightApproach(row.state, row.state.destination));
}

/** A flight shares exactly the same deterministic slot planner as an orbital probe; no special rocket berth exists. */
TEST_CASE("Module flights use existing native approach priority and physical blockers", "[orbital-freight-contract]")
{
	const auto arrival = ModuleFreightArrival(); const auto target = arrival.state.destination;
	SpaceApproachRequest request; request.target = target; request.request_tick = 12; request.staging = {8 * SPACE_Q16, 8 * SPACE_Q16};
	const SpaceApproachRecord rocket{VehicleID{2}, 7, request}, builder{VehicleID{3}, 8, request};
	const SpaceApproachSlot slot{target, 0, {4 * SPACE_Q16, 7 * SPACE_Q16}};
	auto planned = PlanSpaceApproaches(std::array{builder, rocket}, std::array{slot}); REQUIRE(planned.has_value());
	// Arrival submission order cannot override native-ID priority for simultaneous requests.
	for (const auto &row : *planned) CHECK(row.request.slot == (row.craft == rocket.craft ? 0 : UINT8_MAX));
	auto occupied = slot; occupied.occupant = builder.craft;
	planned = PlanSpaceApproaches(std::array{rocket, builder}, std::array{occupied}); REQUIRE(planned.has_value());
	// The original physical occupant retains its berth until actual departure; a queued flight cannot push it aside.
	for (const auto &row : *planned) CHECK(row.request.slot == (row.craft == builder.craft ? 0 : UINT8_MAX));
	LoadCheckData preview; preview.native_space_vehicle_rows.push_back({VehicleID{2}, VehicleType::Space, Owner{0}, {}, {1, 2}});
	preview.Clear(); CHECK(preview.native_space_vehicle_rows.empty());
}
