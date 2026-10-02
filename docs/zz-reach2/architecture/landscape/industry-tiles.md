# Industry tiles

`TileType::Industry` is one tile of an industry. The industry's own type lives on the industry object, not in the tile. The tile stores a graphics id. Accessors are in `src/industry_map.h`. The parent note is [archi-landscape.md](archi-landscape.md).

`GetTileOwner` refuses industry tiles (`src/tile_map.h` lines 178–181).

## `m1`: completion, water class, construction

| Bits | Accessor | Meaning |
| --- | --- | --- |
| 7 | `IsIndustryCompleted`, `HasBit(m1, 7)` (lines 35–38) | Set when the tile is fully built |
| 6..5 | `GetWaterClass`, `GB(m1, 5, 2)` | `Sea`, `Canal`, `River`, or `Invalid` (land). `HasTileWaterClass` includes industry (`src/water_map.h` lines 105–119) |
| 3..2 | `GetIndustryConstructionCounter`, `GB(m1, 2, 2)` (lines 122–125) | Construction counter |
| 1..0 | `GetIndustryConstructionStage`, `GB(m1, 0, 2)` when bit 7 is clear (lines 60–63) | Stage. When bit 7 is set the accessor returns `INDUSTRY_COMPLETED` (3, `src/industry_type.h` line 98) instead of reading these bits |

`ResetIndustryConstructionStage` (lines 147–151) clears `m1` bits 3..0 and bit 7. Bit 4 of `m1` has no accessor in this header. The HTML's bit positions for completion, water class, counter, and stage match. The grid's picture of the low nibble as unused once the tile is completed matches the getter ignoring those bits after completion.

## Graphics id, not the original sprite catalogue

`GetCleanIndustryGfx` (lines 85–88) is `m5 | (GB(m6, 2, 1) << 8)`: the whole of `m5`, plus `m6` bit 2 as bit 8 of the id. `SetIndustryGfx` (lines 109–113) writes it back the same way. `GetIndustryGfx` (lines 97–100) passes that value through `GetTranslatedIndustryTileID`.

`docs/landscape.html` spends most of class 8 on a table of original graphics (coal mine `00`–`06`, power station, toyland industries, and `AF`–`1FF` as NewGRF). That table is a baseset catalogue. It is stale as a map-format description. The tile stores the 9-bit `IndustryGfx` above.

## The rest of the tile

| Field | Bits | Accessor |
| --- | --- | --- |
| Industry id | whole `m2` | `GetIndustryIndex` (lines 23–26) |
| Random bits | whole `m3` | `GetIndustryRandomBits` (lines 185–188). Comment: GRF callbacks |
| Animation loop | whole `m4` | `GetIndustryAnimationLoop` (lines 160–163) |
| Random triggers | `m6` bits 5..3 | `GetIndustryRandomTriggers`, `GB(m6, 3, 3)` (lines 211–214) |
| Graphics bit 8 | `m6` bit 2 | See above |
| Animated state | `m6` bits 1..0 | `GetAnimatedTileState` |
| Animation frame | whole `m7` | `GetAnimationFrame` (`src/tile_map.h` lines 250–253) |

`MakeIndustry` (lines 239–251) zeroes `m1`, stores the industry id, random bits, and graphics id, clears triggers, sets the water class, clears `m6` bits 7..6, and zeroes `m7` and `m8`. No accessor in this header reads `m8`.

The HTML's `m2` index, `m3` random bits, `m4` animation loop, `m6` trigger bits, `m6` bit 2 as bit 8 of the type, and `m7` animation frame match these accessors. What it calls "type" is the graphics id.
