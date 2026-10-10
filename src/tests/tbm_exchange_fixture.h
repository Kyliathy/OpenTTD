/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange_fixture.h A TBM work rig with a real hauler for the wagon exchange tests: the original locomotive
 * with empty Earth and Rock hoppers, built and coupled in a real Surface depot, then placed in the project's bay, on
 * track or in a depot as its hauler cycle (Plan 5 Group 6) expects. Neither train can yet drive itself (master Group 7).
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Groups 1-11 (staged store copies: Group 10)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md, task 10.3 (packet custody audit)
 * @see docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-h-exchange-design.md
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
#include "../station_base.h"
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

/**
 * Temporary packet custody audit (Plan 15 10.3): every cargo packet belongs to exactly one container, a vehicle's or a
 * station's cargo list. The TBM stores name wagons, never packets, and receiver stock is a plain industry counter, so
 * neither can become a second owner of the same tonnes.
 */
class TBMPacketCustody {
public:
	/** @return Whether no packet is in two lists and none made since the audit began is in no list while still allocated. */
	bool IsSole() const
	{
		const auto held = Held();
		const std::set<CargoPacketID> ids(held.begin(), held.end());
		if (ids.size() != held.size()) return false; // One packet in two lists.
		// A packet that leaves its last list is delivered and deleted; one still allocated outside every list was lost.
		for (const CargoPacket *packet : CargoPacket::Iterate()) {
			if (!ids.contains(packet->index) && !this->unheld.contains(packet->index)) return false;
		}
		return true;
	}

private:
	/** Packets outside every list when the audit began, such as those an earlier fixture's pool cleaning left behind. */
	std::set<CargoPacketID> unheld = Unheld();

	/** @return Every packet reference of every vehicle and station cargo list, duplicates included. */
	static std::vector<CargoPacketID> Held()
	{
		std::vector<CargoPacketID> held;
		for (const Vehicle *v : Vehicle::Iterate()) {
			for (const CargoPacket *packet : *v->cargo.Packets()) held.push_back(packet->index);
		}
		// Station lists hold transferred or unloaded cargo; a goods entry without data holds none.
		for (const Station *st : Station::Iterate()) {
			for (const GoodsEntry &ge : st->goods) {
				if (!ge.HasData()) continue;
				const StationCargoPacketMap *packets = ge.GetData().cargo.Packets();
				for (StationCargoList::ConstIterator it(packets->begin()); it != packets->end(); it++) held.push_back((*it)->index);
			}
		}
		return held;
	}

	/** @return Allocated packets in no list. */
	static std::set<CargoPacketID> Unheld()
	{
		const auto held = Held();
		const std::set<CargoPacketID> ids(held.begin(), held.end());
		std::set<CargoPacketID> unheld;
		for (const CargoPacket *packet : CargoPacket::Iterate()) {
			if (!ids.contains(packet->index)) unheld.insert(packet->index);
		}
		return unheld;
	}
};

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
		this->BuildDepotAndHauler(earth, rock);
	}

	/**
	 * Build the hauler with two hoppers of each kind in the depot, for a project whose route turns as given.
	 * @param turns Change of heading at each advance (see TBMProjectRig).
	 */
	explicit TBMExchangeRig(const std::vector<DirDiff> &turns) : TBMWorkRig(turns)
	{
		this->BuildDepotAndHauler(2, 2);
	}

	/**
	 * Build the Surface depot and the hauler in it.
	 * @param earth Empty Earth hoppers behind the locomotive.
	 * @param rock Empty Rock hoppers after them.
	 */
	void BuildDepotAndHauler(uint8_t earth, uint8_t rock)
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
		// The command kernel finishes queued signals after each command; the depot's inside entry must not outlive the
		// depot, which a test may remove before the fixture flushes the rest.
		UpdateSignalsInBuffer();
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

/** One loaded copy of the exchange store, as the save chunks stage it. */
struct TBMExchangeSave {
	std::vector<TBMExchangeAssignment> assignments = CaptureTBMExchangeAssignments(); ///< Assignments in identity order.
	uint32_t next_assignment = GetNextTBMExchangeAssignmentID(); ///< Assignment cursor.
	std::vector<TBMWagonSet> sets = CaptureTBMWagonSets(); ///< Sets in identity order.
	uint32_t next_set = GetNextTBMWagonSetID(); ///< Set cursor.

	/**
	 * Clear the live store, stage this copy and try to publish it.
	 * @return Whether it was accepted.
	 */
	bool Load() const
	{
		ResetTBMExchangeState();
		StageLoadedTBMExchange(this->assignments, this->next_assignment, this->sets, this->next_set);
		CHECK(CaptureTBMExchangeAssignments().empty()); // Staged rows are not live until accepted.
		return AcceptLoadedTBMExchange();
	}

	/** Require this copy to refuse and leave the whole store, cursors included, empty. */
	void Refuse() const
	{
		CHECK_FALSE(this->Load());
		CHECK(CaptureTBMExchangeAssignments().empty());
		CHECK(CaptureTBMWagonSets().empty());
		CHECK(GetNextTBMExchangeAssignmentID() == 0);
		CHECK(GetNextTBMWagonSetID() == 0);
	}

	/** @return Whether this copy loads and then equals the live store, as a save and reload of it would. */
	bool Reloads() const
	{
		return this->Load() && this->IsLive();
	}

	/** @return Whether the live store equals this copy, cursors included. */
	bool IsLive() const
	{
		return CaptureTBMExchangeAssignments() == this->assignments && CaptureTBMWagonSets() == this->sets && GetNextTBMExchangeAssignmentID() == this->next_assignment && GetNextTBMWagonSetID() == this->next_set;
	}

	/**
	 * @param state Set state.
	 * @return The copy's set in that state, which must exist.
	 */
	TBMWagonSet &Set(TBMWagonSetState state)
	{
		const auto found = std::ranges::find(this->sets, state, &TBMWagonSet::state);
		REQUIRE(found != this->sets.end());
		return *found;
	}
};

/** A rig after its first exchange: the cutter rides its Assigned set and the hauler takes its Loaded set to the origin. */
struct TBMExchangeLoadRig : TBMExchangeRig {
	TileIndex origin = INVALID_TILE; ///< Origin station tile.
	uint64_t spoil = 0; ///< Tonnes collected.

	/** Bore, exchange in the source bay and leave towards the origin. */
	TBMExchangeLoadRig()
	{
		this->origin = this->MakeOriginStationTile();
		const auto estimate = this->BoreOneCut();
		this->spoil = estimate.earth_t + estimate.rock_t;
		REQUIRE(this->Tick().phase == TBMPhase::WaitForEmpties);
		this->Meet();
		REQUIRE(this->StepHauler().phase == TBMHaulerPhase::AwaitExchange);
		REQUIRE(this->StepHauler().phase == TBMHaulerPhase::CollectLoaded);
		this->PlaceOnTrack(this->hauler, TileXYL(25, 20, MapLayer::Surface));
		REQUIRE(this->StepHauler().phase == TBMHaulerPhase::VisitOrigin);
	}
};

#endif /* TESTS_TBM_EXCHANGE_FIXTURE_H */
