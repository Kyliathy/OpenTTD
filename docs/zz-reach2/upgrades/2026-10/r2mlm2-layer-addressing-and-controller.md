# R2MLM2 - Stacked-Plane Addressing and the MultiLayerMap Controller

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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 65-80 | `### AD-2 Stacked-plane addressing` and `### AD-3 Two controllers`: the tile formula, stacked `TileY`, the new primitives `TilePlaneY` / `GetTileLayer` / `TileXYL`, and the split between `Map` (storage, plane geometry) and `MultiLayerMap` (layer semantics). This plan implements both; every signature here must match. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 86-88 | `### AD-5 Distances are plane distances`: the `Distance*` functions ignore the layer; same-layer needs are explicit. Group 3 implements it. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 110-112 | `### AD-11 Behaviour-neutral staging`: live games stay at one layer during this plan; three layers are exercised only in unit tests. Every default argument introduced here keeps `layer_count == 1`. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 118-127 | `## Cross-plan Invariants` I-1 to I-5: the properties the unit tests in Group 6 assert. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 141-170 | `## Engineering Conventions` and `## Build and Test Commands`: comment style, naming, file rules, build and test commands. |
| [src/map_func.h](../../../../src/map_func.h) | 198-432 | `struct Map` and the free functions `TileXY`, `TileDiffXY`, `TileVirtXY`, `TileX`, `TileY`. This is the code being extended; `TileY`, `TileXY` and `TileVirtXY` keep their formulas (stacked semantics), only documentation and the `Map` statics change. |

## Outcome and Boundaries

Exported by the end of this plan (later plans consume these names unchanged):

- `src/map_func.h`: `Map::Allocate(uint size_x, uint size_y, uint layer_count = 1)`, `Map::Size()` (all layers), `Map::PlaneSize()`, `Map::LayerCount()`, `Map::WrapToLayer(TileIndex tile, MapLayer layer)`, and the inline primitives `TilePlaneY(TileIndex)`, `GetTileLayer(TileIndex)`, `TileXYL(uint x, uint plane_y, MapLayer layer)`. `Map::WrapToMap` remains as a deprecated surface-plane wrapper until r2mlm3 migrates its three callers.
- `src/map.cpp` and `src/tile_map.h`: plane-correct `IsInnerTile`, `TileAddWrap`, `AddTileIndexDiffCWrap`, `DistanceFromEdge`, `DistanceFromEdgeDir`, `TileVirtXYClampedToMap`, `GetClosestWaterDistance`, and plane-based `DistanceManhattan`, `DistanceSquare`, `DistanceMax`, `DistanceMaxPlusManhattan`.
- `src/map_layer.h` / `src/map_layer.cpp`: `struct MultiLayerMap` with `GetLayerCount`, `IsValidLayer`, `GetLayerBase`, `IsTileInLayer`, `TileInLayer`, `ProjectToSurface`, `IterateLayer`, `RandomTileInLayer`.
- `src/tests/map_layer.cpp`: unit tests for I-1 to I-5 on a three-layer allocation.

Not in this plan: classifying the hundreds of existing call sites (r2mlm3), rules, initialisers, tile loop and savegame (r2mlm4), drawing (r2mlm5). With `layer_count == 1` every changed function returns exactly what it returned before.

## Implementation Tasks

### Area Mandatory Reading - Storage and plane geometry

_Applies to Groups 1-3._

Entry gate. `src/map_layer_type.h` must export `enum class MapLayer` and `MAX_MAP_LAYERS` (r2mlm1 Group 1). Verify with a grep for both names. If either is missing, stop tasks 1.4 to 1.6 and every later group, and finish r2mlm1 Group 1 first; tasks 1.1 to 1.3 may continue. Do not declare a local layer enum, use raw integers for layers, or copy the enum into `map_func.h` as a substitute.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/map.cpp](../../../../src/map.cpp) | 18-104 | `Map` statics, `Map::Allocate`, `Map::CountLandTiles`, `TileVirtXYClampedToMap`, debug `TileAdd`. Allocation validates power-of-two axes and calls `AllocateWaterRegions`; the debug `TileAdd` asserts against `Map::WrapToMap`. |
| [src/map.cpp](../../../../src/map.cpp) | 106-131 | `TileAddWrap`: the freeform north-edge rule and the `MaxX` / `MaxY` wrap test that must use plane Y. |
| [src/map.cpp](../../../../src/map.cpp) | 159-309 | The four distance functions, `DistanceFromEdge`, `DistanceFromEdgeDir`, `GetClosestWaterDistance`: every `TileY` use here becomes `TilePlaneY`, and the water search builds tiles in the origin tile's layer. |
| [src/tile_map.h](../../../../src/tile_map.h) | 89-164 | `GetTileType`, `IsInnerTile`, `SetTileType`, `IsTileType`, `IsValidTile`: the void-fence predicate and the `Map::Size()` bound asserts. `IsInnerTile` must test the plane Y so every layer gets its own fence. |
| [src/map_func.h](../../../../src/map_func.h) | 509-543 | `AddTileIndexDiffCWrap` and `TileIndexToTileIndexDiffC`: coordinate-space wrap and diff, which must stay inside one layer. |
| [src/map_func.h](../../../../src/map_func.h) | 620-656 | `DiagdirBetweenTiles`, `RandomTileSeed`, `RandomTile`: cross-layer tiles have no diagonal direction; random tiles stay on the surface. |
| [docs/zz-reach2/architecture/landscape/archi-landscape.md](../../architecture/landscape/archi-landscape.md) | 135-159 | `## Map size, void tiles, and terraforming`: which edges are void with and without `freeform_edges`. The per-plane fence must reproduce exactly this rule. |

### Group 1 - Map storage and plane geometry

{{MEDIUM}}
- [x] 1.1 Add private statics `Map::layer_count` and `Map::plane_size` and a `Map::plane_mask` (`plane_size - 1`), define them in `src/map.cpp` with `///<` comments, and keep `Map::size` as the tile count of all layers.
- [x] 1.2 Change `Map::Allocate` to `Allocate(uint size_x, uint size_y, uint layer_count = 1)`: validate `1 <= layer_count <= MAX_MAP_LAYERS` with `FatalError`, set the plane statics, set `size = plane_size * layer_count`, and allocate both tile arrays with `size` elements. Update the Javadoc and the debug log line to include the layer count.
- [x] 1.3 Add `Map::PlaneSize()` and `Map::LayerCount()` accessors with Javadoc; update the Javadoc of `Map::Size()`, `Map::SizeY()`, `Map::MaxY()` and `Map::Iterate()` to state whether they are per plane or all layers (AD-3).
- [x] 1.4 Add to `src/map_func.h` (including `map_layer_type.h`) the inline primitives `TilePlaneY`, `GetTileLayer` and `TileXYL`, each with Javadoc; `GetTileLayer` uses `tile.base() >> (Map::LogX() + Map::LogY())`.
- [x] 1.5 Add `Map::WrapToLayer(TileIndex tile, MapLayer layer)` returning the layer base plus `tile & plane_mask`; reimplement `Map::WrapToMap` as `WrapToLayer(tile, MapLayer::Surface)` and mark it `@deprecated` with a pointer to r2mlm3. The old `tile_mask` member is gone. It was the whole-map bit mask, and a three-layer size is not a power of two, so wrapping uses `plane_mask`. The `@deprecated` mark is the Javadoc tag only, so the three remaining callers do not warn before r2mlm3 moves them.
- [x] 1.6 Make `RandomTileSeed` use `Map::WrapToLayer(TileIndex{r}, MapLayer::Surface)` and document that `RandomTile()` always yields a surface tile.
- [x] 1.7 Make `Map::ScaleByLandProportion` divide by `plane_size`, and make `Map::CountLandTiles` count only the surface plane, with comments explaining that land is a surface concept.
- [ ] 1.8 Change the debug `TileAdd` asserts to check the stacked Y bound and add `assert(GetTileLayer(result) == GetTileLayer(tile))`, so any offset arithmetic that crosses layers fails loudly in Debug builds (I-2, I-3). Build both configurations. The asserts are in the code. The build waits on the toolchain.

### Group 2 - Plane-correct edge and wrap predicates

{{MEDIUM}}
- [x] 2.1 Rewrite `IsInnerTile` to use `TilePlaneY(tile)` and update the Javadoc of `IsInnerTile` and `SetTileType` to say the void fence exists in every layer.
- [x] 2.2 Document in `IsValidTile` and `GetTileType` that `Map::Size()` covers all layers; keep the code unchanged.
- [x] 2.3 Rewrite `TileAddWrap` to compute in plane coordinates and rebuild the result with `TileXYL(x, y, GetTileLayer(tile))`.
- [x] 2.4 Rewrite `AddTileIndexDiffCWrap` the same way, bounding `y` by `Map::SizeY()` in the plane.
- [x] 2.5 Make `TileIndexToTileIndexDiffC` assert both tiles share a layer, and make `DiagdirBetweenTiles` return `DiagDirection::Invalid` when the layers differ; comment both.
- [x] 2.6 Rewrite `DistanceFromEdge` and `DistanceFromEdgeDir` with `TilePlaneY`.
- [x] 2.7 Rewrite `TileVirtXYClampedToMap` to derive the layer from the stacked world Y (clamped to the allocated layers), then clamp X and plane Y inside that layer.
- [x] 2.8 Rewrite `GetClosestWaterDistance` to walk plane coordinates and build candidate tiles with `TileXYL` in the origin tile's layer; its no-land fallback iterates only the origin layer.

### Group 3 - Plane distances

{{SIMPLE}}
- [x] 3.1 Rewrite `DistanceManhattan`, `DistanceSquare`, `DistanceMax` and `DistanceMaxPlusManhattan` to use `TilePlaneY`.
- [x] 3.2 Extend each Javadoc with one sentence stating the distance ignores the layer and why (portals are vertical; AD-5).
- [x] 3.3 Add the same note to the declarations in `src/map_func.h`.
- [ ] 3.4 Build both configurations and run the existing `tilearea` tests.

### Area Mandatory Reading - Controller and view wiring

_Applies to Groups 4-5._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/map_func.h](../../../../src/map_func.h) | 200-223 | `Map::Iterator` and `Map::IterateWrapper`: the iterator pattern `MultiLayerMap::IterateLayer` copies, with begin and end bounded by one plane. |
| [src/map_layer_type.h](../../../../src/map_layer_type.h) | Live definitions; no fixed range | Produced by r2mlm1 Group 1. `MapLayer` and `MAX_MAP_LAYERS` are authoritative; this plan adds no enumerator. |
| [src/map_layer_gui.cpp](../../../../src/map_layer_gui.cpp) | Live definitions; no fixed range | Produced by r2mlm1 Group 2. `CycleMainViewportLayer` and `SetViewportLayer` are edited in Group 5; their interim bodies stay otherwise intact. |
| [src/CMakeLists.txt](../../../../src/CMakeLists.txt) | 266-270 | Alphabetical `add_files` neighbourhood for the new `map_layer.h` / `map_layer.cpp`. |

### Group 4 - MultiLayerMap controller core

{{MEDIUM}}
- [x] 4.1 Create `src/map_layer.h` with a Javadoc block on `struct MultiLayerMap` that tells the AD-3 story: `Map` owns storage, `MultiLayerMap` owns layer meaning, and gameplay never adds `Map::PlaneSize()` by hand.
- [x] 4.2 Implement `GetLayerCount()` (forwards `Map::LayerCount()`), `IsValidLayer(MapLayer)` and `GetLayerBase(MapLayer)` with asserts on invalid layers.
- [x] 4.3 Implement `IsTileInLayer(TileIndex, MapLayer)`, `TileInLayer(TileIndex, MapLayer)` (same plane x and y in the target layer) and `ProjectToSurface(TileIndex)`.
- [x] 4.4 Implement `IterateLayer(MapLayer)` returning an iterable over exactly one plane, reusing the `Map::Iterator` shape.
- [x] 4.5 Implement `RandomTileInLayer(MapLayer layer, uint32_t r)` on top of `Map::WrapToLayer`, documenting that callers pass `Random()` so RNG traces stay at the call site.
- [x] 4.6 Create `src/map_layer.cpp` for any non-inline member (at least the layer-count validation helper) and register both files in `src/CMakeLists.txt`.

### Group 5 - View cycling bound to the controller

{{SIMPLE}}
- [x] 5.1 Change `CycleMainViewportLayer` to wrap at `MultiLayerMap::GetLayerCount()` and update its comment (AD-7).
- [x] 5.2 Make `SetViewportLayer` assert `MultiLayerMap::IsValidLayer(layer)` and fall back to `MapLayer::Surface` in release builds.
- [x] 5.3 Disable `WID_TN_MAP_LAYER` and `WID_TE_MAP_LAYER` when `MultiLayerMap::GetLayerCount() == 1`, refreshed in the same `OnInvalidateData` that updates the icon, and add a disabled-state tooltip string `STR_TOOLBAR_TOOLTIP_MAP_LAYER_SINGLE :{BLACK}This game has only one map layer`.
- [ ] 5.4 Build and confirm the button shows as disabled in a normal game (one layer, AD-11).

### Area Mandatory Reading - Tests and neutrality

_Applies to Groups 6-7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tests/tilearea.cpp](../../../../src/tests/tilearea.cpp) | 1-60 | Catch2 test shape that allocates a map directly with `Map::Allocate` and checks `TileX` / `TileY`; the new test file follows it. |
| [src/tests/CMakeLists.txt](../../../../src/tests/CMakeLists.txt) | 1-23 | `add_test_files`: where `map_layer.cpp` is registered. |
| [src/map_layer.h](../../../../src/map_layer.h) | Live definitions; no fixed range | Produced by Group 4. Every `MultiLayerMap` member named in `## Outcome and Boundaries` must exist before Group 6 starts; if one is missing, finish Group 4 first. |
| [docs/zz-reach2/upgrades/2026-10/tools/baseline-digests.txt](tools/baseline-digests.txt) | Live definitions; no fixed range | Produced by r2rvm task 2.5 (master plan `## Baseline Data`). Reference for the neutrality check in task 7.3. |

### Group 6 - Unit tests

{{SIMPLE}}
- [x] 6.1 Create `src/tests/map_layer.cpp` and register it; each `TEST_CASE` gets a one-line comment stating the invariant it guards.
- [x] 6.2 Test that `Map::Allocate(64, 64, 3)` yields `Size() == 3 * PlaneSize()` and `LayerCount() == 3`, and that `Map::Allocate(64, 64)` keeps `Size() == PlaneSize()`.
- [x] 6.3 Test I-1 by round-tripping `TileXYL(x, y, layer)` through `TileX`, `TilePlaneY`, `GetTileLayer` for corners and centres of all three layers, and check `TileY` is stacked.
- [x] 6.4 Test the per-layer void fence with `freeform_edges` off and on: `IsInnerTile` is false exactly on each plane's edge rows and columns.
- [x] 6.5 Test I-2: for every inner tile of a 64x64x3 map and every `DiagDirection` and `Direction`, the one-step neighbour has the same `GetTileLayer`. A step that leaves the plane is the original off-map step from the north or west border, and Debug `TileAdd` already rejects it, so the test takes every step that stays on the plane. With freeform edges on, that is every direction from every inner tile.
- [x] 6.6 Test that the four distance functions return 0 for `TileInLayer(t, other)` and plane values otherwise, and that `DiagdirBetweenTiles` across layers is `DiagDirection::Invalid`.
- [x] 6.7 Test that `IterateLayer` yields exactly `PlaneSize()` tiles all in that layer, and that `RandomTileInLayer` and `RandomTileSeed` stay in their layer for 10 000 seeds.

### Group 7 - Behaviour neutrality

{{SIMPLE}}
- [ ] 7.1 Build both configurations with no new warnings; run `ctest` and confirm all baseline tests plus the new ones pass.
- [ ] 7.2 Start, save and reload a new game in a Debug build; confirm no assert fires (the new `TileAdd` layer assert included).
- [ ] 7.3 Regenerate the four baseline saves of master plan `## Baseline Data` with this build and compare digests with `baseline-digests.txt`; they must be identical (AD-11). Any difference stops this plan.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| The new `TileAdd` layer assert fires during task 7.2 | [src/map.cpp](../../../../src/map.cpp) | Debug `TileAdd` (lines 88-104), then the first caller frame in the stack trace | The caller is not obviously adding a fixed small offset | Decide whether the caller is a real cross-layer bug (record it for the r2mlm3 ledger) or a valid wrap that must use `TileAddWrap`. |
| `AllocateWaterRegions` crashes or asserts after task 1.2 | [src/pathfinder/water_regions.cpp](../../../../src/pathfinder/water_regions.cpp) | `AllocateWaterRegions` | The crash involves region indices derived from `TileY` | Confirm water regions are sized per plane (surface only); the guard itself belongs to r2mlm3 task 7.6. |
| `TileVirtXYClampedToMap` callers misbehave after task 2.7 | [src/aircraft_cmd.cpp](../../../../src/aircraft_cmd.cpp) | Lines 120-130 and 1350-1360 | Aircraft positions near the map edge are wrong | Check the only callers' expectations; aircraft stay on the surface in version 1. |
| A Catch2 test cannot reach `_settings_game` | [src/tests/mock_environment.h](../../../../src/tests/mock_environment.h) | The fixture declarations | Linking or initialisation of settings fails | Find how existing tests set up globals. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [src/map_func.h](../../../../src/map_func.h) | 198-656 | Inventory of every addressing helper; showed which keep their formula (`TileXY`, `TileY`, `TileVirtXY`) and which need plane semantics. |
| [src/map.cpp](../../../../src/map.cpp) | 18-309 | Wrap, clamp and distance helpers; decided Groups 1 to 3. |
| [src/tile_map.h](../../../../src/tile_map.h) | 89-164 | Void-fence predicate behind the per-plane fence. |
| Repository grep of `WrapToMap`, `ScaleByLandProportion`, `CountLandTiles`, `TileVirtXYClampedToMap` | Callers in `industry_cmd.cpp`, `newgrf_house.cpp`, `newgrf_commons.cpp`, `town_cmd.cpp`, `aircraft_cmd.cpp`, `genworld.cpp`, `industry_gui.cpp` | Decided to keep `WrapToMap` as a deprecated surface wrapper and leave caller migration to r2mlm3. |
| [docs/zz-reach2/architecture/pathfinder/archi-pathfinder.md](../../architecture/pathfinder/archi-pathfinder.md) | 238 | The octile heuristic paragraph; one of the reasons distances are plane distances. |

## Completion Conditions

- All symbols listed in `## Outcome and Boundaries` exist with Javadoc and the documented semantics.
- `src/tests/map_layer.cpp` passes and covers I-1 to I-5 on a three-layer allocation.
- Live games still allocate one layer; the layer button is disabled in them.
- The four baseline digests are unchanged.
