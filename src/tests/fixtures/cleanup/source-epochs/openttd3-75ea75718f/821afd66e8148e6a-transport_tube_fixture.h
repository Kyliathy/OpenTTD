/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_fixture.h Shared real capsule/rail/depot owners for bounded tube prototype and construction checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups1B-6 and13CB
 */

#ifndef TESTS_TRANSPORT_TUBE_FIXTURE_H
#define TESTS_TRANSPORT_TUBE_FIXTURE_H

#include "tbm_fixture.h"
#include "underground_fleet_fixture.h"
#include "../transport_tube.h"
#include "../layer_portal_map.h"
#include "../timer/timer_game_tick.h"

/** Real native content, geological floor, engine definitions and command-bought capsules; no synthetic movement/energy owner. */
class TubePrototypeRig : public UndergroundFleetRig {
public:
	RailType tube = INVALID_RAILTYPE; ///< Actual canonical slot, allocated after conventional definitions.
	TileIndex depot = INVALID_TILE; ///< Owned command-built tube prototype depot on explicit fixture-open floor.

	/** Install prior registry/prototype producers and prepare the actual capsule purchase depot. */
	TubePrototypeRig() : UndergroundFleetRig(0)
	{
		// Native capsule art now participates in the ordinary viewport refresh.
		// Mechanical fixtures need the existing mock bounds environment, just
		// like TBM acquisition; these empty headers prove no artwork or pixels.
		// Full-application raster fixtures load the actual packed sprites.
		MockEnvironment::Instance();
		ResetTubePowerState(); ResetUtilityPowerState(); // Each isolated real prototype world owns its own physical/electrical fixture lifetime.
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

	/** Clear only this fixture's power owners before the inherited real capsule/company/map teardown. */
	~TubePrototypeRig() { ResetTubePowerState(); ResetUtilityPowerState(); }

	/**
	 * Fund an explicit native service/binding for an earlier mechanical movement fixture.
	 * @param capsule Actual stopped command-bought native front in its owned tube depot.
	 * Fixture-open portal/wire geometry remains declared boundary input; this never claims normal bootstrap acceptance.
	 */
	void InstallPaidFixturePower(Train &capsule)
	{
		const auto layer = GetTileLayer(capsule.tile);
		const auto rock = TileAddWrap(capsule.tile, 0, -2), door = TileAddWrap(capsule.tile, 0, -1);
		const auto upper_layer = layer == MapLayer::DeepUnderground ? MapLayer::Underground : MapLayer::Surface;
		const auto upper = MultiLayerMap::TileInLayer(rock, upper_layer);
		this->Open(std::array{rock, door});
		if (upper_layer == MapLayer::Underground) this->Open(std::array{upper, TileAddWrap(upper, 0, -1)});
		MakeRailLayerPortal(upper, _current_company, DiagDirection::SE, layer, RAILTYPE_RAIL);
		MakeRailLayerPortal(rock, _current_company, DiagDirection::NW, upper_layer, RAILTYPE_RAIL);
		auto records = CaptureLayerPortalWorkRecords();
		LayerPortalWorkRecord record; record.pair_key = upper; record.company = _current_company; record.state = LayerPortalWorkState::Connected;
		records.push_back(record); std::ranges::sort(records, {}, &LayerPortalWorkRecord::pair_key);
		StageLoadedLayerPortalWork(std::move(records)); AcceptLoadedLayerPortalWork();
		const std::array wire{door, capsule.tile};
		const auto service = ConnectTubeExternalPower(rock, capsule.tile, wire);
		REQUIRE(service.error == TubePowerError::None);
		REQUIRE(BindTubePowerVehicle(capsule.index, service.producer).has_value());
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
