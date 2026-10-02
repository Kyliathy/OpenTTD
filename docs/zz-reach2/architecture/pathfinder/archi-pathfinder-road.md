# Road YAPF

Road search is `CYapfRoad` and `CYapfRoadAnyDepot` in `src/pathfinder/yapf/yapf_road.cpp`. The node is `CYapfRoadNode` in `yapf_node_road.hpp`. The shared loop is in [archi-pathfinder.md](archi-pathfinder.md).

Both types use `CYapfRoad_TypesT`:

| Role | Class |
| --- | --- |
| Follower | `CFollowTrackRoad` |
| Node list | `CRoadNodeList` = `NodeList<CYapfRoadNode, 8, 10>` |
| Vehicle | `RoadVehicle` |
| Follow | `CYapfFollowRoadT` |
| Origin | `CYapfOriginTileT` (one tile, one or more trackdirs) |
| Cache | `CYapfSegmentCostCacheNoneT` |
| Cost | `CYapfCostRoadT` |
| Destination | `CYapfDestinationTileRoadT` or `CYapfDestinationAnyDepotRoadT` |

There is no 90° variant and no global segment cache. Trams and ordinary road vehicles share this pathfinder; `CFollowTrackT` tells them apart with `RoadTypeIsTram` and `GetTrackdirBitsForRoad`.

## What one node means

`CYapfRoadNode` uses `CYapfNodeKeyExitDir`: two trackdirs on the same tile that leave through the same side are one vertex. `Set` records `segment_last_tile` and `segment_last_td`, initially the node's own tile.

`PfCalcCost` walks forward while the follower returns exactly one trackdir. The walk stops when:

- `PfDetectDestinationTile` matches (so the target tile is not stepped over)
- `parent_cost + segment_cost` exceeds `max_cost` (depot radius); the child is rejected
- the tile is a road depot and the trackdir faces back out of it
- `Follow` fails (end of road)
- the next tile offers more than one trackdir (a junction; the segment ends here and `PfFollowNode` will branch)
- the walk returns to the segment's start tile and trackdir (a simple loop); the child is rejected
- the tile count exceeds `MAX_MAP_SIZE`

Tunnel tiles skipped by the follower add `tiles_skipped * YAPF_TILE_LENGTH` (`YAPF_TILE_LENGTH` is 100, `pathfinder_type.h` line 16). The far end is written back to `segment_last_tile` / `segment_last_td`. `PfFollowNode` continues from that far end.

`is_choice` on the child is set by `AddMultipleNodes` when the follower returned more than one trackdir. The vehicle path cache only stores choice nodes.

## Cost

`OneTileCost` (`yapf_road.cpp`):

- diagonal trackdir: `YAPF_TILE_LENGTH`
- any other trackdir: `YAPF_TILE_CORNER_LENGTH` plus `road_curve_penalty` (`YAPF_TILE_CORNER_LENGTH` is 71, `pathfinder_type.h` line 19)

Diagonal extras, still inside `OneTileCost`:

- level crossing: `road_crossing_penalty`
- drive-through stop (not a road waypoint): `road_stop_penalty`
- first tile of a drive-through queue: `entry.GetOccupied() * road_stop_occupied_penalty / entry.GetLength()`
- bay stop: `road_stop_bay_occupied_penalty * (!IsFreeBay(0) + !IsFreeBay(1)) / 2`

`SlopeCost` compares the pixel Z of the two tile centres. A rise of more than 1 adds `road_slope_penalty`. A descent adds 0.

Speed, using the follower's `GetSpeedLimit` (bridge speed doubled for road, then the road-type max):

- `max_veh_speed` is `min(GetDisplayMaxSpeed(), current_order.GetMaxSpeed() * 2)`
- below that max: add `YAPF_TILE_LENGTH * (max_veh_speed - max_speed) * (4 + tiles_skipped) / max_veh_speed`
- min speed above that: add `YAPF_TILE_LENGTH * (min_speed - max_veh_speed)`

The literal `4` and the `* 2` on the order speed are in `PfCalcCost`.

### Settings

The running value is `_settings_game.pf.yapf.<field>`. Defaults are `def` lines in `src/table/settings/pathfinding_settings.ini`.

| Field | Default | Line |
| --- | --- | --- |
| `road_slope_penalty` | `2 * YAPF_TILE_LENGTH` | 318 |
| `road_curve_penalty` | `1 * YAPF_TILE_LENGTH` | 327 |
| `road_crossing_penalty` | `3 * YAPF_TILE_LENGTH` | 336 |
| `road_stop_penalty` | `8 * YAPF_TILE_LENGTH` | 346 |
| `road_stop_occupied_penalty` | `8 * YAPF_TILE_LENGTH` | 354 |
| `road_stop_bay_occupied_penalty` | `15 * YAPF_TILE_LENGTH` | 363 |

Depot search also honours `max_cost`, set from the caller's `max_distance`. `RoadVehicle::FindClosestDepot` passes 0, and `SetMaxCost(0)` disables the cutoff (`max_cost > 0` is the guard in `PfCalcCost`).

## Destination

`CYapfDestinationTileRoadT::SetDestination`:

- go-to station: bus or truck station id from `v->IsBus()`, closest tile via `CalcClosestStationTile`. A non-articulated vehicle may stop on a bay; an articulated vehicle must land on a drive-through stop (`non_artic` is `!HasArticulatedPart()`, and `PfDetectDestinationTile` requires `IsDriveThroughStopTile` when `non_artic` is false).
- go-to waypoint: `StationType::RoadWaypoint`, same articulated rule
- any other order: `v->dest_tile` plus the road trackdirs on that tile

`PfCalcEstimate` uses `OctileDistanceCost` from the segment end and asserts `estimate >= parent->estimate`. On the destination, `estimate = cost`.

`CYapfDestinationAnyDepotRoadT` matches `IsRoadDepotTile` on the segment end and sets `estimate = cost`.

`GetDestinationStation` is how the path-cache trim below finds the station area. It returns null when the order is not a station or waypoint id.

## Vehicle path cache

This is `RoadVehicle::path` (`RoadVehPathCache` in `roadveh.h`): a vector of `{trackdir, tile}`, back element first. It is not the rail segment-cost cache. `CYapfSegmentCostCacheNoneT` means every `PfCalcCost` recomputes the segment.

`ChooseRoadTrack` walks from the best node to the origin. It counts hops, then on the way back pushes a node when `GetIsChoice()` is set and the remaining hop count after the decrement is `< YAPF_ROADVEH_PATH_CACHE_SEGMENTS`. That constant is 8 (`pathfinder_type.h` line 30). Pushing from the destination end toward the origin leaves the next choice at `back()`, which `RoadFindPathToDest` pops.

If the destination station has a drive-through primary stop, or another road stop behind it, the cache is trimmed. The bus or truck station `TileArea` is expanded by `YAPF_ROADVEH_PATH_CACHE_DESTINATION_LIMIT` (8, `pathfinder_type.h` line 33). Entries from the start of the vector (the destination side) are erased while their tile sits inside that expanded area, so the vehicle pathfinds again as it gets close to a multi-stop station.

`RoadFindPathToDest` (`roadveh_cmd.cpp`) clears the cache when:

- the tile has no trackdir reachable from the enter direction
- the tile has exactly one trackdir but the cache expected a choice on this tile
- the cache's back tile is not this tile
- the cached trackdir is no longer in the reachable set

There is no `YapfNotifyTrackLayoutChange` for road geometry. A stale cache survives until the vehicle reaches a choice and the checks above fail.

Special case inside `ChooseRoadTrack`: if `tile == v->dest_tile` and the order is not go-to station, YAPF is skipped and the diagonal trackdir of `enterdir` is returned. The comment says a station's initial destination tile may be a junction, and taking this shortcut there makes the vehicle loop.

## Call sites

| Caller | Function | File |
| --- | --- | --- |
| Road movement, two sites | `RoadFindPathToDest` → `YapfRoadVehicleChooseTrack` | `src/roadveh_cmd.cpp` |
| `RoadVehicle::FindClosestDepot` | `FindClosestRoadDepot` → `YapfRoadVehicleFindNearestDepot` | `src/roadveh_cmd.cpp`, `yapf_road.cpp` |

`RoadFindPathToDest` does not call YAPF when there is no destination (random trackdir via `RandomRange`), when only one trackdir is legal, or when the path cache hits. Red-signal trackdirs are stripped by `FilterRedSignal` after YAPF returns.
