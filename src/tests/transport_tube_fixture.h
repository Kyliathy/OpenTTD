/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_fixture.h Shared real capsule/rail/depot owners for bounded tube prototype and construction checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Groups1B-3
 */

#ifndef TESTS_TRANSPORT_TUBE_FIXTURE_H
#define TESTS_TRANSPORT_TUBE_FIXTURE_H

#include "tbm_fixture.h"
#include "underground_fleet_fixture.h"
#include "../transport_tube.h"

/** Real native content, geological floor, engine definitions and command-bought capsules; no synthetic movement/energy owner. */
class TubePrototypeRig : public UndergroundFleetRig {
public:
	RailType tube = INVALID_RAILTYPE; ///< Actual canonical slot, allocated after conventional definitions.
	TileIndex depot = INVALID_TILE; ///< Owned command-built tube prototype depot on explicit fixture-open floor.

	/** Install prior registry/prototype producers and prepare the actual capsule purchase depot. */
	TubePrototypeRig() : UndergroundFleetRig(0)
	{
		this->tube = InitialiseTubeRailType(this->ActivatedContent());
		REQUIRE(this->tube != INVALID_RAILTYPE);
		REQUIRE(InitialiseTubeEngineDefinitions(this->ActivatedContent()));
		Company::Get(_current_company)->avail_railtypes.Set(this->tube);
		Engine::Get(GetTubeEngine(9))->company_avail.Set(_current_company);
		this->depot = TileXYL(38, 40, MapLayer::Underground);
		const auto entrance = TileIndexDiffCByDiagDir(DiagDirection::SE);
		this->Open(std::array{this->depot, TileAddWrap(this->depot, entrance.x, entrance.y)});
		REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, this->depot, this->tube, DiagDirection::SE).Succeeded());
	}

	/** @return An actual command-purchased self-propelled capsule in the real prototype depot. */
	Train *BuyCapsule()
	{
		const auto [result, id, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute,
			this->depot, GetTubeEngine(9), false, INVALID_CARGO, ClientID::Invalid);
		REQUIRE(result.Succeeded());
		return Train::Get(id);
	}

	/** @param tile Exact fixture-open world cell. @param type Canonical native/ordinary rail definition. @param track Real track bit. */
	void BuildTrack(TileIndex tile, RailType type, Track track = Track::X)
	{
		this->Open(std::array{tile});
		REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, tile, type, track, false).Succeeded());
	}
};

#endif /* TESTS_TRANSPORT_TUBE_FIXTURE_H */
