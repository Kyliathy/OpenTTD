> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2df-a-reproduction-notes.md`; exact source SHA256 `1ac42e0d421e8b77dd30216f78729424f593e0ead71d81688a3cbeb2e1328cea`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# R2DF-A - Demo reproduction notes, 2026-10-15

Companion of [R2DF](r2df-demo-fixes-interface-movement-and-planning.md) Group 1. It records the exact reproduction inputs (task 1.1), the native observations (tasks 1.2-1.7) and the published checkpoint (task 1.8).

## Execution notes carried across compactions

- **Location of companion notes.** The user's execution instruction puts helper documents beside the plan with the plan prefix plus a letter (`r2df-a-...`, `r2df-b-...`). The plan's earlier `demo-fixes/` subfolder is therefore not used.
- **Toolchain on this machine (installed 2026-10-15 with the user's approval).** The machine had no compiler. `winget` installed VS 2022 Build Tools 17.14.41 (no IDE) into `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`, the path `build/CMakeCache.txt` expects. Components: VC Tools workload, MSVC 14.44.35207, Windows 11 SDK 10.0.26100, C++ CMake tools, ATL. The cache's toolchain file `D:/Codez/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake` came from a shallow clone of public vcpkg, bootstrapped because each executable's post-build step runs `vcpkg.exe z-applocal`; manifest install is off, so the libraries stay those in `build/vcpkg_installed`.
- **Build command.** Git Bash has no `cmake` on its PATH. Use `MSYS_NO_PATHCONV=1 "/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config RelWithDebInfo --target openttd_test -j 1 -- /p:CL_MPCount=8 /nodeReuse:false`. Without `MSYS_NO_PATHCONV=1`, Git Bash rewrites `/p:CL_MPCount=8` as a path and MSBuild refuses the command.
- **Native observer.** `src/tests/demo_feedback_runtime.cpp` (tag `[.demo-feedback]`), driven by `src/tests/multilayer/tbm/demo_feedback.py --scenario <name> --input <save> --ticks <n>`. Logs go to `logs/demo-fixes/<scenario>/<run-id>/`; captures go to `mlm/reviews/demo-fixes/<scenario>/<run-id>/images/`.
- **Shared input helpers.** `FeedbackMouse`, `FeedbackClick` and `FeedbackTilePoint` moved from `tbm_feedback_runtime.cpp` to `src/tests/native_input_fixture.h`.

## Reproduction inputs (task 1.1)

### Saves

| Input | SHA256 | Meaning |
| --- | --- | --- |
| `src/tests/fixtures/demos/underground/tbm-work-403.sav` | `060a81a3fe4dd1c54245f67d1f800c918257f47c797ab54dbb70d20789333137` | The `tbm-work` checkpoint (play.ps1 menu 11) the user started from. Matches `demo-checkpoints.json`. Never modified. |
| `build/save/autosave/autosave1.sav` (2026-10-13 21:49:20) | `1d05a72064114ff91caf9210019abf7e5db25d3b67f1042c7f86546b41c53474` | Autosave written during the demo, after screenshot 01 (21:44) and before 02 (22:04). |
| `build/save/autosave/autosave2.sav` (2026-10-13 22:12:56) | `a67c3c67e83c70b68f0b55c634973f6171c0993ae38cd0ca42240eb8dd001c49` | Autosave written between screenshots 04 (22:11) and 05 (22:14). |
| `build/save/am1.sav` (2026-10-13 22:13:10) | `8071f83c0116e158ef05bfa037eb833e702358e9466b5fb6745ef48029186fb2` | The user's own save, 14 seconds after autosave2, also between screenshots 04 and 05. |

Screenshot times (file modification times in `docs/Demo2026-06-13/`): 01 21:44, 02 22:04, 03 22:07, 04 22:11, 05 22:14, 06 22:17. `play.cfg` and `hotkeys.cfg` were last written at 22:18, when the game was closed.

### Configuration

- **Launcher.** `play.ps1` runs `build\<Config>\openttd.exe -c build\play.cfg`; `-Config` defaults to `RelWithDebInfo`.
- **Game settings travel in the save.** `reserve_paths`, `forbid_90_deg` and the YAPF penalties are game settings, so the loaded save decides them, not `play.cfg`. The `[pf]` values in `play.cfg` are the new-game defaults: `reserve_paths = false`, `forbid_90_deg = true`, `wait_for_pbs_path = 30`, `path_backoff_interval = 20`, `yapf.rail_layer_portal_penalty = 400`.
- **Client settings from `play.cfg`.** `default_signal_type = 5` (the last signal type built), `show_track_reservation = true`, `right_click_wnd_close = no`, `refresh_rate = 60`, `video_vsync = false`, `video_hw_accel = true`.
- **Hotkeys (`build/hotkeys.cfg`).** `[global]`: `reset_object_to_place = ESC`, `delete_windows = DEL`, `crash_game = ALT+0`, `money = ALT+1`, `update_coordinates = ALT+2`. `[maintoolbar]`: `smallmap = F4,M`, `cycle_map_layer = N`. `[scenedit_maintoolbar]`: `cycle_map_layer` is empty.
- **Which build.** The three Alt entries are declared only under `#if defined(_DEBUG)` (`main_gui.cpp` lines 490-494). An INI group keeps entries it does not know, so a Debug run on any earlier day would leave them in the file. Their presence alone does not prove the demo ran on Debug.
- **Which build: undetermined, but no money cheat.** Company money in the user's saves is £7,495,422 (`tbm-work`), £7,442,597 (screenshot 03), £7,408,175 (`autosave2`) and £7,408,032 (`am1`). None shows the +£10,000,000 of the Debug-only Alt+1 hotkey, so it was never pressed. Both RelWithDebInfo and Debug keep assertions on (`OPTION_USE_ASSERTS`), so a crash would not tell them apart either. `play.ps1` defaults to RelWithDebInfo. The reproduction below uses RelWithDebInfo built from the same cargo, TBM, portal and economy sources as the demo commit `5d2d50eed9` (no diff in those files since).
- **`autosave1.sav` is not the demo world.** It holds three trains, Connected pairs at (12,12) and (13,60) and no project, so the user played another checkpoint around 21:49. It is not used below.

## Native reproduction (tasks 1.2-1.7)

All runs use `python src/tests/multilayer/tbm/demo_feedback.py --scenario <name>` on `tbm-work-403.sav` (unless stated) and the RelWithDebInfo `openttd_test.exe`. Logs are in `logs/demo-fixes/<scenario>/<run-id>/`, captures in `mlm/reviews/demo-fixes/<scenario>/<run-id>/images/`.

| Scenario | Run id | Evidence |
| --- | --- | --- |
| `trace` (orders, packets) | `20261015-191920-664fb931c4` (`autosave2`), `20261015-191918-3b922f9092` (`tbm-work`) | World dumps, orders, per-packet cargo. |
| `hauler-depot` | `20261015-192109-13ceaa5903` | `03-hauler-held.png`; depot decisions. |
| `forced-hauler` | `20261015-192111-8058e56f24` | The held, loaded hauler forced out as in note 3. |
| `unload-order` | `20261015-192003-804f6e70fa` | The user's "Unload all / No loading" order; assertion at the next station stop. |
| `held` | `20261015-192211-7f7528629f` | `02-held-nothing.png`; per-part cargo at the same tick. |
| `new-area` | `20261015-192814-bbf62fa530` | `04-*` and `05-*` captures; palette and quote texts. |
| `obstruction` | `20261015-193308-25d09cb558` | Project 1 commissioned; `DEMO_CUT_BLOCKER`. |
| `poses` | `20261015-193336-ad3a6d2e15` | `07-consists-zoom0..5.png`; widths and overlaps. |
| `map` | `20261015-193540-2c3cd3f24c` | `01-smallmap-*` captures; layer and size log. |
| `framerate` | `20261015-194647-61a06258a7` | `framerate-console.log` (real Windows driver, the player's `play.cfg`). |

### Harness notes for later groups

- Inside a game tick `_current_company` is not the player. Native GUI input from an observer step must run with `_current_company = _local_company`, or `HandleMouseEvents` asserts.
- A tooltip from earlier hovering can sit on the next click target (`FindWindowFromPt` returned `WindowClass::ToolTips`). The observer's `PlayerClick` closes tooltips first, as a real mouse move would.
- Windows refresh enabled and disabled widgets in `OnPaint`. Input that depends on a freshly enabled button (palette Confirm) runs inside a capture, which paints first.
- The demo world uses wallclock time with a frozen calendar (`minutes_per_calendar_year = 0`, the date is always 1 Jan 1980), so `schedule on-next-calendar-month` never fires there. Values from 2 to 11 are clamped back to frozen. A standalone `openttd.exe` run that set 12 and unpaused still never reached a month within 150 s (unexplained; the load-time pause bits were `Normal | CommandDuringPause`). The in-test reading with `-v win32` is the measurement of record.
- `build/save/am1.sav` and `autosave2.sav` contain the R11 wrapped packets. Loading them is safe until the hauler enters a depot: `ConsistChanged` then truncates the phantom load and `CargoRemoval` asserts. Do not use them as inputs for movement scenarios.

## Checkpoint (task 1.8)

Status per report, with the fixing group. "Confirmed" means reproduced natively with the cause named in the log; "refuted" means the trace contradicts it.

### R1 - Small map cannot be made shorter; "0 ft" legend (Group 2)

- **Confirmed.** On Underground contours the window stays at 307x533 after `ResizeWindow(map, 0, -2000)`; `nested_root->smallest_y` is 533, with an empty legend area below five rows.
- **"0 ft" explained.** The demo saves keep `construction.map_height_limit = 0` (the "auto" value, which `genworld.cpp` resolves only when it generates a map). `BuildLandLegend` then makes `0 / delta + 1 = 1` entry. `RebuildColourIndexIfNecessary` sizes each `height_colours` table to `limit + 1 = 1`, so every contour lookup for a tile above height 0 (`height_colours[TileHeight(tile)]`, three sites) reads past the end. The Surface contour map therefore draws the land black; in a Debug build with checked iterators this is a subscript assertion. Group 2 must use a resolved limit (the auto value or the actual highest tile) for both the legend and the colour tables.

### R2 - The small map does not follow the layer hotkey (Group 2)

- **Confirmed.** After `N` the main view moves from Underground (1) to Deep Underground (3); the small map caption stays "Map - Contours - Underground". Its own dropdown does change its layer (captions Surface, Underground, Outer Space, Deep Underground).

### R3 - Alt+1..4 do nothing (Group 2)

- **Confirmed.** After Alt+1, Alt+2, Alt+3 and Alt+4 through `HandleKeypress` the main view stays on layer 3 and the small map is unchanged.

### R4 - "Held: nothing" while the machine carries spoil (Group 4)

- **Confirmed.** At tick 7692 the projects list reads "returning to the exchange bay; 11/50 cuts complete / Held: nothing" (screenshot 02 also shows 11 cuts) while the cutter's wagons carry Earth 64/64 and 62/64 and Rock 128/128 and 124/128 (378 t). `Held:` prints `TBMBlockedReason::None`, not cargo.

### R5 - The loaded hauler stays in its depot (Group 6)

- **Confirmed, primary hypothesis.** The demo network has **no signals at all**: six plain rail tiles, a depot at (13,0), the Invenville station at (13,4)-(13,6) and the pair at (13,10). The depot's segment therefore runs across the Commissioning pair into the corridor. From tick 38 every `CheckTrainStayInDepot` evaluation (`UpdateSignalsOnSegment`) returns Full with the cutter as occupant, first on the Surface key (13,10,0), then in the corridor. The hauler comes back at tick 10160 with exactly the screenshot's load (Earth 32 and 62 t, Rock 64 and 124 t) and stays until the cutter returns to the magazine, about 9,500 ticks later. `03-hauler-held.png` shows "0 mph - Heading for Invenville" as in screenshot 03.
- **Confirmed cause (task 6.1, stated before any code change).** The hold is the real depot's `CheckTrainStayInDepot` at (13,0,0). It is not the magazine exit, and not a red signal after departure. The segment that `UpdateSignalsOnSegment(depot, Invalid)` explores starts at the depot door and crosses the Commissioning pair through the `TileType::LayerPortal` case of `ExploreSegment`. That case crosses every company rail pair "as one straight piece", whatever its work state, so the segment reaches into the corridor. The occupant is the cutter: first on the Surface key, then in the corridor. `GetTileTrackStatus_LayerPortal` already hides a non-Connected pair from routing, so the two owners disagree. Fix: the segment ends at a non-Connected pair (6.2).
- **Secondary hypotheses not observed.** Magazine entry and departure without signal updates cannot matter in a world without signals, and the key's reservation bit always followed the cutter (set on entry, cleared on leaving). No stale bit was seen.
- **Forcing.** Forced out (the user's workaround), the hauler delivers 96 t at Invenville (the spoil receiver's room), runs on to the portal, is turned back because the corridor is busy, delivers another 96 t and returns to the depot with 90 t, where it is held again. With the default order this does not corrupt cargo.

### R6 - A new area needs a full return and park (Groups 4, 11-12)

- **Confirmed.** With project 0 working, the projects list shows "Return and park the whole machine first; every part must be stopped inside and empty", and **New area** on the portal says "This portal already has a TBM project. Open Projects to inspect its work." After Return and park the release waits about 7,000 more ticks for "Wait for spoil delivery and for every hauler to clear the exchange bay" (the user's `autosave2` shows the same phase 11 wait).

### R7 - Consist sprites overlap (Group 5)

- **Confirmed.** Widest authored pose at normal zoom (width x height in pixels; envelopes are 16x24 for N/S, 24x24 for the diagonals and 32x24 for E/W):

  | Part (asset) | N/S | NE/SE/SW/NW | E/W |
  | --- | --- | --- | --- |
  | Cutter (00) | 12x24, fits | 24 x 21-23, fits | 32x19, fits |
  | Deep cutter (01) | 17-19 x 31-32 | 27-32 x 27-32 | 35 x 19-20 |
  | Earth hopper (02) | 19 x 28-29 | 36-38 x 31 | 43x20 |
  | Rock hopper (03) | 16x32 | 32-33 x 29-30 | 42-43 x 20-21 |
  | Hauler (04) | 17-18 x 32 | 31-35 x 29-31 | 36x25 |

  Every part except the cutter exceeds its envelope, in height as well as width. At the real 8-step pitch, hopper pairs overlap by 195-253 opaque pixels in the N/S and diagonal views, against 38-66 for cutter and support (`07-consists-zoom2.png`). The captures lay the packed poses out at that pitch; they are not a moving train.

### R8 - "Cannot plan a supported route" beside the exit (Groups 4, 8-10)

- **Confirmed, matching screenshot 05 word for word.** The request (13,11)-(20,11) beside the door (door SE), made after the release, shows "The boring machine cannot plan a supported route out of the portal through this area", "Machinery and wagons: £0; later geological work: £0.", "Earth: 0 t; Rock: 0 t; supports: 0; work: 0 ticks." and "Final floor: 2 cells; extra working space: 0; new cuts: 0; uncovered: 6.". The planner error is `OutsideAuthorization` (13); the refused proposal has no authorized cells; (15,11)-(20,11) remain; the quote never shows the failing tile (internally (12,13)).
- **Extension flow.** The same request through **Pause and extend** reports "The boring machine can cover only part of this area" and "Extend Train #2; additional cutting: £0." with the same 2/6 split, PARTIAL wording for an unreachable request.
- **New: 90-degree routes under `forbid_90_deg`.** Floors that start at the door and widen to the right, (13,11)-(16,18), and the larger blocks (13,11)-(6,22) and (13,11)-(20,22), are refused with "This excavation route needs sharp rail bends. Allow 90-degree train turns in the game settings, or select a straight corridor." The planner produced a route the game forbids instead of a 45-degree one. (9,11)-(17,20) fails with `Conflict` (18) at (9,20). Only (13,11)-(10,18) and the straight corridor were accepted. Group 8 owns this (I-13 advancing-turn rule).
- **Fixed by Groups 8-9.**
  - `PlanExcavationApproach` supplies the turning and approach cells, and the quotes charge them.
  - The support path picks side cells that avoid 90-degree bends.
  - The demo's request (13,11)-(20,11) is now accepted with 9 approach cells, and dug to completion natively.
  - Group 10 words the quote. See Later groups.

### R9 - Project 1's TBM waits "something is in the way" forever (Group 7)

- **Confirmed, with a cause the plan did not list.** Project 1 at the same portal (floor (13,11)-(10,18), "Reuse Train #2; later cutting: £750") starts at tick 23242. The cutter reaches the rock-side housing (13,10,1) at tick 23322 and enters phase 2 (cutting) with `stuck=false` and no steering target. From tick 23324 the project is `Obstruction` for good. `PrepareTBMSupport` refuses the first cut's first cell (13,11,1), which holds project 0's straight track **and project 0's support record** (`GetTBMSupportTile` is non-null). The support planner reuses a record only when `support->project == project.id`, and plain rail only when it has no record; anything else is "other infrastructure" and refused. `RetireTBMProject` erases the project but never removes or hands over its `_tbm_support` entries, so every retired project leaves orphan records on its corridor.
- **Candidates (a)-(d) refuted for R9.** (a) No return or steering target is involved; the cutter is not travelling. (b) No other train's part stands on the key or in the cut's swept tiles. (c) The key and housing reservation bits belong to the cutter itself. (d) The first cell is plain straight track, not a junction. Stopping the forced-out hauler is irrelevant: the block starts 38 ticks before the hauler is stopped, on another layer.
- **Quote and cut disagree.** The commission quote accepted a route whose first cut the support planner refuses. Group 7 must make both use the same support rule.
- **Fixed by Group 7.** Released support is reused by the next project, the quotes apply the cuts' support rule, and the status names the blocker. See Later groups.

### R10 - Face exchange (R2DF2)

- Not reproduced here; owned by R2DF2 Groups 1-9.

### R11 (new) - "Unload all" corrupts spoil packets (new Group 6B)

- **Confirmed.** During the demo the user changed the hauler's only order to **Unload all** (unload type 1) and **No loading** (load type 4); `tbm-work` has the defaults (0, 0). The user's `autosave2` and `am1` hold hauler wagons whose packets read `[65440][68][9][9][9][1]` (Earth) and `[65344][136][18][18][18][2]` (Rock): one packet wrapped to -96 or -192, plus a 68 or 136 packet, for a sum of 65536.
- **Cause.** The fork's spoil receiver refusal (`economy.cpp` lines 1766-1781, added 2026-10-05) reassigns only the refused part of the Deliver cargo to Transfer on an Unload or Transfer order. Upstream's `VehicleCargoList::Reassign<Deliver, Transfer>` (`cargopacket.cpp` line 596, unchanged since 2014) splits with `sum - action_counts[Transfer] + max_move` where it needs `sum - (action_counts[Transfer] + max_move)`. Upstream always passes the whole Deliver amount, so its split never runs. A partial amount makes the split larger than the packet, which wraps. With 9 t packets and 32 t refused, the running sum reaches 36, the code splits off 68 t and the packet's remainder wraps to -59: the 68 seen in the save.
- **Consequences.** In the live game the next station stop asserts in `VehicleCargoList::Stage` (`AssertCountConsistency`), reproduced by `unload-order`. After a save and load the cache is rebuilt from the packets, the hauler shows 65536 t in a 64 t hopper, and the next depot entry asserts in `CargoRemoval` (the `am1` runs).

### Q1 - Simulation rate versus frame rate (Groups 1, 3)

- **Measured** on `tbm-work` at normal speed with the real Windows video driver and a copy of the player's `play.cfg` (refresh 60, no vsync, hardware acceleration): 1500 ticks in 40.43 s (37.10 ticks/s). The `fps` report gives Game loop 36.95 fps (expected 37.04), Drawing 60.20 fps and Video output 60.20 fps (expected 60.00). A game-loop tick costs 0.13 ms (trains 0.03 ms, landscape 0.02 ms), drawing 0.02 ms, video output 0.44-0.48 ms. The two clocks differ by design, and the demo is far inside the simulation budget. Task 3.5 explains this in the window.

### Remaining unknowns

- Whether the user's demo binary was RelWithDebInfo or Debug (see task 1.1).
- Why the standalone `openttd.exe` measurement never reached a calendar month after unpausing. This affects only the harness; the in-test measurement replaces it.
- Candidate (a) of R9 (the Source-bay return trackdir) was neither exercised nor refuted in general. It is not the cause of R9 and is not fixed without evidence.

## Later groups

- **Group 2 (2026-10-15).** `[smallmap-layer]` (4 cases) passes, as do the neighbouring `[underground-*]`, `[launch-hub-native]` and `[space-native]` cases. The native `map` run `20261015-200304-71c0f1ce6c` shows the minimum fall from 307x533 to 453x283 (wider because the contour legend now lists 0-4,200 ft in two columns), Surface land coloured, and the small map following `N` and Alt+1..4. `build/hotkeys.cfg` was backed up to `build/hotkeys.cfg.bak-2026-10-15` before its Debug entries moved to `CTRL+ALT+0/1/2`; the rewrite keeps its CRLF line endings. Git Bash `sed -i` turns CRLF files into LF, so use Python byte replacement for that file.
- **Group 3 (2026-10-15).** `[demo-fix-window-toggle]` (4 cases) passes, as do `[smallmap-layer]` and the neighbouring `[underground-*]`, `[portal-window]` and `[layer-industry-window]` cases. The native `windows` scenario (new, run `20261015-203551-445b53f0b8`) presses keys on the real main window and global hotkeys: F2 toggles Game Options, M raises a small map covered by the town directory and then closes it, Escape aborts the armed Survey tool and keeps the palette open, and a focused Game Options filter keeps Escape (clear, unfocus, then close). The native TBM journeys still pass after the Escape change: `native_feedback.py` UI run `20261015-203606-f4bc0bae48` (210 assertions) and `--extend-work` run `20261015-203617-c0c4d1407e` (144). Found during the native run: Game Options (like save/load, queries and other dialogs) has a higher z-priority than map and list windows, so `BringWindowToFront` can never lift the small map above it. Under the plan's literal "front-most window" rule, M then did nothing at all. The toggle therefore closes a window when `IsWindowRaisedAsFarAsPossible` holds, meaning every window in front of it has a higher z-priority; Escape still closes the front-most window. Mock unit tests cannot open Game Options (`AddCustomRefreshRates` needs a video driver) or focus an edit box (`SetFocusedWidget` calls the driver), and the real `MainWindow` needs recolour sprites. The unit tests therefore bind Escape on a stand-in main view, and F2 and edit-box precedence are checked natively.
- **Group 4 (2026-10-15).** What changed:
  - **Wait line.** It reads "Waiting: <reason>", "Working normally" or, for a parked, finished or order-following machine, "Not waiting for anything". It comes from `GetTBMWaitLine` and is shared by the projects list and the portal window. The vehicle caption already said "Waiting:". The plan's "Waiting for: …" would not read with the existing reason texts ("the Earth hoppers are full"), so the wording is "Waiting: …", and "paused" became "the project is paused".
  - **Cargo lines.** `SumTBMCutterSpoil` (by consist role, extracted from `DescribeTBMProject`) and `SumTrainSpoil` (by cargo label, for haulers) are pure. Each list row holds a cutter line and one line per hauler, or "No hauler assigned". All rows reserve lines for the project with the most haulers, and the one-second refresh re-lays the list when that count changes.
  - **Release line.** "To finish and release: …", or "To finish and release: ready now", drawn in forced black because the error texts carry `{WHITE}`.
  - **New area on a busy portal.** It calls `PauseAndPlanTBMProjectExtension`, the same entry as Pause and extend. A finished, parking or machineless project keeps the (reworded) refusal.
  - **Refused quotes.** They hide the price, material and time rows (new `WID_UC_QUOTE_COST_STACK`). The floor row says "Planning stopped at tile x, y (marked ! on the map); n of m floor cells reachable", and `SetConstructionPreview` takes a `blocked` tile that it draws red with a "!" label. An accepted quote's floor row now counts "new solid cells" and "cutter positions" separately.
  - **Same message in both flows.** `PreviewTBMExtension` now tests the planner error before partial cover, so an unreachable extension reads "The boring machine cannot plan a supported route into this area", as the commissioning quote does. That string lost "out of the portal" so it fits both flows.
- **Group 4 evidence.**
  - `[demo-fix-tbm-status]` (6 cases, 252 assertions) passes.
  - Neighbouring `[portal-window]`, `[tbm-controls]`, `[tbm-planning-*]`, `[tbm-project-browser]`, `[tbm-replanning-*]` and `[underground-*]` cases pass, except one, listed under Carry forward.
  - Native captures replacing the screenshots: `held` run `20261015-205852-28b039255e` (`02-held-nothing.png` now shows "Working normally", "Carrying: Earth 126/128 t, Rock 252/256 t" and the hauler line; it needs `--ticks 12000`).
  - `new-area` run `20261015-205929-d89833ae8a` (`04-*`, `05-*`; `--ticks 30000 --timeout 380`). The fresh refusal reads "Planning stopped at tile 12, 13 …; 2 of 8 floor cells reachable" with the "!" tile on the map. The extension refusal stops at (15,59), near the accepted route's far end, where extensions start.
  - Native TBM journeys `20261015-210001-165ca064a6` (UI, 212 assertions, `FEEDBACK_NEW_AREA extendable=true`) and `20261015-210012-7406a97590` (extension, 144).
- **Carry forward.**
  - (a) `[tbm-project]` "Project controls pause and resume without touching orders or phase" fails before and after Group 4: Pause on a project with a travel order moves it to `FollowOrders` (19) and bumps the revision. Group 4 does not touch that command path; leave it for R2DF2 task 10.3's triage.
  - (b) Two `[demo-fix-tbm-status]` cases rely on the demo's beside-exit request being refused. Group 8's producer makes such approaches legal, but the quote uses it only from Group 9 on (they still pass after Group 8). Group 9 must switch those cases to a request that stays impossible.
  - (c) Mock unit tests that post commands must unpause (`ExcavationFixture` pauses the game, and a posted Pause then waits). Planning an extension needs a main view (`UndergroundToolbarTestUI` plus `UndergroundTestViewport`).
- **Group 5 (2026-10-15), task 5.1 listing.** Before re-derivation, `convert_vehicle_art.py --envelopes` lists 432 oversize poses in 168 family/part/view/state groups, all in the bodies of models 01-04 (every state). The widest opaque pose per view (envelopes N/S 16x24, diagonals 24x24, E/W 32x24):

  | Family | N | NE | E | SE | S | SW | W | NW |
  | --- | --- | --- | --- | --- | --- | --- | --- | --- |
  | Deep cutter (01) | 19x31 | 28x32 | 35x20 | 32x28 | 17x32 | 31x27 | 35x19 | 27x32 |
  | Earth hopper (02) | 19x29 | 38x31 | 43x22 | 37x31 | 19x29 | 38x31 | 43x22 | 36x32 |
  | Rock hopper (03) | 16x32 | 33x30 | 42x22 | 32x30 | 16x32 | 33x30 | 43x21 | 32x30 |
  | Hauler (04) | 18x32 | 32x29 | 36x26 | 35x32 | 17x32 | 34x32 | 36x26 | 31x30 |

  The standard cutter (00) and both supports fit. The audit text is in `logs/demo-fixes/vehicle-spacing/envelopes-before-rederivation.txt`.
- **Group 5, tasks 5.2-5.3.**
  - **Refusal.** Every finished pose is measured (alpha >= 128) against its view's envelope. An oversize pose is refused with its family, part, state, frame and view. `multilayer_vehicle_art.py` checks this refusal for every re-derived family.
  - **Declaration.** The four native chassis sources (`deep_body_idle_frame0`, `vehicle_model-02/03/04_base_frame0`) declare `envelope_fit: rederive` with provenance in `vehicle-generation-record.json`. Component sources painted onto them inherit the declaration.
  - **Transform.** One transform per chassis, view and state scales the subject uniformly (BOX) about its unchanged ground contact and horizontal centre. Cue frames are scaled by the same transform, and inside the mapped equipment regions they take the scaled frame's pixels; everywhere else the frame is exactly the scaled chassis. The equipment-region and alpha checks therefore still hold. The report records the mapped regions.
  - **Review.** All views stay legible (`logs/demo-fixes/vehicle-spacing/envelope-review-before-after.png`), so no view was re-authored. Lengths and articulation are unchanged (D6).
- **Group 5, task 5.4 (M1 route on this machine).**
  - **Python.** `m1-pack-scope.json` pinned Python 3.10.0, which this machine does not have; it has 3.10.11. Before any art change, a probe that bypassed only the version guard reproduced the checked-in `map_layers_m1.png`/`.nfo`/header byte for byte, so the pin is now 3.10.11 (noted in the scope's `_documentation`).
  - **grfcodec.** The CMake cache pointed grfcodec/grfid/nforenum at `D:/Codez/Tools/grfcodec/...win64`, which does not exist here. It now points at `build/grfcodec-win32/grfcodec-6.2.0-windows-win32/bin` (6.2.0 banners), which rebuilt the checked-in `openttd.grf` from the old inputs with the identical SHA256 `7acec839…` and GRFID checksum `ad357ce6…`.
  - **Repack.** `convert_vehicle_art.py` (publish and check) and `pack_m1_art.py`. The range header is byte-identical (ranges, IDs and offsets preserved); the PNG and NFO change because the sheet reflows.
  - **Build proofs.** `multilayer_packed_build.py reproducible`: two clean conversions and two GRF builds are identical (GRF `b50a2544…`; the GRFID checksum is unchanged because container v2 checksums only the data section). The expected-sprite fixture was regenerated with `multilayer_packed_art_fixture.py`. `generator-free` then loads all 3,561 sprites pixel-exact. `pack_m1_art.py --bind-packed` binds them.
  - **Fixtures.** `multilayer_sprite_pack.py` corruption fixtures and the five model audits pass.
  - **Public manifest.** The converter's `bind_manifest` resets the vehicles' public `authoring.status`/`runtime_activated` to pending. These two fields were restored from the previous binding; the consumer and ranges are unchanged and the new payload is proven to load.
  - **Evidence.** The evidence selection was regenerated cold (`regenerate_art_proofs.py` initialize, focused, the six captures, finalize; run `20261015-211645-c24563c69c`). Its finalize stops in `activate_m1_art.py`, whose generic path needs `collector-checks` evidence that the previous selection lacked as well. The vent collector is bound only by `--collector-native`, so this break predates Group 5. Leave it for R2DF2 task 10.3's triage.
  - **Backups.** Everything replaced is in `logs/demo-fixes/vehicle-spacing/backup-before-group5/`.
- **Group 5, task 5.5 (`vehicle-spacing` run `20261015-213105-8764afc31c`, `--ticks 12000`).**
  - **Envelopes.** No pose exceeds its envelope at normal zoom.
  - **Straight track.** Neighbour overlap at the real 8-step pitch: hopper pairs 62-65 px (Group 1: 195-253), hauler-hopper 78, cutter-support 66 and Deep cutter-support 82.
  - **Curves.** The demo has no curved track, so 45-degree turns are measured on composites: 105-131 px for every coupling, the cutter's included.
  - **Real game.** Captures `08-moving-zoom0..5` show the moving cutter consist on its corridor at every zoom, with depth sorting and visible coupler gaps; compare with screenshot 04. `09-depot-and-list` shows both consists at normal depot and list widths.
- **Group 6 (2026-10-15/16).** What changed:
  - **Block boundary (6.2).** `IsLayerPortalOpenToNetwork` (new, `layer_portal_work.cpp`) is the one answer to "does this pair join the network?": date-available and Connected. `GetTileTrackStatus_LayerPortal` and the `LayerPortal` case of `ExploreSegment` both use it. A pair that is not open ends the block at each end. A train moving on that end still counts, parts standing aside in the magazine do not, and the far end belongs to the other layer's block. Connected pairs still cross as one block.
  - **Signal refresh (6.3).** The test showed it was needed: with a block signal in front of the door, the signal stayed green while the cutter moved on the key. The key's block is now refreshed when a train leaves its magazine (`CheckTrainStayInMagazine`), when its last part is stored outside the hangar branch (`RunLayerPortalMagazine`; `VehicleEnterDepot` already did this for hangars), and when a part crosses the shaft of a pair that is not open (`TrainController`: the front fills the block it enters, the last part frees the one it leaves). All three go through the new `UpdateLayerPortalSignals`. It finishes queued signal work first, because a TBM controller queues support-track updates inside the tick and `UpdateSignalsOnSegment` asserts an empty queue. The first version called `UpdateSignalsOnSegment` directly and `[tbm-hangar-*]` hit that assertion.
  - **Key reservation (6.4).** The test showed it was needed: once the consist was stored for an exchange, the key still held the reservation that brought it in. `RunLayerPortalMagazine` now releases it when the last part stands aside, unless another train still moves on the housing. Leaving the key through the shaft already cleared it.
  - **Path reservation on departure (6.3, second half).** Not changed. No case showed a defect: with path signals and with `reserve_paths` on, the ordinary train reserved through its path signal to the line end at the door, and the cutter never crosses a signal on its way to the shaft.
- **Group 6 evidence.**
  - **Unit test (6.6).** `[demo-fix-portal-signals]` (`src/tests/portal_signal_boundary.cpp`; 6 generated variants, 89,318 assertions): no signal, block signal or path signal, each with `reserve_paths` off and on. The pair has six cuts so the whole consist fits below; the exchange is stood in for by emptying the cutter. Before 6.3, the block variants failed the red check; before 6.4, every variant failed the key-reservation check. Now all pass.
  - **Neighbours.** These pass: `[portal-movement]` (the Connected pair still forms one block with both ends), `[portal-fleet-date]`, `[portal-tbm-access]`, `[portal-recovery]`, `[tbm-hangar-acceptance]`, `[tbm-selected-hangar]`, `[tbm-hangar-service]`, `[tbm-hangar-orders]`, `[portal-work-state]`, `[tbm-controls]`, `[tbm-hauler-cycle]` and all `[tbm-exchange-*]` except acceptance.
  - **Native R5 on the demo save (6.7).** `hauler-depot` run `20261015-220425-298fa44a13` (`--ticks 30000`). The loaded hauler (282 t) entered depot (13,0) at tick 9698 with the cutter at (13,21,1). It left 19 ticks later, at its first depot evaluation, with no Full segment. The checkpoint `hauler-departing.sav` was saved mid-departure. The cutter was later stored in its magazine at tick 19201, and `stored-consist.sav` was saved then. Both saves are in `mlm/reviews/demo-fixes/hauler-depot/20261015-220425-298fa44a13/saves/`.
  - **Reload (6.7).** Replay runs `20261015-220445-61f2d92277` and `20261015-220447-3bc5744845` (`--input <save> --replay 400`) load each checkpoint. They log the same 400 per-tick digests as the saving run: every train's tile, position, speed, track and load, plus the project's phase, blocked reason and cursor. The guards are identical too. Mid-departure, the depot tile and the four corridor tiles under the cutter are reserved and the key is not. With the consist stored, nothing in the world is reserved. Demolition and connection are both refused (`Conflict`) before and after reload. A real-time timer takes each checkpoint between two game loops, so no save holds half a tick.
  - **Stale.** The `forced-hauler` scenario reproduced the pre-fix hold. It now waits for a hold that no longer happens, so it only runs out its tick limit.
- **Pre-existing failures for R2DF2 task 10.3.**
  - **Confirmed by a baseline.** `[tbm-movement]` and `[tbm-travel-*]` fail identically with Group 6's behaviour reverted (baseline build, `logs/demo-fixes/group6-baseline/`):
    - "A TBM leaves its work hold for a real depot visit and returns to the face": `tbm_movement.cpp` line 327, all four sections;
    - "A started cutter drives to its face…", both bending sections: line 370, fewer exchange breakpoints than expected;
    - "Cancelling a consist that straddles the pair": line 155;
    - "A TBM holds at a junction…": line 1029.
  - **Not baselined, same cause.** These fail on the exchange-breakpoint precondition or on spare wagon capacity. That is planning output, fixed at publication before any train moves, and `excavation*.cpp` is untouched by this plan:
    - `[tbm-loop]` "A cutter bores on with the empties of a mid-route exchange" (line 1094) and "A project commissioned by command…" (line 1163);
    - `[tbm-exchange-acceptance]` lines 77, 133 and 187.
- **Group 6B (2026-10-16).**
  - **Test (6B.1).** `[demo-fix-cargo-transfer]` (`src/tests/cargo_partial_transfer.cpp`, 4 cases). The lists are built the way the game builds them: packets from different stations are appended as kept cargo, so `Append` does not merge them, then staged for delivery on an "Unload all" order. `Append` accepts only Keep or Load, and of the generic `Reassign` pairs only `<Deliver, Keep>` is instantiated. On the unfixed code both partial cases failed exactly as in the demo. The demo Earth hopper (62 t, 32 t refused) split off 67 t, and its 9 t packet wrapped to 65478. The packet-boundary and whole-amount cases passed before and after the fix.
  - **Fix (6B.2).** `VehicleCargoList::Reassign<Deliver, Transfer>` now splits off `sum - (Transfer + max_move)`, with a comment naming the partial caller. When the whole amount is reassigned, the split never runs, so that path is unchanged.
  - **Native (6B.3).** `unload-order` run `20261015-221356-d2bb8f2985` (`--ticks 40000`) applies the user's "Unload all, No loading". The observer's new `StepUnloadOrder` logs each hauler stop. At tick 9515 the hauler arrived at Invenville with 378 t. The receivers took 96 t, their room, and 282 t were transferred to the station. The hauler left empty and stopped there again at tick 10087 without an assertion. No part was ever above capacity. Group 1's run asserted in `Stage` at that stop.
  - **Neighbours.** `[layer-processor*]`, `[layer-freight-accounting]`, `[layer-baseline-receiver]`, `[tbm-conservation]` and the other `[spoil-receiver-*]` cases pass. Three cases fail identically with the fix reverted (baseline build): `[spoil-receiver-delivery]` lines 41, 42 and 90 and `[spoil-receiver-stall]` line 76. Their cut yields less spoil than they assume (108 t against more than 192 t), the same drift as the exchange-breakpoint failures. They are left for R2DF2 task 10.3.
  - **Saves already damaged (6B.4).** Saves that already hold wrapped packets (the user's `am1` and `autosave2`) are not repaired on load. After loading, the cache is rebuilt from the packets, so a wrapped packet (65,440 t, say) cannot be told apart from a real one by any rule the loader could check. Any "repair" would have to guess how much spoil was really aboard, which would invent or destroy cargo and break the conservation invariant (I-3). The fix only stops new damage. Group 1's tooling keeps those two saves out of movement scenarios, and the demo checkpoint `tbm-work-403.sav` holds no wrapped packet.
- **Group 7 (2026-10-16).**
  - **Cause (7.1).** The active `Obstruction` source is the cut participant: `AdvanceTBMFaceWork`, then `PrepareTBMCutParticipation`, then `PrepareTBMSupport`. That chain refuses the first cell (13,11,1) because it holds project 0's support record (`support->project != project.id`). Two conditions contribute. `RetireTBMProject` erases the project but leaves its records. The commission quote never applied the support rule, so it accepted the route. R9 candidates (a)-(d) stay refuted; see the R9 section.
  - **Decision on 7.2 (deviation, the user may revisit).** The plan said to hand the released pieces over as ordinary permanent rail, removing the records. That would give the player free permanent track: the r2tbmf release design charges `conversion_price` through `CmdConvertTBMSupport` and allows "no free footprint", and 7.2 itself asks to keep the payment guards. Instead the support planner treats a released record (its project retired, or finished and awaiting release) of the same company as the project's own. It reuses the piece or adds a junction piece, and the record stays temporary and paid on conversion. While the new project works, `IsTBMSupportInUse` protects those cells through its corridor, so conversion and removal still refuse. Orphan records in existing saves are released records, so loading needs no change. The `[demo-fix-tbm-obstruction]` reload section shows this.
  - **Quote agrees with the cuts (7.3).** `ClassifyTBMSupportCell` is the one per-cell rule. `PrepareTBMSupport` applies it cut by cut. `FindTBMSupportRouteBlocker` walks the whole remaining route with it, and the commission and extension quotes call it after the rail-turn check. A refusal marks its cell through the quote's failure tile. It reads "build that piece first" (`STR_ERROR_TBM_COMMISSION_SUPPORT_PIECE`) when the cell is the company's own permanent track lacking the piece, and "in the way" (`STR_ERROR_TBM_COMMISSION_SUPPORT`) otherwise. The cut keeps its own refusal as the safety net.
  - **Effect on existing tests.** `UndergroundDepotWorkflow` used to quote the further route first and then build its missing pieces. That quote now refuses (the cut would have refused there too). The new `FundFurtherRoute()` builds the pieces from the refused quote's planned route, the documented "pay for any additional track pieces, then refresh the quote" step. `StartFurther`, `[tbm-depot-recommission]` and `[tbm-planning-command]` call it first.
  - **Resuming (7.4).** No code change. The controller retries the face work every tick in Bore, and the cut checks everything again, so a cleared blocker resumes the cutter. A test proves it.
  - **Named blocker (7.5).** `FindTBMObstruction` (pure, display only) finds a train in the next cut's sweep, the cell the next cut's support may not use, or a train moving on either end of the pair. The last applies while the journey is stuck, or while the cutter waits in its magazine to travel. `GetTBMObstructionText` turns it into "Train 3 is in the way" or "the track or building at 13, 11 is in the way". The vehicle caption, the projects list and the portal window show it; otherwise the generic text stays.
- **Group 7 evidence.**
  - **Unit test.** `[demo-fix-tbm-obstruction]` (`src/tests/tbm_obstruction.cpp`, 4 cases) passes. With the released-record reuse and the quote check switched off, three cases fail: the cutter never gets past project 0's records, and the quote accepts the refused route. The key-guard case passes either way, as a guard that already existed should.
  - **Native R9.** `obstruction` run `20261015-224212-2c35b32b02` on the demo save: project 1 at the released portal reaches its first cut 82 ticks after commissioning (`reproduced=0`). Group 1's run had `reproduced=1`.
  - **Neighbours.** These pass: `[demo-fix-tbm-status]`, `[tbm-support]`, `[tbm-commission]`, `[tbm-area]`, `[tbm-planning-*]`, `[tbm-replanning-*]`, `[tbm-release-*]`, `[tbm-recovery]`, `[tbm-depot-recommission]`, `[tbm-depot-foundation]`, the `[underground-area-*]`/window/job cases, `[portal-window]`, `[tbm-controls]`, `[tbm-hauler-cycle]` and `[portal-recovery]`.
  - **Pre-existing, for R2DF2 task 10.3.** `[tbm-depot-cycle]` fails at `underground_depot_fixture.h` line 269, identically with every Group 6 and 7 behaviour switched off. The further job completes, then the finished cutter drives lost on the Surface line while the hauler waits in the Surface depot. The `[tbm-movement]`/`[tbm-loop]` failures are the ones listed under Group 6.

- **Group 8 (2026-10-16).** Design, deviations and measurements are in [r2df-b-approach-design.md](r2df-b-approach-design.md).
  - **Producer.** `PlanExcavationApproach` and `ExcavationApproachProposal` (`excavation.h`, `excavation_route.cpp`).
  - **Deviation: area approaches include turning space.** The sketch's "nearest cell" approach is empty for screenshot 05, because the exit cut already touches the strip; the strip lacks turning space. The producer first tries the existing authorization. If that cannot cover the floor, it covers it in the margin-grown box and returns every swept cell outside the base. Exact goals (tunnels) use one exact-goal search.
  - **Addition, later removed: `ExcavationPlanningLimits::spaced_bends`.** No two same-way bends in a row, for R8's sharp-bend finding. Group 9 removed it: on the demo it turned the beside-exit strip into a 23-cut loop. The support path now picks its side cells instead (see Group 9).
  - **Addition: `ExcavationRouteProposal::blocker`.** The refused cell nearest a failed target, so refusals name the building, claim or map edge rather than the box edge.
  - **Unchanged for existing callers (8.1).** `exact_goal` defaults to false and `spaced_bends` to off; `blocker` is only added. The planner, starter, area, coverage, commission and replanning tags give the same results and assertion counts as the baseline run before the change (`logs/demo-fixes/group8/baseline-tags.log`, `after-tags.log`).
  - **Floors away from the housing.** `MakeTBMWorkOutline` refuses them as disconnected; the producer accepts a base in pieces. Group 9's builder must pass it that way.
- **Group 8 evidence.**
  - **Unit test.** `[demo-fix-approach]` (`src/tests/excavation_approach.cpp`, 9 cases, 2,222 assertions, milliseconds) covers:
    - screenshot 05 (refused today with `OutsideAuthorization`, then covered with 9 charged cells);
    - a floor behind the housing, a diagonal offset and an exact goal;
    - every approach and coverage route under `forbid_90_deg`, which `CheckTBMSupportRailTurns` accepts (since Group 9's side-cell choice);
    - a building and another project's claim, each named;
    - the map edge, layer, locked Deep, unsafe limits, exhausted budget, a sideways or "open" solid start;
    - determinism and purity.
  - **Pre-existing, for R2DF2 task 10.3.** `[tbm-coverage]` fails at `tbm_coverage.cpp` line 236 before and after Group 8. It is the frozen-breakpoint check ("the wagons cannot take the third cut as well"), the same capacity drift as the `[tbm-loop]` and `[tbm-exchange-acceptance]` breakpoint failures.

- **Group 9 (2026-10-16).** Design and measurements are in [r2df-b-approach-design.md](r2df-b-approach-design.md).
  - **Builders (9.1, 9.4).**
    - `MakeTBMWorkOutlineWithApproach` builds floor, ring and housing without `MakeTBMWorkOutline`'s connectivity refusal, adds the approach, and returns one connected outline.
    - It quotes nothing, like `MakeTBMWorkOutline`: an extension's earlier work may be mid-cut, and quoting it refuses.
    - The pose overload of `MakeTBMTunnelOutline` builds an angled tunnel from an exact-goal approach. The internal tunnel branch now reaches an off-axis end by the exact-goal search instead of refusing.
  - **Quotes (9.2, 9.3, 9.5).**
    - The commission plans from the housing with the measured planning points. The extension plans from `ReplayExcavationRoute`'s pose after every accepted cut.
    - Approach refusals set the failure tile and list the whole floor as unreachable.
    - Both previews carry `approach_tiles` and `approach_estimate`, and both fingerprints mix their count and cost.
  - **Found natively: the straight exit.** At a housing already opened by a released project, the planner could turn on the first cut, where support has no path. A start pose whose whole trail stands on its own tile now leaves straight on.
  - **Found natively: the 90-degree rule.**
    - The demo forbids 90-degree turns. Group 8's spaced-bend rule turned the beside-exit strip into a 23-cut loop with 67 extra cells that crossed the housing, so it was removed.
    - `GetTBMWorkRailPath` now keeps its usual side cell unless that forms a forbidden 90-degree bend, then passes the other one.
    - A scratch enumeration shows a legal choice always exists, the greedy choice always finds it, and it never changes an already legal path (existing saves keep their support).
  - **Changed test expectations.**
    - `[underground-workflow-turn-refusal]` became `[underground-workflow-turns]`: the chamber is now accepted with drivable support.
    - `[tbm-planning-intent]`'s off-axis tunnel end is now an angled tunnel.
    - Two `[demo-fix-tbm-status]` cases now claim the request's last cell for another project, so the request stays impossible. A building on that cell is not enough: open floor counts as covered.
    - `ExcavationFixture::ClaimStraightWork` serves those cases and `[demo-fix-approach]`.
- **Group 9 evidence.**
  - **Unit tests.**
    - `[demo-fix-approach-tbm]` (5 cases) covers screenshot 05's commission quote with 90-degree turns allowed and forbidden: charged approach cells, bought for exactly the reviewed route.
    - It also covers a stale confirmation after one approach cell changes, the extension beside the route end, an angled tunnel, and an impossible request naming its claimed tile.
    - `[demo-fix-approach]` (9 cases) passes too.
  - **Native 9.7.** `approach` run `20261015-235953-0166785c88`:
    - Project 0 is released as in the demo, then the beside-exit request (13,11)-(20,11) is quoted with 7 cuts, 9 approach cells and £570 in all, with `forbid_90_deg` on.
    - It is confirmed in the palette, and the released hauler is assigned.
    - The cutter bends at cut 2 and the job completes at tick 23,653: floor 8/8 and approach 9/9 open, no consist part ever off usable rail.
    - Captures `05-approach-quote`, `06-approach-bend` and `07-area-done` are in `mlm/reviews/demo-fixes/approach/<run-id>/images/`.
  - **Extension refusal.** In the same run the extension from project 0's route end is refused at the map edge (`InvalidTile`): the 50-cut corridor ends facing the bottom edge, and the planner requires a straight first advance from an open start. This is the existing planner rule; the refusal is correct.
  - **Neighbours.** These pass:
    - `[excavation-route-*]`, `[excavation-starter]`;
    - `[tbm-commission]`, `[tbm-area]`, `[tbm-replanning-*]`, `[tbm-planning-*]`;
    - `[demo-fix-tbm-status]`, `[demo-fix-tbm-obstruction]`;
    - `[underground-area-*]`, `[underground-job-presentation]`, `[underground-window]`, `[underground-workflow-*]`;
    - `[tbm-support]`, `[tbm-recovery]`, `[tbm-route]`, `[tbm-engine]`;
    - `[tbm-depot-recommission]`, `[tbm-depot-foundation]`;
    - `[tbm-hauler-cycle]`, `[tbm-release-*]`, `[tbm-controls]`;
    - `[portal-window]`, `[portal-recovery]`.
  - **Pre-existing, for R2DF2 task 10.3.** `[tbm-acquisition]` fails at lines 55 and 168-181. Its first cut yields 72 t of Rock where the test needs more than 128 t, so the capacity refusals it expects never come. It quotes straight solid-start cuts through `QuoteExcavationCells`, untouched here: the same yield drift as the spoil-receiver and breakpoint failures. `[tbm-coverage]` line 236 as under Group 8.

- **Group 10 (2026-10-16).**
  - **Summary from the final route.** The approach summary covers the final route's swept cells beyond the base, not the approach proposal's authorization. The coverage route may not need every authorized approach cell, and an undug cell is never charged.
  - **Specific refusals.** Approach refusals now read as specific reasons, so the `[demo-fix-tbm-status]` extension case and `[demo-fix-approach-tbm]`'s impossible request expect "another company's or project's work" for the claimed cell.
  - **Row heights.** The palette's approach, time and floor rows take three lines; the time and floor rows were cut off before.
  - **Evidence.** `[demo-fix-approach]`, `[demo-fix-approach-tbm]` and `[demo-fix-approach-gui]` pass (16 cases), with the status, area, window, workflow-turns, commission, area, replanning and planning tags. The native `approach` run `20261016-001428-77d67fa2da` shows the full line.

- **Group 11 (2026-10-16).**
  - **Own claims in quotes.** `QuoteExcavationCells(tiles, model, own_job)` quotes the replanned job's own claimed cells as the rock (Excavating) or floor they are. The planner passes its continuation's ID everywhere it quotes. Extensions are unchanged: their own claims are always virtual floor.
  - **Partial work kept for an unchanged next cut.** This deviates from the sketch, which always reset it. Group 12's quote should state abandoned work only when `route.front() != job.route[cursor]`.
  - **For Group 12.4.** `PlanExcavationApproach` takes a continuing job by ID and uses its whole route as continuation. A replacement needs the completed prefix only, so give it the kept cut count, or a job copy. `SummarizeTBMApproach` quotes approach cells without an own job, and a replacement's approach may reuse dropped claimed cells, so pass the job there.
  - **Rig note.** `TBMWorkRig` needs `ExchangeAndReturn()` after every `BoreOneCut()`: it stands in for the drive onto the new support, so without it the next cut never completes. The wagons are then empty.
  - **Debug-only check.** The claim-rebuild assert in `ReplaceExcavationJobRemainder` compiles only in Debug (`_DEBUG`). R2DF2 task 10.3's Debug build exercises it.
  - **Pre-existing, for R2DF2 task 10.3.** `[excavation-outline-authorization]` fails at `excavation_outline.cpp` lines 56-57: it expects 6 t of Earth and 12 t of Rock per cell, while the halved yields (R2TBMF Group 5B) give 3 and 6. The same yield drift as `[tbm-acquisition]`. The other 37 cases of the route, cut, outline, starter, job, replanning and approach tags pass.

- **Group 12 (2026-10-16).**
  - **Face piece (found by the native run).**
    - The support piece on the face is laid by the previous cut, aimed at the next planned cut. After a parked change, TravelToFace found no piece towards the new next cut and held for ever with MissingSupport.
    - Fixed twice over. `GetTBMWorkRailPath` keeps the side cell a laid piece already leads to (so straight-laid faces take a straight-first bend). `PrepareTBMFaceSupport` adds a junction piece at the face when still needed, and refuses while a train stands on the face.
  - **Parking during AdvanceSupport (found by the native run).** Return and park pressed on the tick a cut completed left the cutter on its new dead-end cell for ever: `ChooseTBMReturnDirection` had not reversed it, and cutters wait at line ends. From Bore and AdvanceSupport the command now backs the consist out (`BackUpTBMConsist`), as the automatic return does.
  - **Fixture notes.**
    - `ChangeWorkRig` derives from `SpoilReceiverRig` because a real quote needs both receivers at the origin.
    - File-scope `TileXYL` constants are wrong in tests (evaluated before the map exists); use functions.
  - **Evidence.**
    - `[demo-fix-change-work]` and `[demo-fix-change-work-ui]` pass (9 cases), with 59 neighbouring GUI and planning cases and the support/controller tags.
    - Native `change` run `20261016-011136-dc49659e86` and its replays `20261016-011158-3e3bb223c2` and `20261016-011200-993765b610` (identical 400-tick logs).
  - **Pre-existing, for R2DF2 task 10.3 (unchanged with the Group 12 path rule switched off).**
    - `[tbm-corridor]` "Released support stays while a train stands on it" fails at `tbm_corridor.cpp` line 113: `IsTBMSupportInUse` is true for the released cell. Look at Group 7's released-support reuse first.
    - `[tbm-hangar-reuse]` fails at `tbm_hangar.cpp` line 419: the remote commission quote refuses with `STR_ERROR_TBM_COMMISSION_SUPPORT_PIECE`, task 7.3's route check finding permanent rail without the needed piece.
    - `[tbm-movement]` line 155 ("Cancelling a consist that straddles the pair") now passes: Cancel from the face backs the consist out.

## Handoff notes for R2DF2 task 11.3

- **From R2DF task 6.5 (Plan3 portals; signals, reservations and admission).** For `archi-map-layers.md`, Paid boring and real spoil haulage:
  - **Block boundary.** A rail pair joins the network only while `IsLayerPortalOpenToNetwork` holds. Routing (`GetTileTrackStatus_LayerPortal`) and signal blocks (`ExploreSegment`) both ask it. A Sealed or Commissioning pair, or a locked Deep pair, ends the signal block at each end, and a train below never holds a Surface depot or signal.
  - **Signal refresh.** Entering or leaving a magazine, and crossing the shaft of a closed pair, refresh the end's block (`UpdateLayerPortalSignals`), as a depot does.
  - **Key reservation.** A consist stored in its magazine releases the key's reservation.
  - **Admission.** Signals never guarded corridor admission and still do not. `VehicleEnterTile_LayerPortal` admits a train onto a Commissioning pair, and `GetLayerPortalAdmittedTrackdirs` shows it the track, only by the controller's rules:
    - the project's own cutter consist, by `CheckLayerPortalWorkAccess`;
    - its assigned hauler at the key of its Source bay, through the door only, by `IsTBMHaulerAdmittedAtBay`;
    - a train with a hangar order, by `CanTBMUseHangarPortal`.
  - **Admission evidence after 6.2.** An ordinary train still meets a line end at the door (`[tbm-movement]` "An ordinary train is turned back…" and `[demo-fix-portal-signals]`). `[portal-tbm-access]`, `[tbm-hauler-cycle]` and the straight `[tbm-loop]` cases still pass.
- **From R2DF Group 7 (Plan4 TBMs; support and quotes).** For `archi-map-layers.md`, Paid boring and real spoil haulage:
  - **Released support.** Support left by a retired or finished project of the same company is reused by the next project as its own; it stays temporary, and conversion stays paid. While a project works, its corridor protects the cells.
  - **One rule for quotes and cuts.** `ClassifyTBMSupportCell` is shared by `PrepareTBMSupport` (cut by cut) and `FindTBMSupportRouteBlocker`, which the commission and extension quotes call. A quote refuses with the cell marked rather than accept a route its cut would refuse.
  - **Named status.** "Something is in the way" names the train or tile when `FindTBMObstruction` can find it.
- **From R2DF Groups 8-9 (Plan2 geology, Plan4 TBMs; approaches and support).** For `archi-map-layers.md`, Paid boring and real spoil haulage, and the geology section:
  - **Approach producer.** `PlanExcavationApproach` returns the working space a floor needs beyond its floor, ring and housing: approach and turning cells, as explicit charged extra cells. It is pure and bounded (a 4-cell margin box), and it names the blocking tile when it refuses.
  - **Quotes.** The commission and extension quotes authorize floor, ring, housing and approach through one outline. A floor no longer has to touch its housing. Off-axis tunnel ends become angled tunnels. Fingerprints cover the approach.
  - **Quote line (Group 10).** The quote shows the approach (cells, price, spoils, cutter positions) on its own line. Approach refusals read as map edge, building, other work or too far.
  - **Straight exit.** A consist still standing in its housing, solid or opened, leaves straight through the door.
  - **Support side cells.** Where the rail type forbids 90-degree turns, a diagonal advance's support passes the side cell that avoids a 90-degree bend. Every 45-degree route has drivable support, and legal support keeps its cells.
- **From R2DF Groups 11-12 (Plan2 geology, Plan4 TBMs; change of work area).** For `archi-map-layers.md`, Paid boring and real spoil haulage, and the geology section, fold in the contract recorded in [r2df-b-approach-design.md](r2df-b-approach-design.md#change-of-work-area-groups-11-12):
  - kept prefix, dropped remainder, no pause, cursor-bound quote, unchanged phase;
  - `QuoteExcavationCells` own-job quoting;
  - laid support pieces kept by the path;
  - the face junction piece;
  - Return and park from the face backing out.
