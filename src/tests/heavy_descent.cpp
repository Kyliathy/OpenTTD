/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_descent.cpp Immutable sender/dimension receipts and source-era heavy pending-return admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB
 * Detached record values are declared unit inputs, without a native cargo, shell, paid hull or procurement claim.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../space_manifest.h"
#include "../safeguards.h"

/** Supported immutable roles retain exact mass rights after sender loss; retained old unknown provenance never grants a new heavy receipt. */
TEST_CASE("Descent receipts retain exact sender mass rights and old unknown provenance", "[heavy-descent-contract]")
{
	CHECK(GetSpaceDescentMassLimit(15) == 320); CHECK(GetSpaceDescentMassLimit(20) == 320); CHECK(GetSpaceDescentMassLimit(21) == 640);
	CHECK(GetSpaceDescentMassLimit(SPACE_DESCENT_LEGACY_SENDER_MODEL) == 320);
	// Every unrelated role stays outside paid Surface release, even when a plausible cargo dimension or reused native ID could otherwise resemble a miner.
	for (uint16_t model : {uint16_t{0}, uint16_t{14}, uint16_t{16}, uint16_t{17}, uint16_t{18}, uint16_t{19}, uint16_t{22}}) {
		CAPTURE(model); CHECK(GetSpaceDescentMassLimit(model) == 0); CHECK_FALSE(IsSpaceDescentReceiptValid(model, 20, 16, 320));
	}
	CHECK(IsSpaceDescentReceiptValid(15, 20, 16, 320)); CHECK(IsSpaceDescentReceiptValid(20, 40, 8, 320));
	CHECK(IsSpaceDescentReceiptValid(21, 40, 16, 640)); CHECK(IsSpaceDescentReceiptValid(21, 20, 32, 640));
	CHECK_FALSE(IsSpaceDescentReceiptValid(15, 40, 16, 640)); CHECK_FALSE(IsSpaceDescentReceiptValid(20, 20, 32, 640));
	CHECK_FALSE(IsSpaceDescentReceiptValid(21, 0, 16, 0)); CHECK_FALSE(IsSpaceDescentReceiptValid(21, 41, 1, 41));
	CHECK_FALSE(IsSpaceDescentReceiptValid(21, 40, 0, 640)); CHECK_FALSE(IsSpaceDescentReceiptValid(21, 40, 16, 0));
	CHECK_FALSE(IsSpaceDescentReceiptValid(21, 40, 16, 639)); CHECK_FALSE(IsSpaceDescentReceiptValid(21, 40, 32, 1280));
	CHECK(IsSpaceDescentReceiptValid(SPACE_DESCENT_LEGACY_SENDER_MODEL, 20, 0, 0));
	CHECK_FALSE(IsSpaceDescentReceiptValid(SPACE_DESCENT_LEGACY_SENDER_MODEL, 40, 16, 640));
	CHECK_FALSE(IsSpaceDescentReceiptValid(SPACE_DESCENT_LEGACY_SENDER_MODEL, 20, 16, 0));
}

/** Detached new jobs require coherent provenance before owner lookup, and supply-only sources cannot reserve future heavy returns or service. */
TEST_CASE("Heavy pending descents require their own source producer and coherent saved receipt", "[heavy-descent-contract]")
{
	SpaceDescentRecord row; row.pod = {SpacePodID{3}, 9}; row.id = 1; row.center = {StationID{1}, 4}; row.release_point = {20 * SPACE_Q16, 20 * SPACE_Q16};
	row.sender = VehicleID{22}; row.sender_generation = 8; row.sender_model = 21; row.unit_weight = 16; row.freight_mass = 640;
	row.cargo = CargoType{0}; row.quantity = 40; row.remaining_ticks = 111; row.packets.push_back({CargoPacketID{5}, StationID::Invalid()});
	CHECK(ValidateSpaceDescentRecords(std::array{row}, 2, 64, 64));
	// Each independently corrupted historical scalar must fail before a missing/current native sender could mask it or lend a replacement cargo dimension.
	for (uint field = 0; field < 5; ++field) {
		auto changed = row;
		switch (field) {
			case 0: changed.sender_model = 16; break;
			case 1: changed.sender_model = 15; break;
			case 2: changed.unit_weight = 8; break;
			case 3: changed.freight_mass = 320; break;
			case 4: changed.sender_model = SPACE_DESCENT_LEGACY_SENDER_MODEL; break;
		}
		CHECK_FALSE(ValidateSpaceDescentRecords(std::array{changed}, 2, 64, 64));
	}
	SpaceManifestRecord manifest; manifest.craft = {VehicleID{22}, 8}; manifest.returns_remaining = 1; manifest.pending_returns = 1;
	CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, manifest, true)); CHECK(IsSpaceHeavyResupplyStateSupported(21, manifest, true, true));
	CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, manifest, false, true)); CHECK(IsSpaceHeavyResupplyStateSupported(15, manifest, false));
	manifest.service_ticks = 1; CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, manifest, true, true));
	manifest.service_ticks = 0; manifest.services_completed = 1; CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, manifest, true, true));
}
