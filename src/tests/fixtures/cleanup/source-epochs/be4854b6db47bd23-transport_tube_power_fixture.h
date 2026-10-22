/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file transport_tube_power_fixture.h Real 1970 portal/capsule owners for isolated paid service and metered movement tests. @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Groups5-7 */

#ifndef TESTS_TRANSPORT_TUBE_POWER_FIXTURE_H
#define TESTS_TRANSPORT_TUBE_POWER_FIXTURE_H

#include "transport_tube_fixture.h"
#include "../multilayer_bootstrap.h"
#include "../layer_portal_map.h"
#include "../timer/timer_game_economy.h"

/** Explicit fixture-open commissioned geometry; neither this boundary fixture nor its service participant claims a useful production route. */
class TubePowerRig : public TubePrototypeRig {
	AutoRestoreBackup<TimerGameCalendar::Date> date{TimerGameCalendar::date,
		TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1970}, 0, 1)}; ///< Genuine early calendar with locked Deep.
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{1970}}; ///< Keep real calendar components coherent.
	AutoRestoreBackup<TimerGameEconomy::Date> economy{TimerGameEconomy::date, TimerGameEconomy::Date{100}}; ///< Actual synchronized invoice day.
public:
	TileIndex portal = TileXYL(38, 38, MapLayer::Underground); ///< Rock-side actual Connected portal, separate from the capsule network.
	std::array<TileIndex, 2> path{TileXYL(38, 39, MapLayer::Underground), this->depot}; ///< Explicit installed utility cells.
	UtilityProducerID source{UINT32_MAX}; ///< Ledger-issued service identity after real connection admission.
	UtilityConsumerID consumer{UINT32_MAX}; ///< Actual TubeWork account explicitly linked to the physical service.
	Train *capsule = nullptr; ///< Command-bought prototype, owning its service/movement state and cargo.

	/** Build only explicit saved-reference fixture geometry; production activation and normal-game bootstrap remain later acceptance. */
	TubePowerRig()
	{
		ResetUtilityPowerState(); ResetTubePowerState();
		this->Open(std::array{this->portal, this->path[0]});
		const auto surface = MultiLayerMap::TileInLayer(this->portal, MapLayer::Surface);
		MakeRailLayerPortal(surface, _current_company, DiagDirection::SE, MapLayer::Underground, RAILTYPE_RAIL);
		MakeRailLayerPortal(this->portal, _current_company, DiagDirection::NW, MapLayer::Surface, RAILTYPE_RAIL);
		LayerPortalWorkRecord record;
		record.pair_key = surface; record.company = _current_company; record.state = LayerPortalWorkState::Connected;
		StageLoadedLayerPortalWork({record}); AcceptLoadedLayerPortalWork();
		REQUIRE(GetLayerPortalWorkState(this->portal) == LayerPortalWorkState::Connected);
		this->capsule = this->BuyCapsule();
	}
	/** Release only this fixture's physical/electrical authority before actual company/map owners disappear. */
	~TubePowerRig() { ResetTubePowerState(); ResetUtilityPowerState(); }
	/** @param capacity_j Explicit bounded TubeWork request capacity. Register the actual physical paid source and consumer once. */
	void Connect(uint64_t capacity_j = 39960)
	{
		const auto result = ConnectTubeExternalPower(this->portal, this->depot, this->path);
		REQUIRE(result.error == TubePowerError::None); this->source = result.producer;
		const std::array linked{this->source};
		const auto demand = RegisterUtilityPowerConsumer(_current_company, MapLayer::Underground, UtilityConsumerEffect::TubeWork, capacity_j, linked);
		REQUIRE(demand.has_value()); this->consumer = *demand;
	}
	/** @param demand_j Complete requested actual opportunity. @param ticks Canonical source duration, normally one74-tick economy day. */
	void Offer(uint64_t demand_j = 39960, uint32_t ticks = 74)
	{
		const auto interval = CaptureUtilityPowerState().interval;
		REQUIRE(PublishTubeExternalPower(this->source, interval, ticks) == TubePowerError::None);
		const std::array requests{UtilityPowerRequest{this->consumer, demand_j}};
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(interval, requests) == UtilityPowerError::None);
	}
};

/** Actual stopped depot service participant; admission/commit failures deliberately prove complete transaction rollback. */
class CapsuleDepotService final : public UtilityMutationParticipant {
	Train &capsule; ///< Real native capsule being serviced by the ordinary vehicle owner.
public:
	bool admit = true, commit = true; ///< Explicit fixture faults; neither supplies energy or bypasses production movement.
	/** @param capsule Actual prototype capsule in its real depot. */
	explicit CapsuleDepotService(Train &capsule) : capsule(capsule) {}
	/** @param proposal Exact actual TubeWork use. @return Whether complete stopped same-company depot service still admits. */
	bool Validate(const UtilityMutationProposal &proposal) const override
	{
		// Actual owner, stopped depot state and available whole service action
		// must all agree before either the invoice or the source can settle.
		return this->admit && proposal.owner == this->capsule.owner && this->capsule.IsInDepot() && this->capsule.cur_speed == 0;
	}
	/** @param proposal Already admitted exact whole service. @return Complete native service or unchanged injected refusal. */
	bool Commit([[maybe_unused]] const UtilityMutationProposal &proposal) noexcept override
	{
		if (!this->commit) return false;
		VehicleServiceInDepot(&this->capsule);
		return true;
	}
};

#endif /* TESTS_TRANSPORT_TUBE_POWER_FIXTURE_H */
