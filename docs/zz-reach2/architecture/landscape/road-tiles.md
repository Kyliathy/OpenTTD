# Road tiles

`TileType::Road` is a normal road (with or without tram), a level crossing, or a road depot. Accessors are in `src/road_map.h`. The parent note is [archi-landscape.md](archi-landscape.md).

`RoadTileType` is `m5` bits 7..6 (`GetRoadTileType`, `GB(m5, 6, 2)`, lines 22–39):

| Value | Enumerator |
| --- | --- |
| 0 | `Normal` |
| 1 | `Crossing` |
| 2 | `Depot` |

Road and tram types are stored on more than `TileType::Road`. `MayHaveRoad` (`src/road_map.cpp` lines 21–35) is true for every road tile, for a station tile that is a road stop, and for a tunnel or bridge whose transport type is road.

## Road type and tram type

| Field | Bits | Accessor |
| --- | --- | --- |
| Road `RoadType` | `m4` 5..0 | `GetRoadTypeRoad`, `GB(m4, 0, 6)` (`src/road_map.h` lines 152–155) |
| Tram `RoadType` | `m8` 11..6 | `GetRoadTypeTram`, `GB(m8, 6, 6)` (lines 163–166) |

`INVALID_ROADTYPE` is 63 (`src/road_type.h` line 29), which is the 6-bit value `0x3F` the HTML calls "no road" / "no tram". `ROADTYPE_ROAD` is 0 and `ROADTYPE_TRAM` is 1 (lines 24–27). `ROADTYPE_END` is 63, the same numeric value as `INVALID_ROADTYPE`. The HTML's 6-bit widths and the `0x3F` sentinel match these accessors.

On a level crossing, `m8` bits 5..0 are also the rail type (`MakeRoadCrossing` writes `INVALID_ROADTYPE << 6 | rat`, lines 672–684, and `GetRailType` reads `GB(m8, 0, 6)`). Tram type stays in bits 11..6. Bits 15..12 of `m8` are outside both of those accessors.

## Owners

| Road or tram | Where | Width |
| --- | --- | --- |
| Road, on a normal road tile | `m1` bits 4..0 | 5 (`GetRoadOwner`, lines 244–247) |
| Road, on a crossing or a depot | `m7` bits 4..0 | 5 (same function: `IsNormalRoadTile` selects `m1` vs `m7`) |
| Tram | `m3` bits 7..4 | 4 (`GB(m3, 4, 4)`, lines 249–252) |

`SetRoadOwner` for tram (lines 265–266) stores `OWNER_TOWN` when the caller passes `OWNER_NONE`, and `GetRoadOwner` maps `OWNER_TOWN` back to `OWNER_NONE`. The comment says trams do not need `OWNER_TOWN`, and the remap saves one bit. The 5-bit owner space (`OWNER_DEITY` is `0x12`) does not fit in this 4-bit field. The HTML describes the `OWNER_NONE` / `OWNER_TOWN` remap and is accurate for that.

On a crossing, `GetTileOwner` reads `m1` bits 4..0, and `MakeRoadCrossing` (line 674) sets that owner to the rail owner. The road owner is the separate `m7` value.

## Normal road

`RoadBits` for the road are `m5` bits 3..0. Tram bits are `m3` bits 3..0 (`GetRoadBits`, lines 112–116). `RoadBit` (`src/road_type.h` lines 57–62) is `NW = 0`, `SW = 1`, `SE = 2`, `NE = 3`. That matches the HTML piece table.

Disallowed directions are `m5` bits 5..4 (`GetDisallowedRoadDirections`, `GB(m5, 4, 2)`, `src/road_map.h` lines 311–314). `DisallowedRoadDirection` (`src/road_type.h` lines 78–81) is `Southbound = 0`, `Northbound = 1`. The HTML's two bits match this field.

`Roadside` is `m6` bits 5..3 (`GetRoadside`, `GB(m6, 3, 3)`, lines 483–501):

| Value | Enumerator | Header comment |
| --- | --- | --- |
| 0 | `Barren` | Barren land |
| 1 | `Grass` | Grass |
| 2 | `Paved` | Paved sidewalks |
| 3 | `StreetLights` | Street lights |
| 4 | unused | The comment says "unused for historical reasons" |
| 5 | `Trees` | Trees on the sidewalk |
| 6 | `GrassRoadWorks` | Grass with road works |
| 7 | `PavedRoadWorks` | Sidewalks with road works |

The HTML lists 0, 1, 2, 3, 5, 6, 7 and skips 4. That matches the enumerator.

Road-works progress is `m7` bits 3..0 (`IncreaseRoadWorksCounter`, `AB(m7, 0, 4, 1)`, lines 529–533). The counter is finished when those bits equal 15. Snow or desert is `m7` bit 5 (`IsOnSnowOrDesert`, line 469). On a normal road those bits sit beside the road-works counter. On a crossing or depot the low 5 bits of `m7` are the road owner instead, and bit 5 is still the snow/desert flag.

`m2` on a normal road or a crossing is the `TownID` (`GetTownIndex`, `src/town_map.h` lines 23–26). A road depot uses `m2` for the `DepotID` instead, and `GetTownIndex` asserts the tile is not a road depot.

## Level crossing

`MakeRoadCrossing` (lines 672–684) and the crossing accessors:

| Field | Bits | Meaning |
| --- | --- | --- |
| `m5` 7..6 | `RoadTileType::Crossing` (1) | |
| `m5` bit 0 | `GetCrossingRoadAxis`, `GB(m5, 0, 1)` (lines 346–349) | Road axis. `Axis::X = 0`, `Axis::Y = 1` (`src/direction_type.h` lines 133–135). Rail axis is the other one (`GetCrossingRailAxis`, lines 358–361) |
| `m5` bit 4 | `HasCrossingReservation` (lines 390–393) | PBS reservation of the rail track |
| `m5` bit 5 | `IsCrossingBarred` (lines 426–429) | Crossing lights / bar |
| `m1` 4..0 | tile owner | Rail owner, as set by `MakeRoadCrossing` |
| `m7` 4..0 | road owner | |
| `m8` 5..0 | rail type | |
| `m8` 11..6 | tram type | |

The HTML's direction bit (0 = road along X and rail along Y, 1 = the swap) matches `Axis` and `GetCrossingRailAxis`.

## Road depot

`MakeRoadDepot` (lines 706–718):

| Field | Contents |
| --- | --- |
| `m5` bits 7..6 | `RoadTileType::Depot` (2) |
| `m5` bits 1..0 | Exit `DiagDirection` (`GetRoadDepotDirection`, lines 576–579) |
| `m2` | `DepotID` |
| `m1` bits 4..0 and `m7` bits 4..0 | Depot owner, written to both |
| `m4` or `m8` | The road or tram type, via `SetRoadType` |

`GetRoadDepotDirection` uses the same `NE/SE/SW/NW` values as rail depots.
