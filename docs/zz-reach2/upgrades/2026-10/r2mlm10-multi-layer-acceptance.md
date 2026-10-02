# R2MLM10 - Multi-Layer Acceptance, Multiplayer and Documentation

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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 119-128 | `## Cross-plan Invariants` I-1 to I-8: the acceptance criteria. Every result recorded in this plan names the invariant it evidences. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 157-174 | `## Engineering Conventions` and `## Build and Test Commands`: build, test and digest commands; documentation goes under `docs/`, never into source directories. |
| [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | 368-376 | `## Invariants`: same command stream, same RNG draws, no GUI-only state in the simulation. The multiplayer tests check exactly these with layers in play. |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 431-442 | `## Invariants a change must not break`: older saves keep loading, version enum order, chunk order. The savegame matrix checks them. |

## Outcome and Boundaries

- Evidence that the epic holds in multiplayer, across the savegame matrix, at acceptable performance and memory, and in the existing regression suite.
- A new architecture page `docs/zz-reach2/architecture/map-layers/archi-map-layers.md`, plus corrections to the landscape and savegame pages and the architecture index.
- Any open issue copied into the master plan's `## Deferred Points`.

This plan writes no gameplay code. A defect found here is fixed in the plan that owns the code (named in the master's `## Plan Index`) and re-verified here.

## Implementation Tasks

### Area Mandatory Reading - Multiplayer

_Applies to Group 1._

Entry gate. r2mlm8 and r2mlm9 must have every Completion Condition met (check their `## Completion Conditions` against the tree and their task boxes). If either is incomplete, stop Groups 1, 2 and 4; Groups 3 and 5 may start on the parts already delivered. Do not mark an invariant as evidenced from a run that lacks elevators or portals, and do not change gameplay code from this plan to make a test pass.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | 16-44 | `## Why a lockstep sim desyncs`: joining clients receive a savegame, then commands; any state not in the save or not deterministic splits the worlds. |
| [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | 119-172 | `## When dumps and logs are written` to `### Cache checks at desync level 2`: how to enable desync logs, command logs and cache checks for the run. |
| [docs/multiplayer.md](../../../../docs/multiplayer.md) | 15-48 | `## 1.0) Starting a server`, `## 2.0) Connecting to a server` and `## 2.1) Connecting to a server over the console`: starting a dedicated server and connecting a client by address. |

### Group 1 - Multiplayer

{{MEDIUM}}
- [ ] 1.1 Start a dedicated server from `build/` (`openttd.exe -D -d desync=2`) on a 512x512 map and connect one GUI client on the same machine; record the commands used.
- [ ] 1.2 From the client, build an underground bus network with two elevators and an underground rail loop with two portals; run vehicles for two in-game years at normal speed.
- [ ] 1.3 Join a second client mid-game while vehicles are inside elevators and portals; confirm it loads and stays in sync.
- [ ] 1.4 Switch layers rapidly on one client only; confirm no desync (I-4).
- [ ] 1.5 Collect the server and client desync logs and confirm there is no desync or cache-check message; attach the log locations under this task.

### Area Mandatory Reading - Savegame matrix

_Applies to Group 2._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 290-322 | `## Load sequence`: `DoLoad`, `LoadCheck`, old TTO / TTD files through `LoadOldSaveGame`, then `AfterLoadGame`. Each matrix row exercises one path. |
| [docs/zz-reach2/upgrades/2026-10/tools/baseline-digests.txt](tools/baseline-digests.txt) | Live definitions; no fixed range | Produced by master task 1.8. Reference for I-6. |

### Group 2 - Savegame matrix

{{MEDIUM}}
- [ ] 2.1 New game with underground and portal content: save, load, save; all plane digests identical (I-7).
- [ ] 2.2 Each pre-epic baseline save: load, play one month, save, load; three layers present and the surface digest equals the baseline (I-6, I-7).
- [ ] 2.3 A TTO save copied from `!kit/TT/`: load, confirm three layers, play one month.
- [ ] 2.4 Autosave during traffic through portals: load the autosave and continue.
- [ ] 2.5 Scenario editor: build underground roads in a scenario, save the scenario, start a new game from it; underground content is present.
- [ ] 2.6 Load dialog preview of a layered save shows correct map size and no error.
- [ ] 2.7 4096x4096 three-layer game: record save size and save and load time against a one-layer baseline of the same size.

### Area Mandatory Reading - Performance and regression

_Applies to Groups 3-4._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/game-loop/archi-game-loop.md](../../architecture/game-loop/archi-game-loop.md) | 242-309 | `### Normal mode, the order that was verified` and `### One level into the calls`: what runs per tick, so measured costs can be attributed to the tile loop, vehicles or the link graph. |
| [regression/CMakeLists.txt](../../../../regression/CMakeLists.txt) | 1-26 | The `regression` custom target and its suites (`regression`, `stationlist`, `gs`, `gs_compat`). |

### Group 3 - Performance and memory

{{SIMPLE}}
- [ ] 3.1 With the framerate window, record `GameLoopLandscape`, `GameLoopTrains` and `GameLoopRoadVehs` on a 1024x1024 map in the baseline build and in the final build with and without underground traffic.
- [ ] 3.2 Record process memory after loading 256x256, 1024x1024 and 4096x4096 games in both builds.
- [ ] 3.3 If the landscape loop costs more than three times the baseline, stop and ask the user whether non-surface planes may tick at a lower rate (that would be a new decision for the master plan).

### Group 4 - Regression suite

{{MEDIUM}}
- [ ] 4.1 Run `cmake --build build --config RelWithDebInfo --target regression` in the baseline build and record the result.
- [ ] 4.2 Run it in the final build; every suite that passed in the baseline must pass.
- [ ] 4.3 Run the `gs` and `gs_compat` suites with a Debug build and confirm scripts still see one map of the original size (r2mlm3 task 7.5).
- [ ] 4.4 Run `ctest` one final time and record the full list of tests.

### Area Mandatory Reading - Documentation

_Applies to Groups 5-6._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/README.md](../../architecture/README.md) | 7-30 | `## Subsystems` table and `## Still being written`: the index row for the new page. |
| [docs/zz-reach2/architecture/landscape/archi-landscape.md](../../architecture/landscape/archi-landscape.md) | 24-133 | `## Two arrays, ten logical fields` to `## From a coordinate to a TileIndex`: the statements about array size, `TileXY` and `TileY` that now need the layer note, and the tile-type table that gains `LayerPortal`. |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 90-117 | `## Chunk stream`: the `MAPS` description that gains the `layers` field. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 60-117 | `## Architecture Decisions`: the source for the new page; the page describes the delivered code, not the plan. |

### Group 5 - Architecture documentation

{{SIMPLE}}
- [ ] 5.1 Write `docs/zz-reach2/architecture/map-layers/archi-map-layers.md` in the style of the existing pages: what a layer is, stacked addressing, `Map` versus `MultiLayerMap`, rules, initialisation, tile loop, viewport bands, portals, savegame, a code map and invariants, all checked against the final code.
- [ ] 5.2 Add the page to the `## Subsystems` table in `docs/zz-reach2/architecture/README.md`.
- [ ] 5.3 Update `archi-landscape.md`: array size covers all layers, `TileY` is stacked, the void fence exists per layer, `TileType::LayerPortal` is value 11 with a link to the new page.
- [ ] 5.4 Update `archi-savegame.md`: `MAPS` has a `layers` field from `SaveLoadVersion::MapLayers`, and the RIFF planes hold all layers.
- [ ] 5.5 Update `archi-viewport.md` with one paragraph on the viewport layer band and filtering.

### Group 6 - Closure

{{SIMPLE}}
- [ ] 6.1 Grep `src/` for files whose names start with `r2` and for `.md` files; there must be none.
- [ ] 6.2 Grep `src/` for `Map::PlaneSize()` added to or subtracted from a tile index outside `src/map_func.h`, `src/map.cpp`, `src/map_layer.*`, `src/saveload/map_sl.cpp` and `src/saveload/oldloader_sl.cpp` (AD-3, I-3); there must be none.
- [ ] 6.3 Record the evidence for each invariant I-1 to I-8 (test name, digest, log or manual run) under this task, for master task 8.2.
- [ ] 6.4 Copy every open issue found in this plan into the master plan's `## Deferred Points`.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A desync is reported in Group 1 | [docs/debugging_desyncs.md](../../../../docs/debugging_desyncs.md) | `# Debugging / reporting desyncs` (lines 1-55) | The desync log alone does not locate the diverging object | Replay the `dmp_cmds` save as described there; the owning sub-plan fixes the cause. |
| A regression suite fails only in the final build | [regression/regression/](../../../../regression/regression/) | The failing script and its expected output | The failure is not a known layer-related difference | Decide whether the expected output must change (ask the user) or the code is wrong. |
| A client cannot join a layered server | [docs/zz-reach2/architecture/game-coordinator/archi-game-coordinator.md](../../architecture/game-coordinator/archi-game-coordinator.md) | `## Advertise, list, and join` (lines 127-183) | The failure happens before map download | Separate network setup problems from map transfer problems. |
| Memory for 4096x4096 three-layer maps is unacceptable | [r2mlm4-layer-world-lifecycle-and-savegame.md](r2mlm4-layer-world-lifecycle-and-savegame.md) | Task 6.6 | The user wants a mitigation | Revisit the decision with the user; this plan only reports. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | 16-172, 368-376 | Multiplayer test design and logging switches. |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 290-330, 431-442 | Load paths covered by the savegame matrix. |
| [regression/CMakeLists.txt](../../../../regression/CMakeLists.txt) | 1-26 | Existing regression suites. |
| [docs/zz-reach2/architecture/README.md](../../architecture/README.md) | 1-30 | Where documentation of a new subsystem is indexed. |

## Completion Conditions

- Multiplayer runs with layer content stay in sync, including mid-game joins.
- Every savegame matrix row passes and surface digests match the baseline.
- Performance and memory numbers are recorded and accepted by the user if above the stated threshold.
- The regression suite and unit tests pass as in the baseline.
- The architecture page exists, the index and the three affected pages are updated, and each invariant has recorded evidence.
