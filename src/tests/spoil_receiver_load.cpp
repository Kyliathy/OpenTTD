/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file spoil_receiver_load.cpp Staged loading of receiver state (Plan 5 Group 8 persistence): loaded rows stay
 * unpublished until they match the rebuilt receiver industries one for one, with in-range fields and stock; any
 * refusal publishes nothing. The metadata feature flag follows the live receivers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-i-receiver-design.md, Persistence
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"
#include "../multilayer_state.h"

#include "../safeguards.h"

/** Rows publish unchanged only against matching receivers; each kind of mismatch refuses and leaves the store empty. */
TEST_CASE("Loaded receiver rows publish only against matching receiver industries", "[spoil-receiver-load]")
{
	SpoilReceiverRig rig;
	Industry &earth = *rig.BuildReceiver(SpoilReceiverKind::Earth, 15, 22);
	Industry &rock = *rig.BuildReceiver(SpoilReceiverKind::Rock, 14, 16);
	REQUIRE(AdmitSpoilToReceiver(earth, GetCargoTypeByLabel(CargoLabel{"ERTH"}), 16, _current_company) == 16);
	SpoilReceiverRig::Stock(earth) += 16;
	REQUIRE(SetSpoilReceiverMode(rock.index, SpoilReceiverMode::SurplusDisposal, true) == SpoilReceiverError::None);
	for (int tick = 0; tick < 10; ++tick) AdvanceSpoilReceiver(earth);
	const auto saved = CaptureSpoilReceivers();
	REQUIRE(saved.size() == 2);
	REQUIRE(saved[0].industry == earth.index);
	const uint64_t digest = ComputeSpoilReceiverDigest();
	const auto schema = CaptureMultiLayerStateSchema();
	CHECK((schema.feature_flags & MULTILAYER_FEATURE_SPOIL_RECEIVERS) != 0);
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::None);

	/**
	 * Clear the store, stage loaded rows and try to publish them.
	 * @param rows Loaded rows.
	 * @return Whether they were accepted.
	 */
	const auto load = [](std::vector<SpoilReceiver> rows) {
		ResetSpoilReceiverState();
		StageLoadedSpoilReceivers(std::move(rows));
		CHECK(CaptureSpoilReceivers().empty()); // Staged rows are not live until accepted.
		return AcceptLoadedSpoilReceivers();
	};

	SECTION("intact rows publish unchanged") {
		REQUIRE(load(saved));
		CHECK(CaptureSpoilReceivers() == saved);
		CHECK(ComputeSpoilReceiverDigest() == digest);
		CHECK(AcceptLoadedSpoilReceivers()); // Nothing staged: the store stays as it is.
		CHECK(CaptureSpoilReceivers() == saved);
	}
	SECTION("a receiver without its row refuses") {
		auto rows = saved;
		rows.pop_back();
		CHECK_FALSE(load(rows));
		CHECK(CaptureSpoilReceivers().empty());
	}
	SECTION("a row of the wrong kind refuses") {
		auto rows = saved;
		rows[0].kind = SpoilReceiverKind::Rock;
		CHECK_FALSE(load(rows));
	}
	SECTION("a row for a missing industry refuses") {
		auto rows = saved;
		rows.push_back(saved[0]);
		rows.back().industry = IndustryID{999};
		CHECK_FALSE(load(rows));
	}
	SECTION("Earth cannot be in surplus disposal") {
		auto rows = saved;
		rows[0].mode = SpoilReceiverMode::SurplusDisposal;
		CHECK_FALSE(load(rows));
	}
	SECTION("more intake than one interval allows refuses") {
		auto rows = saved;
		rows[0].interval_intake = 33;
		CHECK_FALSE(load(rows));
	}
	SECTION("a stock above the cap refuses") {
		SpoilReceiverRig::Stock(earth) = 1025;
		CHECK_FALSE(load(saved));
	}
	SECTION("without receivers the flag clears") {
		ResetSpoilReceiverState();
		CHECK((CaptureMultiLayerStateSchema().feature_flags & MULTILAYER_FEATURE_SPOIL_RECEIVERS) == 0);
	}
}
