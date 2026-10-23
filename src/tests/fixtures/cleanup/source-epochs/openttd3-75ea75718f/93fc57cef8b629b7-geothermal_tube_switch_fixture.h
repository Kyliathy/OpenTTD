/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_tube_switch_fixture.h Actual paid native and external tube sources on one prepared commissioned Deep route.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups6BC/6BDB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared portal/floor/materials are explicit boundary inputs, separate from normal bootstrap acceptance.
 */
#ifndef TESTS_GEOTHERMAL_TUBE_SWITCH_FIXTURE_H
#define TESTS_GEOTHERMAL_TUBE_SWITCH_FIXTURE_H

#include "../layer_portal_map.h"
#include "../layer_portal_cmd.h"
#include "geothermal_tube_work_fixture.h"

/** Actual purchased native plant, connected guideway and leased external inlet in one explicitly prepared Deep world. */
class GeothermalTubeSwitchRig : public GeothermalTubeWorkRig {
public:
	TubeGeothermalPowerResult geothermal; ///< Original native plant/root/inlet, not a synthetic source.
	UtilityProducerID external{UINT32_MAX}; ///< Actual separately invoiced external service at the same owned inlet.
	/** @param serialized Match shipped native engine/content identities for ordinary files. Prepare explicit commissioned portal geometry, then buy both original services through their real admission/invoice owners. */
	explicit GeothermalTubeSwitchRig(bool serialized = false) : GeothermalTubeWorkRig(serialized)
	{
		ResetLayerPortalWorkState(); this->JoinDepots();
		const auto portal = TileXYL(32, 22, MapLayer::DeepUnderground);
		const auto upper = MultiLayerMap::TileInLayer(portal, MapLayer::Underground);
		const std::array path{TileXYL(32, 23, MapLayer::DeepUnderground), this->inlets[0]};
		this->Open(std::array{portal, upper, TileXYL(32, 21, MapLayer::Underground), path[0]});
		MakeRailLayerPortal(upper, _current_company, DiagDirection::SE, MapLayer::DeepUnderground, RAILTYPE_RAIL);
		MakeRailLayerPortal(portal, _current_company, DiagDirection::NW, MapLayer::Underground, RAILTYPE_RAIL);
		LayerPortalWorkRecord record; record.pair_key = upper; record.company = _current_company; record.state = LayerPortalWorkState::Connected;
		StageLoadedLayerPortalWork({record}); AcceptLoadedLayerPortalWork(); REQUIRE(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Connected);
		const auto service = ConnectTubeExternalPower(portal, this->inlets[0], path); REQUIRE(service.error == TubePowerError::None);
		this->external = service.producer; this->geothermal = this->Connect();
	}
	/** Reset the explicit portal-reference fixture before its physical map/company disappears; capsule/source cleanup retains its inherited actual owners. */
	~GeothermalTubeSwitchRig() { ResetLayerPortalWorkState(); }
	/** @param capsule Actual held native front. @param native True selects the original geothermal inlet, false the paid external service. @param execute False predicts without mutation. @return Complete replacement recipient or unchanged refusal. */
	std::optional<UtilityConsumerID> Switch(Train &capsule, bool native, bool execute = true)
	{
		return native ? ReconnectTubeGeothermalPowerVehicle(capsule.index, this->geothermal.source, execute) : ReconnectTubePowerVehicle(capsule.index, this->external, execute);
	}
	/** @param capsule Actual started native front. Drive a real departure, then use ordinary Stop and native deceleration to obtain held on-guideway custody. */
	void StopOnRoute(Train &capsule)
	{
		uint ticks = 0;
		// Switching is tested at a genuinely travelled location; native physics
		// and player commands alone choose the held footprint and cargo custody.
		while (capsule.IsInDepot() && ticks++ < 1000) this->Run(std::array{&capsule});
		REQUIRE(ticks < 1000); REQUIRE_FALSE(capsule.IsInDepot());
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule.index, true).Succeeded());
		while (capsule.cur_speed != 0 && ticks++ < 1200) this->Run(std::array{&capsule});
		REQUIRE(ticks < 1200); REQUIRE(capsule.cur_speed == 0); REQUIRE(capsule.vehstatus.Test(VehState::Stopped));
	}
};

#endif /* TESTS_GEOTHERMAL_TUBE_SWITCH_FIXTURE_H */
