/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file portal_lifecycle.cpp Real empty-pair removal, conversion and atomic owner/accounting boundaries. @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 11 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../landscape_cmd.h"
#include "../command_func.h"
#include "../rail_cmd.h"
#include "../rail.h"
#include "../road.h"
#include "../tile_cmd.h"
#include "../safeguards.h"

/** Removing from either housing retires one actual record and both infrastructure pieces without opening persistent rock. */
TEST_CASE("Empty sealed portal removal preserves geology from either endpoint", "[portal-lifecycle]")
{
	ExcavationFixture fixture;
	ResetLayerPortalWorkState(); ResetRailTypes();
	const auto source = TileXYL(20, 20, MapLayer::Surface), target = MultiLayerMap::TileInLayer(source, MapLayer::Underground);
	TileIndex clicked = source;
	SECTION("Upper end") {}
	SECTION("Solid rock end") { clicked = target; }
	REQUIRE(CmdBuildLayerPortal({DoCommandFlag::Execute}, source, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE).Succeeded());
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto infrastructure = Company::Get(_current_company)->infrastructure.rail[RAILTYPE_RAIL];
	CHECK(Command<Commands::LandscapeClear>::Do({}, clicked).Succeeded());
	CHECK(GetLayerPortalWorkPairCount() == 1);
	REQUIRE(Command<Commands::LandscapeClear>::Do({DoCommandFlag::Execute}, clicked).Succeeded());
	CHECK(GetTileType(source) == TileType::Clear); CHECK(GetTileType(target) == TileType::Clear);
	CHECK(GetLayerPortalWorkPairCount() == 0);
	CHECK(Company::Get(_current_company)->infrastructure.rail[RAILTYPE_RAIL] == infrastructure - 2);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo);
	CHECK(GetGeologyWorldState().terrain.Get(target) == ExcavationState::Solid);
	ResetLayerPortalWorkState();
}

/** Conversion changes both native housing types over retained solid ground and invalidates the work-site stamp once. */
TEST_CASE("Empty sealed portal conversion commits both housing types", "[portal-lifecycle]")
{
	ExcavationFixture fixture;
	ResetLayerPortalWorkState(); ResetRailTypes();
	Company::Get(_current_company)->avail_railtypes.Set(RAILTYPE_ELECTRIC);
	const auto source = TileXYL(20, 20, MapLayer::Surface), target = MultiLayerMap::TileInLayer(source, MapLayer::Underground);
	TileIndex clicked = source;
	SECTION("Upper end") {}
	SECTION("Solid rock end") { clicked = target; }
	REQUIRE(CmdBuildLayerPortal({DoCommandFlag::Execute}, source, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE).Succeeded());
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto revision = GetLayerPortalWorkRecord(source)->revision;
	REQUIRE(CmdConvertRail({DoCommandFlag::Execute}, clicked, clicked, RAILTYPE_ELECTRIC, false).Succeeded());
	CHECK(GetRailType(source) == RAILTYPE_ELECTRIC); CHECK(GetRailType(target) == RAILTYPE_ELECTRIC);
	CHECK(GetLayerPortalWorkRecord(target)->revision == revision + 1);
	CHECK(GetLayerPortalWorkState(source) == LayerPortalWorkState::Sealed);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo);
	ResetLayerPortalWorkState();
}

/** The real owner callback transfers an empty pair together; bankruptcy retains only explicitly ownerless sealed road housing. */
TEST_CASE("Empty portal owner callback keeps pair and saved accounting consistent", "[portal-lifecycle]")
{
	ExcavationFixture fixture;
	ResetLayerPortalWorkState(); ResetRailTypes(); ResetRoadTypes();
	TransportType transport = TransportType::Rail;
	bool bankrupt = false;
	SECTION("Rail company transfer") {}
	SECTION("Road company transfer") { transport = TransportType::Road; }
	SECTION("Ownerless road housing") { transport = TransportType::Road; bankrupt = true; }
	const auto source = TileXYL(20, 20, MapLayer::Surface), target = MultiLayerMap::TileInLayer(source, MapLayer::Underground);
	REQUIRE(CmdBuildLayerPortal({DoCommandFlag::Execute}, source, transport, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
	REQUIRE(Company::CanAllocateItem()); auto *receiver = Company::Create();
	const auto old_owner = _current_company;
	const Owner new_owner = bankrupt ? INVALID_OWNER : receiver->index;
	const Owner actual_owner = bankrupt ? OWNER_NONE : receiver->index;
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	_tile_type_procs[TileType::LayerPortal]->change_tile_owner_proc(target, old_owner, new_owner);
	CHECK(GetTileOwner(source) == actual_owner); CHECK(GetTileOwner(target) == actual_owner);
	CHECK(GetLayerPortalWorkRecord(source)->company == (bankrupt ? CompanyID::Invalid() : receiver->index));
	CHECK(GetLayerPortalWorkState(source) == LayerPortalWorkState::Sealed);
	CHECK(ValidateLayerPortalWorkRecords(CaptureLayerPortalWorkRecords(), GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
	const auto revision = GetLayerPortalWorkRecord(source)->revision;
	_tile_type_procs[TileType::LayerPortal]->change_tile_owner_proc(source, old_owner, new_owner);
	CHECK(GetLayerPortalWorkRecord(source)->revision == revision); // The later map-walk callback cannot transfer/account the same pair twice.
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo);
	if (transport == TransportType::Rail) { CHECK(Company::Get(old_owner)->infrastructure.rail[RAILTYPE_RAIL] == 0); CHECK(receiver->infrastructure.rail[RAILTYPE_RAIL] == 2); }
	UpdateSignalsInBuffer(); ResetLayerPortalWorkState(); delete receiver;
}
