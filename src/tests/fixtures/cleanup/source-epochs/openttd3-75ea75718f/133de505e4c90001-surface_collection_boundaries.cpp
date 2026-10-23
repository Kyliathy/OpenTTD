/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_collection_boundaries.cpp Isolated full storage, cargo-demand allocation and cross-layer/global-setting boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group19.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../water_map.h"
#include "../road_type.h"
#include "../road.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../hub_cargo.h"
#include "../core/backup_type.hpp"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "../safeguards.h"

/** Explicit isolated native station/packet/demand starting conditions; no constructed fixture state is exported as a paid gameplay predecessor. */
class SurfaceCollectionBoundaryRig {
	ExcavationFixture world;
	std::vector<std::unique_ptr<Company>> foreign_owners; ///< Survive their isolated native stations until those objects retire.
	std::vector<std::unique_ptr<Station>> owners;
public:
	Station *center = nullptr, *rail = nullptr, *lorry = nullptr;
	CargoType ore = INVALID_CARGO, ice = INVALID_CARGO;
	/** @param tile Actual isolated ground-layer position. @param road Whether to construct a lorry bay. @param owner Starting native company authority.
	 * @return Actual isolated station/pool/tile owner, never a copied private station registry.
	 */
	Station *Freight(TileIndex tile, bool road, CompanyID owner)
	{
		REQUIRE(Station::CanAllocateItem()); owners.emplace_back(Station::Create(tile)); auto *station = owners.back().get(); station->owner = owner;
		station->spread.Add(tile); station->facilities.Set(road ? StationFacility::TruckStop : StationFacility::Train);
		if (road) MakeRoadStop(tile, owner, station->index, RoadStopType::Truck, ROADTYPE_ROAD, INVALID_ROADTYPE, DiagDirection::SE);
		else { MakeRailStation(tile, owner, station->index, Axis::X, 0, RAILTYPE_RAIL); station->train_station.Add(tile); }
		return station;
	}
	/** Construct exact20-tile/near20-tile separate members and an isolated owned launch reference with real native packet containers. */
	SurfaceCollectionBoundaryRig()
	{
		ResetRoadTypes(); ResetLaunchCenters(); const auto tile = TileXYL(20, 20, MapLayer::Surface);
		REQUIRE(Station::CanAllocateItem()); owners.emplace_back(Station::Create(tile)); center = owners.back().get(); center->owner = _current_company; center->facilities.Set(StationFacility::LaunchCenter);
		// These explicit boundary tiles establish isolated starting ownership only; actual placement/material-paid recovery is proved by surface_collection_native.cpp.
		for (const auto cell : *GetLaunchCenterFootprint(tile, 0)) { MakeStation(cell, center->owner, center->index, StationType::LaunchCenter, 0); center->spread.Add(cell); }
		MakeRiver(TileAddXY(tile, 0, -1), 0); InstallNewLaunchCenter(*center, tile, 0);
		rail = Freight(TileXYL(40, 20, MapLayer::Surface), false, _current_company); lorry = Freight(TileXYL(39, 23, MapLayer::Surface), true, _current_company);
		ore = GetCargoTypeByLabel(CargoLabel{"NORE"}); ice = GetCargoTypeByLabel(CargoLabel{"WICE"}); REQUIRE(IsValidCargoType(ore)); REQUIRE(IsValidCargoType(ice)); CargoSpec::Get(ore)->weight = CargoSpec::Get(ice)->weight = 16;
	}
	/** Retire only isolated quantities/owners while their company, tile and CargoSpec registry remain valid. */
	~SurfaceCollectionBoundaryRig() { owners.clear(); ResetLaunchCenters(); }
	/** @return A real isolated foreign company retained until all its boundary station owners retire. */
	Company *Foreign()
	{
		REQUIRE(Company::CanAllocateItem()); foreign_owners.emplace_back(Company::Create()); return foreign_owners.back().get();
	}
	/** @param station Actual isolated storage owner. @param cargo Exact supported cargo. @param quantity Explicit initial test-content units, not newly mined/produced goods. */
	void Stock(Station &station, CargoType cargo, uint16_t quantity)
	{
		REQUIRE(CargoPacket::CanAllocateItem()); station.goods[cargo].GetOrCreateData().cargo.Append(CargoPacket::Create(StationID::Invalid(), quantity, Source{0, SourceType::Asteroid}), StationID::Invalid());
	}
};

/** Full storage changes only allocation admission; a subsequent per-cargo demand change cannot steal already assigned physical packets. */
TEST_CASE("Boundary collection filters full storage and changed demands without duplicate assignment", "[surface-collection-contract]")
{
	MockEnvironment::Instance(); SurfaceCollectionBoundaryRig rig; const auto home = GetLaunchCenterReference(*rig.center);
	for (auto *member : {rig.rail, rig.lorry}) REQUIRE(CmdSetLaunchHub(DoCommandFlag::Execute, member->index, home.station, home.generation).Succeeded()); // The two separate members must own actual exclusive generation-qualified links.
	rig.Stock(*rig.rail, rig.ore, 1024); rig.Stock(*rig.center, rig.ore, 20); rig.Stock(*rig.center, rig.ice, 12);
	rig.rail->goods[rig.ore].last_speed = 1; rig.lorry->goods[rig.ore].last_speed = 1; rig.lorry->goods[rig.ice].last_speed = 1;
	const auto cash = Company::Get(_current_company)->money; TickSurfaceHandling(); REQUIRE(rig.center->launch_hub->handling.size() == 2);
	// Both demand ore, but the full lower-ID rail member has no headroom; ice demand exists only at the lorry, so no unsupported cargo reservation may be created for rail.
	for (const auto &job : rig.center->launch_hub->handling) CHECK(job->member.station == rig.lorry->index);
	CHECK(rig.center->goods[rig.ore].TotalCount() == 0); CHECK(rig.center->goods[rig.ice].TotalCount() == 0); CHECK(Company::Get(_current_company)->money == cash);
	REQUIRE(HubCargoTransfer::Consume(rig.rail->goods[rig.ore].GetOrCreateData().cargo, 1024)); rig.lorry->goods[rig.ore].last_speed = 0; rig.Stock(*rig.center, rig.ore, 13); TickSurfaceHandling();
	REQUIRE(rig.center->launch_hub->handling.size() == 3); CHECK(rig.center->launch_hub->handling.back()->member.station == rig.rail->index); CHECK(rig.center->launch_hub->handling.back()->cargo == rig.ore);
	// Removing the lorry's future ore demand leaves its original assigned20 units in place; only the new13-unit packet is eligible for the newly available rail member.
	for (uint tick = 0; tick < 8; ++tick) TickSurfaceHandling();
	CHECK(rig.center->launch_hub->handling.empty()); CHECK(rig.rail->goods[rig.ore].TotalCount() == 13); CHECK(rig.rail->goods[rig.ice].TotalCount() == 0);
	CHECK(rig.lorry->goods[rig.ore].TotalCount() == 20); CHECK(rig.lorry->goods[rig.ice].TotalCount() == 12); CHECK(Company::Get(_current_company)->money == cash - 45);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.ore] == 0); CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.ice] == 0); CHECK(ValidateSurfaceHandling());
}

/** An explicit same-company Surface link is independent of ordinary catchment/spread settings; neither foreign nor same-XY rock-plane membership can acquire its cargo. */
TEST_CASE("Boundary collection membership remains exclusive across global settings and stacked planes", "[surface-collection-contract]")
{
	MockEnvironment::Instance(); SurfaceCollectionBoundaryRig rig; const auto home = GetLaunchCenterReference(*rig.center); AutoRestoreBackup spread(_settings_game.station.station_spread, uint8_t{1});
	// Separate one-tile native stations stay eligible at the circular boundary for both ordinary catchment policies; this changes no global station geometry or IDs.
	for (const bool modified : {false, true}) {
		AutoRestoreBackup catchment(_settings_game.station.modified_catchment, modified);
		for (auto *member : {rig.rail, rig.lorry}) { CHECK(IsLaunchHubEligible(*rig.center, *member)); REQUIRE(CmdSetLaunchHub(DoCommandFlag::Execute, member->index, home.station, home.generation).Succeeded()); CHECK(GetLaunchHubMembership(member->index)->center == home); }
	}
	auto *unlinked = rig.Freight(TileXYL(38, 23, MapLayer::Surface), false, _current_company); CHECK(IsLaunchHubEligible(*rig.center, *unlinked)); CHECK_FALSE(GetLaunchHubMembership(unlinked->index));
	rig.Stock(*rig.center, rig.ore, 10); CHECK_FALSE(CanStartSurfaceHandling(home, {unlinked->index, 1}, rig.ore, 10, false));
	auto *foreign = rig.Foreign(); auto *alien = rig.Freight(TileXYL(38, 24, MapLayer::Surface), false, foreign->index);
	auto *rock = rig.Freight(TileXYL(40, 20, MapLayer::Underground), false, _current_company);
	// Quote and execution both refuse before adding a membership or assigning the center's ten physical units; XY coincidence cannot cross the layer boundary.
	for (auto *invalid : {alien, rock}) for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK_FALSE(IsLaunchHubEligible(*rig.center, *invalid)); CHECK(CmdSetLaunchHub(flags, invalid->index, home.station, home.generation).Failed()); CHECK_FALSE(GetLaunchHubMembership(invalid->index)); CHECK(rig.center->goods[rig.ore].TotalCount() == 10);
	}
	CHECK(ValidateSurfaceHandling());
}
