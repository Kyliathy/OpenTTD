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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 118-127 | `## Cross-plan Invariants` I-1 to I-8: the acceptance criteria. Every result recorded in this plan names the invariant it evidences. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 141-170 | `## Engineering Conventions` and `## Build and Test Commands`: build, test and digest commands; documentation goes under `docs/`, never into source directories. |
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

Entry gate. r2mlm8, r2mlm9, r2mlm12 and r2mlm13 must have every Completion Condition met (check their `## Completion Conditions` against the tree and their task boxes). If any is incomplete, stop Groups 1, 2 and 4; Groups 3 and 5 may start on the parts already delivered. Do not mark an invariant as evidenced from a run that lacks road tunnels, timed elevators or rail portals, and do not change gameplay code from this plan to make a test pass. The user's successful earlier play session predates the fourth-layer and timed-elevator changes; final acceptance remains pending.

2026-10-04 coding-stage hand-off (r2rvm2 task 18.7): the review code fixes are delivered, including the remaining rail portal conversion/accounting corrections in task 18.8. Both configurations build and pass 110/110 unit cases. Acceptance preparation may continue under the resumed goal; final runtime groups still need the representative save, art acceptance and their own evidence. No r2mlm10 task is checked by this hand-off note.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | 16-44 | `## Why a lockstep sim desyncs`: joining clients receive a savegame, then commands; any state not in the save or not deterministic splits the worlds. |
| [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | 119-172 | `## When dumps and logs are written` to `### Cache checks at desync level 2`: how to enable desync logs, command logs and cache checks for the run. |
| [docs/multiplayer.md](../../../../docs/multiplayer.md) | 15-48 | `## 1.0) Starting a server`, `## 2.0) Connecting to a server` and `## 2.1) Connecting to a server over the console`: starting a dedicated server and connecting a client by address. |

### Group 1 - Multiplayer

Closure, 2026-10-04, under the goal's explicit operator/gate allowance: The dedicated-server/GUI-client construction, two-year network run, mid-game joins, client-only layer switching and complete desync-log operator workflow are skipped. No multiplayer determinism or joining-during-wait pass is inferred from singleplayer/unit evidence. Each skipped row is marked `[s]`, never a passing test.

{{MEDIUM}}
- [s] 1.1 Start a dedicated server from `build/` (`openttd.exe -D -d desync=2`) on a 512x512 map and connect one GUI client on the same machine; record the commands used.
- [s] 1.2 From the client, build a bus network with a Surface/Underground road tunnel and two timed elevators to Deep Underground, and an underground rail loop with two rail portals; run vehicles for two in-game years at normal speed. Check the stopped 370-tick elevator ride.
- [s] 1.3 Join a second client mid-game while vehicles are inside road tunnels and rail portals and a bus is waiting in an elevator; confirm it loads, resumes the saved wait and stays in sync.
- [s] 1.4 Switch layers rapidly on one client only; confirm no desync (I-4).
- [s] 1.5 Collect the server and client desync logs and confirm there is no desync or cache-check message; attach the log locations under this task.

### Area Mandatory Reading - Savegame matrix

_Applies to Group 2._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 290-322 | `## Load sequence`: `DoLoad`, `LoadCheck`, old TTO / TTD files through `LoadOldSaveGame`, then `AfterLoadGame`. Each matrix row exercises one path. |
| [docs/zz-reach2/upgrades/2026-10/tools/baseline-digests.txt](tools/baseline-digests.txt) | Live definitions; no fixed range | Produced by r2rvm task 2.5 (master plan `## Baseline Data`). Reference for I-6. |

### Group 2 - Savegame matrix

Closure, 2026-10-04, under the goal's explicit operator/gate allowance: The remaining month-play, TTO, autosave-traffic, scenario-editor-to-game, preview-dialog, large-map and combined three-year traffic operator rows are skipped. Modern paused upgrades and complete-map round trips pass R2RVM2 task 18.1. The plan's legacy example trt00.sv0 is TTD and passes a Debug month/round trip; actual TTO name-conversion failures reproduce in the pre-epic build and are recorded in the master's Deferred Points. No completed full save matrix is claimed. Each skipped row is marked `[s]`, never a passing test.

2026-10-04: the goal's operator/gate allowance skips the pending human art-approval entry gate for the automated rows executed here. The supplied save permits concrete paused round-trip checks now. Remaining traffic, UI and large-map rows are not passed by this evidence; multiplayer and final art acceptance remain separate.

{{MEDIUM}}
- [x] 2.1 New game with underground and portal content: save, load, save; all plane digests identical (I-7).

  Verified 2026-10-04 in Debug using the supplied version-370 game with rail stations, rail portals and road elevators. `build/save/portals-camera-roundtrip-a.sav` and `portals-camera-roundtrip-b.sav` have four 256x256 layers and identical hashes for all ten complete map chunks. Exact lines are in `build/toolbar-review/portals-roundtrip-{a,b}.txt`; the normal-exit log is `camera-fixed.log`. R2MLM1 Group 7B also checks three new-game round trips from every non-surface view. This row evidences paused map-data preservation; it does not cover a save during an elevator wait or road-tunnel traffic.
- [s] 2.2 Each pre-epic baseline save: load, play one month, save, load; four layers present and the surface digest equals the baseline (I-6, I-7).
- [s] 2.3 A TTO save copied from `!kit/TT/`: load, confirm four layers, play one month.
- [s] 2.4 Autosave during traffic through portals: load the autosave and continue.
- [s] 2.5 Scenario editor: build underground roads in a scenario, save the scenario, start a new game from it; underground content is present.
- [s] 2.6 Load dialog preview of a layered save shows correct map size and no error.
- [s] 2.7 4096x4096 four-layer game: record save size and save and load time against a one-layer baseline of the same size.
- [s] 2.8 Combined traffic: in one game, run a bus line and a train line that both cross layers for two in-game years at fast forward in a Debug build, then save, reload and run one more year. No vehicle is lost or stuck and no assert fires (I-3, I-7). This was master-plan task 7.3 until 2026-10-03.

### Area Mandatory Reading - Performance and regression

_Applies to Groups 3-4._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/game-loop/archi-game-loop.md](../../architecture/game-loop/archi-game-loop.md) | 242-309 | `### Normal mode, the order that was verified` and `### One level into the calls`: what runs per tick, so measured costs can be attributed to the tile loop, vehicles or the link graph. |
| [regression/CMakeLists.txt](../../../../regression/CMakeLists.txt) | 1-26 | The `regression` custom target and its suites (`regression`, `stationlist`, `gs`, `gs_compat`). |

### Group 3 - Performance and memory

Closure, 2026-10-04, under the goal's explicit operator/gate allowance: The framerate-window and multi-size process-memory operator measurements, including the conditional cost approval gate, are skipped. No threshold result, timing/memory number, user acceptance or reduced tick rate is inferred. Every layer still ticks as AD-6 requires; M-9 remains an open cost decision. Each skipped row is marked `[s]`, never a passing test.

{{SIMPLE}}
- [s] 3.1 With the framerate window, record `GameLoopLandscape`, `GameLoopTrains` and `GameLoopRoadVehs` on a 1024x1024 map in the baseline build and in the final build with and without underground traffic.
- [s] 3.2 Record process memory after loading 256x256, 1024x1024 and 4096x4096 games in both builds.
- [s] 3.3 If the landscape loop costs more than three times the baseline, stop and ask the user whether non-surface planes may tick at a lower rate (that would be a new decision for the master plan).

### Group 4 - Regression suite

2026-10-04: the human art-approval gate is skipped under the goal's operator allowance for regression verification. These are executable checks of the integrated development tree, not an art-approval or multiplayer pass. Reuse the immutable pre-epic regression result recorded by R2RVM task 1.7; rerunning its build target would write outside the authorised repository, and the baseline has not changed.

{{MEDIUM}}
- [x] 4.1 Run `cmake --build build --config RelWithDebInfo --target regression` in the baseline build and record the result. Equivalent four-suite CTest evidence is reused from R2RVM task 1.7: `D:/Codez/Sandbox/OpenTTD-baseline/build/regression-RelWithDebInfo.log` records 4/4 passing (`regression`, `stationlist`, `gs`, `gs_compat`); the baseline folder is read only here.
- [x] 4.2 Run it in the final build; every suite that passed in the baseline must pass. All four suites pass via the final RelWithDebInfo `regression` target on 2026-10-04 (`build/regression-final.log`, exit 0).
- [x] 4.3 Run the `gs` and `gs_compat` suites with a Debug build and confirm scripts still see one map of the original size (r2mlm3 task 7.5). Both pass (17.47 s and 18.37 s) against unchanged expected outputs in `build/regression-final.log`.
- [x] 4.4 Run `ctest` one final time and record the full list of tests.

  The 2026-10-04 final full RelWithDebInfo run after R2MLM9 task 3.8 passes 115/115 (111 unit cases and all four regression cases, 21.79 s), with the full list in `build/ctest-neighbour-closure-RelWithDebInfo.log`. Debug passes 111/111 unit cases (`build/ctest-neighbour-closure-Debug.log`, 12.06 s). Both configurations build with zero compiler warnings (`build/build-neighbour-closure-*.log`). Earlier both-configuration 111-case evidence remains in `build/ctest-console-input-*.log`. The final full run starts after scripted-save helpers restore the startup script, so regression launches cannot share an active helper script.

### Area Mandatory Reading - Documentation

_Applies to Groups 5-6._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/README.md](../../architecture/README.md) | 7-30 | `## Subsystems` table and `## Still being written`: the index row for the new page. |
| [docs/zz-reach2/architecture/landscape/archi-landscape.md](../../architecture/landscape/archi-landscape.md) | 24-133 | `## Two arrays, ten logical fields` to `## From a coordinate to a TileIndex`: the statements about array size, `TileXY` and `TileY` that now need the layer note, and the tile-type table that gains `LayerPortal`. |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 90-117 | `## Chunk stream`: the `MAPS` description that gains the `layers` field. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 59-116 | `## Architecture Decisions`: the source for the new page; the page describes the delivered code, not the plan. |

### Group 5 - Architecture documentation

{{SIMPLE}}
- [x] 5.1 Write `docs/zz-reach2/architecture/map-layers/archi-map-layers.md` in the style of the existing pages: what a layer is, stacked addressing, `Map` versus `MultiLayerMap`, rules, initialisation, tile loop, viewport bands, portals, savegame, a code map and invariants, all checked against the final code.
- [x] 5.2 Add the page to the `## Subsystems` table in `docs/zz-reach2/architecture/README.md`.
- [x] 5.3 Update `archi-landscape.md`: array size covers all layers, `TileY` is stacked, the void fence exists per layer, `TileType::LayerPortal` is value 11 with a link to the new page.
- [x] 5.4 Update `archi-savegame.md`: `MAPS` has a `layers` field from `SaveLoadVersion::MapLayers`, and the RIFF planes hold all layers.
- [x] 5.5 Update `archi-viewport.md` with one paragraph on the viewport layer band and filtering.

### Group 6 - Closure

{{SIMPLE}}
- [x] 6.1 Grep `src/` for files whose names start with `r2` and for `.md` files; there must be none. No source name starts with `r2`. The only Markdown under `src/` is the pre-existing `src/3rdparty/monocypher/` licence set.
- [x] 6.2 Grep `src/` for `Map::PlaneSize()` added to or subtracted from a tile index outside `src/map_func.h`, `src/map.cpp`, `src/map_layer.*`, `src/saveload/map_sl.cpp` and `src/saveload/oldloader_sl.cpp` (AD-3, I-3); there must be none. The remaining hits are counts, comparisons, the iterator end inside `map_layer.h`, and the identity check in `src/tests/map_layer.cpp`.
- [x] 6.3 Record the evidence for each invariant I-1 to I-8 (test name, digest, log or manual run) under this task.

| Invariant | Recorded evidence | Remaining full acceptance / authorised skips |
| --- | --- | --- |
| I-1 | Passing `TileXYL round trip` in the final unit suite (`src/tests/map_layer.cpp`), including all four layers | None for the addressing identity |
| I-2 | Passing `Neighbour steps stay in their layer` and `Per-layer void fence` | None for the pure neighbour/fence rules |
| I-3 | Passing `Layer portal neighbour mapping and relocation` and `Layer portal kinds and pairs`; road/train movement uses `GetLayerPortalNextTile`. R2MLM13 Group 9C records actual connected-station train round trips | Full road/rail, queue and multi-year traffic matrices are operator skips; short train smoke is not a full traffic pass |
| I-4 | Source audit: view state does not enter commands, ticks or simulation RNG. R2MLM1 Group 7B reads the layer only to normalise GUI VIEW metadata; actual main/editor switching and camera round trips pass | Multiplayer layer switching/join validation is skipped in Group 1; no desync pass is inferred from offline checks |
| I-5 | Passing `Map::Allocate layer count`, `IterateLayer and RandomTileInLayer` and migrated spatial-helper tests; storage chunks contain four planes | Large-map memory/performance measurements are skipped in Groups 2/3 |
| I-6 | All ten Surface chunk prefixes for both seeds (1234, 98765) and both sizes (256x256, 512x1024) match the immutable pre-epic reference; [tools/final-layer-save-digests.txt](tools/final-layer-save-digests.txt), R2RVM2 task 18.1 | None for the recorded generation fixtures |
| I-7 | All four pre-epic saves upgrade to version 370 and round-trip all ten complete map chunks. R2MLM1 Group 7B verifies new games and the supplied portal/station fixture. R2MLM12 task 3.6 verifies authentic version-369 to 370 upgrade. Legacy TTD plays 31 Debug days and round-trips four planes (R2RVM2 Group 18B) | Autosave/in-shaft waits, editor-to-game and large-map rows are operator skips. Actual TTO custom-name failure reproduces pre-epic and is an explicit compatibility gap, not a pass |
| I-8 | Passing `Layer portal neighbour mapping and relocation` and `Layer portal kinds and pairs` cover opposite shafts and supported layer pairs; the actual supplied-save inventory lists paired ends | Full occupied/reserved removal/conversion scenarios remain operator skips |

Evidence counts and exact final logs are recorded in Group 4 and R2MLM9 task 3.8. This task records evidence and its limits; it does not convert skipped scenarios into passing tests.
- [x] 6.4 Copy every open issue found in this plan into the master plan's `## Deferred Points`. The actual save/camera and empty-console defects are fixed by their owning plans; the train movement call now follows I-3. The pre-existing TTO custom-name conversion failure and the outstanding cost/layout decisions are recorded in Deferred Points. Skipped multiplayer, traffic/save, visual and performance acceptance is recorded in each owning group; no acceptance decision is inferred.

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

Closed 2026-10-04 under the goal's operator/gate allowance. Automated save/digest, regression, unit, source-audit and documentation tasks are complete. Groups 1..3 record the skipped operator work; multiplayer, the full traffic/save matrix and memory/performance acceptance are not claimed as passed. The invariant table records actual evidence and remaining validation limits. Original unskipped full-acceptance criteria below remain future validation criteria.

- Multiplayer runs with layer content stay in sync, including mid-game joins.
- Every savegame matrix row passes and surface digests match the baseline.
- Performance and memory numbers are recorded and accepted by the user if above the stated threshold.
- The regression suite and unit tests pass as in the baseline.
- The architecture page exists, the index and the three affected pages are updated, and each invariant has recorded evidence.
