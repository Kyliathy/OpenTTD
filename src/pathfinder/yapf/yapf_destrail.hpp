/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file yapf_destrail.hpp Rail destinations and canonical native cutter/hauler steering. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 9B; r2mlvs5-spoil-wagons-and-haulage.md, Group 13B (recurring native depot destination); docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 4 (the face stand) */

#ifndef YAPF_DESTRAIL_HPP
#define YAPF_DESTRAIL_HPP

#include "../../train.h"
#include "../../waypoint_base.h"
#include "../../tbm_exchange.h"
#include "../../layer_portal_map.h"
#include "../pathfinder_func.h"

/** Shared rail compatibility and transient native work-face target context for YAPF destination variants. */
class CYapfDestinationRailBase {
protected:
	RailTypes compatible_railtypes;
	bool work_steering_destination = false; ///< Native cutter/hauler work destinations are checked without shared segment caching.
	bool cutter_steering_destination = false; ///< Only cutter work travel retains its existing two-way-signal search policy.

public:
	/** @return Whether this search must stop at its project-owned cutter face, hauler stand or exchange bay instead of skipping that tile inside a segment. */
	bool IsWorkSteeringDestination() const { return this->work_steering_destination; }
	/** @return Whether the target belongs to a cutter whose existing work-travel signal policy applies; hauler targets retain ordinary signal behavior. */
	bool IsCutterSteeringDestination() const { return this->cutter_steering_destination; }

	/** @param v Actual train supplying compatible rail types. @param override_rail_type Whether this search permits every physically compatible type. */
	void SetDestination(const Train *v, bool override_rail_type = false)
	{
		this->compatible_railtypes = v->compatible_railtypes;
		if (override_rail_type) this->compatible_railtypes.Set(GetAllCompatibleRailTypes(v->railtypes));
	}

	/** @param rt Actual candidate track type. @return Whether this train's current search can use that rail. */
	bool IsCompatibleRailType(RailType rt)
	{
		return this->compatible_railtypes.Test(rt);
	}

	/** @return The train-owned rail compatibility used by this search. */
	RailTypes GetCompatibleRailTypes() const
	{
		return this->compatible_railtypes;
	}
};

/** Find an actual rail depot while retaining the train's physical rail compatibility. @tparam Types Derived YAPF search/node family. */
template <class Types>
class CYapfDestinationAnyDepotRailT : public CYapfDestinationRailBase {
public:
	typedef typename Types::Tpf Tpf; ///< the pathfinder class (derived from THIS class)
	typedef typename Types::NodeList::Item Node; ///< this will be our node type
	typedef typename Node::Key Key; ///< key to hash tables

	/** @copydoc CYapfBaseT::Yapf */
	Tpf &Yapf()
	{
		return *static_cast<Tpf *>(this);
	}

	/** @copydoc CYapfBaseT::PfDetectDestinationFunc */
	inline bool PfDetectDestination(Node &n)
	{
		return this->PfDetectDestination(n.GetLastTile(), n.GetLastTrackdir());
	}

	/** @copydoc CYapfBaseT::PfDetectDestinationTileFunc */
	inline bool PfDetectDestination(TileIndex tile, [[maybe_unused]] Trackdir td)
	{
		return IsRailDepotTile(tile);
	}

	/** @copydoc CYapfBaseT::PfCalcEstimateFunc */
	inline bool PfCalcEstimate(Node &n)
	{
		n.estimate = n.cost;
		return true;
	}
};

/** Find a reservation-safe waiting position under the current physical turn rule. @tparam Types Derived YAPF search/node family. */
template <class Types>
class CYapfDestinationAnySafeTileRailT : public CYapfDestinationRailBase {
public:
	typedef typename Types::Tpf Tpf; ///< the pathfinder class (derived from THIS class)
	typedef typename Types::NodeList::Item Node; ///< this will be our node type
	typedef typename Node::Key Key; ///< key to hash tables
	typedef typename Types::TrackFollower TrackFollower; ///< TrackFollower. Need to typedef for gcc 2.95

	/** @copydoc CYapfBaseT::Yapf */
	Tpf &Yapf()
	{
		return *static_cast<Tpf *>(this);
	}

	/** @copydoc CYapfBaseT::PfDetectDestinationFunc */
	inline bool PfDetectDestination(Node &n)
	{
		return this->PfDetectDestination(n.GetLastTile(), n.GetLastTrackdir());
	}

	/** @copydoc CYapfBaseT::PfDetectDestinationTileFunc */
	inline bool PfDetectDestination(TileIndex tile, Trackdir td)
	{
		return IsSafeWaitingPosition(Yapf().GetVehicle(), tile, td, true, !TrackFollower::Allow90degTurns()) &&
				IsWaitingPositionFree(Yapf().GetVehicle(), tile, td, !TrackFollower::Allow90degTurns());
	}

	/** @copydoc CYapfBaseT::PfCalcEstimateFunc */
	inline bool PfCalcEstimate(Node &n)
	{
		n.estimate = n.cost;
		return true;
	}
};

/** Resolve ordinary orders or a live project-owned cutter/hauler destination without changing saved orders. @tparam Types Derived YAPF search/node family. */
template <class Types>
class CYapfDestinationTileOrStationRailT : public CYapfDestinationRailBase {
public:
	typedef typename Types::Tpf Tpf; ///< the pathfinder class (derived from THIS class)
	typedef typename Types::NodeList::Item Node; ///< this will be our node type
	typedef typename Node::Key Key; ///< key to hash tables

protected:
	TileIndex dest_tile;
	TrackdirBits dest_trackdirs;
	StationID dest_station_id;
	bool any_depot;

	/** @copydoc CYapfBaseT::Yapf */
	Tpf &Yapf()
	{
		return *static_cast<Tpf *>(this);
	}

public:
	/** @param v Actual train; project-owned travel destinations are derived independently of its ordinary saved order list. */
	void SetDestination(const Train *v)
	{
		this->any_depot = false;
		this->work_steering_destination = false;
		this->cutter_steering_destination = false;
		if (const auto target = GetTBMCutterSteeringTarget(*v); target.has_value()) {
			// Revisited chamber rails must lead to the live face approach instead of an arbitrary branch toward an empty order.
			this->work_steering_destination = true;
			this->cutter_steering_destination = true;
			Yapf().DisableCache(true);
			this->dest_tile = target->tile;
			this->dest_station_id = StationID::Invalid();
			this->dest_trackdirs = target->trackdirs;
			this->CYapfDestinationRailBase::SetDestination(v);
			return;
		}
		/* Empty returns use the face stand behind the cutter, or the validated Source magazine or genuine depot's own
		 * door, without changing their ordinary origin order. Only the assignment supplies this destination and its
		 * trackdirs, so the search stays outside shared segment caching. */
		if (const auto target = GetTBMHaulerSteeringTarget(*v); target.has_value()) {
			// A closed Source magazine can end a segment as a dead end, rather
			// than a native depot. Recognize the admitted goal before that reason
			// discards a valid empty-return route at a chamber junction (Plan18 8FA).
			this->work_steering_destination = true;
			Yapf().DisableCache(true);
			this->dest_tile = target->tile;
			this->dest_station_id = StationID::Invalid();
			this->dest_trackdirs = target->trackdirs;
			this->CYapfDestinationRailBase::SetDestination(v);
			return;
		}
		// Loaded haulers still follow their ordinary Surface orders. Their
		// support clearance depends on the cutter's current body, so a shared
		// segment cached while that body blocked a junction cannot authorize or
		// permanently refuse a later egress after the physical clearance changes.
		if (GetTBMExchangeAssignmentForHauler(v->First()->index) != nullptr) Yapf().DisableCache(true);
		switch (v->current_order.GetType()) {
			case OT_GOTO_WAYPOINT:
				if (!Waypoint::Get(v->current_order.GetDestination().ToStationID())->IsSingleTile()) {
					/* In case of 'complex' waypoints we need to do a look
					 * ahead. This look ahead messes a bit about, which
					 * means that it 'corrupts' the cache. To prevent this
					 * we disable caching when we're looking for a complex
					 * waypoint. */
					Yapf().DisableCache(true);
				}
				[[fallthrough]];

			case OT_GOTO_STATION:
				this->dest_tile = CalcClosestStationTile(v->current_order.GetDestination().ToStationID(), v->GetMovingFront()->tile, v->current_order.IsType(OT_GOTO_STATION) ? StationType::Rail : StationType::RailWaypoint);
				this->dest_station_id = v->current_order.GetDestination().ToStationID();
				this->dest_trackdirs = INVALID_TRACKDIR_BIT;
				break;

			case OT_GOTO_DEPOT:
				if (v->current_order.GetDepotActionType().Test(OrderDepotActionFlag::NearestDepot)) {
					this->any_depot = true;
				}
				[[fallthrough]];

			default:
				this->dest_tile = v->dest_tile == INVALID_TILE ? TileIndex{} : v->dest_tile;
				this->dest_station_id = StationID::Invalid();
				this->dest_trackdirs = GetTileTrackStatus(this->dest_tile, TransportType::Rail, RoadTramType::Invalid).trackdirs;
				break;
		}
		this->CYapfDestinationRailBase::SetDestination(v);
	}

	/** @copydoc CYapfBaseT::PfDetectDestinationFunc */
	inline bool PfDetectDestination(Node &n)
	{
		return this->PfDetectDestination(n.GetLastTile(), n.GetLastTrackdir());
	}

	/** @copydoc CYapfBaseT::PfDetectDestinationTileFunc */
	inline bool PfDetectDestination(TileIndex tile, Trackdir td)
	{
		if (this->dest_station_id != StationID::Invalid()) {
			return HasStationTileRail(tile)
				&& (GetStationIndex(tile) == this->dest_station_id)
				&& (GetRailStationTrack(tile) == TrackdirToTrack(td));
		}

		if (this->any_depot) {
			return IsRailDepotTile(tile);
		}

		return (tile == this->dest_tile) && this->dest_trackdirs.Test(td);
	}

	/** @copydoc CYapfBaseT::PfCalcEstimateFunc */
	inline bool PfCalcEstimate(Node &n)
	{
		if (this->PfDetectDestination(n)) {
			n.estimate = n.cost;
			return true;
		}

		n.estimate = n.cost + OctileDistanceCost(n.GetLastTile(), n.GetLastTrackdir(), this->dest_tile);
		assert(n.estimate >= n.parent->estimate);
		return true;
	}
};

#endif /* YAPF_DESTRAIL_HPP */
