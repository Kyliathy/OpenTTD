/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_fixture.h Actual command-created Surface station for admission and order-save component tests.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups13C.1/13CCC.
 * Cash/map/town/station remain declared component inputs, never an unassisted paid gameplay predecessor.
 * Call MockEnvironment::Instance and LoadGeneratedWindowTestLanguage before construction, before CargoSpecs change.
 */
#ifndef TESTS_SURFACE_SUPPLY_FIXTURE_H
#define TESTS_SURFACE_SUPPLY_FIXTURE_H
#include "../station_base.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include "../town.h"
#include "../town_kdtree.h"
#include "../newgrf_station.h"
#include "../rail.h"
#include "../station_func.h"
#include "../economy_base.h"
#include "../core/backup_type.hpp"
#include "../table/strings.h"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "layer_window_fixture.h"

/** Real command-created Surface station; fixture cash/map/town are explicit and provide no later acceptance predecessor. */
class SurfaceSupplyAdmissionRig : public ExcavationFixture {
	AutoRestoreBackup<CompanyID> local{_local_company, _current_company}; ///< Keep presentation actor local without changing the native buyer.
	AutoRestoreBackup<uint64_t> inflation{_economy.inflation_prices, uint64_t{1} << 16}; ///< Declared normal-price component input.
public:
	StationID station = StationID::Invalid(); ///< Sole actual command-issued receiving station.
	/** Build the original Surface rail facility through its actual admission/placement owner.
	 * @param serialized_native_slots Match distributed native cargo slots for full-file component exports; this never supplies procurement provenance.
	 */
	explicit SurfaceSupplyAdmissionRig(bool serialized_native_slots = false) : ExcavationFixture(serialized_native_slots)
	{
		ResetRailTypes(); StationClass::Reset();
		REQUIRE(Station::GetNumItems() == 0); REQUIRE(Town::GetNumItems() == 0); REQUIRE(Town::CanAllocateItem());
		auto *town = Town::Create(TileXYL(10, 10, MapLayer::Surface)); town->townnametype = SPECSTR_TOWNNAME_START;
		RebuildTownKdtree(); RebuildStationKdtree();
		const auto tile = TileXYL(20, 20, MapLayer::Surface);
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		this->station = GetStationIndex(tile);
	}
	/** Retire the actual station before its naming town and inherited company/world disappear. */
	~SurfaceSupplyAdmissionRig()
	{
		// Native station destruction removes derived catchment/spatial ownership;
		// a naked pool wipe could leak a fixture into another command test.
		for (auto *station : Station::Iterate()) delete station;
		_town_pool.CleanPool(); RebuildTownKdtree();
	}
};

#endif /* TESTS_SURFACE_SUPPLY_FIXTURE_H */
