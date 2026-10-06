/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_facility_fixture.h Shared actual mine, station and depot command fixture. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Groups 6-10 */

#ifndef TESTS_UNDERGROUND_FACILITY_FIXTURE_H
#define TESTS_UNDERGROUND_FACILITY_FIXTURE_H

#include "mine_production_fixture.h"
#include "../station_cmd.h"
#include "../rail_cmd.h"
#include "../road_cmd.h"
#include "../rail.h"
#include "../road.h"
#include "../newgrf_station.h"
#include "../newgrf_roadstop.h"
#include "../order_base.h"
#include "../depot_base.h"
#include "../roadstop_base.h"
#include "../table/strings.h"

/** Actual funded mine with command-ready original station/road definitions and normal Surface naming town. */
class UndergroundFacilityRig : public MineProductionRig {
public:
	/** @param mine_index Native Coal or Deep Lithium fixture resource index. @param serialized_native_slots Match distributed cargo transport for full-game saves. */
	explicit UndergroundFacilityRig(size_t mine_index, bool serialized_native_slots = false) : MineProductionRig(mine_index, serialized_native_slots)
	{
		ResetRailTypes();
		ResetRoadTypes();
		StationClass::Reset();
		RoadStopClass::Reset();
		RebuildStationKdtree(); // Earlier isolated pool fixtures may have intentionally skipped ordinary station destruction.
		Town *town = *Town::Iterate().begin();
		town->townnametype = SPECSTR_TOWNNAME_START;
	}

	/** Remove fixture-built depot identities before the inherited fixture destroys their town/company/map owners. */
	~UndergroundFacilityRig()
	{
		// Ordinary deletion retires station catchment, packets and spatial signs before their industry/town owners disappear.
		for (Station *station : Station::Iterate()) delete station;
		_roadstop_pool.CleanPool();
		_depot_pool.CleanPool();
	}

	/** @return Actual two-cell loading platform anchor below this mine's machinery. */
	TileIndex RailSite() const { return TileAddWrap(this->request.tile, 0, 2); }
	/** @return Actual nearby freight road-stop site, separated from the railway platform by an empty cell. */
	TileIndex RoadSite() const { return TileAddWrap(this->request.tile, 3, 2); }

	/** Open the exact railway floor and both required end approaches through the geological fixture owner. */
	void OpenRailSite()
	{
		const TileIndex tile = this->RailSite();
		this->Open(std::array{tile, TileAddWrap(tile, 1, 0), TileAddWrap(tile, -1, 0), TileAddWrap(tile, 2, 0)});
	}

	/** Open the actual one-ended terminal floor/approach without changing machinery, stock or mine claim. */
	void OpenRoadSite()
	{
		const TileIndex tile = this->RoadSite();
		const auto entry = TileIndexDiffCByDiagDir(DiagDirection::SE);
		this->Open(std::array{tile, TileAddWrap(tile, entry.x, entry.y)});
	}

	/** @param flags Command estimate/execute policy. @param join Existing normal StationID or NEW_STATION for independence. @return Native freight road-stop result. */
	CommandCost BuildTerminal(DoCommandFlags flags, StationID join) const
	{
		return CmdBuildRoadStop(flags, this->RoadSite(), 1, 1, RoadStopType::Truck, false, DiagDirection::SE,
			ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, join, false);
	}
};

#endif /* TESTS_UNDERGROUND_FACILITY_FIXTURE_H */
