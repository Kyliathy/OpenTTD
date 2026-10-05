/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange_fixture.h A TBM work rig with a real hauler for the wagon exchange tests: the original locomotive
 * with empty Earth and Rock hoppers, built and coupled in a real Surface depot, then placed in the project's bay, on
 * track or in a depot as its hauler cycle (Plan 5 Group 6) expects. Neither train can yet drive itself (master Group 7).
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Groups 1-7
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-h-exchange-design.md
 */

#ifndef TESTS_TBM_EXCHANGE_FIXTURE_H
#define TESTS_TBM_EXCHANGE_FIXTURE_H

#include "tbm_fixture.h"
#include "../tbm_exchange.h"
#include "../tbm_cmd.h"
#include "../train_cmd.h"
#include "../rail_cmd.h"
#include "../command_func.h"
#include "../cargotype.h"
#include "../station_map.h"
#include "../tilearea_type.h"

/** Every part of a chain, in order. */
using TBMChainIDs = std::vector<VehicleID>;

/**
 * @param head Chain head.
 * @return Every part ID in chain order.
 */
inline TBMChainIDs GetTBMChainIDs(const Train *head)
{
	TBMChainIDs ids;
	for (const Train *part = head; part != nullptr; part = part->Next()) ids.push_back(part->index);
	return ids;
}

/** Packets aboard one part, as (packet ID, count) pairs in list order. */
using TBMPacketList = std::vector<std::pair<CargoPacketID, uint>>;

/**
 * @param head Chain head.
 * @return For every part, its packets, keyed by the part's ID; proves each packet stays with its own wagon.
 */
inline std::map<VehicleID, TBMPacketList> GetTBMPacketOwners(const Train *head)
{
	std::map<VehicleID, TBMPacketList> owners;
	for (const Train *part = head; part != nullptr; part = part->Next()) {
		auto &list = owners[part->index];
		for (const CargoPacket *packet : *part->cargo.Packets()) list.emplace_back(packet->index, packet->Count());
	}
	return owners;
}

/** One published project boring on a single cut, plus a company hauler with empty hoppers in a real Surface depot. */
class TBMExchangeRig : public TBMWorkRig {
public:
	TileIndex depot = TileXYL(24, 20, MapLayer::Surface); ///< Real Surface depot the hauler is built and coupled in.
	Train *hauler = nullptr; ///< Hauler front engine.

	/**
	 * Build the hauler in the depot.
	 * @param segments Straight cuts of the project.
	 * @param earth Empty Earth hoppers behind the locomotive.
	 * @param rock Empty Rock hoppers after them.
	 */
	explicit TBMExchangeRig(size_t segments = 1, uint8_t earth = 2, uint8_t rock = 2) : TBMWorkRig(segments)
	{
		// Load time resolves an original engine's cargo label; this isolated registry is installed after the engine exists.
		this->fixture.stock->info.cargo_type = GetCargoTypeByLabel(CT_PASSENGERS);
		REQUIRE(IsValidCargoType(this->fixture.stock->info.cargo_type));
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, this->depot, RAILTYPE_RAIL, DiagDirection::NE).Succeeded());
		this->hauler = this->BuildHauler(this->depot, earth, rock);
	}

	/**
	 * Build a locomotive with empty hoppers in a depot through the real construction and coupling owners, with the
	 * unit number and statistics bookkeeping of CmdBuildVehicle.
	 * @param depot_tile Depot to build in.
	 * @param earth Empty Earth hoppers.
	 * @param rock Empty Rock hoppers after them.
	 * @return The stopped front engine.
	 */
	Train *BuildHauler(TileIndex depot_tile, uint8_t earth, uint8_t rock) const
	{
		Vehicle *built = nullptr;
		REQUIRE(Vehicle::CanAllocateItem(1));
		REQUIRE(CmdBuildRailVehicle(DoCommandFlag::Execute, depot_tile, this->fixture.stock, &built).Succeeded());
		Train *head = Train::From(built);
		head->unitnumber = Company::Get(_current_company)->freeunits[VehicleType::Train].UseID(GetFreeUnitNumber(VehicleType::Train));
		GroupStatistics::CountEngine(head, 1);
		GroupStatistics::CountVehicle(head, 1);
		std::vector<uint16_t> models(earth, 2);
		models.insert(models.end(), rock, 3);
		// Each hopper is built free in the depot and coupled at the end of the train through the ordinary move.
		for (uint16_t model : models) {
			REQUIRE(Vehicle::CanAllocateItem(1));
			built = nullptr;
			REQUIRE(CmdBuildRailVehicle(DoCommandFlag::Execute, depot_tile, Engine::Get(GetTBMEngine(model)), &built).Succeeded());
			GroupStatistics::CountEngine(built, 1);
			REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, built->index, head->Last()->index, false).Succeeded());
		}
		REQUIRE(head->IsStoppedInDepot());
		return head;
	}

	/**
	 * Place a whole train in the depot-like state on another site's tile, where the reference vehicle stands, as the
	 * hauler cycle will drive it there.
	 * @param head Train to place.
	 * @param site Vehicle whose tile and position the train takes.
	 */
	void PlaceAt(Train *head, const Train &site) const
	{
		for (Train *part = head; part != nullptr; part = part->Next()) {
			part->tile = site.tile;
			part->track = Track::Depot;
			part->x_pos = site.x_pos;
			part->y_pos = site.y_pos;
			part->z_pos = site.z_pos;
			part->direction = site.direction;
			part->vehstatus.Set(VehState::Hidden);
			part->UpdatePosition();
		}
		head->cur_speed = 0;
	}

	/**
	 * Build an excavated company depot bay on Underground: the depot at (40, 40) with its door to the SW and plain
	 * rail on the door axis for the approach.
	 * @param approach Approach tiles to lay.
	 * @return The depot tile.
	 */
	TileIndex BuildDepotBay(uint approach)
	{
		const TileIndex depot_tile = TileXYL(40, 40, MapLayer::Underground);
		std::vector<TileIndex> open{depot_tile};
		for (uint index = 1; index <= approach + 1; ++index) open.push_back(TileXYL(40 + index, 40, MapLayer::Underground));
		this->fixture.world.Open(open);
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, depot_tile, RAILTYPE_RAIL, DiagDirection::SW).Succeeded());
		for (uint index = 1; index <= approach; ++index) {
			REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, TileXYL(40 + index, 40, MapLayer::Underground), RAILTYPE_RAIL, Track::X, false).Succeeded());
		}
		return depot_tile;
	}

	/**
	 * Place a whole train on one tile's track, as if driving there: started, visible, and nowhere near a bay.
	 * @param head Train to place.
	 * @param tile Tile to stand on.
	 */
	void PlaceOnTrack(Train *head, TileIndex tile) const
	{
		for (Train *part = head; part != nullptr; part = part->Next()) {
			part->tile = tile;
			part->track = Track::X;
			part->x_pos = TileX(tile) * TILE_SIZE + TILE_SIZE / 2;
			part->y_pos = TileY(tile) * TILE_SIZE + TILE_SIZE / 2;
			part->vehstatus.Reset(VehState::Hidden);
			part->UpdatePosition();
		}
		head->vehstatus.Reset(VehState::Stopped); // A driving train is not stopped; the depot's stop ends when it leaves.
		head->cur_speed = 0;
	}

	/**
	 * Place a whole train stopped inside a genuine depot, where ordinary moves may rearrange it.
	 * @param head Train to place.
	 * @param depot_tile Depot tile.
	 */
	void PlaceInDepot(Train *head, TileIndex depot_tile) const
	{
		for (Train *part = head; part != nullptr; part = part->Next()) {
			part->tile = depot_tile;
			part->track = Track::Depot;
			part->x_pos = TileX(depot_tile) * TILE_SIZE + TILE_SIZE / 2;
			part->y_pos = TileY(depot_tile) * TILE_SIZE + TILE_SIZE / 2;
			part->vehstatus.Set(VehState::Hidden);
			part->UpdatePosition();
		}
		head->vehstatus.Set(VehState::Stopped);
		head->cur_speed = 0;
	}

	/**
	 * Give the fixture's origin station a real rail station tile at its sign, so a train can stand on the origin.
	 * @return The station tile.
	 */
	TileIndex MakeOriginStationTile() const
	{
		const TileIndex tile = this->fixture.origin->xy;
		MakeRailStation(tile, _current_company, this->fixture.origin->index, Axis::X, 0, RAILTYPE_RAIL);
		this->fixture.origin->train_station = TileArea(tile, 1, 1);
		this->fixture.origin->spread.Add(tile); // The catchment and acceptance are computed from the spread.
		return tile;
	}

	/**
	 * One controller step of an assigned hauler, which must succeed.
	 * @param head Hauler front engine.
	 * @return Its assignment after the step.
	 */
	static TBMExchangeAssignment StepHauler(const Train &head)
	{
		const auto *assignment = GetTBMExchangeAssignmentForHauler(head.index);
		REQUIRE(assignment != nullptr);
		REQUIRE(AdvanceTBMHauler(assignment->id) == TBMExchangeError::None);
		return *GetTBMExchangeAssignmentForHauler(head.index);
	}

	/** @return The rig's hauler's assignment after one controller step of it. */
	TBMExchangeAssignment StepHauler() const { return StepHauler(*this->hauler); }

	/** Put the hauler in the project's source bay, beside the cutter consist. */
	void HaulerToSourceBay() const { this->PlaceAt(this->hauler, *this->front); }

	/** Reserve the hauler and the bay for the project. */
	void Assign() const { REQUIRE(AssignTBMHauler(this->project.id, this->hauler->index, true) == TBMExchangeError::None); }

	/** Take the hauler to the source bay and assign it, whether or not an exchange is due yet. */
	void Meet() const
	{
		this->HaulerToSourceBay();
		this->Assign();
	}

	/** @return The exchange request of the rig's project and hauler. */
	TBMExchangeRequest Request() const { return {this->project.id, this->hauler->index}; }

	/**
	 * @param head Chain head.
	 * @return Tonnes aboard every part.
	 */
	static uint64_t Aboard(const Train *head)
	{
		uint64_t total = 0;
		for (const Train *part = head; part != nullptr; part = part->Next()) total += part->cargo.TotalCount();
		return total;
	}

	/**
	 * Bore the project's cut, leaving its spoil aboard and the exchange due, then take the hauler to the source bay
	 * and assign it.
	 * @return The cut's estimate.
	 */
	ExcavationCutResult BoreAndMeet() const
	{
		const auto estimate = this->BoreOneCut();
		REQUIRE(IsTBMExchangeDue(this->Project()));
		this->Meet();
		return estimate;
	}
};

#endif /* TESTS_TBM_EXCHANGE_FIXTURE_H */
