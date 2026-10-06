/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_work.cpp Face work against the one reserved face (Group 6) and the atomic paid cut into real wagons
 * (Group 7): bounded work per tick, precise waits that keep the cursor, exact fill, one-cargo fullness, allocation
 * refusal and saves on both sides of a cut.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 6 and 7
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../rail_map.h"
#include "../track_func.h"

#include "../safeguards.h"

/**
 * Run the face up to the last work tick before its cut; every earlier tick must work without a wait.
 * @param rig Work rig in Bore.
 * @param estimate The cut's estimate.
 */
static void WorkToLastTick(const TBMWorkRig &rig, const ExcavationCutResult &estimate)
{
	// The standard cutter works one tick per simulation tick, so the job's partial work counts the ticks exactly.
	for (uint64_t tick = 1; tick < estimate.work_ticks; ++tick) {
		const auto &project = rig.Tick();
		REQUIRE(project.phase == TBMPhase::Bore);
		REQUIRE(project.blocked == TBMBlockedReason::None);
		REQUIRE(rig.Job().partial_work_ticks == tick);
	}
}

/** @return The company's current cash. */
static Money GetCash()
{
	return Company::Get(_current_company)->money;
}

/** Work accrues once per tick up to the requirement; the boundary tick commits one paid cut into the real wagons. */
TEST_CASE("Face work adds bounded work per tick and commits one paid cut", "[tbm-work]")
{
	AutoRestoreBackup<decltype(_settings_game.vehicle.freight_trains)> freight_weight(_settings_game.vehicle.freight_trains, 1);
	TBMWorkRig rig;
	// Real package spoil has mass; zero-weight transaction fixtures cannot reveal a stale physical cache after a paid cut.
	for (CargoLabel label : {CargoLabel{"ERTH"}, CargoLabel{"ROCK"}}) {
		CargoSpec::Get(GetCargoTypeByLabel(label))->weight = 16;
		CargoSpec::Get(GetCargoTypeByLabel(label))->is_freight = true;
	}
	const auto empty_weight = rig.front->gcache.cached_weight;
	const auto empty_acceleration = rig.front->acceleration;
	const auto estimate = EstimateExcavationCut(rig.project.job);
	REQUIRE(estimate.error == ExcavationError::None);
	REQUIRE(estimate.new_cells == 12); // The solid start opens the whole pocket and the first cell beyond the door.
	REQUIRE(estimate.work_ticks > 1);
	const Money cash = GetCash();
	CHECK(IsTBMHoldingTrain(*rig.front)); // A cutter in Bore stays still under the ordinary train controller.

	WorkToLastTick(rig, estimate);
	CHECK(rig.Job().cursor == 0);
	CHECK(GetCash() == cash);
	CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == 0);
	CHECK(rig.Spoil(TBMConsistRole::RockWagon) == 0);
	CHECK(CaptureTBMSupport().empty());

	/* The tick that completes the work commits the cut in the same step; the route's only cut makes the final
	 * exchange due at once, so the consist goes back instead of onto its new support. */
	const TileIndex cell = rig.Job().route[0].cutter_position;
	CHECK(rig.Tick().phase == TBMPhase::ReturnForExchange);
	CHECK(rig.Project().blocked == TBMBlockedReason::None);
	CHECK(rig.Job().cursor == 1);
	CHECK(rig.Job().partial_work_ticks == 0);
	CHECK(rig.Job().completed_earth_t == estimate.earth_t);
	CHECK(rig.Job().completed_rock_t == estimate.rock_t);
	CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == estimate.earth_t);
	CHECK(rig.Spoil(TBMConsistRole::RockWagon) == estimate.rock_t);
	CHECK(rig.front->gcache.cached_weight > empty_weight);
	CHECK(rig.front->acceleration < empty_acceleration);
	CHECK(GetCash() == cash - estimate.cost);
	// Every swept cell is open now, and the new cutter cell carries the axis support track out of the door.
	for (TileIndex tile : rig.Job().route[0].swept_tiles) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
	REQUIRE(IsPlainRailTile(cell));
	CHECK(GetTrackBits(cell) == TrackBits{DiagDirToDiagTrack(GetLayerPortalDoorDirection(rig.proof.destination))});
	REQUIRE(GetTBMSupportTile(cell) != nullptr);
	CHECK(GetTBMSupportTile(cell)->project == rig.project.id);

	/* Stable order: each role fills its first wagon before the next one. */
	for (const auto role : {TBMConsistRole::EarthWagon, TBMConsistRole::RockWagon}) {
		const auto wagons = rig.Wagons(role);
		REQUIRE(wagons.size() == 2);
		const uint total = role == TBMConsistRole::EarthWagon ? static_cast<uint>(estimate.earth_t) : static_cast<uint>(estimate.rock_t);
		CHECK(wagons[0]->cargo.TotalCount() == std::min<uint>(wagons[0]->cargo_cap, total));
		CHECK(wagons[1]->cargo.TotalCount() == total - wagons[0]->cargo.TotalCount());
		// Canonical provenance: no source industry, no first station, fresh age, loaded at the work site.
		for (const Train *wagon : wagons) {
			for (const CargoPacket *packet : *wagon->cargo.Packets()) {
				CHECK_FALSE(packet->GetSource().IsValid());
				CHECK(packet->GetFirstStation() == StationID::Invalid());
				CHECK(packet->GetPeriodsInTransit() == 0);
				CHECK(packet->GetFeederShare() == 0);
				CHECK(packet->GetDistance(cell) == 0);
			}
		}
	}

	/* The only cut is done: the exchange is due, and no second cut is ever attempted. */
	CHECK(IsTBMExchangeDue(rig.Project()));
	CHECK(rig.Tick().phase == TBMPhase::WaitForEmpties); // The consist already stands in its Source bay.
	CHECK(rig.Job().cursor == 1);
	CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == estimate.earth_t);
}

/** At the boundary a missing resource names a precise wait, keeps work and cursor, banks nothing and resumes. */
TEST_CASE("Face work waits precisely at the boundary and resumes from the same cursor", "[tbm-work]")
{
	TBMWorkRig rig;
	const auto estimate = EstimateExcavationCut(rig.project.job);
	REQUIRE(estimate.error == ExcavationError::None);
	const auto earth = rig.Wagons(TBMConsistRole::EarthWagon);
	const auto rock = rig.Wagons(TBMConsistRole::RockWagon);
	const uint earth_capacity = earth[0]->cargo_cap + earth[1]->cargo_cap;
	const uint rock_capacity = rock[0]->cargo_cap + rock[1]->cargo_cap;
	REQUIRE(earth_capacity - estimate.earth_t + 1 <= earth[0]->cargo_cap);
	REQUIRE(rock_capacity - estimate.rock_t + 1 <= rock[0]->cargo_cap);

	/**
	 * Run to the boundary, require the wait to hold, then clear it and require the cut.
	 * @param expected Wait the boundary must report.
	 * @param clear Change that removes the limit.
	 */
	const auto check_wait = [&](TBMBlockedReason expected, const std::function<void()> &clear) {
		const Money cash = GetCash();
		const uint earth_before = rig.Spoil(TBMConsistRole::EarthWagon);
		const uint rock_before = rig.Spoil(TBMConsistRole::RockWagon);
		WorkToLastTick(rig, estimate);
		CHECK(rig.Tick().blocked == expected);
		// Held work stays at the requirement; further ticks neither bank work nor change wagons, cash or terrain.
		const auto idle = rig.Project().idle_ticks;
		const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
		for (int tick = 0; tick < 3; ++tick) {
			CHECK(rig.Tick().blocked == expected);
			CHECK(rig.Project().phase == TBMPhase::Bore);
		}
		CHECK(rig.Project().idle_ticks == idle + 3);
		CHECK(rig.Job().partial_work_ticks == estimate.work_ticks);
		CHECK(rig.Job().cursor == 0);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		CHECK(GetCash() == cash);
		CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == earth_before);
		CHECK(rig.Spoil(TBMConsistRole::RockWagon) == rock_before);
		clear();
		CHECK(rig.Tick().phase == TBMPhase::ReturnForExchange); // The only cut commits, and its final exchange is due.
		CHECK(rig.Job().cursor == 1);
	};

	SECTION("Earth full while Rock is free; freeing one tonne fills the Earth wagons exactly") {
		rig.Preload(earth[0], static_cast<uint16_t>(earth_capacity - estimate.earth_t + 1));
		check_wait(TBMBlockedReason::EarthFull, [&]() { earth[0]->cargo.Truncate(1); });
		CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == earth_capacity);
		CHECK(rig.Spoil(TBMConsistRole::RockWagon) == estimate.rock_t);
	}
	SECTION("Rock full while Earth is free; freeing one tonne fills the Rock wagons exactly") {
		rig.Preload(rock[0], static_cast<uint16_t>(rock_capacity - estimate.rock_t + 1));
		check_wait(TBMBlockedReason::RockFull, [&]() { rock[0]->cargo.Truncate(1); });
		CHECK(rig.Spoil(TBMConsistRole::RockWagon) == rock_capacity);
		CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == estimate.earth_t);
	}
	SECTION("too little cash waits for funds") {
		const Money cash = GetCash();
		Company::Get(_current_company)->money = estimate.cost - 1;
		check_wait(TBMBlockedReason::InsufficientFunds, [&]() { Company::Get(_current_company)->money = cash; });
		CHECK(GetCash() == cash - estimate.cost);
	}
}

/** A participant that refuses inside the transaction changes nothing, and geology then commits nothing either. */
TEST_CASE("A refused participant leaves geology, wagons and funds unchanged", "[tbm-work]")
{
	TBMWorkRig rig;
	const auto estimate = EstimateExcavationCut(rig.project.job);
	REQUIRE(estimate.error == ExcavationError::None);
	WorkToLastTick(rig, estimate);
	// Finish the work through the geology owner directly, so the cut is due but not yet attempted.
	TBMCutParticipant live(rig.Project());
	REQUIRE(AdvanceExcavationWork(rig.project.job, 1, live) == ExcavationError::None);
	REQUIRE(rig.Job().partial_work_ticks == estimate.work_ticks);
	const ExcavationCutRequest request{rig.project.job, rig.Job().revision, rig.Job().cursor};
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const Money cash = GetCash();
	const auto packets = CargoPacket::GetNumItems();
	const auto infrastructure = Company::Get(_current_company)->infrastructure.rail[RAILTYPE_RAIL];

	/**
	 * Require that nothing outside geology changed.
	 */
	const auto unchanged = [&]() {
		CHECK(GetCash() == cash);
		CHECK(CargoPacket::GetNumItems() == packets);
		CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == 0);
		CHECK(rig.Spoil(TBMConsistRole::RockWagon) == 0);
	};

	SECTION("cargo packet storage cannot be allocated") {
		TBMCutParticipant starved(rig.Project(), 0);
		const auto receipt = CommitExcavationCut(request, starved);
		CHECK(receipt.error == ExcavationError::StaleParticipant);
		CHECK(starved.GetRefusal() == TBMBlockedReason::AllocationFailure);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		CHECK(CaptureTBMSupport().empty());
		CHECK(Company::Get(_current_company)->infrastructure.rail[RAILTYPE_RAIL] == infrastructure);
		unchanged();
		// The same cut then commits with real storage, from the same cursor.
		CHECK(CommitExcavationCut(request, live).error == ExcavationError::None);
		CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == estimate.earth_t);
	}
	SECTION("support placement is obstructed after the quote") {
		const auto material = live.Capture();
		const auto quote = PrepareExcavationCut(request, material);
		REQUIRE(quote.error == ExcavationError::None);
		// Foreign track (not this project's registered support, which would be reused) now occupies a support cell;
		// the train's side must refuse before any packet or cash write.
		const auto plan = PrepareTBMSupport(rig.Project(), rig.Job());
		REQUIRE(plan.error == TBMBlockedReason::None);
		REQUIRE_FALSE(plan.pieces.empty());
		MakeRailNormal(plan.pieces.front().tile, _current_company, TrackBits{plan.pieces.front().track}, plan.railtype);
		CHECK_FALSE(live.Commit(material, quote));
		CHECK(live.GetRefusal() == TBMBlockedReason::Obstruction);
		unchanged();
		// Geology sees the occupied work cell too and keeps the cut uncommitted.
		CHECK(CommitExcavationCut(request, live).error != ExcavationError::None);
		CHECK(rig.Job().cursor == 0);
		CHECK(rig.Job().partial_work_ticks == estimate.work_ticks);
		unchanged();
	}
}

/** Only the assigned project advances its claim, and its retreat needs real support on every completed cell. */
TEST_CASE("Only the assigned project advances its claim over supported retreat", "[tbm-work]")
{
	TBMWorkRig rig(2);
	const ExcavationJob job = rig.Job();
	REQUIRE(job.route.size() == 2);
	TBMCutParticipant participant(rig.Project());
	CHECK(participant.IsSupported(job, job.route[0]));
	CHECK(participant.GetRefusal() == TBMBlockedReason::None);

	/**
	 * @param change Edit to a copy of the job.
	 * @return The refusal for the changed job.
	 */
	const auto refuse_job = [&](const std::function<void(ExcavationJob &)> &change) {
		ExcavationJob copy = job;
		change(copy);
		CHECK_FALSE(participant.IsSupported(copy, copy.route[std::min<size_t>(copy.cursor, copy.route.size() - 1)]));
		return participant.GetRefusal();
	};
	CHECK(refuse_job([](ExcavationJob &j) { j.id = ExcavationJobID{j.id.base() + 1}; }) == TBMBlockedReason::MissingProject);
	CHECK(refuse_job([&](ExcavationJob &j) { j.cutter = rig.front->Next()->index; }) == TBMBlockedReason::MissingProject);
	CHECK(refuse_job([](ExcavationJob &j) { j.company = CompanyID{static_cast<uint8_t>(j.company.base() + 1)}; }) == TBMBlockedReason::MissingProject);
	// Before the first cut the next cell has no rail, so a retreat over it is unsupported.
	CHECK(refuse_job([](ExcavationJob &j) { j.cursor = 1; }) == TBMBlockedReason::MissingSupport);

	// A copy of the project under another identity is not the store's assignment for this cutter.
	auto other = rig.Project();
	other.id = TBMProjectID{other.id.base() + 1};
	TBMCutParticipant impostor(other);
	CHECK_FALSE(impostor.IsSupported(job, job.route[0]));
	CHECK(impostor.GetRefusal() == TBMBlockedReason::MissingProject);

	/* After the first real cut, the completed cell carries support and the second cut's retreat is supported. */
	const auto estimate = EstimateExcavationCut(rig.project.job);
	WorkToLastTick(rig, estimate);
	REQUIRE(rig.Tick().phase == TBMPhase::AdvanceSupport); // Both cuts fit the wagons, so the first is no exchange breakpoint.
	REQUIRE(rig.Job().cursor == 1);
	TBMCutParticipant after(rig.Project());
	CHECK(after.IsSupported(rig.Job(), rig.Job().route[1]));
	CHECK(IsTBMSupportUsable(rig.Project(), rig.Job().route[0].cutter_position));
	CHECK(IsTBMSupportUsable(rig.Project(), rig.proof.destination));
	CHECK_FALSE(IsTBMSupportUsable(rig.Project(), rig.Job().route[1].cutter_position)); // Still solid rock.
}

/** Saves just before and just after a cut reload to the same state and the same next step. */
TEST_CASE("Saves on both sides of a cut reload to the same next step", "[tbm-work]")
{
	TBMWorkRig rig;
	const auto estimate = EstimateExcavationCut(rig.project.job);
	REQUIRE(estimate.error == ExcavationError::None);
	WorkToLastTick(rig, estimate);
	auto projects = ComputeTBMProjectDigest();
	auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	rig.Reload();
	CHECK(ComputeTBMProjectDigest() == projects);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(rig.Tick().phase == TBMPhase::ReturnForExchange); // The reloaded face commits on exactly the same tick.

	projects = ComputeTBMProjectDigest();
	geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto support = CaptureTBMSupport();
	REQUIRE(support.size() == 1);
	rig.Reload();
	CHECK(ComputeTBMProjectDigest() == projects);
	CHECK(CaptureTBMSupport() == support);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == estimate.earth_t);

	SECTION("support metadata that no longer matches the track refuses") {
		auto tampered = support;
		// The laid piece is an axis track, so a corner piece cannot be on the tile.
		tampered.back().track = TrackBits{Track::Upper};
		const auto saved = CaptureTBMProjects();
		const auto next = GetNextTBMProjectID();
		ResetTBMProjectState();
		StageLoadedTBMProjects(saved, next);
		StageLoadedTBMSupport(tampered);
		CHECK_FALSE(AcceptLoadedTBMProjects());
		CHECK(CaptureTBMProjects().empty());
		CHECK(CaptureTBMSupport().empty());
	}
	SECTION("support rows without any project refuse") {
		ResetTBMProjectState();
		StageLoadedTBMSupport(support);
		CHECK_FALSE(AcceptLoadedTBMProjects());
	}
}
