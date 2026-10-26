/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_identity_consumers.cpp Issued rocket intents, live navigation and preserved legacy cargo custody.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group14C.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_transition.h"
#include "../company_base.h"
#include "../saveload/cargo_source_sl.h"
#include "probe_mining_fixture.h"
#include "../safeguards.h"

/** Flight history uses lifetime issuance, while successful navigation still requires a live current target. */
TEST_CASE("Space body flight intents use issued identities rather than population caps", "[space-identity-consumers]")
{
	SpaceTransitionRecord row;
	row.craft = {VehicleID{2}, 7};
	auto &flight = row.state;
	flight.phase = SpaceFlightPhase::Preparing;
	flight.source = flight.recovery = {StationID{1}, 4};
	flight.destination = {SpaceOrderTargetKind::Body, 70'000, 0};
	flight.remaining_ticks = 148;
	flight.missions_started = 1;
	CHECK(ValidateSpaceTransitionRecords(std::array{row}));
	CHECK(ValidateSpaceTransitionRecords(std::array{row}, 64, 64, 70'001));
	CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{row}, 64, 64, 70'000));
	CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{row}, 64, 64, 3));
	flight.destination.id = UINT32_MAX - 1;
	CHECK(ValidateSpaceTransitionRecords(std::array{row}, 64, 64, UINT32_MAX));
	flight.destination.id = UINT32_MAX;
	CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{row}, 64, 64, UINT32_MAX));
	flight.destination = {SpaceOrderTargetKind::Body, 70'000, 0};
	flight.phase = SpaceFlightPhase::ReturnRecovery;
	flight.layer = MapLayer::Space;
	flight.remaining_ticks = 222;
	flight.outbound_burned = flight.recovery_burned = true;
	flight.blocked = SpaceFlightBlock::TargetLost;
	CHECK(ValidateSpaceTransitionRecords(std::array{row}, 64, 64, 70'001));
	CHECK_FALSE(ValidateSpaceTransitionRecords(std::array{row}, 64, 64, 3));
}

/** Original preview/demo setup resolves its three starter IDs before publication; missing navigation never creates a body. */
TEST_CASE("Space starter previews and missing body navigation preserve native owners", "[space-identity-consumers]")
{
	ProbeExtractionRig rig;
	const auto initial = GetAsteroidField();
	REQUIRE(PrepareSpaceSurveyPreview());
	CHECK(GetAsteroidField().surveys.size() == 3);
	for (const auto &preview : GetAsteroidField().surveys) {
		const auto *body = FindAsteroidBody(GetAsteroidField(), preview.target);
		REQUIRE(body != nullptr);
		CHECK(body->id.base() == preview.id.base());
		CHECK(to_underlying(body->resource_class) == preview.id.base());
	}
	REQUIRE(RetireSpaceSurveyPreview());
	CHECK(GetAsteroidField() == initial);
	auto &craft = rig.Probe(4, false);
	const auto before = GetAsteroidField();
	const auto fleet = ComputeSpaceVehicleDigest();
	SpacePoint destination{123, 456};
	CHECK(ResolveSpaceNavigationTarget(craft, {SpaceOrderTargetKind::Body, 70'000, 0}, destination) == SpaceOrderBlock::TargetRemoved);
	CHECK(ResolveSpaceNavigationTarget(craft, {SpaceOrderTargetKind::Body, UINT32_MAX, 0}, destination) == SpaceOrderBlock::TargetRemoved);
	CHECK(GetAsteroidField() == before);
	CHECK(ComputeSpaceVehicleDigest() == fleet);
	CHECK_FALSE(PrepareNativeSpaceMiningDemo()); // An existing original hull prevents legacy tour duplication.
	CHECK(ComputeSpaceVehicleDigest() == fleet);
}

/** Current packets remain real native extraction and keep their original legacy source until typed-origin persistence is ready. */
TEST_CASE("Space identity migration preserves actual extraction custody and old source gates", "[space-identity-consumers]")
{
	ProbeExtractionRig rig;
	auto &craft = rig.Probe();
	const auto id = rig.Attach(craft).id;
	const auto reserve = GetMutableAsteroidBody(id)->remaining_reserve;
	// Earn exactly one actual native extraction interval; no copied history or
	// direct packet insertion supplies this custody check's two physical units.
	for (uint32_t tick = 0; tick < 74; ++tick) rig.Step();
	REQUIRE(craft.cargo.TotalCount() == 2);
	CHECK(GetMutableAsteroidBody(id)->remaining_reserve == reserve - 2);
	CHECK(ValidateSpaceVehicles());
	const auto field = GetAsteroidField();
	CHECK_FALSE(PrepareSpaceSurveyPreview()); // Native lease ownership excludes ownerless preview publication.
	CHECK(GetAsteroidField() == field);
	REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, craft.index, UINT32_MAX).Succeeded());
	CHECK(craft.cargo.TotalCount() == 2);
	CHECK(ValidateSpaceVehicles());
	const auto cargo = craft.cargo_type;
	craft.cargo_type = GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[1].cargo);
	REQUIRE(IsValidCargoType(craft.cargo_type));
	REQUIRE(craft.cargo_type != cargo);
	CHECK_FALSE(ValidateSpaceVehicles());
	craft.cargo_type = cargo;
	CHECK(ValidateSpaceVehicles());
	CHECK(craft.cargo.TotalCount() == 2);
	CHECK(IsValidCargoPacketSource(Source{0, SourceType::Asteroid}, 0, SaveLoadVersion::SpaceMiningVehicles));
	CHECK_FALSE(IsValidCargoPacketSource(Source{MAX_ORBITAL_BODIES, SourceType::Asteroid}, 0, SaveLoadVersion::SpaceMiningVehicles));
}
