/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file portal_tbm_access.cpp Restricted Commissioning-pair admission through the actual TBM role contract.
 * Real boundary consists only; final playable entry and retreat movement come with Plan 3 Group 7 / master Group 7.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 5 (task 5.3B)
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../train_cmd.h"
#include "../tile_cmd.h"

#include "../safeguards.h"

/** Every part of the assigned consist may enter and retreat at both ends; nothing else may. */
TEST_CASE("Commissioning pairs admit only parts of their assigned TBM consist", "[portal-tbm-access]")
{
	TBMProjectRig rig;
	const auto source = rig.fixture.request.portal;
	const auto destination = rig.proof.destination;
	const auto job = rig.project.job;
	// Both ends use the same check, so entry at the source and retreat at the housing get the same authority.
	for (const Train *part = rig.front; part != nullptr; part = part->Next()) {
		CHECK(CheckLayerPortalWorkAccess(source, part->index, job) == LayerPortalWorkError::None);
		CHECK(CheckLayerPortalWorkAccess(destination, part->index, job) == LayerPortalWorkError::None);
	}
	CHECK(CheckLayerPortalWorkAccess(source, rig.front->index, ExcavationJobID{UINT32_MAX}) == LayerPortalWorkError::InvalidReference);
	CHECK(CheckLayerPortalWorkAccess(source, VehicleID::Invalid(), job) == LayerPortalWorkError::InvalidReference);
	// Ordinary routing stays hidden: the restricted admission is not a usable edge for pathfinding.
	CHECK_FALSE(GetTileTrackStatus(source, TransportType::Rail, RoadTramType::Invalid).trackdirs.Any());

	SECTION("an unrelated train is refused") {
		const auto second = TileXYL(30, 20, MapLayer::Surface);
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, second, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		auto request = rig.fixture.request;
		request.portal = second;
		request.proposal = rig.base;
		request.proposal.commissioning_anchor = GetLayerPortalPairTile(second);
		Train *other = nullptr;
		const std::array<const Engine *, 1> wagons{Engine::Get(GetTBMEngine(2))};
		REQUIRE(BuildTBMConsistAtSite(request, Engine::Get(GetTBMEngine(0)), 0, GetFreeUnitNumber(VehicleType::Train), wagons, &other).Succeeded());
		// Same company, same cutter model, but not this pair's assigned train or one of its parts.
		for (const Train *part = other; part != nullptr; part = part->Next()) {
			CHECK(CheckLayerPortalWorkAccess(source, part->index, job) == LayerPortalWorkError::InvalidReference);
		}
	}
	SECTION("without a live project the pair's cutter has no authority yet") {
		ResetTBMProjectState();
		CHECK(CheckLayerPortalWorkAccess(source, rig.front->index, job) == LayerPortalWorkError::ControllerUnavailable);
	}
	SECTION("a consist that no longer matches its machinery is refused") {
		auto tampered = CaptureTBMProjects();
		std::swap(tampered.back().permanent_parts[0], tampered.back().permanent_parts[1]);
		const auto next = GetNextTBMProjectID();
		ResetTBMProjectState();
		StageLoadedTBMProjects(tampered, next);
		REQUIRE(AcceptLoadedTBMProjects()); // Loads as a recoverable InvalidConsist block.
		CHECK(CheckLayerPortalWorkAccess(source, rig.front->index, job) == LayerPortalWorkError::InvalidReference);
	}
}
