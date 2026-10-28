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

## Group 8

**8.1** Commit `14a17b91`: the R6 footprint copy in `src/tests/space_return_surface.cpp`, and `native_executable(configuration, name)` in `space_fixes_checks.py` (multi-configuration `build/<config>/<name>.exe` first, else single-configuration `build/<name>`), now used by `space_fixes_checks.py`, `asteroid_lifecycle_work_checks.py`, `space_lifecycle_peer_checks.py` and `space_ordinary_checks.py`. Those harnesses import from `space_fixes_checks`, so run them with `python3 -B` (isolated `-I` mode drops the script directory from `sys.path`).

**8.2** GCC: `ninja -C build -j4 -k 0` (default target) 0 failed; `ctest -N` **1,496** tests (D's 1,361 plus 135 incoming `openttd_test` cases); `ctest -j4 --timeout 600` in 35 s: 1,493 passed, 3 failed (`logs/merge-oct28/baseline/ctest-gcc.{log,xml}`).

**8.3** Clang: `build-clang/` rebuilt with 0 failed in 25.4 min; no `-Wdangling-gsl`. CTest 1,496 tests: 1,495 passed, 1 failed in 32 s; it already included the 8.6 fixes. There are no GCC/Clang differences.

**8.4** `test-cache/linux-baseline/run_focused.sh build logs/merge-oct28/baseline/focused` (its list now includes `openttd_space_fixes_test`): all ten built; nine returned 0. `openttd_space_fixes_test` had 245 cases and 510,652 assertions; one case (2 assertions) failed, the class (b) case below. `openttd_tube_test` now passes as a whole executable. The runner deletes each executable after its run, so rebuild `openttd_space_fixes_test` before using the Space harnesses.

**8.5** Classification of every failure:

| Case | Class | Cause and owner |
| --- | --- | --- |
| `Mining continuation schema distinguishes genuine legacy absence` (`space_mining_wire.cpp:158-159`) | (b) pending-gate | Uses `SaveLoadVersion::AsteroidFieldLifecycle` as an "older than 443" era; under the gate both names equal `MaxVersion`. Group 18. |
| `Finite extraction preserves small tails full holds and original packet provenance` (`probe_extraction.cpp:171`) | (a) S-reference | S's typed origins (r2sff1 Group15) made the test's 16-bit `source.id` check stale; it failed in the S export too (Group 2 had wrongly put it among the D-fixed failures; the ledger is corrected). Fixed in 8.6. |
| `WindowDesc - ini_key validity` (`space_structure_gui.cpp:150`) | (a) S-reference | S gave the construction window the INI key `space_construction` with no sticky or default-size box. Fixed in 8.6. |
| Build failure `transport_tube_provider_switch.cpp:265` | (c) merge | D froze `Commands::End == 223`; fixed in Group 7 (now 224, with `SetSpaceMiningPolicy == 223`). |

**8.6** Commit `8635bd02`: `probe_extraction.cpp` checks `GetAsteroidOrigin()->body`; the construction window gains `WWT_DEFSIZEBOX`. `ctest -R "Finite extraction preserves|WindowDesc - ini_key validity|probe"`: 11/11 passed.

**8.7** **Pending ledger** (class (b); Part 1 must clear all of it by Group 22):

| Pending item | Evidence | Owner |
| --- | --- | --- |
| `Mining continuation schema distinguishes genuine legacy absence` | CTest (GCC, Clang) and `openttd_space_fixes_test` | Group 18 |
| Re-saving any world whose packets carry a typed asteroid origin refuses with "Invalid native cargo source for saving". Loading normalizes legacy asteroid cargo to typed origins, so every Space world with asteroid cargo is affected. | 8.8 `space-heavy-440.sav` load and advance | Groups 15 and 22 (writer 446 carries the typed-origin columns) |

The pure codec cases that pass a pending enumerator straight to a codec pass under the gate, as the plan expected.

**8.8** `python3 -B src/tests/multilayer/acceptance/space_ordinary_checks.py --source <save> --mode <load|advance> --run <id>` (real `build/openttd` with the null video driver, paused save, `quit`). Results: `underground/mine-haulage-403.sav` load: exit 0, saved. `space/space-factory-440.sav` load: exit 0, saved. `space/space-heavy-440.sav` load and advance: both load, then saving refuses (the pending item above), with no crash and no corrupt file. The first attempt exposed an S harness defect: `space_ordinary_checks.py` wrote its log without creating `logs/space-fixes/`. It now creates the directory, as its sibling does (commit `31cc118d`).

## Group 9

**9.1** Pending-gate burn-down. Each row names a site that still compares a pending `SaveLoadVersion` enumerator, which equals `MaxVersion` under the Group 4 gate. Its owning group replaces the comparison with a capability query and ticks the row here. Rebuild the list with `grep -rn "SaveLoadVersion::\(AsteroidCargoOrigins\|AsteroidFieldLifecycle\|SpaceMiningContinuation\|SpaceLegacyDeparture\|SpaceSurveyDeparture\)" src`. Line numbers are as of commit `ec5f4c18`.

| Done | File | Lines | Gate | Owner |
| --- | --- | --- | --- | --- |
| [ ] | `src/saveload/cargopacket_sl.cpp` | 163-164, 256-257 | CAPA `asteroid_body`/`asteroid_mineral` columns (native and copied descriptors) | 15 |
| [ ] | `src/saveload/cargo_source_sl.cpp` | 29, 59 | `DecodeCargoPacketSource` legacy-or-typed branch and the schema check | 15 |
| [ ] | `src/saveload/cargo_source_sl.h` | 48 | `IsValidCargoPacketSource` typed-origin acceptance | 15 |
| [ ] | `src/tests/asteroid_cargo_wire.cpp` | 25-107 (15 sites) | Pure codec cases that pass `AsteroidCargoOrigins` straight to the codec | 15 |
| [ ] | `src/tests/asteroid_cargo_native.cpp` | 215 | Copied-row source check at `AsteroidCargoOrigins` | 15 |
| [ ] | `src/saveload/afterload.cpp` | 3528 | Native legacy asteroid cargo normalization | 16 |
| [ ] | `src/saveload/asteroid_sl.cpp` | 105-106, 179 | ASTR wire era and SPKN lifecycle state | 17 |
| [ ] | `src/saveload/space_order_sl.cpp` | 48, 60-61, 177 | SPOR legacy departure, copied continuation and mining policies | 18 |
| [ ] | `src/saveload/space_mining_sl.cpp` | 62 | Continuation schema decoder | 18 |
| [ ] | `src/saveload/vehicle_sl.cpp` | 1136-1149, 1295 | VEHS `mission.continuation.*` columns and the missing-schema refusal | 18 |
| [ ] | `src/tests/asteroid_cargo_native.cpp` | 87 | Copied mission decision check at `SpaceMiningContinuation` | 18 |
| [ ] | `src/tests/space_mining_wire.cpp` | 157-162 | Continuation-schema absence case (the pending CTest failure) | 18 |
| [ ] | `src/saveload/saveload.cpp` | 3631 | Copied legacy asteroid cargo normalization during load-check | 21 (custody checks in 20) |
| [ ] | `src/saveload/saveload.h` | 525-535 | The five pending enumerators and their interim comment | 22 |
| [ ] | `src/saveload/saveload.cpp` | 105-107 | `static_assert(SAVEGAME_VERSION == MergedLineage)` becomes the 446 writer check | 22 |
| [ ] | `src/saveload/saveload.cpp` | `DoLoad` preflight (added in Group 13) | Pending refusal of admitted `SaveDialect::SpaceFixes` files (`TOO_NEW_SAVEGAME`, "admitted but not yet readable") | 21 (lift once 15-20 read every era; 442-445 also need 22's MaxVersion) |

These sites read the accepted dialect directly instead of a pending enumerator. Once the classifier admits S files, `SaveDialect::SpaceFixes` must reach them as the A-fixed supply wire:

| Done | File | Line | Current rule | Owner |
| --- | --- | --- | --- | --- |
| [ ] | `src/saveload/surface_supply_order_sl.cpp` | 120 | Fixed or tagged SSOR accepted only for `DestinationA` and `Merged` | 13 (classifier), 21 (load order) |
| [ ] | `src/saveload/surface_supply_order_sl.cpp` | 260 | `legacy_a_supply_wire = dialect == DestinationA` (copied preview) | 13, 20 |

**9.2** `src/saveload/save_lineage.h` appends `SaveDialect::SpaceFixes` (the S writer, 441-445 without MLLN) and five capabilities before `Count`: `AsteroidCargoOrigins`, `AsteroidFieldLifecycle`, `SpaceMiningContinuation`, `SpaceLegacyDeparture` and `SpaceSurveyDeparture`. It adds `CURRENT_MERGED_SAVE_VERSION = 446` and `CURRENT_MERGED_SAVE_LINEAGE_SCHEMA = 2` beside the frozen 441/schema 1 constants. They stay reserved: nothing reads them until Groups 13 and 22. No code switches over `SaveDialect`, so the append needs no other edits.

**9.3** `src/tests/save_lineage_policy.cpp`:

- `CAPABILITY_EXPECTATIONS` gains five `SpaceFixes` rows at 441-445, so its `static_assert` on `Count` holds.
- `SPACE_FIXES_MATRIX` writes the plan's seven-row matrix out literally.
- The new case `Save lineage freezes the Space fixes source-format matrix` checks the five columns per row. It also checks every older capability: merged sources carry all of A and B, S sources all of A and none of B.
- The merged-marker case now expects only the A/B capabilities, because merged 441 must lack the five.
- The new case uses `AcceptedSource(dialect, version)` because the classifier cannot produce `SpaceFixes` or 446 before Group 13.
- It is tagged `[!mayfail]` until Group 10.

Quick check without the native tree: `g++ -std=c++20 -Wall -Wextra -DSAVE_LINEAGE_STANDALONE -Isrc src/tests/save_lineage_policy.cpp src/saveload/save_lineage.cpp -o test-cache/merge-oct28/lineage/policy`, then run it (11 s to compile, no warnings). Result: 6 cases, 5 passed and 1 failed as expected (131 matrix assertions, as forecast: the positional rule gives merged 441 the five, and gives S sources nothing).

**9.4** The header comment on the reserved constants says that S's eras are capability rules of `SpaceFixes`, that `MergedLineage = 441` keeps its meaning, and that Group 22 deletes the pending enumerators. The comment on the frozen 441 constants, which still pointed at the old Group44 allocation plan, now says what 441 means.


## Group 10

**10.1** Commit `074285ff`. `HasSaveCapability` now uses one `CapabilityRule` row per capability: first A, first B and first Space fixes version, plus merged 441 and merged 446 membership. It looks the row up by capability, never by enum position.

- Two `static_assert`s, one on the table size and one on each value occurring exactly once, force a row for every future append. That row must choose its merged 441 value explicitly.
- `WriterProducedVersion` first checks each writer's closed range: A 425-440, B 425-434, Space fixes 441-445, merged exactly 441 or 446, Shared none. A version outside that range grants nothing.
- Space fixes rows give every A capability from 441, because that writer began from A440. They give no B capability.

**10.2** The Group 9 matrix case is no longer `[!mayfail]`. A new case covers:

- each Space fixes introduction at n-1/n;
- both 441 dialects;
- foreign versions (S446, merged 442/445/447, A441, B435, Shared, an undeclared dialect value);
- refused statuses;
- `Count` and 255 as unknown capabilities.

Standalone build: 7 cases, 1,459 assertions, all passed.

**10.3** `SlHasSaveCapability`'s writer context is now always `{Accepted, Merged, SAVEGAME_VERSION}`. The `DestinationA` fallback was dead code, because the Group 4 `static_assert` pins the writer to 441. So the five Space fixes capabilities stay off when saving until Group 22. The load context still answers from the bound lineage. `DestinationVersionCapability` is unchanged and has no pending enumerators.

**10.4** `ScopedLoadLineage` clears `_sl.lineage` on construction and destruction, and `SlNullPointers` clears it on failure cleanup. `Bind` stores the classifier result unchanged, so `original_version` stays the envelope version. `GetLoadCapabilities` returns that stored value.

GCC: `ninja -C build openttd_test openttd`, 299 steps in 11 min 49 s, exit 0. Its 308 warnings are all in files this group did not touch.

- `ctest -R "save-lineage|lineage"`: 20/20 passed. These are the 7 policy cases, the 4 native lineage cases and 9 utility-lineage cases (`logs/merge-oct28/group10/ctest-gcc-lineage.{log,xml}`).
- Wider save-load subset `-R "Original|Native scalar|supply wire|merged|Merged|Save envelope|copied|Copied"`: 53/53 passed (`ctest-gcc-saveload.log`).

Clang: `ninja -C build-clang openttd_test`, 298 steps in 9 min 45 s, exit 0. It printed 113 warnings, none in a file this group touched. Lineage subset 20/20 and save-load subset 53/53 passed (`logs/merge-oct28/group10/ctest-clang-{lineage,saveload}.log`). There are no GCC/Clang differences.

## Group 11

**11.1** Commit `463c6626`. `save_lineage.h` adds `SupplyColumnEncoding {Absent, LegacyU32Bool, CanonicalU64U8, Malformed}`. `SaveWireFacts` gains two fields with inert defaults: `cargo_supply_encoding` and `asteroid_origin_columns` (a `SaveEvidenceState`). `supply_columns` and `merged_cargo_valid` stay, now derived from the new observation:

- legacy pair → `supply_columns` Complete;
- malformed pair → `supply_columns` Malformed;
- canonical pair → `merged_cargo_valid`, and the merged packet columns become required.

The classifier's malformed-first rule now also refuses either new observation when it is Malformed.

**11.2** `CheckSelectedColumns` reads both pairs from the CAPA descriptor through `ObserveSupplyColumns` and `ObserveAsteroidOriginColumns`, never from `original_version == 441`.

- Native type codes: U32/I8 for legacy B, U64/U8 for merged, U32/U32 for the asteroid key.
- A half pair or any other widths is recorded as Malformed. It no longer throws, so the classifier refuses it as Corrupt.
- Duplicate names still throw in `ReadColumns`.
- Behaviour change in existing tests: three cases that expected a thrown `SaveFramingError` (half pair, mixed pair, legacy envelope with a merged marker) now check the Malformed fact and a Corrupt classification. Both paths refuse the load with `STR_GAME_SAVELOAD_ERROR_INVALID_LINEAGE`.

**11.3** `ReadRiff`'s MLLN case keeps `exact(8)`: a wrong length is still a framing error. After that it records the tuple. Only `(441, schema 1)` and `(446, schema 2)` with magic MLN1 and `writer == original_version` stay Valid; any other tuple marks the family Malformed. The envelope version is known before any chunk, so the result does not depend on chunk order.

The SSOR branch still chooses its decoder from `original_version == 441`. Group 12.1 removes that. So a synthetic S441 *with* SSOR is not yet scannable, and the Group 11 fixtures leave SSOR out.

**11.4** `src/tests/save_envelope_cargo_columns.cpp` (registered in `openttd_test` and `openttd_tube_test`) builds every stream byte by byte. It has four cases:

- each writer's columns and marker: D441, S441, S445, 446, legacy B434, empty versus populated CAPA, marker first versus last;
- malformed pairs and six bad marker tuples, plus a short marker as a framing error;
- exact work ceilings passing and one less failing for fields, rows and chunks, with every truncation of two streams refusing and rewinding;
- an optional real-save scan.

`Observe` checks that the reader is rewound, the input is unchanged and the digest equals SHA-256 of the input.

Results (GCC, `logs/merge-oct28/group11/`):

- `ninja -C build openttd_test` took 11 min 58 s, exit 0, with no warnings in the touched files.
- `ctest -R "Save envelope|Save lineage|Native lineage|Original|Native scalar|supply wire|merged|Merged|copied|Copied"`: 65/65 passed (`ctest-gcc.{log,xml}`).
- `OTTD_DONOR445_DIR=$PWD/test-cache/merge-oct28/donor445 build/openttd_test "Save envelope observes typed CAPA columns in real donor 445 saves"` scanned all 28 real S445 saves (169 assertions, `donor445-scan.log`). Each is version 445, with typed asteroid columns, no supply pair and no MLLN.

Clang is deferred to the Group 13 close, because each header change rebuilds about 300 objects.

## Group 12

**12.1-12.2** Commit `e185eb8c`. `ReadRiff`'s SSOR case calls `ObserveSupplyBook`, which never looks at the version or at MLLN.

- It reads a header-sized prefix (20 bytes, `MERGED_SURFACE_SUPPLY_HEADER_SIZE`).
- The fixed A440 shape fits when the little-endian count at bytes 12-13 is at most 16384 and the extent is exactly 14 + 53 * n.
- The SSM1 signature is the first word in network order. Only when it is present does the streaming `InspectMergedSurfaceSupplyOrders` run, with a reader that replays the prefix and then continues from the payload. No payload-sized buffer and no receipt vector exist.
- The result goes into `SaveWireFacts::supply_book_shape`:
  - both readings validate → `Ambiguous`;
  - only the tagged book validates → `Tagged`;
  - signature present but the book is invalid → `Malformed` (it never falls back to the fixed reading);
  - no signature and no fixed fit → `Malformed`;
  - otherwise → `Fixed`.
- `merged_supply_valid` and `merged_supply_next_id` are set whenever the tagged book validates.
- The classifier's malformed-first rule refuses `Malformed` and `Ambiguous`. The positive shape requirements (merged needs Tagged, A/S need Fixed) belong to Group 13.

**Risk:** a real merged book whose bytes also satisfy the fixed equation would be refused as Ambiguous. That needs `20 + 54a + 39b == 14 + 53n`, with n taken from the frontier's low 16 bits byte-swapped. It is rare, but possible for some frontiers. The fixture `OverlappingBook()` is one such case: 8 LegacyB rows with next ID 0x600. Group 13 decides whether a valid marker settles this overlap or the plan's refusal stands.

**12.3** Inventory of S-only schema changes, by family:

| Family | Era | Change |
| --- | --- | --- |
| CAPA | 441 | Adds `asteroid_body` and `asteroid_mineral` (U32). Observed in Group 11. |
| ASTR | 442 | Adds the 62-byte lifecycle trailer (`ASTEROID_LIFECYCLE_WIRE_BYTES`) and sparse body IDs. The era maximum comes from `GetAsteroidFieldWireMaximum`. |
| ASTR | 445 | Survey records may carry no target (ownerless return/waiting). |
| SPKN | 442 | Sorted, full-width sparse IDs, bounded by `MAX_ORBITAL_BODIES` instead of the dense population. |
| SPOR | 443 | Order rows grow from 14 to 15 bytes (the policy byte). |
| SPOR | 444 | Admits unqueued legacy departure. |
| VEHS | 443 | Adds the 14 `mission.continuation.*` columns. |
| SDEF | all | S writes content version 3, as A440 does. |

None of these is needed to decide the writer. Typed CAPA columns (Group 11) and the fixed SSOR (Group 12) identify S441-445 positively, and each family's era is validated by its native decoder once the source is known. So no ASTR/SPKN/SPOR/VEHS fact was added.

**12.4** VEHS is not inspected, so its conditional nested-schema path is not needed, and there are no non-Space vehicle cases to add.

**12.5** `src/tests/save_envelope_supply_book.cpp` holds five cases:

- fixed SSOR read by shape in A440, S441 (marker-free, CAPA first or last) and S445, with every truncation rewinding;
- tagged SSOR in D441 and 446 with the marker first or last, plus an empty tagged book;
- historical B `SSOH` + `SSOR` TABLE kept outside the RIFF shapes;
- signature collisions: fixed-shaped books starting with SSM1 become Malformed, and the true overlap becomes Ambiguous;
- refusals: too short, missing records, count 16385, a truncated or over-long tagged book (all Malformed), plus duplicate SSOR, an over-claimed RIFF length and SSOR as a sparse array (framing errors with rewind).

The byte-level builders now live in `src/tests/save_envelope_chunks.h`, shared with Group 11's file. `save_lineage_policy.cpp` refuses `Malformed` and `Ambiguous` on complete merged and A440 facts, and accepts `Tagged` and `Fixed` there.

Results (GCC, `logs/merge-oct28/group12/`):

- Build: 12 min 4 s, exit 0, no warnings in the touched files.
- The focused subset (the Group 11 pattern plus `Merged supply`): 70/70 passed (`ctest-gcc.{log,xml}`).
- With the donor scan extended to the SSOR shape, all 28 real S445 saves read as `Fixed` (197 assertions, `donor445-scan.log`).


## Group 13

**13.1-13.3** Commit `6cd8a5ba`. `ClassifySaveDialect` works in this order:

1. Malformed evidence refuses first: framing, common transport, Group 11's column observations, the `Malformed` book shape and every Malformed family.
2. Versions above 446 are `Unsupported`.
3. A present marker goes to `CompatibleMerged`. It needs exactly 441/schema 1 or 446/schema 2, with the writer equal to the envelope version, plus:
   - the canonical CAPA pair;
   - the tagged book (Tagged or Ambiguous);
   - no typed origins at 441 and typed origins at 446;
   - every retained family check from the old marker branch.
4. An unmarked 446 is Corrupt.
5. A markerless 441-445 goes to `CompatibleSpaceFixes` and is never given a hint. It needs:
   - typed CAPA origins;
   - the Fixed book as a RIFF family;
   - no B metadata bits, CAPA supply pair, B layout or merged book;
   - none of the seven B families;
   - content 0 or 3, with CDIM and SPRC present and `ValidSpaceFamilies`.
6. Everything else goes to `ClassifyLegacy`: the old A/B/shared body and its hint semantics. It now also refuses typed origins, the canonical pair or an SSM1 book at 440 and below. `CompatibleA`'s A440 rule also requires `SupplyBookShape::Fixed`.

The version constants moved to the top of `save_lineage.cpp`.

Plan change, recorded in 13.3: an SSOR overlap (`Ambiguous`) no longer always refuses. A marked file takes the tagged book; a markerless file refuses. Group 12's notes give the collision arithmetic. Always refusing would have made some genuine D441 saves unloadable.

**13.4** `DoLoad` binds the result as before. A new pending refusal follows the accepted-status check: `SaveDialect::SpaceFixes` refuses with `STR_GAME_SAVELOAD_ERROR_TOO_NEW_SAVEGAME` and "admitted but not yet readable". It is a burn-down row owned by Group 21. World initialization did not move.

Three real S445 saves from Group 3 are committed under `src/tests/fixtures/saveload/space-fixes-445/`: typed-origin packets, mining continuation and field lifecycle. `catalogue.json` records the writer, the producer runs and the SHA-256 of each file.

Tests:

- `save_lineage_policy.cpp` builds facts with `Merged446()` and `SpaceFixes(version)`. It covers:
  - both 441 writers, and each one's proof planted in the other's file;
  - S441-445 accepted, including Classic content 0 and hints ignored, with ten foreign-evidence refusals;
  - marked 446 accepted, with refusals for missing origins, the wrong schema, an unmarked file, markers on 442-445, and 447 as Unsupported;
  - the overlap settled by the marker;
  - the A442/A447 expectations updated to Corrupt/Unsupported.
  Standalone result: 10 cases, 1,537 assertions.
- `save_lineage_preflight.cpp` adds `Native preflight admits Space fixes originals and keeps them pending`. Each catalogued original:
  - has the catalogued digest;
  - classifies as SpaceFixes 445 (all five Space capabilities and all of A's, none of B's), and a DonorB hint is ignored;
  - is refused by the network reader, `Check` (`TOO_NEW_SAVEGAME`) and `Load` without one native read or a retained lineage.
  The open world (vehicle, money, map, tick, RNG) is unchanged.
- The optional donor scan also classifies each of the 28 real saves.

GCC results (`logs/merge-oct28/group13/`):

- Build: 12 min 3 s, exit 0.
- Focused subset: 74/74 (`ctest-gcc.{log,xml}`).
- A wider save/load subset `-R "[Ss]ave|[Ll]oad|[Pp]review|[Ll]ineage|[Oo]riginal|[Rr]etained|[Cc]opied|[Rr]eload|[Ss]pace fixes|[Aa]steroid"`: 369/369 (`ctest-gcc-wide.{log,xml}`).
- Donor scan: all 28 saves classify as SpaceFixes 445 (253 assertions, `donor445-scan.log`).
