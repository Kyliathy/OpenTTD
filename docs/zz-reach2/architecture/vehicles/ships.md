# Ships

Ships are `Ship` (`src/ship.h`). Shared tick, orders, cargo, and breakdowns are in [archi-vehicles.md](archi-vehicles.md). A ship is one vehicle. `state` holds `TrackBits` on water, `Track::Depot` in a ship depot, or `Track::Wormhole` on an aqueduct. `direction` is the travel heading. `rotation` is the sprite heading. They differ while the ship is turning in place. `path` is the cached trackdir list.

## Tick

`Ship::Tick` counts `running_ticks` when the ship is not stopped, then calls `ShipController`. The controller always returns; `Tick` returns true.

`ShipController`:

1. Increments `tick_counter` and `current_order_time`.
2. `HandleBreakdown` returns on a true result, so a broken-down ship does not move. The breakdown roll in `CheckVehicleBreakdown` adds `0x6666` to ship reliability before the chance-table lookup, on top of the same offset used for reduced breakdowns.
3. `VehState::Stopped` returns.
4. `ProcessOrders`. A true return and `CheckReverseShip` call `ReverseShip` and return. `CheckReverseShip` can call `YapfShipCheckReverse`.
5. `HandleLoading`, then return on `OT_LOADING`.
6. `CheckShipStayInDepot` returns when the ship must stay in the depot.
7. `ShowVisualEffect`.
8. If `direction != rotation`, the ship does not move. Every 8 ticks `rotation` steps one `DirDiff` toward `direction` through `LimitDirDiff`, the sprite cache direction is invalidated, and the viewport is updated.
9. `ShipMoveUpDownOnLock` consumes the tick when the ship is changing level in a lock.
10. `ShipAccelerate` returns a step count. Each step calls `GetNewVehiclePos` and either stays on the tile, enters a new tile, or slides along an aqueduct.

## Speed

`ShipAccelerate` (`src/ship_cmd.cpp`) sets `cur_speed` to the minimum of `cur_speed + acceleration`, `vcache.cached_max_speed`, and `current_order.GetMaxSpeed() * 2`. The step count is `(GetAdvanceSpeed(cur_speed) + progress) / GetAdvanceDistance()`, and `progress` keeps the remainder. `GetAdvanceSpeed` and `GetAdvanceDistance` are the shared ground-vehicle scaling in `src/vehicle_base.h`.

`Ship::UpdateCache` stores `cached_max_speed` from `PROP_SHIP_SPEED` passed through `ApplyWaterClassSpeedFrac`. The fraction depends on `GetEffectiveWaterClass(tile) == WaterClass::Sea`. The comment in `UpdateCache` says aqueducts are always canals. Entering a new tile with a different effective water class calls `UpdateCache` again. The ship pathfinder prices water class on its own; that cost is in [ship pathfinding](../pathfinder/archi-pathfinder-ship.md).

## One step

On the same tile, a depot ship keeps its coordinates. Any other ship calls `VehicleEnterTile`. `CannotEnter` reverses the ship. `OT_LEAVESTATION` is cleared after a single tick, and the ship reverses when the tile ahead has no water tracks, which is the dock it would otherwise sail back into. A waypoint order completes when Manhattan distance from `dest_tile` to the new tile is 3 or less: the timetable updates, `IncrementRealOrderIndex` runs, and `current_order` becomes a dummy. A depot order completes only on the depot tile with both coordinates at offset 8, and then calls `VehicleEnterDepot`. A station order on a docking tile whose station contains that tile calls `ShipArrivesAt` and `BeginLoading` when the station has `StationFacility::Dock`. A station without a dock becomes `OT_LEAVESTATION` and `IncrementRealOrderIndex` immediately. `BeginLoading` allows a ship even when the moving front is not on a station tile.

On a new tile, `GetAvailShipTracks` supplies the water tracks. No tracks calls `CheckReverseShip` and either `ReverseShipIntoTrackdir` or `ReverseShip`. Otherwise `ChooseShipTrack` picks the track:

- `dest_tile == INVALID_TILE` keeps the current diagonal, or the opposite of a non-diagonal, if that track exists, and does not call the pathfinder. `path_found` is false.
- A cached `path` whose track is still in the tile's tracks is popped. `HandlePathfindingResult` is not called for a cache hit.
- Otherwise `YapfShipChooseTrack` fills `v->path`.

`HandlePathfindingResult` updates `VehicleFlag::PathfinderLost`. The ship search and the water-region graph it plans over are in [ship pathfinding](../pathfinder/archi-pathfinder-ship.md) and [water regions](../pathfinder/archi-pathfinder-water-regions.md).

A heading change of 45° or less keeps `cur_speed` and sets `rotation` to the new direction. A larger turn zeroes `cur_speed`, stores `rotation_x_pos` and `rotation_y_pos` so the sprite does not jump, and leaves `rotation` for the in-place turn on later ticks. `ReverseShip` and `ReverseShipIntoTrackdir` do the same speed and path clear. A double-ended engine (`ExtraEngineFlag::IsDoubleEnded`) flips `VehicleShipFlag::SecondEndFacingForward` and reverses `rotation` instead of playing out a 180° turn. Leaving a depot always clears that flag so the first end faces forward.

On an aqueduct (`Track::Wormhole`), the ship advances until `VehicleEnterTile` reports the wormhole again, then pops one cached path entry because it did not choose a track at the bridge head.

## Depots

`CheckShipStayInDepot` returns false when the ship is not in the depot. It waits for unbunching, re-enters through `VehicleEnterDepot` when the current order names this depot, and stays while `dest_tile` is invalid or another ship on the tile has `cur_speed != 0`. Otherwise it faces the depot exit, may reverse through `CheckReverseShip`, unhides, and calls `VehicleServiceInDepot` and `LeaveUnbunchingDepot`.

`FindClosestShipDepot` does not call YAPF. It BFS-walks reachable water-region patches out to `max_distance / WATER_REGION_EDGE_LENGTH + 1` along each axis, then picks the nearest owned ship depot whose tile lies in that set and inside `max_distance`. `CheckIfShipNeedsService` passes `maximum_go_to_depot_penalty / YAPF_TILE_LENGTH` as that distance. `Ship::FindClosestDepot` uses `MAX_SHIP_DEPOT_SEARCH_DISTANCE` and returns the depot tile and index. The patch graph is the one documented with [water regions](../pathfinder/archi-pathfinder-water-regions.md).

`OnNewEconomyDay` ages the ship, decreases value every 8 days, rolls `CheckVehicleBreakdown`, runs the service check and `CheckOrders`, then charges `ExpensesType::ShipRun`.
