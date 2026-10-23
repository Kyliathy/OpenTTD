/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file signal_func.h Functions related to signals. @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 6 (UpdateLayerPortalSignals) */

#ifndef SIGNAL_FUNC_H
#define SIGNAL_FUNC_H

#include "track_type.h"
#include "tile_type.h"
#include "direction_type.h"
#include "company_type.h"

/**
 * Maps a trackdir to the bit that stores its status in the map arrays, in the
 * direction along with the trackdir.
 * @param trackdir The track dir to consider.
 * @return Bitmask of the storage of the signal in the associated trackdir in the map array.
 */
inline uint8_t SignalAlongTrackdir(Trackdir trackdir)
{
	extern const TrackdirIndexArray<uint8_t> _signal_along_trackdir;
	return _signal_along_trackdir[trackdir];
}

/**
 * Maps a trackdir to the bit that stores its status in the map arrays, in the
 * direction against the trackdir.
 * @param trackdir The track dir to consider.
 * @return Bitmask of the storage of the signal in the associated trackdir in the map array.
 */
inline uint8_t SignalAgainstTrackdir(Trackdir trackdir)
{
	extern const TrackdirIndexArray<uint8_t> _signal_against_trackdir;
	return _signal_against_trackdir[trackdir];
}

/**
 * Maps a Track to the bits that store the status of the two signals that can
 * be present on the given track.
 * @param track The track to consider.
 * @return Bitmask of the storage of the signal in the associated track in the map array.
 */
inline uint8_t SignalOnTrack(Track track)
{
	extern const TrackIndexArray<uint8_t> _signal_on_track;
	return _signal_on_track[track];
}

/** State of the signal segment */
enum class SigSegState : uint8_t {
	Free, ///< Free and has no pre-signal exits or at least one green exit
	Full, ///< Occupied by a train
	Path, ///< Segment is a path segment
};

SigSegState UpdateSignalsOnSegment(TileIndex tile, DiagDirection side, Owner owner);
void SetSignalsOnBothDir(TileIndex tile, Track track, Owner owner);
void AddTrackToSignalBuffer(TileIndex tile, Track track, Owner owner);
void AddSideToSignalBuffer(TileIndex tile, DiagDirection side, Owner owner);
void UpdateSignalsInBuffer();

/**
 * Update the signal block that starts at a rail portal end, from inside a game tick (r2df task 6.3). Signal work
 * queued earlier in the same tick, such as support track a TBM controller laid, is finished first, because
 * UpdateSignalsOnSegment needs an empty queue.
 * @param tile Rail portal end whose block changed: a train left or entered its magazine, or crossed its shaft.
 * @param owner Owner of the portal end and its signals.
 */
void UpdateLayerPortalSignals(TileIndex tile, Owner owner);

#endif /* SIGNAL_FUNC_H */
