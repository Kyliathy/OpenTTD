# October 28 merge: execution notes

@see [Part 1](m2o28-1-merge-oct28.md), [Part 2](m2o28-2-merge-oct28.md) and the [savegame architecture](../../architecture/savegame/archi-savegame.md). This is an execution record, not a mandatory reading queue: read a section only when its group needs it.

Disposable evidence lives in `test-cache/merge-oct28/` and `logs/merge-oct28/` (both git-ignored). Every file named here can be rebuilt with the command given next to it.

## Carry-forward notes

Short facts that later groups need, so a resumed session does not have to rediscover them:

- `SaveCapability` ends with `Count`; Group 9 appends before it, and `src/tests/save_lineage_policy.cpp` has a `static_assert` on `Count` that forces an expectation row per capability.
- The forecast tree changes whenever `mlm-mlv` gains a commit (conflict-marker labels and the plan files). Compare conflicted paths and block counts, not the tree hash.
- Disk is tight: about 8.4 GiB free at the start, with `build/` 8.2 GiB, `build-clang/` 2.7 GiB and `build-dedicated/` 6.1 GiB. Part 2 Group 31's "move the old tree aside" needs revisiting before it runs.

## Group 1

**1.1** `git fetch origin mlm-mlv-2sp mlm-mlv` (2026-10-28). `mlm-mlv` = `799d284d`, a descendant of `d49c3596` whose only changes are planning Markdown plus a comment-only `@see` update in `src/tests/multilayer/support/donor_source_stage_check.py` and `package_transfer.py` (repointed from the deleted m2o26 plan). `origin/mlm-mlv` = `d49c3596`; `origin/mlm-mlv-2sp` = `66713bdf`; merge base `9f36af95`. `git status --short` was empty. No drift.

**1.2** `git switch -c merge-2sp-oct28 mlm-mlv`. The plan files were already committed on `mlm-mlv` (`ce3f75a0`, `799d284d`), so no extra planning commit was needed. The branch has no upstream (`git rev-parse @{u}` fails), and none will be set.

**1.3** `python3 -I test-cache/merge-oct28/dispositions.py 9f36af95 mlm-mlv origin/mlm-mlv-2sp test-cache/merge-oct28/dispositions.tsv`. Result: 5,286 `D-only`, 182 `S-only`, 23 `shared-conflict`, 17 `shared-clean-review`; 6 S-only paths flagged `policy`. S has 128 additions, 94 modifications, 0 renames and 0 deletions. The raw name/status lists are `test-cache/merge-oct28/{d,s}-status.txt`.

**1.4** `git merge-tree --write-tree --name-only mlm-mlv origin/mlm-mlv-2sp > logs/merge-oct28/merge-tree.txt`, with block counts in `logs/merge-oct28/merge-tree-blocks.txt`: 23 paths, 42 blocks, matching the plan. The tree is now `4884d28f…` instead of the planning `d0486387…`. The 32 differing files are the plan files plus the 23 conflicted files, whose markers now say `mlm-mlv` instead of `HEAD`; no conflict content changed.

**1.5** `test-cache/merge-oct28/duplicate-scan.txt`:

| Scan | Result | Resolved in |
| --- | --- | --- |
| Symbols on one side only | `NativeCargoReferencePreview`, `HandleWindowOpeningHotkey`, `SelectMainViewportLayer` exist only in S; `NativeCargoOwnerPreview`, `ToggleHotkeyWindow`, `ShowMainViewportLayerInDisplayOrder` only in D. `GetSpaceSupplyLabel` and `hotkey_toggles_window` exist on both. | 4.5, 6.2-6.4, 5.1, 6.5 |
| Command 211 | D: `CancelLegacySurfaceSupplyOrder`; S: `SetSpaceMiningPolicy`. | 5.3 (S moves to 223) |
| S uses of names Group 6 removes | `toolbar_gui.cpp` (18), `toolbar_gui.h` (4), `map_layer_gui.cpp` (2), `window.cpp` (2), `map_layer_gui.h`, `window_func.h`, `tests/space_fixes_native.cpp`, `tests/multilayer/acceptance/space_fixes_checks.py` (1 each). | 6.2-6.7 |
| Hidden duplicates | `STR_CONFIG_SETTING_HOTKEY_TOGGLES_WINDOW` (+`_HELPTEXT`) and `gui.hotkey_toggles_window` merge twice without markers. The two layer tooltips appear twice only inside conflict blocks. `gui.scroll_mode`, `gui.show_date_in_logs`, `_video_hw_accel` and `_old_diff_custom` repeat in D already (version-ranged). | 6.5 |
| CMake | No source repeated inside one target; every S-added test `.cpp` is registered. | — |

**1.6** This file. Environment: CMake 3.28.3, Ninja 1.11.1, GCC 13.3.0, Clang 18.1.3, Python 3.13.17; no MSVC, no `pwsh`/`powershell`. m2o26's Windows outputs (`test-cache/merge-oct26/`, the staged donor build under `test-cache/m26s/`) do not exist on this machine; Group 2 rebuilds the reference here instead.

## Group 2

**2.1** `git archive origin/mlm-mlv-2sp | tar -x -C test-cache/merge-oct28/s-reference` (commit `66713bdf`, tree `079181de`). The export sits inside this checkout, so every build, test and harness command for it runs with `GIT_CEILING_DIRECTORIES=<repo>/test-cache/merge-oct28`; otherwise Git and CMake's version detection would read this repository's history and stamp the reference with the wrong revision.

**2.2** The r2lbf source hunks were applied in commit order (patches kept in `logs/merge-oct28/s-reference/port-<commit>.patch`). Four hunks were rejected, each for a reason that makes it unnecessary in S: `saveload.h` (S has no `SaveLoad::required_capability`), `surface_supply_order.cpp` twice (S's file has no `std::as_const`, so it needs no `<utility>`), and `tests/surface_supply_order_owner.cpp` (S's file has no `CargoPacket::MAX_COUNT` use). Two `68d0ef40` files do not exist in S. One harmless side effect: the `58c21e51` header hunk added an r2lbf `@see` to S's `surface_supply_order_owner.cpp` in the export only. The S-only R6 site `src/tests/space_return_surface.cpp:39` now copies the footprint before the range-for; Group 8 applies the same fix to the merged tree.

**2.3** GCC 13.3 RelWithDebInfo, `ninja -j4 -k 0 all openttd_space_fixes_test`: 1,542 edges, **0 failed**, 27 min 24 s (`logs/merge-oct28/s-reference/build-gcc.log`). Warnings: 344 `-Wmissing-field-initializers`, 306 `-Wrange-loop-construct`, 76 `-Wunused-function`, 29 `-Wsign-compare`, 13 `-Wmisleading-indentation`, 5 `-Wstringop-overflow`, 5 `-Wredundant-decls`, 1 `-Wmaybe-uninitialized`, 1 `-Wenum-compare`. No `-Wdangling-pointer` remains after the R6 fix.

**2.4** The export's own `native_build.prepare_runtime_assets` filled `build/baseset/` (all five `trg*.grf` present). `ctest -N`: 1,137 tests. `ctest -j4 --timeout 600`: **1,091 passed, 46 failed** in 15 s (`ctest-gcc.log`, `ctest-gcc.xml`). The four `regression_*` tests pass now that the default target is built.

**2.5** `openttd_space_fixes_test`: 245 cases, 510,652 assertions, **0 failures**, about 1 s (`space-fixes.xml`).

**2.6** Ledger: `test-cache/merge-oct28/s-reference-ledger.tsv`. All 46 failures are class **D-fixed**: each one's test file or fixture (`underground_depot_fixture.h`, `tbm_fixture.h`, the Tube environment) was changed on D after the base, by M2O23 (`99d8aa5a`, `0119308a`) or r2lbf, and S never touched any of them. No failure is in an S-only case; there are no S-Linux and no S-behavior failures. A fourth class, D-fixed, was needed beside the plan's three: the failures are base-era defects D already repaired, so for Group 8 they mean "expected to pass once merged".

**Disk:** after 2.3 only 2.3 GiB were free (the reference `build/` is 6.5 GiB). `build-dedicated/` (6.1 GiB of build outputs, only a generated `regression/regression.cfg` copy inside) was deleted; Part 2 Group 31 rebuilds it from scratch anyway. 8.3 GiB free afterwards.

## Group 3

All producers ran headless on Linux with the null video driver, from the export root, with `GIT_CEILING_DIRECTORIES` set as in Group 2. Driver logs are in `logs/merge-oct28/donor445/<run>-driver.log`; each native log and run directory stays in the export (`logs/space-fixes/<run>.log`, `test-cache/space-fixes/<run>/`).

**3.1** Export-only patch to `space_fixes_checks.py`: when `build/<Configuration>/openttd_space_fixes_test.exe` is absent, use `build/openttd_space_fixes_test` (Ninja is single-configuration). Group 8.1 applies the merged-tree form. Results, all exit 0: `--asteroid-preview` on 405 (137 assertions) and 418; `--cargo-origin` on 405 (3 packets, 2 natural; preview, unchanged and resave all true; 112 assertions) and 418 (16 packets; 406 assertions). Each `--cargo-origin` run wrote a fresh `cargo-origins-current.sav`.

**3.2** From retained `space-heavy-440.sav`: `edit` (2,901 assertions) → `continue` (7,486 assertions; before-departure, after-departure, retarget, return-travel and after-unload saves). Light role: `edit-light` (2,860) → `continue-light` (10,214). S's own record had 7,485 assertions for the heavy continuation; the one-assertion difference is in a fresh run, not a failure.

**3.3** `lifecycle` and `lifecycle-hidden` from `m28-continue-heavy/policy-before-departure.sav`: each 107,904 assertions, 1,596 ordinary opportunities, armed delay 1,446, one birth. This matches S's lifecycle record of 1,596 opportunities. All five phase saves are **byte-identical** between the visible and hidden runs. `prepare-high-id` from the after-credit save: 22,071 assertions; 65,526 declared historical turnovers (the declared-prefix limitation stays), then native birth 65536, 4 extracted units, 3,664 opportunities, funded countdown 2,221.

**3.4** `python3 -I test-cache/merge-oct28/collect_donor445.py test-cache/merge-oct28/s-reference test-cache/merge-oct28/donor445 66713bdf 079181de` copied **28 saves** (all `OTTX`, outer version **445**) with `test-cache/merge-oct28/donor445/manifest.json`: per save its run, producer source and arguments, output SHA256 and the executable SHA256 (`openttd_space_fixes_test` `d49e28c2…`). Nothing was written to the tracked retention journal.

**3.5** No producer failed and none needed a real video driver, so no Windows-handoff row comes from this group. These are contemporary reconstructions from retained 405/418/440 originals; historical S441-444 originals stay unavailable and unverified.

## Group 4

**4.1** `git merge --no-ff --no-commit origin/mlm-mlv-2sp` on `merge-2sp-oct28` (`logs/merge-oct28/merge-start.log`): 23 unmerged paths, 221 status entries (S's 222 paths minus the identical `clean.ps1`). The resolution helper `test-cache/merge-oct28/resolve.py <file> <block> <text>` replaces one numbered block and refuses a stale index; resolutions were applied from the last block upward.

**4.2** `saveload.h`: D's tail stays (`MergedLineage = 441`, then `MaxVersion`); the five incoming names follow `MaxVersion` with value `MaxVersion` and a comment naming Groups 9-22 and the "no real games" rule. `required_capability{}` kept. `saveload.cpp` gained `static_assert(SAVEGAME_VERSION == SaveLoadVersion::MergedLineage)`.

**4.3** `cargopacket_sl.cpp`, all five blocks as unions: D's four supply columns and S's two asteroid columns (still on the pending enumerator); `CheckCargoPacketSourceFields` runs D's checks and then S's `ValidateCargoPacketSourceSchema`; native and copied rows decode through `DecodeCargoPacketSource` (which already applies `IsValidCargoPacketSource`), then D's departure/link check; the copied descriptor carries both D's supply columns and S's `source_xy`/`source` columns.

**4.4** `cargopacket.h`: both friends (`AcceptLoadedSurfaceSupplyOrders`, `CAPAChunkHandler`) and `UpgradeLegacyAsteroidSource`.

**4.5** `NativeCargoReferencePreview` and `LoadCheckData::native_cargo_reference_rows` are gone; `NativeCargoOwnerPreview`/`native_cargo_owner_rows` serve both joins. S's bound check "Excessive native cargo meaning references" now guards the owner rows in `station_sl.cpp` and `vehicle_sl.cpp`. `vehicle_sl.cpp` keeps r2lbf's four instantiations and S's `CheckSpaceMiningContinuationHeader`. Discovery: two S tests (`asteroid_cargo_validation.cpp`, `asteroid_cargo_wire.cpp`) build `NativeCargoPacketPreview` positionally with the origin fields right after `feeder_share`, so the merged struct orders S's three fields before D's two supply fields (D only initializes the first two members positionally). S's `asteroid_cargo_validation.cpp` and `space_fixes_native.cpp` now use owner rows.

**4.6** `space_cargo_preview.cpp`: D's descent line, then S's orphan-asteroid loop and `return ValidateSpaceAsteroidCargoPreview(data);`, and S's two functions, which read `native_cargo_owner_rows`. A local `GetOptionalSpaceSupplyLabel` adapts D's `CT_INVALID` label API to S's optional-label semantics at its two call sites. The Group 20 note sits at the label preparation.

**4.7** Every Group 4 file: no markers, `git diff --check` clean, staged.

## Group 5

**5.1** D's `CargoLabel GetSpaceSupplyLabel(SpaceSupply)` kept, S's optional overload removed; the only S caller was the copied preview (4.6).

**5.2** `ValidateAsteroidPacketCustody(const AsteroidField &field)` (S's signature, declared in `asteroid_cargo.h`) now runs on `VisitNativeCargoPackets` with S's checks inside the visitor (canonical source, `ResolveAsteroidCargoOrigin`, valid cargo, Space-plane pickup, no first station, label match). The final pool loop also refuses any non-canonical source. S's hand-written owner walk is gone. The `ValidateSpaceVehicles` call already passed `field`.

**5.3** `SetSpaceMiningPolicy` appended after `SetTubeVehicleGeothermalPower` (223). `merged_command_compatibility.cpp`: `static_assert`s for 222/223 and the suffix expectation `+m441c224p`. S code uses the command only by name.

**5.4** `asteroid_gui.cpp`: `"space_inspector", 560, 380`. The r2lbf key `"space_survey"` was never released, so no `windows.cfg` migration; Group 40 updates `r2lbf-b-windows-handoff.md`.

**5.5** `d2sa-space-adventures.md`: one paragraph with both sides' facts; Group 40 rewords it for writer 446.

**5.6** Checked per file as each was staged.

## Group 6

**6.1-6.2** `toolbar_gui.h/.cpp`: D's `MTHK_SHOW_MAP_LAYER_*`, `MainToolbarEditorHotkeys::ShowMapLayer*` and `show_map_layer_*` INI names. The S `case` lines that merged into both `OnHotkey` bodies without markers (`MTHK_LAYER_*`, `MainToolbarEditorHotkeys::Layer*`, `HandleWindowOpeningHotkey(GameOptions)`) were removed by hand, as was a duplicated `@param` doc line.

**6.3** `ToggleHotkeyWindow` gained S's safeguards for every window-opening key: modal progress → handled, nothing happens; a modal window → it is raised; pinned or `NoClose` targets → raised, never closed. Discovery: S's handler also avoided **recreating** Game Options on a repeated F2 (D's path called `ShowGameOptions()`, which closes and rebuilds the window). `ShowOrRaiseGameOptions()` now raises an existing window or opens a new one, in both toolbars. `HandleWindowOpeningHotkey` was removed from `window.cpp` (taken from the index's merge result so no unrelated whitespace changed) and `window_func.h`.

**6.4** `SelectMainViewportLayer` removed from `map_layer_gui.h/.cpp`; `space_fixes_native.cpp:202` now checks `CHECK_FALSE(ShowMainViewportLayerInDisplayOrder(MAX_MAP_LAYERS))`.

**6.5** One `hotkey_toggles_window` member (D's comment plus the pinned/non-closable rule) and `escape_closes_window`; the unmarked second `[SDTC_BOOL] gui.hotkey_toggles_window` block in `gui_settings.ini` and S's unmarked string pair in `english.txt` were deleted; D's help text now names the raise-intact and pinned/non-closable behavior. The layer tooltips keep D's wording. No `STR_` ID is defined twice.

**6.6** D's Ctrl+Alt comment kept; S's `HotkeyList::Load` migration names (`crash_game`, `money`, `update_coordinates`) match D's global hotkeys. `space_fixes_checks.py` custom hotkey now writes `show_map_layer_space`.

**6.7** `git grep` finds the removed names only in planning documents. The seven header-only blocks were resolved by `test-cache/merge-oct28/resolve_headers.py` (D's description, every `@see` from both sides, plus an m2o28 line), with two duplicated `@see` lines removed afterwards.

## Group 7

**7.1** The 17 clean paths: the four CMake lists add only the incoming units and `openttd_space_fixes_test` (no repeats, checked in 1.5); `cargopacket.cpp` adds `UpgradeLegacyAsteroidSource`; `afterload.cpp` calls `NormalizeLegacyAsteroidCargoPackets` right after `AcceptLoadedAsteroidField` and `saveload.cpp` calls `NormalizeLegacyAsteroidCargoPreview`, both under the pending gate (so for every load, which is right for D441: it has no typed origins); `space_cargo_preview.h` declares the two incoming preview functions; `window.cpp`/`map_layer_gui.cpp` were edited in 6.3/6.4; `clean.ps1` is identical; the six docs wait for Group 40.

**7.2** Pending-gate sites: 32 in production (9 files) and 23 in tests (3 files), as forecast. S-only callers of changed APIs were adapted during Groups 4-6: the copied preview, two S tests on owner rows, the layer test, the harness hotkey name.

**7.3** `ninja -C build -j4 -k 0 openttd openttd_test openttd_space_fixes_test`: 1,187 edges in 31.6 min with **one** failure: `src/tests/transport_tube_provider_switch.cpp:265`, a D test that freezes `Commands::End == 223`. It now asserts `SetSpaceMiningPolicy == 223` and `End == 224`. The rebuild (`build-gcc-2.log`, 45 s) linked all three targets with 0 failures, including `openttd_test`'s POST_BUILD test discovery.

**7.4** No conflict markers anywhere; `git diff --cached --check` clean; `check_swallowed_code.py HEAD` found 0 suspicious lines across the whole merge diff.

**7.5** Merge commit `aa91b303`: first parent `a500f8ea` (descends from `d49c3596`), second parent `66713bdf`. Not pushed.
