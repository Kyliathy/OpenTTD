# Rail tiles

`TileType::Railway` is plain track, track with signals, or a rail depot. A level crossing is a road tile, not a rail tile (`src/tile_type.h` lines 43–46). Accessors are in `src/rail_map.h`. The parent note is [archi-landscape.md](archi-landscape.md).

`docs/elrail.svg` is a drawing of electrified-rail sprites (catenary and pylons on a tile). It is a visual companion for how electric track is drawn. It does not describe the tile bytes, and this note does not transcribe it.

## Which kind of rail tile

`RailTileType` is `m5` bits 7..6 (`GetRailTileType`, `GB(m5, 6, 2)`, `src/rail_map.h` lines 23–39):

| Value | Enumerator |
| --- | --- |
| 0 | `Normal` |
| 1 | `Signals` |
| 3 | `Depot` |

There is no value 2, and there is no checkpoint enumerator. The HTML's "checkpoints" aside on the ground-type table is stale. Waypoints are `TileType::Station` ([station-tiles.md](station-tiles.md)).

`m1` bit 7 is the docking mark for a half-tile with water (`SetDockingTile`, `src/water_map.h` lines 367–370). `m1` bits 4..0 are the owner (`GetTileOwner`).

## Rail type

`GetRailType` (`src/rail_map.h` lines 115–117) is `GB(m8, 0, 6)`. The built-in labels in `src/rail_type.h` lines 26–34 are still:

| Value | Enumerator |
| --- | --- |
| 0 | `RAILTYPE_RAIL` |
| 1 | `RAILTYPE_ELECTRIC` |
| 2 | `RAILTYPE_MONO` |
| 3 | `RAILTYPE_MAGLEV` |

`RAILTYPE_END` is 64, so the 6-bit field can hold indices `0..63`. `INVALID_RAILTYPE` is `0xFF`, which does not fit in those 6 bits. The HTML table that stops at maglev names the four original labels correctly and is stale as a description of the field width.

The same 6 bits are the rail type on level crossings, rail stations, rail waypoints, and rail tunnels and bridges. Those tiles are not `TileType::Railway`; `GetRailType` itself does not assert the tile type.

## Plain track

`GetTrackBits` (lines 136–139) is `GB(m5, 0, 6)` as `TrackBits`. `Track` (`src/track_type.h` lines 19–27) assigns the bits:

| Bit in `m5` | `Track` | Direction in the header comment |
| --- | --- | --- |
| 0 | `X` | Along the x-axis (NE–SW) |
| 1 | `Y` | Along the y-axis (NW–SE) |
| 2 | `Upper` | North corner |
| 3 | `Lower` | South corner |
| 4 | `Left` | West corner |
| 5 | `Right` | East corner |

That matches the HTML track-bit table.

## Ground under the rail

`RailGroundType` is `m4` bits 3..0 (`GetRailGroundType`, `GB(m4, 0, 4)`, lines 568–603). The values match the HTML ground table:

| Value | Enumerator | Header comment |
| --- | --- | --- |
| 0 | `Barren` | Nothing (dirt) |
| 1 | `Grass` | Grassy |
| 2 | `FenceNW` | Fence on the NW edge |
| 3 | `FenceSE` | Fence on the SE edge |
| 4 | `FenceSENW` | Fences on NW and SE |
| 5 | `FenceNE` | Fence on the NE edge |
| 6 | `FenceSW` | Fence on the SW edge |
| 7 | `FenceNESW` | Fences on NE and SW |
| 8 | `FenceVert1` | Fence on the eastern side |
| 9 | `FenceVert2` | Fence on the western side |
| 10 | `FenceHoriz1` | Fence on the southern side |
| 11 | `FenceHoriz2` | Fence on the northern side |
| 12 | `SnowOrDesert` | Icy or sandy |
| 13 | `HalfTileWater` | Fence plus shore or water on the free half-tile |
| 14 | `HalfTileSnow` | Snow only on the higher part of the slope |

On a signal tile, `m4` bits 7..4 are the signal states (below), so the ground type stays in the low nibble.

## Path reservation

`GetRailReservationTrackBits` (lines 194–200) on plain rail:

- `m2` bits 10..8 (`GB(m2, 8, 3)`) store `Track + 1`. Zero means nothing is reserved.
- `m2` bit 11, when set, also reserves the opposite track (`TrackToOppositeTrack`).

`SetTrackReservation` (lines 209–215) refuses overlapping tracks. The HTML's values 0–6 and its "opposite track" bit describe this index-plus-flag, which is the encoding the accessor implements.

## Signals

`HasSignals` is `RailTileType::Signals`. Presence, state, type, and variant are split across `m2`, `m3`, and `m4`.

Which of the four signal bits belongs to a trackdir is `_signal_along_trackdir` in `src/table/signal_data.h` lines 12–14. The bitmask is applied to the 4-bit presence field and the 4-bit state field (signal bit 0 is the least significant bit of that nibble). Along the trackdir the table is:

| `Trackdir` | Mask |
| --- | --- |
| `X_NE`, `Y_SE`, `Upper_E` | `0x8` (bit 3) |
| `Lower_E` | `0x2` (bit 1) |
| `Left_S` | `0x4` (bit 2) |
| `Right_S` | `0x1` (bit 0) |
| `X_SW`, `Y_NW`, `Upper_W` | `0x4` (bit 2) |
| `Lower_W` | `0x1` (bit 0) |
| `Left_N` | `0x8` (bit 3) |
| `Right_N` | `0x2` (bit 1) |

`_signal_against_trackdir` (lines 18–20) is the other direction. `_signal_on_track` (lines 24–26) is `0xC` for `X`, `Y`, `Upper`, and `Left` (bits 2 and 3), and `0x3` for `Lower` and `Right` (bits 0 and 1). The HTML's signal 0–3 table matches these masks: signal bit 0 is lower-west and right-south, bit 1 is lower-east and right-north, bit 2 is the south-west / north-west / upper-west / left-south group, bit 3 is the north-east / south-east / upper-east / left-north group.

| What | Bits | Accessor |
| --- | --- | --- |
| Signals present | `m3` bits 7..4 | `GetPresentSignals`, `GB(m3, 4, 4)` (lines 451–453). A set bit means that signal exists |
| Signal state | `m4` bits 7..4 | `GetSignalStates`, `GB(m4, 4, 4)` (lines 420–422). `GetSignalStateByTrackdir` (lines 505–510) treats a set bit as `SignalState::Green` and a clear bit as `SignalState::Red` (`src/signal_type.h` lines 38–41) |
| Signal type, tracks other than `Lower` and `Right` | `m2` bits 2..0 | `GetSignalType`, `GB(m2, 0, 3)` (lines 303–307) |
| Signal type, `Lower` and `Right` | `m2` bits 6..4 | `GetSignalType`, `GB(m2, 4, 3)` |
| Variant (light vs semaphore), other tracks | `m2` bit 3 | `GetSignalVariant`, `GB(m2, 3, 1)` (lines 385–388) |
| Variant, `Lower` and `Right` | `m2` bit 7 | `GetSignalVariant`, `GB(m2, 7, 1)` |

`SignalType` (`src/signal_type.h` lines 22–29) is `Block = 0`, `Entry = 1`, `Exit = 2`, `Combo = 3`, `Path = 4`, `PathOneWay = 5`. The HTML names (normal, pre-signal, exit, combo, pbs, no-entry) are the old names for the same numbers. `SignalVariant` is `Electric = 0` (light) and `Semaphore = 1` (lines 14–17), which matches the HTML's semaphore bit.

`CycleSignalSide` (lines 368–375) rewrites a 2-bit group inside the presence nibble: bits 4..5 of `m3` for `Lower` and `Right`, bits 6..7 for the other tracks. Decrementing onto 0 wraps to 2 for a path signal and to 3 otherwise. Those two bits are the presence pair from `_signal_on_track`, not a separate field.

On a plain signal tile the reservation encoding above still uses `m2` bits 11..8, and the signal type/variant uses `m2` bits 7..0. Both live in the same `uint16`.

## Depots

`MakeRailDepot` (lines 657–668) stores:

| Field | Contents |
| --- | --- |
| `m2` | `DepotID` (`GetDepotIndex` in `src/depot_map.h` lines 56–60 reads `m2` for rail, road, and ship depots) |
| `m5` bits 7..6 | `RailTileType::Depot` (value 3) |
| `m5` bits 1..0 | Exit `DiagDirection`: `NE = 0`, `SE = 1`, `SW = 2`, `NW = 3` (`src/direction_type.h` lines 83–88; `GetRailDepotDirection`, `src/rail_map.h` lines 171–173) |
| `m5` bit 4 | PBS reservation (`HasDepotReservation`, lines 256–259) |
| `m8` bits 5..0 | Rail type |

`MakeRailNormal` (lines 624–635) clears `m2`, `m3`, `m4`, `m7`, writes `RailTileType::Normal` and the track bits into `m5`, and stores the rail type in `m8`.
