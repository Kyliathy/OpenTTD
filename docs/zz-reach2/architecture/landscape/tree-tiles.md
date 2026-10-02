# Tree tiles

`TileType::Trees` holds one to four trees and a description of the ground under them. Accessors are in `src/tree_map.h`. The parent note is [archi-landscape.md](archi-landscape.md).

`MakeTree` (lines 245–256) sets the owner to `OWNER_NONE`. `m1` bits 4..0 are that owner (`SetTileOwner`). `m1` bits 6..5 are the water class (`SetWaterClass` inside `SetTreeGroundDensity`, lines 145–150): `WaterClass::Sea` when the ground is `TreeGround::Shore`, otherwise `WaterClass::Invalid`. `HasTileWaterClass` includes trees (`src/water_map.h` lines 105–108).

## Ground and density in `m2`

| Bits | Accessor | Meaning |
| --- | --- | --- |
| 8..6 | `GetTreeGround`, `GB(m2, 6, 3)` (lines 102–105) | `TreeGround` |
| 5..4 | `GetTreeDensity`, `GB(m2, 4, 2)` (lines 128–131) | Ground density, not the number of trees |

`TreeGround` (lines 52–58):

| Value | Enumerator | Header comment |
| --- | --- | --- |
| 0 | `Grass` | Normal grass |
| 1 | `Rough` | Rough land |
| 2 | `SnowOrDesert` | Snow or desert, depending on landscape |
| 3 | `Shore` | Shore |
| 4 | `RoughSnow` | Snow with rough land underneath |

The comment on `GetTreeDensity` (lines 109–126) says the density is the snow-line step (0–3) in sub-arctic, and 3 for desert versus 0 otherwise in sub-tropical. The function name means "density of a tile which has trees".

## Tree type, count, growth

| Field | Bits | Accessor |
| --- | --- | --- |
| Tree type | whole `m3` | `GetTreeType` (lines 87–90) |
| Count | `m5` bits 7..6 | `GetTreeCount`, `GB(m5, 6, 2) + 1` (lines 164–167). The stored value is count minus one, so the result is 1–4 |
| Growth | `m5` bits 2..0 | `GetTreeGrowth`, `GB(m5, 0, 3)` (lines 196–199) |

`TreeType` bases (`src/tree_map.h` lines 25–32) match the HTML climate ranges: temperate `0x00`, sub-arctic `0x0C`, rainforest `0x14`, cactus `0x1B`, other sub-tropical `0x1C`, toyland `0x20`. The counts beside them (`TREE_COUNT_*`, lines 41–45) are `TREE_COUNT_TEMPERATE` through `TREE_COUNT_TOYLAND` (toyland is 9). `GetTreeType` returns the raw byte, so a value outside those bases is possible; the comment on lines 76–81 says the enumeration does not have to cover every stored value.

`TreeGrowthStage` (lines 65–73):

| Value | Enumerator |
| --- | --- |
| 0 | `Growing1` |
| 1 | `Growing2` |
| 2 | `Growing3` |
| 3 | `Grown` |
| 4 | `Dying1` |
| 5 | `Dying2` |
| 6 | `Dead` |

The HTML's growth rows (0–2 growing, 3 fully grown, 4–6 withering) match these values. `Dead` is the header's name for the last stage.

`GetTreeCount` and `GetTreeGrowth` do not read `m5` bits 5..3. `MakeTree` writes count at bit 6 and growth at bit 0, so those middle bits are zero on a newly made tile (`t.m5() = count << 6 | growth`, line 253).

`m4`, `m7`, and `m8` are set to 0 by `MakeTree` and have no tree accessor in this header. `m6` bits 7..2 are cleared (`SB(m6, 2, 6, 0)`, line 254). The HTML's picture of trees, including water class, owner, ground, density, type, count, and growth, matches these accessors.
