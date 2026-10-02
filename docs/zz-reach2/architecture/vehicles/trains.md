# Trains

Trains are `Train` (`src/train.h`), a `GroundVehicle<Train, VehicleType::Train>`. Shared tick, orders, cargo, and breakdowns are in [archi-vehicles.md](archi-vehicles.md). This page is what `Train::Tick` does that the other modes do not.

The chain is the consist. `IsFrontEngine` is the primary vehicle. Articulated parts, wagons, and the rear of a multiheaded engine are further `Train` objects on `next`. `other_multiheaded_part` links the two ends of a multiheaded engine. `track` is the current `TrackBits`, or `Track::Depot`, or `Track::Wormhole` inside a tunnel or bridge. `VehicleFlag::DrivingBackwards` on the front selects which end is `GetMovingFront`.

## Tick

`Train::Tick` (`src/train_cmd.cpp`) increments `tick_counter`. On the front engine it counts `running_ticks` when the train is not stopped or `cur_speed` is still above zero, increments `current_order_time` once, then calls `TrainLocoHandler` twice: first with `mode == false`, then with `mode == true` if the train still exists. The first pass deletes a crashed consist through `HandleCrashedTrain`. The second pass keeps a crashed consist. A free wagon that is crashed is not a front engine; its tick only advances `crash_anim_pos` and deletes the chain at 4400.

`TrainLocoHandler` on a live train:

1. `HandleBreakdown`. A true return leaves the train where it is. Because this function runs twice per tick, `HandleBreakdown` decrements `breakdown_delay` only when `(tick_counter & 3) == 0`.
2. If `VehicleRailFlag::Reversing` is set and speed is 0, `ReverseTrainDirection`.
3. A stopped train with `cur_speed == 0` returns.
4. `ProcessOrders`. A true return plus `CheckReverseTrain` zeroes speed and `subspeed`, clears `LeavingStation`, reverses, and returns. `CheckReverseTrain` is false in a depot, in a wormhole, off a diagonal, or when `train_flip_reverse_allowed` is end-of-line only. Otherwise it calls `YapfTrainCheckReverse`. The pathfinder page covers that search.
5. `LeavingStation` tries `TryPathReserve` when the segment is a path block or `pf.reserve_paths` is set, then clears the flag.
6. `HandleLoading(mode)`. The second pass passes `mode == true`, so a train cannot leave the station on the second pass of the same tick.
7. `OT_LOADING` returns. `CheckTrainStayInDepot` returns when the consist must remain in the depot.
8. The first pass (`mode == false`) calls `ShowVisualEffect`. A new order after `OT_NOTHING` calls `CheckNextTrainTile`.
9. A stuck train increments `wait_counter` and usually returns. Every `path_backoff_interval` ticks, and when `reverse_at_signals` says it is time to turn, it calls `TryPathReserve`. Failure can `ReverseTrainDirection` and, after `2 * wait_for_pbs_path * DAY_TICKS`, posts the stuck-train news. `force_proceed != TFP_NONE` clears the stuck flag and continues.
10. `OT_LEAVESTATION` becomes `OT_NOTHING` and the handler returns, so the train does not also move on the departure tick.
11. `UpdateSpeed` returns a distance. While that distance is at least `GetAdvanceDistance` of the moving front, `TrainCheckIfLineEnds` runs and `TrainController` steps the consist. `CheckTrainCollision` stops the loop. A via-station or waypoint on the current tile calls `ProcessOrders` again so a full-speed pass still advances the order. Leftover distance is stored in `progress`.

`TrainForceProceeding` is `TFP_NONE`, `TFP_STUCK` (ignore being stuck until the next signal, including leaving a depot), or `TFP_SIGNAL` (pass the next signal, then stop ignoring). `CmdForceTrainProceed` sets it through `DetermineNextTrainForceProceeding`.

## Speed

`Train::UpdateSpeed` calls `DoUpdateSpeed` (`src/ground_vehicle.hpp`; the carry into `cur_speed` is in the shared page).

- `AccelerationModel::Original`: acceleration is `this->acceleration * 2`, or `this->acceleration * -4` when `GetAccelerationStatus` is `AS_BRAKE`. Minimum speed is 0. Maximum is `GetCurrentMaxSpeed`.
- `AccelerationModel::Realistic`: acceleration is `GetAcceleration()`. Minimum speed is 0 when braking and 2 otherwise.

`AffectSpeedByZChange` runs only for the original model. It uses `_accel_slowdown` for the train's acceleration type (normal, monorail, maglev): uphill subtracts `cur_speed * z_up >> 8`, downhill adds `z_down` up to `cached_max_track_speed`. `Train::GetPower` (`src/train.h`) returns 0 when `HasPowerOnRail` rejects the tile, for an articulated part, or for a multiheaded part after the power is halved. Powered wagons use `pow_wag_power` from the front engine when `VehicleRailFlag::PoweredWagon` is set.

While `breakdown_ctr > 1`, `TrainCheckIfLineEnds` sets `VehState::TrainSlowing` and caps `cur_speed` at `_breakdown_speeds[GB(~breakdown_ctr, 4, 4)]`. The table is 16 values from 225 down to 15. At a line end or a red signal, `TrainApproachingLineEnd` uses the same table from the position inside the tile.

## One step

`TrainController` (`src/train_cmd.cpp`) walks from the moving front toward `nomove` (null from the tick, so the whole consist). Each part calls `GetNewVehiclePos`.

On the same tile, a depot part stays put. Any other part checks `TrainCheckIfLineEnds` on the moving front, then `VehicleEnterTile`. `EnteredStation` calls `TrainEnterStation`, which records `last_station_visited`, may post the first-arrival news, and calls `BeginLoading`.

On a new tile the moving front picks a track with `ChooseTrainTrack`. That extends the reservation with `ExtendTrainReservation` and, at a choice, calls `DoTrainPathfind`, which returns `YapfTrainChooseTrack` (`src/pathfinder/yapf/yapf.h`). Arguments are the consist, the tile about to be entered, the enter direction, and the usable `TrackBits`. Reservation and cost rules are in [rail pathfinding](../pathfinder/archi-pathfinder-rail.md). `CheckCompatibleRail` requires the tile owner to match and, for the front engine when rail type is checked, `compatible_railtypes` to include `GetRailType(tile)`. `Rail90DegTurnDisallowed` drops the crossing track bit for the front; wagons may still take the 90° track because the setting can change mid-turn. The tick calls `TrainController` with `reverse` left at its default. A failed step on that path zeroes `cur_speed` and `subspeed` and calls `ReverseTrainDirection`. A wagon that reaches `invalid_rail` with a previous part still attached hits `FatalError("Disconnecting train")`.

`TryPathReserve` is the PBS reservation walk. `ClearPathReservation` drops the reservation behind the train as wagons leave a tile. `CheckTrainStayInDepot` will not leave until every part is on the depot tile. A consist with `cached_power == 0` is stopped. Otherwise the train waits 37 ticks, then requires a free segment (`UpdateSignalsOnSegment`, or a path block when `pf.reserve_paths` is set) and a successful `TryPathReserve`, unless `force_proceed` is set. An order that points back at this same depot calls `VehicleEnterDepot` again. On the way out the depot tile is reserved, the train is unhidden, and `track` becomes the axis through the depot.

## Electrified rail

Overhead wires and pylons are drawn by `DrawRailCatenary` in `src/elrail.cpp` (depots, ordinary rail, tunnels, bridges, road crossings, and stations). The sprite layout for that catenary is `docs/elrail.svg`.

`vehicle.disable_elrails` is applied by `UpdateDisableElrailSettingState` in the same file. Engines whose `intended_railtypes` include `RAILTYPE_ELECTRIC` lose that bit and gain `RAILTYPE_RAIL` while the setting is on. Existing trains that still have `RAILTYPE_ELECTRIC` in `railtypes` are switched to `RAILTYPE_RAIL`, gain it in `compatible_railtypes`, and set `VehicleRailFlag::AllowedOnNormalRail`. Building a wagon or engine under the setting sets that flag when the engine was intended for electric rail (`src/train_cmd.cpp`, the build path). `ConsistChanged` then adds `RAILTYPE_RAIL` to `railtypes` and `compatible_railtypes` for any part that still has the flag, so an electric engine built while elrails were disabled can run after they are turned back on. `GetPower` still returns 0 on a tile `HasPowerOnRail` rejects, so an electric engine on unelectrified track contributes no power while the flag is clear.
