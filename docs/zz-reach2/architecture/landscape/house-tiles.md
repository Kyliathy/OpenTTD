# House tiles

`TileType::House` is one tile of a town building. Accessors are in `src/town_map.h`. The parent note is [archi-landscape.md](archi-landscape.md).

`GetTileOwner` refuses house tiles (`src/tile_map.h` lines 178–181). `m1` on a house is NewGRF random bits (`GetHouseRandomBits`, `src/town_map.h` lines 297–300), the whole byte.

## Identity and completion

| Field | Bits | Accessor |
| --- | --- | --- |
| Town | whole `m2` | `GetTownIndex` (lines 23–26) |
| House id | `m8` bits 11..0 | `GetCleanHouseType`, `GB(m8, 0, 12)` (lines 48–51). `GetHouseType` (lines 60–62) then calls `GetTranslatedHouseID` |
| Completed | `m3` bit 7 | `IsHouseCompleted` (lines 167–170) |
| Player-protected | `m3` bit 5 | `IsHousePlayerProtected` (lines 82–85). The comment says this stops the town upgrading or removing the house |
| Random triggers | `m3` bits 4..0 | `GetHouseRandomTriggers`, `GB(m3, 0, 5)` (lines 323–326) |

`MakeHouseTile` (lines 375–389) clears `m8` bits 15..12 (`SB(m8, 12, 4, 0)`). No accessor in this header reads those four bits. No accessor in this header reads `m3` bit 6, and none reads `m4`. The HTML's "free" marks on `m8` bits 15..12, `m3` bit 6, and `m4` still match the absence of accessors.

`NUM_HOUSES` is 4096 and `NEW_HOUSE_OFFSET` is 110 (`src/house.h` lines 28–29). A 12-bit id can address that table. `docs/landscape_externals.html` is a catalogue of original house sizes and climates. It is not the tile format. The HTML note that house ids above 110 are NewGRF houses matches `NEW_HOUSE_OFFSET`.

## Construction and age share `m5`

While the house is under construction (`m3` bit 7 clear):

| Bits | Accessor | Meaning |
| --- | --- | --- |
| `m5` 4..3 | `GetHouseBuildingStage`, `GB(m5, 3, 2)` (lines 205–208) | Stage. `TOWN_HOUSE_COMPLETED` is 3 (`src/house.h` line 25) |
| `m5` 2..0 | `GetHouseConstructionTick`, `GB(m5, 0, 3)` (lines 217–220) | Tick counter between stages |

`IncHouseConstructionTick` (lines 230–239) adds one to `m5` bits 4..0. When bits 4..3 become 3, it sets the completed bit. The comment on lines 185–192 describes the wrap: the counter increments on periodic processing, and the stage advances when it wraps.

Once completed, the whole of `m5` is the age in years, clamped at 255 (`IncrementHouseAge` and `GetHouseAge`, lines 259–274). `ResetHouseAge` zeroes `m5` and requires the house to be completed, because during construction those bits are the counter (comment on lines 243–245).

The HTML's completed bit, stage bits, counter bits, and age-in-`m5` description match.

## Lift data and processing time share `m6` and `m7`

Two pairs of accessors use the same bits.

| Bits | Lift accessor | Processing-time / animation accessor |
| --- | --- | --- |
| `m6` 7..2 | `GetLiftPosition`, `GB(m6, 2, 6)` (lines 147–149). Comment: position 0..36 | `GetHouseProcessingTime`, `GB(m6, 2, 6)` (lines 335–338) |
| `m7` bits 3..1 and bit 0 | `GetLiftDestination`, `GB(m7, 1, 3)` (lines 126–128). `LiftHasDestination` is `m7` bit 0 (lines 104–106). `HaltLift` clears `m7` bits 3..0 (lines 137–139) | `GetAnimationFrame` returns the whole of `m7` (`src/tile_map.h` lines 250–253) for house tiles |

`MakeHouseTile` writes the processing time from `HouseSpec::processing_time` into `m6` bits 7..2 (line 388) and sets the animation frame to 0, which zeroes `m7`.

`docs/landscape.html` presents these as exclusive modes: standard houses use `m6`/`m7` for the lift (houses 04 and 05), and "newhouses" uses them for processing time and the animation frame. The headers do not switch modes. Both sets of functions are still called: lift position and destination from `src/town_cmd.cpp` (the lift sprite and `SetLiftDestination`), and processing time from `src/newgrf_house.cpp` (`GetHouseProcessingTime` / `SetHouseProcessingTime`). The HTML's either/or split is stale as a description of the accessors. The bit positions themselves are the ones both callers use.

Animated-tile state remains `m6` bits 1..0 (`GetAnimatedTileState`), underneath the 6-bit lift / processing field.
