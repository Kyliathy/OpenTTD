/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file spoil_receiver_intake.cpp Bounded receiver intake, fees and interval processing (Plan 5 Groups 8.2-8.3):
 * admission never exceeds the free stock or the interval allowance and charges exactly the admitted tonnes once; each
 * interval disposes, crushes or (when chosen) disposes surplus Rock; and a station's room is its catchment's receivers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 8
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-i-receiver-design.md, Frozen recipe and Bounded intake
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"
#include "../core/backup_type.hpp"

#include "../safeguards.h"

/** @return The Earth cargo. */
static CargoType Earth() { return GetCargoTypeByLabel(CargoLabel{"ERTH"}); }
/** @return The Rock cargo. */
static CargoType Rock() { return GetCargoTypeByLabel(CargoLabel{"ROCK"}); }

/** Admission takes at most the room, charges the delivering company for exactly that, and the next interval reopens it. */
TEST_CASE("An Earth receiver admits its room, charges once and disposes per interval", "[spoil-receiver-intake]")
{
	SpoilReceiverRig rig;
	Industry &site = *rig.BuildReceiver(SpoilReceiverKind::Earth, 15, 22);

	SECTION("the interval allowance bounds intake and the next interval disposes it") {
		CHECK(GetSpoilReceiverRoom(site) == 32); // The 32 t interval allowance, well below the free stock.
		const Money cash = SpoilReceiverRig::Cash();
		REQUIRE(AdmitSpoilToReceiver(site, Earth(), 20, _current_company) == 20);
		SpoilReceiverRig::Stock(site) += 20; // The delivery adds what was admitted, as for any industry.
		CHECK(SpoilReceiverRig::Cash() == cash - 40); // 2 Money units per admitted tonne.
		CHECK(GetSpoilReceiverRoom(site) == 12);
		CHECK(AdmitSpoilToReceiver(site, Earth(), 30, _current_company) == 12);
		SpoilReceiverRig::Stock(site) += 12;
		CHECK(AdmitSpoilToReceiver(site, Earth(), 5, _current_company) == 0);
		CHECK(SpoilReceiverRig::Cash() == cash - 64); // Nothing admitted, nothing charged.
		CHECK(AdmitSpoilToReceiver(site, Rock(), 5, _current_company) == 0); // The wrong spoil is never admitted.
		CHECK(GetSpoilReceiver(site.index)->fees == 64);

		for (uint tick = 1; tick < SPOIL_RECEIVER_INTERVAL_TICKS; ++tick) AdvanceSpoilReceiver(site);
		CHECK(GetSpoilReceiverRoom(site) == 0); // One tick short of the interval: still closed.
		CHECK(SpoilReceiverRig::Stock(site) == 32);
		AdvanceSpoilReceiver(site);
		CHECK(SpoilReceiverRig::Stock(site) == 0); // Four 8 t batches disposed.
		CHECK(GetSpoilReceiver(site.index)->disposed_t == 32);
		CHECK(GetSpoilReceiverRoom(site) == 32);
	}
	SECTION("the stock cap bounds intake when the stock is nearly full") {
		SpoilReceiverRig::Stock(site) = 1020;
		CHECK(GetSpoilReceiverRoom(site) == 4);
		CHECK(AdmitSpoilToReceiver(site, Earth(), 32, _current_company) == 4);
	}
	SECTION("an incomplete batch waits for more spoil") {
		SpoilReceiverRig::Stock(site) = 13;
		SpoilReceiverRig::RunIntervals(site, 1);
		CHECK(SpoilReceiverRig::Stock(site) == 5);
		CHECK(GetSpoilReceiver(site.index)->disposed_t == 8);
	}
}

/** Crushing makes aggregate and residue until the aggregate store is full; then the Rock waits until surplus disposal. */
TEST_CASE("The Rock crusher pauses when aggregate is full and its founder may choose surplus disposal", "[spoil-receiver-intake]")
{
	SpoilReceiverRig rig;
	Industry &crusher = *rig.BuildReceiver(SpoilReceiverKind::Rock, 14, 16);
	CHECK(GetSpoilReceiverRoom(crusher) == 64);
	SpoilReceiverRig::Stock(crusher) = 100;
	SpoilReceiverRig::RunIntervals(crusher, 1);
	const SpoilReceiver *state = GetSpoilReceiver(crusher.index);
	CHECK(SpoilReceiverRig::Stock(crusher) == 36); // Eight 8 t batches: the interval's 64 t.
	CHECK(state->crushed_t == 64);
	CHECK(state->aggregate == 48);
	CHECK(state->residue_t == 16);

	// Keep the stock full until the aggregate store can take no further 6 t batch (1,020 of 1,024 t).
	for (int interval = 0; interval < 40 && GetSpoilReceiver(crusher.index)->aggregate + 6 <= 1024; ++interval) {
		SpoilReceiverRig::Stock(crusher) = 1024;
		SpoilReceiverRig::RunIntervals(crusher, 1);
	}
	REQUIRE(GetSpoilReceiver(crusher.index)->aggregate == 1020);
	SpoilReceiverRig::Stock(crusher) = 1024;
	SpoilReceiverRig::RunIntervals(crusher, 1);
	CHECK(SpoilReceiverRig::Stock(crusher) == 1024); // Paused: nothing is crushed and nothing is lost.
	CHECK(GetSpoilReceiverRoom(crusher) == 0); // A full stock admits nothing more.

	// Only the founder chooses the mode, and Earth has no surplus mode.
	Industry &earth = *rig.BuildReceiver(SpoilReceiverKind::Earth, 15, 22);
	CHECK(SetSpoilReceiverMode(earth.index, SpoilReceiverMode::SurplusDisposal, true) == SpoilReceiverError::InvalidMode);
	CHECK(SetSpoilReceiverMode(IndustryID{999}, SpoilReceiverMode::Process, true) == SpoilReceiverError::InvalidIndustry);
	{
		Backup<CompanyID> other(_current_company, CompanyID{1});
		CHECK(SetSpoilReceiverMode(crusher.index, SpoilReceiverMode::SurplusDisposal, true) == SpoilReceiverError::NotFounder);
		other.Restore();
	}
	REQUIRE(Command<Commands::SetSpoilReceiverMode>::Do(DoCommandFlag::Execute, crusher.index, SpoilReceiverMode::SurplusDisposal).Succeeded());
	SpoilReceiverRig::RunIntervals(crusher, 1);
	CHECK(SpoilReceiverRig::Stock(crusher) == 960); // Eight batches disposed, no aggregate made.
	CHECK(GetSpoilReceiver(crusher.index)->disposed_t == 64);
	CHECK(GetSpoilReceiver(crusher.index)->aggregate == 1020);
	const Money cash = SpoilReceiverRig::Cash();
	REQUIRE(AdmitSpoilToReceiver(crusher, Rock(), 10, _current_company) == 10);
	CHECK(SpoilReceiverRig::Cash() == cash - 40); // The surplus fee is 4 Money units per tonne.
}

/** A station delivers into the room of its catchment's receivers of that spoil; real receivers give the origin acceptance. */
TEST_CASE("A station's spoil room is the room of its catchment's receivers", "[spoil-receiver-intake]")
{
	SpoilReceiverRig rig;
	const Station &origin = *rig.fixture.origin;
	CHECK(CheckTBMOriginAdmission(rig.Project()) == TBMExchangeError::NoReceiver);
	Industry &earth = *rig.BuildReceiver(SpoilReceiverKind::Earth, 15, 22);
	rig.BuildReceiver(SpoilReceiverKind::Rock, 14, 16);
	rig.RefreshOrigin();
	CHECK(GetSpoilReceiverRoomAt(origin, Earth()) == 32);
	CHECK(GetSpoilReceiverRoomAt(origin, Rock()) == 64);
	CHECK(GetSpoilReceiverRoomAt(origin, GetCargoTypeByLabel(CT_PASSENGERS)) == UINT_MAX); // Other cargo is unbounded.
	CHECK(origin.goods[Earth()].status.Test(GoodsEntry::State::Acceptance));
	CHECK(origin.goods[Rock()].status.Test(GoodsEntry::State::Acceptance));
	CHECK(CheckTBMOriginAdmission(rig.Project()) == TBMExchangeError::None); // Both real receivers now accept.

	SECTION("a second Earth receiver adds its own room") {
		rig.BuildReceiver(SpoilReceiverKind::Earth, 20, 17);
		rig.RefreshOrigin();
		CHECK(GetSpoilReceiverRoomAt(origin, Earth()) == 64);
	}
	SECTION("a receiver reserved for another supplier gives this station no room") {
		earth.exclusive_supplier = OWNER_NONE;
		CHECK(GetSpoilReceiverRoomAt(origin, Earth()) == 0);
	}
	SECTION("a removed receiver takes its state with it") {
		const IndustryID id = earth.index;
		UnregisterSpoilReceiver(id);
		CHECK(GetSpoilReceiver(id) == nullptr);
		CHECK(GetSpoilReceiverRoomAt(origin, Earth()) == 0);
	}
}
