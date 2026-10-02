# R2MLM3 - Layer Call-Site Migration

## Global Mandatory Reading

> **Execution protocol**
>
> - Read this Global Mandatory Reading section before the first task and again
>   after every context compaction.
> - Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.
> - A referenced heading or exported symbol is authoritative. Global navigation
>   hints and Area line ranges are for navigation only; if they drift, relocate
>   the named section or symbol rather than expanding to the whole file.
> - Mandatory reading is non-transitive. Do not preload a linked file's own
>   Mandatory Reading, Consult on Demand, or Planning Provenance entries unless
>   this plan names them directly for the current area.
> - Do not read Consult on Demand or Planning Provenance merely because those
>   sources are listed. Use a Consult on Demand source only when its trigger
>   occurs.
> - No created source file, test file or JSON file should start with same prefix as a planning MD file (such as r2). Only MD files can have that prefix. No MD file should be created anywhere in source directories unless specifically asked.
> - After EVERY CONTEXT COMPACTION, you must add a note above the task group
>   which is currently in execution. The note will say: "Context has compacted
>   X times while executing this group.". If the compaction occurs before any
>   meaningful work was done (this can happen when there is a lot of baggage
>   from the previous compaction), then and ONLY then you can skip this step.
> - When exceeding 3 context compactions, YOU MUST ask yourself if you're not 
>   chewing more than we can handle :). Heck, plans change! :). In such a case, 
>   the way out is to split the group on the spot,
>   in the plan, into more groups, 
>   by appending B, C, D as a suffixes to those groups (splitting sub-groups 
>   themselves is also allowed). When you do this, create Local Mandatory
>   Reading areas so you can optimize your context only for the
>   ACTUAL task at hand.
>   Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 66-89 | `### AD-2` to `### AD-5`: stacked `TileY`, the `Map` / `MultiLayerMap` split, independent surface-only content, plane distances and surface projection for towns. Every classification decision in this plan applies one of these rules. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 111-128 | `### AD-11 Behaviour-neutral staging` and `## Cross-plan Invariants`: the game still runs with one layer, so every migration must be a no-op at one layer (I-6) while being correct at three. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 157-174 | `## Engineering Conventions` and `## Build and Test Commands`. In particular: port existing comments when rewriting a loop, and add a business comment saying which layer rule applies. |
| [r2mlm3-layer-call-site-migration.md](r2mlm3-layer-call-site-migration.md) | 56-66 | `## Classification Rules` C1 to C9 of this plan: the single vocabulary used in the ledger and in code comments. |
| [src/map_layer.h](../../../../src/map_layer.h) | Live definitions; no fixed range | Produced by r2mlm2 Group 4. `MultiLayerMap::IterateLayer`, `TileInLayer`, `ProjectToSurface`, `RandomTileInLayer`, `GetLayerBase`, `IsTileInLayer` are the only allowed replacements; this plan adds no new layer API. |
| [src/map_func.h](../../../../src/map_func.h) | Live definitions; no fixed range | Extended by r2mlm2 Group 1. `TilePlaneY`, `GetTileLayer`, `TileXYL`, `Map::PlaneSize`, `Map::WrapToLayer` are authoritative; `Map::WrapToMap` is deprecated and removed by task 2.8. |

## Outcome and Boundaries

Every use of a map-geometry API outside `src/map_func.h`, `src/map.cpp`, `src/tile_map.h` and `src/map_layer.*` is classified in `## Call-site Ledger` and, where needed, migrated. At one layer the game behaves exactly as before; at three layers no code silently falls into the wrong layer.

Scope: `Map::Size`, `Map::Iterate`, `Map::SizeY`, `Map::MaxY`, `Map::LogY`, `RandomTile` / `RandomTileSeed`, `Map::WrapToMap`, `TileY` compared with plane bounds, `ClosestTownFromTile` and the town and station k-d trees.

Out of scope, recorded only: viewport, smallmap, screenshot and terraform GUI code (class C7, migrated by r2mlm5) and the map chunk handlers (class C8, migrated by r2mlm4).

## Classification Rules

- **C1 All-layer storage walk.** Conversions, repairs, recounts and ownership changes over every tile keep `Map::Iterate()`, which now covers every layer.
- **C2 Surface-only content.** Terrain generation, land counting, towns, houses, industries, trees, objects, water, disasters and heightmaps use `MultiLayerMap::IterateLayer(MapLayer::Surface)`, `RandomTile()` and `TileXYL(x, y, MapLayer::Surface)`.
- **C3 Plane geometry.** A `TileY` compared or clamped against `Map::SizeY()` / `Map::MaxY()`, or a neighbourhood loop bounded by them, uses `TilePlaneY` and rebuilds tiles with `TileXYL(x, y, GetTileLayer(origin))`.
- **C4 Plane quantity.** `Map::Size()` used as an amount of world (budgets, scaling, node limits) becomes `Map::PlaneSize()`; `Map::Size()` used as a validity bound stays.
- **C5 Town association.** Nearest town, town radius and local authority project the tile with `MultiLayerMap::ProjectToSurface` before querying.
- **C6 Same-layer spatial query.** Station k-d tree searches, catchment, `TileArea` expansion and clamping, spiral searches and station spread clamp within the origin tile's layer.
- **C7 View code.** Viewport, smallmap, screenshots and terraform GUI: recorded only, migrated by r2mlm5.
- **C8 Map chunks.** `src/saveload/map_sl.cpp`: recorded only, migrated by r2mlm4.
- **C9 External interface.** Script API, NewGRF variables, network and social-integration reports keep plane semantics (`Map::SizeY()` per plane is what they already expect); NewGRF nearby-tile offsets wrap inside the origin layer.

## Call-site Ledger

Filled by Group 1 and updated by every later group. One row per call site. `Status` is `todo`, `migrated`, `kept` (correct as is) or `deferred` (C7, C8).

| File:line | API | Class | Status | Note |
| --- | --- | --- | --- | --- |

## Implementation Tasks

### Area Mandatory Reading - Ledger and core helpers

_Applies to Groups 1-2._

Entry gate. Before Group 1 starts, `src/map_layer.h` must declare `MultiLayerMap::IterateLayer`, `TileInLayer`, `ProjectToSurface` and `RandomTileInLayer`, and `src/map_func.h` must declare `TilePlaneY`, `GetTileLayer`, `TileXYL`, `Map::PlaneSize` and `Map::WrapToLayer`. Verify with a grep for each name. If any is missing, stop every task in this plan and finish r2mlm2 Groups 1 and 4 first. Group 1 inventory tasks 1.1 to 1.3 may still run. Do not compute layer bases with `Map::PlaneSize()` arithmetic or local helper functions as a substitute.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tile_map.cpp](../../../../src/tile_map.cpp) | 50-148 | `GetTileSlopeZ`, `IsTileFlat`, `GetTileZ`, `GetTileMaxZ`: each clamps the neighbour row with `std::min(y1 + 1, Map::MaxY())`, which would pull a lower-layer neighbour into the surface. Class C3. `GetTilePixelSlopeOutsideMap` in the same range is view code (C7, r2mlm5). |
| [src/tilearea_orthogonal.cpp](../../../../src/tilearea_orthogonal.cpp) | 113-142 | `OrthogonalTileArea::Expand` and `ClampToMap`: clamp Y to `Map::SizeY()`. Class C6, must clamp inside the area's layer. |
| [src/tilearea_spiral.cpp](../../../../src/tilearea_spiral.cpp) | 19-63 | `SpiralTileIterator` constructors and `SkipOutsideMap`: the `y >= Map::SizeY()` test skips every tile of a lower layer. Class C6. |
| [src/tilearea_diagonal.cpp](../../../../src/tilearea_diagonal.cpp) | 84-132 | `DiagonalTileArea::iterator::operator++`: rebuilds tiles with `TileXY` and an `y >= Map::SizeY()` test. Class C6. |
| [src/station_kdtree.h](../../../../src/station_kdtree.h) | 27-45 | `ForAllStationsRadius`: clamps the search box to `Map::SizeY()`. Class C6. |
| [src/newgrf_commons.cpp](../../../../src/newgrf_commons.cpp) | 397-419 | `GetNearbyTile`: `Map::WrapToMap(tile + TileDiffXY(x, y))`. Class C9: wrap inside the origin layer with `Map::WrapToLayer`. |

### Group 1 - Ledger inventory

{{SIMPLE}}
- [ ] 1.1 Grep `src/` for `Map::Size()`, `Map::Iterate()`, `Map::SizeY()`, `Map::MaxY()`, `Map::LogY()`, `RandomTile`, `RandomTileSeed`, `Map::WrapToMap`, excluding `src/map_func.h`, `src/map.cpp`, `src/tile_map.h`, `src/map_layer.*` and `src/tests/`; paste one ledger row per hit with status `todo`.
- [ ] 1.2 Grep for `TileY(` within 3 lines of `SizeY` or `MaxY`, and for `for (uint y` / `for (int y` loops bounded by `Map::SizeY()` or `Map::MaxY()`; add any hit not yet in the ledger.
- [ ] 1.3 Grep for `ClosestTownFromTile`, `CalcClosestTownFromTile`, `_town_kdtree`, `_station_kdtree`, `GetTownRadiusGroup`; add their call sites with a provisional class.
- [ ] 1.4 Assign a class C1 to C9 to every row, using `## Classification Rules`; rows that fit no class are escalated to the user before Group 2.
- [ ] 1.5 Mark every C7 and C8 row `deferred` with a one-line note for r2mlm5 or r2mlm4.
- [ ] 1.6 Sort the ledger by file and record the total count per class under the ledger table.

### Group 2 - Core spatial helpers

{{MEDIUM}}
- [ ] 2.1 `src/tile_map.cpp`: in `GetTileSlopeZ`, `IsTileFlat`, `GetTileZ` and `GetTileMaxZ`, clamp neighbour rows with the plane Y and rebuild neighbours with `TileXYL` in the tile's layer (C3).
- [ ] 2.2 `src/tilearea_orthogonal.cpp`: make `Expand` and `ClampToMap` clamp within the layer of `this->tile` (C6); keep the asserts on `Map::Size()`.
- [ ] 2.3 `src/tilearea_spiral.cpp`: store the plane Y and the layer of the start tile, test `y` against `Map::SizeY()` in the plane, and yield tiles with `TileXYL` (C6).
- [ ] 2.4 `src/tilearea_diagonal.cpp`: same treatment for the iterator (C6).
- [ ] 2.5 `src/station_kdtree.h`: clamp `ForAllStationsRadius` to the band of the centre's layer and skip stations in other layers (C6).
- [ ] 2.6 `src/tunnel_map.cpp` (`IsTunnelInWay`): compare `TilePlaneY(tile)` with `Map::MaxY() / 2` (C3).
- [ ] 2.7 `src/newgrf_commons.cpp`, `src/newgrf_house.cpp` and `src/industry_cmd.cpp`: replace `Map::WrapToMap` with `Map::WrapToLayer(..., GetTileLayer(origin))` (C9 / C2).
- [ ] 2.8 Remove the deprecated `Map::WrapToMap` definition from `src/map_func.h` (an edit, not a file deletion), build, and mark these ledger rows `migrated`.

### Area Mandatory Reading - World generation

_Applies to Group 3._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/genworld.cpp](../../../../src/genworld.cpp) | 94-232 | `_GenerateWorld`: the generation order (landscape, clear tiles, `CountLandTiles`, towns, industries, objects, trees, tile-loop warm-up). r2mlm4 later inserts non-surface initialisation here; this group only makes each step surface-explicit. |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 845-858 | `InitializeLandscape`: fills the inner rectangle and voids the south edges using `Map::MaxX` / `Map::MaxY` loops. Class C2 for now (surface plane); r2mlm4 generalises it per layer. |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 1618-1719 | `GenerateLandscape`: the generator dispatch (original, TGP, heightmap) and the freeform north-edge voiding. Class C2. |
| [docs/zz-reach2/architecture/landscape/archi-landscape.md](../../architecture/landscape/archi-landscape.md) | 135-159 | `## Map size, void tiles, and terraforming`: the four places that void the north edges (`landscape.cpp`, `tgp.cpp`, `heightmap.cpp`, `genworld.cpp`); all four must stay surface-only. |

### Group 3 - Surface-only world generation

{{MEDIUM}}
- [ ] 3.1 `src/landscape.cpp`: make `InitializeLandscape`, `GenerateTerrain`, the river and desert or rainforest passes, `GenerateLandscape` and the freeform voiding iterate `MultiLayerMap::IterateLayer(MapLayer::Surface)` or build tiles with `TileXYL(..., MapLayer::Surface)` (C2).
- [ ] 3.2 `src/tgp.cpp` and `src/heightmap.cpp`: make every `TileXY` write and every `Map::SizeY()` / `MaxY()` loop explicitly surface (C2).
- [ ] 3.3 `src/genworld.cpp`: make the freeform voiding loops surface-explicit and add a comment marking where r2mlm4 inserts non-surface initialisation.
- [ ] 3.4 `src/clear_cmd.cpp` (`GenerateClearTile`) and `src/tree_cmd.cpp`: confirm `RandomTile()` is surface and convert `Map::Size()` budgets to `Map::PlaneSize()` (C2, C4).
- [ ] 3.5 `src/industry_cmd.cpp` and `src/object_cmd.cpp`: convert placement loops, `Map::MaxY()` bounds and budgets to surface plane semantics (C2, C3, C4).
- [ ] 3.6 `src/water_cmd.cpp` (`ConvertGroundTilesIntoWaterTiles` and the flood checks): surface-only (C2).
- [ ] 3.7 `src/disaster_vehicle.cpp`: target tiles, start positions and `Map::MaxY()` bounds use the surface plane (C2, C3).
- [ ] 3.8 Build, regenerate the four baseline saves, and diff digests against `baseline-digests.txt`; any difference blocks Group 4.

### Area Mandatory Reading - Savegame conversions

_Applies to Group 4._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 351-369 | `## AfterLoadGame`: version-gated conversions over the loaded image. Explains why most loops are C1: they repair whatever tiles exist, and old files only contain surface tiles. |
| [src/saveload/afterload.cpp](../../../../src/saveload/afterload.cpp) | 558-564 | Start of `AfterLoadGame`, including the `_cur_tileloop_tile` zero fix that stays valid per plane (AD-6). |
| [src/saveload/oldloader_sl.cpp](../../../../src/saveload/oldloader_sl.cpp) | 544-565 | TTDPatch header tiles at `Map::Size() - 3`, `- 2`, `- 1` and the `ClearOldMap3` loop from `TileXY(0, Map::MaxY())`: these mean "end of the surface plane" and become `Map::PlaneSize()` (C4, C3). |
| [src/saveload/oldloader_sl.cpp](../../../../src/saveload/oldloader_sl.cpp) | 1527-1566 | `LoadOldMapPart1` and `LoadOldMapPart2`: allocation for TTO files and the loops that read one byte per tile straight from the file. The file holds one 256x256 plane, so these loops must iterate the surface plane only (C2), or a three-layer allocation would read past the data. |

### Group 4 - Savegame conversion loops

{{HARD}}
- [ ] 4.1 Walk the 52 `Map::Iterate()` loops in `src/saveload/afterload.cpp`; for each, record in the ledger whether it is C1 (keep) or uses `TileY` against plane bounds (C3, migrate).
- [ ] 4.2 Migrate every C3 loop in `afterload.cpp`, porting the existing comments and adding the class tag in a `//` comment.
- [ ] 4.3 `src/saveload/oldloader_sl.cpp`: convert the TTDPatch header tile indices and the `ClearOldMap3` loop to plane semantics (C4, C3).
- [ ] 4.4 `src/saveload/oldloader_sl.cpp`: convert the read loops in `LoadOldMapPart1` / `LoadOldMapPart2` and the other whole-map loops to `MultiLayerMap::IterateLayer(MapLayer::Surface)` (C2), so a later three-layer allocation still reads exactly one plane from the file.
- [ ] 4.5 `src/saveload/company_sl.cpp`, `station_sl.cpp`, `town_sl.cpp`, `waypoint_sl.cpp`: classify their loops (expected C1) and record them.
- [ ] 4.6 Load a TTO save copied from `!kit/TT/` (for example `trt00.sv0`) and a modern baseline save in a Debug build; confirm both load and play for one in-game month.

### Area Mandatory Reading - Towns, stations, commands and interfaces

_Applies to Groups 5-7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/town_cmd.cpp](../../../../src/town_cmd.cpp) | 3990-4047 | `CalcClosestTownFromTile` and `ClosestTownFromTile`: the k-d tree nearest query and the ownership shortcuts for roads and depots. Class C5: project before the query. |
| [src/town_kdtree.h](../../../../src/town_kdtree.h) | 16-23 | `Kdtree_TownXYFunc`: keys towns by `TileX` / `TileY`; towns are surface-only, so queries must be projected rather than the tree changed. |
| [src/script/api/script_map.cpp](../../../../src/script/api/script_map.cpp) | 16-46 | `ScriptMap::IsValidTile`, `GetMapSize`, `GetMapSizeX`, `GetMapSizeY`, `GetTileX`, `GetTileY`: the script-visible map. Class C9: scripts see the surface plane only in version 1 (AD-12). |
| [src/pathfinder/water_regions.cpp](../../../../src/pathfinder/water_regions.cpp) | 31-41 | Water-region coordinate helpers using `TileY`: water regions exist for the surface plane only, so non-surface tiles must be ignored before indexing. |
| [src/command_func.h](../../../../src/command_func.h) | 167-193 | `Command<Tcmd>::Do` tile bound check `tile >= Map::Size()`: a validity bound (C4 keep) that now admits every layer, which is intended (AD-2). |

### Group 5 - Town and station association

{{MEDIUM}}
- [ ] 5.1 `CalcClosestTownFromTile` and `ClosestTownFromTile`: project with `MultiLayerMap::ProjectToSurface` before the k-d tree query and before the distance threshold test (C5).
- [ ] 5.2 `GetTownRadiusGroup`, local-authority lookups and town rating checks in `src/town_cmd.cpp`: project the tile (C5).
- [ ] 5.3 The remaining `Map::Iterate()` and `Map::Size()` uses in `src/town_cmd.cpp` (town growth, house counts, `ScaleByLandProportion` budget): classify; growth and house loops are C2.
- [ ] 5.4 `src/station_cmd.cpp`: station spread, `GetStationAround`, rectangle checks and the three `Map::Size()` uses: classify; spatial searches stay in the origin layer (C6).
- [ ] 5.5 `src/station_gui.cpp`: the join-station search and `Map::MaxY()` bound use the origin layer (C6).
- [ ] 5.6 Record every change in the ledger and build.

### Group 6 - Commands, economy and vehicles

{{MEDIUM}}
- [ ] 6.1 `src/rail_cmd.cpp`, `src/road_cmd.cpp`, `src/train_cmd.cpp`, `src/terraform_cmd.cpp`: classify the `Map::Size()` uses (expected C4 keep as bounds) and the single `Map::Iterate()` in `road_cmd.cpp`.
- [ ] 6.2 `src/economy.cpp`: the two company-removal loops are C1 (every layer must be cleaned on bankruptcy); confirm and record. Classify the `Map::MaxY()` use.
- [ ] 6.3 `src/cheat_gui.cpp`, `src/industry_gui.cpp`: classify (the land-count refresh is C2).
- [ ] 6.4 `src/settings_table.cpp` and `src/console_cmds.cpp`: map-size settings and console reports are plane (C9 / C3).
- [ ] 6.5 `src/aircraft_cmd.cpp`, `src/effectvehicle.cpp`, `src/sound.cpp`: clamps against `Map::MaxY()` use the plane of the vehicle or tile (C3); aircraft stay on the surface (AD-12).
- [ ] 6.6 `src/openttd.cpp`, `src/network/network_client.cpp`: social-integration map size reports are plane (C9); record.

### Group 7 - External interfaces and surface-only subsystems

{{MEDIUM}}
- [ ] 7.1 `src/linkgraph/demands.cpp`: the `base_distance` scale from `TileXY(0, 0)` to `TileXY(Map::MaxX(), Map::MaxY())` is a plane quantity (C4); confirm it is unchanged by plane distances.
- [ ] 7.2 `src/newgrf_railtype.cpp`, `src/newgrf_roadtype.cpp`: label-map loops are C1.
- [ ] 7.3 `src/newgrf/newgrf_actd.cpp`, `src/newgrf_airport.cpp`: map-size variables and airport bounds are plane (C9, C3).
- [ ] 7.4 `src/network/core/network_game_info.cpp`, `src/network/network_admin.cpp`: reported map height is the plane height (C9).
- [ ] 7.5 `src/script/api/script_map.cpp`, `script_tilelist.cpp`, `script_object.hpp`: `GetMapSize` returns `Map::PlaneSize()`, `ScriptMap::IsValidTile` also requires `MultiLayerMap::IsTileInLayer(t, MapLayer::Surface)`, `GetMapSizeY` stays per plane, and the tile-list bound uses the plane (C9, C4). Scripts therefore see exactly the old single map.
- [ ] 7.6 `src/pathfinder/water_regions.cpp`: return early from `InvalidateWaterRegion` and the other public entry points for non-surface tiles, with a comment that ships are surface-only (AD-12).
- [ ] 7.7 `src/pathfinder/yapf/yapf_ship_regions.cpp`: the node limit uses `Map::PlaneSize()` (C4).

### Area Mandatory Reading - Deferred records and verification

_Applies to Group 8._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 1195-1320 | `GetViewportY` and `ViewportAddLandscape`: the main C7 site, recorded only; its migration is r2mlm5 Group 2. |
| [src/saveload/map_sl.cpp](../../../../src/saveload/map_sl.cpp) | 22-95 | `_map_desc`, `MAPSChunkHandler`, `MAPTChunkHandler`: the C8 pattern repeated by every plane chunk; recorded only, migrated by r2mlm4 Group 5. |
| [docs/zz-reach2/upgrades/2026-10/tools/baseline-digests.txt](tools/baseline-digests.txt) | Live definitions; no fixed range | Produced by master task 1.8. The neutrality reference for task 8.6. |

### Group 8 - Deferred records and verification

{{SIMPLE}}
- [ ] 8.1 Confirm every hit in `src/viewport.cpp`, `src/smallmap_gui.cpp`, `src/screenshot.cpp` and `src/terraform_gui.cpp` has a `deferred` C7 row with a note naming the r2mlm5 group that will migrate it.
- [ ] 8.2 Confirm every hit in `src/saveload/map_sl.cpp` has a `deferred` C8 row naming r2mlm4 Group 5.
- [ ] 8.3 Re-run the Group 1 greps; every hit must have a ledger row whose status is not `todo`.
- [ ] 8.4 Build both configurations without new warnings; run `ctest`.
- [ ] 8.5 Add to `src/tests/map_layer.cpp` tests for the migrated helpers on a three-layer map: spiral search, `OrthogonalTileArea::Expand`, `GetTileSlopeZ` neighbours and `ProjectToSurface` all stay in the expected layer.
- [ ] 8.6 Regenerate the four baseline saves and compare digests with `baseline-digests.txt`; they must match (I-6).
- [ ] 8.7 Play a new game for one in-game year at fast forward in a Debug build with towns, industries and a few player routes; confirm no assert fires.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A ledger row fits none of C1 to C9 | [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | `## Architecture Decisions` (lines 60-117) | The decision still depends on gameplay intent | Find the AD that governs the case; if none does, ask the user and record the answer as a new class. |
| An `afterload.cpp` loop's purpose is unclear in task 4.1 | [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | `## Changing the format` (lines 371-389) | The loop's version gate does not explain the repair | Understand whether the loop repairs bytes or rebuilds caches; the code remains the authority. |
| Digests differ in task 3.8 or 9.4 | [docs/zz-reach2/architecture/landscape/archi-landscape.md](../../architecture/landscape/archi-landscape.md) | `## Dispatch` (lines 183-207) | The difference is in tile types rather than heights | Locate which generator step changed tile content. |
| A town or station behaves oddly at the map's south edge after Group 5 | [src/town_cmd.cpp](../../../../src/town_cmd.cpp) | `GetTownRadiusGroup` | The issue involves `TileY` clamps | Check for a missed C3 site. |
| A NewGRF industry or house misplaces sprites after task 2.7 | [docs/zz-reach2/architecture/newgrf/archi-newgrf.md](../../architecture/newgrf/archi-newgrf.md) | `## Callback resolution (industry production)` (lines 155-184) | The variable involved is a nearby-tile query | Confirm how nearby-tile offsets are consumed. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| Repository grep of `Map::Iterate`, `Map::Size`, `Map::SizeY`, `Map::MaxY`, `RandomTile` (per-file counts) | `src/` | Sized the work (86 iterate uses in 19 files, 96 `Size` uses, 117 `SizeY` / `MaxY` uses, 23 random-tile uses) and grouped files into Groups 2 to 8. |
| [src/tile_map.cpp](../../../../src/tile_map.cpp) | 50-148 | Found the `std::min(y1 + 1, Map::MaxY())` clamp that silently pulls a lower layer into the surface. |
| [src/tilearea_orthogonal.cpp](../../../../src/tilearea_orthogonal.cpp), [src/tilearea_spiral.cpp](../../../../src/tilearea_spiral.cpp) | 113-142, 19-63 | Showed spatial helpers that would skip every lower-layer tile, motivating class C6. |
| [src/saveload/oldloader_sl.cpp](../../../../src/saveload/oldloader_sl.cpp) | 544-565, 1527-1566 | `Map::Size() - 3` header tiles and per-tile file reads that assume one plane. |
| [src/town_cmd.cpp](../../../../src/town_cmd.cpp) | 3990-4047 | Nearest-town logic behind class C5. |
| [src/pathfinder/water_regions.cpp](../../../../src/pathfinder/water_regions.cpp) | 31-41 | Water-region indexing by `TileY`, which needs a surface guard. |

## Completion Conditions

- `## Call-site Ledger` lists every grep hit from Group 1 with a class and a non-`todo` status.
- No migrated code computes a layer base by hand; all replacements use the r2mlm2 API.
- `Map::WrapToMap` no longer exists.
- Surface digests match `baseline-digests.txt`, and the new helper tests pass on a three-layer allocation.
