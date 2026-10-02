# Water tiles

`TileType::Water` is open water, coast, a lock, or a ship depot. Accessors are in `src/water_map.h`. The parent note is [archi-landscape.md](archi-landscape.md).

## `m5` type field

The layout constants are lines 16–28:

| Name | Value | Role |
| --- | --- | --- |
| `WBL_TYPE_BEGIN` | 4 | Start of the type field |
| `WBL_TYPE_COUNT` | 4 | Width of the type field (`m5` bits 7..4) |
| `WBL_LOCK_ORIENT_BEGIN` / `COUNT` | 0 / 2 | Lock direction, `m5` bits 1..0 |
| `WBL_LOCK_PART_BEGIN` / `COUNT` | 2 / 2 | Lock part, `m5` bits 3..2 |
| `WBL_DEPOT_PART` | 0 | Depot part, `m5` bit 0 |
| `WBL_DEPOT_AXIS` | 1 | Depot axis, `m5` bit 1 |

`WaterTileType` (`GetWaterTileType`, lines 31–38 and 82–85):

| Value | Enumerator |
| --- | --- |
| 0 | `Clear` — plain water |
| 1 | `Coast` |
| 2 | `Lock` |
| 3 | `Depot` |
| 4 | `ClearRocks` — rocks on water |
| 5 | `CoastRocks` — rocks on coast |

`docs/landscape.html` and the water rows of `docs/landscape_grid.html` stop at depot (values 0–3). `ClearRocks` and `CoastRocks` are missing there. That part of the HTML is stale. `IsCoast` (lines 205–208) treats both `Coast` and `CoastRocks` as coast. `IsWater` (lines 151–153) is only `Clear`, so rocky open water is not "plain water".

## Class, owner, docking, flooding

| Field | Bits | Accessor |
| --- | --- | --- |
| Water class | `m1` 6..5 | `GetWaterClass`, `GB(m1, 5, 2)` (lines 116–119). `Sea = 0`, `Canal = 1`, `River = 2`, `Invalid = 3` (lines 41–46) |
| Owner | `m1` 4..0 | `GetTileOwner`. `MakeSea` / `MakeRiver` / `MakeShore` use `OWNER_WATER` (lines 389–446) |
| Docking | `m1` bit 7 | `IsDockingTile` (lines 378–380) |
| Non-flooding | `m3` bit 0 | `IsNonFloodingWaterTile` (lines 548–551) |
| Random bits | whole `m4` | `GetWaterTileRandomBits` (lines 343–346). `MakeCanal` and `MakeRiver` store the caller's random bits in `m4` |

`IsSea` / `IsCanal` / `IsRiver` (lines 162–186) require `WaterTileType::Clear` plus the matching class. A canal tile keeps a company owner (`MakeCanal` asserts the owner is not `OWNER_WATER`, lines 455–457).

The HTML's class, owner, docking, non-flooding, and canal/river random-bit descriptions match these accessors.

## Locks

`LockPart` (lines 67–71) is `Middle = 0`, `Lower = 1`, `Upper = 2`. Direction is a `DiagDirection` in `m5` bits 1..0 (`GetLockDirection`, lines 319–322): `NE = 0`, `SE = 1`, `SW = 2`, `NW = 3`. `MakeLock` (lines 520–530) writes three tiles: the given tile as the middle, `tile - diag` as the lower part, and `tile + diag` as the upper part. Each part keeps the water class it was given so the class can be restored when the lock is removed (comment on lines 526–527).

The HTML lock-part and lock-direction tables match `LockPart` and `DiagDirection`.

## Ship depots

A ship depot is two tiles.

| Field | Bits | Accessor |
| --- | --- | --- |
| Type | `m5` 7..4 | `WaterTileType::Depot` (3) |
| Axis | `m5` bit 1 | `GetShipDepotAxis`, `GB(m5, WBL_DEPOT_AXIS, 1)` (lines 248–251). `Axis::X = 0`, `Axis::Y = 1` |
| Part | `m5` bit 0 | `GetShipDepotPart` (lines 260–263). `DepotPart::North = 0`, `DepotPart::South = 1` (lines 60–63) |
| Depot id | whole `m2` | `MakeShipDepot` (line 476). `GetDepotIndex` (`src/depot_map.h` lines 56–60) reads `m2` |

`GetOtherShipDepotTile` (lines 283–285) steps by one tile along the axis, towards decreasing coordinates for the north part and increasing coordinates for the south part. The HTML's axis and part bits match.

`m6` bits 7..2 are cleared by `MakeShore`, `MakeWater`, `MakeShipDepot`, and `MakeLockTile` (`SB(m6, 2, 6, 0)`). `m7` and `m8` are set to 0. No water accessor in this header reads them afterwards.
