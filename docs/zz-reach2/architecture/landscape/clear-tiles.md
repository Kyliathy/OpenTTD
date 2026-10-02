# Clear tiles

`TileType::Clear` is empty ground: grass, rough land, rocks, farm fields, desert, and a snow overlay. The HTML calls this class 0, "Ground". Accessors are in `src/clear_map.h`. The parent note is [archi-landscape.md](archi-landscape.md).

`ClearGround` (`src/clear_map.h` lines 16–30) is a 3-bit value (`CLEAR_GROUND_BITS`):

| Value | Enumerator | Comment in the header |
| --- | --- | --- |
| 0 | `Grass` | Plain grass with dirt transition, density 0–3 |
| 1 | `Rough` | Rough mounds, density 3 |
| 2 | `Rocks` | Rocks with snow transition, density 0–3 |
| 3 | `Fields` | Farm fields, density 3 |
| 5 | `Desert` | Desert with transition, density 1 or 3 |

There is no enumerator for 4. `ClearGround::MaxSize` is 8.

## `m5`: ground, density, counter

| Bits | Accessor | Meaning |
| --- | --- | --- |
| 7..5 | `GetClearCounter`, `GB(m5, 5, 3)` (lines 114–117) | Counter that advances density or field type |
| 4..2 | `GetClearGround`, `GB(m5, 2, 3)` (lines 52–55) | `ClearGround` |
| 1..0 | `GetClearDensity`, `GB(m5, 0, 2)` (lines 77–80) | Density |

`SetClearGroundDensity` (lines 152–155) writes `m5` as `(type << 2) | density` and clears the counter.

## Snow is `m3` bit 4

`IsSnowTile` (lines 40–43) is `HasBit(m3, 4)`, and it requires a clear tile. `MakeSnow` (lines 294–302) sets that bit. If the tile was a field, it also switches the ground to `ClearGround::Grass` and stores the given density. `ClearSnow` (lines 310–314) clears the bit and sets density to 3.

`docs/landscape.html` still lists snow as clear-ground type 4 in `m5` bits 4..2, with its own density column. That encoding is stale. `src/saveload/afterload.cpp` lines 2428–2431 rewrite a loaded `ClearGround{4}` into `ClearGround::Grass` and set `m3` bit 4. The comment there calls the old value `CLEAR_SNOW`.

## Fields

When the ground is `ClearGround::Fields`:

| Field | Accessor | Meaning |
| --- | --- | --- |
| `m2` | `GetIndustryIndexOfField` (lines 189–192) | `IndustryID` of the farm |
| `m3` bits 3..0 | `GetFieldType`, `GB(m3, 0, 4)` (lines 165–168) | Field type / production stage |
| `m4` bits 4..2 | `GetFence` for `DiagDirection::SE`, `GB(m4, 2, 3)` (lines 215–221) | Fence on the SE edge |
| `m4` bits 7..5 | `GetFence` for `DiagDirection::SW`, `GB(m4, 5, 3)` | Fence on the SW edge |
| `m3` bits 7..5 | `GetFence` for `DiagDirection::NE`, `GB(m3, 5, 3)` | Fence on the NE edge |
| `m6` bits 4..2 | `GetFence` for `DiagDirection::NW`, `GB(m6, 2, 3)` | Fence on the NW edge |

`GetFence` asserts the tile is a field. A zero fence value means no fence (the comment on `SetFence`, lines 228–231). NE fence bits and the snow bit share `m3`: snow is bit 4, the NE fence is bits 7..5, and the field type is bits 3..0. `MakeSnow` on a field replaces the field ground, so the field-type bits do not survive that conversion (`MakeSnow`, lines 298–299).

`MakeClear` (lines 253–264) sets owner `OWNER_NONE`, zeroes the other bytes, and stores ground plus density. `MakeField` (lines 274–285) does the same with `ClearGround::Fields`, density 3, the industry id in `m2`, and the field type in `m3`.

The HTML's owner, density, counter, field-type, and fence positions match these accessors. Its snow-as-ground-type row does not.
