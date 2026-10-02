# Object tiles

`TileType::Object` is a placed object: transmitters, owned land, and anything else the object pool tracks. Accessors are in `src/object_map.h`. The parent note is [archi-landscape.md](archi-landscape.md).

`GetObjectType` is declared here and defined elsewhere. The tile itself stores an `ObjectID`, not the object-type enum.

| Field | Bits | Accessor |
| --- | --- | --- |
| Object id, low 16 bits | whole `m2` | `GetObjectIndex` (lines 47–50) returns `ObjectID(m2 \| m5 << 16)` |
| Object id, bits 16..23 | whole `m5` | The high byte of that same expression. `MakeObject` writes `m5 = index >> 16` (line 82) |
| Owner | `m1` bits 4..0 | `SetTileOwner` inside `MakeObject` (line 77) |
| Water class | `m1` bits 6..5 | `SetWaterClass` (line 78). `HasTileWaterClass` includes objects |
| Random bits | whole `m3` | `GetObjectRandomBits` (lines 59–62) |
| Animated state | `m6` bits 1..0 | `GetAnimatedTileState` |
| Animation frame | whole `m7` | `GetAnimationFrame` (`src/tile_map.h` lines 250–253), which allows object tiles |

`MakeObject` (lines 74–85) also zeroes `m4` and `m8` and clears `m6` bits 7..2. No accessor in this header reads `m4` or `m8`.

The HTML's split of the object index across `m2` (bits 0–15) and `m5` (bits 16–23), plus random bits, water class, owner, and `m7` as the animation counter, matches `GetObjectIndex` and `GetAnimationFrame`. The HTML calls `m7` an animation counter; the shared accessor returns it as the animation frame.
