# Road vehicles

Road vehicles are `RoadVehicle` (`src/roadveh.h`), a `GroundVehicle<RoadVehicle, VehicleType::Road>`. Shared tick, orders, cargo, and breakdowns are in [archi-vehicles.md](archi-vehicles.md). Buses and trucks use the same controller. Trams are the same type with a tram road type; the pathfinder tells them apart.

`IsFrontEngine` is the primary vehicle. Articulated parts follow on `next`. `state` is the road-vehicle frame state (`RVSB_IN_ROAD_STOP`, `RVSB_IN_DT_ROAD_STOP`, and the depot frames). `frame` is the index into the per-tile road path. `overtaking` is non-zero while the vehicle is committed to an overtake. `path` is the cached trackdir list from the last pathfinder call.

## Tick

`RoadVehicle::Tick` increments `tick_counter`. Only the front engine counts `running_ticks` (when not stopped) and calls `RoadVehController`. Every other part returns true. The front's controller moves the articulated parts itself.

`RoadVehController`:

1. Increments `current_order_time` and decrements `reverse_ctr` when it is non-zero.
2. A crash, or `RoadVehCheckTrainCrash`, goes through `RoadVehIsCrashed`.
3. `HandleBreakdown` stops the tick when it returns true.
4. `VehState::Stopped` calls `SetLastSpeed` and returns.
5. `ProcessOrders`, then `HandleLoading` with the default `mode == false`.
6. `OT_LOADING` returns.
7. In a depot, `IsWaitingForUnbunching` waits, otherwise `RoadVehLeaveDepot` on the front.
8. `ShowVisualEffect`, then `UpdateSpeed`. While the returned distance is at least `GetAdvanceDistance`, each articulated part runs `IndividualRoadVehicleController`. A blocked part ends the loop. `progress` is set to `adv_spd - 1` when blocked, so the next tick tries to move again instead of accelerating in place. Otherwise `progress` keeps the unused distance.

`RoadVehicle::UpdateSpeed` calls `DoUpdateSpeed`:

- `AccelerationModel::Original`: acceleration is 512 while `overtaking != 0` and 256 otherwise. Minimum speed is 0.
- `AccelerationModel::Realistic`: acceleration is `GetAcceleration()`, plus 256 while overtaking. Minimum speed is 0 when braking and 4 otherwise.

The maximum is `GetCurrentMaxSpeed`. On the original model, a direction change inside `IndividualRoadVehicleController` subtracts `cur_speed >> 2`. `RoadZPosAffectSpeed` runs after a successful step, with the result of `UpdateInclination`.

## One step

`IndividualRoadVehicleController` advances `frame` along the road path for the current tile, or turns the vehicle around at a dead end. `GetNewVehiclePos` is not the train helper; the road frame table supplies the next `x` and `y` inside the tile. `VehicleEnterTile` can refuse the step (`cur_speed` becomes 0) or report a wormhole, in which case `frame` is not incremented.

A new tile, or a junction where more than one trackdir is available, calls `RoadFindPathToDest`. That function (`src/roadveh_cmd.cpp`):

- With `dest_tile == INVALID_TILE`, picks a random usable trackdir.
- With one trackdir, takes it and drops a stale path cache entry.
- With a cached `path` whose back tile is this tile and whose trackdir is still present, pops that entry.
- Otherwise calls `YapfRoadVehicleChooseTrack` and stores the rest of the path on `v->path`, then `HandlePathfindingResult`.

Track costs, depots, and one-way roads are in [road pathfinding](../pathfinder/archi-pathfinder-road.md). `RoadVehicle::SetDestTile` clears `path` when the destination changes.

Overtaking starts from `RoadVehCheckOvertake` when `RoadVehFindCloseTo` finds another road vehicle in front and `overtaking` is still 0. `CheckRoadBlockedForOvertaking` rejects a tile the vehicle's road types cannot use, a trackdir that does not continue, a junction or barred crossing whose next tile is not one-way, and any extra road vehicle already on the tile. While `overtaking != 0`, `UpdateSpeed` uses the higher acceleration above. The tick does not document a separate overtake physics model beyond that flag and those checks.

A curve costs an extra frame: the comment in the controller says a vehicle spends at least 9 frames on a tile so the next articulated part, moved after the front, can stay within one tile. The short inner curve has 8 frames and is stretched to 10.

## Stops and depots

`RoadVehArrivesAt` sets `StationVehicleType::Bus` or `Truck` the first time, posts the arrival news, and notifies AI and Game Script. The string depends on road versus tram.

`BeginLoading` runs from two places in the controller, both on the front engine:

- Speed has already been forced to 0 inside a drive-through stop (`RVSB_IN_DT_ROAD_STOP`), the order says to stop, the tile owner matches, and the stop type matches bus or truck.
- `frame` has reached the bay stop frame, or the drive-through stop frame, for a stop of the right type. A drive-through continuation whose next bay is free and road-compatible advances `frame` instead of loading. The first arrival clears the entrance-busy bit and sets `RVS_ENTERED_STOP`.

`OT_LEAVESTATION` on a drive-through tile is cleared to `OT_NOTHING` after `VehicleEnterTile` in the same step.

`OnNewEconomyDay` on the front engine ages the vehicle, decreases value every 8 days, calls `CheckVehicleBreakdown` only when `blocked_ctr == 0`, then `CheckIfRoadVehNeedsService` and `CheckOrders`. The service helper uses `FindClosestRoadDepot`, which returns the current tile when the vehicle is already on a road depot and otherwise calls `YapfRoadVehicleFindNearestDepot`. A depot inside `maximum_go_to_depot_penalty` becomes `MakeGoToDepot` with `OrderDepotTypeFlag::Service`. An existing service order is kept, without a new search, on 19 out of 20 random rolls when it is already non-stop.

`FindClosestDepot` is that same search with distance 0 and wraps the tile as a `ClosestDepot`. `RoadVehLeaveDepot` stays inside while any part is off `RVSB_IN_DEPOT` or off the depot tile. On the front, an order whose `dest_tile` is this depot calls `VehicleEnterDepot` again, and a vehicle already sitting on the exit position keeps the consist inside. Otherwise the front is serviced, `LeaveUnbunchingDepot` runs, the chain is unhidden, and `state` becomes the depot exit trackdir at `RVC_DEPOT_START_FRAME`. The same function pulls the next articulated part out when the part ahead reaches `cached_veh_length + RVC_DEPOT_START_FRAME`.
