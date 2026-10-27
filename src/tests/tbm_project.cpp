/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_project.cpp Typed project assignment, player controls, command encoding and staged loading.
 * The project is published in the order master Group 6 will use (acquire, reserve, Commissioning pair, publish),
 * but through test scaffolding: production publication stays guarded until then.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 3
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 12 (pause/resume compare against the order-driven baseline)
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../tbm_cmd.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../order_base.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../command_func.h"
#include "../multilayer_state.h"
#include "../misc/endian_buffer.hpp"

#include "../safeguards.h"

/** A project is its own typed record, keyed by its cutter, and is published at most once per cutter and job. */
TEST_CASE("A project is a typed assignment published once per cutter", "[tbm-project]")
{
	TBMProjectRig rig;
	const auto id = rig.project.id;
	CHECK(id == TBMProjectID{0});
	CHECK(rig.project.revision == 1);
	CHECK(GetNextTBMProjectID() == TBMProjectID{1});
	REQUIRE(GetTBMProject(id) != nullptr);
	CHECK(*GetTBMProject(id) == rig.project);
	CHECK(GetTBMProjectForCutter(rig.front->index) == GetTBMProject(id));
	CHECK(GetTBMProjectForCutter(rig.front->Next()->index) == nullptr); // The support part is machinery, not an assignment key.
	CHECK(CaptureTBMProjects() == std::vector<TBMProject>{rig.project});
	CHECK(ValidateTBMProjects(CaptureTBMProjects(), GetNextTBMProjectID()) == TBMProjectError::None);
	CHECK(ValidateTBMProjects(CaptureTBMProjects(), id) == TBMProjectError::Duplicate); // Every ID lies below the cursor.

	auto again = rig.project;
	again.id = TBMProjectID{UINT32_MAX};
	CHECK(PublishTBMProject(again, false) == TBMProjectError::Duplicate);
	CHECK(PublishTBMProject(again, true) == TBMProjectError::Duplicate);
	CHECK(CaptureTBMProjects().size() == 1);

	/**
	 * Validate a changed copy of the published project.
	 * @param change Edit to apply.
	 * @return The validation result.
	 */
	const auto check = [&](const std::function<void(TBMProject &)> &change) {
		auto copy = rig.project;
		change(copy);
		return ValidateTBMProject(copy);
	};
	CHECK(check([](TBMProject &p) { std::swap(p.permanent_parts[0], p.permanent_parts[1]); }) == TBMProjectError::InvalidConsist);
	CHECK(check([&](TBMProject &p) { p.cutter = rig.front->Next()->index; }) == TBMProjectError::MissingCutter);
	CHECK(check([](TBMProject &p) { p.work_remainder = GetTBMCutterModel(0)->work_rate_denominator; }) == TBMProjectError::InvalidProject);
	CHECK(check([](TBMProject &p) { p.phase = static_cast<TBMPhase>(99); }) == TBMProjectError::InvalidProject);
	CHECK(check([](TBMProject &p) { p.heading = Direction::Invalid; }) == TBMProjectError::InvalidProject);
	CHECK(check([&](TBMProject &p) { p.portal = rig.proof.destination; }) == TBMProjectError::MissingPortal);
	CHECK(check([&](TBMProject &p) { p.exchange_bay = rig.proof.destination; }) == TBMProjectError::MissingPortal);
	// A lost reference is accepted only as an explicit invalid ID with its matching recoverable reason.
	CHECK(check([](TBMProject &p) { p.origin = StationID::Invalid(); }) == TBMProjectError::MissingOrigin);
	CHECK(check([](TBMProject &p) { p.origin = StationID::Invalid(); p.blocked = TBMBlockedReason::MissingOrigin; }) == TBMProjectError::None);
	CHECK(check([](TBMProject &p) { p.job = ExcavationJobID{UINT32_MAX}; }) == TBMProjectError::MissingJob);
	CHECK(check([](TBMProject &p) { p.job = ExcavationJobID{UINT32_MAX}; p.blocked = TBMBlockedReason::MissingProject; p.phase = TBMPhase::TravelToFace; }) == TBMProjectError::None);
	// While still Commissioning an unblocked project needs the pair to bind exactly its job and cutter; a blocked one
	// may have lost that binding together with its job.
	CHECK(check([](TBMProject &p) { p.job = ExcavationJobID{UINT32_MAX}; p.blocked = TBMBlockedReason::MissingProject; }) == TBMProjectError::None);
	CHECK(check([](TBMProject &p) { p.phase = TBMPhase::Commissioning; p.job = ExcavationJobID{7}; }) == TBMProjectError::MissingJob); // An ID that resolves to nothing never passes.
}

/** Pause and Resume change only the saved interruption; the phase, orders and every other owner stay untouched. */
TEST_CASE("Project controls pause and resume without touching orders or phase", "[tbm-project]")
{
	TBMProjectRig rig;
	rig.AddTravelOrder();
	const auto id = rig.project.id;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto portal = ComputeLayerPortalWorkDigest();
	const auto start = ComputeTBMProjectDigest();
	// The travel order already handed the project to its orders (FollowOrders, r2tbmf-tbm-fixes.md requirement 8), which
	// is itself a revision; pause and resume must leave that phase alone and add exactly one revision each.
	const auto phase = GetTBMProject(id)->phase;
	const auto revision = GetTBMProject(id)->revision;
	CHECK(Command<Commands::ControlTBMProject>::Do({}, id, TBMProjectAction::Pause).Succeeded());
	CHECK(ComputeTBMProjectDigest() == start); // An estimate changes nothing.
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Pause).Succeeded());
	CHECK(GetTBMProject(id)->blocked == TBMBlockedReason::Paused);
	CHECK(GetTBMProject(id)->phase == phase);
	CHECK(GetTBMProject(id)->revision == revision + 1);
	CHECK(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Pause).Failed());
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Resume).Succeeded());
	CHECK(GetTBMProject(id)->blocked == TBMBlockedReason::None);
	CHECK(GetTBMProject(id)->revision == revision + 2);
	CHECK(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Resume).Failed());
	CHECK(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Retire).Failed()); // Only a finished project retires; cancellation has its own tests (tbm_recovery.cpp).
	CHECK(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::End).Failed());
	CHECK(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, TBMProjectID{7}, TBMProjectAction::Pause).Failed());
	REQUIRE(Company::CanAllocateItem());
	Company *other = Company::Create();
	{
		AutoRestoreBackup actor(_current_company, other->index);
		CHECK(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Pause).Failed()); // Only the owner controls it.
	}
	delete other;
	CHECK(GetTBMProject(id)->revision == revision + 2);
	CHECK(rig.front->GetNumOrders() == 1);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeLayerPortalWorkDigest() == portal);
}

/** Sharing or copying the cutter's travel orders moves no project: assignment is keyed by cutter, not by orders. */
TEST_CASE("Shared or copied travel orders never move a project", "[tbm-project]")
{
	TBMProjectRig rig;
	rig.AddTravelOrder();
	const auto second = TileXYL(30, 20, MapLayer::Surface);
	REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, second, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
	auto request = rig.fixture.request;
	request.portal = second;
	request.proposal = rig.base;
	request.proposal.commissioning_anchor = GetLayerPortalPairTile(second);
	Train *other = nullptr;
	const std::array<const Engine *, 1> wagons{Engine::Get(GetTBMEngine(2))};
	REQUIRE(BuildTBMConsistAtSite(request, Engine::Get(GetTBMEngine(0)), 0, GetFreeUnitNumber(VehicleType::Train), wagons, &other).Succeeded());
	REQUIRE(other != nullptr);
	const auto projects = ComputeTBMProjectDigest();

	REQUIRE(CmdCloneOrder(DoCommandFlag::Execute, CO_SHARE, other->index, rig.front->index).Succeeded());
	CHECK(other->orders == rig.front->orders);
	CHECK(GetTBMProjectForCutter(other->index) == nullptr);
	CHECK(ComputeTBMProjectDigest() == projects);
	CHECK(ValidateTBMProject(*GetTBMProject(rig.project.id)) == TBMProjectError::None);

	REQUIRE(CmdCloneOrder(DoCommandFlag::Execute, CO_UNSHARE, other->index, VehicleID::Invalid()).Succeeded());
	REQUIRE(CmdCloneOrder(DoCommandFlag::Execute, CO_COPY, other->index, rig.front->index).Succeeded());
	CHECK(other->GetNumOrders() == 1);
	CHECK(GetTBMProjectForCutter(other->index) == nullptr);
	CHECK(ComputeTBMProjectDigest() == projects);

	// Pointing the record at the other train does not transfer it: its chain does not hold the saved machinery.
	auto moved = rig.project;
	moved.cutter = other->index;
	CHECK(ValidateTBMProject(moved) == TBMProjectError::InvalidConsist);
}

/** The command's typed arguments survive the ordinary command buffer used for network and replay. */
TEST_CASE("Project command arguments round-trip through the command buffer", "[tbm-project]")
{
	using Args = CommandTraits<Commands::ControlTBMProject>::Args;
	const Args args{TBMProjectID{7}, TBMProjectAction::Resume};
	const auto buffer = EndianBufferWriter<CommandDataBuffer>::FromValue(args);
	CHECK(buffer.size() == 5); // A 32-bit project ID plus a one-byte action; no order or station field is involved.
	CHECK(EndianBufferReader::ToValue<Args>(buffer) == args);
}

/** Loaded projects stay unpublished until validated against rebuilt owners; any refusal leaves the store empty. */
TEST_CASE("Loaded projects publish only after validation against rebuilt owners", "[tbm-project]")
{
	TBMProjectRig rig;
	rig.AddTravelOrder();
	const auto saved = CaptureTBMProjects();
	const auto next = GetNextTBMProjectID();
	const auto digest = ComputeTBMProjectDigest();
	const auto flags = CaptureMultiLayerStateSchema().feature_flags;
	CHECK((flags & MULTILAYER_FEATURE_TBM_PROJECTS) != 0);
	CHECK((flags & MULTILAYER_FEATURE_EXCAVATION_JOBS) != 0);
	auto schema = CaptureMultiLayerStateSchema();
	schema.feature_flags &= ~MULTILAYER_FEATURE_EXCAVATION_JOBS;
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::UnsupportedFeatures); // Project history implies job history.

	/**
	 * Reset the store, stage a loaded set and try to publish it.
	 * @param projects Loaded rows.
	 * @param cursor Loaded allocation cursor.
	 * @return Whether the set was accepted.
	 */
	const auto load = [](std::vector<TBMProject> projects, TBMProjectID cursor) {
		ResetTBMProjectState();
		StageLoadedTBMProjects(std::move(projects), cursor);
		CHECK(CaptureTBMProjects().empty()); // Staged rows are not live until accepted.
		return AcceptLoadedTBMProjects();
	};

	SECTION("an intact set publishes unchanged, and orders are untouched") {
		REQUIRE(load(saved, next));
		CHECK(ComputeTBMProjectDigest() == digest);
		CHECK(rig.front->GetNumOrders() == 1);
		CHECK(AcceptLoadedTBMProjects()); // Nothing staged: the store stays as it is.
		CHECK(ComputeTBMProjectDigest() == digest);
	}
	SECTION("an ID at the cursor refuses") {
		CHECK_FALSE(load(saved, saved.back().id));
		CHECK(CaptureTBMProjects().empty());
	}
	SECTION("two projects for one cutter refuse") {
		auto twice = saved;
		twice.push_back(saved.back());
		twice.back().id = TBMProjectID{next.base()};
		CHECK_FALSE(load(twice, TBMProjectID{next.base() + 1}));
		CHECK(CaptureTBMProjects().empty());
	}
	SECTION("machinery that no longer matches loads as a recoverable block") {
		auto tampered = saved;
		std::swap(tampered.back().permanent_parts[0], tampered.back().permanent_parts[1]);
		CHECK(load(tampered, next));
		CHECK(GetTBMProject(tampered.back().id)->blocked == TBMBlockedReason::InvalidConsist);
	}
	SECTION("an unknown blocked reason refuses") {
		auto broken = saved;
		broken.back().blocked = TBMBlockedReason::End;
		CHECK_FALSE(load(broken, next));
		CHECK(CaptureTBMProjects().empty());
	}
	SECTION("a lost origin loads as a recoverable block") {
		auto lost = saved;
		lost.back().origin = StationID::Invalid();
		lost.back().blocked = TBMBlockedReason::MissingOrigin;
		CHECK(load(lost, next));
		CHECK(GetTBMProject(lost.back().id)->blocked == TBMBlockedReason::MissingOrigin);
	}
	SECTION("deleting the cutter's orders leaves the project unchanged") {
		DeleteVehicleOrders(rig.front);
		CHECK(rig.front->GetNumOrders() == 0);
		CHECK(ComputeTBMProjectDigest() == digest);
	}
}
