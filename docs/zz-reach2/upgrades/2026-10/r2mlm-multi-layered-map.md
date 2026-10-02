# R2MLM Master Plan - Multi-Layered Map (Surface, Underground, Space)

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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 60-128 | `## Architecture Decisions` (AD-1 to AD-12) and `## Cross-plan Invariants` (I-1 to I-8) of this plan. They are the canonical authority every sub-plan consumes: the layer vocabulary, the stacked-plane addressing, the split between `Map` and `MultiLayerMap`, the portal model and the savegame rule. A checkpoint task that accepts a sub-plan must test that sub-plan against these decisions, not against the sub-plan's own restatement. |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-203 | `### 1. Command as the only world mutation`, `### 2. Pool-indexed game objects`, `### 3. Tile map as a packed array`. Commands are the determinism boundary, pools store typed IDs and raw `TileIndex` values, the world is one packed `Tile` array. This is why AD-2 addresses layers inside `TileIndex`: pools, orders and command arguments need no format change, and no command gains a layer argument. |
| [docs/zz-reach2/architecture/landscape/archi-landscape.md](../../architecture/landscape/archi-landscape.md) | 116-159 | `## From a coordinate to a TileIndex` and `## Map size, void tiles, and terraforming`: the `TileXY` formula, power-of-two axes, and the void-border rule enforced by `IsInnerTile` / `SetTileType`. AD-2 stacks planes with exactly this formula and relies on each plane's void fence to keep one-tile neighbour steps inside a layer (I-2). |
| [src/map_func.h](../../../../src/map_func.h) | 198-367 | `struct Map`: the existing map controller (`LogX`, `SizeX`, `SizeY`, `Size`, `MaxY`, `WrapToMap`, `ScaleByLandProportion`, `Iterate`). AD-3 extends it and places `MultiLayerMap` above it; every checkpoint must confirm which members are per-plane and which cover all layers. |
| [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | 368-376 | `## Invariants`: same command stream, same RNG draws, no GUI-only state in the simulation. Constrains AD-4 (layer initialisation draws no `Random()`) and AD-7 (`Viewport::layer` is view state only). |

## Outcome

OpenTTD gains two extra maps that sit under and above the existing land map. The player switches the view between them with one toolbar button placed right next to the map button. Buses ride elevators between the surface and the underground. Trains drive into a special tunnel, built with new graphics, and continue on tracks the player builds underground. Every layer is simulated every tick, saved, loaded, and kept in sync in multiplayer.

## Key Points

1. **Layer button.** A new button in the main toolbar, immediately to the right of the map button, cycles the main view through Surface, Underground and Space, and back to Surface. Its icon and tooltip show the layer being viewed. The scenario editor toolbar gets the same button.
2. **Independent maps under a multi-layered map controller.** The Underground and Space maps are complete, independent maps with their own terrain, rules and drawing. A new controller, `MultiLayerMap`, sits above the existing `Map` controller, owns the set of layers, and is the API that switches which map a view shows.
3. **Buses take elevators.** A road elevator joins a surface road to an underground road. Buses drive into it, ride, and drive out on the other layer, and the road pathfinder routes through it.
4. **Trains take a special tunnel.** A rail layer portal, drawn with a new graphical asset, carries trains from surface track to underground track. The player continues building track, stations, depots and signals underground.
5. **Graphical assets.** Layer icons, underground and space ground, elevators and portals are generated with AI image calls and packed into OpenTTD's own extra GRF.
6. **Acceptance.** Savegames, multiplayer determinism, performance and documentation are verified for the whole epic.

## Architecture Decisions

### AD-1 Layer vocabulary (owner: r2mlm1)

`enum class MapLayer : uint8_t { Surface = 0, Underground = 1, Space = 2, End, Invalid = 0xFF }` lives in the new `src/map_layer_type.h`, with `MAX_MAP_LAYERS = to_underlying(MapLayer::End)`. The numeric values are the stacking order and are implicit in every saved `TileIndex`, so they are never reordered. Display names, tooltips and toolbar icons per layer live in `src/map_layer_gui.cpp` (`GetMapLayerName`, `GetMapLayerIcon`).

### AD-2 Stacked-plane addressing (owner: r2mlm2)

All layers share the one existing `TileIndex` space. Layer `L` is a full plane of `Map::SizeX() x Map::SizeY()` tiles stored at `[L * Map::PlaneSize(), (L + 1) * Map::PlaneSize())`. Because `TileXY(x, y) = (y << LogX) + x`, this is the same as stacking the planes along the Y axis.

- `TileY(t)` keeps its formula and returns the **stacked** Y (`L * SizeY + plane_y`). `TileXY(TileX(t) + dx, TileY(t) + dy)` therefore stays in the layer of `t`.
- `TilePlaneY(t)` returns `plane_y`, `GetTileLayer(t)` returns `L`, and `TileXYL(x, plane_y, L)` builds a tile.
- World coordinates are stacked the same way (`y_pos` includes the layer band), so `TileVirtXY`, the vehicle tile hash, the vehicle viewport hash, signs and the k-d trees keep working without a layer field.
- Each plane keeps its own void fence: `IsInnerTile` tests `TilePlaneY`. One-tile neighbour steps from a non-void tile never leave its layer.

Rejected alternatives: swapping an "active layer" tile array during the tick (every layer must simulate every tick, pools store raw `TileIndex`, every command would need a layer argument); a separate layer field with plane-local `TileY` (every existing `TileXY(x, y)` call would silently fall back to the surface); a per-tile "underground" flag in one plane (layers would not be independent maps and the view could not switch maps).

### AD-3 Two controllers (owner: r2mlm2; view part r2mlm1 and r2mlm5)

- `Map` (`src/map_func.h`) stays the low-level storage and geometry controller. `Map::Allocate(size_x, size_y, layer_count)`. `Map::Size()` becomes the tile count of all layers (storage bound, `IsValidTile` bound, `Map::Iterate()` range). `Map::PlaneSize()` is one layer. `Map::SizeX/SizeY/MaxX/MaxY/LogX/LogY` stay per plane. `Map::LayerCount()` reports allocated layers.
- `MultiLayerMap` (`src/map_layer.h`, `src/map_layer.cpp`) is the multi-layered map controller and the only API for layer semantics: `GetLayerCount`, `IsValidLayer`, `IterateLayer`, `TileInLayer`, `ProjectToSurface`, `RandomTileInLayer`, `GetRules`, `InitialiseLayer`, and the per-layer tile loop. Gameplay code never adds `Map::PlaneSize()` to a tile by hand.
- Switching maps for the player is view state owned by `src/map_layer_gui.cpp`: `SetViewportLayer(Window *w, MapLayer layer)` and `CycleMainViewportLayer()`.

### AD-4 Independent layers (owners: r2mlm4, r2mlm5)

Each layer is an independent map: own void fence, own `MapLayerRules` (allowed construction, terraform, water, trees, towns, industries, climate, flat base height, ground style), own plane initialiser, own ground drawing, own smallmap view and own viewport band. Surface generation is unchanged: for the same seed it produces a bit-identical surface plane. Initialising Underground and Space never calls `Random()`.

### AD-5 Distances are plane distances (owners: r2mlm2, r2mlm3)

`DistanceManhattan`, `DistanceSquare`, `DistanceMax` and `DistanceMaxPlusManhattan` measure in the plane (`TilePlaneY`). Portals are vertical, so plane distance is the true travel distance and cargo payment cannot be inflated by a layer offset. Code that needs "same layer" tests `GetTileLayer` equality explicitly. YAPF estimates (`OctileDistanceCost`) also use plane coordinates: a stacked-Y estimate overestimates across a portal and trips the closed-node consistency `NOT_REACHED` in `yapf_base.hpp`. Town association (nearest town, local authority) projects to the surface with `MultiLayerMap::ProjectToSurface`.

### AD-6 Every layer is simulated (owner: r2mlm4)

`CallVehicleTicks` already walks the whole vehicle pool. `RunTileLoop` applies the shared LFSR cursor `_cur_tileloop_tile` to each layer plane in turn, so no new saved state is needed. Climate, flooding and growth effects run only where `MapLayerRules` allow them.

### AD-7 Viewing a layer is view state (owners: r2mlm1, r2mlm5)

`Viewport::layer` selects which band a viewport draws and picks. It is client state: commands and tick code never read it (desync invariant 3). The toolbar button cycles the main viewport through `MultiLayerMap::GetLayerCount()` layers.

### AD-8 Cross-layer movement only through layer portals (owner: r2mlm7)

The new `TileType::LayerPortal` (value 11) is a pair of tiles at the same plane (x, y) in two adjacent layers, built and removed together by `Commands::BuildLayerPortal`. Each end has one door side and one shaft side, and the pair's shaft direction is the reverse of this end's. Leaving an end through its shaft side enters the pair through its shaft side, so a vehicle keeps its heading (drive-through). `GetLayerPortalNextTile(tile, exitdir)` is the single neighbour function used by followers, vehicle controllers and the signal walk. Traversal is a tile-edge relocation without wormhole state; visibility toggles at door frames like tunnel mouths. The bus elevator (r2mlm8) is a road portal; the special train tunnel (r2mlm9) is a rail portal.

### AD-9 Savegame (owner: r2mlm4)

`SaveLoadVersion::MapLayers` is inserted immediately before `MaxVersion`. `MAPS` gains a `layers` field. The RIFF planes `MAPT` to `MAP8` store every layer in `TileIndex` order. Files without the field load as one surface plane, and `AfterLoadGame` initialises the missing planes. `LayerPortal` tiles only exist in files at or after `MapLayers`.

### AD-10 Graphics (owner: r2mlm6)

New sprites live in OpenTTD's extra GRF (`media/baseset/openttd`) as Action 5 type `0x1C` "Map layer graphics", mapped to `SPR_MAP_LAYER_BASE` in `src/table/sprites.h`. Images are generated with the `GenerateImage` AI tool and quantised to the DOS palette by a repeatable script. Until they exist, plans draw named stand-ins (existing sprites) behind the constants that will replace them.

### AD-11 Behaviour-neutral staging (owners: r2mlm2 to r2mlm4)

r2mlm2 and r2mlm3 keep live games at `Map::LayerCount() == 1` and exercise three layers only in unit tests. r2mlm4 switches new games and loaded games to `MAX_MAP_LAYERS`. Each earlier plan can therefore merge without changing gameplay.

### AD-12 Version-1 scope

In scope: the three layers; switching the view; road elevators (road, not tram); rail portals; building rail, road, stations, depots and signals underground. Out of scope (see `## Deferred Points`): construction in Space; trams, ships and aircraft crossing layers; towns, houses, industries, trees or water off the surface; catchment for underground stations; script API and NewGRF layer awareness.

## Cross-plan Invariants

- **I-1** `t == GetTileLayer(t) * Map::PlaneSize() + TilePlaneY(t) * Map::SizeX() + TileX(t)` for every tile.
- **I-2** One-tile steps (`TileAddByDiagDir`, `TileAddByDir`) from a non-void tile stay in its layer.
- **I-3** During simulation only `GetLayerPortalNextTile`, built on `MultiLayerMap::TileInLayer`, yields a tile in another layer.
- **I-4** `Viewport::layer` and other view state never influence commands, ticks or `Random()`.
- **I-5** `Map::Size()` bounds the storage of all layers; per-plane logic uses `Map::PlaneSize()` and `TilePlaneY()`.
- **I-6** The same seed yields an identical surface plane before and after the epic.
- **I-7** A save written by this epic round-trips every layer; a pre-epic save loads, gains the missing planes, and plays.
- **I-8** Every `TileType::LayerPortal` end has a pair whose shaft direction is reversed.

## Plan Index

| Plan | Point | Delivers | Depends on |
| --- | --- | --- | --- |
| [r2mlm1-layer-toolbar-button.md](r2mlm1-layer-toolbar-button.md) | 1 | `MapLayer` vocabulary, `Viewport::layer`, the layer button next to the map button in the main and scenario-editor toolbars | none |
| [r2mlm2-layer-addressing-and-controller.md](r2mlm2-layer-addressing-and-controller.md) | 2 | Stacked-plane addressing, `Map` extension, `MultiLayerMap` core, plane distances, unit tests | r2mlm1 |
| [r2mlm3-layer-call-site-migration.md](r2mlm3-layer-call-site-migration.md) | 2 | Classification and migration of every map-geometry call site, surface digest check | r2mlm2 |
| [r2mlm4-layer-world-lifecycle-and-savegame.md](r2mlm4-layer-world-lifecycle-and-savegame.md) | 2 | Layer rules, plane initialisers, per-layer tile loop, construction gating, savegame, three live layers | r2mlm3 |
| [r2mlm5-layer-viewport-switching.md](r2mlm5-layer-viewport-switching.md) | 1, 2 | Viewport bands, picking, filtering, layer ground, smallmap per layer, real switching behind the button | r2mlm4 |
| [r2mlm6-layer-graphics-assets.md](r2mlm6-layer-graphics-assets.md) | 5 | AI-generated icons, ground, elevator and portal sprites in `openttd.grf` | r2mlm1; runs in parallel with r2mlm2 to r2mlm5 |
| [r2mlm7-layer-portal-foundation.md](r2mlm7-layer-portal-foundation.md) | 3, 4 | `TileType::LayerPortal`, pair command, neighbour contract, follower, signal and reservation integration | r2mlm5 |
| [r2mlm8-bus-elevators.md](r2mlm8-bus-elevators.md) | 3 | Road elevators: placement GUI, road vehicle traversal, pathfinding, underground bus routes | r2mlm7 |
| [r2mlm9-rail-layer-portals.md](r2mlm9-rail-layer-portals.md) | 4 | Rail portals: placement GUI, train traversal, reservations, signals, underground track building | r2mlm7 |
| [r2mlm10-multi-layer-acceptance.md](r2mlm10-multi-layer-acceptance.md) | 6 | Multiplayer, savegame matrix, performance, architecture documentation | r2mlm8, r2mlm9 |

r2mlm8 and r2mlm9 may run in either order after r2mlm7. Art from r2mlm6 is optional for every plan: each consumer has a stand-in and an explicit swap task.

## Deferred Points

These were considered and deliberately left out of the 2026-10 plans. A later epic should start from here.

- Construction and units in Space (space elevator, rockets, orbital stations).
- Metro catchment: underground stations serving the houses directly above them.
- Trams through elevators, ships and aircraft across layers.
- Script API (`ScriptMapLayer`) and NewGRF variables that expose the layer.
- Changing the layer count of an existing game beyond the `AfterLoadGame` upgrade.

## Engineering Conventions

- **Comment style (user rule, overrides `CODINGSTYLE.md` comment syntax).** Inline comments are written exactly as `//comment` with no space after `//`. Every function and every parameter gets a Javadoc block (`/** ... @param ... @return ... */`). `if` and loop blocks get a short business-level comment; obvious statements get none. When refactoring, port existing comments and update them; never drop them.
- **Naming.** Capitalise type names and product terms in prose (Layer, Tile, Map, MultiLayerMap); refer to variables in their exact casing. Code follows OpenTTD naming: `this->member`, `_global`, CamelCase functions, `snake_case` variables, CamelCase scoped enumerators, file suffixes `_type.h`, `_map.h`, `_cmd.h` / `_cmd.cpp`, `_gui.cpp`.
- **Files.** New sources are registered in `src/CMakeLists.txt` (`add_files`), new tests in `src/tests/CMakeLists.txt`. No created source, test or JSON file name starts with `r2`. No Markdown in source directories. Helper scripts go in `docs/zz-reach2/upgrades/2026-10/tools/`.
- **Repository safety.** Never run `git checkout`, `git reset` or `git stash`, and never move or delete files; copying is allowed. Stand-ins are retired by editing code, not by deleting files.
- **Strings.** Only `src/lang/english.txt` is edited; translations fall back to English.

## Build and Test Commands

Group 1 verifies and, if needed, corrects these. Every sub-plan reuses them.

- CMake: `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe` (not on `PATH`; call it by full path or prepend its folder to `PATH` in the session).
- Configure: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static`
- Build: `cmake --build build --config RelWithDebInfo --target openttd openttd_test` (use `--config Debug` when asserts are needed).
- Unit tests: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- Run: from `build/`, `.\RelWithDebInfo\openttd.exe`. Base graphics come from the TTD files copied into `build/baseset/` by task 1.5.
- Surface digest: `python docs/zz-reach2/upgrades/2026-10/tools/savegame_plane_digest.py <save>` (written by task 1.7).

## Implementation Tasks

### Area Mandatory Reading - Toolchain and baseline

_Applies to Group 1._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 22-48 | `### Windows` and `### CMake products`: Visual Studio 2022 with vcpkg static triplets, and the `openttd` / `openttd_test` targets. Contract for tasks 1.1 to 1.4. |
| [media/baseset/openttd/CMakeLists.txt](../../../../media/baseset/openttd/CMakeLists.txt) | 1-5 | The leading comment: `openttd.grf` is regenerated only when NFORenum and GRFCodec are found, otherwise the cached copy in git is used. The baseline build must not need those tools. |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 90-177 | `## Chunk stream`, `## Gamma lengths`, `## Chunk types` through `### RIFF`: the eight-byte header, chunk ids, gamma lengths, and the RIFF 28-bit length. Contract for the digest script in task 1.7, which must skip table chunks by gamma length and hash RIFF payloads. |
| [src/saveload/map_sl.cpp](../../../../src/saveload/map_sl.cpp) | 368-382 | `map_chunk_handlers`: the `MAPS` table chunk then the RIFF planes `MAPT` to `MAP8`. The digest script hashes exactly these planes. |

### Group 1 - Toolchain and baseline

Establish a reproducible build and the pre-epic reference data that later plans compare against.

{{MEDIUM}}
- [ ] 1.1 Confirm the Visual Studio 2022 bundled CMake and vcpkg paths listed in `## Build and Test Commands`; if either is missing, stop and ask the user to install the Visual Studio "C++ CMake tools" / vcpkg components. Record the CMake and vcpkg versions in that section.
- [ ] 1.2 Configure `build/` with the static triplet. If vcpkg fails, record the error verbatim in this task and fix the environment; never edit `vcpkg.json` to work around it.
- [ ] 1.3 Build `openttd` and `openttd_test` in RelWithDebInfo and in Debug; record the warning count of each as the baseline.
- [ ] 1.4 Run `ctest` and record the list of passing tests in this task as the baseline.
- [ ] 1.5 Copy (never move) `!kit/TTD/*.grf` and `!kit/TTD/sample.cat` into `build/baseset/`, start `.\RelWithDebInfo\openttd.exe` from `build/`, and confirm the title screen appears without a missing-graphics warning.
- [ ] 1.6 Create `build/baseline.cfg` with `map_x = 8`, `map_y = 8` under `[game_creation]`, and `build/scripts/on_dedicated.scr` containing `save baseline_<seed>` then `quit`; run `.\RelWithDebInfo\openttd.exe -D -x -c baseline.cfg -G <seed>` for seeds 1234 and 98765, then repeat with `map_x = 9`, `map_y = 10`. Record where the saves were written.
- [ ] 1.7 Write `docs/zz-reach2/upgrades/2026-10/tools/savegame_plane_digest.py`: read the 8-byte header, decompress `OTTX` (lzma), `OTTZ` (zlib) or `OTTN`, walk chunk ids, skip non-RIFF chunks by gamma lengths until gamma 0, and print the SHA-256 of each `MAPT` to `MAP8` payload, plus an optional `--tiles N` argument that hashes only the first N tiles of each chunk (N bytes for 8-bit planes, 2N for `MAP2` and `MAP8`), so the surface plane of a layered save can be compared with a one-layer save.
- [ ] 1.8 Run the digest script on the four baseline saves and store the output in `docs/zz-reach2/upgrades/2026-10/tools/baseline-digests.txt`. This file is the reference for I-6.

### Area Mandatory Reading - Layer foundation checkpoints

_Applies to Groups 2-4._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2mlm1-layer-toolbar-button.md](r2mlm1-layer-toolbar-button.md) | 199-205 | `## Completion Conditions` of r2mlm1: the button, the `MapLayer` vocabulary and the interim cycling behaviour that Group 2 accepts. |
| [r2mlm2-layer-addressing-and-controller.md](r2mlm2-layer-addressing-and-controller.md) | 188-193 | `## Completion Conditions` of r2mlm2: addressing primitives, `MultiLayerMap` core and unit tests accepted in Group 3. |
| [r2mlm3-layer-call-site-migration.md](r2mlm3-layer-call-site-migration.md) | 245-250 | `## Completion Conditions` of r2mlm3: every call site ledgered and the surface digest unchanged; accepted in Group 3. |
| [r2mlm4-layer-world-lifecycle-and-savegame.md](r2mlm4-layer-world-lifecycle-and-savegame.md) | 227-233 | `## Completion Conditions` of r2mlm4: three live layers, rules, initialisers, savegame upgrade; accepted in Group 4. |
| [r2mlm5-layer-viewport-switching.md](r2mlm5-layer-viewport-switching.md) | 271-277 | `## Completion Conditions` of r2mlm5: the button really switches maps; accepted in Group 4. |
| [docs/zz-reach2/upgrades/2026-10/tools/baseline-digests.txt](tools/baseline-digests.txt) | Live definitions; no fixed range | Produced by task 1.8. Authoritative reference for I-6 in tasks 3.4 and 4.3. |

### Group 2 - Point 1 checkpoint: the layer button

{{SIMPLE}}
- [ ] 2.1 Execute [r2mlm1-layer-toolbar-button.md](r2mlm1-layer-toolbar-button.md) to its Completion Conditions.
- [ ] 2.2 Verify the button sits immediately right of the map button in the full-width toolbar and in every split arrangement, and in the scenario editor toolbar.
- [ ] 2.3 Verify AD-1: `MapLayer` values and `MAX_MAP_LAYERS` match this plan exactly.
- [ ] 2.4 Mark r2mlm1 as done in `## Plan Index` (append "done" to its Delivers cell).

### Group 3 - Point 2 checkpoint A: addressing and call sites

{{HARD}}
- [ ] 3.1 Execute [r2mlm2-layer-addressing-and-controller.md](r2mlm2-layer-addressing-and-controller.md) to its Completion Conditions.
- [ ] 3.2 Verify AD-2, AD-3 and AD-5 against the merged code: `TileY` unchanged, `TilePlaneY`, `GetTileLayer`, `TileXYL`, `Map::PlaneSize`, `Map::LayerCount` and the plane-distance functions exist with the documented semantics.
- [ ] 3.3 Execute [r2mlm3-layer-call-site-migration.md](r2mlm3-layer-call-site-migration.md) to its Completion Conditions.
- [ ] 3.4 Re-run tasks 1.6 to 1.8 on the new build and diff against `baseline-digests.txt`; any difference blocks Group 4 (I-6).
- [ ] 3.5 Grep the tree for `Map::PlaneSize()` added to or subtracted from a tile index outside `src/map_func.h`, `src/map.cpp`, `src/map_layer.*` and the map chunk loaders (`src/saveload/map_sl.cpp`, `src/saveload/oldloader_sl.cpp`); any such hit violates AD-3 and I-3 and must be fixed. Uses of `PlaneSize()` as a quantity (C4) are fine.
- [ ] 3.6 Mark r2mlm2 and r2mlm3 as done in `## Plan Index`.

### Group 4 - Point 2 checkpoint B: live layers and switching

{{HARD}}
- [ ] 4.1 Execute [r2mlm4-layer-world-lifecycle-and-savegame.md](r2mlm4-layer-world-lifecycle-and-savegame.md) to its Completion Conditions.
- [ ] 4.2 Verify AD-4, AD-6 and AD-9: new games allocate `MAX_MAP_LAYERS`, Underground and Space initialise without `Random()`, the tile loop visits every plane, and `SaveLoadVersion::MapLayers` sits immediately before `MaxVersion`.
- [ ] 4.3 Load each baseline save from task 1.6 in the new build, save it again, reload it, and confirm three layers exist and the surface plane digest (`--tiles` = one plane) equals the baseline (I-6, I-7).
- [ ] 4.4 Execute [r2mlm5-layer-viewport-switching.md](r2mlm5-layer-viewport-switching.md) to its Completion Conditions.
- [ ] 4.5 Manually press the layer button through a full cycle in a new game and confirm each layer draws only its own map, at the same plane position, and that the button icon and tooltip follow (AD-7).
- [ ] 4.6 Mark r2mlm4 and r2mlm5 as done in `## Plan Index`.

### Area Mandatory Reading - Content and units checkpoints

_Applies to Groups 5-7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2mlm6-layer-graphics-assets.md](r2mlm6-layer-graphics-assets.md) | 221-226 | `## Completion Conditions` of r2mlm6: sprite block, constants and visual acceptance checked in Group 5. |
| [r2mlm7-layer-portal-foundation.md](r2mlm7-layer-portal-foundation.md) | 281-287 | `## Completion Conditions` of r2mlm7: portal tile, pair command and neighbour contract accepted in Group 6. |
| [r2mlm8-bus-elevators.md](r2mlm8-bus-elevators.md) | 218-223 | `## Completion Conditions` of r2mlm8: buses round-trip through elevators; accepted in Group 7. |
| [r2mlm9-rail-layer-portals.md](r2mlm9-rail-layer-portals.md) | 232-237 | `## Completion Conditions` of r2mlm9: trains round-trip through rail portals with signals and reservations; accepted in Group 7. |

### Group 5 - Point 5 checkpoint: graphics

{{MEDIUM}}
- [ ] 5.1 Execute [r2mlm6-layer-graphics-assets.md](r2mlm6-layer-graphics-assets.md) to its Completion Conditions (this may start any time after Group 2).
- [ ] 5.2 Verify AD-10: Action 5 type `0x1C`, `SPR_MAP_LAYER_BASE` directly after the bridge-deck block, and `SPR_NEWGRFS_BASE` moved behind the new block.
- [ ] 5.3 Start the game and confirm the sprite-count warning for missing extra graphics does not appear.
- [ ] 5.4 Mark r2mlm6 as done in `## Plan Index`.

### Group 6 - Points 3 and 4 checkpoint A: portal foundation

{{HARD}}
- [ ] 6.1 Execute [r2mlm7-layer-portal-foundation.md](r2mlm7-layer-portal-foundation.md) to its Completion Conditions.
- [ ] 6.2 Verify AD-8 and I-8 against the merged code: `TileType::LayerPortal` is value 11, `GetLayerPortalNextTile` exists and is the only cross-layer neighbour function used by `follow_track.hpp` and `signal.cpp`.
- [ ] 6.3 Verify the YAPF estimate uses plane coordinates (AD-5) by reading `OctileDistanceCost`.
- [ ] 6.4 Mark r2mlm7 as done in `## Plan Index`.

### Group 7 - Points 3 and 4 checkpoint B: buses and trains

{{HARD}}
- [ ] 7.1 Execute [r2mlm8-bus-elevators.md](r2mlm8-bus-elevators.md) to its Completion Conditions.
- [ ] 7.2 Execute [r2mlm9-rail-layer-portals.md](r2mlm9-rail-layer-portals.md) to its Completion Conditions.
- [ ] 7.3 In one game, run a bus line and a train line that both cross layers for two in-game years at fast forward, then save, reload and continue for one more year; confirm no vehicle is lost or stuck and no assert fires in a Debug build.
- [ ] 7.4 Mark r2mlm8 and r2mlm9 as done in `## Plan Index`.

### Area Mandatory Reading - Acceptance and closure

_Applies to Group 8._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2mlm10-multi-layer-acceptance.md](r2mlm10-multi-layer-acceptance.md) | 169-175 | `## Completion Conditions` of r2mlm10: multiplayer, savegame matrix, performance and documentation results that close the epic. |
| [docs/zz-reach2/architecture/README.md](../../architecture/README.md) | 7-30 | `## Subsystems` and `## Still being written`: the index the new architecture page from r2mlm10 must be added to. |

### Group 8 - Point 6 closure

{{MEDIUM}}
- [ ] 8.1 Execute [r2mlm10-multi-layer-acceptance.md](r2mlm10-multi-layer-acceptance.md) to its Completion Conditions.
- [ ] 8.2 Check every Cross-plan Invariant (I-1 to I-8) against the final tree and record the evidence (test name, digest, or manual run) next to each invariant in a short list under this task.
- [ ] 8.3 Copy any still-open item from the sub-plans into `## Deferred Points` so nothing is lost.
- [ ] 8.4 Mark r2mlm10 and the master plan as done in `## Plan Index`.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A sub-plan's Completion Conditions contradict an AD in this plan | The sub-plan linked from [`## Plan Index`](#plan-index) | Its `## Outcome and Boundaries` section | The contradiction is in a concrete symbol or file, not in wording | Decide whether the sub-plan or this plan must change; this plan's AD wins unless the user approves a change. |
| vcpkg or CMake configure fails in task 1.2 | [COMPILING.md](../../../../COMPILING.md) | The Windows section | The failure names a missing library or toolchain component | Find the documented dependency list; it does not authorise editing `vcpkg.json`. |
| The digest script misreads a chunk in task 1.7 | [docs/savegame_format.md](../../../../docs/savegame_format.md) | The chunk type and gamma sections | The live code in `src/saveload/saveload.cpp` (`SlLoadChunk`) disagrees with the architecture page | Cross-check chunk layout details; `saveload.cpp` remains the authority. |
| A multiplayer desync appears in Group 7 or Group 8 | [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | `## When dumps and logs are written` | The dump shows divergence in tile or vehicle data | Capture and replay the desync; authority limited to diagnosis procedure. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/README.md](../../architecture/README.md) | 1-30 | Map of subsystem notes used to pick the docs each sub-plan cites. |
| [docs/zz-reach2/architecture/landscape/archi-landscape.md](../../architecture/landscape/archi-landscape.md) | 24-60, 116-207, 272-276 | Tile storage, indexing, void fence and dispatch facts that led to stacked-plane addressing (AD-2) instead of an active-layer swap. |
| [docs/zz-reach2/architecture/gui/archi-viewport.md](../../architecture/gui/archi-viewport.md) | 15-58, 98-128 | Projection and draw pipeline; showed that a viewport band plus filtering is enough to switch maps (AD-7). |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 90-117, 167-177, 351-389 | RIFF map planes and versioning rules behind AD-9. |
| [docs/zz-reach2/architecture/pathfinder/archi-pathfinder.md](../../architecture/pathfinder/archi-pathfinder.md) | 178-246 | The closed-node consistency check that forces plane-based estimates (AD-5). |
| [docs/zz-reach2/architecture/vehicles/archi-vehicles.md](../../architecture/vehicles/archi-vehicles.md) | 35-77, 198-228 | Vehicle positions, hashes and tick order; confirmed stacked world coordinates need no vehicle layer field. |
| [docs/zz-reach2/architecture/landscape/tunnelbridge-tiles.md](../../architecture/landscape/tunnelbridge-tiles.md) | 5-41 | Tunnel bit layout used as the template for the portal tile (AD-8). |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1342-1373, 1522-1842, 1971-2261 | Toolbar arrays and arrangements that decided the button placement and the r2mlm1 decomposition. |
| [src/pathfinder/follow_track.hpp](../../../../src/pathfinder/follow_track.hpp) | 209-245 | `FollowTileExit` tunnel branch; showed one neighbour function can carry cross-layer movement. |

## Completion Conditions

- Groups 1 to 8 are checked, and every sub-plan in `## Plan Index` is marked done.
- `baseline-digests.txt` exists and the surface digests of the final build match it (I-6).
- Each Cross-plan Invariant has recorded evidence under task 8.2.
- Pressing the layer button cycles Surface, Underground and Space; buses and trains cross layers; saves and multiplayer games stay consistent.
