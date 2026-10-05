/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange_consumer.cpp The published exchange producer (Plan 5 Group 5): the exact public signatures of
 * tbm_exchange.h, their invalid-ID semantics, and the direct Plan 4 consumer, whose exchange edges (task 4.2B) read the
 * bay admission and the actual wagon contents.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 5 (Public Handoff Contract)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, task 4.2B
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"

#include "../safeguards.h"

/* The Public Handoff Contract symbols, with the exact signatures consumers compile against. */
static_assert(std::is_same_v<decltype(&ValidateTBMWagonExchange), TBMExchangeProposal (*)(const TBMExchangeRequest &)>);
static_assert(std::is_same_v<decltype(&ExecuteTBMWagonExchange), TBMExchangeProposal (*)(const TBMExchangeRequest &)>);
static_assert(std::is_same_v<decltype(&GetTBMExchangeAssignment), const TBMExchangeAssignment *(*)(TBMProjectID)>);
static_assert(std::is_same_v<decltype(TBMExchangeAssignment::project), TBMProjectID>);
static_assert(std::is_same_v<decltype(TBMExchangeAssignment::hauler), VehicleID>);
static_assert(std::is_same_v<decltype(TBMExchangeAssignment::set), TBMWagonSetID>);
static_assert(std::is_same_v<decltype(TBMWagonSet::wagons), std::vector<VehicleID>>);
static_assert(std::is_same_v<decltype(TBMExchangeProposal::loaded_units), std::vector<VehicleID>>);

/** Invalid IDs refuse with explicit reasons and never touch a train. */
TEST_CASE("Exchange producer refuses invalid identities explicitly", "[tbm-exchange-consumer]")
{
	TBMExchangeRig rig;
	const auto vehicles = Vehicle::GetNumItems();
	CHECK(ValidateTBMWagonExchange({TBMProjectID{UINT32_MAX}, rig.hauler->index}).error == TBMExchangeError::InvalidProject);
	CHECK(ExecuteTBMWagonExchange({TBMProjectID{77}, rig.hauler->index}).error == TBMExchangeError::InvalidProject);
	CHECK(ExecuteTBMWagonExchange({rig.project.id, VehicleID::Invalid()}).error == TBMExchangeError::NotAssigned);
	CHECK(GetTBMExchangeAssignment(TBMProjectID{UINT32_MAX}) == nullptr);
	CHECK(GetTBMWagonSet(TBMWagonSetID{UINT32_MAX}) == nullptr);
	CHECK(GetTBMWagonSetForWagon(VehicleID::Invalid()) == nullptr);
	CHECK(ReleaseTBMHauler(VehicleID::Invalid(), true) == TBMExchangeError::NotAssigned);
	CHECK(AdvanceTBMHauler(TBMExchangeAssignmentID{UINT32_MAX}) == TBMExchangeError::NotAssigned);
	CHECK(GetTBMExchangeAssignmentForHauler(VehicleID::Invalid()) == nullptr);
	CHECK(GetTBMExchangeAssignmentAtBay(INVALID_TILE) == nullptr);
	CHECK(ResolveTBMExchangeBay(rig.Project(), INVALID_TILE).error == TBMExchangeError::InvalidBay);
	CHECK(AssignTBMHauler(rig.project.id, VehicleID::Invalid(), true) == TBMExchangeError::InvalidHauler);
	CHECK(Command<Commands::ExchangeTBMWagons>::Do(DoCommandFlag::Execute, rig.project.id, rig.hauler->index).Failed()); // No assignment.
	CHECK(Vehicle::GetNumItems() == vehicles);
	// A released project leaves no assignment that a later project could inherit.
	rig.Assign();
	ResetTBMProjectState();
	CHECK(CaptureTBMExchangeAssignments().empty());
	CHECK(CaptureTBMWagonSets().empty());
}

/** The Plan 4 consumer: the exchange edges of the pure phase step (task 4.2B). */
TEST_CASE("Exchange edges of the controller step follow the bay and the actual wagons", "[tbm-exchange-consumer]")
{
	TBMProjectRig rig;
	const auto digest = ComputeTBMProjectDigest();
	auto project = rig.project;
	project.phase = TBMPhase::ReturnForExchange;
	TBMPhaseInputs inputs;
	inputs.exchange_due = true;
	CHECK(StepTBMProjectPhase(project, inputs).phase == TBMPhase::ReturnForExchange); // Not admitted at the bay yet.
	inputs.at_exchange = true;
	CHECK(StepTBMProjectPhase(project, inputs).phase == TBMPhase::WaitForEmpties);

	project.phase = TBMPhase::WaitForEmpties;
	const auto held = StepTBMProjectPhase(project, inputs);
	CHECK(held.phase == TBMPhase::WaitForEmpties); // Spoil still aboard.
	CHECK_FALSE(held.phase_changed);
	inputs.exchange_due = false;
	CHECK(StepTBMProjectPhase(project, inputs).phase == TBMPhase::Resume);

	project.phase = TBMPhase::Resume;
	CHECK(StepTBMProjectPhase(project, {}).phase == TBMPhase::TravelToFace);
	TBMPhaseInputs done;
	done.route_complete = true;
	CHECK(StepTBMProjectPhase(project, done).phase == TBMPhase::Completed);
	// Conditions still hold these phases like any other.
	TBMPhaseInputs stopped;
	stopped.train_stopped = true;
	stopped.at_exchange = true;
	project.phase = TBMPhase::ReturnForExchange;
	CHECK(StepTBMProjectPhase(project, stopped).blocked == TBMBlockedReason::Stopped);
	CHECK(ComputeTBMProjectDigest() == digest);

	// The observed input comes from the real bay admission.
	CHECK(ObserveTBMPhaseInputs(*GetTBMProject(rig.project.id)).at_exchange); // The cutter consist was assembled in its source bay.
	rig.front->cur_speed = 3;
	CHECK_FALSE(ObserveTBMPhaseInputs(*GetTBMProject(rig.project.id)).at_exchange);
}
