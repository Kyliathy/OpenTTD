/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file transport_tube_meter_fixture.h Actual command-built paid capsule route shared by metering and recovery tests.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups6-7
 */

#ifndef TESTS_TRANSPORT_TUBE_METER_FIXTURE_H
#define TESTS_TRANSPORT_TUBE_METER_FIXTURE_H

#include "transport_tube_power_fixture.h"
#include "movement_fixture.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../timer/timer_game_tick.h"
#include "../pathfinder/yapf/yapf_costcache.hpp"
#include "../economy_func.h"

/** Real command-built capsule route; the normal front tick executes both locomotive substeps through one paid participant. */
class TubeMeterRig : public TubePowerRig {
	PathfinderDefaults pathfinder; ///< Real routing budgets restored after this fixture.
	AutoRestoreBackup<uint64_t> tick{TimerGameTick::counter, uint64_t{0}}; ///< Explicit actual synchronized opportunity, independent of capsule cache refreshes.
public:
	TileIndex destination = TileXYL(38, 49, MapLayer::Underground); ///< Actual native halt depot reached through ordinary orders and physical ticks.
	/** Build only fixture-open floor and native tube track/depot commands; create no electrical balance or vehicle directly. */
	TubeMeterRig()
	{
		// A straight paid prototype route isolates the native grant gate from
		// later production-terminal and useful-route acceptance requirements.
		for (int y = 41; y < 49; ++y) this->BuildTrack(TileXYL(38, y, MapLayer::Underground), this->tube, Track::Y);
		this->Open(std::array{this->destination});
		REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, this->destination, this->tube, DiagDirection::NW).Succeeded());
		Order order;
		order.MakeGoToDepot(Depot::GetByTile(this->destination)->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {OrderDepotActionFlag::Halt});
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, this->capsule->index, 0, order).Succeeded());
		ProcessOrders(this->capsule);
	}
	/** Install the actual leased inlet and native capsule's explicit ledger binding; no manual reservation fixture stands in for scheduling. */
	void Bind()
	{
		const auto service = ConnectTubeExternalPower(this->portal, this->depot, this->path);
		REQUIRE(service.error == TubePowerError::None); this->source = service.producer;
		const auto consumer = BindTubePowerVehicle(this->capsule->index, this->source);
		REQUIRE(consumer.has_value()); this->consumer = *consumer;
	}
	/** @param ticks Bounded whole simulation ticks; ordinary source allocation and actual front movement share one synchronized epoch. */
	void Run(uint ticks = 1)
	{
		UpdateSignalsInBuffer();
		// One synchronized preparation before the real front preserves global
		// allocation order, and both native movement substeps consume only once.
		for (uint step = 0; step < ticks; ++step) {
			++TimerGameTick::counter; PrepareTubePowerTick();
			REQUIRE(this->capsule->Tick());
			REQUIRE_FALSE(this->capsule->vehstatus.Test(VehState::Crashed));
			REQUIRE(IsTrainOnItsTiles(this->capsule));
		}
	}
	/** Start the actual capsule through the ordinary player command, with power admission remaining in its native tick. */
	void Start() { REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, this->capsule->index, true).Succeeded()); }
};

#endif /* TESTS_TRANSPORT_TUBE_METER_FIXTURE_H */
