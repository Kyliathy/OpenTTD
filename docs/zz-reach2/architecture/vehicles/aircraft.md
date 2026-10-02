# Aircraft

Aircraft are `Aircraft` (`src/aircraft.h`). Shared tick, orders, cargo, and breakdowns are in [archi-vehicles.md](archi-vehicles.md). Aircraft do not call YAPF. Movement on the ground and in the air is the airport finite-state machine in `src/aircraft_cmd.cpp`, plus `UpdateAircraftSpeed` while a state handler is flying, taxiing, or landing.

## Chain

`CmdBuildAircraft` builds two vehicles, and a third for a helicopter. The front is `AIR_HELICOPTER` (subtype 0) or `AIR_AIRCRAFT` (subtype 2), chosen from `AIR_CTOL` on the engine. It carries the passenger cargo, starts `VehState::Hidden` and `Stopped`, with `state == HANGAR` and `targetairport` set to the station on the build tile. The next vehicle is `AIR_SHADOW` (subtype 4), unclickable, and holds the mail capacity when mail exists. A helicopter appends `AIR_ROTOR` (subtype 6) at `z_pos + ROTOR_Z_OFFSET`. `IsNormalAircraft` is `subtype <= AIR_AIRCRAFT`, so the shadow and the rotor are not primary and their `Tick` returns immediately. `IsInDepot` is hidden on a hangar tile.

`GetOrderStationLocation` does not return a destination tile. In the `FLYING` state it calls `AircraftNextAirportPos_and_Order` and then returns an empty tile. `UpdateOrderDest` special-cases aircraft depot orders for that reason.

## Tick

`Aircraft::Tick` on a normal aircraft increments `tick_counter` and `running_ticks` when not stopped, calls `HelicopterTickHandler` for `AIR_HELICOPTER`, increments `current_order_time` once, then runs `AircraftEventHandler` twice. A false return from the first pass means the aircraft was deleted (`HandleCrashedAircraft`) and `Tick` returns false.

`HelicopterTickHandler` uses the rotor, `Next()->Next()`. A hidden rotor returns. While `current_order` is `OT_LOADING` or the aircraft is stopped, a non-zero rotor `cur_speed` counts upward and is cleared once it is at least `0x80` and `state` is `HRS_ROTOR_MOVING_3`. Otherwise a stopped rotor is set to `0x70` and then decremented while it stays at or above `0x50`. The rotor sprite advances when `tick_counter` reaches `cur_speed >> 4`, cycling `HRS_ROTOR_MOVING_1` through `HRS_ROTOR_MOVING_3`. Speed 0 selects `HRS_ROTOR_STOPPED`.

`AircraftEventHandler`:

1. A crashed aircraft goes to `HandleCrashedAircraft`.
2. `VehState::Stopped` returns true without moving. `CmdStartStopVehicle` rejects a stop while `state` is from `STARTTAKEOFF` up to `TERM7`, or while `VehicleAirFlag::HelicopterDirectDescent` is set.
3. `HandleBreakdown`. For aircraft this sets `VehState::AircraftBroken` when the countdown hits 2 and always returns false, so the aircraft is not held in place. `UpdateAircraftSpeed` then caps the limit at `SPEED_LIMIT_BROKEN` while that bit is set. The breakdown ends when the aircraft arrives at the airport; `HandleBreakdown` does not time it out.
4. `HandleAircraftSmoke`, then `ProcessOrders`, then `HandleLoading(loop != 0)`. The second pass of the tick cannot leave the terminal, because `HandleLoading` returns immediately when `mode` is true.
5. `OT_LOADING` and `OT_LEAVESTATION` return, so the airport machine does not run during the stop or the departure tick.
6. States from `ENDTAKEOFF` through `HELIENDLANDING` clear `VehicleAirFlag::DestinationTooFar`. On the ground, a non-zero `cached_max_range_sqr` compares the squared distance between the current `targetairport` and the next station or depot order. A distance above that cache sets the flag, posts `AdviceType::AircraftDestinationTooFar`, and skips `AirportGoToNextPosition` until the flag clears.
7. Otherwise `AirportGoToNextPosition`.

`HandleMissingAircraftOrders` runs from `ProcessOrders` when the aircraft has no valid goto order (a nearest-depot order does not count). If `targetairport` is not a live airport, it executes `Commands::SendVehicleToDepot`. Failure crashes the aircraft through `CrashAirplane`. A still-valid airport with an order other than `OT_GOTO_DEPOT` clears `current_order` and lets the aircraft finish the airport it is already flying to.

## Airport machine

`AirportGoToNextPosition` returns until `AircraftController` reports the aircraft is in position. It then `AirportClearBlock`s and calls `AirportMove`.

`AirportMove` reads `AirportFTAClass::layout[pos]` for `Station::Get(targetairport)->airport.GetFTA()`. When `heading` equals `state`, the position is a terminal, hangar, or other named state: `_aircraft_state_handlers[state]` runs, `previous_pos` is kept unless the state is `FLYING`, and `UpdateAircraftCache` runs if `state` or `pos` changed. Otherwise the machine follows `next`. A single successor calls `AirportSetBlocks` and sets `pos` to `next_position`. Several successors pick the one whose `heading` is `state` or `TO_ALL`. `AirportHasBlock` treats the next layout node's `blocks` as busy when they differ from the current block and the airport already has those bits taken.

`AircraftEventHandler_EnterTerminal` calls `AircraftEntersTerminal` and copies the layout heading into `state`. The hangar handler is `HandleAircraftEnterHangar`, reached from `VehicleEnterDepot` for `VehicleType::Aircraft`. `AircraftLeaveHangar` zeroes speed, `subspeed`, and `progress`, unhides the chain, gives the rotor `cur_speed` 80, services the aircraft, and calls `LeaveUnbunchingDepot`.

`FindClosestDepot` uses the current `targetairport` when that station has a hangar the aircraft can use. Otherwise `FindNearestHangar`. There is no YAPF call on this path.

`UpdateAirplanesOnNewStation` retargets every normal aircraft whose `targetairport` is that station and whose `state` is `FLYING`: a hangar order is cancelled when the new station has no hangar or the aircraft cannot land, and `pos` is recomputed from `AircraftGetEntryPoint`. A station with no hangar also runs `RemoveOrderFromAllVehicles` for `OT_GOTO_DEPOT` at that station.

## Speed

`UpdateAircraftSpeed` is the aircraft step generator. The comment on the function states the unit: `acceleration` is 3/8 mph per call, the function is called twice per tick, and the added speed is about `acceleration * 77` in the internal km-ish/h scale. `subspeed` accumulates that quantity. The high byte, plus a carry when `subspeed` wraps, is added to `cur_speed`.

The limit is `speed_limit * plane_speed`, then `SPEED_LIMIT_BROKEN` when `VehState::AircraftBroken` is set, then `cached_max_speed`. A soft limit (the `hard_limit` argument is false, or the aircraft is still below the limit) reduces an over-speed `cur_speed` by `max(1, (cur_speed * cur_speed) / 16384 / plane_speed)` instead of clamping in one call. The comment says 16384 was chosen so the result matches the old `speed / 48` curve at low speed and still slows a fast aircraft inside the landing distance. After the limit, `plane_speed > 1` divides `spd`. The distance step uses `GetOldAdvanceSpeed`: diagonal headings keep the value, cardinal headings use three quarters. `progress` accumulates that distance and the return value is `spd >> 8`, the number of position steps the caller may take.

Callers inside the state handlers pass `SPEED_LIMIT_TAXI` while taxiing and other limits for takeoff and landing. `GetAircraftFlightLevel` keeps `z_pos` between the bounds from `GetAircraftFlightLevelBounds`. Those bounds start at `GetTileHeightBelowAircraft`, add 10 for headings `N` through `SE`, add `min(20 * (cached_max_speed / 200) - 90, 0)`, and then add `AIRCRAFT_MIN_FLYING_ALTITUDE` (120) or `AIRCRAFT_MAX_FLYING_ALTITUDE` (360). Helicopters also add `HELICOPTER_HOLD_MAX_FLYING_ALTITUDE - PLANE_HOLD_MAX_FLYING_ALTITUDE` to the base. Below the minimum, `z` increases by 2 during takeoff and by 1 otherwise, and `VehicleAirFlag::InMinimumHeightCorrection` stays set until `z` reaches the midpoint. Above the maximum, outside takeoff, `z` decreases by 1 and `InMaximumHeightCorrection` stays set until the midpoint. `GetAircraftHoldMaxAltitude` is the tile height plus `PLANE_HOLD_MAX_FLYING_ALTITUDE` (150) or `HELICOPTER_HOLD_MAX_FLYING_ALTITUDE` (184).

`OnNewEconomyDay` on a normal aircraft ages it, decreases value every 8 days, runs `CheckOrders`, `CheckVehicleBreakdown`, and `CheckIfAircraftNeedsService`, then charges `ExpensesType::AircraftRun`. The service helper only writes a goto-depot order when the current target airport has a hangar and `CanVehicleUseStation` accepts the aircraft. A goto-depot order whose airport no longer qualifies becomes a dummy.
