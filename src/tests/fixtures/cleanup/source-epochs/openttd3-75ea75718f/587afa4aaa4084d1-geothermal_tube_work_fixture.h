/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_tube_work_fixture.h Actual native paid geothermal supply and ordered capsule movement/service participants.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BCD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor and delivered construction stock remain explicit boundary inputs, not full bootstrap acceptance.
 */
#ifndef TESTS_GEOTHERMAL_TUBE_WORK_FIXTURE_H
#define TESTS_GEOTHERMAL_TUBE_WORK_FIXTURE_H

#include "mock_environment.h"
#include "movement_fixture.h"
#include "../depot_base.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../cargopacket.h"
#include "../signal_func.h"
#include "geothermal_tube_capsule_fixture.h"

/** Command-purchased real capsules with one common native preparation and unchanged packet custody. */
class GeothermalTubeWorkRig : public GeothermalTubeCapsuleRig {
	PathfinderDefaults pathfinder; ///< Actual ordinary pathfinder budgets, restored after this world.
public:
	/** @param serialized Preserve shipped native engine/content identities for ordinary work-file acceptance; false keeps the existing focused runtime fixture. */
	explicit GeothermalTubeWorkRig(bool serialized = false) : GeothermalTubeCapsuleRig(serialized) {}
	/** @param capsule Actual purchased native capsule. @return Original partially loaded packet identity, never an auxiliary cargo buffer. */
	CargoPacketID Load(Train &capsule)
	{
		REQUIRE(CargoPacket::CanAllocateItem());
		auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
		packet->UpdateLoadingTile(capsule.tile); capsule.cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
		// Normal native loading refreshes the complete consist and original
		// acceleration as well as weight; a direct cache subset would diverge
		// when an ordinary loader rebuilds the same actual loaded capsule.
		capsule.MarkDirty();
		return packet->index;
	}
	/** @param capsule Actual purchased capsule. @param destination Original native depot to reach and halt in. Install ordinary orders, then start through the shared command. */
	void Start(Train &capsule, TileIndex destination)
	{
		Order order;
		order.MakeGoToDepot(Depot::GetByTile(destination)->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {OrderDepotActionFlag::Halt});
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, capsule.index, 0, order).Succeeded()); ProcessOrders(&capsule);
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule.index, true).Succeeded());
	}
	/** @param capsules Actual native fronts in simulation order. Advance one shared epoch and their complete native ticks, with no fixture-generated electricity or physical relocation. */
	void Run(std::span<Train *const> capsules)
	{
		UpdateSignalsInBuffer(); ++TimerGameTick::counter; PrepareTubePowerTick();
		// Every actual front receives its ordinary work opportunity only after
		// all companies and original forwarding roots share one allocation.
		for (auto *capsule : capsules) {
			REQUIRE(capsule->Tick()); REQUIRE_FALSE(capsule->vehstatus.Test(VehState::Crashed)); REQUIRE(IsTrainOnItsTiles(capsule));
		}
	}
};

#endif /* TESTS_GEOTHERMAL_TUBE_WORK_FIXTURE_H */
