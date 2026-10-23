/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file tbm_merge_observation.cpp Native reference, phase-identity and running-corridor observations.
	* @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md paid work and real train ownership.
	* @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group20.
	* Train placement is an explicit component input; these read-only queries activate no steering or saved face phase.
	*/
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"
#include "../safeguards.h"

/** The appended observation identity cannot renumber any original saved phase. */
TEST_CASE("TBM face observation appends its phase after every original native identity", "[tbm-merge-observation]")
{
	const std::array original{TBMPhase::Commissioning, TBMPhase::TravelToFace, TBMPhase::Bore, TBMPhase::AdvanceSupport,
	 TBMPhase::ReturnForExchange, TBMPhase::WaitForEmpties, TBMPhase::Resume, TBMPhase::Completed, TBMPhase::Cancelled,
	 TBMPhase::ReturnToPark, TBMPhase::WaitToPark, TBMPhase::Parked, TBMPhase::ReturnToRelease, TBMPhase::WaitToRelease,
	 TBMPhase::ReleasePending, TBMPhase::ReturnForHangar, TBMPhase::WaitForHangar, TBMPhase::TravelToHangar, TBMPhase::ParkedHangar, TBMPhase::FollowOrders};
	// Original0-19 are independent serialized ABI expectations. The new pure
	// predicate must not reinterpret any of them as a face wait.
	for (size_t i = 0; i < original.size(); ++i) { CHECK(to_underlying(original[i]) == i); CHECK_FALSE(IsTBMAwaitingFaceTransfer(original[i])); }
	CHECK(to_underlying(TBMPhase::AwaitFaceTransfer) == 20); CHECK(IsTBMAwaitingFaceTransfer(TBMPhase::AwaitFaceTransfer));
}

/** Exposing the original predicates shares exactly the current company/model/binding contract rather than loosening it for face geometry. */
TEST_CASE("TBM shared live reference observations refuse missing foreign and wrong-model inputs unchanged", "[tbm-merge-observation]")
{
	TBMExchangeRig rig(4); const auto before = rig.Digest(); const auto project = rig.Project();
	CHECK(GetLiveTBMCutter(project) == rig.front); CHECK(GetLiveTBMJob(project) == &rig.Job());
	auto foreign = project; foreign.company = CompanyID::Invalid(); CHECK(GetLiveTBMCutter(foreign) == nullptr); CHECK(GetLiveTBMJob(foreign) == nullptr);
	auto wrong_model = project; wrong_model.model_id = UINT16_MAX; CHECK(GetLiveTBMCutter(wrong_model) == nullptr); CHECK(GetLiveTBMJob(wrong_model) == nullptr);
	auto no_cutter = project; no_cutter.cutter = VehicleID::Invalid(); CHECK(GetLiveTBMCutter(no_cutter) == nullptr);
	// An original unbound geological job remains valid independently of a
	// missing cutter; callers must consume both predicates before authorizing work.
	REQUIRE(rig.Job().cutter == VehicleID::Invalid()); CHECK(GetLiveTBMJob(no_cutter) == &rig.Job());
	auto no_job = project; no_job.job = ExcavationJobID{UINT32_MAX}; CHECK(GetLiveTBMJob(no_job) == nullptr);
	CHECK(rig.Digest() == before); CHECK(rig.Project().phase == project.phase);
}

/** Actual assignment membership, physical running parts and native consist lengths alone define the prerequisite corridor facts. */
TEST_CASE("TBM corridor observation excludes stored and unassigned trains and selects the lowest running occupant", "[tbm-merge-observation]")
{
	TBMExchangeRig rig(4); const auto &project = rig.Project(); const auto key = rig.fixture.request.portal; const auto housing = GetLayerPortalPairTile(key);
	CHECK(GetTBMHaulerRoomCells(project.id) == 0); CHECK_FALSE(IsTBMHaulerInCorridor(project, rig.hauler->index));
	CHECK(FindTBMHaulerInCorridor(project) == VehicleID::Invalid()); rig.Assign();
	const uint first_cells = (rig.hauler->gcache.cached_total_length + TILE_SIZE - 1) / TILE_SIZE;
	CHECK(GetTBMHaulerRoomCells(project.id) == first_cells); CHECK_FALSE(IsTBMHaulerInCorridor(project, rig.hauler->index));
	auto *second = rig.BuildHauler(rig.depot, 3, 3); REQUIRE(AssignTBMHauler(project.id, second->index, true) == TBMExchangeError::None);
	const uint second_cells = (second->gcache.cached_total_length + TILE_SIZE - 1) / TILE_SIZE; REQUIRE(second_cells > first_cells);
	CHECK(GetTBMHaulerRoomCells(project.id) == second_cells);
	auto *tail = second->Last(); tail->tile = housing; tail->track = Track::X; tail->UpdatePosition();
	CHECK(IsTBMHaulerInCorridor(project, second->index)); CHECK(FindTBMHaulerInCorridor(project) == second->index);
	rig.hauler->tile = key; rig.hauler->track = Track::X; rig.hauler->UpdatePosition();
	const auto before = rig.Digest(); const auto assignments = CaptureTBMExchangeAssignments();
	const auto lowest = std::min(rig.hauler->index, second->index);
	CHECK(IsTBMHaulerInCorridor(project, rig.hauler->index)); CHECK(FindTBMHaulerInCorridor(project) == lowest);
	CHECK(rig.Digest() == before); CHECK(CaptureTBMExchangeAssignments() == assignments);
	rig.hauler->track = Track::Depot; CHECK_FALSE(IsTBMHaulerInCorridor(project, rig.hauler->index)); CHECK(FindTBMHaulerInCorridor(project) == second->index);
	tail->track = Track::Depot; CHECK_FALSE(IsTBMHaulerInCorridor(project, second->index)); CHECK(FindTBMHaulerInCorridor(project) == VehicleID::Invalid());
	auto another_project = project; another_project.id = TBMProjectID{UINT32_MAX}; CHECK_FALSE(IsTBMHaulerInCorridor(another_project, second->index));
	CHECK(GetTBMHaulerRoomCells(another_project.id) == 0); CHECK_FALSE(IsTBMHaulerInCorridor(project, VehicleID::Invalid()));
}
