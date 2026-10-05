/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_route.cpp Canonical passes bound to the actual train (Group 10): the saved cutting heading follows only
 * committed advances, sharp or stationary steps wait, the swept envelope is revalidated against other vehicles, and
 * turn-space cells are charged once as ordinary spoil-producing work. Group 13: backing never turns the cutter.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 10 and 13
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../train_cmd.h"
#include "../direction_func.h"

#include "../safeguards.h"

/** @return The company's current cash. */
static Money GetCash()
{
	return Company::Get(_current_company)->money;
}

/** Through a 90-degree change made of two 45-degree advances every swept cell, turn space included, is charged once. */
TEST_CASE("Turn-space cuts are charged once as ordinary spoil-producing work", "[tbm-route]")
{
	TBMWorkRig rig({DirDiff::Same, DirDiff::Right45, DirDiff::Right45, DirDiff::Same});
	const ExcavationJob planned = rig.Job();
	REQUIRE(planned.route.size() == 4);
	std::set<TileIndex> swept;
	for (const auto &cut : planned.route) swept.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
	// Turn space lies beyond the requested floor but inside the accepted area, so cutting it is never implicit.
	std::vector<TileIndex> turn_space;
	std::ranges::set_difference(swept, planned.requested_tiles, std::back_inserter(turn_space));
	REQUIRE_FALSE(turn_space.empty());
	for (TileIndex tile : swept) CHECK(std::ranges::binary_search(planned.authorized_tiles, tile));

	const Money cash = GetCash();
	uint64_t cells = 0, earth = 0, rock = 0;
	Money cost = 0;
	// Each cut is one ordinary paid transaction; the saved heading follows the advance it committed.
	for (size_t index = 0; index < planned.route.size(); ++index) {
		if (index != 0) rig.ExchangeAndReturn();
		const auto estimate = rig.BoreOneCut();
		cells += estimate.new_cells;
		earth += estimate.earth_t;
		rock += estimate.rock_t;
		cost += estimate.cost;
		CHECK(rig.Project().heading == planned.route[index].heading);
	}
	CHECK(cells == swept.size());
	CHECK(rig.Job().completed_earth_t == earth);
	CHECK(rig.Job().completed_rock_t == rock);
	CHECK(rig.Job().completed_work_cost == cost);
	CHECK(GetCash() == cash - cost);
	CHECK(rig.Job().state == ExcavationJobState::Completed);
	for (TileIndex tile : swept) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
}

/** A segment more than 45 degrees off the saved heading waits without working; repeating it turns nothing. */
TEST_CASE("Sharp or repeated steps cannot turn the cutter", "[tbm-route]")
{
	TBMWorkRig rig;
	const Direction sharp = ChangeDir(rig.proof.exit_heading, DirDiff::Right90);
	rig.Reload(std::nullopt, [sharp](TBMProject &project) { project.heading = sharp; });
	// The waiting face neither works nor rotates, however often it is stepped.
	for (int tick = 0; tick < 4; ++tick) {
		CHECK(rig.Tick().blocked == TBMBlockedReason::IllegalTurn);
		CHECK(rig.Project().phase == TBMPhase::Bore);
	}
	CHECK(rig.Job().partial_work_ticks == 0);
	CHECK(rig.Project().heading == sharp);
	// Forty-five degrees off the saved heading is a legal advance.
	const Direction slight = ChangeDir(rig.proof.exit_heading, DirDiff::Left45);
	rig.Reload(std::nullopt, [slight](TBMProject &project) { project.heading = slight; });
	CHECK(rig.Tick().blocked == TBMBlockedReason::None);
	CHECK(rig.Job().partial_work_ticks == 1);
}

/** Another train standing in the swept envelope keeps the due cut uncommitted until it leaves. */
TEST_CASE("Another vehicle in the swept envelope keeps the cut uncommitted", "[tbm-route]")
{
	TBMWorkRig rig;
	// A second consist on another pair: same company and model, but not this project's train.
	const auto second = TileXYL(30, 20, MapLayer::Surface);
	REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, second, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
	auto request = rig.fixture.request;
	request.portal = second;
	request.proposal = rig.base;
	request.proposal.commissioning_anchor = GetLayerPortalPairTile(second);
	Train *other = nullptr;
	const std::array<const Engine *, 1> wagons{Engine::Get(GetTBMEngine(2))};
	REQUIRE(BuildTBMConsistAtSite(request, Engine::Get(GetTBMEngine(0)), 0, GetFreeUnitNumber(VehicleType::Train), wagons, &other).Succeeded());
	Train *intruder = other->Last();
	const TileIndex home = intruder->tile;

	const auto estimate = EstimateExcavationCut(rig.project.job);
	REQUIRE(estimate.error == ExcavationError::None);
	for (uint64_t tick = 1; tick < estimate.work_ticks; ++tick) REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	const Money cash = GetCash();
	// The intruder stands in the envelope when the cut becomes due.
	intruder->tile = rig.Job().route[0].swept_tiles.front();
	intruder->UpdatePosition();
	CHECK(rig.Tick().blocked == TBMBlockedReason::Obstruction);
	CHECK(rig.Job().cursor == 0);
	CHECK(rig.Job().partial_work_ticks == estimate.work_ticks);
	CHECK(GetCash() == cash);
	CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == 0);
	// Once it has left, the same cut commits on the next tick, and its final exchange is due.
	intruder->tile = home;
	intruder->UpdatePosition();
	CHECK(rig.Tick().phase == TBMPhase::ReturnForExchange);
	CHECK(rig.Job().cursor == 1);
}

/** Backing reverses the train, never the cutter: a reversed facing cannot cut backwards or skip the 45-degree rule. */
TEST_CASE("Backing never turns the cutter or enables backwards cuts", "[tbm-backing]")
{
	TBMWorkRig rig;
	const Direction heading = rig.proof.exit_heading;
	// The train's own facing is train state; turning every part round leaves the saved heading and the forward cut.
	for (Train *part = rig.front; part != nullptr; part = part->Next()) part->direction = ReverseDir(part->direction);
	CHECK(rig.Tick().blocked == TBMBlockedReason::None);
	CHECK(rig.Project().heading == heading);
	CHECK(rig.Job().partial_work_ticks == 1);
	// A saved heading pointing backwards, as if backing had turned the cutter, is refused rather than cut.
	rig.Reload(std::nullopt, [](TBMProject &project) { project.heading = ReverseDir(project.heading); });
	CHECK(rig.Tick().blocked == TBMBlockedReason::IllegalTurn);
	CHECK(rig.Job().partial_work_ticks == 1);
	// After backing away, the cutter is not at the face, so TravelToFace cannot become Bore.
	rig.Reload(TBMPhase::TravelToFace, [heading](TBMProject &project) { project.heading = heading; });
	CHECK(rig.Tick().phase == TBMPhase::TravelToFace);
	CHECK(rig.Job().partial_work_ticks == 1);
}
