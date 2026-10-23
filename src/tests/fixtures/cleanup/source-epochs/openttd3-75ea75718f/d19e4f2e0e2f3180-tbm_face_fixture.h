/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_fixture.h A boring project whose cutter stands at its face with a stand behind it, and helpers to put
 * trains on the work path, in the pair's magazine or on the corridor, for the face exchange tests; and a cutter waiting
 * on its face with spoil aboard and its hauler on the stand, for the face transfer (Group 5).
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Groups 2-7
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-a-face-exchange-design.md
 */

#ifndef TESTS_TBM_FACE_FIXTURE_H
#define TESTS_TBM_FACE_FIXTURE_H

#include "tbm_exchange_fixture.h"
#include "../tbm_exchange.h"
#include "../rail_map.h"
#include "../track_func.h"
#include "../layer_portal_map.h"
#include <map>
#include <tuple>

/**
 * @param head Train to count.
 * @return Number of parts in its chain.
 */
inline size_t CountTrainParts(const Train &head)
{
	size_t parts = 0;
	for (const Train *part = &head; part != nullptr; part = part->Next()) ++parts;
	return parts;
}

/**
 * Stand a train on work-path cells as if it had driven there: the front on one cell and the parts spread evenly back
 * to another, visible on the running line.
 * @param head Train to place.
 * @param path Work path from the housing onward.
 * @param front Path index of the front's cell.
 * @param tail Path index of the rearmost part's cell, at most front.
 */
inline void PlaceTrainOnPath(Train *head, const std::vector<TileIndex> &path, size_t front, size_t tail)
{
	REQUIRE(tail <= front);
	REQUIRE(front < path.size());
	const size_t parts = CountTrainParts(*head);
	size_t i = 0;
	// Part i takes its share of the span, so the front and the rearmost part land exactly on the given cells.
	for (Train *part = head; part != nullptr; part = part->Next(), ++i) {
		const size_t index = parts == 1 ? front : front - i * (front - tail) / (parts - 1);
		const TileIndex tile = path[index];
		part->tile = tile;
		part->track = IsPlainRailTile(tile) ? FindFirstTrack(GetTrackBits(tile)) : Track::X;
		part->x_pos = TileX(tile) * TILE_SIZE + TILE_SIZE / 2;
		part->y_pos = TileY(tile) * TILE_SIZE + TILE_SIZE / 2;
		part->vehstatus.Reset(VehState::Hidden);
		part->UpdatePosition();
	}
	head->cur_speed = 0;
}

/**
 * @param rig Rig whose job is read.
 * @return The work path through the completed cuts.
 */
inline std::vector<TileIndex> CompletedWorkPath(const TBMWorkRig &rig)
{
	std::vector<size_t> ends;
	auto path = GetTBMWorkRailPath(rig.Job(), rig.Job().cursor, ends);
	REQUIRE(ends.size() == rig.Job().cursor);
	return path;
}

/** A project boring on a straight route with a company hauler in a Surface depot, whose cutter can be stood at its face. */
class TBMFaceRig : public TBMExchangeRig {
public:
	size_t parts = 0; ///< Parts of the cutter consist.

	/**
	 * Publish the project and build the hauler.
	 * @param segments Straight cuts of the project.
	 */
	explicit TBMFaceRig(size_t segments = 24) : TBMExchangeRig(segments)
	{
		this->parts = CountTrainParts(*this->front);
	}

	/**
	 * Complete cuts, emptying the wagons after each one as the rig's stand-in exchange does.
	 * @param cuts Number of cuts.
	 */
	void Bore(uint cuts) const
	{
		for (uint i = 0; i < cuts; ++i) {
			this->BoreOneCut();
			this->ExchangeAndReturn();
		}
	}

	/**
	 * Bore enough cuts for a whole consist, one part per cell, to stand clear of the housing with a stand behind it that
	 * also leaves room for a whole hauler beyond the housing (GetTBMHaulerRoomCells, with its spare cells).
	 */
	void BoreClearOfHousing() const
	{
		const uint hauler_cells = CeilDiv(this->hauler->gcache.cached_total_length, TILE_SIZE);
		const uint cuts = static_cast<uint>(this->parts) + hauler_cells + 3;
		REQUIRE(cuts < this->Job().route.size());
		this->Bore(cuts);
	}

	/** @return The work path through the completed cuts. */
	std::vector<TileIndex> Path() const { return CompletedWorkPath(*this); }

	/** Stand the cutter on its face with one part per cell behind it. */
	void StandAtFace() const
	{
		const auto path = this->Path();
		PlaceTrainOnPath(this->front, path, path.size() - 1, path.size() - this->parts);
	}

	/** @return The pair's key on the Surface, which holds the magazine. */
	TileIndex Key() const { return this->fixture.request.portal; }

	/** @return The pair's rock-side housing, where the work path starts. */
	TileIndex Housing() const { return GetLayerPortalPairTile(this->Key()); }

	/**
	 * Store a whole train in the key's magazine, as a hauler queued at the Source bay waits.
	 * @param head Train to store.
	 */
	void StoreInMagazine(Train *head) const
	{
		const TileIndex key = this->Key();
		for (Train *part = head; part != nullptr; part = part->Next()) {
			part->tile = key;
			part->track = Track::Depot;
			part->x_pos = TileX(key) * TILE_SIZE + TILE_SIZE / 2;
			part->y_pos = TileY(key) * TILE_SIZE + TILE_SIZE / 2;
			part->vehstatus.Set(VehState::Hidden);
			part->UpdatePosition();
		}
		head->cur_speed = 0;
	}

	/**
	 * Assign another hauler to the project.
	 * @param earth Empty Earth hoppers.
	 * @param rock Empty Rock hoppers.
	 * @return Its front engine, in the Surface depot.
	 */
	Train *AssignAnother(uint8_t earth = 2, uint8_t rock = 2) const
	{
		Train *head = this->BuildHauler(this->depot, earth, rock);
		REQUIRE(AssignTBMHauler(this->project.id, head->index, true) == TBMExchangeError::None);
		return head;
	}
};

/** Tonnes and feeder share per cargo, source tile, first station and age: what moving packets between trains keeps. */
using TBMPacketTotals = std::map<std::tuple<CargoType, TileIndex, StationID, uint16_t>, std::pair<uint64_t, Money>>;

/**
 * Sum the packets of some trains by what identifies their cargo, so merged or split packets compare equal.
 * @param heads Trains to read.
 * @return The totals.
 */
inline TBMPacketTotals CountTBMPackets(std::initializer_list<const Train *> heads)
{
	TBMPacketTotals totals;
	for (const Train *head : heads) {
		for (const Train *part = head; part != nullptr; part = part->Next()) {
			// A part's packets carry its cargo type; their own fields name where and when the cargo was loaded.
			for (const CargoPacket *packet : *part->cargo.Packets()) {
				auto &total = totals[{part->cargo_type, packet->GetSourceTile(), packet->GetFirstStation(), packet->GetPeriodsInTransit()}];
				total.first += packet->Count();
				total.second += packet->GetFeederShare();
			}
		}
	}
	return totals;
}

/** Everything a refused or estimated face transfer must leave alone, read from the actual stores and trains. */
struct TBMFaceSnapshot {
	AcquisitionDigest digest; ///< Pools, money, random seeds, geology, portal work and counted trains/engines.
	TBMChainIDs cutter; ///< Cutter consist parts in order.
	TBMChainIDs hauler; ///< Hauler parts in order.
	std::map<VehicleID, TBMPacketList> packets; ///< Packets on each part of both trains.
	std::vector<TBMExchangeAssignment> assignments; ///< Assignment store.
	std::vector<TBMWagonSet> sets; ///< Wagon set store.
	std::tuple<TBMPhase, uint64_t, uint32_t> project; ///< The project's phase, phase timer and revision.

	/**
	 * @param rig Exchange rig.
	 * @return The current snapshot.
	 */
	static TBMFaceSnapshot Capture(const TBMExchangeRig &rig)
	{
		TBMFaceSnapshot snapshot{rig.Digest(), GetTBMChainIDs(rig.front), GetTBMChainIDs(rig.hauler), GetTBMPacketOwners(rig.front), CaptureTBMExchangeAssignments(), CaptureTBMWagonSets(), {rig.Project().phase, rig.Project().phase_ticks, rig.Project().revision}};
		snapshot.packets.merge(GetTBMPacketOwners(rig.hauler));
		return snapshot;
	}

	/** @param other Snapshot to compare. @return Whether everything matches. */
	bool operator==(const TBMFaceSnapshot &other) const = default;
};

/**
 * A cutter on its face with one cut's spoil aboard and its exchange due, and the assigned hauler stopped on the stand
 * behind it, heading towards it: the meeting a face transfer needs (r2df2 Group 5).
 */
class TBMFaceTransferRig : public TBMFaceRig {
public:
	/**
	 * Bore clear of the housing, keep the last cut's spoil aboard, and meet at the face.
	 * @param phase Phase the cutter waits in: ReturnToPark (the park request makes the real partial load due without
	 *              another cut) or AwaitFaceTransfer (the last cut reaches a pass breakpoint).
	 * @param dwell Ticks the cutter has already waited in that phase.
	 * @param segments Straight cuts of the project; a breakpoint wait needs one breakpoint beyond the housing's room.
	 */
	explicit TBMFaceTransferRig(TBMPhase phase = TBMPhase::ReturnToPark, uint64_t dwell = TBM_FACE_TRANSFER_TICKS, size_t segments = 24) : TBMFaceRig(segments)
	{
		this->BoreClearOfHousing();
		if (phase == TBMPhase::AwaitFaceTransfer) {
			// Empty after every cut short of the next breakpoint; the cut that reaches it keeps its spoil aboard.
			const auto &after = this->Job().pass_terms->exchange_after;
			std::string listed;
			for (uint16_t cut : after) listed += fmt::format(" {}", cut);
			INFO("cursor " << this->Job().cursor << " of " << this->Job().route.size() << ", breakpoints" << listed);
			while (!std::ranges::binary_search(after, static_cast<uint16_t>(this->Job().cursor + 1))) {
				REQUIRE(this->Job().cursor + 2 < this->Job().route.size());
				this->Bore(1);
			}
		}
		this->BoreOneCut();
		this->StandAtFace();
		this->Reload(phase, [dwell](TBMProject &project) { project.phase_ticks = dwell; });
		REQUIRE(IsTBMExchangeDue(this->Project()));
		// After the reload, which starts the exchange store afresh, as a hauler assigned to the waiting cutter.
		this->Assign();
		this->HaulerOnStand();
	}

	/**
	 * Stand the hauler on the work path behind the cutter, stopped, as if it had driven there.
	 * @param back Cells short of the stand its front stops at; 0 is on the stand.
	 * @param towards Whether it heads towards the cutter, along the stand's trackdir, or away from it.
	 */
	void HaulerOnStand(size_t back = 0, bool towards = true) const
	{
		const auto path = this->Path();
		const auto stand = FindTBMFaceStand(this->Project());
		REQUIRE(stand.has_value());
		const size_t at = static_cast<size_t>(std::ranges::find(path, stand->tile) - path.begin());
		REQUIRE(at < path.size());
		REQUIRE(back < at);
		const size_t front = at - back;
		PlaceTrainOnPath(this->hauler, path, front, front - std::min(front, CountTrainParts(*this->hauler) - 1));
		// Every part heads along the laid straight piece, so the front's trackdir is the stand's or its reverse.
		const DiagDirection exit = TrackdirToExitdir(stand->trackdir);
		const Direction heading = DiagDirToDir(towards ? exit : ReverseDiagDir(exit));
		for (Train *part = this->hauler; part != nullptr; part = part->Next()) part->direction = heading;
		if (back == 0) REQUIRE(this->hauler->GetVehicleTrackdir() == (towards ? stand->trackdir : ReverseTrackdir(stand->trackdir)));
	}

	/**
	 * @param role Spoil role.
	 * @return The hauler's own set wagons of that role.
	 */
	std::vector<Train *> SetWagons(TBMConsistRole role) const
	{
		std::vector<Train *> result;
		const auto *assignment = GetTBMExchangeAssignmentForHauler(this->hauler->index);
		REQUIRE(assignment != nullptr);
		const TBMWagonSet *set = GetTBMWagonSet(assignment->set);
		REQUIRE(set != nullptr);
		const CargoLabel label = role == TBMConsistRole::EarthWagon ? CargoLabel{"ERTH"} : CargoLabel{"ROCK"};
		for (VehicleID id : set->wagons) {
			Train *unit = Train::Get(id);
			if (CargoSpec::Get(unit->cargo_type)->label == label) result.push_back(unit);
		}
		return result;
	}

	/**
	 * Add a kept packet of another source, which cannot merge into the spoil already there.
	 * @param part Wagon part to load.
	 * @param count Tonnes.
	 * @param action What the packet is reserved for.
	 */
	static void AddPacket(Train *part, uint16_t count, VehicleCargoList::MoveToAction action = VehicleCargoList::MoveToAction::Keep)
	{
		REQUIRE(CargoPacket::CanAllocateItem());
		part->cargo.Append(CargoPacket::Create(StationID::Invalid(), count, Source{Source::Invalid, SourceType::Town}), action);
	}
};

#endif /* TESTS_TBM_FACE_FIXTURE_H */
