# Rail YAPF

Rail search is the `CYapfT` bundle in `src/pathfinder/yapf/yapf_rail.cpp`, with cost in `yapf_costrail.hpp`, destinations in `yapf_destrail.hpp`, and the node in `yapf_node_rail.hpp`. The shared loop, open list, and cache flush are in [archi-pathfinder.md](archi-pathfinder.md).

`_settings_game.pf.forbid_90_deg` selects the follower. `false` uses `CFollowTrackRail` (90° allowed). `true` uses `CFollowTrackRailNo90`. Safe-tile search uses the masking variants `CFollowTrackFreeRail` and `CFollowTrackFreeRailNo90`, which drop reserved tracks inside `Follow`.

## The six pathfinders

`CYapfRail_TypesT` binds the same base, origin, cache, and cost for every rail search. Destination and follower change:

| Type | Destination | Follower | Used by |
| --- | --- | --- | --- |
| `CYapfRail` / `CYapfRailNo90` | `CYapfDestinationTileOrStationRailT` | `CYapfFollowRailT` | `YapfTrainChooseTrack`, `YapfTrainCheckReverse` |
| `CYapfAnyDepotRail` / `CYapfAnyDepotRailNo90` | `CYapfDestinationAnyDepotRailT` | `CYapfFollowAnyDepotRailT` | `YapfTrainFindNearestDepot` |
| `CYapfAnySafeTileRail` / `CYapfAnySafeTileRailNo90` | `CYapfDestinationAnySafeTileRailT` | `CYapfFollowAnySafeTileRailT` | `YapfTrainFindNearestSafeTile` |

All six set `PfCache` to `CYapfSegmentCostCacheGlobalT` and `PfOrigin` to `CYapfOriginTileTwoWayT`. The node list is `CRailNodeList` (`NodeList<CYapfRailNode, 8, 10>`).

`CYapfRailBase` adds `PruneIntermediateNodeBranch`. When a red two-way signal ends a branch, the best intermediate node might sit on the near side of an earlier rail-type change. The prune walks parents back to the node whose segment ended because a choice follows, and if the intermediate was on the pruned branch it is moved to that junction. The train then does not keep heading at the red signal.

## What one node means

`CYapfRailNode` extends `CYapfNodeT<CYapfNodeKeyTrackDir, …>`. The key is the tile and trackdir where the segment **starts**. Inherited from the parent, and reset on an origin node:

- `num_signals_passed`
- `target_seen`, `choice_seen`, `last_signal_was_red`
- `last_red_signal_type`, `last_signal_type`

An origin node's `last_signal_type` is `SignalType::Path`. The comment in `Set` says a train that is pathfinding from a safe waiting point is already inside a PBS section and must count reservation cost; in a plain block the train should be alone, so that cost is zero anyway.

`PfFollowNode` calls `CFollowTrackT::Follow` from the segment's **last** tile and trackdir, then `AddMultipleNodes` for every trackdir the follower returns. Children are new segment starts.

## Segment cache

`CYapfRailSegment` (`yapf_node_rail.hpp`) is the cached payload: last tile, last trackdir, `cost` (negative until computed), last signal tile and trackdir, and `end_segment_reason`. The segment key packs `tile << 4 | trackdir` (`CYapfRailSegmentKey::Set`).

`CYapfCostRailT::CanUseGlobalCache` is true only when the cache is enabled, the node has a parent, and `parent->num_signals_passed` is at least the length of the look-ahead cost array. That array has one entry per `rail_look_ahead_max_signals` (default 10, `pathfinding_settings.ini`).

`ConnectNodeToCachedData` points the node at the cached or local segment. A fresh segment copies the node's tile and trackdir into `last_tile` / `last_td` so the walk has a defined end even if it aborts immediately.

`DisableCache(true)` is used for:

- complex (multi-tile) waypoint orders, because the occupancy look-ahead writes segment state that would poison the cache (`CYapfDestinationTileOrStationRailT::SetDestination` in `yapf_destrail.hpp`)
- depot search when `max_penalty != 0` (`stFindNearestDepotTwoWay`)
- the second, comparison search when desync logging is enabled

## Cost walk

`PfCalcCost` (`yapf_costrail.hpp`) walks from the segment start until an `EndSegmentReason` fires. The comment in that function splits the node cost into three parts. The first transition (curve and slip from the parent) is `segment_entry_cost` and is **not** stored in the cache. Later transitions and tile costs are `segment_cost` and are stored. `extra_cost` is applied only on this path (target penalties, speed limits, occupied waypoints) and is not stored.

On a cache hit the walk skips straight to the stored end tile, copies `segment.cost`, and refreshes `last_signal_was_red` from the live signal state on `last_signal_tile`.

`MAX_SEGMENT_COST` is 10000 (`yapf_costrail.hpp`, the `static constexpr` in `CYapfCostRailT`). Above that, the walk closes the segment on a plain railway tile with `EndSegmentReason::SegmentTooLong`.

Reasons that abort the branch when the destination was not seen are `ESRF_ABORT_PF_MASK` (`yapf_type.hpp`): dead end, path too long, infinite loop, first two-way red. Reasons that can be written back into a cached segment are `ESRF_CACHED_MASK`. `PathTooLong`, `FirstTwoWayRed`, `LookAheadEnd`, and `TargetReached` are internal and must not appear in the cache.

Base tile cost is `OneTileCost`:

- diagonal trackdir: `YAPF_TILE_LENGTH` (`pathfinder_type.h` line 16, value 100)
- any other trackdir: `YAPF_TILE_CORNER_LENGTH` (`pathfinder_type.h` line 19, value 71)
- diagonal level crossing: also `rail_crossing_penalty`

Skipped tunnel, bridge, or station tiles add `YAPF_TILE_LENGTH * follower->tiles_skipped`.

### Penalties that are settings

The running value is `_settings_game.pf.yapf.<field>`. The default is the `def` line in `src/table/settings/pathfinding_settings.ini`. Application site is `yapf_costrail.hpp` unless noted.

| Field | Default (`pathfinding_settings.ini`) | Where it is added |
| --- | --- | --- |
| `rail_slope_penalty` | `2 * YAPF_TILE_LENGTH` (line 165) | `SlopeCost` when `CYapfCostBase::stSlopeCost` is true (uphill diagonal, including a bridge ramp that is not a flat ramp; tunnel mouths are excluded) |
| `rail_curve45_penalty` | `1 * YAPF_TILE_LENGTH` (line 174) | `CurveCost` when the next trackdir is not `NextTrackdir` of the previous and the step is not a 90° cross |
| `rail_curve90_penalty` | `6 * YAPF_TILE_LENGTH` (line 183) | `CurveCost` when 90° turns are allowed and `TrackdirCrossesTrackdirs` hits |
| `rail_doubleslip_penalty` | `1 * YAPF_TILE_LENGTH` (line 273) | `SwitchCost` when both plain-rail tiles have more than one track reaching across the shared side |
| `rail_crossing_penalty` | `3 * YAPF_TILE_LENGTH` (line 201) | `OneTileCost` on a diagonal level crossing |
| `rail_depot_reverse_penalty` | `50 * YAPF_TILE_LENGTH` (line 192) | segment cost when the walk stays on the same depot tile (a reverse inside the depot) |
| `rail_station_penalty` | `10 * YAPF_TILE_LENGTH` (line 156) | per platform tile while the station is treated as a station the train passes through |
| `rail_firstred_penalty` | `10 * YAPF_TILE_LENGTH` (line 120) | first signal on the path, red, type block or entry (`SignalCost`) |
| `rail_firstred_exit_penalty` | `100 * YAPF_TILE_LENGTH` (line 129) | first signal on the path, red, type combo or exit |
| `rail_lastred_penalty` | `10 * YAPF_TILE_LENGTH` (line 138) | extra cost once the destination is seen and the last signal was red and is not an exit or a PBS signal; also added for a multi-tile waypoint whose far end is not a free safe waiting position |
| `rail_lastred_exit_penalty` | `100 * YAPF_TILE_LENGTH` (line 147) | extra cost when the destination is seen and the last red signal is an exit; also when the next tile is the back of a one-way path signal while searching for a safe tile |
| `rail_pbs_cross_penalty` | `3 * YAPF_TILE_LENGTH` (line 246) | `ReservationCost` per reserved overlapping tile, scaled by `YAPF_TILE_CORNER_LENGTH / YAPF_TILE_LENGTH` when the trackdir is not diagonal |
| `rail_pbs_station_penalty` | `8 * YAPF_TILE_LENGTH` (line 255) | `ReservationCost` per tile of a reserved station platform |
| `rail_pbs_signal_back_penalty` | `15 * YAPF_TILE_LENGTH` (line 264) | passing the back of a PBS signal while `num_signals_passed` is still below `rail_look_ahead_max_signals` |
| `rail_longer_platform_penalty` | `8 * YAPF_TILE_LENGTH` (line 282) | destination station longer than the train (`PlatformLengthPenalty`) |
| `rail_longer_platform_per_tile_penalty` | `0 * YAPF_TILE_LENGTH` (line 291) | multiplied by the extra platform tiles |
| `rail_shorter_platform_penalty` | `40 * YAPF_TILE_LENGTH` (line 300) | destination station shorter than the train |
| `rail_shorter_platform_per_tile_penalty` | `0 * YAPF_TILE_LENGTH` (line 309) | multiplied by the missing platform tiles |
| `rail_look_ahead_signal_p0` | 500 (line 219) | constant term of the per-signal look-ahead polynomial |
| `rail_look_ahead_signal_p1` | -100 (line 228) | linear term |
| `rail_look_ahead_signal_p2` | 5 (line 237) | quadratic term |
| `rail_look_ahead_max_signals` | 10 (line 210) | length of that polynomial table |
| `rail_firstred_twoway_eol` | true (line 113) | when set, and this search asked for it, the first red two-way signal after a choice is a dead end |

Look-ahead cost for the i-th signal is `p0 + i * (p1 + i * p2)`, computed once in the `CYapfCostRailT` constructor. A red non-PBS signal adds that value when it is positive. A green signal **subtracts** it when it is negative, which turns a negative polynomial into a positive penalty (`cost -= look_ahead_cost` in `SignalCost`). Signals past the end of the table add 0.

`ReservationCost` returns 0 once `num_signals_passed` reaches half the look-ahead table, and whenever `last_signal_type` is not a PBS signal.

Platform length at a destination replaces the pass-through station penalty: extra cost subtracts `rail_station_penalty * platform_length`, then adds `PlatformLengthPenalty`. Missing length is `CeilDiv(cached_total_length, TILE_SIZE) - platform_length`.

Speed, only inside the look-ahead window (`yapf_costrail.hpp`, the `min_speed` / `max_speed` block):

- if the tile's max speed is below the train's display max speed capped by the order: add `YAPF_TILE_LENGTH * (max_veh_speed - max_speed) * (4 + tiles_skipped) / max_veh_speed`
- if the tile's min speed is above that: add `YAPF_TILE_LENGTH * (min_speed - max_veh_speed)`

The literal `4` is in that expression. `GetSpeedLimit` (`follow_track.hpp`) fills max speed from the bridge spec and the rail type.

`YapfTrainCheckReverse` (`yapf_rail.cpp`) can also write a reverse-origin penalty before search:

- `DRIVING_BACKWARDS_PENALTY` is `100 * YAPF_TILE_LENGTH` (local constexpr in `YapfTrainCheckReverse`), added when the train is not already driving backwards and the difficulty setting disallows flip-reverse unless the last wagon can lead
- wormhole distance: `DistanceManhattan` times `YAPF_TILE_LENGTH`, subtracted for the forward end and added for the rear end
- if the penalty is still 0 it is set to 1, so the winning origin is detectable (`node->cost != 0` means the reverse origin won)

`YapfTrainFindNearestDepot` passes `YAPF_INFINITE_PENALTY` (`pathfinder_type.h` line 27) as the reverse penalty into `stFindNearestDepotTwoWay`.

## Destination

`CYapfDestinationTileOrStationRailT::SetDestination`:

- go-to station: closest rail station tile of that station (`CalcClosestStationTile`), match any trackdir on a tile of that station whose rail track equals the arrival track
- go-to waypoint: same with `StationType::RailWaypoint`; a waypoint that is not a single tile disables the segment cache
- go-to depot with the nearest-depot flag: any rail depot tile (`any_depot`)
- otherwise: `v->dest_tile`, and the trackdirs `GetTileTrackStatus` reports there

`PfCalcEstimate` returns `cost` on the destination itself, otherwise `cost + OctileDistanceCost` from the segment end, and asserts the estimate is at least the parent's.

`CYapfDestinationAnyDepotRailT` matches `IsRailDepotTile`. `CYapfDestinationAnySafeTileRailT` matches `IsSafeWaitingPosition` and `IsWaitingPositionFree`. Both set `estimate = cost`.

Compatible rail types come from the train. Safe-tile search can widen them with `GetAllCompatibleRailTypes` when `override_railtype` is set (`SetDestination` on `CYapfDestinationRailBase`).

## Reservation

`CYapfFollowRailT` and `CYapfFollowAnySafeTileRailT` inherit `CYapfReserveTrack`. After a path is found, `ChooseRailTrack` sets the reservation target to the best node's last tile, then walks back toward the origin. `FindSafePositionOnNode` shortens that target to a safe waiting tile when the parent has passed fewer than 2 signals.

`TryReservePath` walks the node chain and reserves each tile (`TryReserveRailTrack`, or the whole station platform). Opposing green path signals are turned red and remembered. Failure unreserves and restores those signals. A reserved target node that `CanUseGlobalCache` accepts triggers `YapfNotifyTrackLayoutChange(INVALID_TILE, Track::Invalid)`, which bumps `s_rail_change_counter` and drops the segment cache on the next search.

`ChooseRailTrack` only calls `TryReservePath` when the caller passed `reserve_track` and `FindPath` returned true. `path_found` is also forced true when the search stopped on the first red two-way signal (`stopped_on_first_two_way_signal`).

`CYapfFollowAnySafeTileRailT::PfFollowNode` masks reserved tracks before adding children. `FindNearestSafeTile` reserves unless the desync comparison asked it not to.

## Call sites in `train_cmd.cpp`

| Caller | YAPF function | When |
| --- | --- | --- |
| `ChooseTrainTrack` via `DoTrainPathfind` | `YapfTrainChooseTrack` | Reservation walk hit a choice or a possible target that is not yet safe |
| `TryReserveSafeTrack` | `YapfTrainFindNearestSafeTile` | No order target could be reserved; search any safe tile. A second call passes `override_railtype = true` |
| `CheckReverseTrain` | `YapfTrainCheckReverse` | Diagonal movement, not in a depot or wormhole, and the difficulty setting allows reverse other than at end of line |
| `FindClosestTrainDepot` | `YapfTrainFindNearestDepot` | `Train::FindClosestDepot` |
