/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_fleet_fixture.h Real native engines, funded mine and ordinary depots for focused fleet acceptance. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Groups 7B-10 */

#ifndef TESTS_UNDERGROUND_FLEET_FIXTURE_H
#define TESTS_UNDERGROUND_FLEET_FIXTURE_H

#include "underground_facility_fixture.h"
#include "../underground_vehicle.h"
#include "../tbm.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../vehicle_cmd.h"
#include "../vehicle_func.h"
#include "../train.h"
#include "../roadveh.h"
#include "../group.h"

/** Ordinary native freight constructed through the actual commands; no substitute vehicles or service/accounting owner. */
class UndergroundFleetRig : public UndergroundFacilityRig {
	decltype(_engine_mngr.mappings) mappings = _engine_mngr.mappings; ///< Restore actual caller EIDS scope.
	AutoRestoreBackup<UnitID> trains{_settings_game.vehicle.max_trains, UnitID{500}}; ///< Allow normal fixture train acquisition.
	AutoRestoreBackup<UnitID> roads{_settings_game.vehicle.max_roadveh, UnitID{500}}; ///< Allow normal fixture truck acquisition.
	AutoRestoreBackup<uint8_t> length{_settings_game.vehicle.max_train_length, uint8_t{7}}; ///< Normal train length constraint.
	AutoRestoreBackup<Money> train_price{_price[Price::BuildVehicleTrain], Money{100000}}; ///< Real cost quoting has a nonzero baseline.
	AutoRestoreBackup<Money> wagon_price{_price[Price::BuildVehicleWagon], Money{10000}}; ///< Real wagon price.
	AutoRestoreBackup<Money> road_price{_price[Price::BuildVehicleRoad], Money{10000}}; ///< Real truck price.
	AutoRestoreBackup<Money> rail_running{_price[Price::RunningTrainDiesel], Money{1000}}; ///< Ordinary powered-engine running price.
	AutoRestoreBackup<Money> road_running{_price[Price::RunningRoadveh], Money{1000}}; ///< Ordinary truck running price.
public:
	/** @param mine_index Actual Coal or Lithium finite mine used by the shared facility fixture. @param full_save Install dense original mappings before native definitions and use distributed cargo slots. */
	explicit UndergroundFleetRig(size_t mine_index = 0, bool full_save = false) : UndergroundFacilityRig(mine_index, full_save)
	{
		REQUIRE(Engine::GetNumItems() == 0);
		REQUIRE(Vehicle::GetNumItems() == 0);
		ResetLayerPortalWorkState();
		ResetTBMProjectState();
		if (full_save) {
			_engine_mngr.ResetToDefaultMapping();
			SetupEngines(); // Normal full-world engine transport must reserve every original identity before appending native EIDS definitions.
		}
		REQUIRE(InitialiseTBMEngineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseUndergroundEngineDefinitions(this->ActivatedContent()));
		// Test normal company availability independently of the technology predicate; stale availability flags must never bypass dates.
		for (uint16_t model = 2; model <= 8; ++model) Engine::Get(GetUndergroundEngine(model))->company_avail.Set(_current_company);
	}

	/** Delete real vehicle chains while stations, cargo and company owners still exist, then restore definition identity. */
	~UndergroundFleetRig()
	{
		// Native destruction retires orders, station loading references, packets and group counts before the inherited facilities disappear.
		while (Vehicle::GetNumItems() != 0) delete (*Vehicle::Iterate().begin())->First();
		ResetTBMProjectState();
		ResetLayerPortalWorkState();
		_group_pool.CleanPool();
		_engine_pool.CleanPool();
		_engine_mngr.mappings = std::move(this->mappings);
	}

	/** @param layer Actual permitted ground plane. @param road Whether to build a truck rather than train depot. @return Command-built owned depot tile. */
	TileIndex BuildDepot(MapLayer layer, bool road)
	{
		const auto tile = TileXYL(road ? 45 : 40, 40, layer);
		const auto entry = TileIndexDiffCByDiagDir(DiagDirection::SE);
		// Surface depots use ordinary terrain; geology owns excavation only for rock planes.
		if (layer != MapLayer::Surface) this->Open(std::array{tile, TileAddWrap(tile, entry.x, entry.y)});
		REQUIRE((road ? CmdBuildRoadDepot(DoCommandFlag::Execute, tile, ROADTYPE_ROAD, DiagDirection::SE) :
			CmdBuildTrainDepot(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, DiagDirection::SE)).Succeeded());
		return tile;
	}

	/** @param depot Actual owned compatible facility. @param model Canonical ordinary design. @param cargo Proposed exact cargo/refit. @return Actual command-created vehicle. */
	Vehicle *Buy(TileIndex depot, uint16_t model, CargoType cargo = INVALID_CARGO)
	{
		const auto [result, id, capacity, mail_capacity, cargo_capacities] = CmdBuildVehicle(DoCommandFlag::Execute, depot,
			GetUndergroundEngine(model), false, cargo, ClientID::Invalid);
		REQUIRE(result.Succeeded());
		REQUIRE(Vehicle::IsValidID(id));
		return Vehicle::Get(id);
	}
};

#endif /* TESTS_UNDERGROUND_FLEET_FIXTURE_H */
