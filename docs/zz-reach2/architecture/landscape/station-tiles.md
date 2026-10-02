# Station tiles

`TileType::Station` covers rail stations, rail waypoints, road stops, road waypoints, airports, docks, buoys, and oil-rig heliports. Accessors are in `src/station_map.h`. The parent note is [archi-landscape.md](archi-landscape.md).

## Common fields

| Field | Bits | Accessor |
| --- | --- | --- |
| Station id | whole `m2` | `GetStationIndex` (lines 26–29), a `StationID` |
| Station type | `m6` bits 6..3 | `GetStationType`, `GB(m6, 3, 4)` (lines 42–45) |
| Graphics | whole `m5` | `GetStationGfx` (lines 66–69). The type is `StationGfx`, a `uint8_t` (line 18) |
| Owner | `m1` bits 4..0 | `GetTileOwner` |
| Water class | `m1` bits 6..5 | `GetWaterClass`, for buoys, docks, airports, and anything else that uses the station water-class path |
| Docking | `m1` bit 7 | `IsDockingTile` (`src/water_map.h` lines 378–380) |
| Animated state | `m6` bits 1..0 | `GetAnimatedTileState` (`src/animated_tile_map.h` lines 30–32) |
| Animation frame | whole `m7` | `GetAnimationFrame` (`src/tile_map.h` lines 250–253), for station tiles among others |

`StationType` (`src/station_type.h` lines 29–39), which is the value in `m6` bits 6..3:

| Value | Enumerator |
| --- | --- |
| 0 | `Rail` |
| 1 | `Airport` |
| 2 | `Truck` |
| 3 | `Bus` |
| 4 | `Oilrig` |
| 5 | `Dock` |
| 6 | `Buoy` |
| 7 | `RailWaypoint` |
| 8 | `RoadWaypoint` |

`MakeStation` (lines 707–720) writes the owner, water class, station id, graphics byte, and station type, and clears `m6` bit 2 and bit 7. PBS reservation for rail stations and rail waypoints is that bit 2 (`HasStationReservation`, lines 546–549).

The HTML's `m6` type field and reservation bit match. Its graphics tables (`00`–`07` for rail platforms, `00`–`05` for road stops, and so on) describe original baseset layouts. The stored value is the full `m5` byte. Airport graphics are translated by `GetTranslatedAirportTileID` (declared at line 279). Two constants are part of the layout the accessors actually test:

- `GFX_TRUCK_BUS_DRIVETHROUGH_OFFSET` is 4 (line 34). A road stop or road waypoint is drive-through when `GetStationGfx >= 4` (`IsDriveThroughStopTile`, lines 273–276).
- `GFX_DOCK_BASE_WATER_PART` is 4 (line 33). `MakeDock` (line 832) stores the water half as `4 + axis`.

`GetRailStationAxis` (lines 501–504) is `Axis::Y` when `m5` bit 0 is set, and `Axis::X` when it is clear. The HTML's "bit 0 clear = X, set = Y" for rail station graphics matches that accessor.

## Rail station and rail waypoint

| Field | Bits | Accessor |
| --- | --- | --- |
| Random bits | `m3` 7..4 | `GetStationTileRandomBits`, `GB(m3, 4, 4)` (lines 692–695) |
| Blocked | `m3` bit 0 | `IsStationTileBlocked` (lines 429–432) |
| Catenary wires allowed | `m3` bit 1 | `CanStationTileHaveWires` (lines 453–456) |
| Catenary pylons allowed | `m3` bit 2 | `CanStationTileHavePylons` (lines 477–480) |
| Custom spec index | whole `m4` | `GetCustomStationSpecIndex` (lines 632–635). `HasStationTileRail` is required. `IsCustomStationSpecIndex` (lines 608–611) is true when `m4` is non-zero |
| Rail type | `m8` 5..0 | `GetRailType`, `GB(m8, 0, 6)` (`src/rail_map.h` lines 115–117) |
| Reservation | `m6` bit 2 | `HasStationReservation` |
| Animation | whole `m7` | `GetAnimationFrame` |

The HTML's wires, pylons, blocked bit, custom spec in `m4`, and rail type in `m8` match. `docs/elrail.svg` shows the catenary sprites; see [rail-tiles.md](rail-tiles.md).

## Road stop and road waypoint

`MayHaveRoad` is true for any road stop (`src/road_map.cpp` lines 27–28), so the road-type fields from [road-tiles.md](road-tiles.md) apply: road type in `m4` bits 5..0, tram type in `m8` bits 11..6, road owner in `m7` bits 4..0 (a station tile is not a normal road tile, so `GetRoadOwner` reads `m7`), tram owner in `m3` bits 7..4 with the `OWNER_NONE` / `OWNER_TOWN` remap.

Custom road-stop spec index is `m8` bits 5..0 (`GetCustomRoadStopSpecIndex`, `GB(m8, 0, 6)`, `src/station_map.h` lines 668–671). On a road stop those bits are the spec index. On a rail station the same bits are the rail type. The accessors assert the station kind before reading.

Road-waypoint pavement is `m3` bits 3..2 (`GetRoadWaypointRoadside`, `GB(m3, 2, 2)`, lines 286–289), stored as a `Roadside`. The field is 2 bits wide. Snow or desert on a road waypoint is `m8` bit 15 (`IsRoadWaypointOnSnowOrDesert`, `HasBit(m8, 15)`, lines 308–311). That bit sits above the tram-type field (`m8` bits 11..6).

The HTML's split of `m8` (tram type, custom road spec, snow on waypoints) and of `m4` (road type on stops, custom rail spec on rail stations) matches these accessors. The grid's picture of airport, dock, buoy, and oil-rig `m8` as unused matches `MakeStation` zeroing `m8` (line 721) and no later station accessor in this header writing `m8` for those types. Rail type and road-spec accessors assert rail or road-stop tiles.

## What the HTML graphics catalogue is

The long `m5` tables in `docs/landscape.html` (open platform, roofed platform, dock coast parts, and the rest) are the original graphics indices. The header stores an 8-bit `StationGfx` and, for airports, translates it. Treat the HTML ranges as the baseset catalogue. The bit positions of type, reservation, wires, owners, and road/rail types are the part that the accessors still implement.
